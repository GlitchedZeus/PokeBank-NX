// Gen IV Home card through the shared fingerprinted snapshot cache. Card
// output is unchanged except for the provider label: the same trainer,
// gender, dex and party species/level/form the hub always showed, plus the
// Gen I-V Home shiny badge and gender (campaign directive: one Home card rule),
// parsed once per save change instead of on every focus change.
// Synthetic DP save only: block offsets/footers mirror
// tests/test_gen4_readonly_foundation.cpp (cited in Gen4ReadOnlySave.cpp);
// CRC comes from the pinned PKSM-Core oracle.
#include "Encryption/Encryption4.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4GameCardPreview.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveProviderIdentity.h"
#include "Utils/Gen4TextCodec.h"
#include "utils/crypto.hpp"

#include <cassert>
#include <cstddef>
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

namespace S = PokeVault::Source;
namespace G4 = PokeVault::Integration::Gen4;

namespace {
constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC_INTL = 0x20060623;
// Diamond/Pearl general/storage block geometry (same as the foundation test).
constexpr size_t GENERAL_SIZE = 0xC100, STORAGE_START = 0xC100, STORAGE_SIZE = 0x121E0;
constexpr size_t FOOTER = 0x14, TRAINER = 0x64, PARTY = 0x98, BOX_DATA = 4, BOX_STRIDE = 0xFF0;

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v); b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    for (int i = 0; i < 4; ++i) b[o + i] = static_cast<uint8_t>(v >> (8 * i));
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v); b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    for (int i = 0; i < 4; ++i) b[o + i] = static_cast<std::byte>(v >> (8 * i));
}
void put(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i) dst[off + i] = static_cast<uint8_t>(src[i]);
}
void put(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(), dst.begin() + static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> partyEntity(uint16_t species, uint8_t level, uint8_t form,
                                   uint32_t pid = 0x12345678u, uint8_t gender = 0) {
    const uint8_t language = static_cast<uint8_t>(Enums::LanguageID::English);
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    wb32(d, 0, pid);
    wb16(d, 0x08, species);
    wb16(d, 0x0C, 12345);
    wb16(d, 0x0E, 54321);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x17] = static_cast<std::byte>(language);
    wb16(d, 0x28, 33);
    d[0x30] = std::byte{35};
    d[0x40] = static_cast<std::byte>((form << 3) | (gender << 1));
    put(d, 0x48, Utils::encodeGen4Field(u"MON", 11, 10, language));
    put(d, 0x68, Utils::encodeGen4Field(u"ASH", 8, 7, language));
    wb32(d, 0x88, 0);
    d[0x8C] = static_cast<std::byte>(level);
    wb16(d, 0x8E, 30); wb16(d, 0x90, 30);
    wb16(d, 0x06, Encryption::checksum4(d));
    return Encryption::encryptArray4(d);
}

void footer(std::vector<uint8_t>& save, size_t off, size_t len, uint32_t major, uint32_t minor) {
    const size_t end = off + len;
    w32(save, end - 0x14, major); w32(save, end - 0x10, minor);
    w32(save, end - 0x0C, static_cast<uint32_t>(len)); w32(save, end - 0x08, MAGIC_INTL);
    const uint16_t crc = pksm::crypto::ccitt16(std::span<const uint8_t>(save).subspan(off, len - FOOTER));
    w16(save, end - 2, crc);
}

struct Member { uint16_t species; uint8_t level; uint8_t form;
                uint32_t pid = 0x12345678u; uint8_t gender = 0; };
// (TID 12345 ^ SID 54321 ^ PIDhi ^ PIDlo) == 0 -> shiny under the Gen IV rule.
constexpr uint32_t kShinyPid = (static_cast<uint32_t>(0x1111u ^ 12345u ^ 54321u) << 16) | 0x1111u;

