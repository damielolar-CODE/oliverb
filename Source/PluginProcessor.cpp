// =============================================================================
//  OLIVERB — PluginProcessor.cpp
//
//  Signal flow (all of it running at 2x, so the non-linear stages fold their
//  harmonics somewhere other than back into the audible band):
//
//      in -> [FILTER if Pre] -> ECHO -> SPRING -> [FILTER if Post] -> out
//
//  The Pre/Post switch matters more than it looks. Pre = the filter feeds the
//  echo, so the repeats inherit whatever the dial is doing — sweep the dial
//  and the tail sweeps with it. Post = the filter sits across the whole
//  finished thing, echo tails included, and sweeping it drags everything
//  under at once. Tubby used both; they are different instruments.
// =============================================================================

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{

struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;   // real-world units
};

const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets =
    {
        { "Init", {} },

        { "Waterhouse Rockers", {
            { pid::filterStep, 4 }, { pid::filterType, 0 }, { pid::impedance, 0.55f },
            { pid::magnetism, 0.35f }, { pid::character, 0.30f }, { pid::filterPost, 0 },
            { pid::echoTime, 375.0f }, { pid::echoFb, 0.55f }, { pid::echoMix, 0.35f },
            { pid::echoAge, 0.5f }, { pid::springAmt, 0.25f } } },

        { "Snare Throw", {
            { pid::filterStep, 6 }, { pid::impedance, 0.7f }, { pid::filterPost, 0 },
            { pid::echoTime, 300.0f }, { pid::echoFb, 0.85f }, { pid::echoMix, 0.6f },
            { pid::echoIn, 1.1f }, { pid::springAmt, 0.35f }, { pid::springDrive, 0.4f } } },

        { "Big Dial Sweep", {
            { pid::filterStep, 7 }, { pid::impedance, 1.0f }, { pid::magnetism, 0.8f },
            { pid::dynamics, 0.7f }, { pid::character, 0.6f }, { pid::artefacts, 0.7f },
            { pid::echoMix, 0.2f }, { pid::springAmt, 0.15f } } },

        { "Tape Wash", {
            { pid::filterStep, 2 }, { pid::impedance, 0.25f },
            { pid::echoTime, 700.0f }, { pid::echoFb, 0.72f }, { pid::echoMix, 0.5f },
            { pid::echoHiss, 0.45f }, { pid::echoAge, 0.9f }, { pid::springAmt, 0.3f },
            { pid::springDecay, 0.75f } } },

        { "Held Echo (Dub Out)", {
            { pid::echoSend, 0 }, { pid::echoFb, 1.0f }, { pid::echoMix, 1.0f },
            { pid::echoOut, 1.0f }, { pid::filterStep, 3 }, { pid::impedance, 0.6f },
            { pid::filterPost, 1 }, { pid::springAmt, 0.4f } } },

        { "Spring Crash", {
            { pid::springAmt, 0.85f }, { pid::springDecay, 0.9f }, { pid::springDrive, 0.7f },
            { pid::echoMix, 0.15f }, { pid::filterStep, 2 } } },

        { "Roots Bass Tighten", {
            { pid::filterStep, 1 }, { pid::impedance, 0.15f }, { pid::magnetism, 0.5f },
            { pid::character, 0.35f }, { pid::echoMix, 0.0f }, { pid::springAmt, 0.0f },
            { pid::filterGain, 2.0f } } },

        { "Siren", {
            { pid::filterStep, 9 }, { pid::impedance, 1.0f }, { pid::magnetism, 1.0f },
            { pid::dynamics, 1.0f }, { pid::character, 0.8f }, { pid::artefacts, 1.0f },
            { pid::echoTime, 180.0f }, { pid::echoFb, 0.9f }, { pid::echoMix, 0.6f },
            { pid::springAmt, 0.5f } } },
    };
    return presets;
}

} // namespace

