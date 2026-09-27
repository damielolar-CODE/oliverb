#include "DxWidgets.h"

using namespace juce;
using P = Dx::Palette;

namespace
{
    constexpr float kPi = MathConstants<float>::pi;
    constexpr float kTwoPi = MathConstants<float>::twoPi;
    constexpr float kKnobA0 = kPi * 1.25f, kKnobA1 = kPi * 2.75f;   // 7:30 -> 4:30, like the phone knob

    Point<float> polar (float cx, float cy, float juceAngle, float radius)
    {
        // JUCE rotary convention: 0 = 12 o'clock, clockwise
        return { cx + std::sin (juceAngle) * radius, cy - std::cos (juceAngle) * radius };
    }

    // Static backdrops (wood, plates, meter faces) are rendered once per size/scale into an image at the
    // physical pixel scale, so the blur-based shadows never run at meter rate and 2x / host zoom stay crisp.
    void paintCached (Graphics& g, Rectangle<int> bounds, bool opaque, Image& image, float& scale,
                      const std::function<void (Graphics&)>& render)
    {
        const float s = g.getInternalContext().getPhysicalPixelScaleFactor();
        const int w = jmax (1, roundToInt ((float) bounds.getWidth() * s));
        const int h = jmax (1, roundToInt ((float) bounds.getHeight() * s));

        if (image.isNull() || image.getWidth() != w || image.getHeight() != h || std::abs (s - scale) > 1.0e-3f)
        {
            image = Image (opaque ? Image::RGB : Image::ARGB, w, h, ! opaque);
            scale = s;
            Graphics ig (image);
            ig.addTransform (AffineTransform::scale ((float) w / (float) bounds.getWidth(), (float) h / (float) bounds.getHeight()));
            render (ig);
        }

        g.drawImageTransformed (image, AffineTransform::scale ((float) bounds.getWidth() / (float) w,
                                                              (float) bounds.getHeight() / (float) h), false);
    }

    String paramText (const RangedAudioParameter& p, float normalised, const String& unit)
    {
        String t = p.getText (jlimit (0.f, 1.f, normalised), 0);
        if (unit.isNotEmpty() && ! t.containsIgnoreCase (unit))
            t << " " << unit;
        return t;
    }
}

// ============================================================================
// DxLookAndFeel
// ============================================================================
DxLookAndFeel::DxLookAndFeel()
{
    const Colour menuBg (0xff211D1A), menuOutline (0xff3E3A36), textCream (0xffE9E8E6);

    setColourScheme ({ P::ink, Colour (0xff14110F), menuBg, Colour (0xff3E3A36), textCream, Colour (0xff3E3A36),
                       Colours::white, P::orange, textCream });

    setColour (Slider::rotarySliderFillColourId,        P::orange);
    setColour (Slider::rotarySliderOutlineColourId,     P::greyMid);
    setColour (Slider::thumbColourId,                   Colour (0xffCBCBCE));
    setColour (Slider::trackColourId,                   P::orange);
    setColour (Slider::backgroundColourId,              Colour (0xff100E0C));
    setColour (Slider::textBoxTextColourId,             textCream);
    setColour (Slider::textBoxBackgroundColourId,       Colour (0xff14110F));
    setColour (Slider::textBoxOutlineColourId,          menuOutline);

    setColour (TextButton::buttonColourId,              Colour (0xff3A3A40));
    setColour (TextButton::buttonOnColourId,            P::orange);
    setColour (TextButton::textColourOffId,             textCream);
    setColour (TextButton::textColourOnId,              Colours::white);

    setColour (ToggleButton::textColourId,              P::greyLight);
    setColour (ToggleButton::tickColourId,              P::orange);

    setColour (Label::textColourId,                     textCream);
    setColour (Label::backgroundColourId,               Colours::transparentBlack);
    setColour (Label::outlineColourId,                  Colours::transparentBlack);

    setColour (ComboBox::backgroundColourId,            Colour (0xff14110F));
    setColour (ComboBox::textColourId,                  textCream);
    setColour (ComboBox::outlineColourId,               menuOutline);
    setColour (ComboBox::arrowColourId,                 P::orange);
    setColour (ComboBox::focusedOutlineColourId,        P::orange);

    setColour (PopupMenu::backgroundColourId,           menuBg);
    setColour (PopupMenu::textColourId,                 textCream);
    setColour (PopupMenu::headerTextColourId,           P::greyLight);
    setColour (PopupMenu::highlightedBackgroundColourId, P::orange);
    setColour (PopupMenu::highlightedTextColourId,      Colours::white);

    setColour (TooltipWindow::backgroundColourId,       Colour (0xff2A2622));
    setColour (TooltipWindow::textColourId,             textCream);
    setColour (TooltipWindow::outlineColourId,          P::orange.withAlpha (0.35f));

    setColour (TextEditor::backgroundColourId,          Colour (0xff14110F));
    setColour (TextEditor::textColourId,                textCream);
    setColour (TextEditor::highlightColourId,           P::orange.withAlpha (0.35f));
    setColour (TextEditor::outlineColourId,             menuOutline);
    setColour (TextEditor::focusedOutlineColourId,      P::orange);
    setColour (CaretComponent::caretColourId,           P::orange);

    setColour (AlertWindow::backgroundColourId,         Colour (0xff2A2622));
    setColour (AlertWindow::textColourId,               textCream);
    setColour (AlertWindow::outlineColourId,            menuOutline);
    setColour (ResizableWindow::backgroundColourId,     P::ink);
    setColour (ScrollBar::thumbColourId,                P::greyMid);
    setColour (BubbleComponent::backgroundColourId,     P::ink);
    setColour (BubbleComponent::outlineColourId,        menuOutline);
}

Font DxLookAndFeel::getPopupMenuFont()                          { return Font (FontOptions (13.f)); }
Font DxLookAndFeel::getComboBoxFont (ComboBox&)                 { return Fonts::mono (11.f, true); }
Font DxLookAndFeel::getLabelFont (Label& l)                     { return l.getFont(); }
Font DxLookAndFeel::getTextButtonFont (TextButton&, int height) { return Fonts::mono (jlimit (9.f, 11.f, (float) height * 0.42f), true); }
Font DxLookAndFeel::getAlertWindowMessageFont()                 { return Font (FontOptions (14.f)); }
Font DxLookAndFeel::getAlertWindowTitleFont()                   { return Fonts::righteous (19.f); }

void DxLookAndFeel::drawPopupMenuBackground (Graphics& g, int w, int h)
{
    g.fillAll (findColour (PopupMenu::backgroundColourId));
    g.setColour (Colours::white.withAlpha (0.05f));
    g.fillRect (0, 0, w, 1);
    g.setColour (P::orange.withAlpha (0.35f));
    g.drawRect (0, 0, w, h, 1);
}

void DxLookAndFeel::drawPopupMenuItem (Graphics& g, const Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                                       bool isTicked, bool hasSubMenu, const String& text, const String& shortcutKeyText,
                                       const Drawable* icon, const Colour* textColour)
{
    LookAndFeel_V4::drawPopupMenuItem (g, area, isSeparator, isActive, isHighlighted, isTicked, hasSubMenu,
                                       Paint::utf8 (text), Paint::utf8 (shortcutKeyText), icon, textColour);
}

void DxLookAndFeel::drawPopupMenuSectionHeader (Graphics& g, const Rectangle<int>& area, const String& sectionName)
{
    LookAndFeel_V4::drawPopupMenuSectionHeader (g, area, Paint::utf8 (sectionName));
}

