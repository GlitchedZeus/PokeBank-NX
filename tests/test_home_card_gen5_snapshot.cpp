// Gen V Home card: the current validated save's trainer, portrait gender,
// party (species, level, shiny), Pokedex and real provider are available on
// the card WITHOUT opening the editor, refresh when the emulator rewrites the
// save, are not reparsed while unchanged, and turn explicitly stale/missing.
// Synthetic SAV5 only: offsets mirror tests/test_gen5_readonly_save.cpp,
// which cites the layout constants in Gen5SaveBlockLayout.h (PKHeX SAV5).
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5GameCardPreview.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Pokemon/Pokemon5ReadOnly.h"
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveProviderIdentity.h"
#include "Utils/CRC16.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace S = PokeVault::Source;

namespace {

void write32(std::vector<uint8_t>& dst, size_t off, uint32_t value) {
    C::write16(dst, off, static_cast<uint16_t>(value));
    C::write16(dst, off + 2, static_cast<uint16_t>(value >> 16));
}

void stampBlocks(std::vector<uint8_t>& sav) {
    auto partition = std::span<uint8_t>(sav).subspan(0, L::BlackWhiteCopySize);
    for (const auto& b : L::BlackWhite) {
        const uint16_t crc = Utils::crc16ccitt(partition.data() + b.offset, b.length);
        C::write16(partition, b.checksumOffset, crc);
        C::write16(partition, b.mirrorOffset, crc);
    }
}

struct PartyMember { uint16_t species; uint8_t level; bool shiny; uint32_t pid; uint8_t form = 0;
                     uint8_t gender = 0; };

// Pokemon Black (version 21) with an explicit party.
std::vector<uint8_t> makeBlack(const std::vector<PartyMember>& party, bool newGame = false) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t tr = 0x19400;
    if (!newGame) {
        sav[tr + 4] = 'N';
        sav[tr + 6] = 'X';
        C::write16(sav, tr + 0x14, 12345);
        C::write16(sav, tr + 0x16, 54321);
        sav[tr + 0x21] = 1; // female
        C::write16(sav, tr + 0x24, 23);
        sav[tr + 0x26] = 11;
    }
    sav[tr + 0x1E] = 2;
    sav[tr + 0x1F] = 21;
    sav[L::PartyOffset + 4] = static_cast<uint8_t>(party.size());
    for (size_t i = 0; i < party.size(); ++i) {
        std::vector<uint8_t> pk(C::PartySize, 0);
        // A shiny PID for TID 12345 / SID 54321: (tid ^ sid ^ hi ^ lo) < 8.
        write32(pk, 0, party[i].pid);
        C::write16(pk, 8, party[i].species);
        C::write16(pk, 12, 12345);
        C::write16(pk, 14, 54321);
        pk[0x40] = static_cast<uint8_t>((party[i].form << 3) | (party[i].gender << 1));
        pk[0x8C] = party[i].level;
        const auto encrypted = C::encryptCandidate(pk);
        std::copy(encrypted.begin(), encrypted.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8 + i * C::PartySize));
    }
    const size_t dex = 0x21600;
    sav[dex + 8 + 3] |= 1u; // species 25 seen
    sav[dex + 8 + 0x54 + 3] |= 1u;
    stampBlocks(sav);
    return sav;
}

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes, long mtime) {
    FILE* f = std::fopen(path.c_str(), "wb");
    assert(f);
    assert(std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size());
    assert(std::fclose(f) == 0);
    const struct timeval times[2] = {{mtime, 0}, {mtime, 0}};
    assert(::utimes(path.c_str(), times) == 0);
}

uint32_t shinyPid() {
    // hi ^ lo ^ 12345 ^ 54321 == 0  ->  shiny under the Gen V rule.
    const uint16_t lo = 0x1111;
    const uint16_t hi = static_cast<uint16_t>(lo ^ 12345 ^ 54321);
    return (static_cast<uint32_t>(hi) << 16) | lo;
}

struct Fixture {
    std::string dir;
    std::string save;
    PokeVault::Legacy::LegacySourceBindings bindings;
    S::HomeSnapshotCache cache;
    int parses = 0;

    explicit Fixture(std::string root)
        : dir(std::move(root)), save(dir + "/Pokemon Black.sav"), bindings(dir + "/bindings") {}

