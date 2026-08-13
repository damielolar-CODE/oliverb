// =============================================================================
//  OLIVERB — PluginEditor.cpp
//
//  Everything is laid out at a fixed design size and then scaled with an
//  AffineTransform, so the window can be dragged to any size without the
//  layout maths having to cope with it.
// =============================================================================

#include "PluginEditor.h"

using namespace wh::colours;

namespace
{
constexpr int kDesignW = 920;
constexpr int kDesignH = 792;
constexpr float kRotStart = juce::MathConstants<float>::pi * 1.20f;
constexpr float kRotEnd   = juce::MathConstants<float>::pi * 2.80f;
} // namespace

// =============================================================================
KnobBox::KnobBox (const juce::String& c, juce::Colour a) : caption (c), accent (a)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setRotaryParameters (kRotStart, kRotEnd, true);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 15);
    slider.setColour (juce::Slider::rotarySliderFillColourId, a);
    slider.setVelocityBasedMode (false);
    slider.setDoubleClickReturnValue (true, 0.0);
    addAndMakeVisible (slider);
}

void KnobBox::attach (juce::AudioProcessorValueTreeState& state, const juce::String& paramID)
{
    if (auto* p = state.getParameter (paramID))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        state, paramID, slider);
}

void KnobBox::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (15));
}

void KnobBox::paint (juce::Graphics& g)
{
    g.setColour (creamDim);
    g.setFont (OliverbLNF::faceFont (11.0f, true));
    g.drawText (caption.toUpperCase(), getLocalBounds().withHeight (14),
                juce::Justification::centred, false);
}

// =============================================================================
BigDial::BigDial (std::function<int()> bankProvider) : getBank (std::move (bankProvider))
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setRotaryParameters (kRotStart, kRotEnd, true);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setMouseDragSensitivity (170);
}

void BigDial::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    // Work backwards from the bounds: the frequency ring sits at 1.34 x radius
    // plus half a text box, and it must not clip.
    const float half = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float radiusFit = (half - 16.0f) / 1.42f;
    const float size = radiusFit * 2.0f;
    auto area = juce::Rectangle<float> (size, size)
                    .withCentre ({ bounds.getCentreX(), bounds.getCentreY() });

    const auto centre = area.getCentre();
    const float radius = size * 0.5f;
    const int bank = getBank ? getBank() : 0;
    const int steps = wh::PassiveHighPass::kNumSteps;

    // Detent marks + silkscreened frequencies
    g.setFont (OliverbLNF::faceFont (10.0f, false));
    for (int i = 0; i < steps; ++i)
    {
        const float t = static_cast<float> (i) / static_cast<float> (steps - 1);
        const float a = kRotStart + t * (kRotEnd - kRotStart);
        const float ca = std::cos (a - juce::MathConstants<float>::halfPi);
        const float sa = std::sin (a - juce::MathConstants<float>::halfPi);

        const bool selected = (juce::roundToInt (getValue()) - 1) == i;

        juce::Line<float> tick (centre.translated (ca * radius * 1.06f, sa * radius * 1.06f),
                                centre.translated (ca * radius * 1.17f, sa * radius * 1.17f));
        g.setColour (selected ? redBright : juce::Colour (0xff453c35));
        g.drawLine (tick, selected ? 2.6f : 1.6f);

        const float hz = wh::PassiveHighPass::stepFrequency (bank, i);
        const juce::String text = hz >= 1000.0f
                                      ? juce::String (hz / 1000.0f, 1) + "k"
                                      : juce::String (juce::roundToInt (hz));

        auto textArea = juce::Rectangle<float> (34.0f, 13.0f)
                            .withCentre (centre.translated (ca * radius * 1.42f,
                                                            sa * radius * 1.42f));
        g.setColour (selected ? cream : creamDim.withAlpha (0.75f));
        g.drawText (text, textArea, juce::Justification::centred, false);
    }

    const float t = (float) ((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
    OliverbLNF::paintKnobBody (g, area, red, kRotStart + t * (kRotEnd - kRotStart), true);

    // Corner frequency readout on the cap skirt
    const float hz = wh::PassiveHighPass::stepFrequency (bank, juce::roundToInt (getValue()) - 1);
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (OliverbLNF::faceFont (15.0f, true));
    g.drawText (hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + " kHz"
                              : juce::String (juce::roundToInt (hz)) + " Hz",
                juce::Rectangle<float> (110.0f, 18.0f)
                    .withCentre (centre.translated (0.0f, radius * 0.52f)),
                juce::Justification::centred, false);
}

// =============================================================================
LevelMeter::LevelMeter (OliverbProcessor& p) : proc (p) { startTimerHz (30); }

void LevelMeter::timerCallback()
{
    auto fall = [] (float display, float target)
    {
        return target > display ? target : display * 0.82f;
    };
    displayL = fall (displayL, proc.meterL.load());
    displayR = fall (displayR, proc.meterR.load());
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff100e0d));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (knobEdge);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);

    const float h = (r.getHeight() - 5.0f) * 0.5f;
    for (int i = 0; i < 2; ++i)
    {
        const float level = juce::jlimit (0.0f, 1.0f,
                                          juce::Decibels::gainToDecibels (i == 0 ? displayL : displayR,
                                                                          -48.0f) / 48.0f + 1.0f);
        auto bar = juce::Rectangle<float> (r.getX() + 2.0f,
                                           r.getY() + 2.0f + i * (h + 1.0f),
                                           (r.getWidth() - 4.0f) * level, h);
        juce::ColourGradient grad (green, r.getX(), 0.0f, red, r.getRight(), 0.0f, false);
        grad.addColour (0.72, brass);
        g.setGradientFill (grad);
        g.fillRect (bar);
    }
}

