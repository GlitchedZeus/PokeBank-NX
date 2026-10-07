#ifndef UTILS_PHYSICAL_MOVE_AUDIT_H
#define UTILS_PHYSICAL_MOVE_AUDIT_H

namespace PokeBank::Storage::PhysicalAudit {

/// Audit mode is opt-in through an exact marker file under PokeBank's owned audit directory.
/// Ordinary launches return false and follow the normal product UI.
bool shouldRun();

/// Runs the disposable Switch interruption/recovery harness. No installed-game, RetroArch,
/// emulator-source, or normal Bank path is opened by this harness.
int run();

} // namespace PokeBank::Storage::PhysicalAudit

#endif
