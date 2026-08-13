// =============================================================================
//  OLIVERB — PluginProcessor.h
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "dsp/PassiveHighPass.h"
#include "dsp/TapeEcho.h"
#include "dsp/SpringReverb.h"
#include "dsp/Modulation.h"

class OliverbProcessor : public juce::AudioProcessor
{
public:
    OliverbProcessor();
    ~OliverbProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;   // keep the double-precision overload visible

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    juce::AudioProcessorParameter* getBypassParameter() const override { return bypassParam; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "OLIVERB",
                                              wh::createParameterLayout() };

    /** Frequency currently selected on the big dial, for the editor's readout. */
    float currentCornerHz() const noexcept;

    /** Post-filter output level, 0..1, for the editor's meter. */
    std::atomic<float> meterL { 0.0f }, meterR { 0.0f };

private:
    void pullParameters();

    template <typename T>
    T* getRaw (const char* id) const
    {
        return dynamic_cast<T*> (apvts.getParameter (id));
    }

    // Cached parameter pointers (looking these up by string per block is wasteful)
    juce::AudioParameterBool*   bypassParam   = nullptr;
    juce::AudioParameterBool*   filterOnP     = nullptr;
    juce::AudioParameterBool*   filterPostP   = nullptr;
    juce::AudioParameterChoice* filterTypeP   = nullptr;
    juce::AudioParameterInt*    filterStepP   = nullptr;
    juce::AudioParameterBool*   echoOnP       = nullptr;
    juce::AudioParameterBool*   echoSyncP     = nullptr;
    juce::AudioParameterBool*   echoSendP     = nullptr;
    juce::AudioParameterChoice* echoDivP      = nullptr;
    juce::AudioParameterBool*   springOnP     = nullptr;
    juce::AudioParameterBool*   lfoOnP        = nullptr;
    juce::AudioParameterBool*   lfoSyncP      = nullptr;
    juce::AudioParameterChoice* lfoShapeP     = nullptr;
    juce::AudioParameterChoice* lfoDivP       = nullptr;

    std::atomic<float>* pImpedance = nullptr;
    std::atomic<float>* pMagnetism = nullptr;
    std::atomic<float>* pCharacter = nullptr;
    std::atomic<float>* pDynamics  = nullptr;
    std::atomic<float>* pArtefacts = nullptr;
    std::atomic<float>* pFilterGain = nullptr;
    std::atomic<float>* pEchoTime  = nullptr;
    std::atomic<float>* pEchoFb    = nullptr;
    std::atomic<float>* pEchoIn    = nullptr;
    std::atomic<float>* pEchoOut   = nullptr;
    std::atomic<float>* pEchoHiss  = nullptr;
    std::atomic<float>* pEchoMix   = nullptr;
    std::atomic<float>* pEchoAge   = nullptr;
    std::atomic<float>* pSpringAmt = nullptr;
    std::atomic<float>* pSpringDecay = nullptr;
    std::atomic<float>* pSpringDrive = nullptr;
    std::atomic<float>* pLfoRate   = nullptr;
    std::atomic<float>* pLfoDepth  = nullptr;
    std::atomic<float>* pEnvDepth  = nullptr;
    std::atomic<float>* pEnvSens   = nullptr;
    std::atomic<float>* pEnvSpeed  = nullptr;
    std::atomic<float>* pOutput    = nullptr;

    // DSP
    wh::PassiveHighPass filterL, filterR;
    wh::TapeEcho        echo;
    wh::SpringReverb    spring;
    wh::LFO             lfo;
    wh::EnvFollower     modEnv;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::SmoothedValue<float> outputSmooth;

    double hostBpm = 120.0;
    double hostPpq = 0.0;
    bool   hostPlaying = false;
    int    currentProgram = 0;
    bool   prepared = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OliverbProcessor)
};
