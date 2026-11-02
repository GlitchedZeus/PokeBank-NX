// Gen V real-save failure diagnosis (owner hardware report: Black 2 .dsv shown
// as "Party 0 / Boxed 0", Trainer TID 0, unreadable name, Dex 0).
// Valid block checksums do NOT prove a copy holds the player's game. These
// fixtures reproduce retail states: an initial/blank copy whose blocks are
// checksum-valid but whose PlayerData holds only the version byte, beside a
// populated copy; encrypted-blank box slots (PID 0, checksum 0); genuinely
// empty saves; unreadable names; and damaged declared party records.
// Layout: PKHeX 6501f0ab SaveBlockAccessor5BW/B2W2, PlayerData5, SAV5.
#include "Integration/Gen5/Gen5SaveDiagnostics.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5TrainerScreen.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace T = PokeVault::Integration::Gen5::TrainerLayout5;
using State = G::CopyState5;

namespace {

size_t copySize(G::SaveFamily f) { return f == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize; }

void stamp(std::vector<uint8_t>& sav, size_t base, G::SaveFamily f) {
    const auto blocks = f == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                                       : std::span<const L::SaveBlock>(L::Black2White2);
    auto copy = std::span<uint8_t>(sav).subspan(base, copySize(f));
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

// Fill every box slot with the encrypted blank PK5 the game stores for "no
// Pokemon" (PID 0, checksum 0, zero payload => PRNG keystream bytes).
void encryptedBlankBoxes(std::vector<uint8_t>& sav, size_t base) {
    const std::vector<uint8_t> blank(C::StoredSize, 0);
    const auto enc = C::encryptCandidate(blank);
    assert(enc.size() == C::StoredSize);
    bool nonzero = false;
    for (auto b : enc) nonzero |= b != 0;
    assert(nonzero);  // the encrypted blank is NOT zero-filled
    for (size_t box = 0; box < L::BoxCount; ++box)
        for (size_t slot = 0; slot < L::BoxSlots; ++slot)
            std::copy(enc.begin(), enc.end(),
                      sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + box * L::BoxStride +
                                                                slot * C::StoredSize));
}

// Initial/blank copy: only the game version byte, encrypted-blank boxes.
void blankCopy(std::vector<uint8_t>& sav, size_t base, G::SaveFamily f, uint8_t version) {
    sav[base + 0x19400 + T::GameOffset] = version;
    encryptedBlankBoxes(sav, base);
    stamp(sav, base, f);
}

// Populated copy: female trainer "Rosa", TID/SID, party of 2, boxed Pokemon,
// dex flags, money; remaining box slots are encrypted blanks.
void populatedCopy(std::vector<uint8_t>& sav, size_t base, G::SaveFamily f, uint8_t version,
                   const char16_t* name = u"Rosa", uint8_t gender = 1) {
    encryptedBlankBoxes(sav, base);
    const size_t player = base + 0x19400;
    for (size_t i = 0; name[i]; ++i) C::write16(sav, player + T::NameOffset + i * 2, name[i]);
    size_t len = 0;
    while (name[len]) ++len;
    C::write16(sav, player + T::NameOffset + len * 2, 0xFFFF);
    C::write16(sav, player + T::TidOffset, 24680);
    C::write16(sav, player + T::SidOffset, 13579);
    sav[player + T::GameOffset] = version;
    sav[player + T::GenderOffset] = gender;
    C::write16(sav, player + T::HoursOffset, 42);
    sav[base + L::PartyOffset + 4] = 2;
    for (size_t i = 0; i < 2; ++i) {
        std::vector<uint8_t> pk(C::PartySize, 0);
        C::write16(pk, 0, static_cast<uint16_t>(0x2000 + i));
        C::write16(pk, 8, static_cast<uint16_t>(i ? 501 : 495));
        pk[0x8C] = 12;
        const auto e = C::encryptCandidate(pk);
        std::copy(e.begin(), e.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8 + i * C::PartySize));
    }
    std::vector<uint8_t> boxed(C::StoredSize, 0);
    C::write16(boxed, 0, 0x3141);
    C::write16(boxed, 8, 504);
    const auto e = C::encryptCandidate(boxed);
    std::copy(e.begin(), e.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + 3 * C::StoredSize));
    const size_t dex = base + (f == G::SaveFamily::BlackWhite ? 0x21600 : 0x21400);
    for (const size_t sp : {495u, 501u, 504u}) {
        sav[dex + 0x08 + ((sp - 1) >> 3)] |= uint8_t(1u << ((sp - 1) & 7));
        sav[dex + 0x5C + ((sp - 1) >> 3)] |= uint8_t(1u << ((sp - 1) & 7));
    }
    stamp(sav, base, f);
}