    // Same request/parse wiring as SaveSelectScreen::refreshAssignedSnapshot.
    const S::HomeCardSnapshot* refresh(S::SnapshotRefresh& outcome) {
        const auto assigned = bindings.resolveFileForGame("profile", "black_nds");
        if (assigned.status != PokeVault::Legacy::AssignedFileStatus::Ready) return nullptr;
        return &cache.refresh(S::assignedSnapshotRequest("profile", "black_nds", assigned),
            S::statRegularFile, [&] {
                ++parses;
                return G::homeCardSnapshot(G::openAssignedSource(bindings, "profile", "black_nds"));
            }, &outcome);
    }
};

void currentSaveOnHomeWithoutEditor(const std::string& root) {
    for (std::string provider : {"DraStic", "melonDS", "RetroArch", "Manual"}) {
        Fixture f(root + "/" + provider);
        assert(::mkdir(f.dir.c_str(), 0700) == 0);
        writeFile(f.save, makeBlack({{25, 50, true, shinyPid()}, {133, 12, false, 0xABCDEF01},
                             {585, 20, false, 0x12345678, 2, 1}}), // female Deerling Autumn
                  1000000);
        const auto probe = G::inspectSourceFile(f.save, provider);
        assert(probe.ready());
        assert(f.bindings.load());
        assert(f.bindings.assignFileAndSave(probe.sourceIdentity,
            {"profile", "black_nds", f.save, provider, "BW"}));

        S::SnapshotRefresh outcome{};
        const auto* snapshot = f.refresh(outcome);
        assert(snapshot && outcome == S::SnapshotRefresh::Parsed && f.parses == 1);
        assert(snapshot->current());
        assert(snapshot->trainerName == "NX");
        assert(snapshot->trainerGenderKnown && snapshot->trainerGender == 1);
        assert(snapshot->dexSeen == 1 && snapshot->dexCaught == 1 && snapshot->dexTotal == 649);
        assert(snapshot->partyCount == 3);
        assert(snapshot->party[0].species == 25 && snapshot->party[0].level == 50 &&
               snapshot->party[0].shiny && snapshot->party[0].form == 0);
        assert(snapshot->party[1].species == 133 && snapshot->party[1].level == 12 &&
               !snapshot->party[1].shiny);
        // The party strip keeps the native form (QA: no form drop).
        assert(snapshot->party[2].species == 585 && snapshot->party[2].form == 2 &&
               snapshot->party[2].gender == 1 && snapshot->party[0].gender == 0);
        assert(snapshot->party[3].species == 0);
        assert(snapshot->contentFingerprint == probe.contentFingerprint);
        // Same wording as Gen I-IV: current, never "cached".
        assert(S::homePartyStatus(*snapshot) == "Current save party");

        // Home card provider comes from the persisted Save Instance metadata.
        const auto assigned = f.bindings.resolveFileForGame("profile", "black_nds");
        const auto line = S::homeSourceLine("Nintendo DS",
            S::homeCardSourceText("REMEMBERED", assigned.binding.sourceType));
        assert(line == "Nintendo DS \xE2\x80\xA2 " +
               (provider == "Manual" ? std::string("Manual file") : provider));
        // Save Instance row for the same file.
        assert(S::saveInstanceSourceLine(probe).rfind(
            provider == "Manual" ? "Manual file / " : provider + " / ", 0) == 0);
    }
}

