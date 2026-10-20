#pragma once

#include <array>
#include <string>

// NEW-19 / R2-TRAINER / R2-BUG-002: the Gen I-IV staged-session exit menu.
//
// Leaving a Gen I, II, III or IV session (B on the loaded-game hub, + anywhere, B out of the Items
// shortcut) while staged changes exist used to offer only Discard / Cancel. This is the Gen V
// ConfirmExit menu for the classic generations, plus the verified Export Copy every generation's
// Review already offers:
//   Y Discard & Exit, X Review, A Export Copy, B Continue.
// Staged changes include Trainer-only edits (Name / Money): every Gen I-IV adapter's
// hasStagedChanges() counts its Trainer editor, so a Trainer-only session reaches this menu too.
//
// Review opens that generation's OWN Review page (the same rows, Export and Discard All as from
// Box Actions); B there returns to this menu while changes remain. Export Copy calls that
// generation's existing verified export. Discard & Exit leaves exactly as the old Discard did:
// in-memory staged edits are dropped, the source save is never written.
namespace PokeBank::UIModel::ClassicStagedExit {

enum class Choice { None, Continue, Review, ExportCopy, DiscardAndExit };

struct Button {
    const char* glyph;
    const char* label;
    Choice choice;
};

// Gen V order (Y, X, B) with Export Copy before Continue. Every visible button is a real choice;
// a tap on a boxed row or footer glyph presses exactly that button.
inline constexpr std::array<Button, 4> kButtons{{
    {"Y", "Discard & Exit", Choice::DiscardAndExit},
    {"X", "Review", Choice::Review},
    {"A", "Export Copy", Choice::ExportCopy},
    {"B", "Continue", Choice::Continue},
}};

inline constexpr const char* kTitle = "Staged Changes Remain In Memory";
inline constexpr const char* kMessage =
    "Exit discards in-memory edits. Export Copy first to keep them.";
inline constexpr const char* kSafety = "The original save is never written.";

// One choice per frame. A chord never loses work: B (Continue) wins, then Review, then Export;
// Discard & Exit only when Y is the one button pressed.
constexpr Choice choiceFor(bool a, bool b, bool x, bool y) noexcept {
    if (b) return Choice::Continue;
    if (x) return Choice::Review;
    if (a) return Choice::ExportCopy;
    if (y) return Choice::DiscardAndExit;
    return Choice::None;
}

enum class Effect { None, Close, OpenReview, Export, Exit };

struct Session {
    const void* owner = nullptr;
    std::string sourceGameId;
    bool open = false;
    bool viaPlus = false;         // + asked to leave the app, not just the session
    bool itemsShortcut = false;   // the exit came from Product Home -> Items
    bool reviewFromExit = false;  // the generation's Review was opened from this menu
};

// One session per loaded screen and game, like the generation surfaces' own state.
inline Session& sessionFor(const void* owner, const std::string& sourceGameId) {
    static Session session;
    if (session.owner != owner || session.sourceGameId != sourceGameId) {
        session = {};
        session.owner = owner;
        session.sourceGameId = sourceGameId;
    }
    return session;
}

template <class Screen>
Session& sessionFor(const Screen& screen) {
    return sessionFor(static_cast<const void*>(&screen), screen.sourceGameId);
}

// Takes over the shell's dirty-exit prompt (saveConfirmActive + exitingWithUnsavedChanges) for a
// Gen I-IV source with staged changes. The shell flags are cleared so its two-button dialog is
// never drawn or handled. Returns true when the menu is (now) open.
inline bool adopt(Session& session, bool& saveConfirmActive, bool& exitingWithUnsavedChanges,
                  bool& exitingViaPlus, bool itemsShortcut, bool hasStagedChanges) noexcept {
    if (saveConfirmActive && exitingWithUnsavedChanges && hasStagedChanges) {
        session.open = true;
        session.viaPlus = exitingViaPlus;
        session.itemsShortcut = itemsShortcut;
        session.reviewFromExit = false;
        saveConfirmActive = false;
        exitingWithUnsavedChanges = false;
        exitingViaPlus = false;
    }
    return session.open;
}

// Applies a choice. Export keeps the menu open so the user can then leave safely or continue.
inline Effect choose(Session& session, Choice choice) noexcept {
    if (!session.open) return Effect::None;
    switch (choice) {
        case Choice::None: return Effect::None;
        case Choice::Continue: session.open = false; return Effect::Close;
        case Choice::Review:
            session.open = false;
            session.reviewFromExit = true;
            return Effect::OpenReview;
        case Choice::ExportCopy: return Effect::Export;
        case Choice::DiscardAndExit: session.open = false; return Effect::Exit;
    }
    return Effect::None;
}

// The generation could not open its Review: stay on the menu.
inline void reviewUnavailable(Session& session) noexcept {
    session.reviewFromExit = false;
    session.open = true;
}

// Called by a generation's Review when it closes (B, or after Discard All). Returns true when the
// Review was opened from this menu: the caller then closes its Review completely (no Box Actions
// sheet for a stale slot) and the menu reopens while staged changes remain.
inline bool returnFromReview(Session& session, bool hasStagedChanges) noexcept {
    if (!session.reviewFromExit) return false;
    session.reviewFromExit = false;
    session.open = hasStagedChanges;
    return true;
}

} // namespace PokeBank::UIModel::ClassicStagedExit
