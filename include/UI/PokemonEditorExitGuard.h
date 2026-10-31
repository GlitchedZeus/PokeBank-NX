#pragma once

namespace PokeBank::UIModel::PokemonEditorExitGuard {

// Shared Create/Edit exit policy for every Pokemon editor generation.
// Future generation adapters must route their top-level Back action through
// this contract instead of silently closing or discarding a local draft.
enum class SessionKind {
    View,
    Create,
    Edit,
};

// Dirty state is intentionally not part of the decision. Create and Edit
// always require an explicit user choice before leaving; View remains a
// normal non-destructive Back action.
constexpr bool requiresConfirmation(SessionKind kind, bool dirty = false) noexcept {
    (void)dirty;
    return kind == SessionKind::Create || kind == SessionKind::Edit;
}

constexpr const char* heading(SessionKind kind) noexcept {
    return kind == SessionKind::Create ? "Add this new Pokemon?"
         : kind == SessionKind::Edit ? "Keep this Pokemon edit?"
                                     : "Leave Pokemon view?";
}

constexpr const char* primaryAction(SessionKind kind) noexcept {
    return kind == SessionKind::Create ? "Add Staged"
         : kind == SessionKind::Edit ? "Keep Staged"
                                     : "Back";
}

constexpr bool futureGenerationsUseSharedExitGuard() noexcept { return true; }

} // namespace PokeBank::UIModel::PokemonEditorExitGuard
