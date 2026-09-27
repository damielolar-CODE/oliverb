#include "DxTheme.h"
#include "BinaryData.h"

using namespace juce;

String Dx::u8 (const char* s) { return String::fromUTF8 (s); }

// ============================================================================
// Fonts — embedded typefaces created once per process
// ============================================================================
Typeface::Ptr Fonts::righteousTypeface()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::Righteous_ttf, BinaryData::Righteous_ttfSize);
    return t;
}

Typeface::Ptr Fonts::monoTypeface()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::SpaceMono_ttf, BinaryData::SpaceMono_ttfSize);
    return t;
}

Typeface::Ptr Fonts::monoBoldTypeface()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    return t;
}

Typeface::Ptr Fonts::oswaldTypeface()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::Oswald_ttf, BinaryData::Oswald_ttfSize);
    return t;
}

Typeface::Ptr Fonts::bebasTypeface()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::BebasNeue_ttf, BinaryData::BebasNeue_ttfSize);
    return t;
}

Font Fonts::engraved (float size)
{
    if (auto tf = oswaldTypeface())
        return Font (FontOptions (tf).withHeight (size));
    return Font (FontOptions (size, Font::bold));
}

Font Fonts::plate (float size)
{
    if (auto tf = bebasTypeface())
        return Font (FontOptions (tf).withHeight (size));
    return Font (FontOptions (size, Font::bold));
}

Font Fonts::righteous (float size)
{
    if (auto tf = righteousTypeface())
        return Font (FontOptions (tf).withHeight (size));

    return Font (FontOptions (size, Font::bold));
}

Font Fonts::mono (float size, bool bold)
{
    if (auto tf = bold ? monoBoldTypeface() : monoTypeface())
        return Font (FontOptions (tf).withHeight (size));

    return Font (FontOptions (Font::getDefaultMonospacedFontName(), size, bold ? Font::bold : Font::plain));
}

Font Fonts::label (float size)
{
    return mono (size, true);
}

Font Fonts::serifItalic (float size)
{
    // Georgia (macOS + Windows) is the closest desktop match to the phone's New York italic
    static const String name = [] () -> String
    {
        for (auto& n : Font::findAllTypefaceNames())
            if (n.equalsIgnoreCase ("Georgia"))
                return n;
        return Font::getDefaultSerifFontName();
    }();
    return Font (FontOptions (name, size, Font::bold | Font::italic));
}

// ============================================================================
// Art
// ============================================================================
const Image& Art::wood()
{
    static Image img = ImageCache::getFromMemory (BinaryData::fx_wood_png, BinaryData::fx_wood_pngSize);
    return img;
}

// ============================================================================
// Paint helpers
// ============================================================================
String Paint::utf8 (const String& s)
{
    bool suspicious = false;
    for (auto t = s.getCharPointer(); ! t.isEmpty();)
    {
        const auto c = t.getAndAdvance();
        if (c >= 0x80)
        {
            if (c > 0xFF) return s;      // already holds real Unicode: nothing to repair
            suspicious = true;
        }
    }
    if (! suspicious) return s;          // plain ASCII fast path

    MemoryBlock bytes;
    for (auto t = s.getCharPointer(); ! t.isEmpty();)
    {
        const uint8 b = (uint8) t.getAndAdvance();
        bytes.append (&b, 1);
    }
    const auto* data = static_cast<const char*> (bytes.getData());
    if (CharPointer_UTF8::isValidString (data, (int) bytes.getSize()))
        return String::fromUTF8 (data, (int) bytes.getSize());

    return s;                             // genuine Latin-1 text
}

bool Paint::isLight (Colour c) { return c.getPerceivedBrightness() > 0.6f; }

void Paint::softShadow (Graphics& g, Rectangle<float> r, float radius, float alpha, float spread, float offsetY)
{
    constexpr int layers = 6;
    constexpr float weightSum = (float) (layers * (layers + 1) / 2);
    for (int i = layers; i >= 1; --i)
    {
        const float grow = spread * (float) i / (float) layers;
        const float a = alpha * (float) (layers - i + 1) / weightSum;
        g.setColour (Colours::black.withAlpha (a));
        g.fillRoundedRectangle (r.expanded (grow).translated (0.f, offsetY), radius + grow);
    }
}

void Paint::innerShadow (Graphics& g, Rectangle<float> r, float radius, float alpha, int blur, int offsetY)
{
    // shadow of the "everything outside the rect" shape, clipped to the rect: falls inwards
    Path inside;
    inside.addRoundedRectangle (r, radius);
    Path ring;
    ring.addRectangle (r.expanded ((float) blur * 3.f));
    ring.addRoundedRectangle (r, radius);
    ring.setUsingNonZeroWinding (false);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (inside);
    DropShadow (Colours::black.withAlpha (alpha), blur, { 0, offsetY }).drawForPath (g, ring);
}