void DxLookAndFeel::drawComboBox (Graphics& g, int width, int height, bool, int, int, int, int, ComboBox& box)
{
    const auto r = Rectangle<float> (0.f, 0.f, (float) width, (float) height).reduced (1.f);
    g.setColour (box.findColour (ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 5.f);
    Paint::innerShadow (g, r, 5.f, 0.5f, 4, 1);
    g.setColour (box.findColour (box.hasKeyboardFocus (true) ? ComboBox::focusedOutlineColourId : ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 5.f, 1.f);
    Path arrow;
    const float cx = r.getRight() - 12.f, cy = r.getCentreY();
    arrow.addTriangle (cx - 4.f, cy - 2.f, cx + 4.f, cy - 2.f, cx, cy + 3.f);
    g.setColour (box.findColour (ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 1.f : 0.4f));
    g.fillPath (arrow);
}

// generic raised chip for any plain TextButton other units may use ("onColour" property = lit colour)
void DxLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    const float alpha = b.isEnabled() ? 1.f : 0.45f;
    const float rad = jmin (5.f, r.getHeight() * 0.3f);

    Colour onC = P::orange;
    if (auto* v = b.getProperties().getVarPointer ("onColour"))
        onC = Colour::fromString (v->toString());

    if (down) r = r.translated (0.f, 1.f);
    else Paint::softShadow (g, r, rad, 0.30f * alpha, 2.5f, 1.5f);

    Colour top = on ? onC.brighter (0.12f) : Colour (0xff45454B);
    Colour bot = on ? onC.darker (0.18f)   : Colour (0xff2C2C31);
    if (over && ! down) { top = top.brighter (0.08f); bot = bot.brighter (0.08f); }
    if (down)           { top = top.darker (0.15f);   bot = bot.darker (0.15f); }
    g.setGradientFill (ColourGradient (top.withMultipliedAlpha (alpha), r.getX(), r.getY(), bot.withMultipliedAlpha (alpha), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (Colours::white.withAlpha ((on ? 0.30f : 0.12f) * alpha));
    g.drawLine (r.getX() + rad, r.getY() + 1.f, r.getRight() - rad, r.getY() + 1.f, 1.f);
    g.setColour ((on ? onC.darker (0.5f) : Colours::black).withAlpha ((on ? 0.8f : 0.5f) * alpha));
    g.drawRoundedRectangle (r, rad, 1.f);
    if (on)
    {
        g.setColour (onC.withAlpha (0.28f * alpha));
        g.drawRoundedRectangle (r.expanded (1.5f), rad + 1.5f, 1.5f);
    }
}

void DxLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    const float alpha = b.isEnabled() ? 1.f : 0.5f;
    g.setColour ((b.getToggleState() ? Colours::white : Colour (0xffE4E4E8)).withMultipliedAlpha (alpha));
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.drawFittedText (Paint::utf8 (b.getButtonText()), b.getLocalBounds().reduced (4, 1), Justification::centred, 1, 0.8f);
}

void DxLookAndFeel::drawLabel (Graphics& g, Label& label)
{
    g.fillAll (label.findColour (Label::backgroundColourId));
    if (label.isBeingEdited()) { LookAndFeel_V4::drawLabel (g, label); return; }

    const float alpha = label.isEnabled() ? 1.0f : 0.5f;
    const Font font (getLabelFont (label));
    g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
    g.setFont (font);
    const auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
    g.drawFittedText (Paint::utf8 (label.getText()), textArea, label.getJustificationType(),
                      jmax (1, (int) ((float) textArea.getHeight() / font.getHeight())), label.getMinimumHorizontalScale());
}

static TextLayout tooltipLayout (const String& text, Colour colour)
{
    AttributedString s;
    s.setJustification (Justification::centred);
    s.append (Paint::utf8 (text), FontOptions (13.0f, Font::bold), colour);
    TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (s, 400.0f);
    return tl;
}

Rectangle<int> DxLookAndFeel::getTooltipBounds (const String& tipText, Point<int> screenPos, Rectangle<int> parentArea)
{
    const TextLayout tl (tooltipLayout (tipText, Colours::black));
    const int w = (int) (tl.getWidth() + 14.0f);
    const int h = (int) (tl.getHeight() + 10.0f);
    return Rectangle<int> (screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6)  : screenPos.y + 6,
                           w, h).constrainedWithin (parentArea);
}

void DxLookAndFeel::drawTooltip (Graphics& g, const String& text, int w, int h)
{
    const Rectangle<float> r (0.f, 0.f, (float) w, (float) h);
    g.setColour (findColour (TooltipWindow::backgroundColourId));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (findColour (TooltipWindow::outlineColourId));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);
    tooltipLayout (text, findColour (TooltipWindow::textColourId)).draw (g, r);
}

void DxLookAndFeel::drawCornerResizer (Graphics& g, int w, int h, bool over, bool dragging)
{
    // a quiet moulded grip: six tiny dots on the diagonal, only while the mouse is over it (the corner screw lives there)
    if (! over && ! dragging) return;
    g.setColour (Colours::white.withAlpha (0.4f));
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3 - i; ++j)
            g.fillEllipse ((float) w - 4.f - 4.f * (float) i, (float) h - 4.f - 4.f * (float) j, 1.8f, 1.8f);
}

// ============================================================================
// RackPanel — STUDIO SERIES chassis
// ============================================================================
namespace
{
    Dx::Finish finishForStyle (RackPanel::Style st)
    {
        switch (st)
        {
            case RackPanel::Cream: return Dx::Finish::aluminium();
            case RackPanel::Tan:   return Dx::Finish::ivory();
            case RackPanel::Slate: return Dx::Finish::slate();
            case RackPanel::Dark:
            default:               return Dx::Finish::gunmetal();
        }
    }

    String cleanTitle (const String& t);
    String defaultModel (const String& title)
    {
        String m = cleanTitle (title).toUpperCase();
       #ifdef JucePlugin_VersionString
        m << "   v" JucePlugin_VersionString;
       #endif
        return m;
    }

    // "D ' M A S T E R" (legacy letter-spaced) -> "D'MASTER"; normal titles pass through
    String cleanTitle (const String& t)
    {
        int singles = 0, words = 0;
        for (auto& w : StringArray::fromTokens (t, " ", {}))
            if (w.isNotEmpty()) { ++words; if (w.length() == 1) ++singles; }
        if (words > 3 && singles >= words - 1) return t.removeCharacters (" ");
        return t;
    }
}

RackPanel::RackPanel (String t, String s, const Dx::Finish& f)
    : title (cleanTitle (std::move (t))), subtitle (cleanTitle (std::move (s))), fin (f)
{
    setOpaque (true);
    setInterceptsMouseClicks (false, true);
    model = defaultModel (title);
}

RackPanel::RackPanel (String t, String s, Style st, Colour tc, bool)
    : RackPanel (std::move (t), std::move (s), finishForStyle (st))
{
    style = st;
    ignoreUnused (tc);
}

void RackPanel::setBrand (String b, String) { brand = std::move (b); backdrop = Image(); repaint(); }
void RackPanel::setModel (String m)         { model = std::move (m); backdrop = Image(); repaint(); }
void RackPanel::setFinish (const Dx::Finish& f) { fin = f; backdrop = Image(); repaint(); }
void RackPanel::setStyle (Style s)          { style = s; setFinish (finishForStyle (s)); }
void RackPanel::setSections (Array<Section> s)
{
    bool same = s.size() == sections.size();
    for (int i = 0; same && i < s.size(); ++i)
        same = s[i].area == sections[i].area && s[i].title == sections[i].title && s[i].screws == sections[i].screws;
    if (same) return;
    sections = std::move (s); backdrop = Image(); repaint();
}

Rectangle<int> RackPanel::plateArea() const
{
    return getLocalBounds().reduced (earWidth(), 0);
}

Rectangle<int> RackPanel::toggleArea() const
{
    auto header = plateArea().reduced (kPad, 0).withTrimmedTop (10).removeFromTop (kHeaderH);
    return header.removeFromRight (kToggleW).withSizeKeepingCentre (kToggleW, kToggleH);
}

Rectangle<int> RackPanel::headerArea() const
{
    auto header = plateArea().reduced (kPad, 0).withTrimmedTop (10).removeFromTop (kHeaderH);
    header.removeFromRight (kToggleW + 10);
    return header;
}

Rectangle<int> RackPanel::contentArea() const
{
    auto r = plateArea().reduced (kPad, 0);
    r.removeFromTop (10 + kHeaderH);
    r.removeFromBottom (kPad);
    return r;
}

