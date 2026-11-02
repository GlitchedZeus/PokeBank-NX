// PKSE 620c5f6 review "PID, shiny and gender preservation" (owner directive): through the
// production staged-editing routes of Generations III, IV and V,
//   1. an IV / EV edit never changes the PID, shininess, gender, nature or ability slot;
//   2. re-staging a field to its current value is not an edit (no PID reroll, same bytes,
//      no dirty / pending state);
//   3. Cancel / Discard / rollback restores the native bytes exactly and clears that state;
//   4. a record that is already unusual (gender or ability disagreeing with its PID) is not
//      silently altered by an unrelated IV / EV edit;
//   5. after Keep / Review and serialization a strict reparse shows the same identity and the
//      new IV / EV values, and the source bytes are unchanged.
// Routes: Gen III Gen3SharedEditor::Session -> StagedPokemonEditor (box and Party) ->
// finalizedBytes; Gen IV Gen4SharedEditor::Session -> Gen4StagedPokemonEditor (box) ->
// describeStagedReviewChanges -> finalizedBytes; Gen V stageSharedField ->
// Gen5SharedPokemonSession::keep -> verifyStagedReview -> buildStagedSaveCandidate.
// Fixture builders follow tests/test_gen3_staged_pokemon_editor.cpp,
// tests/test_gen4_staged_pokemon_editor.cpp and tests/test_gen5_staged_save_candidate.cpp.
// All data is synthetic.
#undef NDEBUG
#include "Integration/Gen4/Gen4AbilityChoice.h"
#include "Encryption/Encryption3FRLG.h"
#include "Encryption/Encryption4.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen5/Gen5EditorFields.h"
#include "Integration/Gen5/Gen5SharedEditorBridge.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Pokemon/PersonalInfo4DP.h"
#include "Pokemon/PersonalInfo4HGSS.h"
#include "Pokemon/PersonalInfo4PT.h"
#include "Pokemon/PersonalInfoTable.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "UI/Gen4SharedPokemonSession.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr uint16_t kGeodude = 74; // two native abilities and a 50% gender ratio in Gen III-V
constexpr std::array<uint8_t, 6> kNewIvs{{31, 0, 15, 7, 30, 1}};
constexpr std::array<uint8_t, 6> kNewEvs{{252, 0, 4, 0, 252, 0}};

// Stats whose EV goes down come first, so a stat-by-stat route never passes the 510 cap.
std::array<size_t, 6> evOrder(const std::array<uint8_t, 6>& current) {
    std::array<size_t, 6> order{};
    size_t n = 0;
    for (size_t i = 0; i < 6; ++i) if (kNewEvs[i] < current[i]) order[n++] = i;
    for (size_t i = 0; i < 6; ++i) if (kNewEvs[i] >= current[i]) order[n++] = i;
    return order;
}

bool endsWith(const std::string& text, std::string_view suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// ---------------------------------------------------------------- Generation III
namespace gen3 {

using namespace PokeVault::Integration::Gen3;
namespace S3 = PokeBank::UIModel::Gen3SharedEditor;

enum class Family { RS, Emerald, FRLG };
using LogicalSectors =
    std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

constexpr uint16_t kTid = 12345, kSid = 54321;

struct Mon3 {
    const char* name;
    uint32_t pid;
    bool abilityBit; // IV32 bit 31 (stored ability slot)
};

constexpr uint32_t shinyPid(uint16_t low) {
    return (static_cast<uint32_t>(kTid ^ kSid ^ low) << 16) | low;
}

// PID low bit 1 = stored ability bit; shiny; male (low byte 0xF1 >= ratio 127).
constexpr Mon3 kShiny{"shiny", shinyPid(0x00F1), true};
// PID low bit 0 = stored ability bit; not shiny.
constexpr Mon3 kPlain{"plain", 0x5A5A00F0u, false};
// Unusual: stored ability bit 1 disagrees with the PID low bit 0.
constexpr Mon3 kAbilityMismatch{"ability-bit/PID mismatch", 0x5A5A00F0u, true};

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}

uint8_t origin(SourceGame game) {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}

std::array<uint8_t, 80> pk3(SourceGame game, const Mon3& m) {
    std::array<uint8_t, 80> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(m.pid);
    p.setTID16(kTid);
    p.setSID16(kSid);
    p.setSpecies(kGeodude);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"ROCKY");
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setLevel(20);
    p.setMove(0, 33);
    p.setMovePP(0, 35);
    for (int i = 0; i < 6; ++i) {
        p.setIV(i, static_cast<uint8_t>(10 + i));
        p.setEV(i, static_cast<uint8_t>(20 + i));
    }
    // The stored ability bit is written raw so a PID/bit disagreement can be expressed.
    const uint32_t iv = p.iv32();
    p.wr32(0x48, m.abilityBit ? (iv | 0x80000000u) : (iv & ~0x80000000u));
    p.refreshChecksum();
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::array<uint8_t, 80> out{};
    std::memcpy(out.data(), encrypted, out.size());
    delete[] encrypted;
    return out;
}

LogicalSectors makeLogical(Family family, SourceGame game, const Mon3& m) {
    LogicalSectors logical{};
    if (family == Family::FRLG) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, key);
        write32(logical[1], 0x0290, key);
    } else if (family == Family::Emerald) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        write32(logical[1], 0x0490, key);
    } else {
        write32(logical[0], 0xAC, 0);
        write32(logical[1], 0x0490, 0);
    }
    write16(logical[0], 0x0A, kTid);
    write16(logical[0], 0x0C, kSid);
    const auto mon = pk3(game, m);
    const std::size_t logicalOffset = 4; // Box 1 slot 1 (PC buffer after the current-box u32)
    for (std::size_t i = 0; i < mon.size(); ++i) {
        const std::size_t position = logicalOffset + i;
        logical[5 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] = mon[i];
    }
    return logical;
}