void Paint::screw (Graphics& g, Point<float> c, float r, float slotAngle)
{
    const auto area = Rectangle<float> (r * 2.f, r * 2.f).withCentre (c);

    // countersink + drop shadow
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (area.expanded (1.2f).translated (0.f, 0.8f));
    g.setColour (Colour (0xff2A1A0E).withAlpha (0.8f));
    g.fillEllipse (area.expanded (0.9f));

    // steel head, lit top-left
    ColourGradient head (Colour (0xffD8D6D0), c.x - r * 0.6f, c.y - r * 0.7f, Colour (0xff6E6C68), c.x + r * 0.8f, c.y + r * 0.9f, true);
    head.addColour (0.55, Colour (0xffA9A7A2));
    g.setGradientFill (head);
    g.fillEllipse (area);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (area, 0.7f);
    g.setColour (Colours::white.withAlpha (0.35f));
    Path hi;
    hi.addCentredArc (c.x, c.y, r * 0.78f, r * 0.78f, 0.f, MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 1.8f, true);
    g.strokePath (hi, PathStrokeType (r * 0.14f));

    // slot
    Path slot;
    slot.addRoundedRectangle (-r * 0.78f, -r * 0.13f, r * 1.56f, r * 0.26f, r * 0.06f);
    const auto rot = AffineTransform::rotation (slotAngle).translated (c);
    g.setColour (Colours::white.withAlpha (0.22f));
    g.fillPath (slot, rot.translated (0.f, 0.6f));
    g.setColour (Colours::black.withAlpha (0.62f));
    g.fillPath (slot, rot);
}

void Paint::woodFrame (Graphics& g, Rectangle<float> r, float radius, bool screws)
{
    softShadow (g, r, radius, 0.42f, 7.f, 3.f);

    Path p;
    p.addRoundedRectangle (r, radius);

    const auto& wood = Art::wood();
    if (wood.isValid())
    {
        // 480 px tile drawn at half size so the grain reads as veneer rather than planks
        g.setFillType (FillType (wood, AffineTransform::scale (0.5f).translated (r.getX(), r.getY())));
        g.fillPath (p);
    }
    else
    {
        g.setColour (Dx::Palette::walnut);
        g.fillPath (p);
    }

    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (p);

        // lacquer: a soft top-lit sheen across the veneer
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), r.getX(), r.getY(),
                                           Colours::black.withAlpha (0.16f), r.getX(), r.getBottom(), false));
        g.fillPath (p);

        // burnished inner edge + a bright bevel line just inside it
        g.setColour (Colours::black.withAlpha (0.30f));
        g.drawRoundedRectangle (r.reduced (1.5f), jmax (0.f, radius - 1.5f), 3.f);
        g.setColour (Colours::white.withAlpha (0.13f));
        g.drawRoundedRectangle (r.reduced (3.5f), jmax (0.f, radius - 3.5f), 1.f);
    }

    g.setColour (Colour (0x80211008));   // rgba(33,16,8,0.5) — FxPanel's stroke
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.f);

    if (screws)
    {
        const float in = jmax (6.5f, radius * 0.5f), sr = 3.1f;
        screw (g, { r.getX() + in,     r.getY() + in },      sr, 0.45f);
        screw (g, { r.getRight() - in, r.getY() + in },      sr, 1.95f);
        screw (g, { r.getX() + in,     r.getBottom() - in }, sr, 2.60f);
        screw (g, { r.getRight() - in, r.getBottom() - in }, sr, 0.95f);
    }
}

void Paint::plate (Graphics& g, Rectangle<float> r, Colour fill, float radius)
{
    const bool light = isLight (fill);
    Path p;
    p.addRoundedRectangle (r, radius);

    // base: a barely-there vertical falloff so the plate has a top and a bottom
    g.setGradientFill (ColourGradient (fill.brighter (light ? 0.05f : 0.10f), r.getX(), r.getY(),
                                       fill.darker (light ? 0.04f : 0.12f), r.getX(), r.getBottom(), false));
    g.fillPath (p);

    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (p);

        // brushed finish: deterministic hairline noise (vector, so it stays crisp at any zoom)
        Random rng (0x0DEE5);
        const Colour line = light ? Colours::black : Colours::white;
        const float base = light ? 0.018f : 0.022f, var = light ? 0.030f : 0.040f;
        for (float y = r.getY() + 1.f; y < r.getBottom(); y += 1.f)
        {
            const float a = base + var * rng.nextFloat();
            g.setColour (line.withAlpha (a));
            g.drawHorizontalLine ((int) y, r.getX() + 1.f, r.getRight() - 1.f);
        }

        // vignette: soft light at the top centre, darker corners
        ColourGradient v (Colours::white.withAlpha (light ? 0.16f : 0.045f), r.getCentreX(), r.getY(),
                          Colours::black.withAlpha (light ? 0.06f : 0.20f), r.getX(), r.getBottom(), true);
        g.setGradientFill (v);
        g.fillPath (p);
    }

    // recessed into the wood: shadow falls in from the top edge, the bottom lip catches light
    innerShadow (g, r, radius, light ? 0.30f : 0.55f, 7, 2);

    g.setColour ((light ? Colours::white.withAlpha (0.55f) : Colours::white.withAlpha (0.07f)));
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.withTrimmedTop (r.getHeight() - radius - 1.f).toNearestInt());
        g.drawRoundedRectangle (r.reduced (1.f), jmax (0.f, radius - 1.f), 1.f);
    }
    g.setColour (Colours::black.withAlpha (light ? 0.22f : 0.65f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.f);
}