void RackPanel::paint (Graphics& g)
{
    paintCached (g, getLocalBounds(), true, backdrop, backdropScale, [this] (Graphics& bg)
    {
        bg.fillAll (Colour (0xff0A0A0B));
        const auto b = getLocalBounds().toFloat();
        const float ew = (float) earWidth();
        const bool light = fin.isLight();

        Paint::faceplate (bg, plateArea().toFloat(), fin, 2.f);
        if (fin.woodCheeks)
        {
            Paint::woodCheek (bg, b.withWidth (ew), true);
            Paint::woodCheek (bg, b.withTrimmedLeft (b.getWidth() - ew), false);
        }
        else
        {
            Paint::rackEar (bg, b.withWidth (ew), fin, true, brand, true);
            Paint::rackEar (bg, b.withTrimmedLeft (b.getWidth() - ew), fin, false, model, false);
        }

        // nameplate: product name in Bebas, subtitle engraved beside it on the same baseline
        auto header = headerArea().toFloat();
        const auto nameFont = Fonts::plate (jmin (30.f, header.getHeight() * 0.78f)).withExtraKerningFactor (0.04f);
        const auto name = Paint::utf8 (title).toUpperCase();
        const float nameW = GlyphArrangement::getStringWidth (nameFont, name) + 4.f;
        auto nameArea = header.removeFromLeft (jmin (nameW, header.getWidth()));
        bg.setColour (light ? Colours::white.withAlpha (0.5f) : Colours::black.withAlpha (0.6f));
        bg.setFont (nameFont);
        bg.drawText (name, nameArea.translated (0.f, 1.f), Justification::centredLeft, false);
        bg.setColour (fin.title);
        bg.drawText (name, nameArea, Justification::centredLeft, false);
        if (subtitle.isNotEmpty())
        {
            header.removeFromLeft (12.f);
            // a fine rule, then the subtitle
            bg.setColour (fin.inkDim.withAlpha (0.6f));
            bg.fillRect (header.getX() - 6.f, header.getCentreY() - 7.f, 1.f, 14.f);
            Paint::engrave (bg, subtitle.toUpperCase(), header.withTrimmedLeft (2.f).translated (0.f, 2.f), fin.inkDim,
                            Fonts::engraved (11.f), Justification::centredLeft, light, 0.14f);
        }
        // a hairline under the header
        const float hy = (float) (plateArea().getY() + 10 + kHeaderH + 3);
        bg.setColour (Colours::black.withAlpha (light ? 0.18f : 0.5f));
        bg.fillRect ((float) plateArea().getX() + kPad, hy, (float) plateArea().getWidth() - 2.f * kPad, 1.f);
        bg.setColour (Colours::white.withAlpha (light ? 0.5f : 0.06f));
        bg.fillRect ((float) plateArea().getX() + kPad, hy + 1.f, (float) plateArea().getWidth() - 2.f * kPad, 1.f);

        for (auto& sec : sections)
            Paint::section (bg, sec.area.toFloat(), fin, sec.title, sec.screws);
    });
}

// ============================================================================
// DxKnob
// ============================================================================
class DxKnob::KnobSlider : public Slider
{
public:
    explicit KnobSlider (DxKnob& o) : owner (o)
    {
        setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        setRotaryParameters (kKnobA0, kKnobA1, true);
        setScrollWheelEnabled (true);
        setMouseDragSensitivity (200);
        setVelocityBasedMode (false);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setOpaque (false);
    }
    void paint (Graphics&) override {}                       // the owner draws; this child only takes the mouse
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); owner.showBubble (false); }
    void mouseExit (const MouseEvent& e) override  { Slider::mouseExit (e);  owner.showBubble (false); }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& d) override { Slider::mouseWheelMove (e, d); owner.showBubble (true); }
    void mouseDoubleClick (const MouseEvent& e) override { Slider::mouseDoubleClick (e); owner.showBubble (true); }
private:
    DxKnob& owner;
};

DxKnob::DxKnob (RangedAudioParameter& p, String capt, Body b, String u)
    : param (p), caption (std::move (capt)), unit (std::move (u)), body (b), captionCol (P::labelGrey),
      sliderImpl (std::make_unique<KnobSlider> (*this)), slider (*sliderImpl)
{
    addAndMakeVisible (slider);
    slider.setTitle (caption);
    slider.onValueChange = [this]
    {
        targetPos = param.getNormalisableRange().convertTo0to1 ((float) slider.getValue());
        if (animPos < 0.f) animPos = targetPos;
        startTimerHz (60);
    };
    slider.onDragStart = [this] { dragging = true; startTimerHz (60); repaint(); };
    slider.onDragEnd   = [this] { dragging = false; bubbleFrames = 24; startTimerHz (60); };

    attachment = std::make_unique<SliderParameterAttachment> (param, slider);
    targetPos = param.getNormalisableRange().convertTo0to1 ((float) slider.getValue());
    animPos = targetPos;
    setSize (knobPx);
}

DxKnob::~DxKnob()
{
    stopTimer();
    attachment.reset();
}

float DxKnob::labelSize() const        { return jlimit (7.5f, 10.5f, (float) knobPx * 0.13f); }
float DxKnob::outerRadius() const      { return (float) knobPx * 0.5f * 1.24f + (showScale ? labelSize() * 1.75f : 2.f); }
int   DxKnob::preferredHeight() const  { return kCaptionH + roundToInt (outerRadius() * 2.f) + 2; }
int   DxKnob::preferredWidth() const   { return roundToInt (outerRadius() * 2.f + labelSize() * 1.6f); }

Point<float> DxKnob::knobCentre() const
{
    return { (float) getWidth() * 0.5f, (float) kCaptionH + outerRadius() + 1.f };
}

void DxKnob::placeIn (Rectangle<int> area, int px)
{
    setSize (px);
    setBounds (Rectangle<int> (preferredWidth(), preferredHeight()).withCentre (area.getCentre()));
}

void DxKnob::setSize (int px)
{
    knobPx = jmax (24, px);
    Component::setSize (preferredWidth(), preferredHeight());
}

void DxKnob::setCaptionColour (Colour c)               { captionCol = c; captionOverride = true; repaint(); }
void DxKnob::setTooltip (const String& t)              { slider.setTooltip (t); }
void DxKnob::setValueText (std::function<String (float)> f) { valueText = std::move (f); }

String DxKnob::currentText() const
{
    return valueText ? valueText (targetPos) : paramText (param, targetPos, unit);
}

void DxKnob::showBubble (bool sticky)
{
    if (sticky) bubbleFrames = 45;
    startTimerHz (60);
}

void DxKnob::flashValue (int frames)
{
    bubbleFrames = jmax (bubbleFrames, frames);
    startTimerHz (60);
}

void DxKnob::resized()
{
    const auto c = knobCentre();
    const float r = (float) knobPx * 0.5f * 1.12f;
    slider.setBounds (Rectangle<float> (r * 2.f, r * 2.f).withCentre (c).toNearestInt());
}

void DxKnob::timerCallback()
{
    bool busy = false;
    const float d = targetPos - animPos;
    if (std::abs (d) > 0.0005f) { animPos += d * 0.45f; busy = true; }
    else animPos = targetPos;

    const bool hot = dragging || slider.isMouseOver();
    const float gt = hot ? 1.f : 0.f;
    if (std::abs (glow - gt) > 0.01f) { glow += (gt - glow) * 0.25f; busy = true; }
    else glow = gt;

    if (bubbleFrames > 0) { --bubbleFrames; busy = true; }
    if (dragging) busy = true;

    repaint();
    if (! busy) stopTimer();
}

// Engraved scale values straight from the parameter: "12.5 ms" -> "12.5", "+6.0 dB" -> "+6", "1.2 kHz" -> "1.2k".
StringArray DxKnob::labelsForScale() const
{
    if (! autoScale) return scaleLabels;
    StringArray out;
    const int n = knobPx < 42 ? 3 : labelCount;           // small knobs: min / centre / max only, so neighbours never collide
    for (int k = 0; k < n; ++k)
    {
        const float t = (float) k / (float) (n - 1);
        String txt = valueText ? valueText (t) : param.getText (t, 0);
        txt = txt.trim();
        const bool kilo = txt.containsIgnoreCase ("khz");
        String num = txt.retainCharacters ("+-.0123456789");
        if (num.isEmpty() || num == "." || num == "-" || num == "+")
        {
            out.add (txt.upToFirstOccurrenceOf (" ", false, false).substring (0, 4).toUpperCase());
            continue;
        }
        double v = num.getDoubleValue();
        String shown = std::abs (v) >= 10.0 || std::abs (v - std::round (v)) < 0.05 ? String (roundToInt (v)) : String (v, 1);
        if (num.startsWithChar ('+') && v > 0.0) shown = "+" + shown;
        if (kilo) shown << "k";
        out.add (shown);
    }
    return out;
}

