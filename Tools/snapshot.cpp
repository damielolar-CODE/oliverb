// snapshot.cpp — renders the OLIVERB editor to a PNG offscreen (no host, no audio device). Dev only, never shipped.
//   OliverbSnapshot <out.png> [scale]        DX_SNAPSHOT_CALLOUT=1 also opens the update details panel.
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"
#include "Updates/DeestechUpdate.h"
#include <cstdio>
#include <cstdlib>

static void findPills (juce::Component& c, juce::Array<dtupdate::UpdatePill*>& out)
{
    for (auto* ch : c.getChildren())
    {
        if (auto* p = dynamic_cast<dtupdate::UpdatePill*> (ch)) out.add (p);
        findPills (*ch, out);
    }
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::fprintf (stderr, "usage: %s <out.png> [scale]\n", argv[0]); return 64; }
    juce::ScopedJuceInitialiser_GUI init;
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 2.f;
    OliverbProcessor proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc.prepareToPlay (48000.0, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setVisible (true);
    editor->resized();
    juce::MessageManager::getInstance()->runDispatchLoopUntil (400);
    if (std::getenv ("DX_SNAPSHOT_CALLOUT") != nullptr)
    {
        juce::Array<dtupdate::UpdatePill*> pills; findPills (*editor, pills);
        for (auto* p : pills) p->showDetails();
        juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
    }
    const auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);
    juce::File out (argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png; png.writeImageToStream (img, stream); stream.flush();
    std::printf ("%s  %dx%d\n", out.getFullPathName().toRawUTF8(), editor->getWidth(), editor->getHeight());
    editor.reset();
    return 0;
}