void Paint::label (Graphics& g, const String& text, Rectangle<int> r, Colour c, float size, Justification just, float tracking)
{
    g.setColour (c);
    g.setFont (Fonts::label (size).withExtraKerningFactor (tracking));
    g.drawText (utf8 (text), r, just, false);
}

void Paint::title (Graphics& g, const String& text, Rectangle<float> r, Colour c, float size, bool italic, Justification just, float tracking)
{
    g.setColour (c);
    g.setFont ((italic ? Fonts::serifItalic (size) : Fonts::righteous (size)).withExtraKerningFactor (tracking));
    g.drawText (utf8 (text), r, just, false);
}

void Paint::valueBubble (Graphics& g, const String& text, Point<float> centre, float maxWidth)
{
    const Font f = Fonts::mono (9.5f, true);
    const String t = utf8 (text);
    const float w = jmin (maxWidth, GlyphArrangement::getStringWidth (f, t) + 14.f);
    const float h = 17.f;
    const auto r = Rectangle<float> (w, h).withCentre (centre);

    softShadow (g, r, 5.f, 0.35f, 3.f, 1.5f);
    g.setColour (Dx::Palette::ink.withAlpha (0.94f));
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (Dx::Palette::orange.withAlpha (0.55f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.f, 1.f);
    g.setColour (Dx::Palette::orange);
    g.setFont (f);
    g.drawText (t, r.toNearestInt(), Justification::centred, false);
}


// =====================================================================================================================
// STUDIO SERIES — finishes
// =====================================================================================================================
namespace
{
    Dx::Finish makeFinish (uint32 top, uint32 bottom, uint32 ear, uint32 earInk, Colour section, uint32 ink, uint32 inkDim,
                           uint32 title, uint32 accent, float grain, bool brushed, bool wood)
    {
        Dx::Finish f;
        f.plateTop = Colour (top); f.plateBottom = Colour (bottom); f.ear = Colour (ear); f.earInk = Colour (earInk);
        f.section = section; f.ink = Colour (ink); f.inkDim = Colour (inkDim); f.title = Colour (title); f.accent = Colour (accent);
        f.grain = grain; f.brushed = brushed; f.woodCheeks = wood; f.sectionInk = f.ink;
        return f;
    }
    const Colour darkRecess  = Colours::black.withAlpha (0.24f);
    const Colour lightRecess = Colours::black.withAlpha (0.075f);
}

Dx::Finish Dx::Finish::gunmetal()  { return makeFinish (0xff2E3034, 0xff1D1E21, 0xff141517, 0xff8A8D93, darkRecess,  0xffD4D6DA, 0xff868A91, 0xffF4F4F2, 0xffE0452F, 0.050f, true,  false); }
Dx::Finish Dx::Finish::champagne() { return makeFinish (0xffD3C9B6, 0xffBDB19B, 0xff8F8676, 0xff3A352D, lightRecess, 0xff2A2622, 0xff6D665B, 0xff1E1B17, 0xffD57A26, 0.045f, false, false); }
Dx::Finish Dx::Finish::ivory()     { return makeFinish (0xffE9E1CB, 0xffD6CBB0, 0xff8C7F66, 0xff3A3025, lightRecess, 0xff3A2D22, 0xff7A6B58, 0xff6E1F1A, 0xff9E2B22, 0.040f, false, true); }
Dx::Finish Dx::Finish::aluminium() { return makeFinish (0xffCDD1D5, 0xffB0B5BA, 0xff7C8288, 0xff2A2D30, lightRecess, 0xff24272A, 0xff62676D, 0xff121416, 0xffC7372C, 0.030f, true,  false); }
Dx::Finish Dx::Finish::neveBlue()  { return makeFinish (0xff5B6C80, 0xff46546A, 0xff2B333D, 0xffAEB8C4, Colours::black.withAlpha (0.18f), 0xffE8EBEE, 0xffAFBAC6, 0xffFFFFFF, 0xffD33A2C, 0.045f, false, false); }
Dx::Finish Dx::Finish::sage()      { return makeFinish (0xffADB9A7, 0xff95A28F, 0xff66735F, 0xff1E241C, lightRecess, 0xff1F251E, 0xff4E5A4B, 0xff151A14, 0xff2E8E6A, 0.045f, false, false); }
Dx::Finish Dx::Finish::midnight()  { return makeFinish (0xff222E40, 0xff161E2B, 0xff0D121A, 0xff7F8FAE, darkRecess,  0xffC9D2E1, 0xff7F8BA0, 0xffAFC3F4, 0xff8EA8F2, 0.050f, false, false); }
Dx::Finish Dx::Finish::obsidian()  { return makeFinish (0xff1E1E20, 0xff121213, 0xff0B0B0C, 0xff6F6F72, darkRecess,  0xffDADADA, 0xff8A8A8D, 0xffEBCB8B, 0xffD9A63C, 0.050f, true,  false); }
Dx::Finish Dx::Finish::slate()     { return makeFinish (0xff363C43, 0xff272C31, 0xff16191C, 0xff8A939C, darkRecess,  0xffC8CFD6, 0xff838C95, 0xffFFFFFF, 0xff3CC46B, 0.050f, false, false); }
Dx::Finish Dx::Finish::cocoa()     { return makeFinish (0xff34271F, 0xff211913, 0xff120E0B, 0xffA88F72, darkRecess,  0xffDCCAB2, 0xff9C8770, 0xffEAD9BF, 0xffD99A45, 0.050f, false, false); }
Dx::Finish Dx::Finish::tiger()     { return makeFinish (0xffDCE0D6, 0xffC6CCBF, 0xff97ABA3, 0xff243230, lightRecess, 0xff272C28, 0xff5E655F, 0xff1B1F1B, 0xff3A8FD6, 0.035f, false, false); }
Dx::Finish Dx::Finish::tigerNight() { return makeFinish (0xff2A2E2D, 0xff1B1E1D, 0xff4F635E, 0xffBFD3CC, darkRecess, 0xffD6DDD8, 0xff8A948F, 0xffE9EFEA, 0xff5AA8E8, 0.045f, false, false); }
Dx::Finish Dx::Finish::goldSix()   { return makeFinish (0xff1F1F21, 0xff121213, 0xff0A0A0B, 0xff77777A, darkRecess,  0xffDEDBD3, 0xff8D8A84, 0xffF1D08A, 0xffF0A12A, 0.050f, true,  false); }
Dx::Finish Dx::Finish::mahogany()
{
    auto f = makeFinish (0xff1F1B18, 0xff141110, 0xff0C0A09, 0xff8A7B6C, Colour (0xffDC7428), 0xffE8DCCB, 0xff9A8B7B, 0xffF08A2C, 0xffF08A2C, 0.050f, false, true);
    f.filledSections = true; f.sectionInk = Colour (0xff1E1510);
    return f;
}
Dx::Finish Dx::Finish::oxblood()
{
    return makeFinish (0xff2C1B19, 0xff1B1110, 0xff0E0908, 0xff9A7E76, darkRecess, 0xffE6D6C8, 0xff9C8579, 0xffF2E3D2, 0xffD0412F, 0.050f, false, true);
}

// =====================================================================================================================
// STUDIO SERIES — painters
// =====================================================================================================================
const Image& Paint::grainTile()
{
    static Image tile = []
    {
        Image img (Image::ARGB, 128, 128, true);
        Random rnd (0x5EEDD);
        Image::BitmapData bd (img, Image::BitmapData::writeOnly);
        for (int y = 0; y < 128; ++y)
            for (int x = 0; x < 128; ++x)
            {
                const float n = rnd.nextFloat();
                const bool lightSpeck = n > 0.5f;
                const uint8 a = (uint8) (std::abs (n - 0.5f) * 2.f * 255.f);
                bd.setPixelColour (x, y, (lightSpeck ? Colours::white : Colours::black).withAlpha (a));
            }
        return img;
    }();
    return tile;
}

void Paint::faceplate (Graphics& g, Rectangle<float> r, const Dx::Finish& f, float radius)
{
    Path shape; shape.addRoundedRectangle (r, radius);
    g.setGradientFill (ColourGradient (f.plateTop, 0.f, r.getY(), f.plateBottom, 0.f, r.getBottom(), false));
    g.fillPath (shape);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (shape);
        // powder-coat grain
        g.setTiledImageFill (grainTile(), 0, 0, f.grain);
        g.fillRect (r);
        // brushing: deterministic horizontal hairlines
        if (f.brushed)
        {
            Random rnd (0xB1234);
            for (float y = r.getY(); y < r.getBottom(); y += 1.f)
            {
                const float a = rnd.nextFloat() * (f.isLight() ? 0.05f : 0.035f);
                g.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (a));
                const float x0 = r.getX() + rnd.nextFloat() * r.getWidth() * 0.3f;
                g.fillRect (x0, y, r.getWidth() * (0.4f + rnd.nextFloat() * 0.6f), 1.f);
            }
        }
        // light falls from the top: a soft sheen on the upper third
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (f.isLight() ? 0.18f : 0.05f), 0.f, r.getY(),
                                           Colours::transparentWhite, 0.f, r.getY() + r.getHeight() * 0.4f, false));
        g.fillRect (r);
    }
    // bevel: bright top edge, dark bottom edge, fine outline
    g.setColour (Colours::white.withAlpha (f.isLight() ? 0.55f : 0.14f));
    g.drawHorizontalLine (roundToInt (r.getY() + 1.f), r.getX() + radius, r.getRight() - radius);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.drawHorizontalLine (roundToInt (r.getBottom() - 1.f), r.getX() + radius, r.getRight() - radius);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.strokePath (shape, PathStrokeType (1.f));
}