void DxKnob::paint (Graphics& g)
{
    const auto c = knobCentre();
    const float R = (float) knobPx * 0.5f;
    const bool enabled = isEnabled();
    const float alpha = enabled ? 1.f : 0.45f;
    const float pos = jlimit (0.f, 1.f, animPos < 0.f ? targetPos : animPos);
    const float ang = kKnobA0 + (kKnobA1 - kKnobA0) * pos;

    const auto* panel = findParentComponentOfClass<RackPanel>();
    const Dx::Finish fin = panel != nullptr ? panel->finish() : Dx::Finish::gunmetal();
    const bool light = fin.controlSurfaceLight();
    const Colour ink = captionOverride ? captionCol : fin.controlInk();

    // caption, engraved above the knob
    Paint::engrave (g, caption.toUpperCase(), Rectangle<float> (0.f, 0.f, (float) getWidth(), (float) kCaptionH),
                    ink.withMultipliedAlpha (alpha), Fonts::engraved (11.f), Justification::centred, light, 0.12f);

    if (showScale)
        Paint::knobScale (g, c, R, kKnobA0, kKnobA1, autoScale && knobPx < 42 ? 5 : labelCount, labelsForScale(),
                          ink.withMultipliedAlpha (alpha), light, labelSize());

    int bodyIndex = 2;
    Colour capC = Colours::transparentBlack, accent = Colours::transparentBlack;
    switch (body)
    {
        case Marconi:  bodyIndex = 0; capC = cap; break;
        case Chrome:   bodyIndex = 3; capC = cap; break;
        case Chunky:
        case Cream:    bodyIndex = 1; break;
        case Bakelite:
        case Dark:
        default:       bodyIndex = 2; accent = fin.accent; break;
    }
    Paint::knob (g, c, R, ang, bodyIndex, capC, accent, alpha);

    // hover: a faint ring of light on the plate
    if (glow > 0.01f)
    {
        g.setColour ((light ? Colours::black : Colours::white).withAlpha (0.07f * glow));
        g.drawEllipse (Rectangle<float> (R * 2.3f, R * 2.3f).withCentre (c), 1.5f);
    }

    if (dragging || bubbleFrames > 0 || (glow > 0.6f && slider.isMouseOver()))
        Paint::valueBubble (g, currentText(), { c.x, c.y - R - 6.f }, (float) getWidth() + 30.f);
}

// ============================================================================
// DxToggle — raised rocker: orange when IN, grey when OFF
// ============================================================================
DxToggle::DxToggle (RangedAudioParameter& p, String onT, String offT)
    : onText (std::move (onT)), offText (std::move (offT)),
      attachment (p, [this] (float v) { const bool nv = v > 0.5f; if (nv != on) { on = nv; repaint(); } })
{
    setRepaintsOnMouseActivity (true);
    setTooltip ("Switch the unit in or out of the signal path");
    attachment.sendInitialUpdate();
}

DxToggle::~DxToggle() = default;

void DxToggle::mouseEnter (const MouseEvent&) { over = true; repaint(); }
void DxToggle::mouseExit (const MouseEvent&)  { over = false; down = false; repaint(); }
void DxToggle::mouseDown (const MouseEvent&)  { down = true; repaint(); }
void DxToggle::mouseUp (const MouseEvent& e)
{
    const bool inside = down && getLocalBounds().contains (e.getPosition());
    down = false;
    if (inside && isEnabled())
        attachment.setValueAsCompleteGesture (on ? 0.f : 1.f);
    repaint();
}

void DxToggle::paint (Graphics& g)
{
    const auto* panel = findParentComponentOfClass<RackPanel>();
    const bool light = panel != nullptr && panel->finish().isLight();
    auto r = getLocalBounds().toFloat().reduced (1.f);
    Graphics::ScopedSaveState ss (g);
    if (! isEnabled()) g.setOpacity (0.45f);
    Paint::lampButton (g, r, lens, on, over, down, on ? onText : offText, light);
}

// ============================================================================
// DxLampButton
// ============================================================================
DxLampButton::DxLampButton (String t, Colour l) : text (std::move (t)), lens (l)
{
    setMouseCursor (MouseCursor::PointingHandCursor);
}

void DxLampButton::mouseUp (const MouseEvent& e)
{
    const bool inside = down && getLocalBounds().contains (e.getPosition());
    down = false;
    repaint();
    if (inside && isEnabled() && onClick) onClick();
}

void DxLampButton::paint (Graphics& g)
{
    const auto* panel = findParentComponentOfClass<RackPanel>();
    const bool light = panel != nullptr && panel->finish().isLight();
    Graphics::ScopedSaveState ss (g);
    if (! isEnabled()) g.setOpacity (0.45f);
    Paint::lampButton (g, getLocalBounds().toFloat().reduced (1.f), lens, lit, over, down, text, light);
}

// ============================================================================
// DxChoiceRow — raised chips for a choice parameter
// ============================================================================
DxChoiceRow::DxChoiceRow (RangedAudioParameter& p, StringArray l, Colour onC, int redIdx)
    : labels (std::move (l)), onColour (onC), redIndex (redIdx),
      attachment (p, [this] (float v) { const int nv = roundToInt (v); if (nv != selected) { selected = nv; repaint(); } })
{
    attachment.sendInitialUpdate();
}

DxChoiceRow::~DxChoiceRow() = default;

void DxChoiceRow::setChipTooltips (StringArray t) { tips = std::move (t); }

String DxChoiceRow::getTooltip()
{
    const int i = hover >= 0 ? hover : chipAt (getMouseXYRelative());
    if (i >= 0 && i < tips.size()) return tips[i];
    return SettableTooltipClient::getTooltip();
}

void DxChoiceRow::resized()
{
    chips.clear();
    const int n = labels.size();
    if (n == 0) return;
    const auto b = getLocalBounds();
    const int cw = jmax (10, jmin (chipMaxW, (b.getWidth() - chipGap * (n - 1)) / n));
    const int total = cw * n + chipGap * (n - 1);
    int x = b.getCentreX() - total / 2;
    for (int i = 0; i < n; ++i)
    {
        chips.add (Rectangle<int> (x, b.getY(), cw, b.getHeight()));
        x += cw + chipGap;
    }
}

int DxChoiceRow::chipAt (Point<int> p) const
{
    for (int i = 0; i < chips.size(); ++i)
        if (chips[i].expanded (chipGap / 2, 0).contains (p)) return i;
    return -1;
}

void DxChoiceRow::mouseMove (const MouseEvent& e) { const int h = chipAt (e.getPosition()); if (h != hover) { hover = h; repaint(); } }
void DxChoiceRow::mouseExit (const MouseEvent&)   { hover = -1; pressed = -1; repaint(); }
void DxChoiceRow::mouseDown (const MouseEvent& e) { pressed = chipAt (e.getPosition()); repaint(); }
void DxChoiceRow::mouseUp (const MouseEvent& e)
{
    const int i = chipAt (e.getPosition());
    if (i >= 0 && i == pressed && isEnabled())
        attachment.setValueAsCompleteGesture ((float) i);
    pressed = -1;
    repaint();
}

void DxChoiceRow::paint (Graphics& g)
{
    const auto* panel = findParentComponentOfClass<RackPanel>();
    const bool light = panel != nullptr && panel->finish().isLight();
    Graphics::ScopedSaveState ss (g);
    if (! isEnabled()) g.setOpacity (0.45f);
    for (int i = 0; i < chips.size(); ++i)
    {
        const Colour lens = (i == redIndex) ? P::lampRed : onColour;
        Paint::lampButton (g, chips[i].toFloat().reduced (0.5f), lens, i == selected, i == hover, i == pressed, labels[i], light);
    }
}

