// =============================================================================
//  OLIVERB — LookAndFeel.h
//  A 1960s console faceplate: warm black crackle paint, cream silkscreen,
//  oxidised brass, and one very large red knob.
// =============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace wh::colours
{
inline const juce::Colour background   { 0xff141110 };
inline const juce::Colour panel        { 0xff1d1917 };
inline const juce::Colour panelEdge    { 0xff2e2723 };
inline const juce::Colour cream        { 0xffe6dac4 };
inline const juce::Colour creamDim     { 0xff8f8474 };
inline const juce::Colour red          { 0xffb8372b };
inline const juce::Colour redBright    { 0xffd9503f };
inline const juce::Colour brass        { 0xffc9a227 };
inline const juce::Colour green        { 0xff4f8a5b };
inline const juce::Colour knobBody     { 0xff23201d };
inline const juce::Colour knobEdge     { 0xff3a332e };
} // namespace wh::colours

class OliverbLNF : public juce::LookAndFeel_V4
{
public:
    OliverbLNF();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawLabel (juce::Graphics&, juce::Label&) override;

    /** Shared helpers used by the editor's custom painting. */
    static void paintPanel (juce::Graphics&, juce::Rectangle<float> bounds,
                            const juce::String& title, juce::Colour accent);
    static void paintKnobBody (juce::Graphics&, juce::Rectangle<float> area,
                               juce::Colour cap, float angle, bool bigCap);
    static juce::Font faceFont (float height, bool bold = false);
};