// =============================================================================
OliverbProcessor::OliverbProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    bypassParam = getRaw<juce::AudioParameterBool> (pid::bypass);
    filterOnP   = getRaw<juce::AudioParameterBool> (pid::filterOn);
    filterPostP = getRaw<juce::AudioParameterBool> (pid::filterPost);
    filterTypeP = getRaw<juce::AudioParameterChoice> (pid::filterType);
    filterStepP = getRaw<juce::AudioParameterInt> (pid::filterStep);
    echoOnP     = getRaw<juce::AudioParameterBool> (pid::echoOn);
    echoSyncP   = getRaw<juce::AudioParameterBool> (pid::echoSync);
    echoSendP   = getRaw<juce::AudioParameterBool> (pid::echoSend);
    echoDivP    = getRaw<juce::AudioParameterChoice> (pid::echoDiv);
    springOnP   = getRaw<juce::AudioParameterBool> (pid::springOn);

    pImpedance   = apvts.getRawParameterValue (pid::impedance);
    pMagnetism   = apvts.getRawParameterValue (pid::magnetism);
    pCharacter   = apvts.getRawParameterValue (pid::character);
    pDynamics    = apvts.getRawParameterValue (pid::dynamics);
    pArtefacts   = apvts.getRawParameterValue (pid::artefacts);
    pFilterGain  = apvts.getRawParameterValue (pid::filterGain);
    pEchoTime    = apvts.getRawParameterValue (pid::echoTime);
    pEchoFb      = apvts.getRawParameterValue (pid::echoFb);
    pEchoIn      = apvts.getRawParameterValue (pid::echoIn);
    pEchoOut     = apvts.getRawParameterValue (pid::echoOut);
    pEchoHiss    = apvts.getRawParameterValue (pid::echoHiss);
    pEchoMix     = apvts.getRawParameterValue (pid::echoMix);
    pEchoAge     = apvts.getRawParameterValue (pid::echoAge);
    pSpringAmt   = apvts.getRawParameterValue (pid::springAmt);
    pSpringDecay = apvts.getRawParameterValue (pid::springDecay);
    pSpringDrive = apvts.getRawParameterValue (pid::springDrive);
    pOutput      = apvts.getRawParameterValue (pid::outputGain);
}

// =============================================================================
void OliverbProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampler->reset();
    setLatencySamples (juce::roundToInt (oversampler->getLatencyInSamples()));

    const double osRate = sampleRate * 2.0;

    filterL.prepare (osRate);
    filterR.prepare (osRate);
    echo.prepare (osRate);
    spring.prepare (osRate);

    echo.snapTimeMs (pEchoTime->load());

    outputSmooth.reset (sampleRate, 0.02);
    outputSmooth.setCurrentAndTargetValue (wh::dbToGain (pOutput->load()));

    prepared = true;
}

bool OliverbProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return in == out;
}

// =============================================================================
void OliverbProcessor::pullParameters()
{
    const int type = filterTypeP->getIndex();
    const int step = filterStepP->get() - 1;

    for (auto* f : { &filterL, &filterR })
    {
        f->setStep (type, step);
        f->setImpedance (pImpedance->load());
        f->setMagnetism (pMagnetism->load());
        f->setCharacter (pCharacter->load());
        f->setDynamics (pDynamics->load());
        f->setArtefacts (pArtefacts->load());
        f->setGain (pFilterGain->load());
    }

    float timeMs = pEchoTime->load();
    if (echoSyncP->get())
    {
        const float quarters = wh::divisionInQuarterNotes (echoDivP->getIndex());
        timeMs = static_cast<float> (60000.0 / juce::jmax (20.0, hostBpm)) * quarters;
        timeMs = juce::jlimit (wh::TapeEcho::kMinTimeMs, wh::TapeEcho::kMaxTimeMs, timeMs);
    }

    echo.setTimeMs (timeMs);
    echo.setFeedback (pEchoFb->load());
    echo.setInputLevel (pEchoIn->load());
    echo.setOutputLevel (pEchoOut->load());
    echo.setHiss (pEchoHiss->load());
    echo.setMix (echoOnP->get() ? pEchoMix->load() : 0.0f);
    echo.setAge (pEchoAge->load());
    echo.setSend (echoSendP->get());

    spring.setAmount (springOnP->get() ? pSpringAmt->load() : 0.0f);
    spring.setDecay (pSpringDecay->load());
    spring.setDrive (pSpringDrive->load());

    outputSmooth.setTargetValue (wh::dbToGain (pOutput->load()));
}

