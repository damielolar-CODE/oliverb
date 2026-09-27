// DxWidgets.h — THE SHARED RACK UI KIT for every Deestronix unit (and DEES TAPE's built-in FX editors).
//
//   RackPanel     walnut frame + brushed plate + title strip            DxEditorBase  editor scaffold (L&F, panel, 30 Hz tick)
//   DxKnob        3D knurled hardware knob bound to a parameter        DxToggle      the orange IN / grey OFF rocker
//   DxChoiceRow   chip row for a choice parameter (ratio 1:1 … NUKE)   DxFader       vertical fader (EQ bands)
//   LedRow        13-LED gain-reduction ladder                          VuMeter       cream analogue VU with ballistics
//   DxLamp        jewel lamp (ceiling / clip)                           DxCaption     tracked micro-caption ("R A T I O")
//
// Every control is bound to its juce::RangedAudioParameter: the parameter is the only state, the widget
// just draws it (host automation, presets and undo all show up). Widgets paint themselves (no LookAndFeel
// dependency) so they look identical inside any host or inside DEES TAPE.
#pragma once
#include "DxTheme.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

// Update notices (Plugins/Common/DeestechUpdate.h): a plug-in built with DX_UPDATE_ID="deestronix/<slug>" (CMake does this
// for every unit) shows an orange UPDATE x.y.z pill in its footer when deestechholdings.com/updates.json lists a newer
// version. DEES TAPE's built-in copies of this kit leave DX_UPDATE_ID undefined and never check.
#if defined (DX_UPDATE_ID) && __has_include ("DeestechUpdate.h")
 #include "DeestechUpdate.h"
 #define DX_HAS_UPDATER 1
#else
 #define DX_HAS_UPDATER 0
#endif

// ============================================================================
// The one LookAndFeel for the kit: fonts (Righteous / Space Mono / system), popup menus, combo boxes,
// tooltips, labels and the host resize corner — all text through Paint::utf8().
// ============================================================================
class DxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DxLookAndFeel();
    juce::Font getPopupMenuFont() override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int height) override;
    juce::Font getAlertWindowMessageFont() override;
    juce::Font getAlertWindowTitleFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>& area, const juce::String& sectionName) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void drawTooltip (juce::Graphics&, const juce::String& text, int w, int h) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};

// ============================================================================
// RackPanel — STUDIO SERIES chassis: [rack ear | faceplate | rack ear] (or walnut cheeks), the unit's Finish,
// the nameplate header (product name in Bebas, subtitle engraved), recessed modules (setSections) with engraved
// titles and corner screws. Children (knobs, buttons, meters) are added to the panel and laid out by the editor
// inside contentArea() / the section rectangles. The static art is cached at the physical pixel scale.
// ============================================================================
class RackPanel : public juce::Component
{
public:
    enum Style { Dark, Cream, Tan, Slate };          // legacy styles, mapped onto finishes

    struct Section
    {
        Section() = default;
        Section (juce::Rectangle<int> a, juce::String t, bool s = true) : area (a), title (std::move (t)), screws (s) {}
        juce::Rectangle<int> area; juce::String title; bool screws = true;
    };

    RackPanel (juce::String title, juce::String subtitle, const Dx::Finish&);
    RackPanel (juce::String title, juce::String subtitle, Style style, juce::Colour titleColour, bool italicTitle = false);

    juce::Rectangle<int> plateArea() const;          // the faceplate between the ears
    juce::Rectangle<int> headerArea() const;         // nameplate strip (left of the toggle slot)
    juce::Rectangle<int> toggleArea() const;         // where the IN lamp button goes (top right)
    juce::Rectangle<int> contentArea() const;        // faceplate minus padding minus header
    void setBrand (juce::String leftEar, juce::String unused = {});   // engraving on the left ear (default DEESTRONIX)
    void setModel (juce::String rightEar);           // engraving on the right ear (default: the title + version)
    void setFooter (juce::String) {}                 // legacy no-op (the version lives on the right ear)
    void setFinish (const Dx::Finish&);
    void setStyle (Style);
    void setSections (juce::Array<Section>);         // modules, in panel coordinates (call from resized)
    void setSections (std::initializer_list<Section> list) { juce::Array<Section> a; for (auto& x : list) a.add (x); setSections (a); }
    const Dx::Finish& finish() const { return fin; }
    Style getStyle() const { return style; }
    juce::Colour plateColour() const { return fin.plateTop; }
    juce::Colour captionColour() const { return fin.ink; }
    juce::Colour titleColour() const { return fin.title; }