void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation,
               Family family, SourceGame game, const Mon3& m) {
    auto logical = makeLogical(family, game, m);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(),
                  save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}

// Two valid save slots; slot 1 (counter 9) is the active one. Party count is zero.
std::vector<uint8_t> fixture(Family family, SourceGame game, const Mon3& m) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game, m);
    writeSlot(save, 1, 9, 5, family, game, m);
    return save;
}

// Same record as a 100-byte Party record (slot 1) of the active save slot, as the Gen III
// editor test builds it: native stats recalculated, current HP below max.
std::vector<uint8_t> fixtureWithParty(Family family, SourceGame game, const Mon3& m) {
    auto source = fixture(family, game, m);
    const auto selected = Detail::validateSlot(source, 1);
    assert(selected.valid);
    const auto sector = selected.logicalSectorOffsets[1];
    const auto body = sector + Detail::partyDataOffset(game);
    source[sector + Detail::partyCountOffset(game)] = 1;
    const auto boxed = pk3(game, m);
    std::copy(boxed.begin(), boxed.end(), source.begin() + static_cast<std::ptrdiff_t>(body));
    Pokemon::Pokemon3FRLG native(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(source.data() + body), 100));
    native.recalculateStats();
    native.setStatHPCurrent(static_cast<uint16_t>(native.statHPMax() - 3));
    auto* encoded = Encryption::encryptArray3FRLG(native.getData());
    std::memcpy(source.data() + body, encoded, 100);
    delete[] encoded;
    write16(source, sector + 0xFF6, Detail::sectorChecksum(
        std::span<const uint8_t>(source).subspan(sector, Detail::kChunkLengths[1])));
    return source;
}

struct RawBits { uint32_t pid; bool abilityBit; bool egg; };
RawBits rawBits(const std::array<uint8_t, 80>& encrypted) {
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(encrypted.data()), encrypted.size()));
    return {p.pid(), (p.iv32() >> 31) != 0, p.isEgg()};
}

void assertSameIdentity(const StagedPokemonRecord& a, const StagedPokemonRecord& b) {
    assert(a.pid == b.pid);
    assert(a.shiny == b.shiny);
    assert(a.gender == b.gender);
    assert(a.nature == b.nature);
    assert(a.abilityNumber == b.abilityNumber);
    assert(a.ability == b.ability);
    assert(a.tid == b.tid && a.sid == b.sid && a.species == b.species);
    const auto ra = rawBits(a.encryptedBytes), rb = rawBits(b.encryptedBytes);
    assert(ra.pid == rb.pid && ra.abilityBit == rb.abilityBit && ra.egg == rb.egg);
}

std::optional<StagedPokemonRecord> read(const StagedPokemonEditor& editor, bool party,
                                        std::string& error) {
    return party ? editor.partyPokemon(0, error) : editor.boxedPokemon(0, 0, error);
}

