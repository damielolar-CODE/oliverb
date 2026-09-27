// =============================================================================
//  OLIVERB — PluginEditor.cpp — see PluginEditor.h
// =============================================================================
#include "PluginEditor.h"

using namespace juce;

namespace
{
    RangedAudioParameter& prm (OliverbProcessor& p, const char* id) { return *p.apvts.getParameter (id); }
    const Colour kAmber  { 0xffE8952E };
}

OliverbEditor::OliverbEditor (OliverbProcessor& p)
    : DxEditorBase (p, "Oliverb", String::fromUTF8 ("MK II \xc2\xb7 VALVE \xc2\xb7 PASSIVE FILTER \xc2\xb7 TAPE ECHO \xc2\xb7 SPRING TANK"),
                    Dx::Finish::oxblood()),
      proc (p),
      bypass      (prm (p, pid::bypass), "BYPASS", "BYPASS"),
      valveOn     (prm (p, pid::valveOn),   "VALVE", "VALVE"),
      valvePost   (prm (p, pid::valvePost), "AT OUTPUT", "AT INPUT"),
      valveEcho   (prm (p, pid::valveEcho), "ECHO AMP", "ECHO AMP"),
      valveDrive  (prm (p, pid::valveDrive), "DRIVE", DxKnob::Marconi),
      valveBias   (prm (p, pid::valveBias),  "BIAS",  DxKnob::Marconi),
      valveSag    (prm (p, pid::valveSag),   "SAG",   DxKnob::Marconi),
      valveTone   (prm (p, pid::valveTone),  "TONE",  DxKnob::Marconi),
      valveMix    (prm (p, pid::valveMix),   "MIX",   DxKnob::Marconi),
      filterOn    (prm (p, pid::filterOn),   "FILTER", "FILTER"),
      filterType  (prm (p, pid::filterType), "BANK B", "BANK A"),
      filterPost  (prm (p, pid::filterPost), "POST", "PRE"),
      bigDial     (prm (p, pid::filterStep), "FREQUENCY", DxKnob::Marconi),
      impedance   (prm (p, pid::impedance),  "IMPEDANCE", DxKnob::Marconi),
      magnetism   (prm (p, pid::magnetism),  "MAGNETISM", DxKnob::Marconi),
      character   (prm (p, pid::character),  "CHARACTER", DxKnob::Marconi),
      dynamics    (prm (p, pid::dynamics),   "DYNAMICS",  DxKnob::Marconi),
      artefacts   (prm (p, pid::artefacts),  "ARTEFACTS", DxKnob::Marconi),
      filterGain  (prm (p, pid::filterGain), "GAIN",      DxKnob::Marconi),
      echoOn      (prm (p, pid::echoOn),   "ECHO", "ECHO"),
      echoSync    (prm (p, pid::echoSync), "SYNC", "SYNC"),
      echoSend    (prm (p, pid::echoSend), "DUB SEND", "HELD"),
      echoTime    (prm (p, pid::echoTime), "TIME",     DxKnob::Marconi),
      echoFb      (prm (p, pid::echoFb),   "FEEDBACK", DxKnob::Marconi),
      echoIn      (prm (p, pid::echoIn),   "INPUT",    DxKnob::Marconi),
      echoOut     (prm (p, pid::echoOut),  "OUTPUT",   DxKnob::Marconi),
      echoHiss    (prm (p, pid::echoHiss), "HISS",     DxKnob::Marconi),
      echoMix     (prm (p, pid::echoMix),  "MIX",      DxKnob::Marconi),
      echoAge     (prm (p, pid::echoAge),  "WEAR",     DxKnob::Marconi),
      lfoOn       (prm (p, pid::lfoOn),   "LFO", "LFO"),
      lfoSync     (prm (p, pid::lfoSync), "SYNC", "SYNC"),
      lfoRate     (prm (p, pid::lfoRate),  "RATE",      DxKnob::Marconi),
      lfoDepth    (prm (p, pid::lfoDepth), "LFO DEPTH", DxKnob::Marconi),
      envDepth    (prm (p, pid::envDepth), "ENV DEPTH", DxKnob::Marconi),
      envSens     (prm (p, pid::envSens),  "SENS",      DxKnob::Marconi),
      envSpeed    (prm (p, pid::envSpeed), "SPEED",     DxKnob::Marconi),
      springOn    (prm (p, pid::springOn),    "SPRING", "SPRING"),
      springAmt   (prm (p, pid::springAmt),   "SPRING",  DxKnob::Marconi),
      springDecay (prm (p, pid::springDecay), "TENSION", DxKnob::Marconi),
      springDrive (prm (p, pid::springDrive), "DRIVE",   DxKnob::Marconi),
      outputKnob  (prm (p, pid::outputGain), "OUTPUT", DxKnob::Bakelite)
{
    panel.setBrand ("DEESTECH");

    for (auto* c : std::initializer_list<Component*> {
             &presetBox, &bypass,
             &valveOn, &valvePost, &valveEcho, &valveDrive, &valveBias, &valveSag, &valveTone, &valveMix, &tube, &tubeCaption,
             &filterOn, &filterType, &filterPost, &bigDial, &impedance, &magnetism, &character, &dynamics, &artefacts, &filterGain,
             &echoOn, &echoSync, &echoSend, &divBox, &divCaption, &echoTime, &echoFb, &echoIn, &echoOut, &echoHiss, &echoMix, &echoAge,
             &lfoOn, &lfoSync, &shapeBox, &shapeCaption, &lfoDivBox, &lfoDivCaption, &lfoRate, &lfoDepth, &envDepth, &envSens, &envSpeed,
             &springOn, &springAmt, &springDecay, &springDrive,
             &meterL, &meterR, &meterCaption, &chainCaption, &specCaption, &outputKnob })
        panel.addAndMakeVisible (c);

    // cap colours by module
    for (auto* k : { &valveDrive, &valveBias, &valveSag, &valveTone, &valveMix }) k->setCap (Dx::Cap::orange);
    for (auto* k : { &impedance, &magnetism, &character, &dynamics, &artefacts, &filterGain, &bigDial }) k->setCap (Dx::Cap::red);
    for (auto* k : { &echoTime, &echoFb, &echoIn, &echoOut, &echoHiss, &echoMix, &echoAge }) k->setCap (Dx::Cap::yellow);
    for (auto* k : { &lfoRate, &lfoDepth, &envDepth, &envSens, &envSpeed }) k->setCap (Dx::Cap::lilac);
    for (auto* k : { &springAmt, &springDecay, &springDrive }) k->setCap (Dx::Cap::green);

    // lamps: each module's own colour
    for (auto* t : { &valveOn, &valvePost, &valveEcho }) t->setLens (kAmber);
    for (auto* t : { &filterOn, &filterType, &filterPost }) t->setLens (Colour (0xffE0452F));
    for (auto* t : { &echoOn, &echoSync, &echoSend }) t->setLens (Colour (0xffE9C14A));
    for (auto* t : { &lfoOn, &lfoSync }) t->setLens (Colour (0xffA7B0F4));
    springOn.setLens (Colour (0xff4FC47A));
    bypass.setLens (Dx::Palette::lampRed);

    bigDial.setScaleSteps (wh::PassiveHighPass::kNumSteps);
    refreshDialScale();

    for (auto* l : { &meterL, &meterR }) { l->setFillFromLeft (true); l->setFullScaleDb (48.f); l->setCount (20); }

    presetBox.setTextWhenNothingSelected ("Presets");
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram())
            proc.setCurrentProgram (idx);
    };

    divBox.addItemList (wh::divisionNames(), 1);
    divAtt = std::make_unique<ComboAtt> (proc.apvts, pid::echoDiv, divBox);
    shapeBox.addItemList ({ "Sine", "Triangle", "Saw Down", "Square", "S+H" }, 1);
    shapeAtt = std::make_unique<ComboAtt> (proc.apvts, pid::lfoShape, shapeBox);
    lfoDivBox.addItemList (wh::divisionNames(), 1);
    lfoDivAtt = std::make_unique<ComboAtt> (proc.apvts, pid::lfoDiv, lfoDivBox);

    {
        dtupdate::Style st;
        st.pillFill = kAmber; st.pillText = Colours::white;
        st.panelFill = Colour (0xff1B1110); st.panelText = Colour (0xffE6D6C8); st.panelDim = Colour (0xff9C8579); st.accent = kAmber;
        st.font = Fonts::engraved (11.f);
        st.corner = 3.f;
        updatePill = std::make_unique<dtupdate::UpdatePill> ("oliverb", JucePlugin_VersionString, st);
        updatePill->onVisibilityChange = [this] { resized(); };
        panel.addChildComponent (*updatePill);
    }

    setMinZoom (0.6f);
    setBaseSize (kWidth, kHeight);
    tick();
}