    void paint (juce::Graphics&) override;

    static constexpr int kEar = 30;         // rack ear width (22 for walnut cheeks)
    static constexpr int kPad = 14;         // faceplate padding
    static constexpr int kHeaderH = 40;
    static constexpr int kToggleW = 54, kToggleH = 26;
    static constexpr int kMargin = 0, kWood = 0, kFooterH = 0;   // legacy geometry names

private:
    int earWidth() const { return fin.woodCheeks ? 22 : kEar; }
    juce::String title, subtitle, brand { "DEESTRONIX" }, model;
    Style style = Dark;
    Dx::Finish fin;
    juce::Array<Section> sections;
    juce::Image backdrop; float backdropScale = 0.f;
};

// ============================================================================
// DxKnob — a hardware knob bound to a parameter: engraved caption ABOVE, the knob, and a printed scale round it
// (ticks + values taken from the parameter's own text, or setScaleLabels). Bodies:
//   Marconi  fluted black skirt + coloured cap (setCap)   Chunky   cream knurled (Tiger style)
//   Bakelite glossy black on a knurled metal skirt        Chrome   Marconi cap on a chrome skirt
//   (legacy: Dark = Bakelite, Cream = Chunky)
// Mouse: drag (vertical/horizontal), wheel, double-click = default, shift = fine; value bubble while dragging.
// ============================================================================
class DxKnob : public juce::Component, private juce::Timer
{
public:
    enum Body { Dark, Cream, Marconi, Chunky, Bakelite, Chrome };

    DxKnob (juce::RangedAudioParameter&, juce::String caption, Body body = Dark, juce::String unit = {});
    ~DxKnob() override;

    using juce::Component::setSize;
    void setSize (int px);                          // knob diameter; the component becomes preferredWidth x preferredHeight
    int  preferredHeight() const;
    int  preferredWidth() const;
    void placeIn (juce::Rectangle<int> area, int px);    // setSize (px) and centre the knob in area
    void flashValue (int frames = 60);
    void setCaptionColour (juce::Colour);           // overrides the panel's ink
    void setTooltip (const juce::String&);
    void setValueText (std::function<juce::String (float normalised)>);
    void setCap (juce::Colour c) { cap = c; repaint(); }
    void setBody (Body b) { body = b; repaint(); }
    void setScaleLabels (juce::StringArray labels) { scaleLabels = std::move (labels); autoScale = false; repaint(); }
    void setScaleSteps (int labelCount) { labelCount = juce::jmax (2, labelCount); repaint(); }
    void setScaleVisible (bool v) { showScale = v; repaint(); }

    juce::Slider& getSlider() { return slider; }
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kCaptionH = 15, kTopPad = 0, kCaptionOverlap = 0;

private:
    class KnobSlider;
    void timerCallback() override;
    juce::String currentText() const;
    void showBubble (bool sticky);
    float labelSize() const;
    float outerRadius() const;
    juce::Point<float> knobCentre() const;
    juce::StringArray labelsForScale() const;

    juce::RangedAudioParameter& param;
    juce::String caption, unit;
    Body body;
    juce::Colour cap { Dx::Cap::grey };
    int knobPx = 60, labelCount = 6;
    juce::Colour captionCol;
    bool captionOverride = false, showScale = true, autoScale = true;
    juce::StringArray scaleLabels;
    std::unique_ptr<KnobSlider> sliderImpl;
    juce::Slider& slider;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
    std::function<juce::String (float)> valueText;
    float animPos = -1.f, targetPos = 0.f, glow = 0.f;
    bool dragging = false;
    int bubbleFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxKnob)
};

// ============================================================================
// DxToggle — the orange IN / grey OFF rocker (the phone's "IN" chip as a real raised switch)
// ============================================================================
class DxToggle : public juce::Component, public juce::SettableTooltipClient
{
public:
    DxToggle (juce::RangedAudioParameter& boolParam, juce::String onText = "IN", juce::String offText = "OFF");
    ~DxToggle() override;

    bool isOn() const { return on; }
    void setLens (juce::Colour c) { lens = c; repaint(); }   // lamp colour when on (default amber)
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::String onText, offText;
    juce::ParameterAttachment attachment;
    juce::Colour lens { 0xffF4A63A };
    bool on = false, over = false, down = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxToggle)
};