void Paint::phillips (Graphics& g, Point<float> c, float rad, float angle)
{
    auto disc = Rectangle<float> (rad * 2.f, rad * 2.f).withCentre (c);
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillEllipse (disc.translated (0.f, rad * 0.25f).expanded (rad * 0.12f));
    g.setGradientFill (ColourGradient (Colour (0xffE9E9E6), c.x - rad * 0.5f, c.y - rad * 0.6f,
                                       Colour (0xff6F7074), c.x + rad * 0.6f, c.y + rad * 0.8f, false));
    g.fillEllipse (disc);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawEllipse (disc, jmax (0.6f, rad * 0.12f));
    // cross slot
    const float l = rad * 0.62f, w = jmax (0.8f, rad * 0.22f);
    for (int k = 0; k < 2; ++k)
    {
        const float a = angle + (float) k * MathConstants<float>::halfPi;
        Line<float> ln (c.x - std::cos (a) * l, c.y - std::sin (a) * l, c.x + std::cos (a) * l, c.y + std::sin (a) * l);
        g.setColour (Colours::black.withAlpha (0.7f));
        g.drawLine (ln, w);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.drawLine (ln.getStartX() + 0.4f, ln.getStartY() + 0.5f, ln.getEndX() + 0.4f, ln.getEndY() + 0.5f, jmax (0.4f, w * 0.35f));
    }
}