G::ReadOnlyProbe probe(const std::vector<uint8_t>& sav, const char* game, G::SaveCopySelection sel) {
    G::SourceContext ctx{game, "drastic", "DraStic", "sdmc:/drastic/backup/test.dsv",
                         "sdmc:/drastic/backup/test.dsv", "inode:1", "id", "profile", 3};
    return G::probeNormalizedBattery(sav, ctx, sel);
}

void dualCopyBlankPrimary(G::SaveFamily f, const char* game, uint8_t version) {
    // Reproduces the owner symptom: the Primary copy is checksum-valid but
    // blank (version byte only), the Backup copy holds the real game.
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    blankCopy(sav, 0, f, version);
    populatedCopy(sav, copySize(f), f, version);
    const auto report = G::inspectSave5(sav, "desmume-dsv", G::SaveCopySelection::Primary);
    assert(report.copies.size() == 2 && !report.copiesIdentical);
    assert(report.copies[0].blocksValid == report.copies[0].blocksTotal);
    // A new-game copy next to a real game is a stale copy: refused below.
    assert(report.copies[0].state == State::NewGameNoTrainer);
    assert(!report.copies[0].trainerIdPresent && report.copies[0].nameLength == 0);
    assert(report.copies[0].blankEncryptedBoxSlots == 720 && report.copies[0].occupiedBoxRecords == 0);
    assert(report.copies[1].state == State::Populated);
    assert(report.copies[1].validPartyRecords == 2 && report.copies[1].occupiedBoxRecords == 1);
    assert(report.copies[1].blankEncryptedBoxSlots == 719 && report.copies[1].genderByte == 1);
    assert(report.copies[1].dexCaught == 3 && report.copies[1].dexSeen == 3);
    assert(report.chosenIndex && *report.chosenIndex == 0);
    assert(*G::openableAlternative5(report) == 1);

    // Automatic selection refuses to guess between two different copies.
    assert(!G::Gen5ReadOnlySave::parse(sav, game, nullptr, G::SaveCopySelection::Automatic));
    // The blank Primary is refused with an explicit reason naming the Backup;
    // nothing is silently swapped.
    const auto primary = probe(sav, game, G::SaveCopySelection::Primary);
    assert(!primary.ready() && primary.instance.validation == PokeVault::Source::ValidationStatus::Invalid);
    assert(primary.instance.diagnostic.find("Primary copy: no trainer data") != std::string::npos);
    assert(primary.instance.diagnostic.find("Backup copy is valid, populated") != std::string::npos);
    const auto backup = probe(sav, game, G::SaveCopySelection::Backup);
    assert(backup.ready() && backup.save->selectedBackupPartition());
    assert(backup.instance.trainerName == "Rosa" && backup.instance.partyCount == 2);
    // The report carries no personal data.
    const auto text = G::formatSaveReport5(report, game);
    assert(text.find("Rosa") == std::string::npos && text.find("24680") == std::string::npos);
    assert(text.find("COPY0_STATE=no trainer data") != std::string::npos);
    assert(text.find("COPY1_STATE=valid, populated") != std::string::npos);
    assert(text.find("COPY1_GENDER_BYTE=1") != std::string::npos);
}