// ============================================================================
// LedRow — 13 jewel LEDs, fill from the right, instant attack / slow decay / peak hold
// ============================================================================
LedRow::LedRow()  { setInterceptsMouseClicks (false, false); }
LedRow::~LedRow() { stopTimer(); }

void LedRow::setDb (float db)
{
    target = jlimit (0.f, fullScale, db);
    if (target > shown) shown = target;                    // LEDs strike instantly
    if (target >= peak) { peak = target; peakHold = 18; }
    if (! isTimerRunning()) startTimerHz (30);
}

void LedRow::timerCallback()
{
    bool busy = false;
    if (shown > target)          { shown = jmax (target, shown - fullScale / 60.f); busy = true; }   // ~0.45 dB/frame
    if (peakHold > 0)            { --peakHold; busy = true; }
    else if (peak > shown)       { peak = jmax (shown, peak - fullScale / 22.f); busy = true; }
    repaint();
    if (! busy) stopTimer();
}

void LedRow::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const int n = count;
    const float along = vertical ? b.getHeight() : b.getWidth();
    const float across = vertical ? b.getWidth() : b.getHeight();
    const float gap = jmax (1.5f, along / (float) n * 0.22f);
    const float seg = (along - gap * (float) (n - 1)) / (float) n;
    const float thick = jmin (across, vertical ? across : 9.f);
    const float litCount  = (shown / fullScale) * (float) n;
    const float peakCount = (peak  / fullScale) * (float) n;

    // bezel strip behind the segments
    auto strip = vertical ? Rectangle<float> (b.getCentreX() - thick * 0.5f - 2.f, b.getY() - 2.f, thick + 4.f, b.getHeight() + 4.f)
                          : Rectangle<float> (b.getX() - 2.f, b.getCentreY() - thick * 0.5f - 2.f, b.getWidth() + 4.f, thick + 4.f);
    g.setColour (Colours::black.withAlpha (0.85f));
    g.fillRoundedRectangle (strip, 2.5f);

    for (int i = 0; i < n; ++i)
    {
        // "level" index: 0 = the first segment to light
        const int lvl = fromLeft ? i : (n - 1) - i;
        const float frac = (float) lvl / (float) (n - 1);
        const bool lit  = (float) lvl < litCount;
        const bool held = ! lit && (float) lvl < peakCount && (float) lvl + 1.f >= peakCount;
        const Colour c = frac > 0.78f ? Colour (0xffFF3B30) : (frac > 0.50f ? P::ledAmber : P::ledOn);
        const float pos = (float) i * (seg + gap);
        Rectangle<float> r = vertical ? Rectangle<float> (b.getCentreX() - thick * 0.5f, b.getBottom() - pos - seg, thick, seg)
                                      : Rectangle<float> (b.getX() + pos, b.getCentreY() - thick * 0.5f, seg, thick);
        Paint::led (g, r, c, lit ? 1.f : (held ? 0.55f : 0.f));
    }
}

// ============================================================================
// VuMeter — the DEES TAPE VU face in a bezel, with glass
// ============================================================================
namespace
{
    struct VuMark { float db; float frac; bool major; const char* text; };
    const VuMark kVuMarks[] =
    {
        { -20.f, 0.00f, true,  "20" }, { -10.f, 0.20f, true,  "10" }, { -7.f, 0.32f, true,  "7" },
        {  -5.f, 0.43f, true,  "5"  }, {  -3.f, 0.55f, true,  "3"  }, { -2.f, 0.62f, false, "" },
        {  -1.f, 0.69f, false, ""   }, {   0.f, 0.77f, true,  "0"  }, {  1.f, 0.85f, false, "" },
        {   2.f, 0.93f, false, ""   }, {   3.f, 1.00f, true,  "+3" }
    };

    float vuFraction (float db)
    {
        if (db <= kVuMarks[0].db) return 0.f;
        for (size_t i = 1; i < numElementsInArray (kVuMarks); ++i)
            if (db <= kVuMarks[i].db)
            {
                const auto& a = kVuMarks[i - 1];
                const auto& b = kVuMarks[i];
                return a.frac + (b.frac - a.frac) * (db - a.db) / (b.db - a.db);
            }
        return 1.f;
    }

    Point<float> fromMath (float cx, float cy, float mathAngle, float radius)
    {
        return { cx + std::cos (mathAngle) * radius, cy + std::sin (mathAngle) * radius };
    }
    float toJuceAngle (float mathAngle) { return mathAngle + kPi * 0.5f; }
}

VuMeter::VuMeter()  { setInterceptsMouseClicks (false, false); }
VuMeter::~VuMeter() { stopTimer(); }

void VuMeter::setDb (float db)
{
    float t;
    if (db < -20.f) t = -0.05f * jlimit (0.f, 1.f, (-20.f - db) / 20.f);   // rest against the left stop
    else            t = vuFraction (jmin (3.f, db));
    if (reverse) t = 1.f - t;
    target = t;
    if (! isTimerRunning()) startTimerHz (60);
}

void VuMeter::setLevel (float linear)
{
    setDb (linear > 1.0e-4f ? 20.f * std::log10 (linear) : -80.f);
}

void VuMeter::timerCallback()
{
    // damped needle: ~300 ms to 99 % up, a touch slower down (VU ballistics at 60 Hz)
    const float k = target > needle ? 0.24f : 0.16f;
    const float next = needle + (target - needle) * k;
    if (std::abs (next - needle) > 0.0004f) { needle = next; repaint(); }
    else if (std::abs (needle - target) > 0.f) { needle = target; repaint(); }
    else stopTimer();
}

