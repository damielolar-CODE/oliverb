// =============================================================================
//  OLIVERB — PluginEditor.h
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "LookAndFeel.h"
#include "Updates/DeestechUpdate.h"

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
    explicit LevelMeter (OliverbProcessor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    OliverbProcessor& proc;
    float displayL = 0.0f, displayR = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LevelMeter)
};

// -----------------------------------------------------------------------------
/** The valve's filament lamp: glows brighter the harder the stage works. */
class GlowLamp : public juce::Component, private juce::Timer
{
public:
    explicit GlowLamp (OliverbProcessor&);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    OliverbProcessor& proc;
    float display = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlowLamp)
};

// -----------------------------------------------------------------------------
class OliverbEditor : public juce::AudioProcessorEditor,
                         private juce::Timer
{
public:
    explicit OliverbEditor (OliverbProcessor&);
    ~OliverbEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void layoutContent();
    void refreshPresetBox();

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    OliverbProcessor& proc;
    OliverbLNF lnf;

    juce::Component content;    // laid out at a fixed design size, then scaled

    // Header
    juce::ComboBox   presetBox;
    juce::ToggleButton bypassButton { "BYPASS" };
    std::unique_ptr<ButtonAtt> bypassAtt;
    std::unique_ptr<KnobBox> outputKnob;
    std::unique_ptr<LevelMeter> meter;

    // Valve (v2)
    juce::ToggleButton valveOnButton { "VALVE" }, valvePostButton { "OUTPUT" }, valveEchoButton { "ECHO AMP" };
    std::unique_ptr<ButtonAtt> valveOnAtt, valvePostAtt, valveEchoAtt;
    std::unique_ptr<KnobBox> valveDrive, valveBias, valveSag, valveTone, valveMix;
    std::unique_ptr<GlowLamp> glowLamp;

    // UPDATE x.y.z pill next to the MK II legend: invisible until deestechholdings.com/updates.json lists a newer OLIVERB
    std::unique_ptr<dtupdate::UpdatePill> updatePill;

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

    // Modulation
    juce::ToggleButton lfoOnButton { "LFO" }, lfoSyncButton { "SYNC" };
    std::unique_ptr<ButtonAtt> lfoOnAtt, lfoSyncAtt;
    juce::ComboBox shapeBox, lfoDivBox;
    std::unique_ptr<ComboAtt> shapeAtt, lfoDivAtt;
    std::unique_ptr<KnobBox> lfoRate, lfoDepth, envDepth, envSens, envSpeed;

    // Spring
    juce::ToggleButton springOnButton { "SPRING" };
    std::unique_ptr<ButtonAtt> springOnAtt;
    std::unique_ptr<KnobBox> springAmt, springDecay, springDrive;

    juce::Label cornerReadout;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OliverbEditor)
};
