// Save Instances for Modern-adapter titles (Gen VI first): discovery over the verified Checkpoint
// layout, strict validation, explicit per-profile assignment, fingerprint-checked reopen, and the
// read-only Home card snapshot. Everything runs against a temporary directory tree; no source file
// is ever written by the code under test.
#include "Integration/Modern/ModernSources.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"
#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"
#include "fixtures/gen8b/Gen8bSyntheticSave.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace Modern = PokeVault::Integration::Modern;
namespace Layout = PokeVault::Integration::Gen6::Layout;
using PokeVault::Legacy::LegacySourceBindings;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
std::string makeTempDir() {
    char pattern[] = "/tmp/modern-sources-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
bool writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    // ---- family dispatch ----
    check(Modern::isModernAssignedId("x_3ds") && Modern::isModernAssignedId("ultramoon_3ds"), "3DS titles are Modern-assigned");
    check(!Modern::isModernAssignedId("black_nds") && !Modern::isModernAssignedId("scarlet_switch") &&
              !Modern::isModernAssignedId("legends_arceus_switch"),
          "DS titles and Switch titles without a verified adapter are not");
    check(Modern::isModernAssignedId("sword_switch") && Modern::isModernAssignedId("shield_switch") &&
              Modern::gameGroupFor("shield_switch") == Enums::GameVersion::SWSH && Modern::expectedFamily("sword_switch") == "SWSH" &&
              Modern::platformLabelFor("sword_switch") == "Nintendo Switch" && Modern::generationLabelFor("sword_switch") == "Generation VIII" &&
              Modern::exportsRootFor("sword_switch").ends_with("/exports/gen8"),
          "Sword/Shield linked files are served by the Gen VIII adapter");
    check(Modern::sourceVariantFor("x_3ds") == Modern::kSourceVariant3ds &&
              Modern::sourceVariantFor("sword_switch") == Modern::kSourceVariantSwitchMain && Modern::sourceVariantFor("black_nds").empty(),
          "source variant per platform");
    check(Modern::gameGroupFor("x_3ds") == Enums::GameVersion::XY && Modern::gameGroupFor("alphasapphire_3ds") == Enums::GameVersion::ORAS &&
              Modern::gameGroupFor("moon_3ds") == Enums::GameVersion::SM && Modern::gameGroupFor("ultrasun_3ds") == Enums::GameVersion::USUM,
          "game groups");
    check(Modern::expectedFamily("omegaruby_3ds") == "ORAS" && Modern::platformLabelFor("y_3ds") == "Nintendo 3DS" &&
              Modern::generationLabelFor("sun_3ds") == "Generation VII",
          "family/platform/generation labels");
    check(Modern::exportsRootFor("x_3ds").ends_with("/exports/gen6") && Modern::exportsRootFor("sun_3ds").empty(),
          "export roots: Gen VI has one, SM/USUM has none (signing gate)");

    // ---- Checkpoint-layout tree with one X save and one ORAS save ----
    const std::string root = makeTempDir();
    check(!root.empty(), "temp dir");
    const std::string checkpoint = root + "/Checkpoint/saves";
    const std::string xDir = checkpoint + "/0x0055D Pokemon X/2026-10-31";
    const std::string orDir = checkpoint + "/0x011C4 Pokemon Omega Ruby/b1";
    ::mkdir((root + "/Checkpoint").c_str(), 0755);
    ::mkdir(checkpoint.c_str(), 0755);
    ::mkdir((checkpoint + "/0x0055D Pokemon X").c_str(), 0755);
    ::mkdir(xDir.c_str(), 0755);
    ::mkdir((checkpoint + "/0x011C4 Pokemon Omega Ruby").c_str(), 0755);
    ::mkdir(orDir.c_str(), 0755);
    const auto xSave = Gen6Fixture::makeSave({});
    const auto orSave = Gen6Fixture::makeSave({Layout::Family::ORAS, Layout::VersionOmegaRuby, 3, true});
    check(writeFile(xDir + "/main", xSave) && writeFile(orDir + "/main", orSave), "fixture files written");
    const auto pristine = slurp(xDir + "/main");

    Modern::DiscoveryRoots roots;
    roots.checkpointRoot = checkpoint;
    roots.dekoponPointerFile = root + "/no-such-pointer.txt";
    roots.dekoponDefaultUserDir = root + "/no-dekopon/";
    const auto found = Modern::discoverKnownSources("x_3ds", roots);
    check(found.rootsConsidered == 2 && found.rootsOpened == 1, "Checkpoint root opened, Dekopon root absent");
    check(found.instances.size() == 1 && found.filesExamined == 1, "exactly the X export is examined for X");
    const auto& x = found.instances.front();
    check(x.ready() && x.readOnly() && x.gameId == "x_3ds" && x.generation == 6, "X row validated read-only");
    check(x.providerLabel == "Checkpoint" && x.providerId == "checkpoint" && x.platformLabel == "Nintendo 3DS", "provider identity");
    check(x.kind == PokeVault::Source::SaveInstanceKind::Backup && x.containerType == "3ds-save-archive-main", "kind/container");
    check(!x.contentFingerprint.empty() && !x.sourceIdentity.empty() && !x.physicalIdentity.empty(), "fingerprint and identities");
    check(x.trainerName == "TESTER" && x.partyCount == 2, "trainer preview");
    const auto foundOr = Modern::discoverKnownSources("omegaruby_3ds", roots);
    check(foundOr.instances.size() == 1 && foundOr.instances.front().gameId == "omegaruby_3ds", "ORAS export found for Omega Ruby");
    check(Modern::discoverKnownSources("y_3ds", roots).instances.empty(), "no Y export: nothing invented");

    // Manual inspection: wrong assignment, dot segments, missing file.
    const auto wrong = Modern::inspectSourceFile(xDir + "/main", "Manual", "y_3ds");
    check(!wrong.ready() && wrong.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch, "X bytes refuse a Y assignment");
    const auto dotted = Modern::inspectSourceFile(checkpoint + "/../saves/0x0055D Pokemon X/2026-10-31/main", "Manual", "x_3ds");
    check(dotted.validation == PokeVault::Source::ValidationStatus::Unsupported, "dot segments refused");
    const auto missing = Modern::inspectSourceFile(root + "/nothing/main", "Manual", "x_3ds");
    check(missing.validation == PokeVault::Source::ValidationStatus::Missing, "missing file reported");
    check(Modern::inspectSourceChoices(xDir + "/main", "Manual", "x_3ds").size() == 1, "one copy per SAV6 (no Primary/Backup pair)");
    const auto unsupported = Modern::inspectSourceFile(xDir + "/main", "Manual", "black_nds");
    check(unsupported.validation == PokeVault::Source::ValidationStatus::Unsupported, "non-Modern id unsupported here");

    // ---- catalog / assignment ----
    LegacySourceBindings bindings(root + "/bindings.cfg");
    const std::string profileA = "profile-a", profileB = "profile-b";
    auto catalog = Modern::forGameAndProfile(found.instances, bindings, profileA, "x_3ds");
    check(catalog.validQuery && catalog.rows.size() == 1 && !catalog.rows.front().rememberedSource, "catalog lists the unassigned row");
    check(Modern::forGameAndProfile(found.instances, bindings, profileA, "y_3ds").rows.empty() &&
              Modern::forGameAndProfile(found.instances, bindings, profileA, "y_3ds").wrongGame == 1,
          "X row is wrongGame for Y");
    auto assigned = Modern::assignValidatedSource(bindings, catalog.rows.front(), profileA, "x_3ds");
    check(assigned.success, "assignment: " + assigned.diagnostic);
    check(assigned.instance.rememberedSource && assigned.instance.claimedProfile == profileA, "assigned instance remembered");
    const auto resolved = bindings.resolveFileForGame(profileA, "x_3ds");
    check(resolved.status == PokeVault::Legacy::AssignedFileStatus::Ready && resolved.binding.sourceType == "Checkpoint" &&
              resolved.binding.expectedRawFamily == "XY" && resolved.binding.contentFingerprint == x.contentFingerprint,
          "binding record persisted with provider, family and fingerprint");
    catalog = Modern::forGameAndProfile(found.instances, bindings, profileA, "x_3ds");
    check(catalog.rows.size() == 1 && catalog.rows.front().rememberedSource, "catalog marks the remembered row");
    const auto other = Modern::forGameAndProfile(found.instances, bindings, profileB, "x_3ds");
    check(other.rows.empty() && other.ownedByOtherProfile == 1, "another profile does not see the claimed save");
    check(!Modern::assignValidatedSource(bindings, catalog.rows.front(), profileB, "x_3ds").success, "cross-profile assignment refused");
    check(!Modern::assignValidatedSource(bindings, catalog.rows.front(), profileA, "y_3ds").success, "wrong-game assignment refused");

    // ---- open ----
    auto opened = Modern::openAssignedSource(bindings, profileA, "x_3ds");
    check(opened.ready(), "open assigned: " + opened.diagnostic);
    check(opened.ready() && opened.session->exactGameId() == "x_3ds" && opened.session->partyCount() == 2, "session is the X save");
    check(opened.instance.contentFingerprint == x.contentFingerprint, "open reports the same fingerprint");
    check(Modern::openAssignedSource(bindings, profileB, "x_3ds").status == Modern::AssignedOpenStatus::Unassigned, "unassigned for profile B");
    check(Modern::openAssignedSource(bindings, profileA, "x_3ds", "deadbeef").status == Modern::AssignedOpenStatus::InvalidSave,
          "stale selection fingerprint refused");
    const auto snapshot = Modern::homeCardSnapshot(opened);
    check(snapshot.current() && snapshot.trainerName == "TESTER" && snapshot.partyCount == 2 && snapshot.party[0].species == 1 &&
              snapshot.party[1].species == 2 && snapshot.dexTotal == 721 && snapshot.trainerGenderKnown && snapshot.trainerGender == 1,
          "home card snapshot from the strict session");
    check(!Modern::homeCardSnapshot(Modern::openAssignedSource(bindings, profileB, "x_3ds")).hasData(), "no snapshot without assignment");

    // ---- the source changes under the assignment ----
    auto edited = xSave;
    edited[Layout::MyStatusOffset + 0x48] = 'X';  // trainer name byte; CRC no longer matches
    check(writeFile(xDir + "/main", edited), "rewrite fixture");
    auto reopened = Modern::openAssignedSource(bindings, profileA, "x_3ds");
    check(!reopened.ready() && reopened.status == Modern::AssignedOpenStatus::InvalidSave, "corrupted block CRC refuses open: " + reopened.diagnostic);
    check(writeFile(xDir + "/main", orSave), "replace with an ORAS save at the same path");
    reopened = Modern::openAssignedSource(bindings, profileA, "x_3ds");
    check(!reopened.ready() && reopened.status == Modern::AssignedOpenStatus::AssignmentMismatch, "a different title at the path is a mismatch, never adopted");
    check(writeFile(xDir + "/main", xSave), "restore fixture");
    check(Modern::openAssignedSource(bindings, profileA, "x_3ds").ready(), "restored bytes open again");
    check(slurp(xDir + "/main") == pristine, "the code under test never wrote the source");

    // Forget and reassign through the shared bindings.
    check(bindings.unassignGameAndSave(profileA, "x_3ds"), "forget");
    check(Modern::openAssignedSource(bindings, profileA, "x_3ds").status == Modern::AssignedOpenStatus::Unassigned, "forgotten");

    // ---- Gen VII through the same layer: Sun export in the Checkpoint layout (0x16480) ----
    {
        namespace L7 = PokeVault::Integration::Gen7::Layout;
        const std::string sunDir = checkpoint + "/0x01648 Pokemon Sun/2026-10-31";
        ::mkdir((checkpoint + "/0x01648 Pokemon Sun").c_str(), 0755);
        ::mkdir(sunDir.c_str(), 0755);
        const auto sunSave = Gen7Fixture::makeSave({});
        check(writeFile(sunDir + "/main", sunSave), "Sun fixture written");
        const auto sun = Modern::discoverKnownSources("sun_3ds", roots);
        check(sun.instances.size() == 1 && sun.instances.front().ready() && sun.instances.front().gameId == "sun_3ds" &&
                  sun.instances.front().generation == 7,
              "Sun export discovered and validated (signature verified by the lane parser)");
        check(Modern::discoverKnownSources("moon_3ds", roots).instances.empty(), "no Moon export invented from the Sun folder");
        check(Modern::discoverKnownSources("x_3ds", roots).instances.size() == 1, "Gen VI discovery ignores the Gen VII folder");
        const auto wrongSun = Modern::inspectSourceFile(sunDir + "/main", "Manual", "ultrasun_3ds");
        check(!wrongSun.ready() && wrongSun.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch,
              "Sun bytes refuse an Ultra Sun assignment");
        auto catalog7 = Modern::forGameAndProfile(sun.instances, bindings, profileA, "sun_3ds");
        check(catalog7.rows.size() == 1, "Sun row catalogued");
        const auto assigned7 = Modern::assignValidatedSource(bindings, catalog7.rows.front(), profileA, "sun_3ds");
        check(assigned7.success, "Sun assignment: " + assigned7.diagnostic);
        check(bindings.resolveFileForGame(profileA, "sun_3ds").binding.expectedRawFamily == "SM", "SM family persisted");
        auto opened7 = Modern::openAssignedSource(bindings, profileA, "sun_3ds");
        check(opened7.ready() && opened7.session->exactGameId() == "sun_3ds" && opened7.session->boxCount() == 32,
              "Sun opens through the Gen VII adapter: " + opened7.diagnostic);
        check(opened7.ready() && !opened7.session->exportSupport().available &&
                  std::string(opened7.session->exportSupport().reason).find("SAV7") != std::string::npos,
              "Gen VII export stays blocked with the SAV7 signing reason");
        const auto snap7 = Modern::homeCardSnapshot(opened7);
        check(snap7.current() && snap7.partyCount == 2 && snap7.dexTotal > 0, "Sun home card snapshot");
        check(slurp(sunDir + "/main") == sunSave, "Sun fixture never written");
    }

    // ---- Gen VIII Sword/Shield: manual link only (no Switch save-manager layout is invented) ----
    {
        const std::string swDir = root + "/linked-sword";
        ::mkdir(swDir.c_str(), 0755);
        const auto swordSave = Gen8Fixture::makeSave();
        check(writeFile(swDir + "/main", swordSave), "Sword fixture written");
        check(Modern::discoverKnownSources("sword_switch", roots).instances.empty(), "no Switch discovery layout is assumed");
        const auto row = Modern::inspectSourceFile(swDir + "/main", "Manual", "sword_switch");
        check(row.ready() && row.generation == 8 && row.containerType == "switch-swish-main" &&
                  row.sourceVariant == Modern::kSourceVariantSwitchMain && row.partyCount == 2 && row.trainerName == "Hop" &&
                  row.readOnly() && !row.contentFingerprint.empty(),
              "linked Sword main validates strictly: " + row.diagnostic);
        const auto wrong = Modern::inspectSourceFile(swDir + "/main", "Manual", "shield_switch");
        check(!wrong.ready() && wrong.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch,
              "Sword bytes refuse a Shield assignment");
        std::vector<uint8_t> junk(swordSave.size(), 0x33);
        check(writeFile(swDir + "/junk", junk), "junk written");
        const auto bad = Modern::inspectSourceFile(swDir + "/junk", "Manual", "sword_switch");
        check(!bad.ready() && bad.validation == PokeVault::Source::ValidationStatus::Invalid && !bad.diagnostic.empty(),
              "a corrupt container is Invalid with a reason");
        auto catalog8 = Modern::forGameAndProfile(std::vector<PokeVault::Source::SaveInstance>{row}, bindings, profileA, "sword_switch");
        check(catalog8.rows.size() == 1, "Sword row catalogued");
        const auto assigned8 = Modern::assignValidatedSource(bindings, catalog8.rows.front(), profileA, "sword_switch");
        check(assigned8.success, "Sword assignment: " + assigned8.diagnostic);
        const auto resolved = bindings.resolveFileForGame(profileA, "sword_switch");
        check(resolved.binding.expectedRawFamily == "SWSH" && resolved.binding.sourceVariant == Modern::kSourceVariantSwitchMain,
              "SWSH family and Switch variant persisted");
        auto opened8 = Modern::openAssignedSource(bindings, profileA, "sword_switch");
        check(opened8.ready() && opened8.session->exactGameId() == "sword_switch" && opened8.session->boxCount() == 32 &&
                  opened8.session->familyLabel() == "Generation VIII",
              "Sword opens through the Gen VIII adapter: " + opened8.diagnostic);
        check(opened8.ready() && !opened8.session->exportSupport().available, "nothing to export before a staged change");
        const auto snap8 = Modern::homeCardSnapshot(opened8);
        check(snap8.current() && snap8.partyCount == 2, "Sword home card snapshot");
        check(Modern::openAssignedSource(bindings, profileA, "sword_switch", "deadbeef").status == Modern::AssignedOpenStatus::InvalidSave,
              "a stale fingerprint is refused");
        check(Modern::openAssignedSource(bindings, profileB, "sword_switch").status == Modern::AssignedOpenStatus::Unassigned,
              "profile B has no Sword assignment");
        {
            // The binding store refuses a Switch-variant record that names the wrong family or a 3DS title.
            PokeVault::Legacy::BindingRecord forged = resolved.binding;
            forged.expectedRawFamily = "XY";
            check(!bindings.replaceFileAssignmentAndSave(resolved.sourceIdentity, forged), "Switch variant with a 3DS family refused");
            forged = resolved.binding;
            forged.gameIdentity = "x_3ds";
            forged.expectedRawFamily = "SWSH";
            check(!bindings.replaceFileAssignmentAndSave(resolved.sourceIdentity, forged), "Switch variant on a 3DS title refused");
            check(Modern::openAssignedSource(bindings, profileA, "sword_switch").ready(), "the valid Sword assignment survives the refusals");
        }
        check(slurp(swDir + "/main") == swordSave, "Sword fixture never written");
    }

    // ---- Let's Go: manual link, same contract ----
    {
        const std::string lgDir = root + "/linked-letsgo";
        ::mkdir(lgDir.c_str(), 0755);
        const auto lg = Gen7bFixture::makeSave();
        check(writeFile(lgDir + "/main", lg), "Let's Go fixture written");
        check(Modern::isModernAssignedId("letsgo_pikachu_switch") && Modern::gameGroupFor("letsgo_eevee_switch") == Enums::GameVersion::GG &&
                  Modern::expectedFamily("letsgo_pikachu_switch") == "LGPE" &&
                  Modern::sourceVariantFor("letsgo_pikachu_switch") == Modern::kSourceVariantSwitchMain &&
                  Modern::exportsRootFor("letsgo_pikachu_switch").ends_with("/exports/lgpe"),
              "Let's Go dispatch");
        const auto row = Modern::inspectSourceFile(lgDir + "/main", "Manual", "letsgo_pikachu_switch");
        check(row.ready() && row.generation == 7 && row.partyCount == 2 && row.trainerName == "Red" &&
                  row.sourceVariant == Modern::kSourceVariantSwitchMain,
              "linked Let's Go main validates strictly: " + row.diagnostic);
        const auto wrong = Modern::inspectSourceFile(lgDir + "/main", "Manual", "letsgo_eevee_switch");
        check(!wrong.ready() && wrong.validation == PokeVault::Source::ValidationStatus::AssignmentMismatch, "Pikachu bytes refuse Eevee");
        auto catalog = Modern::forGameAndProfile(std::vector<PokeVault::Source::SaveInstance>{row}, bindings, profileA,
                                                 "letsgo_pikachu_switch");
        const auto assigned = catalog.rows.size() == 1
                                  ? Modern::assignValidatedSource(bindings, catalog.rows.front(), profileA, "letsgo_pikachu_switch")
                                  : Modern::SourceAssignmentResult{};
        check(assigned.success, "Let's Go assignment: " + assigned.diagnostic);
        check(bindings.resolveFileForGame(profileA, "letsgo_pikachu_switch").binding.expectedRawFamily == "LGPE", "LGPE family persisted");
        auto opened = Modern::openAssignedSource(bindings, profileA, "letsgo_pikachu_switch");
        check(opened.ready() && opened.session->boxCount() == 40 && opened.session->slotsPerBox() == 25,
              "Let's Go opens through the Gen7b adapter: " + opened.diagnostic);
        check(Modern::homeCardSnapshot(opened).partyCount == 2, "Let's Go home card snapshot");
        check(slurp(lgDir + "/main") == lg, "Let's Go fixture never written");
    }

    // ---- BD/SP: manual link of SaveData.bin, same contract ----
    {
        const std::string bdDir = root + "/linked-bdsp";
        ::mkdir(bdDir.c_str(), 0755);
        const auto bd = Gen8bFixture::makeSave();
        check(writeFile(bdDir + "/SaveData.bin", bd), "BD/SP fixture written");
        check(Modern::gameGroupFor("shining_pearl_switch") == Enums::GameVersion::BDSP && Modern::expectedFamily("brilliant_diamond_switch") == "BDSP" &&
                  Modern::exportsRootFor("brilliant_diamond_switch").ends_with("/exports/bdsp"),
              "BD/SP dispatch");
        const auto row = Modern::inspectSourceFile(bdDir + "/SaveData.bin", "Manual", "brilliant_diamond_switch");
        check(row.ready() && row.generation == 8 && row.partyCount == 2 && row.trainerName == "Lucas",
              "linked BD/SP save validates strictly: " + row.diagnostic);
        check(Modern::inspectSourceFile(bdDir + "/SaveData.bin", "Manual", "shining_pearl_switch").validation ==
                  PokeVault::Source::ValidationStatus::AssignmentMismatch,
              "BD bytes refuse SP");
        auto catalog = Modern::forGameAndProfile(std::vector<PokeVault::Source::SaveInstance>{row}, bindings, profileA, "brilliant_diamond_switch");
        const auto assigned = catalog.rows.size() == 1
                                  ? Modern::assignValidatedSource(bindings, catalog.rows.front(), profileA, "brilliant_diamond_switch")
                                  : Modern::SourceAssignmentResult{};
        check(assigned.success && bindings.resolveFileForGame(profileA, "brilliant_diamond_switch").binding.expectedRawFamily == "BDSP",
              "BD/SP assignment: " + assigned.diagnostic);
        auto opened = Modern::openAssignedSource(bindings, profileA, "brilliant_diamond_switch");
        check(opened.ready() && opened.session->boxCount() == 40 && Modern::homeCardSnapshot(opened).partyCount == 2,
              "BD/SP opens through the Gen8b adapter: " + opened.diagnostic);
        check(slurp(bdDir + "/SaveData.bin") == bd, "BD/SP fixture never written");
    }

    std::cout << "test_modern_sources: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
