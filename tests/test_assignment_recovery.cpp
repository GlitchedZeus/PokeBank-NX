// Round 16 P0 regression: the Round 15 hardware failure ("Couldn't remember that exact save choice"
// / "The save assignment could not be stored safely"). Assignment became mandatory to open Gen I-IV,
// and the binding database refused EVERY write once its committed primary was unreadable (for
// example torn by a crash or power loss), even with a valid .bak. These scenarios run against the
// production storage path (sdmc:/switch/PokeBank-NX/config/...) resolved inside a private working
// directory, with Switch fsdev rename semantics (rename never replaces an existing target) and the
// session recovery the app enables in UIManager. Strict mode (tests' default) is covered by
// test_legacy_source_bindings and stays unchanged.
#include "Legacy/LegacySourceBindings.h"
#include "Utils/PokeBankPaths.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;
using PokeVault::Legacy::BindingRecord;
using PokeVault::Legacy::BindingResult;
using PokeVault::Legacy::LegacySourceBindings;
using PokeVault::Source::SaveInstance;
using PokeVault::Source::ValidationStatus;

namespace {
int checks = 0, failures = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << "\n"; }
}
std::string faultStage;
int switchRename(const char* from, const char* to) {
    // libnx fsdev: FsFileSystem::RenameFile fails when the destination exists.
    if (fs::exists(to)) { errno = EEXIST; return -1; }
    return std::rename(from, to);
}
int fault(const char* stage) {
    if (faultStage == stage) { errno = EIO; return -1; }
    return 0;
}
const PokeVault::Legacy::BindingFileOps kSwitchOps{switchRename, fault};

std::string contents(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void write(const std::string& path, const std::string& bytes) {
    std::ofstream file(path, std::ios::binary);
    file << bytes;
}
std::string hex(const std::string& value) {
    static const char* d = "0123456789abcdef";
    std::string out;
    for (unsigned char c : value) { out += d[c >> 4]; out += d[c & 15]; }
    return out;
}
std::string row(std::initializer_list<std::string> fields) {
    std::string out;
    for (const auto& f : fields) { if (!out.empty()) out += '\t'; out += hex(f); }
    return out + "\n";
}
const std::string kHeader = "# PokeBank NX legacy source bindings v1\n";
const std::string kA = "0123456789abcdef0000000000000001";  // profile identities as the app writes them
const std::string kB = "0123456789abcdef0000000000000002";
const std::string kYellowPath =
    "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Yellow Version (USA, Europe) (GBC Edition).srm";

SaveInstance instance(const std::string& identity, const std::string& game, const std::string& path) {
    SaveInstance s;
    s.sourceIdentity = identity;
    s.gameId = game;
    s.location = path;
    s.validation = ValidationStatus::Ready;
    return s;
}
LegacySourceBindings production() {
    LegacySourceBindings b(PokeBank::Paths::legacySourceBindingsFile(), kSwitchOps);
    b.enableSessionRecovery();
    return b;
}
std::string db() { return PokeBank::Paths::legacySourceBindingsFile(); }
}

