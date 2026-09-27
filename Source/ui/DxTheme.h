// DxTheme.h — Deestronix rack UI kit: palette, fonts, art and the paint helpers every widget shares.
//
// The look is the DEES TAPE phone FX rack (FxRackView.swift / Views.swift) rendered as real hardware:
// walnut surround, brushed plate, 3D knurled knobs, jewel lamps, analogue VU. Everything is vector
// (crisp at 1x / 2x / host zoom); the only bitmap is the tileable fx_wood.png veneer.
//
// Text: build strings from UTF-8 literals with Dx::u8 ("…") and every draw call routes through
// Paint::utf8() so "·" / "↔" never come out as mojibake in Release.
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>

namespace Dx
{
    // ---- the palette (iOS Deco palette + FxRackView plate/title colours) ----------------
    struct Palette
    {
        static inline const juce::Colour orange     { 0xffF56B0F };   // (0.96, 0.42, 0.06) the accent
        static inline const juce::Colour orangeDark { 0xffDB5408 };   // (0.86, 0.33, 0.03)
        static inline const juce::Colour ink        { 0xff191613 };   // warm near-black window background
        static inline const juce::Colour plateDark  { 0xff1C1C21 };   // Deestress plate (0.11, 0.11, 0.13)
        static inline const juce::Colour plateSlate { 0xff252629 };   // D'Master plate (0.145, 0.15, 0.163)
        static inline const juce::Colour plateCream { 0xffE3E0D6 };   // LA-D plate (0.89, 0.88, 0.84)
        static inline const juce::Colour plateTan   { 0xffE0D1B0 };   // Vintage Delay plate (0.88, 0.82, 0.69)
        static inline const juce::Colour cream      { 0xffE9E8E6 };   // text on dark, VU face base
        static inline const juce::Colour greyMid    { 0xff706B63 };   // (0.44, 0.42, 0.39) knob ticks
        static inline const juce::Colour greyLight  { 0xffB9B9BD };
        static inline const juce::Colour labelGrey  { 0xff8E8E93 };   // SwiftUI .gray captions
        static inline const juce::Colour ledOff     { 0xff35353B };   // unlit LED glass
        static inline const juce::Colour ledOn      { 0xff34C759 };   // green
        static inline const juce::Colour ledAmber   { 0xffFFCC00 };
        static inline const juce::Colour lampRed    { 0xffE53935 };   // NUKE / ceiling / clip
        static inline const juce::Colour cyan       { 0xff80D9E8 };   // D'Master title (0.50, 0.85, 0.91)
        static inline const juce::Colour mint       { 0xff8CD4B8 };   // De-Ess title (0.55, 0.83, 0.72)
        static inline const juce::Colour lilac      { 0xff9EB8F2 };   // Dimension title (0.62, 0.72, 0.95)
        static inline const juce::Colour gold       { 0xffF2C75C };   // Preamp title (0.95, 0.78, 0.36)
        static inline const juce::Colour brownTitle { 0xff4A3B21 };   // Vintage Delay / Spring title (0.29, 0.23, 0.13)
        static inline const juce::Colour brandRed   { 0xff8F2417 };   // "DEESTRONIX" wordmark (0.56, 0.14, 0.09)
        static inline const juce::Colour walnut     { 0xff5A3A1E };   // wood fallback if the PNG is missing
    };

    // UTF-8 literal -> String (the only correct way to get "·" / "↔" out of a const char*)
    juce::String u8 (const char*);

    // ---- STUDIO SERIES (2026-09 design language) ------------------------------------------------
    // Every Deestech unit is a piece of rack hardware: [ear | faceplate | ear] (or walnut cheeks), a finish of its
    // own, recessed modules with engraved titles, printed scales round every knob, lamp buttons, real meters.
    // A Finish is the whole colour story of one unit.
    struct Finish
    {
        juce::Colour plateTop, plateBottom;      // faceplate gradient
        juce::Colour ear, earInk;                // rack ears (anodised) + the engraving on them
        juce::Colour section;                    // module overlay (usually black/white at low alpha) or an opaque fill
        juce::Colour ink, inkDim;                // engraved labels / scales, secondary labels
        juce::Colour title, accent;              // product nameplate colour, the unit's accent (pointer dot, lamps)
        float grain = 0.05f;                     // powder-coat noise strength
        bool  brushed = false;                   // horizontal brushing on the faceplate
        bool  woodCheeks = false;                // walnut end cheeks instead of rack ears
        bool  filledSections = false;            // modules are opaque painted panels (section colour) instead of recesses
        juce::Colour sectionInk;                 // ink on filled sections (only used when filledSections)