std::vector<uint8_t> makeDiamond(const std::vector<Member>& party, uint8_t gender) {
    const uint8_t language = static_cast<uint8_t>(Enums::LanguageID::English);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    for (int p = 0; p < 2; ++p) {
        const size_t base = static_cast<size_t>(p) * PARTITION;
        std::fill(save.begin() + base, save.begin() + base + GENERAL_SIZE, 0);
        const size_t st = base + STORAGE_START;
        std::fill(save.begin() + st, save.begin() + st + STORAGE_SIZE, 0);
        const size_t tr = base + TRAINER;
        put(save, tr, Utils::encodeGen4Field(u"ASH", 8, 7, language));
        w16(save, tr + 0x10, 12345); w16(save, tr + 0x12, 54321); w32(save, tr + 0x14, 999);
        save[tr + 0x18] = gender; save[tr + 0x19] = language; save[tr + 0x1A] = 8; save[tr + 0x1C] = 7;
        save[base + PARTY - 4] = static_cast<uint8_t>(party.size());
        for (size_t i = 0; i < party.size(); ++i)
            put(save, base + PARTY + i * Encryption::SIZE_PARTY4,
                partyEntity(party[i].species, party[i].level, party[i].form,
                            party[i].pid, party[i].gender));
        for (size_t box = 0; box < 18; ++box)
            for (size_t slot = 0; slot < 30; ++slot)
                put(save, st + BOX_DATA + box * BOX_STRIDE + slot * Encryption::SIZE_STORED4, blank);
        for (size_t box = 0; box < 18; ++box)
            put(save, st + 4 + 18 * 0xFF0 + box * 40, Utils::encodeGen4Field(u"BOX", 20, 8, language));
        footer(save, base, GENERAL_SIZE, p == 0 ? 20 : 10, 1);
        footer(save, st, STORAGE_SIZE, p == 0 ? 30 : 15, 2);
    }
    return save;
}

void writeFile(const std::string& path, const std::vector<uint8_t>& bytes, long mtime) {
    FILE* f = std::fopen(path.c_str(), "wb");
    assert(f);
    assert(std::fwrite(bytes.data(), 1, bytes.size(), f) == bytes.size());
    assert(std::fclose(f) == 0);
    const struct timeval times[2] = {{mtime, 0}, {mtime, 0}};
    assert(::utimes(path.c_str(), times) == 0);
}

