// DeestechUpdate.cpp — see DeestechUpdate.h. CANONICAL COPY: DeesTapeDesktop/Source/Updates/.
#include "DeestechUpdate.h"

namespace dtupdate
{
using namespace juce;

// =====================================================================================================================
// pure helpers
// =====================================================================================================================
static Array<int> versionParts (String v)
{
    v = v.trim();
    if (v.startsWithIgnoreCase ("v")) v = v.substring (1);
    Array<int> parts;
    for (auto& token : StringArray::fromTokens (v, ".", {}))
        parts.add (token.retainCharacters ("0123456789").isEmpty() ? 0
                                                                   : token.initialSectionContainingOnly ("0123456789").getIntValue());
    return parts;
}

int compareVersions (const String& a, const String& b)
{
    const auto pa = versionParts (a), pb = versionParts (b);
    const int n = jmax (pa.size(), pb.size());
    for (int i = 0; i < n; ++i)
    {
        const int x = i < pa.size() ? pa[i] : 0, y = i < pb.size() ? pb[i] : 0;
        if (x != y) return x < y ? -1 : 1;
    }
    return 0;
}

String platformKey()
{
   #if JUCE_MAC
    return "mac";
   #elif JUCE_WINDOWS
    return "win";
   #else
    return "linux";
   #endif
}

static bool isLoopbackHttp (const String& u)
{
    return u.startsWithIgnoreCase ("http://127.0.0.1:") || u.startsWithIgnoreCase ("http://127.0.0.1/")
        || u.startsWithIgnoreCase ("http://localhost:") || u.startsWithIgnoreCase ("http://localhost/");
}

static bool acceptableUrl (const String& u)   { return u.startsWithIgnoreCase ("https://") || isLoopbackHttp (u); }

String feedUrl()
{
    const auto over = SystemStats::getEnvironmentVariable ("DEESTECH_UPDATE_FEED", {}).trim();
    return over.isNotEmpty() && acceptableUrl (over) ? over : String (kFeedUrl);
}

bool Release::isValid() const
{
    return productId.isNotEmpty() && version.isNotEmpty() && compareVersions (version, "0") > 0 && acceptableUrl (url);
}

bool isValidFeed (const var& feed)
{
    return feed.isObject() && (int) feed.getProperty ("schema", 0) == kSchema && feed.getProperty ("products", {}).isObject();
}

static String absolute (const String& u)
{
    if (u.startsWith ("/")) return String (kSiteRoot) + u;
    return u;
}

Release findRelease (const var& feed, const String& productId, const String& platform)
{
    Release r;
    if (! isValidFeed (feed) || productId.isEmpty()) return r;

    const auto p = feed.getProperty ("products", {}).getProperty (Identifier (productId), {});
    if (! p.isObject()) return r;
    const auto build = p.getProperty (Identifier (platform), {});
    if (! build.isObject()) return r;

    r.productId = productId;
    r.name      = p.getProperty ("name", productId).toString();
    r.page      = absolute (p.getProperty ("page", {}).toString());
    r.date      = p.getProperty ("date", {}).toString();
    r.critical  = (bool) p.getProperty ("critical", false);
    if (auto* notes = p.getProperty ("notes", {}).getArray())
        for (auto& line : *notes)
            if (line.toString().trim().isNotEmpty()) r.notes.add (line.toString().trim());

    r.version = build.getProperty ("version", {}).toString().trim();
    r.url     = absolute (build.getProperty ("url", {}).toString().trim());
    r.sha256  = build.getProperty ("sha256", {}).toString().trim().toLowerCase();
    r.size    = (int64) build.getProperty ("size", 0);
    if (! acceptableUrl (r.page)) r.page = {};
    if (! r.isValid()) return {};
    return r;
}

// =====================================================================================================================
// per-computer state: <app data>/Deestech/Updates/{feed.json,state.json}
// Every Deestech product (and every plug-in instance) shares these files. Writes are whole-file replacements under an
// inter-process lock, so two hosts checking at the same moment cannot interleave.
// =====================================================================================================================
File updatesFolder()
{
    // tests (and only tests) point this somewhere disposable
    const auto over = SystemStats::getEnvironmentVariable ("DEESTECH_UPDATE_DIR", {}).trim();
    if (over.isNotEmpty() && File::isAbsolutePath (over))
    {
        File f (over);
        if (! f.isDirectory()) f.createDirectory();
        return f;
    }
   #if JUCE_MAC
    auto f = File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Application Support/Deestech/Updates");
   #else
    auto f = File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Deestech").getChildFile ("Updates");
   #endif
    if (! f.isDirectory()) f.createDirectory();
    return f;
}

static InterProcessLock& fileLock()
{
    static InterProcessLock l ("DeestechUpdatesState");
    return l;
}

static var readJson (const File& f)
{
    if (! f.existsAsFile() || f.getSize() > 2 * 1024 * 1024) return {};
    return JSON::parse (f.loadFileAsString());
}

static void writeJson (const File& f, const var& v)
{
    f.replaceWithText (JSON::toString (v, false));
}

static File stateFile() { return updatesFolder().getChildFile ("state.json"); }
static File cacheFile() { return updatesFolder().getChildFile ("feed.json"); }

// read-modify-write of state.json under the lock
template <typename Fn>
static void editState (Fn&& fn)
{
    const InterProcessLock::ScopedLockType sl (fileLock());
    auto st = readJson (stateFile());
    if (! st.isObject()) st = var (new DynamicObject());
    fn (*st.getDynamicObject());
    writeJson (stateFile(), st);
}

static var readState()
{
    const InterProcessLock::ScopedLockType sl (fileLock());
    auto st = readJson (stateFile());
    return st.isObject() ? st : var (new DynamicObject());
}

static var subObject (DynamicObject& o, const Identifier& id)
{
    auto v = o.getProperty (id);
    if (! v.isObject()) { v = var (new DynamicObject()); o.setProperty (id, v); }
    return v;
}

bool automaticChecksEnabled()           { return (bool) readState().getProperty ("enabled", true); }
void setAutomaticChecksEnabled (bool e) { editState ([e] (DynamicObject& o) { o.setProperty ("enabled", e); }); }

void skipVersion (const String& productId, const String& version)
{
    editState ([&] (DynamicObject& o) { subObject (o, "skipped").getDynamicObject()->setProperty (Identifier (productId), version); });
}

bool isSkipped (const String& productId, const String& version)
{
    const auto v = readState().getProperty ("skipped", {}).getProperty (Identifier (productId), {}).toString();
    return v.isNotEmpty() && compareVersions (v, version) >= 0;
}

void snooze (const String& productId, const String& version, int days)
{
    const auto until = Time::currentTimeMillis() + (int64) jmax (0, days) * 24 * 60 * 60 * 1000;
    editState ([&] (DynamicObject& o)
    {
        auto entry = var (new DynamicObject());
        entry.getDynamicObject()->setProperty ("version", version);
        entry.getDynamicObject()->setProperty ("until", until);
        subObject (o, "snoozed").getDynamicObject()->setProperty (Identifier (productId), entry);
    });
}

bool isSnoozed (const String& productId, const String& version)
{
    const auto e = readState().getProperty ("snoozed", {}).getProperty (Identifier (productId), {});
    return e.isObject() && compareVersions (e.getProperty ("version", {}).toString(), version) >= 0
        && (int64) e.getProperty ("until", 0) > Time::currentTimeMillis();
}

// =====================================================================================================================
// Service::Fetcher — one background GET of the feed. Cancellable: the owner's destructor cancels the stream and joins,
// so no thread ever outlives the plug-in binary that started it.
// =====================================================================================================================
class Service::Fetcher : public Thread
{
public:
    explicit Fetcher (Service& s) : Thread ("Deestech update check"), owner (s) {}
    ~Fetcher() override { cancel(); stopThread (10000); }