void VuMeter::paint (Graphics& g)
{
    const auto bounds = getLocalBounds();
    const int capH = caption.isNotEmpty() ? 14 : 0;
    const auto bezel = bounds.withTrimmedBottom (capH).toFloat().reduced (1.f);
    const float bw = jmax (4.f, bezel.getHeight() * 0.09f);
    const auto face = bezel.reduced (bw);
    const float w = face.getWidth(), h = face.getHeight();
    if (w < 10.f || h < 10.f) return;

    // geometry from the face size: the scale arc spans 80 degrees and fills ~84 % of the width, its top at 30 % height
    const float px = face.getCentreX();
    const float r  = jmin (w * 0.42f / std::sin (degreesToRadians (40.f)), h * 1.25f);
    const float py = face.getY() + h * 0.34f + r;
    const float a0 = degreesToRadians (-130.f), a1 = degreesToRadians (-50.f);
    auto angleFor = [&] (float frac) { return a0 + (a1 - a0) * frac; };

    Path fp;
    fp.addRoundedRectangle (face, 3.f);

    paintCached (g, bounds, false, backdrop, backdropScale, [&] (Graphics& bg)
    {
        // bezel: black anodised frame, four tiny screws
        Paint::softShadow (bg, bezel, 5.f, 0.5f, 4.f, 2.f);
        bg.setGradientFill (ColourGradient (Colour (0xff3B3B3E), bezel.getX(), bezel.getY(), Colour (0xff0F0F10), bezel.getX(), bezel.getBottom(), false));
        bg.fillRoundedRectangle (bezel, 5.f);
        bg.setColour (Colours::white.withAlpha (0.14f));
        bg.drawLine (bezel.getX() + 5.f, bezel.getY() + 1.f, bezel.getRight() - 5.f, bezel.getY() + 1.f, 1.f);
        bg.setColour (Colours::black);
        bg.drawRoundedRectangle (bezel, 5.f, 1.f);
        const float sr = jlimit (1.6f, 2.8f, bw * 0.34f);
        for (auto pnt : { bezel.getTopLeft().translated (bw * 0.5f, bw * 0.5f), bezel.getTopRight().translated (-bw * 0.5f, bw * 0.5f),
                          bezel.getBottomLeft().translated (bw * 0.5f, -bw * 0.5f), bezel.getBottomRight().translated (-bw * 0.5f, -bw * 0.5f) })
            Paint::phillips (bg, pnt, sr, 0.5f);

        // face: warm parchment, backlit from below (the amber lamp behind the meter)
        bg.setGradientFill (ColourGradient (Colour (0xffF7EDD0), face.getX(), face.getY(), Colour (0xffE9D9AE), face.getX(), face.getBottom(), false));
        bg.fillPath (fp);
        {
            Graphics::ScopedSaveState ss (bg);
            bg.reduceClipRegion (fp);
            bg.setGradientFill (ColourGradient (Colour (0xffFFC870).withAlpha (0.55f), px, face.getBottom(),
                                                Colour (0xffFFC870).withAlpha (0.0f), px, face.getY() + h * 0.15f, true));
            bg.fillRect (face);

            Path arc;
            arc.addCentredArc (px, py, r, r, 0.f, toJuceAngle (angleFor (0.f)), toJuceAngle (angleFor (0.77f)), true);
            bg.setColour (Colour (0xff1A1612).withAlpha (0.85f));
            bg.strokePath (arc, PathStrokeType (jmax (1.f, h * 0.016f)));
            Path red;
            red.addCentredArc (px, py, r, r, 0.f, toJuceAngle (angleFor (0.77f)), toJuceAngle (angleFor (1.f)), true);
            bg.setColour (Colour (0xffCF2A22));
            bg.strokePath (red, PathStrokeType (jmax (2.2f, h * 0.055f)));
            // second (percent) arc below, the way real VU faces print it
            Path arc2;
            arc2.addCentredArc (px, py, r - h * 0.07f, r - h * 0.07f, 0.f, toJuceAngle (angleFor (0.f)), toJuceAngle (angleFor (1.f)), true);
            bg.setColour (Colour (0xff1A1612).withAlpha (0.35f));
            bg.strokePath (arc2, PathStrokeType (0.8f));

            const float tickW = jmax (1.1f, h * 0.025f);
            const bool labels = h >= 36.f;
            for (const auto& m : kVuMarks)
            {
                const float a = angleFor (m.frac);
                const float len = m.major ? h * 0.10f : h * 0.055f;
                bg.setColour (m.db > 0.f ? Colour (0xffCF2A22) : Colour (0xff1A1612).withAlpha (0.85f));
                bg.drawLine (Line<float> (fromMath (px, py, a, r), fromMath (px, py, a, r + len)), tickW);
                if (labels && m.text[0] != 0)
                {
                    const float fs = jlimit (7.f, 12.f, h * 0.14f);
                    const auto cpt = fromMath (px, py, a, r + len + fs * 0.75f);
                    bg.setFont (Fonts::engraved (fs));
                    bg.drawText (m.text, Rectangle<float> (fs * 2.2f, fs * 1.1f).withCentre (cpt).constrainedWithin (face.reduced (2.f)), Justification::centred, false);
                }
            }

            bg.setColour (Colour (0xff1A1612).withAlpha (0.8f));
            bg.setFont (Fonts::plate (h * 0.24f).withExtraKerningFactor (0.06f));
            bg.drawText (Paint::utf8 (legend), Rectangle<float> (w, h * 0.26f).withCentre ({ px, face.getY() + h * 0.66f }), Justification::centred, false);
            bg.setColour (Colour (0xff1A1612).withAlpha (0.40f));
            bg.setFont (Fonts::righteous (jlimit (5.f, 7.5f, h * 0.075f)).withExtraKerningFactor (0.12f));
            bg.drawText ("DEESTECH", Rectangle<float> (w * 0.4f, h * 0.12f).withPosition (face.getX() + 5.f, face.getBottom() - h * 0.15f), Justification::centredLeft, false);

            Paint::innerShadow (bg, face, 3.f, 0.55f, 6, 2);
            // glass: a diagonal sheen
            Path glass;
            glass.startNewSubPath (face.getX(), face.getY());
            glass.lineTo (face.getRight(), face.getY());
            glass.lineTo (face.getRight(), face.getY() + h * 0.18f);
            glass.lineTo (face.getX(), face.getY() + h * 0.46f);
            glass.closeSubPath();
            bg.setColour (Colours::white.withAlpha (0.16f));
            bg.fillPath (glass);
        }
        bg.setColour (Colours::black.withAlpha (0.9f));
        bg.strokePath (fp, PathStrokeType (1.2f));

        if (capH > 0)
        {
            const auto* panel = findParentComponentOfClass<RackPanel>();
            const auto fin = panel != nullptr ? panel->finish() : Dx::Finish::gunmetal();
            Paint::engrave (bg, caption.toUpperCase(), bounds.withTrimmedTop (bounds.getHeight() - capH).toFloat(), fin.controlInk(),
                            Fonts::engraved (10.f), Justification::centred, fin.controlSurfaceLight(), 0.14f);
        }
    });

    // needle (live): shadow + black blade with a red tip + pivot cover
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (fp);
        const float na = angleFor (jlimit (-0.06f, 1.04f, needle));
        Path np;
        np.startNewSubPath (fromMath (px, py, na, 0.f));
        np.lineTo (fromMath (px, py, na, r + h * 0.09f));
        const PathStrokeType stroke (jmax (1.3f, w * 0.014f), PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour (Colours::black.withAlpha (0.22f));
        g.strokePath (np, stroke, AffineTransform::translation (1.5f, 2.2f));
        g.setColour (Colour (0xff151210));
        g.strokePath (np, stroke);
        // the pivot cover: a small black hood sitting on the bottom edge
        const float pw = jmin (w * 0.16f, h * 0.30f), ph = pw * 0.42f;
        Path hood; hood.addPieSegment (px - pw * 0.5f, face.getBottom() - ph, pw, ph * 2.f, -MathConstants<float>::halfPi, MathConstants<float>::halfPi, 0.f);
        g.setGradientFill (ColourGradient (Colour (0xff3A3A3D), px, face.getBottom() - ph, Colour (0xff0B0B0C), px, face.getBottom(), false));
        g.fillPath (hood);
    }
}

// ============================================================================
// DxFader
// ============================================================================
class DxFader::FaderSlider : public Slider
{
public:
    explicit FaderSlider (DxFader& o) : owner (o)
    {
        setSliderStyle (Slider::LinearVertical);
        setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        setScrollWheelEnabled (true);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
        setOpaque (false);
    }
    void paint (Graphics&) override {}
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); owner.repaint(); }
    void mouseExit (const MouseEvent& e) override  { Slider::mouseExit (e);  owner.repaint(); }
private:
    DxFader& owner;
};

DxFader::DxFader (RangedAudioParameter& p, String l)
    : param (p), label (std::move (l)), captionCol (P::labelGrey),
      sliderImpl (std::make_unique<FaderSlider> (*this)), slider (*sliderImpl)
{
    addAndMakeVisible (slider);
    slider.setTitle (label);
    slider.onValueChange = [this] { startTimerHz (60); };
    slider.onDragStart = [this] { dragging = true; startTimerHz (60); repaint(); };
    slider.onDragEnd   = [this] { dragging = false; bubbleFrames = 24; startTimerHz (60); };
    attachment = std::make_unique<SliderParameterAttachment> (param, slider);
    animPos = param.getNormalisableRange().convertTo0to1 ((float) slider.getValue());
    setSize (34, 150);
}

DxFader::~DxFader()
{
    stopTimer();
    attachment.reset();
}

void DxFader::setCaptionColour (Colour c) { captionCol = c; captionOverride = true; repaint(); }
void DxFader::setTooltip (const String& t) { slider.setTooltip (t); }

void DxFader::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedBottom (kLabelH));
}

void DxFader::timerCallback()
{
    bool busy = dragging;
    const float target = param.getNormalisableRange().convertTo0to1 ((float) slider.getValue());
    const float d = target - animPos;
    if (std::abs (d) > 0.0005f) { animPos += d * 0.45f; busy = true; }
    else animPos = target;
    if (bubbleFrames > 0) { --bubbleFrames; busy = true; }
    repaint();
    if (! busy) stopTimer();
}