void Paint::engrave (Graphics& g, const String& text, Rectangle<float> r, Colour ink, Font font, Justification just,
                     bool lightSurface, float tracking)
{
    const auto t = utf8 (text);
    const auto f = font.withExtraKerningFactor (tracking);
    g.setFont (f);
    g.setColour (lightSurface ? Colours::white.withAlpha (0.55f) : Colours::black.withAlpha (0.65f));
    g.drawText (t, r.translated (0.f, lightSurface ? 0.8f : 0.9f), just, false);
    g.setColour (ink);
    g.drawText (t, r, just, false);
}

void Paint::rackEar (Graphics& g, Rectangle<float> r, const Dx::Finish& f, bool left, const String& engraving, bool brandFont)
{
    g.setGradientFill (ColourGradient (f.ear.brighter (0.12f), r.getX(), 0.f, f.ear.darker (0.25f), r.getRight(), 0.f, false));
    if (! left) g.setGradientFill (ColourGradient (f.ear.darker (0.25f), r.getX(), 0.f, f.ear.brighter (0.12f), r.getRight(), 0.f, false));
    g.fillRect (r);
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.toNearestInt());
        g.setTiledImageFill (grainTile(), 0, 0, 0.06f);
        g.fillRect (r);
    }
    // the fold where the ear meets the faceplate
    const float foldX = left ? r.getRight() - 2.f : r.getX();
    g.setColour (Colours::black.withAlpha (0.55f)); g.fillRect (foldX, r.getY(), 2.f, r.getHeight());
    g.setColour (Colours::white.withAlpha (0.10f)); g.fillRect (left ? r.getX() : r.getRight() - 1.f, r.getY(), 1.f, r.getHeight());

    // two oval mounting slots with screws
    const float sw = r.getWidth() * 0.46f, sh = jmin (22.f, r.getHeight() * 0.12f);
    for (float cy : { r.getY() + 14.f + sh * 0.5f, r.getBottom() - 14.f - sh * 0.5f })
    {
        auto slot = Rectangle<float> (sw, sh).withCentre ({ r.getCentreX(), cy });
        g.setColour (Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (slot, sw * 0.5f);
        g.setColour (Colours::white.withAlpha (0.12f));
        g.drawRoundedRectangle (slot.translated (0.f, 0.8f), sw * 0.5f, 0.8f);
        phillips (g, { slot.getCentreX(), slot.getCentreY() + (left ? -2.f : 2.f) }, sw * 0.42f, left ? 0.35f : 1.1f);
    }

    // vertical engraving between the slots
    if (engraving.isNotEmpty())
    {
        Graphics::ScopedSaveState ss (g);
        const auto c = r.getCentre();
        g.addTransform (AffineTransform::rotation (left ? -MathConstants<float>::halfPi : MathConstants<float>::halfPi, c.x, c.y));
        const float len = r.getHeight() - 2.f * (sh + 30.f);
        auto area = Rectangle<float> (len, r.getWidth()).withCentre (c);
        const auto font = brandFont ? Fonts::righteous (jmin (13.f, r.getWidth() * 0.46f))
                                    : Fonts::engraved (jmin (11.f, r.getWidth() * 0.40f));
        engrave (g, engraving, area, f.earInk, font, Justification::centred, Paint::isLight (f.ear), brandFont ? 0.12f : 0.22f);
    }
}

void Paint::woodCheek (Graphics& g, Rectangle<float> r, bool left)
{
    Path shape; shape.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), 5.f, 5.f, left, false, left, false);
    const auto& wood = Art::wood();
    {
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (shape);
        if (wood.isValid())
        {
            g.setTiledImageFill (wood, roundToInt (r.getX()), 0, 1.f);
            g.fillRect (r);
        }
        else { g.setColour (Dx::Palette::walnut); g.fillRect (r); }
        // round the cheek: light on the outer face, dark into the joint
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.18f), left ? r.getX() : r.getRight(), 0.f,
                                           Colours::black.withAlpha (0.45f), left ? r.getRight() : r.getX(), 0.f, false));
        g.fillRect (r);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), 0.f, r.getY(), Colours::black.withAlpha (0.25f), 0.f, r.getBottom(), false));
        g.fillRect (r);
    }
    g.setColour (Colours::black.withAlpha (0.7f));
    g.strokePath (shape, PathStrokeType (1.f));
}