// ============================================================================
// DxLampButton — a lamp push button that is NOT bound to a parameter (editor state: NIGHT, preset actions,
// mode selectors driven by hand). setLit() shows the state; onClick fires on release inside.
// ============================================================================
class DxLampButton : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit DxLampButton (juce::String text, juce::Colour lens = juce::Colour (0xffF4A63A));
    void setLit (bool l) { if (l != lit) { lit = l; repaint(); } }
    bool isLit() const { return lit; }
    void setText (juce::String t) { text = std::move (t); repaint(); }
    void setLens (juce::Colour c) { lens = c; repaint(); }
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { over = false; down = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override  { down = true; repaint(); }
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::String text; juce::Colour lens;
    bool lit = false, over = false, down = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxLampButton)
};

// ============================================================================
// DxChoiceRow — a row of raised chips for a choice parameter (RATIO 1:1 … NUKE with a red NUKE chip)
// ============================================================================
class DxChoiceRow : public juce::Component, public juce::SettableTooltipClient
{
public:
    DxChoiceRow (juce::RangedAudioParameter& choiceParam, juce::StringArray labels, juce::Colour onColour, int redIndex = -1);
    ~DxChoiceRow() override;

    void setChipTooltips (juce::StringArray);      // one per chip (optional)
    void setChipSize (int maxWidth, int gap) { chipMaxW = maxWidth; chipGap = gap; resized(); }
    int  getSelected() const { return selected; }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    juce::String getTooltip() override;

private:
    int chipAt (juce::Point<int>) const;
    juce::StringArray labels, tips;
    juce::Colour onColour;
    int redIndex, selected = 0, hover = -1, pressed = -1;
    int chipMaxW = 52, chipGap = 6;
    juce::Array<juce::Rectangle<int>> chips;
    juce::ParameterAttachment attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxChoiceRow)
};

// ============================================================================
// LedRow — 13 jewel LEDs that fill from the right (green -> amber -> red), for gain reduction in dB
// (0..26 dB full scale, like the phone). Instant attack, slow visual decay, 600 ms peak hold.
// ============================================================================
class LedRow : public juce::Component, private juce::Timer
{
public:
    LedRow();
    ~LedRow() override;
    void setDb (float gainReductionDb);
    void setFullScaleDb (float db) { fullScale = juce::jmax (1.f, db); }
    void setGainReductionDb (float db) { setDb (db); }   // first-kit name
    void setVertical (bool v) { vertical = v; repaint(); }
    void setFillFromLeft (bool v) { fromLeft = v; repaint(); }   // level meters fill from the left / bottom
    void setCount (int n) { count = juce::jlimit (4, 40, n); repaint(); }
    void paint (juce::Graphics&) override;

    static constexpr int kCount = 13;

private:
    void timerCallback() override;
    float target = 0.f, shown = 0.f, peak = 0.f, fullScale = 26.f;
    int peakHold = 0, count = kCount;
    bool vertical = false, fromLeft = false;
};

// ============================================================================
// VuMeter — cream analogue VU with needle ballistics (the DEES TAPE Widgets VUMeter, in a bezel with
// glass). setLevel() takes a linear value: 1.0 = 0 VU; the face runs -20 .. +3.
// ============================================================================
class VuMeter : public juce::Component, private juce::Timer
{
public:
    VuMeter();
    ~VuMeter() override;
    void setLevel (float linear);
    void setDb (float db);                         // same thing in dB (0 = 0 VU)
    void setCaption (const juce::String& s) { caption = s; backdrop = juce::Image(); repaint(); }
    void setLegend (const juce::String& s) { legend = s; backdrop = juce::Image(); repaint(); }   // text under the "VU" (e.g. "GAIN REDUCTION")
    void setReverse (bool r) { reverse = r; }     // gain-reduction meters rest at 0 and swing left
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    float needle = 0.f, target = 0.f;
    bool reverse = false;
    juce::String caption, legend { "VU" };
    juce::Image backdrop; float backdropScale = 0.f;   // bezel + face, cached at the physical pixel scale
};

// ============================================================================
// DxFader — vertical fader bound to a parameter (the 6-band EQ bands): dark rail with a centre/unity mark,
// grey cap with the orange line, band label + orange value readout underneath, value bubble while dragging.
// ============================================================================
class DxFader : public juce::Component, private juce::Timer
{
public:
    DxFader (juce::RangedAudioParameter&, juce::String bandLabel);
    ~DxFader() override;