float OliverbProcessor::currentCornerHz() const noexcept
{
    return wh::PassiveHighPass::stepFrequency (filterTypeP->getIndex(), filterStepP->get() - 1);
}

// =============================================================================
void OliverbProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (! prepared || numSamples == 0)
        return;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                hostBpm = *bpm;

    pullParameters();

    const bool bypassed = bypassParam != nullptr && bypassParam->get();
    const bool useFilter = filterOnP->get();
    const bool filterAtEnd = filterPostP->get();

    // The oversampler is always fed two channels so mono and stereo hosts
    // share one code path (and one reported latency).
    juce::AudioBuffer<float> stereo (2, numSamples);
    stereo.copyFrom (0, 0, buffer, 0, 0, numSamples);
    stereo.copyFrom (1, 0, buffer, numIn > 1 ? 1 : 0, 0, numSamples);

    juce::dsp::AudioBlock<float> block (stereo);
    auto up = oversampler->processSamplesUp (block);

    if (! bypassed)
    {
        auto* L = up.getChannelPointer (0);
        auto* R = up.getChannelPointer (1);
        const int n = static_cast<int> (up.getNumSamples());

        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];

            if (useFilter && ! filterAtEnd)
            {
                l = filterL.process (l);
                r = filterR.process (r);
            }

            echo.process (l, r);
            spring.process (l, r);

            if (useFilter && filterAtEnd)
            {
                l = filterL.process (l);
                r = filterR.process (r);
            }

            L[i] = l;
            R[i] = r;
        }
    }

    oversampler->processSamplesDown (block);

    // Output trim + safety clamp. A passive filter with the termination wide
    // open plus a hot echo loop can easily hand back more than it was given.
    float peakL = 0.0f, peakR = 0.0f;
    auto* outL = stereo.getWritePointer (0);
    auto* outR = stereo.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        const float g = bypassed ? 1.0f : outputSmooth.getNextValue();
        float l = outL[i] * g;
        float r = outR[i] * g;

        if (! std::isfinite (l)) l = 0.0f;
        if (! std::isfinite (r)) r = 0.0f;

        l = juce::jlimit (-4.0f, 4.0f, l);
        r = juce::jlimit (-4.0f, 4.0f, r);

        outL[i] = l;
        outR[i] = r;
        peakL = juce::jmax (peakL, std::abs (l));
        peakR = juce::jmax (peakR, std::abs (r));
    }

    buffer.copyFrom (0, 0, stereo, 0, 0, numSamples);
    if (numOut > 1)
        buffer.copyFrom (1, 0, stereo, 1, 0, numSamples);

    // Meter ballistics live in the editor; this is just a per-block peak.
    meterL.store (peakL);
    meterR.store (peakR);
}

// =============================================================================
juce::AudioProcessorEditor* OliverbProcessor::createEditor()
{
    return new OliverbEditor (*this);
}

// =============================================================================
int OliverbProcessor::getNumPrograms() { return static_cast<int> (factoryPresets().size()); }

const juce::String OliverbProcessor::getProgramName (int index)
{
    const auto& p = factoryPresets();
    return juce::isPositiveAndBelow (index, static_cast<int> (p.size()))
               ? juce::String (p[static_cast<size_t> (index)].name)
               : juce::String();
}

void OliverbProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow (index, static_cast<int> (presets.size())))
        return;

    currentProgram = index;

    // Every preset starts from the defaults, then applies its own deltas —
    // otherwise presets inherit whatever was left behind by the last one.
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp->paramID != pid::bypass)
                rp->setValueNotifyingHost (rp->getDefaultValue());

    for (const auto& [id, value] : presets[static_cast<size_t> (index)].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (value));
}

// =============================================================================
void OliverbProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void OliverbProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// =============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OliverbProcessor();
}