        bool isLight() const { return plateTop.getPerceivedBrightness() > 0.55f; }
        // what controls sitting inside the modules print with (dark ink on painted modules)
        juce::Colour controlInk() const   { return filledSections ? sectionInk : ink; }
        bool controlSurfaceLight() const  { return filledSections ? section.getPerceivedBrightness() > 0.55f : isLight(); }

        static Finish gunmetal();    // Deestress Comp — dark brushed gunmetal, red accent
        static Finish champagne();   // D'Master — warm champagne powder coat
        static Finish ivory();       // Vintage Delay — ivory enamel, oxblood, walnut cheeks
        static Finish aluminium();   // LA-D — brushed aluminium, big VU
        static Finish neveBlue();    // Preamp — console blue-grey
        static Finish sage();        // De-Ess — pale sage enamel
        static Finish midnight();    // Dimension — midnight navy, lilac
        static Finish mahogany();    // Spring Reverb — black + orange modules, walnut cheeks
        static Finish obsidian();    // 6-Band EQ — black, coloured caps, gold nameplate
        static Finish slate();       // Gate — slate, green lamps
        static Finish cocoa();       // Tape Saturator — brown lacquer, brass
        static Finish tiger();       // CLEAN Air — pale sage-cream, teal ears
        static Finish tigerNight();  // CLEAN Air, NIGHT — graphite with teal ears
        static Finish goldSix();     // Neo TUBE — black + amber glass
        static Finish oxblood();     // OLIVERB — oxblood dub desk, walnut cheeks
    };

    // Marconi knob cap colours (console convention: colour tells you the function at a glance)
    namespace Cap
    {
        inline const juce::Colour blue   { 0xff2F6DB5 };
        inline const juce::Colour red    { 0xffC7342A };
        inline const juce::Colour green  { 0xff2E8B57 };
        inline const juce::Colour yellow { 0xffE2B41F };
        inline const juce::Colour grey   { 0xff9C9EA2 };
        inline const juce::Colour orange { 0xffE8772A };
        inline const juce::Colour cream  { 0xffEDE7D6 };
        inline const juce::Colour black  { 0xff1A1A1B };
        inline const juce::Colour lilac  { 0xff8E9BE6 };
    }

    // ---- short aliases kept for code written against the first kit --------------------
    inline const juce::Colour& orange     = Palette::orange;
    inline const juce::Colour& orangeDark = Palette::orangeDark;
    inline const juce::Colour& red        = Palette::lampRed;
    inline const juce::Colour& ink        = Palette::ink;
    inline const juce::Colour& greyMid    = Palette::greyMid;
    inline const juce::Colour& greyLight  = Palette::greyLight;
    inline const juce::Colour& labelGrey  = Palette::labelGrey;
    inline const juce::Colour& walnut     = Palette::walnut;
    inline const juce::Colour& brandRed   = Palette::brandRed;
    inline const juce::Colour& deestressPlate = Palette::plateDark;
    inline const juce::Colour  deestressTitle { 0xffFFFFFF };
    inline const juce::Colour& dmasterPlate   = Palette::plateSlate;
    inline const juce::Colour& dmasterTitle   = Palette::cyan;
    inline const juce::Colour& delayPlate     = Palette::plateTan;
    inline const juce::Colour& delayTitle     = Palette::brownTitle;
}

// Fonts embedded from Resources/ (BinaryData): Righteous (titles / wordmark), Space Mono (captions, values).
// The serif italic ("Deestress Comp", the DEESTRONIX brand) is the system serif (Georgia where present).
struct Fonts
{
    static juce::Font engraved (float size);                    // Oswald: engraved labels, scales, module titles
    static juce::Font plate (float size);                       // Bebas Neue: the product nameplate
    static juce::Typeface::Ptr oswaldTypeface();
    static juce::Typeface::Ptr bebasTypeface();
    static juce::Font righteous (float size);
    static juce::Font mono (float size, bool bold = false);
    static juce::Font label (float size);                       // Space Mono Bold: tracked micro-caps
    static juce::Font serifItalic (float size);                 // bold italic serif
    static juce::Typeface::Ptr righteousTypeface();
    static juce::Typeface::Ptr monoTypeface();
    static juce::Typeface::Ptr monoBoldTypeface();
};

// Images embedded from Resources/
struct Art
{
    static const juce::Image& wood();      // tileable 480 px walnut (the phone's FxWood)
};