void singleBlankCopy() {
    // Only a new-game Primary validates and nothing better exists: it opens
    // (read-only presentation says "no trainer yet"), never as invented data.
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    blankCopy(sav, 0, G::SaveFamily::Black2White2, 23);
    const auto p = probe(sav, "black2_nds", G::SaveCopySelection::Automatic);
    assert(p.ready() && p.instance.trainerName.empty() && p.instance.partyCount == 0);
    // A blank copy that is NOT a new game (play time recorded, no identity)
    // is still refused.
    std::vector<uint8_t> odd(L::FullSaveSize, 0);
    odd[0x19400 + T::GameOffset] = 23;
    C::write16(odd, 0x19400 + T::HoursOffset, 5);
    encryptedBlankBoxes(odd, 0);
    stamp(odd, 0, G::SaveFamily::Black2White2);
    assert(G::inspectSave5(odd, "raw").copies[0].state == State::NoTrainerIdentity);
    const auto q = probe(odd, "black2_nds", G::SaveCopySelection::Automatic);
    assert(!q.ready() && q.instance.diagnostic.find("blank or initial copy") != std::string::npos);
    assert(q.instance.diagnostic.find("other copy is checksums invalid") != std::string::npos);
}

// Models the real state the owner's four DraStic Gen V files were found in
// (built from the format, never from the owner's files): both copies valid
// and identical, version bytes 20-23, no trainer written yet, every box slot
// the encrypted blank. It opens; nothing shows "Male", zeros are labelled
// as a new-game save, all 720 slots accept Create, and Create explains why it
// cannot copy an Original Trainer.
void realNewGameSave(G::SaveFamily f, const char* game, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    blankCopy(sav, 0, f, version);
    blankCopy(sav, copySize(f), f, version);
    const auto report = G::inspectSave5(sav, "desmume-dsv");
    assert(report.copiesIdentical && report.copies.size() == 2);
    assert(report.copies[0].state == State::NewGameNoTrainer && report.copies[1].state == State::NewGameNoTrainer);
    assert(report.copies[0].blankEncryptedBoxSlots == 720 && report.copies[0].zeroFilledBoxSlots == 0);
    assert(!G::openableAlternative5(report));
    const auto p = probe(sav, game, G::SaveCopySelection::Automatic);
    assert(p.ready() && p.save && p.instance.trainerName.empty());
    const auto& t = p.save->trainer();
    assert(G::newGameNoTrainer5(t.rawName.size(), t.tid, t.sid, t.playedHours, t.playedMinutes, t.playedSeconds));
    G::Gen5StagedPokemonWorkspace ws(*p.save);
    using Screen = G::Gen5TrainerScreen;
    assert(Screen::value(ws, Screen::Row::Gender) == "-");
    assert(Screen::value(ws, Screen::Row::Name) == "- (no trainer yet)");
    assert(Screen::value(ws, Screen::Row::TrainerId) == "-");
    Screen screen;
    std::string error;
    assert(screen.enter(ws, error));
    assert(!screen.beginEdit(ws, error) && error.find("New-game save") != std::string::npos);
    size_t empty = 0;
    for (size_t box = 0; box < L::BoxCount; ++box)
        for (size_t slot = 0; slot < L::BoxSlots; ++slot) empty += ws.emptyCloneDestination(box, slot);
    assert(empty == 720);
    assert(!G::Gen5SharedScreenState::sharedCreateDefaults(ws, 1, "Pikachu", {2026, 10, 15}, error));
    assert(error.find("no trainer yet") != std::string::npos);
}

