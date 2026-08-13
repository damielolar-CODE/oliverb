// =============================================================================
//  OLIVERB — Parameters.h
//  Single source of truth for every parameter ID, range and default.
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace pid
{
// Filter
inline constexpr const char* filterOn    = "filterOn";
inline constexpr const char* filterType  = "filterType";
inline constexpr const char* filterStep  = "filterStep";
inline constexpr const char* impedance   = "impedance";
inline constexpr const char* magnetism   = "magnetism";
inline constexpr const char* character   = "character";
inline constexpr const char* dynamics    = "dynamics";
inline constexpr const char* artefacts   = "artefacts";
inline constexpr const char* filterGain  = "filterGain";
inline constexpr const char* filterPost  = "filterPost";

// Echo
inline constexpr const char* echoOn      = "echoOn";
inline constexpr const char* echoSync    = "echoSync";
inline constexpr const char* echoTime    = "echoTime";
inline constexpr const char* echoDiv     = "echoDiv";
inline constexpr const char* echoFb      = "echoFeedback";
inline constexpr const char* echoSend    = "echoSend";
inline constexpr const char* echoIn      = "echoInput";
inline constexpr const char* echoOut     = "echoOutput";
inline constexpr const char* echoHiss    = "echoHiss";
inline constexpr const char* echoMix     = "echoMix";
inline constexpr const char* echoAge     = "echoAge";

// Spring
inline constexpr const char* springOn    = "springOn";
inline constexpr const char* springAmt   = "springAmount";
inline constexpr const char* springDecay = "springDecay";
inline constexpr const char* springDrive = "springDrive";

// Global
inline constexpr const char* outputGain  = "outputGain";
inline constexpr const char* bypass      = "bypass";
} // namespace pid

namespace wh
{

/** Note divisions for the tempo-synced echo, expressed in quarter notes. */
inline const juce::StringArray& divisionNames()
{
    static const juce::StringArray names
        { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2", "1/1" };
    return names;
}

inline float divisionInQuarterNotes (int index)
{
    static const float v[] = { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f,
                               1.0f, 1.5f, 2.0f, 4.0f };
    const int n = static_cast<int> (sizeof (v) / sizeof (v[0]));
    return v[juce::jlimit (0, n - 1, index)];
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (float v) { return String (roundToInt (v * 100.0f)) + " %"; };

    auto addFloat = [&] (const char* id, const String& name, NormalisableRange<float> range,
                         float def, std::function<String (float, int)> fmt = {})
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fmt))));
    };

    auto addBool = [&] (const char* id, const String& name, bool def,
                        const String& onText = "On", const String& offText = "Off")
    {
        layout.add (std::make_unique<AudioParameterBool> (
            ParameterID { id, 1 }, name, def,
            AudioParameterBoolAttributes().withStringFromValueFunction (
                [onText, offText] (bool v, int) { return v ? onText : offText; })));
    };

    // ---- Filter -------------------------------------------------------------
    addBool (pid::filterOn, "Filter On", true);

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { pid::filterType, 1 }, "Filter Type", StringArray { "A", "B" }, 0));

    layout.add (std::make_unique<AudioParameterInt> (
        ParameterID { pid::filterStep, 1 }, "Frequency", 1, 11, 1));

    addFloat (pid::impedance, "Impedance", { 0.0f, 1.0f }, 0.35f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::magnetism, "Magnetism", { 0.0f, 1.0f }, 0.30f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::character, "Character", { 0.0f, 1.0f }, 0.25f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::dynamics, "Dynamics", { 0.0f, 1.0f }, 0.30f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::artefacts, "Artefacts", { 0.0f, 1.0f }, 0.25f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::filterGain, "Filter Gain", { -18.0f, 18.0f, 0.1f }, 0.0f,
              [] (float v, int) { return String (v, 1) + " dB"; });

    addBool (pid::filterPost, "Filter Position", false, "Post", "Pre");

    // ---- Echo ---------------------------------------------------------------
    addBool (pid::echoOn, "Echo On", true);
    addBool (pid::echoSync, "Echo Sync", false, "Sync", "Free");

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { pid::echoTime, 1 }, "Echo Time",
        NormalisableRange<float> { 20.0f, 2000.0f, 0.01f, 0.35f }, 375.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float v, int) { return String (v, v < 100.0f ? 1 : 0) + " ms"; })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { pid::echoDiv, 1 }, "Echo Division", divisionNames(), 5));

    addFloat (pid::echoFb, "Feedback", { 0.0f, 1.25f }, 0.42f,
              [] (float v, int) { return String (roundToInt (v * 80.0f)) + " %"; });
    addBool (pid::echoSend, "Send", true, "Dub", "Held");
    addFloat (pid::echoIn, "Echo Input", { 0.0f, 1.5f }, 0.85f,
              [] (float v, int) { return String (roundToInt (v * 66.7f)) + " %"; });
    addFloat (pid::echoOut, "Echo Output", { 0.0f, 1.5f }, 0.9f,
              [] (float v, int) { return String (roundToInt (v * 66.7f)) + " %"; });
    addFloat (pid::echoHiss, "Hiss", { 0.0f, 1.0f }, 0.18f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::echoMix, "Echo Mix", { 0.0f, 1.0f }, 0.32f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::echoAge, "Wear", { 0.0f, 1.0f }, 0.45f,
              [pct] (float v, int) { return pct (v); });

    // ---- Spring -------------------------------------------------------------
    addBool (pid::springOn, "Spring On", true);
    addFloat (pid::springAmt, "Spring", { 0.0f, 1.0f }, 0.22f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::springDecay, "Tension", { 0.0f, 1.0f }, 0.55f,
              [pct] (float v, int) { return pct (v); });
    addFloat (pid::springDrive, "Tank Drive", { 0.0f, 1.0f }, 0.25f,
              [pct] (float v, int) { return pct (v); });

    // ---- Global -------------------------------------------------------------
    addFloat (pid::outputGain, "Output", { -24.0f, 12.0f, 0.1f }, 0.0f,
              [] (float v, int) { return String (v, 1) + " dB"; });
    addBool (pid::bypass, "Bypass", false);

    return layout;
}

} // namespace wh
