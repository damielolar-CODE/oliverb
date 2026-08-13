// =============================================================================
//  Waterhouse — PluginEditor.h
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "LookAndFeel.h"

// -----------------------------------------------------------------------------
/** A knob with its silkscreen caption above and its value underneath. */
class KnobBox : public juce::Component
{
public:
    KnobBox (const juce::String& caption, juce::Colour accent);

    void attach (juce::AudioProcessorValueTreeState& state, const juce::String& paramID);
    void resized() override;
    void paint (juce::Graphics&) override;

    juce::Slider slider;

private:
    juce::String caption;
    juce::Colour accent;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KnobBox)
};

// -----------------------------------------------------------------------------
/** The big red one. Eleven detented positions, frequencies silkscreened
    around the skirt, current corner in the middle. */
class BigDial : public juce::Slider
{
public:
    explicit BigDial (std::function<int()> bankProvider);

    void paint (juce::Graphics&) override;

private:
    std::function<int()> getBank;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BigDial)
};

// -----------------------------------------------------------------------------
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (WaterhouseProcessor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    WaterhouseProcessor& proc;
    float displayL = 0.0f, displayR = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

// -----------------------------------------------------------------------------
class WaterhouseEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit WaterhouseEditor (WaterhouseProcessor&);
    ~WaterhouseEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutContent();
    void refreshPresetBox();

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    WaterhouseProcessor& proc;
    WaterhouseLNF lnf;

    juce::Component content;    // laid out at a fixed design size, then scaled

    // Header
    juce::ComboBox   presetBox;
    juce::ToggleButton bypassButton { "BYPASS" };
    std::unique_ptr<ButtonAtt> bypassAtt;
    std::unique_ptr<KnobBox> outputKnob;
    std::unique_ptr<LevelMeter> meter;

    // Filter
    juce::ToggleButton filterOnButton { "FILTER" }, typeButton { "BANK B" }, postButton { "POST" };
    std::unique_ptr<ButtonAtt> filterOnAtt, typeAtt, postAtt;
    std::unique_ptr<BigDial> bigDial;
    std::unique_ptr<SliderAtt> bigDialAtt;
    std::unique_ptr<KnobBox> impedance, magnetism, character, dynamics, artefacts, filterGain;

    // Echo
    juce::ToggleButton echoOnButton { "ECHO" }, syncButton { "SYNC" }, sendButton { "DUB SEND" };
    std::unique_ptr<ButtonAtt> echoOnAtt, syncAtt, sendAtt;
    juce::ComboBox divBox;
    std::unique_ptr<ComboAtt> divAtt;
    std::unique_ptr<KnobBox> echoTime, echoFb, echoIn, echoOut, echoHiss, echoMix, echoAge;

    // Spring
    juce::ToggleButton springOnButton { "SPRING" };
    std::unique_ptr<ButtonAtt> springOnAtt;
    std::unique_ptr<KnobBox> springAmt, springDecay, springDrive;

    juce::Label cornerReadout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaterhouseEditor)
};