void singleBlankCopyFile() {
    // Automatic on a file whose only valid copy is a new-game save: a Ready
    // row that says so, never a row that looks like a played game with zeros.
    char pattern[] = "/tmp/pokebank_gen5_blank_XXXXXX";
    assert(::mkdtemp(pattern));
    const std::string dir = pattern, src = dir + "/w.sav";
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    blankCopy(sav, 0, G::SaveFamily::BlackWhite, 20);
    {
        FILE* f = std::fopen(src.c_str(), "wb");
        assert(f && std::fwrite(sav.data(), 1, sav.size(), f) == sav.size() && std::fclose(f) == 0);
    }
    const auto choices = G::inspectSourceChoices(src, "DraStic", "white_nds");
    assert(choices.size() == 1 && choices[0].ready());
    assert(choices[0].sourceLabel.find("New-game save: no trainer yet") != std::string::npos);
    assert(choices[0].sourceLabel.find("NOT OPENABLE") == std::string::npos);
    assert(std::remove(src.c_str()) == 0 && ::rmdir(dir.c_str()) == 0);
}

void otherStates() {
    // Genuinely empty: trainer present, no Pokemon -> opens.
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    populatedCopy(sav, 0, G::SaveFamily::BlackWhite, 21);
    sav[0x18E00 + 4] = 0;
    for (size_t i = 0; i < C::StoredSize; ++i) sav[L::BoxOffset + 3 * C::StoredSize + i] = 0;
    stamp(sav, 0, G::SaveFamily::BlackWhite);
    auto r = G::inspectSave5(sav, "raw-nds-battery");
    assert(r.copies[0].state == State::GenuinelyEmpty && G::copyOpenable5(r.copies[0].state));
    assert(probe(sav, "black_nds", G::SaveCopySelection::Automatic).ready());

    // Unreadable name (lone surrogate) is openable but flagged.
    std::vector<uint8_t> odd(L::FullSaveSize, 0);
    populatedCopy(odd, 0, G::SaveFamily::BlackWhite, 20, u"\xD800" u"A", 0);
    r = G::inspectSave5(odd, "raw-nds-battery");
    assert(r.copies[0].state == State::UnreadableTrainerName && !r.copies[0].nameDecodes);
    const auto oddProbe = probe(odd, "white_nds", G::SaveCopySelection::Automatic);
    assert(oddProbe.ready() && oddProbe.instance.trainerName.empty());

    // Damaged declared party: refused by the strict reader too.
    std::vector<uint8_t> bad(L::FullSaveSize, 0);
    populatedCopy(bad, 0, G::SaveFamily::Black2White2, 22);
    bad[L::PartyOffset + 8 + 30] ^= 0x55;
    stamp(bad, 0, G::SaveFamily::Black2White2);
    r = G::inspectSave5(bad, "raw-nds-battery");
    assert(r.copies[0].state == State::InvalidPokemonRecords && r.copies[0].invalidPartyRecords == 1);
    assert(!probe(bad, "white2_nds", G::SaveCopySelection::Automatic).ready());

    // Checksum-invalid image: no copy chosen, reason recorded.
    std::vector<uint8_t> broken(L::FullSaveSize, 0);
    broken[0x19400 + T::GameOffset] = 21;
    r = G::inspectSave5(broken, "raw-nds-battery");
    assert(!r.chosenIndex && r.copies.size() == 2 && r.copies[0].state == State::ChecksumInvalid);
    assert(G::chosenState5(r) == State::ChecksumInvalid);
}

