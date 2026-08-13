#include "LookAndFeel.h"

using namespace wh::colours;

// =============================================================================
juce::Font WaterhouseLNF::faceFont (float height, bool bold)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                          height,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

WaterhouseLNF::WaterhouseLNF()
{
    setColour (juce::Slider::textBoxTextColourId, cream);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, red.withAlpha (0.35f));
    setColour (juce::Label::textColourId, cream);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff100e0d));
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, knobEdge);
    setColour (juce::ComboBox::arrowColourId, brass);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff171413));
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, red.withAlpha (0.55f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff232019));
    setColour (juce::TextButton::textColourOffId, creamDim);
    setColour (juce::TextButton::textColourOnId, cream);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff171413));
    setColour (juce::TooltipWindow::textColourId, cream);
}

juce::Font WaterhouseLNF::getLabelFont (juce::Label& l) { return faceFont (l.getFont().getHeight()); }
juce::Font WaterhouseLNF::getComboBoxFont (juce::ComboBox&) { return faceFont (13.0f); }
juce::Font WaterhouseLNF::getTextButtonFont (juce::TextButton&, int h)
{
    return faceFont (juce::jmin (14.0f, h * 0.58f), true);
}

void WaterhouseLNF::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setColour (label.findColour (juce::Label::textColourId));
    g.setFont (getLabelFont (label));
    g.drawFittedText (label.getText(), label.getLocalBounds(),
                      label.getJustificationType(), 2, 0.9f);
}

// =============================================================================
void WaterhouseLNF::paintKnobBody (juce::Graphics& g, juce::Rectangle<float> area,
                                   juce::Colour cap, float angle, bool bigCap)
{
    const auto centre = area.getCentre();
    const float radius = area.getWidth() * 0.5f;

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (area.translated (0.0f, radius * 0.09f).reduced (radius * 0.02f));

    // Body: brushed metal, lit from above
    juce::ColourGradient body (cap.brighter (0.35f), centre.x, area.getY(),
                               cap.darker (0.55f), centre.x, area.getBottom(), false);
    body.addColour (0.55, cap);
    g.setGradientFill (body);
    g.fillEllipse (area);

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (area, radius * 0.045f);

    // Inset skirt
    const auto skirt = area.reduced (radius * (bigCap ? 0.26f : 0.30f));
    g.setColour (cap.darker (0.75f).withAlpha (0.55f));
    g.drawEllipse (skirt, radius * 0.03f);

    // Pointer
    juce::Path p;
    const float len = radius * (bigCap ? 0.86f : 0.80f);
    const float w   = radius * (bigCap ? 0.075f : 0.09f);
    p.addRoundedRectangle (-w * 0.5f, -len, w, len * (bigCap ? 0.58f : 0.52f), w * 0.5f);
    g.setColour (cream);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (centre));

    // Centre cap
    g.setColour (cap.darker (0.6f));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.30f, radius * 0.30f).withCentre (centre));
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillEllipse (juce::Rectangle<float> (radius * 0.22f, radius * 0.16f)
                       .withCentre (centre.translated (0.0f, -radius * 0.05f)));
}

void WaterhouseLNF::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                      float sliderPos, float startAngle, float endAngle,
                                      juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto area = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());

    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const auto centre = area.getCentre();
    const float radius = size * 0.5f;

    // Track + value arc
    const float arcR = radius * 1.14f;
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2a2522));
    g.strokePath (track, juce::PathStrokeType (radius * 0.10f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (value, juce::PathStrokeType (radius * 0.10f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }

    paintKnobBody (g, area.reduced (size * 0.10f), knobBody, angle, false);
}

// =============================================================================
void WaterhouseLNF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                      bool isHighlighted, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const float corner = juce::jmin (5.0f, r.getHeight() * 0.28f);

    juce::ColourGradient bg (on ? red.brighter (0.15f) : juce::Colour (0xff211d1a),
                             r.getCentreX(), r.getY(),
                             on ? red.darker (0.35f) : juce::Colour (0xff171413),
                             r.getCentreX(), r.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (r, corner);

    g.setColour (on ? redBright.withAlpha (0.9f) : knobEdge);
    g.drawRoundedRectangle (r, corner, 1.2f);

    if (isHighlighted || isDown)
    {
        g.setColour (juce::Colours::white.withAlpha (isDown ? 0.10f : 0.05f));
        g.fillRoundedRectangle (r, corner);
    }

    // Indicator lamp
    auto lamp = juce::Rectangle<float> (6.0f, 6.0f)
                    .withCentre ({ r.getX() + 11.0f, r.getCentreY() });
    g.setColour (on ? juce::Colour (0xfff2d06b) : juce::Colour (0xff332c27));
    g.fillEllipse (lamp);
    if (on)
    {
        g.setColour (juce::Colour (0xfff2d06b).withAlpha (0.25f));
        g.fillEllipse (lamp.expanded (3.5f));
    }

    g.setColour (on ? juce::Colours::white.withAlpha (0.95f) : creamDim);
    g.setFont (faceFont (juce::jmin (13.0f, r.getHeight() * 0.52f), true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (18.0f).withTrimmedRight (4.0f),
                juce::Justification::centred, false);
}

void WaterhouseLNF::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                          const juce::Colour& backgroundColour,
                                          bool isHighlighted, bool isDown)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = juce::jmin (5.0f, r.getHeight() * 0.28f);

    g.setColour (backgroundColour.withMultipliedBrightness (isDown ? 0.8f : (isHighlighted ? 1.25f : 1.0f)));
    g.fillRoundedRectangle (r, corner);
    g.setColour (knobEdge);
    g.drawRoundedRectangle (r, corner, 1.0f);
}

void WaterhouseLNF::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                  int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    juce::Path arrow;
    const float cx = r.getRight() - 13.0f, cy = r.getCentreY();
    arrow.addTriangle (cx - 4.5f, cy - 2.5f, cx + 4.5f, cy - 2.5f, cx, cy + 3.5f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId).withAlpha (box.isEnabled() ? 0.95f : 0.3f));
    g.fillPath (arrow);
}

// =============================================================================
void WaterhouseLNF::paintPanel (juce::Graphics& g, juce::Rectangle<float> bounds,
                                const juce::String& title, juce::Colour accent)
{
    juce::ColourGradient grad (panel.brighter (0.05f), bounds.getX(), bounds.getY(),
                               panel.darker (0.25f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (bounds, 7.0f);

    g.setColour (panelEdge);
    g.drawRoundedRectangle (bounds, 7.0f, 1.2f);

    // Accent stripe down the left edge — the coloured tape on a patch strip
    juce::Path stripe;
    stripe.addRoundedRectangle (bounds.getX() + 1.5f, bounds.getY() + 1.5f,
                                4.0f, bounds.getHeight() - 3.0f, 2.0f);
    g.setColour (accent.withAlpha (0.85f));
    g.fillPath (stripe);

    g.setColour (accent.withAlpha (0.9f));
    g.setFont (faceFont (12.0f, true));
    g.drawText (title.toUpperCase(),
                bounds.withTrimmedLeft (16.0f).withHeight (22.0f).translated (0.0f, 6.0f),
                juce::Justification::centredLeft, false);
}