    void cancel()
    {
        signalThreadShouldExit();
        const ScopedLock sl (streamLock);
        if (stream != nullptr) stream->cancel();
    }

    void run() override
    {
        String err;
        var parsed;
        MemoryBlock body;

        {
            auto s = std::make_unique<WebInputStream> (URL (feedUrl()), false);
            s->withConnectionTimeout (8000)
              .withNumRedirectsToFollow (3)
              .withExtraHeaders ("Accept: application/json\r\nCache-Control: no-cache\r\n");
            { const ScopedLock sl (streamLock); stream = s.get(); }

            if (! threadShouldExit() && s->connect (nullptr))
            {
                const int status = s->getStatusCode();
                if (status == 200)
                {
                    char buf[8192];
                    while (! threadShouldExit() && ! s->isExhausted())
                    {
                        const int n = s->read (buf, (int) sizeof (buf));
                        if (n <= 0) break;
                        body.append (buf, (size_t) n);
                        if (body.getSize() > 1024 * 1024) { err = "The update feed is unexpectedly large."; break; }
                    }
                }
                else
                {
                    err = "The update server answered " + String (status) + ".";
                }
            }
            else if (! threadShouldExit())
            {
                err = "Could not reach deestechholdings.com. Check your internet connection.";
            }

            { const ScopedLock sl (streamLock); stream = nullptr; }
        }

        if (threadShouldExit()) return;

        const auto now = Time::currentTimeMillis();
        if (err.isEmpty())
        {
            parsed = JSON::parse (body.toString());
            if (! isValidFeed (parsed)) err = "The update feed could not be read.";
        }

        // Persist for every other Deestech product on this computer (and for the next launch).
        {
            const InterProcessLock::ScopedLockType sl (fileLock());
            auto cache = readJson (cacheFile());
            if (! cache.isObject()) cache = var (new DynamicObject());
            auto* o = cache.getDynamicObject();
            o->setProperty ("attemptedAt", now);
            if (err.isEmpty())
            {
                o->setProperty ("fetchedAt", now);
                o->setProperty ("feed", parsed);
            }
            writeJson (cacheFile(), cache);
        }

        owner.deliver (parsed, err, now);
    }

private:
    Service& owner;
    CriticalSection streamLock;
    WebInputStream* stream = nullptr;
};

// =====================================================================================================================
// Service
// =====================================================================================================================
Service::Service()  { loadCache(); }

Service::~Service()
{
    fetcher.reset();            // cancels + joins the network thread
    cancelPendingUpdate();
}

void Service::addListener (Listener* l)    { listeners.add (l); }
void Service::removeListener (Listener* l) { listeners.remove (l); }

bool   Service::isChecking() const    { return checking.load(); }
String Service::lastError() const     { const ScopedLock sl (lock); return error; }
int64  Service::lastSuccessMs() const { const ScopedLock sl (lock); return fetchedAt; }

void Service::loadCache()
{
    const auto cache = [] { const InterProcessLock::ScopedLockType sl (fileLock()); return readJson (cacheFile()); }();
    const auto f     = cache.getProperty ("feed", {});
    const auto at    = (int64) cache.getProperty ("fetchedAt", 0);
    if (! isValidFeed (f)) return;

    const ScopedLock sl (lock);
    if (at > fetchedAt) { feed = f; fetchedAt = at; }    // another product (or an earlier session) fetched more recently
}

void Service::check (bool force)
{
    loadCache();
    if (checking.load()) return;

    if (! force)
    {
        if (! automaticChecksEnabled()) return;
        const auto cache = [] { const InterProcessLock::ScopedLockType sl (fileLock()); return readJson (cacheFile()); }();
        const auto now = Time::currentTimeMillis();
        if (now - (int64) cache.getProperty ("fetchedAt", 0) < kFreshMs)   return;
        if (now - (int64) cache.getProperty ("attemptedAt", 0) < kRetryMs) return;
    }
    startFetch();
}

void Service::startFetch()
{
    if (fetcher != nullptr && fetcher->isThreadRunning()) return;
    fetcher.reset();
    checking = true;
    fetcher = std::make_unique<Fetcher> (*this);
    if (! fetcher->startThread (Thread::Priority::low))
    {
        checking = false;
        const ScopedLock sl (lock);
        error = "Could not start the update check.";
    }
}

void Service::deliver (const var& newFeed, const String& err, int64 when)
{
    {
        const ScopedLock sl (lock);
        pendingFeed = newFeed; pendingError = err; pendingAt = when; hasPending = true;
    }
    triggerAsyncUpdate();
}

void Service::handleAsyncUpdate()
{
    {
        const ScopedLock sl (lock);
        if (! hasPending) return;
        hasPending = false;
        error = pendingError;
        if (pendingError.isEmpty() && isValidFeed (pendingFeed)) { feed = pendingFeed; fetchedAt = pendingAt; }
        pendingFeed = var();
    }
    checking = false;
    listeners.call ([] (Listener& l) { l.updateInfoChanged(); });
}

Release Service::latest (const String& productId) const
{
    const ScopedLock sl (lock);
    return findRelease (feed, productId);
}

Release Service::availableUpdate (const String& productId, const String& currentVersion, bool includeSkipped) const
{
    auto r = latest (productId);
    if (! r.isValid() || compareVersions (r.version, currentVersion) <= 0) return {};
    if (! includeSkipped && ! r.critical && isSkipped (productId, r.version)) return {};
    return r;
}

// =====================================================================================================================
// UI
// =====================================================================================================================
namespace
{
    // A flat button painted entirely here, so it looks the same under any host / plug-in LookAndFeel.
    struct FlatButton : public Button
    {
        FlatButton (const String& text, Colour f, Colour t, Font fo, float c)
            : Button (text), fill (f), ink (t), font (fo), corner (c) { setMouseCursor (MouseCursor::PointingHandCursor); }