OliverbEditor::~OliverbEditor() = default;

void OliverbEditor::refreshPresetBox()
{
    presetBox.clear (dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
}

void OliverbEditor::refreshDialScale()
{
    int bank = 0;
    if (auto* c = dynamic_cast<AudioParameterChoice*> (proc.apvts.getParameter (pid::filterType))) bank = c->getIndex();
    if (bank == lastBank) return;
    lastBank = bank;
    StringArray labels;
    for (int i = 0; i < wh::PassiveHighPass::kNumSteps; ++i)
    {
        const float hz = wh::PassiveHighPass::stepFrequency (bank, i);
        labels.add (hz >= 1000.f ? String (hz / 1000.f, 1) + "k" : String (roundToInt (hz)));
    }
    bigDial.setScaleLabels (labels);
    bigDial.setScaleSteps (wh::PassiveHighPass::kNumSteps);
}

void OliverbEditor::tick()
{
    // sync swaps TIME / RATE for their note divisions
    const bool synced = echoSync.isOn();
    divBox.setVisible (synced); divCaption.setVisible (synced);
    echoTime.setVisible (! synced);
    const bool lfoSynced = lfoSync.isOn();
    lfoDivBox.setVisible (lfoSynced); lfoDivCaption.setVisible (lfoSynced);
    lfoRate.setVisible (! lfoSynced);

    refreshDialScale();

    auto db = [] (float v) { return v > 1.0e-5f ? jmax (0.f, 48.f + 20.f * std::log10 (v)) : 0.f; };
    meterL.setDb (db (proc.meterL.load()));
    meterR.setDb (db (proc.meterR.load()));
    tube.setGlow (valveOn.isOn() ? jlimit (0.f, 1.f, 0.25f + 0.75f * proc.valveGlow.load()) : 0.f);

    StringArray chain { "IN" };
    const bool vOn = valveOn.isOn(), vOut = valvePost.isOn(), fPost = filterPost.isOn();
    if (vOn && ! vOut) chain.add ("VALVE");
    if (! fPost) chain.add ("FILTER");
    chain.add ("ECHO");
    chain.add ("SPRING");
    if (fPost) chain.add ("FILTER");
    if (vOn && vOut) chain.add ("VALVE");
    chain.add ("OUT");
    const auto chainText = chain.joinIntoString (String::fromUTF8 ("  \xe2\x80\xba  "));
    if (chainText != lastChainText) { lastChainText = chainText; chainCaption.setText (chainText); }
    specCaption.setText (String::fromUTF8 ("18 dB / OCT  \xc2\xb7  2X OVERSAMPLED"));

    if (presetBox.getSelectedId() - 1 != proc.getCurrentProgram())
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
}

void OliverbEditor::resized()
{
    DxEditorBase::resized();

    // header: presets + BYPASS (right), the UPDATE pill left of them
    const auto t = panel.toggleArea();
    bypass.setBounds (t.withSizeKeepingCentre (84, RackPanel::kToggleH).withRightX (t.getRight()));
    presetBox.setBounds (bypass.getX() - 10 - 200, t.getY(), 200, RackPanel::kToggleH);
    if (updatePill != nullptr)
    {
        const int w = jmax (96, updatePill->preferredWidth (20));
        updatePill->setBounds (presetBox.getX() - 10 - w, t.getCentreY() - 10, w, 20);
    }

    auto r = panel.contentArea().withTrimmedTop (6);
    auto rowV = r.removeFromTop (150); r.removeFromTop (10);
    auto rowF = r.removeFromTop (250); r.removeFromTop (10);
    auto rowE = r.removeFromTop (150); r.removeFromTop (10);
    auto rowM = r.removeFromTop (150); r.removeFromTop (10);
    auto rowMaster = r;
    auto modS = rowM.removeFromLeft (rowM.getWidth() * 60 / 100); rowM.removeFromLeft (10);
    auto springS = rowM;
    panel.setSections ({ { rowV, "Valve" }, { rowF, String::fromUTF8 ("Filter  \xc2\xb7  big dial") }, { rowE, String::fromUTF8 ("Echo  \xc2\xb7  two track") },
                         { modS, String::fromUTF8 ("Mod  \xc2\xb7  LFO / envelope") }, { springS, "Spring tank" }, { rowMaster, "Master" } });

    auto buttonColumn = [] (Rectangle<int> col, std::initializer_list<Component*> buttons, int bw = 104)
    {
        const int n = (int) buttons.size(), h = 26, gap = 8;
        auto b = col.withSizeKeepingCentre (bw, n * h + (n - 1) * gap);
        for (auto* c : buttons) { c->setBounds (b.removeFromTop (h)); b.removeFromTop (gap); }
    };
    auto knobRow = [] (Rectangle<int> a, std::initializer_list<DxKnob*> knobs, int px)
    {
        const int n = (int) knobs.size(), slot = a.getWidth() / jmax (1, n);
        int i = 0;
        for (auto* k : knobs) k->placeIn (a.withX (a.getX() + i++ * slot).withWidth (slot), px);
    };

    // VALVE
    {
        auto a = rowV.reduced (10).withTrimmedTop (10);
        buttonColumn (a.removeFromLeft (124), { &valveOn, &valvePost, &valveEcho });
        auto tubeCol = a.removeFromRight (90);
        tubeCaption.setBounds (tubeCol.removeFromBottom (16));
        tube.setBounds (tubeCol);
        knobRow (a, { &valveDrive, &valveBias, &valveSag, &valveTone, &valveMix }, 44);
    }
    // FILTER
    {
        auto a = rowF.reduced (10).withTrimmedTop (10);
        buttonColumn (a.removeFromRight (124), { &filterOn, &filterType, &filterPost });
        bigDial.placeIn (a.removeFromLeft (270), 120);
        auto top = a.removeFromTop (a.getHeight() / 2);
        knobRow (top, { &impedance, &magnetism, &character }, 44);
        knobRow (a, { &dynamics, &artefacts, &filterGain }, 44);
    }
    // ECHO
    {
        auto a = rowE.reduced (10).withTrimmedTop (10);
        buttonColumn (a.removeFromLeft (124), { &echoOn, &echoSync, &echoSend });
        knobRow (a, { &echoTime, &echoFb, &echoIn, &echoOut, &echoHiss, &echoMix, &echoAge }, 44);
        const auto tb = echoTime.getBounds();
        divCaption.setBounds (tb.withHeight (16).withY (tb.getY()));
        divBox.setBounds (tb.withSizeKeepingCentre (jmin (tb.getWidth() - 8, 96), 26));
    }
    // MOD
    {
        auto a = modS.reduced (10).withTrimmedTop (10);
        buttonColumn (a.removeFromLeft (100), { &lfoOn, &lfoSync }, 86);
        auto shapeCol = a.removeFromRight (104);
        auto sc = shapeCol.withSizeKeepingCentre (shapeCol.getWidth(), 16 + 26);
        shapeCaption.setBounds (sc.removeFromTop (16));
        shapeBox.setBounds (sc.removeFromTop (26).reduced (4, 0));
        knobRow (a, { &lfoRate, &lfoDepth, &envDepth, &envSens, &envSpeed }, 38);
        const auto rb = lfoRate.getBounds();
        lfoDivCaption.setBounds (rb.withHeight (16));
        lfoDivBox.setBounds (rb.withSizeKeepingCentre (jmin (rb.getWidth() - 6, 84), 26));
    }
    // SPRING
    {
        auto a = springS.reduced (10).withTrimmedTop (10);
        buttonColumn (a.removeFromLeft (100), { &springOn }, 86);
        knobRow (a, { &springAmt, &springDecay, &springDrive }, 40);
    }
    // MASTER
    {
        auto a = rowMaster.reduced (12).withTrimmedTop (12);
        auto meters = a.removeFromLeft (280);
        auto m = meters.withSizeKeepingCentre (meters.getWidth() - 10, 12 + 8 + 12 + 16);
        meterL.setBounds (m.removeFromTop (12)); m.removeFromTop (8);
        meterR.setBounds (m.removeFromTop (12));
        meterCaption.setBounds (m.removeFromTop (16));
        outputKnob.placeIn (a.removeFromRight (150), 46);
        auto c = a.withSizeKeepingCentre (a.getWidth(), 18 + 16);
        chainCaption.setBounds (c.removeFromTop (18));
        specCaption.setBounds (c.removeFromTop (16));
    }
}