// =============================================================================
OliverbEditor::OliverbEditor (OliverbProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (content);

    auto makeKnob = [this] (std::unique_ptr<KnobBox>& k, const juce::String& caption,
                            const char* paramID, juce::Colour accent)
    {
        k = std::make_unique<KnobBox> (caption, accent);
        k->attach (proc.apvts, paramID);
        content.addAndMakeVisible (*k);
    };

    auto makeToggle = [this] (juce::ToggleButton& b, std::unique_ptr<ButtonAtt>& att,
                              const char* paramID)
    {
        content.addAndMakeVisible (b);
        att = std::make_unique<ButtonAtt> (proc.apvts, paramID, b);
    };

    // ---- Header -------------------------------------------------------------
    content.addAndMakeVisible (presetBox);
    presetBox.setTextWhenNothingSelected ("Presets");
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram())
            proc.setCurrentProgram (idx);
    };

    makeToggle (bypassButton, bypassAtt, pid::bypass);
    makeKnob (outputKnob, "Output", pid::outputGain, brass);

    meter = std::make_unique<LevelMeter> (proc);
    content.addAndMakeVisible (*meter);

    // ---- Filter -------------------------------------------------------------
    makeToggle (filterOnButton, filterOnAtt, pid::filterOn);
    makeToggle (typeButton, typeAtt, pid::filterType);
    makeToggle (postButton, postAtt, pid::filterPost);

    bigDial = std::make_unique<BigDial> ([this]
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (pid::filterType)))
            return c->getIndex();
        return 0;
    });
    content.addAndMakeVisible (*bigDial);
    bigDialAtt = std::make_unique<SliderAtt> (proc.apvts, pid::filterStep, *bigDial);

    makeKnob (impedance,  "Impedance", pid::impedance,  red);
    makeKnob (magnetism,  "Magnetism", pid::magnetism,  red);
    makeKnob (character,  "Character", pid::character,  red);
    makeKnob (dynamics,   "Dynamics",  pid::dynamics,   red);
    makeKnob (artefacts,  "Artefacts", pid::artefacts,  red);
    makeKnob (filterGain, "Gain",      pid::filterGain, red);

    // ---- Echo ---------------------------------------------------------------
    makeToggle (echoOnButton, echoOnAtt, pid::echoOn);
    makeToggle (syncButton, syncAtt, pid::echoSync);
    makeToggle (sendButton, sendAtt, pid::echoSend);

    content.addAndMakeVisible (divBox);
    divBox.addItemList (wh::divisionNames(), 1);
    divAtt = std::make_unique<ComboAtt> (proc.apvts, pid::echoDiv, divBox);

    makeKnob (echoTime, "Time",     pid::echoTime, brass);
    makeKnob (echoFb,   "Feedback", pid::echoFb,   brass);
    makeKnob (echoIn,   "Input",    pid::echoIn,   brass);
    makeKnob (echoOut,  "Output",   pid::echoOut,  brass);
    makeKnob (echoHiss, "Hiss",     pid::echoHiss, brass);
    makeKnob (echoMix,  "Mix",      pid::echoMix,  brass);
    makeKnob (echoAge,  "Wear",     pid::echoAge,  brass);

    // ---- Spring -------------------------------------------------------------
    makeToggle (springOnButton, springOnAtt, pid::springOn);
    makeKnob (springAmt,   "Spring",  pid::springAmt,   green);
    makeKnob (springDecay, "Tension", pid::springDecay, green);
    makeKnob (springDrive, "Drive",   pid::springDrive, green);

    // ---- Modulation ---------------------------------------------------------
    makeToggle (lfoOnButton, lfoOnAtt, pid::lfoOn);
    makeToggle (lfoSyncButton, lfoSyncAtt, pid::lfoSync);

    content.addAndMakeVisible (shapeBox);
    shapeBox.addItemList ({ "Sine", "Triangle", "Saw Down", "Square", "S+H" }, 1);
    shapeAtt = std::make_unique<ComboAtt> (proc.apvts, pid::lfoShape, shapeBox);

    content.addAndMakeVisible (lfoDivBox);
    lfoDivBox.addItemList (wh::divisionNames(), 1);
    lfoDivAtt = std::make_unique<ComboAtt> (proc.apvts, pid::lfoDiv, lfoDivBox);

    makeKnob (lfoRate,  "Rate",      pid::lfoRate,  juce::Colour (0xff7a6db0));
    makeKnob (lfoDepth, "LFO Depth", pid::lfoDepth, juce::Colour (0xff7a6db0));
    makeKnob (envDepth, "Env Depth", pid::envDepth, juce::Colour (0xff7a6db0));
    makeKnob (envSens,  "Sens",      pid::envSens,  juce::Colour (0xff7a6db0));
    makeKnob (envSpeed, "Speed",     pid::envSpeed, juce::Colour (0xff7a6db0));

    // Initial visibility (the timer keeps these in step afterwards)
    divBox.setVisible (syncButton.getToggleState());
    if (echoTime) echoTime->setVisible (! syncButton.getToggleState());
    lfoDivBox.setVisible (lfoSyncButton.getToggleState());
    if (lfoRate) lfoRate->setVisible (! lfoSyncButton.getToggleState());

    content.addAndMakeVisible (cornerReadout);
    cornerReadout.setJustificationType (juce::Justification::centredRight);
    cornerReadout.setFont (OliverbLNF::faceFont (12.5f));
    cornerReadout.setColour (juce::Label::textColourId, creamDim);

    content.setSize (kDesignW, kDesignH);
    layoutContent();

    setResizable (true, true);
    setResizeLimits (kDesignW / 2, kDesignH / 2, kDesignW * 3 / 2, kDesignH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) kDesignW / (double) kDesignH);
    setSize (kDesignW, kDesignH);

    startTimerHz (12);
}