void route(SourceGame game, Family family, const Mon3& m, bool party) {
    const auto source = party ? fixtureWithParty(family, game, m) : fixture(family, game, m);
    const auto immutable = source;
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    assert(editor && error.empty());
    const auto before = read(*editor, party, error);
    assert(before && error.empty());
    // Fixture identity, so the assertions below compare against a known record.
    assert(before->species == kGeodude && before->pid == m.pid);
    assert(before->abilityNumber == (m.abilityBit ? 2 : 1));
    assert(rawBits(before->encryptedBytes).abilityBit == m.abilityBit);
    assert(before->shiny == (&m == &kShiny));
    assert(before->nature == m.pid % 25);

    // (1, 4) IV + EV edit through the shared session: preview, adopt, Keep.
    {
        S3::Session draft;
        draft.begin(*before, S3::Mode::Edit);
        draft.working.ivs = kNewIvs;
        draft.working.evs = kNewEvs;
        assert(draft.dirty());
        const auto request = draft.editRequest();
        assert(request.ivs && *request.ivs == kNewIvs && request.evs && *request.evs == kNewEvs);
        assert(!request.nature && !request.gender && !request.shiny && !request.abilityNumber &&
               !request.form && !request.species && !request.tid);
        if (!party) {
            const auto preview = editor->previewEdit(0, 0, request, error);
            assert(preview && error.empty());
            assertSameIdentity(*preview, *before);
            draft.adoptPreview(*preview);
            assert(draft.working.nature == before->nature && draft.working.gender == before->gender &&
                   draft.working.shiny == before->shiny &&
                   draft.working.abilityNumber == before->abilityNumber);
            assert(editor->stagedBytes() == immutable); // preview stages nothing
        }
        assert(draft.keep(*editor, 0, 0, error, party));
        assert(error.empty() && draft.mode == S3::Mode::None);
        const auto after = read(*editor, party, error);
        assert(after && error.empty());
        assertSameIdentity(*after, *before);
        assert(after->ivs == kNewIvs && after->evs == kNewEvs);
        // Review lists only the IV / EV rows; no PID, Shiny, Gender, Nature or Ability row.
        assert(editor->hasPendingChanges());
        for (const auto& change : editor->pendingChanges())
            assert(endsWith(change.label, " IV") || endsWith(change.label, " EV"));

        // (5) Serialize and strictly reparse; the source is untouched.
        const auto bytes = editor->finalizedBytes(error);
        assert(!bytes.empty() && error.empty() && bytes != immutable);
        auto reopened = StagedPokemonEditor::create(bytes, game, error);
        assert(reopened && error.empty());
        const auto persisted = read(*reopened, party, error);
        assert(persisted && error.empty());
        assertSameIdentity(*persisted, *before);
        assert(persisted->ivs == kNewIvs && persisted->evs == kNewEvs);
        assert(source == immutable && editor->originalBytes() == immutable);

        // (3) Discard restores the native bytes exactly and clears the pending list.
        editor->discard();
        assert(editor->stagedBytes() == immutable && !editor->hasPendingChanges());
        const auto restored = read(*editor, party, error);
        assert(restored && restored->encryptedBytes == before->encryptedBytes);
    }

    // (3) Back on a dirty draft asks first; Discard drops the draft and stages nothing.
    {
        S3::Session draft;
        draft.begin(*before, S3::Mode::Edit);
        draft.working.ivs[0] = static_cast<uint8_t>(draft.working.ivs[0] ^ 1);
        assert(draft.dirty() && !draft.back() && draft.confirmExit);
        draft.discardDraft();
        assert(!draft.dirty() && draft.mode == S3::Mode::None);
        assert(editor->stagedBytes() == immutable && !editor->hasPendingChanges());
        // Changing an IV and setting it back is not a change either.
        draft.begin(*before, S3::Mode::Edit);
        draft.working.ivs[2] = 31;
        draft.working.ivs[2] = before->ivs[2];
        assert(!draft.dirty());
    }

    // (2) Re-choosing every current value: no request, Keep stages nothing.
    {
        S3::Session draft;
        draft.begin(*before, S3::Mode::Edit);
        assert(draft.setNature(before->nature));
        assert(draft.setGender(before->gender));
        assert(draft.setShiny(before->shiny));
        assert(draft.setAbilityNumber(before->abilityNumber));
        draft.working.ivs = before->ivs;
        draft.working.evs = before->evs;
        assert(!draft.dirty());
        const auto request = draft.editRequest();
        assert(!request.nature && !request.gender && !request.shiny && !request.abilityNumber &&
               !request.ivs && !request.evs);
        assert(draft.keep(*editor, 0, 0, error, party) && error.empty());
        assert(editor->stagedBytes() == immutable && !editor->hasPendingChanges());
    }
    // (2) The backend given the unchanged IV / EV arrays is a no-op as well.
    {
        BoxPokemonEdit same;
        same.ivs = before->ivs;
        same.evs = before->evs;
        assert(party ? editor->stagePartyPokemonEdit(0, same, error)
                     : editor->stageBoxPokemonEdit(0, 0, same, error));
        assert(editor->stagedBytes() == immutable && !editor->hasPendingChanges());
    }
    assert(source == immutable);
}

void run() {
    const auto& personal = Pokemon::getPersonalInfoG3(kGeodude);
    assert(personal.ability1 != 0 && personal.ability2 != 0 && personal.ability1 != personal.ability2);
    const std::array<std::pair<SourceGame, Family>, 5> games{{
        {SourceGame::RubyGBA, Family::RS}, {SourceGame::SapphireGBA, Family::RS},
        {SourceGame::EmeraldGBA, Family::Emerald}, {SourceGame::FireRedGBA, Family::FRLG},
        {SourceGame::LeafGreenGBA, Family::FRLG}}};
    for (const auto& [game, family] : games)
        for (const Mon3* m : {&kShiny, &kPlain, &kAbilityMismatch})
            for (const bool party : {false, true}) route(game, family, *m, party);
}

} // namespace gen3

// ---------------------------------------------------------------- Generation IV
namespace gen4 {

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;
namespace S4 = PokeBank::UIModel::Gen4SharedEditor;

constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC = 0x20060623;
constexpr uint16_t kTid = 12345, kSid = 54321;

struct Spec {
    size_t generalSize, storageStart, storageSize, footerSize, trainer, party, boxData, boxStride;
};
Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
}
std::string gameId(Layout layout) {
    if (layout == Layout::DiamondPearl) return "diamond_nds";
    if (layout == Layout::Platinum) return "platinum_nds";
    return "heartgold_nds";
}
const Pokemon::PersonalRecord& personal(Layout layout) {
    if (layout == Layout::DiamondPearl) return Pokemon::getPersonalInfo4DP(kGeodude, 0);
    if (layout == Layout::Platinum) return Pokemon::getPersonalInfo4PT(kGeodude, 0);
    return Pokemon::getPersonalInfo4HGSS(kGeodude, 0);
}

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    w16(b, o, static_cast<uint16_t>(v));
    w16(b, o + 2, static_cast<uint16_t>(v >> 16));
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    wb16(b, o, static_cast<uint16_t>(v));
    wb16(b, o + 2, static_cast<uint16_t>(v >> 16));
}
void copy(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i) dst[off + i] = static_cast<uint8_t>(src[i]);
}
void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(), dst.begin() + static_cast<std::ptrdiff_t>(off));
}