void reportFile() {
    char pattern[] = "/tmp/pokebank_gen5_diag_XXXXXX";
    assert(::mkdtemp(pattern));
    const std::string dir = pattern;
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    blankCopy(sav, 0, G::SaveFamily::Black2White2, 23);
    populatedCopy(sav, L::Black2White2CopySize, G::SaveFamily::Black2White2, 23);
    const std::string src = dir + "/b2.sav";
    {
        FILE* f = std::fopen(src.c_str(), "wb");
        assert(f && std::fwrite(sav.data(), 1, sav.size(), f) == sav.size() && std::fclose(f) == 0);
    }
    // Save Instances rows use the same gate: the checksum-valid blank Primary
    // never becomes a Ready "Party 0 / Boxed 0" row; both copies are offered.
    {
        const auto primary = G::inspectSourceFile(src, "DraStic", "black2_nds", G::SaveCopySelection::Primary);
        assert(!primary.ready() && primary.validation == PokeVault::Source::ValidationStatus::Invalid);
        assert(primary.sourceLabel.find("NOT OPENABLE") != std::string::npos);
        assert(primary.sourceLabel.find("Party 0") == std::string::npos && primary.partyCount == 0);
        assert(primary.diagnostic.find("Backup copy is valid, populated - choose it explicitly") != std::string::npos);
        const auto choices = G::inspectSourceChoices(src, "DraStic", "black2_nds");
        assert(choices.size() == 2);
        assert(!choices[0].ready() && choices[0].sourceVariant == "gen5-copy-0-primary");
        assert(choices[1].ready() && choices[1].sourceVariant == "gen5-copy-1-backup");
        assert(choices[1].partyCount == 2 && choices[1].trainerName == "Rosa");
        assert(choices[1].sourceLabel.find("Backup copy / Party 2 / Boxed 1") != std::string::npos);
    }
    std::string written, error;
    assert(G::writeGen5DiagnosticsReport(src, "black2_nds", "20261015_101010", dir + "/diagnostics", written, error));
    std::ifstream in(written);
    const std::string text((std::istreambuf_iterator<char>(in)), {});
    assert(text.find("COPY0_STATE=no trainer data") != std::string::npos);
    assert(text.find("COPY1_STATE=valid, populated") != std::string::npos);
    assert(text.find("Rosa") == std::string::npos);
    // Never overwrites: the same name fails; unsafe ids are refused.
    std::string again;
    assert(!G::writeGen5DiagnosticsReport(src, "black2_nds", "20261015_101010", dir + "/diagnostics", again, error));
    assert(!G::writeGen5DiagnosticsReport(src, "../x", "1", dir + "/diagnostics", again, error));
    // The source is only read.
    std::ifstream s(src, std::ios::binary);
    const std::vector<uint8_t> after((std::istreambuf_iterator<char>(s)), {});
    assert(after == sav);
    assert(std::remove(written.c_str()) == 0 && ::rmdir((dir + "/diagnostics").c_str()) == 0);
    assert(std::remove(src.c_str()) == 0 && ::rmdir(dir.c_str()) == 0);
}

} // namespace

int main() {
    // The encrypted blank is what Crypto::decrypt maps back to all zeros.
    const std::vector<uint8_t> blank(C::StoredSize, 0);
    assert(G::nativeBlankSlot5(C::encryptCandidate(blank)) && G::nativeBlankSlot5(blank));
    dualCopyBlankPrimary(G::SaveFamily::BlackWhite, "black_nds", 21);
    dualCopyBlankPrimary(G::SaveFamily::BlackWhite, "white_nds", 20);
    dualCopyBlankPrimary(G::SaveFamily::Black2White2, "black2_nds", 23);
    dualCopyBlankPrimary(G::SaveFamily::Black2White2, "white2_nds", 22);
    singleBlankCopy();
    singleBlankCopyFile();
    realNewGameSave(G::SaveFamily::BlackWhite, "white_nds", 20);
    realNewGameSave(G::SaveFamily::BlackWhite, "black_nds", 21);
    realNewGameSave(G::SaveFamily::Black2White2, "white2_nds", 22);
    realNewGameSave(G::SaveFamily::Black2White2, "black2_nds", 23);
    otherStates();
    reportFile();
    std::cout << "Gen V save diagnostics: blank/stale copy, populated, empty, unreadable name, "
                 "invalid party, checksum-invalid, report file PASS\n";
}