OliverbEditor::~OliverbEditor()
{
    setLookAndFeel (nullptr);
}

void OliverbEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void OliverbEditor::timerCallback()
{
    const bool synced = syncButton.getToggleState();
    divBox.setVisible (synced);
    if (echoTime) echoTime->setVisible (! synced);

    const bool lfoSynced = lfoSyncButton.getToggleState();
    lfoDivBox.setVisible (lfoSynced);
    if (lfoRate) lfoRate->setVisible (! lfoSynced);

    bigDial->repaint();

    const juce::String chain = postButton.getToggleState()
                                   ? "IN  >  ECHO  >  SPRING  >  FILTER  >  OUT"
                                   : "IN  >  FILTER  >  ECHO  >  SPRING  >  OUT";
    cornerReadout.setText (chain + "\n18 dB per octave  /  2x oversampled",
                           juce::dontSendNotification);

    if (presetBox.getSelectedId() - 1 != proc.getCurrentProgram())
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

// =============================================================================
void OliverbEditor::layoutContent()
{
    auto full = juce::Rectangle<int> (0, 0, kDesignW, kDesignH).reduced (12, 10);

    // ---- Header -------------------------------------------------------------
    auto header = full.removeFromTop (58);
    auto headerRight = header.removeFromRight (330);

    outputKnob->setBounds (headerRight.removeFromRight (78).withSizeKeepingCentre (78, 58));
    headerRight.removeFromRight (8);
    meter->setBounds (headerRight.removeFromRight (86).withSizeKeepingCentre (86, 18));
    headerRight.removeFromRight (8);
    bypassButton.setBounds (headerRight.removeFromRight (86).withSizeKeepingCentre (86, 24));
    headerRight.removeFromRight (8);
    presetBox.setBounds (headerRight.removeFromRight (150).withSizeKeepingCentre (150, 24));

    full.removeFromTop (6);

    // ---- Filter panel -------------------------------------------------------
    auto filterPanel = full.removeFromTop (252);
    auto fp = filterPanel.reduced (22, 8).withTrimmedTop (20);

    auto dialArea = fp.removeFromLeft (250);
    bigDial->setBounds (dialArea.reduced (2));

    fp.removeFromLeft (10);

    auto filterSwitches = fp.removeFromBottom (30);
    filterSwitches.removeFromTop (6);
    filterOnButton.setBounds (filterSwitches.removeFromLeft (104).withHeight (24));
    filterSwitches.removeFromLeft (10);
    typeButton.setBounds (filterSwitches.removeFromLeft (104).withHeight (24));
    filterSwitches.removeFromLeft (10);
    postButton.setBounds (filterSwitches.removeFromLeft (104).withHeight (24));

    {
        KnobBox* row[] = { impedance.get(), magnetism.get(), character.get(),
                           dynamics.get(), artefacts.get(), filterGain.get() };
        const int n = 6;
        const int w = fp.getWidth() / n;
        for (int i = 0; i < n; ++i)
            row[i]->setBounds (fp.removeFromLeft (w).reduced (4, 0));
    }

    full.removeFromTop (10);

    // ---- Echo panel ---------------------------------------------------------
    auto echoPanel = full.removeFromTop (192);
    auto ep = echoPanel.reduced (22, 8).withTrimmedTop (20);

    auto echoSwitches = ep.removeFromBottom (30);
    echoOnButton.setBounds (echoSwitches.removeFromLeft (104).withHeight (24));
    echoSwitches.removeFromLeft (10);
    syncButton.setBounds (echoSwitches.removeFromLeft (104).withHeight (24));
    echoSwitches.removeFromLeft (10);
    sendButton.setBounds (echoSwitches.removeFromLeft (104).withHeight (24));
    echoSwitches.removeFromLeft (16);
    divBox.setBounds (echoSwitches.removeFromLeft (96).withHeight (24));

    {
        KnobBox* row[] = { echoTime.get(), echoFb.get(), echoIn.get(), echoOut.get(),
                           echoHiss.get(), echoMix.get(), echoAge.get() };
        const int n = 7;
        const int w = ep.getWidth() / n;
        for (int i = 0; i < n; ++i)
            row[i]->setBounds (ep.removeFromLeft (w).reduced (4, 0));
    }

    full.removeFromTop (10);

    // ---- Mod panel ----------------------------------------------------------
    auto modPanel = full.removeFromTop (106);
    auto mp = modPanel.reduced (22, 8).withTrimmedTop (20);

    auto modLeft = mp.removeFromLeft (124);
    lfoOnButton.setBounds (modLeft.removeFromTop (24).withWidth (104));
    modLeft.removeFromTop (6);
    lfoSyncButton.setBounds (modLeft.removeFromTop (24).withWidth (104));

    {
        KnobBox* row[] = { lfoRate.get(), lfoDepth.get(), envDepth.get(),
                           envSens.get(), envSpeed.get() };
        for (int i = 0; i < 5; ++i)
        {
            auto slot = mp.removeFromLeft (104).reduced (4, 0);
            row[i]->setBounds (slot);
            if (i == 0)
                lfoDivBox.setBounds (slot.withSizeKeepingCentre (88, 24));
        }
    }

    shapeBox.setBounds (mp.removeFromLeft (110).withSizeKeepingCentre (104, 24));

    full.removeFromTop (10);

    // ---- Spring panel -------------------------------------------------------
    auto springPanel = full;
    auto sp = springPanel.reduced (22, 8).withTrimmedTop (20);

    springOnButton.setBounds (sp.removeFromLeft (104).withSizeKeepingCentre (104, 24));
    sp.removeFromLeft (20);

    {
        KnobBox* row[] = { springAmt.get(), springDecay.get(), springDrive.get() };
        for (int i = 0; i < 3; ++i)
            row[i]->setBounds (sp.removeFromLeft (104).reduced (4, 0));
    }

    cornerReadout.setBounds (sp.reduced (16, 8));
}

void OliverbEditor::resized()
{
    const float scale = juce::jmin ((float) getWidth() / (float) kDesignW,
                                    (float) getHeight() / (float) kDesignH);
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, kDesignW, kDesignH);
}