struct Mon4 {
    const char* name;
    uint32_t pid;
    uint8_t storedGender;  // 0x40 bits 1-2
    uint8_t abilitySlot;   // which personal slot the stored ability id (0x15) names
};
constexpr uint32_t shinyPid(uint16_t low) {
    return (static_cast<uint32_t>(kTid ^ kSid ^ low) << 16) | low;
}
// Ratio 127: low byte 0xF1 / 0xF0 reads male, 0x10 reads female. PID bit 0 is the ability slot.
constexpr Mon4 kShiny{"shiny", shinyPid(0x00F1), 0, 1};
constexpr Mon4 kPlain{"plain", 0x5A5A00F0u, 0, 0};
constexpr Mon4 kGenderMismatch{"stored gender/PID mismatch", 0x5A5A0010u, 0, 0};
constexpr Mon4 kAbilityMismatch{"stored ability/PID mismatch", 0x5A5A00F0u, 0, 1};

std::vector<std::byte> pk4(Layout layout, const Mon4& m) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, m.pid);
    wb16(d, 0x08, kGeodude);
    wb16(d, 0x0C, kTid);
    wb16(d, 0x0E, kSid);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    const auto& p = personal(layout);
    d[0x15] = static_cast<std::byte>(m.abilitySlot ? p.ability2 : p.ability1);
    d[0x17] = std::byte{2};
    for (size_t i = 0; i < 6; ++i) d[0x18 + i] = static_cast<std::byte>(20 + i);
    uint32_t iv32 = 0;
    for (uint32_t i = 0; i < 6; ++i) iv32 |= (10u + i) << (i * 5);
    wb32(d, 0x38, iv32);
    d[0x40] = static_cast<std::byte>(m.storedGender << 1);
    copy(d, 0x48, Utils::encodeGen4Field(u"ROCKY", 11, 10, 2));
    copy(d, 0x68, Utils::encodeGen4Field(u"ASH", 8, 7, 2));
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{20};
    d[0x86] = std::byte{17};
    return Encryption::encryptArray4(d);
}

void stamp(std::vector<uint8_t>& save, size_t off, size_t len, size_t footer, uint32_t major) {
    const size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, MAGIC);
    w16(save, end - 2, Utils::crc16ccitt(save.data() + off, len - footer));
}

// Both partitions valid, partition 0 newer; the record is Box 1 slot 1, the Party is empty.
std::vector<uint8_t> makeSave(Layout layout, const Mon4& m) {
    const auto sp = spec(layout);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = pk4(layout, m);
    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(storage),
                  save.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);
        copy(save, base + sp.trainer, Utils::encodeGen4Field(u"Kylie", 8, 7, 2));
        w16(save, base + sp.trainer + 0x10, kTid);
        w16(save, base + sp.trainer + 0x12, kSid);
        save[base + sp.trainer + 0x18] = 1;
        save[base + sp.trainer + 0x19] = 2;
        save[base + sp.trainer + 0x1C] = 7;
        for (size_t box = 0; box < 18; ++box) {
            const size_t boxBase = storage + sp.boxData + box * sp.boxStride;
            for (size_t slot = 0; slot < 30; ++slot)
                copy(save, boxBase + slot * Encryption::SIZE_STORED4, blank);
            if (layout == Layout::HeartGoldSoulSilver)
                std::fill(save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0xFF0),
                          save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0x1000),
                          static_cast<uint8_t>(0xC0 + box));
        }
        copy(save, storage + sp.boxData, occupied);
        if (layout == Layout::HeartGoldSoulSilver) save[storage + 0x12000] = 0;
        else save[storage] = 0;
        stamp(save, base, sp.generalSize, sp.footerSize, partition == 0 ? 20 : 10);
        stamp(save, storage, sp.storageSize, sp.footerSize, partition == 0 ? 30 : 15);
    }
    return save;
}

void assertSameIdentity(const Pokemon::Pokemon4ReadOnly& a, const Pokemon::Pokemon4ReadOnly& b) {
    assert(a.pid() == b.pid());
    assert(a.gender() == b.gender()); // stored gender bits
    assert(a.ability() == b.ability());
    assert(a.tid() == b.tid() && a.sid() == b.sid() && a.species() == b.species());
    assert((a.iv32() >> 30) == (b.iv32() >> 30)); // Egg / nicknamed flags
    const Pokemon::Pokemon4ReadOnlyView av(a), bv(b);
    assert(av.nature() == bv.nature());
    assert(av.isShiny(a.id32(), {}) == bv.isShiny(b.id32(), {}));
}

