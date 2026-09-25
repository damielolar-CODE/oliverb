// =============================================================================
//  DeestechUpdate.h — update notifications for every Deestech desktop product.
//
//  CANONICAL COPY: DeesTapeDesktop/Source/Updates/DeestechUpdate.{h,cpp}.
//  Verbatim copies live in CLEAN Air, Neo TUBE, Deestronix (Plugins/Common) and OLIVERB. Edit here, then run
//  DeesTapeDesktop/scripts/sync_updater.sh so every product ships the same code.
//
//  How it works
//    * The site publishes ONE feed: https://deestechholdings.com/updates.json (generated at deploy time from the
//      downloads.json files — deestech-site/tools/build_updates_feed.py). Shape:
//          { "schema": 1, "generated": "...",
//            "products": { "<productId>": { "name", "page", "notes": [..], "date", "critical",
//                                           "mac": { "version", "url", "sha256", "size" },
//                                           "win": { ... } } } }
//      productIds: deestape-desktop, cleanair, neotube, oliverb, deestronix/<slug>.
//    * Service (one per process per binary, shared by every open editor through SharedResourcePointer) loads the
//      cached feed from disk and fetches a fresh copy at most once every 20 hours — on a background thread, never on
//      the audio thread, never while no editor is open. The cache and the user's choices are shared by EVERY Deestech
//      product on the computer: <app data>/Deestech/Updates/{feed.json,state.json}.
//    * Nothing is sent except a plain HTTPS GET of the feed (no ids, no version, no licence data).
//    * UpdatePill: a small button that is invisible until a newer version exists; clicking it opens a call-out with
//      the release notes, DOWNLOAD (opens the installer URL) and SKIP THIS VERSION.
//
//  Testing: set DEESTECH_UPDATE_FEED=http://127.0.0.1:<port>/updates.json in the environment of the process
//  (only https:// or a loopback http:// URL is accepted) to point it at a local feed, and DEESTECH_UPDATE_DIR=<abs dir>
//  to keep the cache/state away from the real one. Tests: DeesTapeDesktop/Tools/update_test.cpp.
// =============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace dtupdate
{
    inline constexpr const char* kFeedUrl   = "https://deestechholdings.com/updates.json";
    inline constexpr const char* kSiteRoot  = "https://deestechholdings.com";
    inline constexpr int         kSchema    = 1;
    inline constexpr juce::int64 kFreshMs   = 20LL * 60 * 60 * 1000;   // a cached feed younger than this is used as is
    inline constexpr juce::int64 kRetryMs   = 60LL * 60 * 1000;        // after a failed fetch, wait this long before retrying

    // ---- pure helpers ---------------------------------------------------------------------------------------------
    /** Numeric dotted-version comparison: "1.10.0" > "1.9.2", "v2.0" == "2.0.0". Returns -1, 0 or 1. */
    int compareVersions (const juce::String& a, const juce::String& b);

    /** "mac", "win" or "linux" — the key used inside each product entry of the feed. */
    juce::String platformKey();

    /** The feed URL in use (kFeedUrl unless DEESTECH_UPDATE_FEED holds an acceptable override). */
    juce::String feedUrl();

    struct Release
    {
        juce::String productId, name, version, url, page, sha256, date;
        juce::int64  size = 0;
        juce::StringArray notes;
        bool critical = false;

        bool isValid() const;       // has a version and an acceptable download URL
    };

    /** The release for productId on `platform` in a parsed feed; an invalid Release when absent or malformed. */
    Release findRelease (const juce::var& feed, const juce::String& productId, const juce::String& platform = platformKey());

    /** True when `feed` is a JSON object with the expected schema and a products object. */
    bool isValidFeed (const juce::var& feed);

    // ---- per-computer state (shared by every Deestech product) ------------------------------------------------------
    juce::File updatesFolder();                         // <app data>/Deestech/Updates (created on demand)
    bool automaticChecksEnabled();                      // default true
    void setAutomaticChecksEnabled (bool);
    void skipVersion (const juce::String& productId, const juce::String& version);
    bool isSkipped (const juce::String& productId, const juce::String& version);
    void snooze (const juce::String& productId, const juce::String& version, int days);
    bool isSnoozed (const juce::String& productId, const juce::String& version);

    // ---- the checker ------------------------------------------------------------------------------------------------
    class Service : private juce::AsyncUpdater
    {
    public:
        Service();
        ~Service() override;

        struct Listener
        {
            virtual ~Listener() = default;
            virtual void updateInfoChanged() = 0;       // message thread: a check finished (fresh data or an error)
        };
        void addListener (Listener*);
        void removeListener (Listener*);

        /** Loads the cached feed and, when it is stale (and automatic checks are on), fetches a fresh one.
            force = fetch now even if the cache is fresh or checks are off (a "CHECK FOR UPDATES" button). */
        void check (bool force = false);

        bool         isChecking() const;
        juce::String lastError() const;                 // empty after a successful fetch
        juce::int64  lastSuccessMs() const;             // when the feed in memory was fetched (0 = never)

        /** The newest release in the feed for productId (invalid when unknown). */
        Release latest (const juce::String& productId) const;

        /** A release newer than currentVersion, or an invalid Release. Skipped versions are filtered out unless
            includeSkipped is true; critical releases are never filtered. */
        Release availableUpdate (const juce::String& productId, const juce::String& currentVersion,
                                 bool includeSkipped = false) const;

    private:
        class Fetcher;
        void handleAsyncUpdate() override;
        void loadCache();
        void startFetch();
        void deliver (const juce::var& newFeed, const juce::String& err, juce::int64 when);   // Fetcher thread → message thread

        mutable juce::CriticalSection lock;
        juce::var feed;                                  // guarded by lock
        juce::int64 fetchedAt = 0;                       // guarded by lock
        juce::String error;                              // guarded by lock
        juce::var pendingFeed;                           // guarded by lock: handed over by the Fetcher
        juce::String pendingError;
        juce::int64 pendingAt = 0;
        bool hasPending = false;
        std::atomic<bool> checking { false };
        std::unique_ptr<Fetcher> fetcher;
        juce::ListenerList<Listener> listeners;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Service)
    };

    using SharedService = juce::SharedResourcePointer<Service>;

    // ---- UI -----------------------------------------------------------------------------------------------------------
    /** Colours and font for the pill and its call-out, so each product can match its own faceplate. */
    struct Style
    {
        juce::Colour pillFill   { 0xffE8702A };          // the pill: bright, it has to be noticed
        juce::Colour pillText   { 0xffFFFFFF };
        juce::Colour panelFill  { 0xff1C1A18 };          // the call-out
        juce::Colour panelText  { 0xffEDE6D8 };
        juce::Colour panelDim   { 0xff9A9186 };
        juce::Colour accent     { 0xffE8702A };          // DOWNLOAD button
        juce::Font   font       { juce::FontOptions (11.0f, juce::Font::bold) };
        float        corner     = 4.0f;
        juce::String label      = "UPDATE";              // pill text is "<label> <version>"
    };

    /** Invisible until an update exists for productId; click → an in-window panel with the notes, DOWNLOAD and
        SKIP THIS VERSION (drawn inside the plug-in window over a dimmed backdrop — not a CallOutBox, which JUCE
        dismisses whenever the process is not in the foreground, e.g. hosts that run plug-ins in a separate process).
        Place it once in resized(); it shows and hides itself. */
    class UpdatePill : public juce::Component,
                       public juce::SettableTooltipClient,
                       private Service::Listener
    {
    public:
        UpdatePill (juce::String productId, juce::String currentVersion, Style = {});
        ~UpdatePill() override;

        bool hasUpdate() const { return release.isValid(); }
        const Release& getRelease() const { return release; }
        int preferredWidth (int height) const;           // width that fits "<label> <version>" at this height

        /** Replaces the default click action (the call-out). DEES TAPE uses it to open its install window. */
        std::function<void (const Release&)> onClick;
        /** Called after the pill shows or hides, so an editor can re-lay-out around it if it wants to. */
        std::function<void()> onVisibilityChange;

        void refresh();                                  // re-reads the service (e.g. after SKIP)
        void showDetails();                              // what a click does: onClick, else the details panel
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override  { repaint(); }

    private:
        void updateInfoChanged() override { refresh(); }

        juce::String productId, current;
        Style style;
        Release release;
        SharedService service;
        std::unique_ptr<juce::Component> details;        // the open panel (a child of the top-level window)

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UpdatePill)
    };

    /** The details panel: dims `host` and shows "<Name> <version> is available", notes, DOWNLOAD / SKIP THIS VERSION.
        The caller owns the returned component (it is already a child of host). onClosed is called (asynchronously)
        when the user dismisses it — click outside, Esc, ×, DOWNLOAD or SKIP; the caller should then destroy it.
        onSkipped is called after SKIP THIS VERSION. */
    std::unique_ptr<juce::Component> createDetailsPanel (juce::Component& host, const Release&, const juce::String& currentVersion,
                                                         const Style&, std::function<void()> onClosed,
                                                         std::function<void()> onSkipped = {});
}