    void setCentreMark (bool centred) { centreMark = centred; repaint(); }   // mark at the middle (EQ) or at 0.8 (gain)
    void setCapColour (juce::Colour c) { capColour = c; repaint(); }       // the coloured stripe on the fader cap
    void setCaptionColour (juce::Colour);
    void setTooltip (const juce::String&);
    juce::Slider& getSlider() { return slider; }

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kLabelH = 26;

private:
    class FaderSlider;
    void timerCallback() override;

    juce::RangedAudioParameter& param;
    juce::String label;
    juce::Colour captionCol, capColour { Dx::Cap::cream };
    bool centreMark = true, dragging = false, captionOverride = false;
    std::unique_ptr<FaderSlider> sliderImpl;
    juce::Slider& slider;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
    float animPos = -1.f;
    int bubbleFrames = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxFader)
};

// ============================================================================
// DxLamp — jewel lamp in a chrome bezel (ceiling / clip / power). set (colour, lit)
// ============================================================================
class DxLamp : public juce::Component
{
public:
    void set (juce::Colour, bool lit);
    void paint (juce::Graphics&) override;
private:
    juce::Colour colour { Dx::Palette::lampRed };
    bool lit = false;
};

// ============================================================================
// DxTube — a glass vacuum tube (12AX7 style) with a glowing filament: bulb, plates, getter flash, bakelite base.
// setGlow (0..1) sets how hot it runs; it flickers gently on its own while lit.
// ============================================================================
class DxTube : public juce::Component, private juce::Timer
{
public:
    DxTube();
    ~DxTube() override;
    void setGlow (float g);
    void paint (juce::Graphics&) override;
private:
    void timerCallback() override;
    float glow = 0.f, shown = 0.f, flicker = 0.f;
    juce::Random rng;
};

// ============================================================================
// DxCaption — a tracked micro-caption ("G A I N   R E D U C T I O N", "R A T I O")
// ============================================================================
class DxCaption : public juce::Component
{
public:
    DxCaption (juce::String text, juce::Colour colour, float size = 7.5f, float tracking = 0.30f);
    void setText (juce::String t) { text = std::move (t); repaint(); }
    void setColour (juce::Colour c) { colour = c; repaint(); }
    void paint (juce::Graphics&) override;
private:
    juce::String text; juce::Colour colour; float size, tracking;
};
using MicroCaption = DxCaption;

// ============================================================================
// DxEditorBase — sets the DxLookAndFeel, owns the RackPanel, runs a 30 Hz meter timer (override tick()),
// and handles host resizing: the whole rack scales as one vector transform, 1x .. 1.5x, aspect locked.
//
//   struct MyEditor : DxEditorBase {
//       MyEditor (MyProcessor& p) : DxEditorBase (p, "VINTAGE DELAY", "T A P E   E C H O", RackPanel::Tan, Dx::Palette::brownTitle),
//                                   time (*p.apvts.getParameter ("time"), "TIME", DxKnob::Cream) {
//           panel.addAndMakeVisible (time); …; setBaseSize (440, 240); }
//       void resized() override { DxEditorBase::resized(); auto r = panel.contentArea(); … } };
// ============================================================================
struct DxEditorBase : public juce::AudioProcessorEditor, private juce::Timer
{
    DxEditorBase (juce::AudioProcessor&, juce::String title, juce::String subtitle, RackPanel::Style style,
                  juce::Colour titleColour, bool italicTitle = false);
    DxEditorBase (juce::AudioProcessor&, juce::String title, juce::String subtitle, const Dx::Finish&);
    ~DxEditorBase() override;

    void setBaseSize (int width, int height);       // the 1x size; the host may zoom it up to 1.5x
    void setMinZoom (float z);                      // allow shrinking below 1x (tall units on small screens); call before setBaseSize
    float getZoom() const { return zoom; }
    void paint (juce::Graphics&) override;
    void resized() override;                        // keeps the panel at base size and scales it — call from overrides

    DxLookAndFeel lnf;
    RackPanel panel;
    juce::TooltipWindow tooltips;
   #if DX_HAS_UPDATER
    std::unique_ptr<dtupdate::UpdatePill> updatePill;   // header, left of the IN lamp; hidden until a newer version exists
   #endif

    static constexpr float kMaxZoom = 1.5f;

protected:
    virtual void tick() {}                          // 30 Hz: poll meters here

private:
    void timerCallback() override { tick(); }
    void initialise();
    int baseW = 0, baseH = 0;
    float zoom = 1.f, minZoom = 1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DxEditorBase)
};