void route(Layout layout, const Mon4& m) {
    const auto source = makeSave(layout, m);
    const auto immutable = source;
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, layout, gameId(layout), &error);
    assert(editor && error.empty() && !editor->hasChanges());
    const auto before = editor->boxedPokemon(0, 0, &error);
    assert(before && error.empty());
    {
        // Fixture identity.
        const auto mutableView = Pokemon::Pokemon4Mutable::fromEncrypted(
            before->originalEncryptedBytes(), editor->sourceGroup(), &error);
        assert(mutableView);
        assert(before->pid() == m.pid && before->gender() == m.storedGender);
        assert(mutableView->abilitySlot() == (m.pid & 1u));
        assert(before->ability() == mutableView->abilityForSlot(m.abilitySlot));
        assert(mutableView->shiny() == (&m == &kShiny));
    }

    // (1, 4) IV + EV edit through the shared session and Keep.
    {
        S4::Session draft;
        assert(draft.begin(*before, editor->sourceGroup(), S4::Mode::Edit, error));
        for (size_t i = 0; i < 6; ++i) assert(draft.working->setIV(i, kNewIvs[i]));
        // One stat at a time, lowering first, as the EV prompt does (510 total at each step).
        for (const size_t i : evOrder(draft.working->evs())) assert(draft.working->setEV(i, kNewEvs[i]));
        assert(draft.dirty());
        assert(draft.working->pid() == m.pid && draft.working->gender() == m.storedGender);
        assert(draft.keep(*editor, 0, 0, error) && error.empty());
        assert(!draft.editable() && editor->hasChanges());
        const auto after = editor->boxedPokemon(0, 0, &error);
        assert(after && error.empty());
        assertSameIdentity(*after, *before);
        assert(after->ivs() == kNewIvs && after->evs() == kNewEvs);

        // Review lists the slot line and IV / EV rows only.
        const auto rows = PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor);
        assert(!rows.empty());
        size_t fieldRows = 0;
        for (const auto& [label, value] : rows) {
            if (label == "Staged change") continue;
            ++fieldRows;
            assert(label.find(" / IV ") != std::string::npos ||
                   label.find(" / EV ") != std::string::npos);
        }
        assert(fieldRows > 0);

        // (5) Serialize and strictly reparse; the source is untouched.
        const auto bytes = editor->finalizedBytes(&error);
        assert(!bytes.empty() && error.empty() && bytes != immutable);
        const auto parsed = Gen4ReadOnlySave::parse(bytes, layout, gameId(layout), &error);
        assert(parsed && error.empty());
        assertSameIdentity(parsed->box(0, 0), *before);
        assert(parsed->box(0, 0).ivs() == kNewIvs && parsed->box(0, 0).evs() == kNewEvs);
        assert(source == immutable && editor->originalBytes() == immutable);

        // (3) Discard restores the native bytes exactly; Review is empty again.
        editor->discard();
        assert(!editor->hasChanges() && editor->stagedBytes() == immutable);
        assert(editor->finalizedBytes(&error) == immutable);
        assert(PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor).empty());
    }

    // (3) Back on a dirty draft asks first; Discard stages nothing.
    {
        S4::Session draft;
        assert(draft.begin(*before, editor->sourceGroup(), S4::Mode::Edit, error));
        assert(draft.working->setEV(0, 200));
        assert(draft.dirty() && !draft.back() && draft.confirmExit);
        draft.discardDraft();
        assert(!draft.editable() && !draft.dirty());
        assert(!editor->hasChanges() && editor->stagedBytes() == immutable);
    }

    // (2) Re-choosing the current IVs, EVs, Nature, Gender and Shiny changes no byte.
    {
        S4::Session draft;
        assert(draft.begin(*before, editor->sourceGroup(), S4::Mode::Edit, error));
        const auto ivs = draft.working->ivs(), evs = draft.working->evs();
        for (size_t i = 0; i < 6; ++i) assert(draft.working->setIV(i, ivs[i]));
        for (size_t i = 0; i < 6; ++i) assert(draft.working->setEV(i, evs[i]));
        assert(draft.working->setNature(draft.working->nature()));
        assert(draft.working->setGender(draft.working->gender()));
        assert(draft.working->setShiny(draft.working->shiny()));
        assert(draft.working->pid() == m.pid);
        assert(!draft.dirty());
        // An IV changed and set back is not a change.
        assert(draft.working->setIV(3, ivs[3] == 31 ? 0 : 31) && draft.dirty());
        assert(draft.working->setIV(3, ivs[3]) && !draft.dirty());
        assert(draft.keep(*editor, 0, 0, error) && error.empty());
        assert(!editor->hasChanges() && editor->stagedBytes() == immutable);
    }
    assert(source == immutable);
}

void run() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        const auto& p = personal(layout);
        assert(p.ability1 != 0 && p.ability2 != 0 && p.ability1 != p.ability2 && p.genderRatio == 127);
        for (const Mon4* m : {&kShiny, &kPlain, &kGenderMismatch, &kAbilityMismatch})
            route(layout, *m);
    }
}

} // namespace gen4