void DxFader::paint (Graphics& g)
{
    const auto sb = slider.getBounds().toFloat();
    if (sb.getHeight() < 30.f) return;
    const float alpha = isEnabled() ? 1.f : 0.45f;
    const bool hot = dragging || slider.isMouseOver();
    const auto* panel = findParentComponentOfClass<RackPanel>();
    const Dx::Finish fin = panel != nullptr ? panel->finish() : Dx::Finish::gunmetal();
    const bool light = fin.controlSurfaceLight();
    const Colour ink = captionOverride ? captionCol : fin.controlInk();

    const float capH = 22.f, capW = jlimit (18.f, 34.f, sb.getWidth() * 0.66f);
    const float cx = sb.getCentreX();
    const float yTop = sb.getY() + slider.getPositionOfValue (slider.getMaximum());
    const float yBot = sb.getY() + slider.getPositionOfValue (slider.getMinimum());
    const float travel = jmax (1.f, yBot - yTop);
    const float capY = yBot - travel * jlimit (0.f, 1.f, animPos);

    // printed scale left of the slot: ticks every 1/8, values at 5 points from the parameter
    for (int i = 0; i <= 8; ++i)
    {
        const float ty = yBot - travel * ((float) i / 8.f);
        const bool major = i % 2 == 0;
        g.setColour (ink.withAlpha ((major ? 0.9f : 0.5f) * alpha));
        g.fillRect (Rectangle<float> (cx - capW * 0.5f - (major ? 8.f : 5.f), ty - 0.5f, major ? 6.f : 3.f, 1.f));
        g.fillRect (Rectangle<float> (cx + capW * 0.5f + 2.f, ty - 0.5f, major ? 6.f : 3.f, 1.f));
    }

    // slot
    const float railW = 5.f;
    const Rectangle<float> rail (cx - railW * 0.5f, yTop - 4.f, railW, travel + 8.f);
    g.setColour (Colour (0xff050505).withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (rail, 2.5f);
    Paint::innerShadow (g, rail, 2.5f, 0.8f, 3, 1);
    g.setColour ((light ? Colours::white.withAlpha (0.55f) : Colours::white.withAlpha (0.08f)).withMultipliedAlpha (alpha));
    g.drawLine (rail.getRight() + 0.5f, rail.getY() + 2.f, rail.getRight() + 0.5f, rail.getBottom() - 2.f, 0.8f);

    // cap: black moulded with a coloured stripe
    const Rectangle<float> cap (cx - capW * 0.5f, capY - capH * 0.5f, capW, capH);
    Paint::softShadow (g, cap, 3.f, 0.55f * alpha, 4.f, 3.f);
    g.setGradientFill (ColourGradient ((hot ? Colour (0xff4A4A4E) : Colour (0xff3A3A3D)).withMultipliedAlpha (alpha), cap.getX(), cap.getY(),
                                       Colour (0xff0D0D0E).withMultipliedAlpha (alpha), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 3.f);
    // ridges
    for (int k = 1; k <= 3; ++k)
    {
        const float yy = cap.getY() + cap.getHeight() * (float) k / 4.f;
        if (k == 2) continue;
        g.setColour (Colours::white.withAlpha (0.10f * alpha)); g.fillRect (cap.getX() + 2.f, yy, cap.getWidth() - 4.f, 1.f);
        g.setColour (Colours::black.withAlpha (0.5f * alpha));  g.fillRect (cap.getX() + 2.f, yy + 1.f, cap.getWidth() - 4.f, 1.f);
    }
    g.setColour (capColour.withMultipliedAlpha (alpha));
    g.fillRect (Rectangle<float> (cap.getX() + 1.5f, cap.getCentreY() - 1.5f, cap.getWidth() - 3.f, 3.f));
    g.setColour (Colours::black.withAlpha (0.8f * alpha));
    g.drawRoundedRectangle (cap, 3.f, 1.f);

    // band label + value
    auto labels = getLocalBounds().removeFromBottom (kLabelH).toFloat();
    Paint::engrave (g, label.toUpperCase(), labels.removeFromTop (14.f), ink.withMultipliedAlpha (alpha), Fonts::engraved (11.f),
                    Justification::centred, light, 0.08f);
    g.setColour (fin.accent.withMultipliedAlpha (alpha));
    g.setFont (Fonts::mono (8.f, true));
    g.drawText (Paint::utf8 (paramText (param, param.getNormalisableRange().convertTo0to1 ((float) slider.getValue()), {})),
                labels, Justification::centred, false);

    if (dragging || bubbleFrames > 0)
        Paint::valueBubble (g, paramText (param, param.getNormalisableRange().convertTo0to1 ((float) slider.getValue()), {}),
                            { cx, jmax (sb.getY() + 9.f, capY - capH - 4.f) }, (float) getWidth() + 40.f);
}

// ============================================================================
// DxLamp — jewel lamp in a chrome bezel
// ============================================================================
void DxLamp::set (Colour c, bool on)
{
    if (colour != c || lit != on) { colour = c; lit = on; repaint(); }
}

void DxLamp::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float d = jmin (b.getWidth(), b.getHeight());
    if (d < 5.f) return;

    const auto full = Rectangle<float> (d, d).withCentre (b.getCentre());
    const float cx = full.getCentreX(), cy = full.getCentreY();
    const auto ring = full.reduced (d * 0.08f);
    const auto jewel = full.reduced (d * 0.26f);

    if (lit)
    {
        g.setGradientFill (ColourGradient (colour.withAlpha (0.55f), cx, cy, colour.withAlpha (0.f), cx + d * 0.55f, cy, true));
        g.fillEllipse (full.expanded (d * 0.15f));
    }

    // chrome bezel
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillEllipse (ring.translated (0.f, 0.8f).expanded (0.5f));
    ColourGradient chrome (Colour (0xffE2E1DD), cx - d * 0.3f, cy - d * 0.35f, Colour (0xff4E4D4A), cx + d * 0.35f, cy + d * 0.4f, true);
    chrome.addColour (0.5, Colour (0xff9C9B97));
    g.setGradientFill (chrome);
    g.fillEllipse (ring);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.drawEllipse (ring, 0.7f);

    // well
    g.setColour (Colour (0xff141212));
    g.fillEllipse (jewel.expanded (d * 0.05f));

    if (lit)
    {
        g.setGradientFill (ColourGradient (colour.brighter (0.8f), jewel.getX() + jewel.getWidth() * 0.3f, jewel.getY() + jewel.getHeight() * 0.3f,
                                           colour.darker (0.1f), jewel.getRight(), jewel.getBottom(), true));
        g.fillEllipse (jewel);
        g.setColour (Colours::white.withAlpha (0.6f));
        g.fillEllipse (jewel.reduced (jewel.getWidth() * 0.32f).translated (-jewel.getWidth() * 0.10f, -jewel.getHeight() * 0.14f));
    }
    else
    {
        g.setGradientFill (ColourGradient (colour.darker (0.55f).withAlpha (0.9f), jewel.getX(), jewel.getY(),
                                           colour.darker (0.85f), jewel.getX(), jewel.getBottom(), false));
        g.fillEllipse (jewel);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.fillEllipse (jewel.reduced (jewel.getWidth() * 0.32f).translated (-jewel.getWidth() * 0.10f, -jewel.getHeight() * 0.14f));
    }
}

// ============================================================================
// DxTube
// ============================================================================
DxTube::DxTube() : rng (0x7A0E) { setInterceptsMouseClicks (false, false); startTimerHz (30); }
DxTube::~DxTube() { stopTimer(); }
void DxTube::setGlow (float g) { glow = jlimit (0.f, 1.f, g); }

void DxTube::timerCallback()
{
    const float target = glow * (0.92f + 0.08f * rng.nextFloat());
    const float next = shown + (target - shown) * 0.2f;
    if (std::abs (next - shown) > 0.002f) { shown = next; repaint(); }
}

void DxTube::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = jmin (b.getWidth(), b.getHeight() * 0.42f);
    const float h = w / 0.42f;
    auto area = Rectangle<float> (w, h).withCentre (b.getCentre());
    auto base = area.removeFromBottom (h * 0.16f);
    auto glass = area;
    const float cx = glass.getCentreX();
    const float heat = shown;

    // warm light on the plate behind the tube
    if (heat > 0.02f)
    {
        g.setGradientFill (ColourGradient (Colour (0xffFF7A1A).withAlpha (0.45f * heat), cx, glass.getCentreY() + h * 0.1f,
                                           Colour (0xffFF7A1A).withAlpha (0.f), cx + w * 1.2f, glass.getCentreY(), true));
        g.fillEllipse (glass.expanded (w * 0.9f, h * 0.1f));
    }

    // base: black bakelite with a brass band and two visible pins
    Paint::softShadow (g, base, 3.f, 0.5f, 3.f, 2.f);
    g.setGradientFill (ColourGradient (Colour (0xff3A3A3C), base.getX(), 0.f, Colour (0xff0B0B0C), base.getRight(), 0.f, false));
    g.fillRoundedRectangle (base, 3.f);
    g.setColour (Colour (0xffB08D4A).withAlpha (0.8f));
    g.fillRect (base.getX() + 1.f, base.getY() + 1.f, base.getWidth() - 2.f, 2.f);

    // bulb
    Path bulb;
    bulb.addRoundedRectangle (glass.getX(), glass.getY(), glass.getWidth(), glass.getHeight(), w * 0.48f, w * 0.48f, true, true, false, false);
    g.setColour (Colour (0xff101012).withAlpha (0.35f));
    g.fillPath (bulb);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (bulb);
        // getter flash at the top: a silvery mirror
        g.setGradientFill (ColourGradient (Colour (0xffD8D6D0).withAlpha (0.85f), cx, glass.getY(), Colour (0xff4A4844).withAlpha (0.0f), cx, glass.getY() + h * 0.28f, false));
        g.fillRect (glass.withHeight (h * 0.28f));
        // plates: two dark grey rectangles
        auto plates = Rectangle<float> (w * 0.62f, h * 0.36f).withCentre ({ cx, glass.getY() + glass.getHeight() * 0.60f });
        for (int k = 0; k < 2; ++k)
        {
            auto pl = plates.withWidth (plates.getWidth() * 0.44f).withX (plates.getX() + (float) k * plates.getWidth() * 0.56f);
            g.setGradientFill (ColourGradient (Colour (0xff6A6A6E), pl.getX(), 0.f, Colour (0xff2A2A2C), pl.getRight(), 0.f, false));
            g.fillRect (pl);
            g.setColour (Colours::black.withAlpha (0.5f)); g.drawRect (pl, 0.8f);
        }
        // filament glow between the plates
        if (heat > 0.02f)
        {
            const auto fc = Point<float> (cx, plates.getCentreY());
            g.setGradientFill (ColourGradient (Colour (0xffFFD08A).withAlpha (heat), fc.x, fc.y,
                                               Colour (0xffFF5A10).withAlpha (0.f), fc.x + w * 0.45f, fc.y, true));
            g.fillEllipse (Rectangle<float> (w * 0.9f, h * 0.42f).withCentre (fc));
            g.setColour (Colour (0xffFFE2B0).withAlpha (heat));
            g.drawLine (fc.x, plates.getY() + 3.f, fc.x, plates.getBottom() - 3.f, jmax (1.f, w * 0.04f));
        }
        // micas + wires
        g.setColour (Colour (0xffBDB8AE).withAlpha (0.35f));
        g.drawLine (glass.getX() + w * 0.15f, plates.getY() - 3.f, glass.getRight() - w * 0.15f, plates.getY() - 3.f, 1.f);
        g.drawLine (glass.getX() + w * 0.15f, plates.getBottom() + 3.f, glass.getRight() - w * 0.15f, plates.getBottom() + 3.f, 1.f);
        // glass reflections
        g.setColour (Colours::white.withAlpha (0.35f));
        g.fillRoundedRectangle (Rectangle<float> (w * 0.10f, glass.getHeight() * 0.62f).withPosition (glass.getX() + w * 0.14f, glass.getY() + h * 0.12f), w * 0.05f);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (Rectangle<float> (w * 0.06f, glass.getHeight() * 0.45f).withPosition (glass.getRight() - w * 0.22f, glass.getY() + h * 0.2f), w * 0.03f);
    }
    g.setColour (Colours::white.withAlpha (0.28f));
    g.strokePath (bulb, PathStrokeType (1.f));
}