        void paintButton (Graphics& g, bool over, bool down) override
        {
            auto r = getLocalBounds().toFloat().reduced (0.5f);
            auto c = fill;
            if (down) c = c.darker (0.2f); else if (over) c = c.brighter (0.12f);
            if (! c.isTransparent()) { g.setColour (c); g.fillRoundedRectangle (r, corner); }
            else { g.setColour (ink.withAlpha (over ? 0.55f : 0.30f)); g.drawRoundedRectangle (r, corner, 1.0f); }
            g.setColour (ink);
            g.setFont (font);
            g.drawFittedText (getButtonText(), getLocalBounds().reduced (6, 0), Justification::centred, 1, 0.8f);
        }

        Colour fill, ink; Font font; float corner;
    };

    // Dimmed backdrop over the whole window + a centred card. Click outside / Esc / × closes it.
    class DetailsPanel : public Component
    {
    public:
        DetailsPanel (const Release& r, const String& current, const Style& s, std::function<void()> closed, std::function<void()> skipped)
            : release (r), style (s), onClosed (std::move (closed)), onSkipped (std::move (skipped)),
              download ("DOWNLOAD " + r.version, s.accent, s.pillText, s.font, s.corner),
              skip ("SKIP THIS VERSION", Colours::transparentBlack, s.panelDim, s.font, s.corner),
              closeX (String::fromUTF8 ("\xc3\x97"), Colours::transparentBlack, s.panelDim, s.font, s.corner)
        {
            title = r.name + " " + r.version + " is available";
            sub   = "You have " + current + (r.date.isNotEmpty() ? String::fromUTF8 ("  \xc2\xb7  released ") + r.date : String());
            lines = r.notes;
            if (lines.isEmpty()) lines.add ("Fixes and improvements.");

            addAndMakeVisible (download);
            addAndMakeVisible (skip);
            addAndMakeVisible (closeX);
            skip.setVisible (! r.critical);
            closeX.setTooltip ("Close");

            download.onClick = [this] { URL (release.url).launchInDefaultBrowser(); close(); };
            skip.onClick     = [this] { skipVersion (release.productId, release.version); if (onSkipped) onSkipped(); close(); };
            closeX.onClick   = [this] { close(); };
            setWantsKeyboardFocus (true);
        }