// ---------------------------------------------------------------- Generation V
namespace gen5 {

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
using Field = G::StagedPokemon5Record::Field;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Identity = PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;

constexpr uint16_t kTid = 4321, kSid = 1234;

struct Mon5 {
    const char* name;
    uint32_t pid;
    uint8_t storedGender;  // 0x40 bits 1-2
    uint8_t abilitySlot;   // which personal slot the stored ability id (0x15) names
    uint8_t nature;        // 0x41 (Gen V stores nature apart from the PID)
};
constexpr uint32_t shinyPid(uint16_t low) {
    return (static_cast<uint32_t>(kTid ^ kSid ^ low) << 16) | low;
}
// Ratio 127: low byte 0xF0 reads male. PID bit 16 is the ability slot.
constexpr Mon5 kShiny{"shiny", shinyPid(0x00F0), 0, (shinyPid(0x00F0) >> 16) & 1u, 7};
constexpr Mon5 kPlain{"plain", 0x5A5A00F0u, 0, 0, 11};
// Unusual: stored female while the PID low byte reads male (the PKSE 620c5f6 case).
constexpr Mon5 kGenderMismatch{"stored gender/PID mismatch", 0x5A5A00F0u, 1, 0, 3};
// Unusual: stored ability is slot 2 while PID bit 16 selects slot 1.
constexpr Mon5 kAbilityMismatch{"stored ability/PID mismatch", 0x5A5A00F0u, 0, 1, 19};

void write32(std::span<uint8_t> s, size_t o, uint32_t v) {
    C::write16(s, o, static_cast<uint16_t>(v));
    C::write16(s, o + 2, static_cast<uint16_t>(v >> 16));
}

std::vector<uint8_t> pk5(size_t size, const Mon5& m, bool b2w2, uint8_t originVersion) {
    const auto personal = G::personal5(kGeodude, 0, b2w2);
    assert(personal);
    std::vector<uint8_t> d(size, 0);
    write32(d, 0, m.pid);
    C::write16(d, 0x08, kGeodude);
    C::write16(d, 0x0C, kTid);
    C::write16(d, 0x0E, kSid);
    write32(d, 0x10, G::experienceForLevel5(personal.info->growthRate, 20));
    d[0x14] = 70;
    d[0x15] = G::abilityForSlot5(*personal.info, m.abilitySlot);
    d[0x17] = 2;
    for (size_t i = 0; i < 6; ++i) d[0x18 + i] = static_cast<uint8_t>(20 + i);
    uint32_t iv32 = 0;
    for (uint32_t i = 0; i < 6; ++i) iv32 |= (10u + i) << (i * 5);
    write32(d, 0x38, iv32);
    d[0x40] = static_cast<uint8_t>(m.storedGender << 1);
    d[0x41] = m.nature;
    d[0x5F] = originVersion;
    d[0x84] = 20;
    if (size == C::PartySize) {
        // Native party cache: level and the stats the game computes for it, 5 HP of damage.
        d[0x8C] = 20;
        const std::array<uint8_t, 6> evs{{d[0x18], d[0x19], d[0x1A], d[0x1B], d[0x1C], d[0x1D]}};
        const auto stats = G::computeStats5(kGeodude, 0, 20, m.nature, iv32, evs, b2w2);
        assert(stats);
        for (size_t i = 0; i < 6; ++i) C::write16(d, 0x90 + 2 * i, (*stats)[i]);
        C::write16(d, 0x8E, static_cast<uint16_t>((*stats)[0] - 5));
    }
    return C::encryptCandidate(d);
}

// Party slot 1 and Box 1 slot 1 hold the same identity; checksums are stamped for the copy.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version, bool backup, const Mon5& m) {
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const size_t copySize = b2w2 ? L::Black2White2CopySize : L::BlackWhiteCopySize;
    const size_t base = backup ? copySize : 0;
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    sav[base + 0x19400 + 0x1F] = version;
    sav[base + L::PartyOffset + 4] = 1;
    const auto party = pk5(C::PartySize, m, b2w2, version);
    std::copy(party.begin(), party.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    const auto box = pk5(C::StoredSize, m, b2w2, version);
    std::copy(box.begin(), box.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset));
    const auto blocks = b2w2 ? std::span<const L::SaveBlock>(L::Black2White2)
                             : std::span<const L::SaveBlock>(L::BlackWhite);
    auto copy = std::span<uint8_t>(sav).subspan(base, copySize);
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

void assertSameIdentity(const G::Pokemon5ReadOnly& a, const G::Pokemon5ReadOnly& b) {
    assert(a.pid() == b.pid());
    assert(a.shiny() == b.shiny());
    assert(a.gender() == b.gender()); // stored gender bits
    assert(a.nature() == b.nature());
    assert(a.ability() == b.ability() && a.hiddenAbility() == b.hiddenAbility());
    assert(a.tid() == b.tid() && a.sid() == b.sid() && a.species() == b.species());
    const auto da = a.decodedBytes(), db = b.decodedBytes();
    assert(da[0x40] == db[0x40] && da[0x42] == db[0x42]); // gender/form/fateful, hidden flag
    assert((C::read32(da, 0x38) >> 30) == (C::read32(db, 0x38) >> 30)); // Egg / nicknamed
}

std::optional<G::Pokemon5ReadOnly> view(const G::Gen5StagedPokemonWorkspace& ws, Region region) {
    return region == Region::Party ? ws.viewParty(0) : ws.viewBox(0, 0);
}
std::optional<G::Pokemon5ReadOnly> view(const G::Gen5ReadOnlySave& save, Region region) {
    return region == Region::Party ? save.partyPokemon(0) : save.boxPokemon(0, 0);
}
bool sameBytes(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

void route(G::SaveFamily family, std::string_view game, uint8_t version, bool backup,
           const Mon5& m) {
    const auto input = fixture(family, version, backup, m);
    const auto immutable = input;
    const auto selected = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, game, &error, selected);
    assert(parsed && error.empty());
    const bool b2w2 = family == G::SaveFamily::Black2White2;
    const auto personal = G::personal5(kGeodude, 0, b2w2);
    assert(personal && personal.info->genderRatio == 127 &&
           personal.info->ability1 != personal.info->ability2);

    for (const Region region : {Region::Party, Region::Box}) {
        const G::Gen5StagedPokemonWorkspace::Slot location{region, 0, 0};
        const auto source = view(*parsed, region);
        assert(source && source->valid() && !source->empty());
        // Fixture identity.
        assert(source->pid() == m.pid && source->gender() == m.storedGender &&
               source->nature() == m.nature && !source->hiddenAbility());
        assert(source->ability() == G::abilityForSlot5(*personal.info, m.abilitySlot));
        assert(source->shiny() == (&m == &kShiny));
        const bool genderLinked =
            G::genderFromPid5(source->pid(), personal.info->genderRatio) == source->gender();
        const bool abilityLinked =
            G::abilityForSlot5(*personal.info, G::abilityBit5(source->pid())) == source->ability();
        assert(genderLinked == (&m != &kGenderMismatch));
        assert(abilityLinked == (&m != &kAbilityMismatch));

        G::Gen5StagedPokemonWorkspace ws(*parsed);

        // (1, 4) IV + EV edit through the shared-editor bridge, then Keep.
        {
            G::Gen5SharedPokemonSession draft;
            assert(draft.begin(ws, location, G::Gen5SharedPokemonSession::Mode::Edit, error));
            for (size_t i = 0; i < 6; ++i)
                assert(G::stageSharedField(draft, Identity::IV, i, kNewIvs[i], error));
            for (const size_t i : evOrder(source->evs()))
                assert(G::stageSharedField(draft, Identity::EV, i, kNewEvs[i], error));
            assert(draft.dirty());
            assertSameIdentity(*draft.current(), *source);
            assert(draft.keep(ws, error) && error.empty());
            assert(draft.mode() == G::Gen5SharedPokemonSession::Mode::None && ws.hasChanges());
            const auto after = view(ws, region);
            assert(after && after->valid());
            assertSameIdentity(*after, *source);
            assert(after->ivs() == kNewIvs && after->evs() == kNewEvs);
        }

        // Review: the independent replay accepts exactly IV / EV field changes.
        const auto pending = ws.pendingReview();
        assert(pending.size() == 1);
        const auto verified = G::verifyStagedReview(pending[0], &error);
        assert(verified && error.empty() && !verified->fields.empty());
        for (const auto& field : verified->fields)
            assert(field.field == Field::IV || field.field == Field::EV);

        // (5) Staged save candidate, strictly reparsed; the source is untouched.
        {
            const auto candidate = G::buildStagedSaveCandidate(ws, &error);
            assert(candidate && error.empty() && candidate->changedRecords == 1);
            const auto reparsed = G::Gen5ReadOnlySave::parse(candidate->bytes, game, &error, selected);
            assert(reparsed && error.empty());
            const auto persisted = view(*reparsed, region);
            assert(persisted && persisted->valid() && !persisted->empty());
            assertSameIdentity(*persisted, *source);
            assert(persisted->ivs() == kNewIvs && persisted->evs() == kNewEvs);
            assert(input == immutable && sameBytes(ws.sourceSave().sourceBytes(), immutable));
        }

        // (3) Discard All restores the native bytes exactly and clears Review.
        ws.discardAll();
        assert(!ws.hasChanges() && ws.pendingReview().empty());
        assert(sameBytes(view(ws, region)->originalEncryptedBytes(), source->originalEncryptedBytes()));
        assert(!G::buildStagedSaveCandidate(ws, &error));

        // (3) Back on a dirty draft asks first; Discard stages nothing. A record-level rollback
        // returns the exact original bytes.
        {
            G::Gen5SharedPokemonSession draft;
            assert(draft.begin(ws, location, G::Gen5SharedPokemonSession::Mode::Edit, error));
            assert(G::stageSharedField(draft, Identity::IV, 1, 30, error));
            assert(draft.dirty() && !draft.back() && draft.confirmExit());
            draft.discardDraft();
            assert(draft.mode() == G::Gen5SharedPokemonSession::Mode::None && !ws.hasChanges());

            auto record = G::StagedPokemon5Record::create(source->originalEncryptedBytes(), &error, b2w2);
            assert(record);
            assert(record->stage(Field::IV, 0, 31, &error) && record->stage(Field::EV, 0, 200, &error));
            assert(record->dirty());
            record->rollback();
            assert(!record->dirty() && sameBytes(record->stagedBytes(), source->originalEncryptedBytes()));
        }

        // (2) Re-staging the current IV, EV, Nature, Gender and Shiny values is not an edit.
        {
            G::Gen5SharedPokemonSession draft;
            assert(draft.begin(ws, location, G::Gen5SharedPokemonSession::Mode::Edit, error));
            const auto ivs = source->ivs(), evs = source->evs();
            for (size_t i = 0; i < 6; ++i) assert(G::stageSharedField(draft, Identity::IV, i, ivs[i], error));
            for (size_t i = 0; i < 6; ++i) assert(G::stageSharedField(draft, Identity::EV, i, evs[i], error));
            assert(G::stageSharedField(draft, Identity::Nature, 0, source->nature(), error));
            assert(draft.stage(Field::Gender, 0, source->gender(), error));
            assert(draft.stage(Field::Shiny, 0, source->shiny() ? 1 : 0, error));
            assert(!draft.dirty());
            assert(sameBytes(draft.current()->originalEncryptedBytes(), source->originalEncryptedBytes()));
            assert(draft.keep(ws, error) && error.empty());
            assert(!ws.hasChanges() && ws.pendingReview().empty());
            // Workspace level: an unchanged value creates no pending record; a change staged
            // back to the original value removes it.
            const auto stage = [&](Field field, size_t stat, uint32_t value) {
                return region == Region::Party ? ws.stageParty(0, field, stat, value, &error)
                                               : ws.stageBox(0, 0, field, stat, value, &error);
            };
            assert(stage(Field::IV, 4, ivs[4]) && stage(Field::EV, 4, evs[4]) && !ws.hasChanges());
            assert(stage(Field::IV, 4, 0) && ws.hasChanges());
            assert(stage(Field::IV, 4, ivs[4]) && !ws.hasChanges() && ws.pendingReview().empty());
            assert(sameBytes(view(ws, region)->originalEncryptedBytes(), source->originalEncryptedBytes()));
        }
    }
    assert(input == immutable);
}

void run() {
    for (const Mon5* m : {&kShiny, &kPlain, &kGenderMismatch, &kAbilityMismatch}) {
        route(G::SaveFamily::BlackWhite, "white_nds", 20, false, *m);
        route(G::SaveFamily::BlackWhite, "black_nds", 21, true, *m);
        route(G::SaveFamily::Black2White2, "white2_nds", 22, false, *m);
        route(G::SaveFamily::Black2White2, "black2_nds", 23, true, *m);
    }
}

} // namespace gen5

