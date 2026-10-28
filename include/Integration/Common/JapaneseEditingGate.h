#pragma once
// Owner rule: Japanese Generation I / II saves stay read-only until their native writers are
// validated on real saves. Only the owner flips this. While it is false the Legacy bridges
// (RBYReadOnlyTrainer, GSCReadOnlyTrainer) create no staged editor for a Japanese save, so no
// Bag, Pokemon or Trainer entry point can stage a change and nothing can be exported.
namespace PokeVault::Integration::Common {

inline constexpr bool kJapaneseClassicEditingEnabled = false;

inline constexpr const char* kJapaneseGen1ReadOnlyReason = "Japanese Generation I saves are read-only";
inline constexpr const char* kJapaneseGen2ReadOnlyReason = "Japanese Generation II saves are read-only";

} // namespace PokeVault::Integration::Common