// Drawing helpers shared by every widget
namespace Paint
{
    // Repairs a String built from a UTF-8 literal through juce::String (const char*), which decodes the bytes as
    // Latin-1 in Release ("·" -> "Â·"). Pure ASCII and properly decoded strings come back unchanged.
    juce::String utf8 (const juce::String&);

    // cheap layered rounded drop shadow — no image blur, safe every paint
    void softShadow (juce::Graphics&, juce::Rectangle<float>, float radius, float alpha = 0.22f,
                     float spread = 8.f, float offsetY = 3.f);
    // blurred shadow that falls INSIDE a rounded rect (a recessed plate / meter window)
    void innerShadow (juce::Graphics&, juce::Rectangle<float>, float radius, float alpha = 0.5f, int blur = 8, int offsetY = 2);
    // walnut rack surround: tiled veneer (0.5x), burnished edge, sheen, dark stroke, corner screws
    void woodFrame (juce::Graphics&, juce::Rectangle<float>, float radius = 14.f, bool screws = true);
    // the unit's plate inside the wood: recessed, brushed (deterministic hairline noise), vignette
    void plate (juce::Graphics&, juce::Rectangle<float>, juce::Colour fill, float radius = 10.f);
    // a slotted machine screw
    void screw (juce::Graphics&, juce::Point<float> centre, float radius, float slotAngle);
    // tracked uppercase micro label ("G A I N   R E D U C T I O N")
    void label (juce::Graphics&, const juce::String&, juce::Rectangle<int>, juce::Colour, float size = 9.f,
                juce::Justification = juce::Justification::centred, float tracking = 0.12f);
    // title text: Righteous, or the serif bold italic when italic = true
    void title (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour, float size,
                bool italic = false, juce::Justification = juce::Justification::centredLeft, float tracking = 0.f);
    // the little dark value bubble knobs/faders show while dragging
    void valueBubble (juce::Graphics&, const juce::String&, juce::Point<float> centre, float maxWidth);
    // true when text should be dark on this surface
    bool isLight (juce::Colour surface);

    // ---- STUDIO SERIES painters (all vector, cached by the components that use them) --------------------------------
    // faceplate: finish gradient, powder-coat grain or brushing, edge bevel
    void faceplate (juce::Graphics&, juce::Rectangle<float>, const Dx::Finish&, float radius = 3.f);
    // one rack ear: anodised strip, two oval slots with Phillips screws, vertical engraving
    void rackEar (juce::Graphics&, juce::Rectangle<float>, const Dx::Finish&, bool left, const juce::String& engraving,
                  bool brandFont);
    // a walnut end cheek (instead of a rack ear)
    void woodCheek (juce::Graphics&, juce::Rectangle<float>, bool left);
    // a module: recessed (or painted, finish.filledSections) with its engraved title top-left and corner screws
    void section (juce::Graphics&, juce::Rectangle<float>, const Dx::Finish&, const juce::String& title, bool screws = true);
    // engraved / printed text with a 1 px relief highlight (tracking = extra kerning)
    void engrave (juce::Graphics&, const juce::String&, juce::Rectangle<float>, juce::Colour ink, juce::Font,
                  juce::Justification, bool lightSurface, float tracking = 0.08f);
    // a pan-head Phillips screw
    void phillips (juce::Graphics&, juce::Point<float> centre, float radius, float angle);
    // knob bodies. body: 0 Marconi (skirt + coloured cap), 1 Chunky (cream knurled), 2 Bakelite (black gloss + metal
    // skirt), 3 Chrome. angle in JUCE rotary radians (0 = 12 o'clock, clockwise).
    void knob (juce::Graphics&, juce::Point<float> centre, float radius, float angle, int body, juce::Colour cap,
               juce::Colour accent, float alpha = 1.f);
    // printed scale round a knob: ticks from a0 to a1, labels spread evenly over the ticks that carry them
    void knobScale (juce::Graphics&, juce::Point<float> centre, float radius, float a0, float a1, int ticks,
                    const juce::StringArray& labels, juce::Colour ink, bool lightSurface, float labelSize);
    // a square illuminated push button (lens colour, lit or dark), text printed on the lens
    void lampButton (juce::Graphics&, juce::Rectangle<float>, juce::Colour lens, bool lit, bool over, bool down,
                     const juce::String& text, bool lightSurface);
    // one rectangular LED segment (amount 0 = dark glass, 1 = full)
    void led (juce::Graphics&, juce::Rectangle<float>, juce::Colour, float amount);
    // tiled 128 px grain (static)
    const juce::Image& grainTile();
}