void Paint::section (Graphics& g, Rectangle<float> r, const Dx::Finish& f, const String& title, bool screws)
{
    const float rad = 4.f;
    const bool light = f.isLight();
    if (f.filledSections)
    {
        softShadow (g, r, rad, 0.35f, 5.f, 2.f);
        g.setGradientFill (ColourGradient (f.section.brighter (0.10f), 0.f, r.getY(), f.section.darker (0.12f), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, rad + 2.f);
        {
            Graphics::ScopedSaveState ss (g);
            Path p; p.addRoundedRectangle (r, rad + 2.f); g.reduceClipRegion (p);
            g.setTiledImageFill (grainTile(), 0, 0, 0.05f); g.fillRect (r);
        }
        g.setColour (Colours::white.withAlpha (0.25f)); g.drawHorizontalLine (roundToInt (r.getY() + 1.f), r.getX() + 6.f, r.getRight() - 6.f);
        g.setColour (Colours::black.withAlpha (0.5f));  g.drawRoundedRectangle (r, rad + 2.f, 1.f);
    }
    else
    {
        g.setColour (f.section);
        g.fillRoundedRectangle (r, rad);
        innerShadow (g, r, rad, light ? 0.22f : 0.55f, 6, 2);
        // groove: dark line + light lip below
        g.setColour (Colours::black.withAlpha (light ? 0.30f : 0.65f));
        g.drawRoundedRectangle (r, rad, 1.f);
        g.setColour (Colours::white.withAlpha (light ? 0.45f : 0.07f));
        g.drawRoundedRectangle (r.translated (0.f, 1.f).reduced (-0.5f), rad, 0.7f);
    }
    if (screws)
        for (auto c : { r.getTopLeft().translated (7.f, 7.f), r.getTopRight().translated (-7.f, 7.f),
                        r.getBottomLeft().translated (7.f, -7.f), r.getBottomRight().translated (-7.f, -7.f) })
            phillips (g, c, 2.6f, 0.6f + c.x * 0.01f);
    if (title.isNotEmpty())
    {
        const bool filledLight = f.filledSections ? Paint::isLight (f.section) : light;
        engrave (g, title.toUpperCase(), r.reduced (screws ? 14.f : 8.f, 4.f).removeFromTop (14.f),
                 f.filledSections ? f.sectionInk : f.inkDim, Fonts::engraved (9.5f), Justification::centredLeft, filledLight, 0.18f);
    }
}

void Paint::knob (Graphics& g, Point<float> c, float R, float ang, int body, Colour cap, Colour accent, float alpha)
{
    const float cx = c.x, cy = c.y;
    auto circle = [] (Point<float> p, float r) { return Rectangle<float> (r * 2.f, r * 2.f).withCentre (p); };
    auto at = [&] (float a, float r) { return Point<float> (cx + std::sin (a) * r, cy - std::cos (a) * r); };

    // contact shadow
    softShadow (g, circle ({ cx, cy + R * 0.10f }, R * 0.98f), R, 0.55f * alpha, R * 0.22f, R * 0.08f);

    if (body == 0 || body == 3)            // ---- Marconi: fluted black skirt + coloured cap (3 = Chrome skirt)
    {
        const bool chrome = body == 3;
        const Colour sk0 = chrome ? Colour (0xffE4E5E6) : Colour (0xff3A3B3D), sk1 = chrome ? Colour (0xff6D6F72) : Colour (0xff0C0C0D);
        g.setGradientFill (ColourGradient (sk0.withMultipliedAlpha (alpha), cx - R * 0.6f, cy - R, sk1.withMultipliedAlpha (alpha), cx + R * 0.5f, cy + R, false));
        g.fillEllipse (circle (c, R));
        // flutes
        const int n = 30;
        for (int i = 0; i < n; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / (float) n;
            const float lit = 0.5f + 0.5f * std::cos (a + 0.8f);
            g.setColour ((chrome ? Colours::black : Colours::white).withAlpha ((chrome ? 0.10f : 0.05f + 0.10f * lit) * alpha));
            g.drawLine (Line<float> (at (a, R * 0.80f), at (a, R * 0.985f)), jmax (0.8f, R * 0.045f));
        }
        g.setColour (Colours::black.withAlpha (0.7f * alpha)); g.drawEllipse (circle (c, R), 1.f);
        // skirt index line
        g.setColour ((chrome ? Colours::black : Colours::white).withAlpha (0.9f * alpha));
        g.drawLine (Line<float> (at (ang, R * 0.78f), at (ang, R * 0.99f)), jmax (1.4f, R * 0.07f));
        // cap
        const float cr = R * 0.66f;
        softShadow (g, circle ({ cx, cy + cr * 0.06f }, cr), cr, 0.45f * alpha, cr * 0.12f, cr * 0.05f);
        g.setGradientFill (ColourGradient (cap.brighter (0.45f).withMultipliedAlpha (alpha), cx - cr * 0.6f, cy - cr * 0.8f,
                                           cap.darker (0.55f).withMultipliedAlpha (alpha), cx + cr * 0.6f, cy + cr, false));
        g.fillEllipse (circle (c, cr));
        g.setColour (Colours::white.withAlpha (0.30f * alpha));
        g.fillEllipse (circle ({ cx - cr * 0.18f, cy - cr * 0.34f }, cr * 0.46f).withHeight (cr * 0.38f));
        g.setColour (Colours::black.withAlpha (0.5f * alpha)); g.drawEllipse (circle (c, cr), 0.8f);
        // cap pointer
        g.setColour ((Paint::isLight (cap) ? Colours::black : Colours::white).withAlpha (0.85f * alpha));
        g.drawLine (Line<float> (at (ang, cr * 0.25f), at (ang, cr * 0.92f)), jmax (1.2f, cr * 0.09f));
    }
    else if (body == 1)                    // ---- Chunky: cream knurled
    {
        const Colour c0 = cap.isTransparent() ? Colour (0xffF4F1E8) : cap.brighter (0.2f), c1 = cap.isTransparent() ? Colour (0xffB9B4A8) : cap.darker (0.35f);
        g.setGradientFill (ColourGradient (c0.withMultipliedAlpha (alpha), cx - R * 0.7f, cy - R, c1.withMultipliedAlpha (alpha), cx + R * 0.5f, cy + R, false));
        g.fillEllipse (circle (c, R));
        const int n = 48;
        for (int i = 0; i < n; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / (float) n;
            g.setColour (Colours::black.withAlpha (0.16f * alpha));
            g.drawLine (Line<float> (at (a, R * 0.84f), at (a, R * 0.99f)), jmax (0.7f, R * 0.028f));
        }
        g.setColour (Colours::black.withAlpha (0.45f * alpha)); g.drawEllipse (circle (c, R), 1.f);
        // dished top
        const float tr = R * 0.78f;
        g.setGradientFill (ColourGradient (c1.brighter (0.25f).withMultipliedAlpha (alpha), cx, cy - tr, c0.withMultipliedAlpha (alpha), cx, cy + tr, false));
        g.fillEllipse (circle (c, tr));
        g.setColour (Colours::white.withAlpha (0.55f * alpha)); g.drawEllipse (circle (c, tr).translated (0.f, 0.6f), 0.8f);
        g.setColour (Colour (0xff171717).withMultipliedAlpha (alpha));
        g.drawLine (Line<float> (at (ang, tr * 0.18f), at (ang, R * 0.96f)), jmax (1.6f, R * 0.075f));
    }
    else                                   // ---- Bakelite: glossy black on a knurled metal skirt
    {
        g.setGradientFill (ColourGradient (Colour (0xffD8D9DB).withMultipliedAlpha (alpha), cx - R, cy - R, Colour (0xff5B5D61).withMultipliedAlpha (alpha), cx + R, cy + R, false));
        g.fillEllipse (circle (c, R));
        const int n = 56;
        for (int i = 0; i < n; ++i)
        {
            const float a = MathConstants<float>::twoPi * (float) i / (float) n;
            g.setColour (Colours::black.withAlpha (0.25f * alpha));
            g.drawLine (Line<float> (at (a, R * 0.86f), at (a, R * 0.99f)), jmax (0.6f, R * 0.022f));
        }
        g.setColour (Colours::black.withAlpha (0.6f * alpha)); g.drawEllipse (circle (c, R), 0.9f);
        const float br = R * 0.80f;
        softShadow (g, circle ({ cx, cy + br * 0.05f }, br), br, 0.5f * alpha, br * 0.1f, br * 0.05f);
        g.setGradientFill (ColourGradient (Colour (0xff4A4A4D).withMultipliedAlpha (alpha), cx - br * 0.6f, cy - br * 0.9f,
                                           Colour (0xff060607).withMultipliedAlpha (alpha), cx + br * 0.4f, cy + br, false));
        g.fillEllipse (circle (c, br));
        g.setColour (Colours::white.withAlpha (0.22f * alpha));
        g.fillEllipse (circle ({ cx - br * 0.22f, cy - br * 0.40f }, br * 0.42f).withHeight (br * 0.30f));
        g.setColour (Colours::black.withAlpha (0.7f * alpha)); g.drawEllipse (circle (c, br), 0.8f);
        g.setColour ((cap.isTransparent() ? Colours::white : cap).withMultipliedAlpha (0.95f * alpha));
        g.drawLine (Line<float> (at (ang, br * 0.30f), at (ang, br * 0.95f)), jmax (1.5f, br * 0.085f));
    }
    // accent dot at the pointer tip (the Deestech signature)
    if (! accent.isTransparent())
    {
        const float dotR = jmax (1.4f, R * 0.07f);
        const float dr = body == 0 || body == 3 ? R * 0.53f : body == 1 ? R * 0.62f : R * 0.62f;
        g.setColour (accent.withMultipliedAlpha (alpha));
        g.fillEllipse (circle (at (ang, dr), dotR));
    }
}

void Paint::knobScale (Graphics& g, Point<float> c, float R, float a0, float a1, int ticks, const StringArray& labels,
                       Colour ink, bool light, float labelSize)
{
    ticks = jmax (2, ticks);
    auto at = [&] (float a, float r) { return Point<float> (c.x + std::sin (a) * r, c.y - std::cos (a) * r); };
    const int minorPer = 1;
    const int total = (ticks - 1) * (minorPer + 1) + 1;
    for (int i = 0; i < total; ++i)
    {
        const float t = (float) i / (float) (total - 1);
        const float a = a0 + (a1 - a0) * t;
        const bool major = i % (minorPer + 1) == 0;
        const float r0 = R * 1.10f, r1 = R * (major ? 1.24f : 1.17f);
        g.setColour (ink.withAlpha (major ? 0.95f : 0.55f));
        g.drawLine (Line<float> (at (a, r0), at (a, r1)), major ? 1.1f : 0.8f);
    }
    if (labels.isEmpty()) return;
    const auto font = Fonts::engraved (labelSize);
    const int n = labels.size();
    for (int k = 0; k < n; ++k)
    {
        const float t = n == 1 ? 0.5f : (float) k / (float) (n - 1);
        const float a = a0 + (a1 - a0) * t;
        const auto p = at (a, R * 1.24f + labelSize * 0.95f);
        auto box = Rectangle<float> (labelSize * 3.2f, labelSize * 1.2f).withCentre (p);
        engrave (g, labels[k], box, ink, font, Justification::centred, light, 0.02f);
    }
}

void Paint::lampButton (Graphics& g, Rectangle<float> r, Colour lens, bool lit, bool over, bool down, const String& text, bool lightSurface)
{
    ignoreUnused (lightSurface);
    // bezel
    softShadow (g, r, 3.f, 0.5f, 3.f, 1.5f);
    g.setGradientFill (ColourGradient (Colour (0xff3C3D40), 0.f, r.getY(), Colour (0xff111112), 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (Colours::black); g.drawRoundedRectangle (r, 3.f, 1.f);
    auto lensR = r.reduced (jmax (2.f, r.getHeight() * 0.12f)).translated (0.f, down ? 0.8f : 0.f);
    const Colour on = lens, off = lens.withSaturation (lens.getSaturation() * 0.55f).darker (1.4f);
    if (lit)
    {
        softShadow (g, lensR, 2.f, 0.0f, 0.f, 0.f);
        g.setColour (on.withAlpha (0.35f)); g.fillRoundedRectangle (lensR.expanded (3.f), 4.f);
    }
    g.setGradientFill (ColourGradient ((lit ? on.brighter (0.5f) : off.brighter (0.25f)), lensR.getCentreX(), lensR.getY(),
                                       (lit ? on.darker (0.15f) : off.darker (0.4f)), lensR.getCentreX(), lensR.getBottom(), false));
    g.fillRoundedRectangle (lensR, 2.f);
    if (lit)
    {
        ColourGradient glow (Colours::white.withAlpha (0.55f), lensR.getCentreX(), lensR.getCentreY(),
                             Colours::transparentWhite, lensR.getRight(), lensR.getBottom(), true);
        g.setGradientFill (glow); g.fillRoundedRectangle (lensR, 2.f);
    }
    g.setColour (Colours::white.withAlpha (over ? 0.30f : 0.18f));
    g.fillRoundedRectangle (lensR.withHeight (lensR.getHeight() * 0.42f).reduced (1.f, 0.5f), 1.5f);
    g.setColour (Colours::black.withAlpha (0.6f)); g.drawRoundedRectangle (lensR, 2.f, 0.8f);
    if (text.isNotEmpty())
    {
        const auto ink = lit ? (Paint::isLight (on) ? Colour (0xff1A1410) : Colours::white) : Colours::white.withAlpha (0.55f);
        g.setColour (ink);
        g.setFont (Fonts::engraved (jmin (12.f, lensR.getHeight() * 0.58f)).withExtraKerningFactor (0.10f));
        g.drawFittedText (utf8 (text), lensR.toNearestInt().reduced (2, 0), Justification::centred, 1, 0.7f);
    }
}

void Paint::led (Graphics& g, Rectangle<float> r, Colour c, float amount)
{
    amount = jlimit (0.f, 1.f, amount);
    g.setColour (Colours::black.withAlpha (0.8f)); g.fillRoundedRectangle (r.expanded (0.8f), 1.5f);
    const Colour off = c.withSaturation (c.getSaturation() * 0.5f).darker (2.2f);
    const Colour col = off.interpolatedWith (c.brighter (0.3f), amount);
    if (amount > 0.05f) { g.setColour (c.withAlpha (0.25f * amount)); g.fillRoundedRectangle (r.expanded (2.f), 2.5f); }
    g.setGradientFill (ColourGradient (col.brighter (0.3f), 0.f, r.getY(), col.darker (0.2f), 0.f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 1.f);
    g.setColour (Colours::white.withAlpha (0.12f + 0.25f * amount));
    g.fillRect (r.reduced (1.f, 0.f).withHeight (jmax (1.f, r.getHeight() * 0.3f)));
}