// =============================================================================
void OliverbEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    // Faint horizontal brush texture across the faceplate
    juce::Random rng (0x5EED);
    g.setColour (juce::Colours::white.withAlpha (0.012f));
    for (int i = 0; i < 220; ++i)
    {
        const float y = rng.nextFloat() * (float) getHeight();
        g.drawHorizontalLine ((int) y, 0.0f, (float) getWidth());
    }

    const float scale = juce::jmin ((float) getWidth() / (float) kDesignW,
                                    (float) getHeight() / (float) kDesignH);
    juce::Graphics::ScopedSaveState save (g);
    g.addTransform (juce::AffineTransform::scale (scale));

    auto full = juce::Rectangle<int> (0, 0, kDesignW, kDesignH).reduced (12, 10);

    // Header text
    auto header = full.removeFromTop (58);
    g.setColour (cream);
    g.setFont (OliverbLNF::faceFont (27.0f, true));
    g.drawText ("OLIVERB", header.withHeight (32), juce::Justification::topLeft, false);

    g.setColour (red);
    g.setFont (OliverbLNF::faceFont (11.5f, true));
    g.drawText ("PASSIVE FILTER  /  TAPE ECHO  /  SPRING TANK",
                header.withTrimmedTop (32).withHeight (18), juce::Justification::topLeft, false);

    full.removeFromTop (6);

    OliverbLNF::paintPanel (g, full.removeFromTop (252).toFloat(), "Filter   -   Big Dial", red);
    full.removeFromTop (10);
    OliverbLNF::paintPanel (g, full.removeFromTop (192).toFloat(), "Echo   -   Two Track", brass);
    full.removeFromTop (10);
    OliverbLNF::paintPanel (g, full.removeFromTop (106).toFloat(),
                            "Mod   -   LFO / Envelope  >  Big Dial",
                            juce::Colour (0xff7a6db0));
    full.removeFromTop (10);
    OliverbLNF::paintPanel (g, full.toFloat(), "Spring   -   Tank", green);
}