void refreshOnChangeNoReparseWhenUnchanged(const std::string& root) {
    Fixture f(root + "/watch");
    assert(::mkdir(f.dir.c_str(), 0700) == 0);
    writeFile(f.save, makeBlack({{25, 50, false, 0x12345678}}), 2000000);
    const auto probe = G::inspectSourceFile(f.save, "DraStic");
    assert(probe.ready() && f.bindings.load());
    assert(f.bindings.assignFileAndSave(probe.sourceIdentity,
        {"profile", "black_nds", f.save, "DraStic", "BW"}));
    S::SnapshotRefresh outcome{};
    assert(f.refresh(outcome)->party[0].species == 25 && f.parses == 1);

    // Profile/focus changes and 1.5 s polls while unchanged: stat only.
    for (int poll = 0; poll < 50; ++poll) {
        f.refresh(outcome);
        assert(outcome == S::SnapshotRefresh::Reused);
    }
    assert(f.parses == 1);

    // The emulator writes a new party -> re-validated automatically.
    writeFile(f.save, makeBlack({{495, 7, false, 0x0BADF00D}, {25, 51, false, 0x12345678}}),
              2000060);
    const auto* snapshot = f.refresh(outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && f.parses == 2);
    assert(snapshot->current() && snapshot->partyCount == 2);
    assert(snapshot->party[0].species == 495 && snapshot->party[0].level == 7);
    assert(snapshot->party[1].species == 25 && snapshot->party[1].level == 51);
    assert(snapshot->contentFingerprint != probe.contentFingerprint);

    // Rewrite of identical bytes (mtime only): re-validated once, same data.
    const std::string fingerprint = snapshot->contentFingerprint;
    writeFile(f.save, makeBlack({{495, 7, false, 0x0BADF00D}, {25, 51, false, 0x12345678}}),
              2000120);
    snapshot = f.refresh(outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && f.parses == 3);
    assert(snapshot->current() && snapshot->contentFingerprint == fingerprint);

    // Half-written / corrupted save: last validated party stays visible but
    // STALE; it is never presented as current.
    auto broken = makeBlack({{495, 7, false, 0x0BADF00D}});
    broken[0x19400 + 4] ^= 0xFF; // trainer block checksum no longer matches
    writeFile(f.save, broken, 2000180);
    snapshot = f.refresh(outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && f.parses == 4);
    assert(snapshot->state == S::SnapshotState::Stale && !snapshot->current());
    assert(snapshot->party[0].species == 495);
    assert(S::homePartyStatus(*snapshot).rfind("STALE", 0) == 0);
    f.refresh(outcome);
    assert(outcome == S::SnapshotRefresh::Reused && f.parses == 4);

    // Fixed by the emulator's next write.
    writeFile(f.save, makeBlack({{25, 52, false, 0x12345678}}), 2000240);
    snapshot = f.refresh(outcome);
    assert(snapshot->current() && snapshot->party[0].level == 52);

    // File removed: the binding reports Missing; the UI invalidates and the
    // card shows the separate missing state (no stale party as current).
    assert(::unlink(f.save.c_str()) == 0);
    assert(f.bindings.resolveFileForGame("profile", "black_nds").status ==
           PokeVault::Legacy::AssignedFileStatus::Missing);
    assert(f.refresh(outcome) == nullptr);
    assert(S::homeCardSourceText("MISSING", "DraStic") == "Missing source");
}

void newGameAndCopyGateStatesPreserved(const std::string& root) {
    Fixture f(root + "/newgame");
    assert(::mkdir(f.dir.c_str(), 0700) == 0);
    writeFile(f.save, makeBlack({}, true), 3000000);
    const auto probe = G::inspectSourceFile(f.save, "melonDS");
    assert(probe.ready()); // 35cbf163 accepts a freshly initialized save as NewGameNoTrainer
    assert(f.bindings.load());
    assert(f.bindings.assignFileAndSave(probe.sourceIdentity,
        {"profile", "black_nds", f.save, "melonDS", "BW"}));
    S::SnapshotRefresh outcome{};
    const auto* snapshot = f.refresh(outcome);
    assert(snapshot && snapshot->current() && snapshot->newGameNoTrainer);
    assert(!snapshot->trainerGenderKnown); // neutral portrait, never "Male"
    assert(S::homePartyStatus(*snapshot) == "New-game save: no trainer or party yet.");
    assert(S::homeCardSourceText("REMEMBERED / NEW-GAME SAVE, NO TRAINER YET", "melonDS") ==
           "melonDS \xE2\x80\xA2 New game, no trainer yet");
    // Copy gate: the card names the state; the snapshot has no data.
    G::AssignedSourceReadOnly ambiguous;
    ambiguous.diagnostic = "two valid but different copies; choose it explicitly";
    const auto gated = G::homeCardSnapshot(ambiguous);
    assert(!gated.hasData());
    assert(gated.reason == G::cardFailureReason(G::CardFailure::ChooseCopy));
}

} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_home_gen5_XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    assert(temp);
    const std::string root = temp;
    currentSaveOnHomeWithoutEditor(root);
    refreshOnChangeNoReparseWhenUnchanged(root);
    newGameAndCopyGateStatesPreserved(root);
    const std::string cleanup = "rm -rf '" + root + "'";
    assert(std::system(cleanup.c_str()) == 0);
    std::cout << "Gen V Home card: current validated save (trainer, gender, party sprites/"
                 "levels/shiny, dex, provider) without the editor; refresh on change, no "
                 "reparse when unchanged, stale/missing explicit PASS\n";
    return 0;
}