// DEFECT probe, kept out of the passing routes above. On a record whose stored ability disagrees
// with its PID ability bit, the Gen IV and Gen V Ability rows show the stored ability, but the
// Ability picker opens on the PID-derived slot (src/UI/Gen4SharedPokemonSurface.inc:738;
// Gen5EditorFields.h abilityChoiceFor via src/UI/Gen5SharedReadOnlySurface.inc:583). Accepting that
// preselected row is applied with no prompt (the PID does not change) and rewrites the stored
// ability: Gen IV Pokemon4Mutable::setAbilitySlot (src/Pokemon/Pokemon4Mutable.cpp:948),
// Gen V StagedPokemon5Record::stage(Field::AbilitySlot) (include/Integration/Gen5/Gen5StagedPokemonRecord.h:180).
// Owner Oct 28 PID-route review: the Gen IV/V Ability row shows the STORED ability, but the picker used
// to open on the PID's slot, so pressing A on the preselected row rewrote a stored ability that
// disagrees with the PID (Sturdy -> Rock Head, draft dirty, no prompt). The picker now opens on the slot
// holding the stored ability (Gen4AbilityChoice.h / Gen5EditorFields abilityChoiceFor), and choosing the
// stored ability is a no-op in both surfaces (they skip the stage call when abilityChoiceKeepsStored).
void abilityPickerKeepsUnusualStoredAbility() {
    std::string error;
    for (const auto layout : {gen4::Layout::DiamondPearl, gen4::Layout::Platinum,
                              gen4::Layout::HeartGoldSoulSilver}) {
        const auto source = gen4::makeSave(layout, gen4::kAbilityMismatch);
        auto editor = gen4::Gen4StagedPokemonEditor::create(source, layout, gen4::gameId(layout), &error);
        assert(editor);
        const auto record = editor->boxedPokemon(0, 0, &error);
        assert(record && record->ability() == gen4::personal(layout).ability2);
        gen4::S4::Session draft;
        assert(draft.begin(*record, editor->sourceGroup(), gen4::S4::Mode::Edit, error));
        assert(draft.working->abilitySlot() == 0);                               // the PID's slot
        const uint8_t row = PokeVault::Integration::Gen4::abilityPickerRow(*draft.working);
        assert(row == 1);                                                         // the stored ability's slot
        assert(PokeVault::Integration::Gen4::abilityChoiceKeepsStored(*draft.working, row));
        assert(!PokeVault::Integration::Gen4::abilityChoiceKeepsStored(*draft.working, 0));
        assert(draft.working->ability() == record->ability() && !draft.dirty());   // nothing staged
        assert(draft.working->pid() == gen4::kAbilityMismatch.pid);
        // An ordinary record: the PID slot holds the stored ability, so the row is the PID slot.
        const auto plainSource = gen4::makeSave(layout, gen4::kPlain);
        auto plainEditor = gen4::Gen4StagedPokemonEditor::create(plainSource, layout, gen4::gameId(layout), &error);
        const auto plain = plainEditor->boxedPokemon(0, 0, &error);
        gen4::S4::Session plainDraft;
        assert(plainDraft.begin(*plain, plainEditor->sourceGroup(), gen4::S4::Mode::Edit, error));
        assert(PokeVault::Integration::Gen4::abilityPickerRow(*plainDraft.working) == plainDraft.working->abilitySlot());
    }
    for (const bool b2w2 : {false, true}) {
        const auto family = b2w2 ? gen5::G::SaveFamily::Black2White2 : gen5::G::SaveFamily::BlackWhite;
        const std::string_view game = b2w2 ? "white2_nds" : "white_nds";
        const auto input = gen5::fixture(family, b2w2 ? 22 : 20, false, gen5::kAbilityMismatch);
        const auto parsed = gen5::G::Gen5ReadOnlySave::parse(input, game, &error,
                                                            gen5::G::SaveCopySelection::Primary);
        assert(parsed);
        gen5::G::Gen5StagedPokemonWorkspace ws(*parsed);
        for (const auto region : {gen5::Region::Party, gen5::Region::Box}) {
            gen5::G::Gen5SharedPokemonSession draft;
            assert(draft.begin(ws, {region, 0, 0}, gen5::G::Gen5SharedPokemonSession::Mode::Edit, error));
            const auto shown = draft.current();
            const auto row = gen5::G::EditorFields::abilityChoiceFor(*shown, b2w2);
            assert(row == 1);                                                     // the stored ability's slot
            assert(gen5::G::EditorFields::abilityChoiceKeepsStored(*shown, b2w2, row));
            assert(!gen5::G::EditorFields::abilityChoiceKeepsStored(*shown, b2w2, 0));
            assert(!draft.dirty() && draft.current()->ability() == shown->ability());
            draft.discardDraft();
        }
    }
}

} // namespace

int main() {
    gen3::run();
    gen4::run();
    gen5::run();
    abilityPickerKeepsUnusualStoredAbility();
    std::cout << "Gen III/IV/V PID, shiny, gender, nature and ability preservation routes: PASS\n";
    return 0;
}