void gen4CardsThroughSnapshotCache(const std::string& root) {
    const std::string save = root + "/diamond.sav";
    writeFile(save, makeDiamond({{25, 25, 0, kShinyPid, 1}, {201, 9, 3, 0x12345678u, 2}}, 1), 1000);
    for (std::string provider : {"DraStic", "melonDS", "RetroArch/DeSmuME", "RetroArch", "Manual"}) {
        PokeVault::Legacy::LegacySourceBindings bindings(root + "/bindings-" + S::providerIdFor(provider));
        assert(bindings.load());
        const auto candidate = G4::inspectSourceFile(save, provider, "diamond_nds");
        assert(candidate.ready());
        assert(bindings.assignFileAndSave(candidate.sourceIdentity,
            {"user", "diamond_nds", save, provider, candidate.expectedRawFamily}));
        S::HomeSnapshotCache cache;
        int parses = 0;
        auto refresh = [&](S::SnapshotRefresh& outcome) {
            const auto assigned = bindings.resolveFileForGame("user", "diamond_nds");
            return &cache.refresh(S::assignedSnapshotRequest("user", "diamond_nds", assigned),
                S::statRegularFile, [&] {
                    ++parses;
                    return G4::homeCardSnapshot(G4::openAssignedSource(bindings, "user", "diamond_nds"));
                }, &outcome);
        };
        S::SnapshotRefresh outcome{};
        const auto* snapshot = refresh(outcome);
        assert(snapshot->current() && parses == 1);

        // Exactly what the hub showed before: real save trainer, gender, dex,
        // party species/level/form, now with the shared shiny badge and gender.
        const auto direct = G4::openAssignedSource(bindings, "user", "diamond_nds");
        assert(direct.status == G4::OpenStatus::Ready && direct.save);
        assert(snapshot->trainerName == Utils::utf16ToUtf8(direct.save->trainer().name));
        assert(snapshot->trainerName == "ASH");
        assert(snapshot->trainerGenderKnown && snapshot->trainerGender == direct.save->trainer().gender);
        assert(snapshot->dexSeen == direct.save->dexProgress().seen &&
               snapshot->dexCaught == direct.save->dexProgress().caught &&
               snapshot->dexTotal == direct.save->dexProgress().total);
        const auto party = direct.save->party();
        assert(party.size() == 2);
        for (size_t i = 0; i < party.size(); ++i) {
            assert(snapshot->party[i].species == party[i].species());
            assert(snapshot->party[i].level == party[i].partyLevel());
            assert(snapshot->party[i].form == party[i].form());
            assert(snapshot->party[i].gender == party[i].gender());
        }
        assert(snapshot->party[0].shiny && snapshot->party[0].gender == 1);
        assert(!snapshot->party[1].shiny && snapshot->party[1].gender == 2);
        assert(snapshot->party[0].species == 25 && snapshot->party[0].level == 25);
        assert(snapshot->party[1].species == 201);
        assert(snapshot->party[2].species == 0);
        assert(S::homePartyStatus(*snapshot) == "Current save party");

        // Only the provider label changes on the card.
        const auto assigned = bindings.resolveFileForGame("user", "diamond_nds");
        const std::string shown = S::homeSourceLine("Nintendo DS",
            S::homeCardSourceText("REMEMBERED", assigned.binding.sourceType));
        assert(shown == "Nintendo DS \xE2\x80\xA2 " + S::providerDisplayName(provider));
        assert(shown.find("Linked save") == std::string::npos);
        // Save Instance row built by Gen IV discovery for the same file.
        const auto row = G4::toSaveInstance(candidate, "diamond_nds");
        assert(S::saveInstanceSourceLine(row).rfind(S::providerDisplayName(provider) + " / ", 0) == 0);

        // Unchanged: no reparse across many focus changes.
        for (int i = 0; i < 30; ++i) {
            refresh(outcome);
            assert(outcome == S::SnapshotRefresh::Reused);
        }
        assert(parses == 1);
    }

    // Change + stale on the real parser.
    PokeVault::Legacy::LegacySourceBindings bindings(root + "/bindings-watch");
    assert(bindings.load());
    const auto candidate = G4::inspectSourceFile(save, "DraStic", "diamond_nds");
    assert(bindings.assignFileAndSave(candidate.sourceIdentity,
        {"user", "diamond_nds", save, "DraStic", candidate.expectedRawFamily}));
    S::HomeSnapshotCache cache;
    auto refresh = [&](S::SnapshotRefresh& outcome) {
        const auto assigned = bindings.resolveFileForGame("user", "diamond_nds");
        return &cache.refresh(S::assignedSnapshotRequest("user", "diamond_nds", assigned),
            S::statRegularFile, [&] {
                return G4::homeCardSnapshot(G4::openAssignedSource(bindings, "user", "diamond_nds"));
            }, &outcome);
    };
    S::SnapshotRefresh outcome{};
    assert(refresh(outcome)->party[0].species == 25);
    writeFile(save, makeDiamond({{448, 40, 0}}, 0), 2000);
    const auto* snapshot = refresh(outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && snapshot->current());
    assert(snapshot->party[0].species == 448 && snapshot->party[0].level == 40 &&
           snapshot->trainerGender == 0 && snapshot->party[1].species == 0);
    std::vector<uint8_t> garbage(SAVE_SIZE, 0x5A);
    writeFile(save, garbage, 3000);
    snapshot = refresh(outcome);
    assert(snapshot->state == S::SnapshotState::Stale && snapshot->party[0].species == 448);
    assert(S::homePartyStatus(*snapshot).rfind("STALE", 0) == 0);
    // Never-validated assignment that fails: the accepted Gen IV wording.
    S::HomeSnapshotCache fresh;
    const auto assigned = bindings.resolveFileForGame("user", "diamond_nds");
    const auto& failed = fresh.refresh(S::assignedSnapshotRequest("user", "diamond_nds", assigned),
        S::statRegularFile, [&] {
            return G4::homeCardSnapshot(G4::openAssignedSource(bindings, "user", "diamond_nds"));
        });
    assert(!failed.hasData() && S::homePartyStatus(failed) == G4::UnavailablePartyReason);
}

} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_home_gen4_XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    assert(temp);
    const std::string root = temp;
    gen4CardsThroughSnapshotCache(root);
    const std::string cleanup = "rm -rf '" + root + "'";
    assert(std::system(cleanup.c_str()) == 0);
    std::cout << "Gen IV Home card: unchanged trainer/dex/party output through the snapshot cache, "
                 "real provider label, refresh on change, stale explicit PASS\n";
    return 0;
}
