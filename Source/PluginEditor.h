// =============================================================================
//  OLIVERB — PluginEditor.h — STUDIO SERIES faceplate (the Deestech rack kit in Source/ui, synced from
//  Deestronix/Plugins/Common by Deestronix/scripts/sync_kit.sh). Finish: oxblood dub desk, walnut cheeks.
//  A channel strip read top to bottom in signal order; each module keyed by its cap colour:
//    VALVE   (amber)  VALVE · OUTPUT · ECHO AMP, DRIVE BIAS SAG TONE MIX, the glowing 12AX7
//    FILTER  (red)    the big stepped dial (its scale = the eleven corner frequencies of the chosen bank),
//                     IMPEDANCE MAGNETISM CHARACTER DYNAMICS ARTEFACTS GAIN, FILTER · BANK B · POST
//    ECHO    (gold)   ECHO · SYNC · DUB SEND, TIME (or the note division when synced) FEEDBACK INPUT OUTPUT HISS MIX WEAR
//    MOD     (lilac)  LFO · SYNC, RATE (or division) LFO DEPTH ENV DEPTH SENS SPEED, LFO shape
//    SPRING  (green)  SPRING, SPRING TENSION DRIVE
//    MASTER           L / R peak ladders, the signal chain, OUTPUT
//  980 x 930 at 1x; the host may zoom it from 0.6x to 1.5x.
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "Updates/DeestechUpdate.h"
#include "DxWidgets.h"

class OliverbEditor : public DxEditorBase
{
public:
    explicit OliverbEditor (OliverbProcessor&);
    ~OliverbEditor() override;
    void resized() override;

    static constexpr int kWidth = 980, kHeight = 930;

private:
    void tick() override;
    void refreshPresetBox();
    void refreshDialScale();

    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    OliverbProcessor& proc;

    juce::ComboBox presetBox;
    DxToggle bypass;
    std::unique_ptr<dtupdate::UpdatePill> updatePill;

    // valve
    DxToggle valveOn, valvePost, valveEcho;
    DxKnob valveDrive, valveBias, valveSag, valveTone, valveMix;
    DxTube tube;
    DxCaption tubeCaption { "12AX7", Dx::Palette::labelGrey };

    // filter
    DxToggle filterOn, filterType, filterPost;
    DxKnob bigDial, impedance, magnetism, character, dynamics, artefacts, filterGain;

    // echo
    DxToggle echoOn, echoSync, echoSend;
    juce::ComboBox divBox;
    std::unique_ptr<ComboAtt> divAtt;
    DxKnob echoTime, echoFb, echoIn, echoOut, echoHiss, echoMix, echoAge;
    DxCaption divCaption { "DIVISION", Dx::Palette::labelGrey };

    // mod
    DxToggle lfoOn, lfoSync;
    juce::ComboBox shapeBox, lfoDivBox;
    std::unique_ptr<ComboAtt> shapeAtt, lfoDivAtt;
    DxKnob lfoRate, lfoDepth, envDepth, envSens, envSpeed;
    DxCaption shapeCaption { "SHAPE", Dx::Palette::labelGrey }, lfoDivCaption { "DIVISION", Dx::Palette::labelGrey };

    // spring
    DxToggle springOn;
    DxKnob springAmt, springDecay, springDrive;

    // master
    LedRow meterL, meterR;
    DxCaption meterCaption { "L / R", Dx::Palette::labelGrey };
    DxCaption chainCaption { "", Dx::Palette::labelGrey }, specCaption { "", Dx::Palette::labelGrey };
    DxKnob outputKnob;

    int lastBank = -1;
    juce::String lastChainText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OliverbEditor)
};
