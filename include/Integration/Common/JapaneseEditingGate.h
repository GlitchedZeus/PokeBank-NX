#pragma once
// The one switch that lets the app EDIT Japanese Gen I / II saves (owner rule, Round 8 / Oct 28):
// "Japanese save editing must remain read-only until the applicable native writers pass strict
// parser, checksum, byte-preservation, export and real-save validation. Do not lift the gate
// automatically."
//
// The writers and the kana text are built and tested (Makefile.host.jp-gen12, including opt-in real
// Japanese saves). Only the owner flips this to true. While it is false, Japanese saves open
// read-only exactly as before: no editor, no import, no Export Copy of an edited Japanese save.

namespace PokeVault::Integration::Common {

inline constexpr bool kJapaneseClassicEditingEnabled = false;

}  // namespace PokeVault::Integration::Common