// ============================================================================
// DxCaption
// ============================================================================
DxCaption::DxCaption (String t, Colour c, float sz, float tr) : text (std::move (t)), colour (c), size (sz), tracking (tr)
{
    setInterceptsMouseClicks (false, false);
}

void DxCaption::paint (Graphics& g)
{
    // engraved: the panel's ink unless a colour other than the legacy grey was asked for
    const auto* panel = findParentComponentOfClass<RackPanel>();
    const auto fin = panel != nullptr ? panel->finish() : Dx::Finish::gunmetal();
    const Colour ink = colour == P::labelGrey ? (fin.filledSections ? fin.sectionInk.withAlpha (0.8f) : fin.inkDim) : colour;
    // legacy captions were letter-spaced by hand ("G A I N"): Oswald does the spacing now
    String t = text;
    const auto words = StringArray::fromTokens (t, " ", {});
    int singles = 0; for (auto& w : words) if (w.length() == 1) ++singles;
    if (words.size() >= 5 && singles >= words.size() - 1) t = t.replace ("   ", "|").removeCharacters (" ").replace ("|", " ");
    Paint::engrave (g, t.toUpperCase(), getLocalBounds().toFloat(), ink, Fonts::engraved (jmax (9.f, size + 2.5f)),
                    Justification::centred, fin.controlSurfaceLight(), jmin (0.2f, tracking));
}

// ============================================================================
// DxEditorBase
// ============================================================================
DxEditorBase::DxEditorBase (AudioProcessor& p, String title, String subtitle, RackPanel::Style style, Colour titleColour, bool italicTitle)
    : AudioProcessorEditor (p), panel (std::move (title), std::move (subtitle), style, titleColour, italicTitle), tooltips (this, 600)
{
    initialise();
}

DxEditorBase::DxEditorBase (AudioProcessor& p, String title, String subtitle, const Dx::Finish& f)
    : AudioProcessorEditor (p), panel (std::move (title), std::move (subtitle), f), tooltips (this, 600)
{
    initialise();
}

void DxEditorBase::initialise()
{
    setLookAndFeel (&lnf);
    setOpaque (true);
    addAndMakeVisible (panel);
   #if DX_HAS_UPDATER && defined (JucePlugin_VersionString)
    {
        dtupdate::Style st;
        st.pillFill = P::orange; st.pillText = Colours::white;
        st.panelFill = P::ink; st.panelText = P::cream; st.panelDim = P::labelGrey; st.accent = P::orange;
        st.font = Fonts::engraved (11.f);
        st.corner = 3.f;
        updatePill = std::make_unique<dtupdate::UpdatePill> (DX_UPDATE_ID, JucePlugin_VersionString, st);
        panel.addChildComponent (*updatePill);
    }
   #endif
    startTimerHz (30);
}

DxEditorBase::~DxEditorBase()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void DxEditorBase::setMinZoom (float z) { minZoom = jlimit (0.4f, 1.f, z); }

void DxEditorBase::setBaseSize (int w, int h)
{
    baseW = w; baseH = h;
    setResizable (true, true);
    setResizeLimits (roundToInt ((float) w * minZoom), roundToInt ((float) h * minZoom),
                     roundToInt ((float) w * kMaxZoom), roundToInt ((float) h * kMaxZoom));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
    setSize (w, h);
}

void DxEditorBase::paint (Graphics& g)
{
    g.fillAll (P::ink);
}

void DxEditorBase::resized()
{
    if (baseW <= 0 || baseH <= 0) return;
    zoom = jlimit (minZoom, kMaxZoom, (float) getWidth() / (float) baseW);
    panel.setTransform (AffineTransform());
    panel.setBounds (0, 0, baseW, baseH);
    panel.setTransform (AffineTransform::scale (zoom));
   #if DX_HAS_UPDATER
    if (updatePill != nullptr)
    {
        // header, just left of the IN lamp button
        const auto t = panel.toggleArea();
        const int h = 20, w = jmax (92, updatePill->preferredWidth (h));
        updatePill->setBounds (Rectangle<int> (t.getX() - 10 - w, t.getCentreY() - h / 2, w, h));
    }
   #endif
}