        void resized() override
        {
            // the card: up to 340 wide, as many note lines as fit
            const int w = jmin (340, getWidth() - 20);
            const int fixedH = 20 + 20 + 16 + 10 + 12 + 26 + 14;          // padding, title, sub, gap, gap, keys, padding
            const int maxLines = jmax (1, (getHeight() - 20 - fixedH) / 16);
            shown = jmin (lines.size(), jmin (6, maxLines));
            const int h = fixedH + shown * 16;
            card = Rectangle<int> (w, jmin (h, getHeight() - 20)).withCentre (getLocalBounds().getCentre());

            auto r = card.reduced (14, 10);
            closeX.setBounds (card.getRight() - 30, card.getY() + 6, 24, 20);
            auto keys = r.removeFromBottom (26);
            download.setBounds (keys.removeFromLeft (skip.isVisible() ? jmin (160, keys.getWidth() / 2) : keys.getWidth()));
            keys.removeFromLeft (8);
            skip.setBounds (keys);
        }

        void paint (Graphics& g) override
        {
            g.fillAll (Colours::black.withAlpha (0.45f));
            g.setColour (Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (card.toFloat().translated (0.f, 3.f).expanded (2.f), style.corner + 4.f);
            g.setColour (style.panelFill);
            g.fillRoundedRectangle (card.toFloat(), style.corner + 2.f);
            g.setColour (style.accent);
            g.fillRoundedRectangle (card.toFloat().withWidth (4.f), 2.f);

            auto r = card.reduced (14, 10);
            g.setColour (style.panelText);
            g.setFont (style.font.withHeight (13.5f));
            g.drawFittedText (title, r.removeFromTop (20).withTrimmedRight (22), Justification::centredLeft, 1, 0.7f);
            g.setColour (style.panelDim);
            g.setFont (style.font.withHeight (10.f).withStyle (Font::plain));
            g.drawFittedText (sub, r.removeFromTop (16), Justification::centredLeft, 1, 0.8f);
            r.removeFromTop (10);

            g.setFont (style.font.withHeight (11.f).withStyle (Font::plain));
            for (int i = 0; i < shown; ++i)
            {
                auto row = r.removeFromTop (16);
                g.setColour (style.accent);
                g.fillEllipse (row.removeFromLeft (10).toFloat().withSizeKeepingCentre (4.f, 4.f));
                g.setColour (style.panelText.withAlpha (0.9f));
                g.drawFittedText (lines[i], row, Justification::centredLeft, 1, 0.7f);
            }
        }

        void mouseDown (const MouseEvent& e) override    { if (! card.contains (e.getPosition())) close(); }
        bool keyPressed (const KeyPress& k) override     { if (k == KeyPress::escapeKey) { close(); return true; } return false; }

    private:
        void close()
        {
            setVisible (false);
            auto cb = onClosed;
            MessageManager::callAsync ([cb] { if (cb) cb(); });    // the owner destroys us — never from inside our own click
        }

        Release release; Style style; std::function<void()> onClosed, onSkipped;
        String title, sub; StringArray lines; int shown = 0;
        Rectangle<int> card;
        FlatButton download, skip, closeX;
    };
}

std::unique_ptr<Component> createDetailsPanel (Component& host, const Release& r, const String& currentVersion, const Style& s,
                                               std::function<void()> onClosed, std::function<void()> onSkipped)
{
    if (! r.isValid()) return {};
    auto panel = std::make_unique<DetailsPanel> (r, currentVersion, s, std::move (onClosed), std::move (onSkipped));
    host.addAndMakeVisible (*panel);
    panel->setBounds (host.getLocalBounds());
    panel->toFront (true);
    return panel;
}

UpdatePill::UpdatePill (String pid, String cur, Style s)
    : productId (std::move (pid)), current (std::move (cur)), style (std::move (s))
{
    setMouseCursor (MouseCursor::PointingHandCursor);
    setVisible (false);
    service->addListener (this);
    service->check (false);
    refresh();
}

UpdatePill::~UpdatePill() { details.reset(); service->removeListener (this); }

void UpdatePill::refresh()
{
    const bool was = isVisible();
    release = service->availableUpdate (productId, current);
    const bool now = release.isValid();
    if (now)
        setTooltip (release.name + " " + release.version + " is available (you have " + current + "). Click for details.");
    setVisible (now);
    repaint();
    if (was != now && onVisibilityChange) onVisibilityChange();
}

int UpdatePill::preferredWidth (int height) const
{
    const auto f = style.font.withHeight ((float) height * 0.62f);
    const auto text = style.label + " " + (release.isValid() ? release.version : String ("0.0.0"));
    return roundToInt (GlyphArrangement::getStringWidth (f, text)) + height;   // + rounded ends
}

void UpdatePill::paint (Graphics& g)
{
    if (! release.isValid()) return;
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool over = isMouseOver (true);
    g.setColour (over ? style.pillFill.brighter (0.15f) : style.pillFill);
    g.fillRoundedRectangle (r, jmin (style.corner, r.getHeight() * 0.5f));
    g.setColour (style.pillText);
    g.setFont (style.font.withHeight (jmin (style.font.getHeight(), r.getHeight() * 0.62f)));
    g.drawFittedText (style.label + " " + release.version, getLocalBounds().reduced (4, 0), Justification::centred, 1, 0.7f);
}

void UpdatePill::mouseUp (const MouseEvent& e)
{
    if (getLocalBounds().contains (e.getPosition())) showDetails();
}

void UpdatePill::showDetails()
{
    if (! release.isValid()) return;
    if (onClick) { onClick (release); return; }
    auto* host = getTopLevelComponent();
    if (host == nullptr || host == this) return;
    Component::SafePointer<UpdatePill> safe (this);
    details = createDetailsPanel (*host, release, current, style,
                                  [safe] { if (safe != nullptr) safe->details.reset(); },
                                  [safe] { if (safe != nullptr) safe->refresh(); });
}
}