int main() {
    char pattern[] = "/tmp/pokebank-assign-recovery-XXXXXX";
    if (!::mkdtemp(pattern)) return 1;
    const fs::path root(pattern), home = fs::current_path();
    fs::current_path(root);
    fs::create_directories("sdmc:/retroarch/cores/savefiles/Gambatte");
    const std::string yellowBytes(32768, '\x5a');
    write(kYellowPath, yellowBytes);
    const std::string gen4Path = "sdmc:/switch/drastic/backup/platinum.dsv";
    fs::create_directories("sdmc:/switch/drastic/backup");
    write(gen4Path, std::string(524288 + 122, '\x11'));
    const std::string gen4Bytes = contents(gen4Path);
    const auto yellow = instance("ra:gambatte/yellow", "yellow_gbc", kYellowPath);

    // 1. First run: no /switch/PokeBank-NX at all.
    {
        auto b = production();
        check(b.load() && b.size() == 0, "1 first run: empty load succeeds");
        check(b.preferGameSourceAndSave(yellow, kA, "yellow_gbc"), "1 first run: discovered .srm assigned");
        check(b.lastResult() == BindingResult::Succeeded && !b.hasUnsavedSessionChanges(),
              "1 first run: stored (not session-only)");
        check(fs::is_regular_file(db()), "1 first run: database created under sdmc:/switch/PokeBank-NX/config");
    }
    // 2 + 8. Existing valid DB; restart retains.
    {
        auto b = production();
        check(b.load(), "2 existing DB loads");
        check(b.isPreferredGameSource(yellow, kA, "yellow_gbc"), "8 restart retains the Yellow assignment");
    }
    // 3. Older formats written by earlier rounds: 2-column claim, 3-column game, 6-column Gen IV file,
    //    8-column Gen V copy without fingerprint. Every row survives a new classic write.
    {
        const std::string legacy = kHeader +
            row({"ra:old-claim", kA}) +
            row({"ra:gambatte/yellow", kA, "yellow_gbc"}) +
            row({"gen4:platinum", kA, "platinum_nds", gen4Path, "DraStic", "pt"}) +
            row({"gen5:black2", kA, "black2_nds", "sdmc:/switch/drastic/backup/b2.dsv", "DraStic", "B2W2",
                 "gen5-copy-0-primary", "path:b2"});
        write(db(), legacy);
        fs::remove(db() + ".bak");
        auto b = production();
        check(b.load() && b.size() == 4, "3 older-format rows all load");
        const auto red = instance("ra:gambatte/red", "red_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/red.srm");
        check(b.preferGameSourceAndSave(red, kA, "red_gb") && b.lastResult() == BindingResult::Succeeded,
              "3 a new classic assignment is stored over an older-format database");
        LegacySourceBindings reread(db());
        check(reread.load() && reread.size() == 5, "3 older rows + new row persist");
        check(reread.resolveFileForGame(kA, "platinum_nds").binding.sourcePath == gen4Path,
              "12 Gen IV assignment retained");
        check(reread.resolveFileForGame(kA, "black2_nds").binding.sourceVariant == "gen5-copy-0-primary",
              "12 Gen V assignment retained");
        check(contents(db() + ".bak") == legacy, "3 previous generation kept byte-exact at .bak");
    }
    // 4. Corrupt committed primary (torn by crash) + valid .bak: quarantined byte-for-byte, writing resumes.
    {
        const std::string good = contents(db());
        write(db() + ".bak", good);
        const std::string torn = good.substr(0, good.size() / 2);  // a torn half row
        write(db(), torn);
        auto b = production();
        check(b.load(), "4 load recovers from .bak");
        check(b.preferGameSourceAndSave(yellow, kA, "yellow_gbc"), "4 Yellow assignable after corruption");
        check(b.lastResult() == BindingResult::Succeeded, "4 stored, not session-only");
        check(!b.quarantinedPath().empty() && contents(b.quarantinedPath()) == torn,
              "4 damaged primary preserved byte-for-byte at " + b.quarantinedPath());
        LegacySourceBindings reread(db());
        check(reread.load() && reread.size() == 5, "4 nothing reset: every prior row still present");
        // A second corruption picks the next free quarantine name; the first is never overwritten.
        write(db() + ".bak", contents(db()));
        write(db(), "garbage");
        auto c = production();
        check(c.load() && c.preferGameSourceAndSave(yellow, kA, "yellow_gbc"), "4 second corruption recovers too");
        check(contents(db() + ".corrupt-1") == torn && contents(db() + ".corrupt-2") == "garbage",
              "4 earlier quarantine untouched; new one separate");
    }
    // 4b. Corrupt primary and NO usable backup: never reset to empty, file untouched, game still
    //     opens for the session, and other profiles' legible claims stay enforced.
    {
        const std::string damaged = kHeader + row({"ra:gambatte/yellow", kB, "yellow_gbc"}) + "zz-not-hex\n";
        write(db(), damaged);
        fs::remove(db() + ".bak");
        auto b = production();
        check(!b.load(), "4b damaged database without backup reports failure");
        check(b.lastResult() == BindingResult::ExistingMetadataCorrupt, "4b classified as existing metadata corrupt");
        check(!b.preferGameSourceAndSave(yellow, kA, "yellow_gbc") &&
              b.lastResult() == BindingResult::BelongsToAnotherProfile,
              "4b Profile B's legible claim still refuses Profile A");
        const auto blue = instance("ra:gambatte/blue", "blue_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/blue.srm");
        check(b.preferGameSourceAndSave(blue, kA, "blue_gb") && b.lastResult() == BindingResult::KeptForSession,
              "4b unclaimed save opens for this session");
        check(b.isPreferredGameSource(blue, kA, "blue_gb"), "4b all routes see the session choice");
        check(contents(db()) == damaged && !fs::exists(db() + ".bak"), "4b damaged file never rewritten or reset");
        check(b.lastError().find("read-existing") != std::string::npos, "4b diagnostic names the failing step");
        fs::rename(db(), "damaged-keep.cfg");
    }
    // 5. Failed write then retry: the on-disk generation is kept; the session keeps the choice;
    //    the retry persists it.
    {
        fs::remove(db() + ".bak");
        auto b = production();
        check(b.load() && b.size() == 0, "5 fresh database");
        check(b.preferGameSourceAndSave(yellow, kA, "yellow_gbc"), "5 baseline stored");
        const std::string committed = contents(db());
        const auto red = instance("ra:gambatte/red", "red_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/red.srm");
        for (const char* stage : {"write-temp", "validate-temp", "old-to-backup", "temp-to-target", "validate-target"}) {
            faultStage = stage;
            check(b.preferGameSourceAndSave(red, kA, "red_gb") && b.lastResult() == BindingResult::KeptForSession,
                  std::string("5 ") + stage + ": Red still opens for this session");
            check(b.lastError().find(stage) != std::string::npos, std::string("5 ") + stage + ": diagnostic");
            faultStage.clear();
            LegacySourceBindings disk(db());
            check(disk.load() && !disk.isPreferredGameSource(red, kA, "red_gb") &&
                  disk.isPreferredGameSource(yellow, kA, "yellow_gbc"),
                  std::string("5 ") + stage + ": committed generation unchanged on disk");
            (void)committed;
        }
        const auto crystal = instance("ra:gambatte/crystal", "crystal_gbc", "sdmc:/retroarch/cores/savefiles/Gambatte/c.srm");
        check(b.preferGameSourceAndSave(crystal, kA, "crystal_gbc") && b.lastResult() == BindingResult::Succeeded &&
              !b.hasUnsavedSessionChanges(), "5 retry succeeds");
        LegacySourceBindings disk(db());
        check(disk.load() && disk.isPreferredGameSource(red, kA, "red_gb") &&
              disk.isPreferredGameSource(crystal, kA, "crystal_gbc"), "5 retry persisted the session choice too");
    }
    // 6 + 7. Discovered .srm, then the SAME file chosen with the SD picker, then the discovered entry again:
    //        one physical save, same profile and game, never a conflict.
    {
        auto b = production();
        check(b.load(), "7 load");
        BindingRecord manual{kA, "yellow_gbc", kYellowPath, "Manual", "GBC"};
        check(b.assignClassicFileAndSave("ra:gambatte/yellow", manual), "7 manual .srm assigned");
        check(b.preferGameSourceAndSave(yellow, kA, "yellow_gbc"),
              "6 picking the discovered entry of the manually assigned save is accepted");
        check(b.isPreferredGameSource(yellow, kA, "yellow_gbc"), "6 it is the remembered Yellow save");
        // 11. Wrong game: a Yellow file is never accepted for Red, in strict or session mode.
        auto asRed = yellow; asRed.gameId = "red_gb";
        check(!b.preferGameSourceAndSave(asRed, kA, "red_gb") &&
              b.lastResult() == BindingResult::BelongsToAnotherProfile, "11 wrong game refused");
        check(!b.preferGameSourceAndSave(yellow, kA, "red_gb"), "11 game id mismatch refused");
        // 9. Profile B cannot take Profile A's save (never kept for session either).
        check(!b.preferGameSourceAndSave(yellow, kB, "yellow_gbc") && !b.hasUnsavedSessionChanges(),
              "9 Profile B refused");
        BindingRecord manualB{kB, "yellow_gbc", kYellowPath, "Manual", "GBC"};
        check(!b.assignClassicFileAndSave("ra:gambatte/yellow", manualB), "9 Profile B manual refused");
        // 10. Forget then reassign.
        check(b.forgetGameAndSave(kA, "yellow_gbc", {"ra:gambatte/yellow"}), "10 forget");
        check(!b.hasGameRecord(kA, "yellow_gbc"), "10 nothing remembered");
        check(b.preferGameSourceAndSave(yellow, kB, "yellow_gbc"), "10 after Forget Profile B may assign it");
        check(b.forgetGameAndSave(kB, "yellow_gbc", {"ra:gambatte/yellow"}) &&
              b.preferGameSourceAndSave(yellow, kA, "yellow_gbc"), "10 forget and reassign to A");
        LegacySourceBindings disk(db());
        check(disk.load() && disk.isPreferredGameSource(yellow, kA, "yellow_gbc") &&
              !disk.isPreferredGameSource(yellow, kB, "yellow_gbc"), "8/10 restart sees the final assignment");
    }
    // 1b. config/ missing while the app root exists (the 41f19ec scenario) and an absent card root.
    {
        fs::remove_all("sdmc:/switch/PokeBank-NX/config");
        auto b = production();
        check(b.load() && b.preferGameSourceAndSave(yellow, kA, "yellow_gbc") &&
              b.lastResult() == BindingResult::Succeeded, "1b config/ recreated on first assignment");
        const std::string diag = b.storageDiagnostics();
        check(diag.find("probe=ok") != std::string::npos, "diagnostics probe ok: " + diag);
        check(!fs::exists(PokeBank::Paths::configRoot() + "/.write-probe"), "probe leaves no file behind");
    }
    // 14. Source saves are never written.
    check(contents(kYellowPath) == yellowBytes, "14 Yellow .srm byte-identical");
    check(contents(gen4Path) == gen4Bytes, "14 Gen IV save byte-identical");

    fs::current_path(home);
    std::error_code ignored;
    fs::remove_all(root, ignored);
    if (failures) { std::cerr << failures << " of " << checks << " check(s) failed\n"; return 1; }
    std::cout << "Round 16 assignment recovery (fresh SD, older formats, torn primary, failed writes, "
                 ".srm discovered/manual, profiles, Forget, Gen IV/V retention; " << checks << " checks): PASS\n";
    return 0;
}
