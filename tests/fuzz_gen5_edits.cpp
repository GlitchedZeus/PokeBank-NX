// QA fuzz target: Gen V staged edit SEQUENCES through the real workspace API
// (Gen5StagedPokemonWorkspace: stageParty/stageBox for every field, name edits, clone, remove),
// then the export candidate. Phase 6 audit of the PID-coordinated edits (Gen5PidCoordination.h,
// Gen5StagedPokemonRecord.h) and of the edit -> export path.
//
// Input = an operation tape applied to a synthetic BW/B2W2 save whose party/boxes hold: a Gen V
// origin Pikachu (variable gender ratio 127) whose stored gender matches its PID, a Gen IV origin
// Eevee (ratio 31) whose PID is fixed by its origin, a Gen V origin Eevee in a box, and retail
// encrypted-blank box slots.
//
// Invariants on every operation:
//  * a refused operation changes nothing (target view bytes and changed-record count identical);
//  * an accepted operation leaves the target a valid, occupied PK5;
//  * a Gen V origin record whose PID-derived gender matched its stored gender still matches;
//  * a Gen III/IV origin record's PID never changes (gender/ability/nature are fixed by it);
//  * the PID changes only for the PID-coordinated fields (Gender, Shiny, AbilitySlot, Personality);
//  * shininess changes only through Shiny/Personality/TrainerId/SecretId edits;
// and at the end the export candidate either refuses or re-parses as the same game and copy with
// every staged slot byte-identical to the workspace view, outside-slot bytes limited to the copy.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"

#include <map>

namespace G5 = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
using Field = G5::StagedPokemon5Record::Field;
using Region = G5::Gen5StagedPokemonWorkspace::Region;
using Slot = G5::Gen5StagedPokemonWorkspace::Slot;

namespace {

constexpr uint16_t kTid = 12345, kSid = 54321;

qafuzz::Bytes plainRecord(bool party, uint32_t pid, uint16_t species, uint8_t origin, uint8_t gender) {
    qafuzz::Bytes r(party ? C::PartySize : C::StoredSize, 0);
    qasynth::w32(r, 0, pid);
    qasynth::w16(r, 0x08, species);
    qasynth::w16(r, 0x0C, kTid);
    qasynth::w16(r, 0x0E, kSid);
    qasynth::w32(r, 0x10, 1000);
    r[0x14] = 70;
    // Retail-consistent ability: the species' ability for the PID's Gen V ability slot (bit 16).
    // (An inconsistent ability byte disables the PID gender-link guard: QA-F4.)
    const auto personal = G5::personal5(species, 0, false);
    r[0x15] = personal.info ? G5::abilityForSlot5(*personal.info, (pid >> 16) & 1u) : 0;
    r[0x17] = 2;
    r[0x40] = static_cast<uint8_t>(gender << 1);
    r[0x41] = 3;
    r[0x5F] = origin;
    r[0x83] = 4;
    r[0x84] = 0x80 | 5;
    if (party) r[0x8C] = 10;
    return r;
}

// Seed save: family/version from the tape's first byte, party of two, a boxed Gen V origin Eevee,
// the rest of box 0 encrypted retail blanks (Gen5BlankSlot.h).
qafuzz::Bytes richSave(G5::SaveFamily family, uint8_t version) {
    const size_t copy = qasynth::gen5::copySize(family);
    auto sav = qasynth::gen5::save(family, version, 1);
    const auto put = [&](size_t at, const qafuzz::Bytes& plain) {
        const auto enc = C::encryptCandidate(plain);
        std::copy(enc.begin(), enc.end(), sav.begin() + static_cast<std::ptrdiff_t>(at));
    };
    sav[L::PartyOffset + 4] = 2;
    // Pikachu ratio 127: PID low byte 0x80 -> male (0); stored gender 0 matches.
    put(L::PartyOffset + 8, plainRecord(true, 0x2A3B0080u, 25, version, 0));
    // Gen IV (Diamond = 10) origin Eevee, ratio 31, low byte 0x10 -> female (1).
    put(L::PartyOffset + 8 + C::PartySize, plainRecord(true, 0x77770010u, 133, 10, 1));
    put(L::BoxOffset, plainRecord(false, 0x11220090u, 133, version, 0));
    const auto& blank = G5::encryptedBlankBoxSlot5();
    for (size_t s = 1; s < L::BoxSlots; ++s)
        std::copy(blank.begin(), blank.end(),
                  sav.begin() + static_cast<std::ptrdiff_t>(L::BoxOffset + s * C::StoredSize));
    qasynth::gen5::stamp(std::span<uint8_t>(sav).subspan(0, copy), qasynth::gen5::blocks(family));
    return sav;
}

const qafuzz::Bytes& seedFor(uint8_t selector) {
    static const std::array<qafuzz::Bytes, 4> seeds{
        richSave(G5::SaveFamily::BlackWhite, 21), richSave(G5::SaveFamily::BlackWhite, 20),
        richSave(G5::SaveFamily::Black2White2, 23), richSave(G5::SaveFamily::Black2White2, 22)};
    return seeds[selector % 4];
}

std::optional<G5::Pokemon5ReadOnly> viewOf(const G5::Gen5StagedPokemonWorkspace& ws, const Slot& s) {
    return s.region == Region::Party ? ws.viewParty(s.slot) : ws.viewBox(s.box, s.slot);
}

qafuzz::Bytes viewBytes(const G5::Gen5StagedPokemonWorkspace& ws, const Slot& s) {
    const auto v = viewOf(ws, s);
    if (!v) return {};
    const auto b = v->originalEncryptedBytes();
    return qafuzz::Bytes(b.begin(), b.end());
}

uint8_t genderRatio(const G5::Pokemon5ReadOnly& p, bool b2w2) {
    const auto personal = G5::personal5(p.species(), p.form(), b2w2);
    return personal.info ? personal.info->genderRatio : 255;
}

bool pidCoordinated(Field f) {
    return f == Field::Gender || f == Field::Shiny || f == Field::AbilitySlot || f == Field::Personality;
}
bool mayChangeShiny(Field f) {
    return f == Field::Shiny || f == Field::Personality || f == Field::TrainerId || f == Field::SecretId;
}

void runOne(std::span<const uint8_t> data) {
    if (data.empty()) return;
    const auto& sav = seedFor(data[0]);
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(sav, {}, &error, G5::SaveCopySelection::Primary);
    QA_CHECK(parsed.has_value());
    qafuzz::noteAccepted();
    G5::Gen5StagedPokemonWorkspace ws(*parsed);
    const bool b2w2 = ws.black2White2();
    std::map<std::pair<int, size_t>, bool> touched;

    auto tape = data.subspan(1);
    size_t ops = 0;
    while (tape.size() >= 8 && ops++ < 64) {
        const uint8_t kind = tape[0] % 8;
        const bool party = (tape[1] & 1) == 0;
        const size_t slot = party ? tape[2] % 7 : tape[2] % 31;  // includes one out-of-range slot
        const size_t box = party ? 0 : (tape[1] >> 1) % 2;
        const uint8_t rawField = tape[3] % 33;                    // > enum range = forged field
        const size_t stat = tape[4] % 8;
        const uint32_t value = static_cast<uint32_t>(tape[5]) | (static_cast<uint32_t>(tape[6]) << 8) |
                               (static_cast<uint32_t>(tape[7] & 0x0F) << 16);
        tape = tape.subspan(8);
        // Clone and remove always act on a box slot (the native APIs take box coordinates).
        const bool boxOnly = kind == 6 || kind == 7;
        const Slot target{party && !boxOnly ? Region::Party : Region::Box, box, slot};
        const auto before = viewBytes(ws, target);
        const auto beforeView = viewOf(ws, target);
        const size_t beforeCount = ws.changedRecordCount();
        bool ok = false;
        Field field = static_cast<Field>(rawField);
        switch (kind) {
            case 0: case 1: case 2: case 3: case 4:
                ok = party ? ws.stageParty(slot, field, stat, value, &error)
                           : ws.stageBox(box, slot, field, stat, value, &error);
                break;
            case 5: {
                static const char* names[] = {"QA", "PIKA", "ZZZZZZZZZZZZ", "", "Bob"};
                ok = party ? ws.stagePartyName(slot, (value & 1) != 0, names[value % 5], &error)
                           : ws.stageBoxName(box, slot, (value & 1) != 0, names[value % 5], &error);
                field = Field::NicknameText;
                break;
            }
            case 6: {
                const Slot from{(value & 1) ? Region::Party : Region::Box, 0, (value >> 1) % 3};
                ok = ws.cloneToEmptyBox(from, box, slot, &error);
                field = Field::CloneBox;
                break;
            }
            default:
                ok = ws.removeBox(box, slot, &error);
                field = Field::RemoveBox;
                break;
        }
        const auto after = viewBytes(ws, target);
        static const bool trace = std::getenv("QA_TRACE") != nullptr;
        if (trace)
            std::fprintf(stderr, "op kind=%u party=%d box=%zu slot=%zu field=%u stat=%zu value=%u -> %s %s\n",
                         kind, party, box, slot, rawField, stat, value, ok ? "OK" : "refused",
                         ok ? "" : error.c_str());
        if (!ok) {
            QA_CHECK(!error.empty());
            QA_CHECK(after == before && ws.changedRecordCount() == beforeCount);
            continue;
        }
        touched[{target.region == Region::Party ? 0 : 1, box * 100 + slot}] = true;
        if (field == Field::RemoveBox) continue;
        const auto afterView = viewOf(ws, target);
        QA_CHECK(afterView && afterView->valid() && !afterView->empty());
        if (!beforeView || !beforeView->valid() || beforeView->empty() || field == Field::CloneBox) continue;
        const uint32_t pidBefore = beforeView->pid(), pidAfter = afterView->pid();
        const uint8_t origin = beforeView->originVersion();
        if (G5::earlierGenerationOrigin5(origin)) QA_CHECK(pidAfter == pidBefore);
        if (!pidCoordinated(field) && field != Field::Species && field != Field::Form)
            QA_CHECK(pidAfter == pidBefore);
        if (!mayChangeShiny(field) && field != Field::Species && field != Field::Form)
            QA_CHECK(afterView->shiny() == beforeView->shiny());
        const uint8_t ratioBefore = genderRatio(*beforeView, b2w2);
        const uint8_t ratioAfter = genderRatio(*afterView, b2w2);
        // Species edits may change the ratio; the kept PID must still read as the stored gender.
        if (G5::gen5Origin5(origin) && G5::variableGenderRatio5(ratioBefore) &&
            G5::variableGenderRatio5(ratioAfter) &&
            G5::genderFromPid5(pidBefore, ratioBefore) == beforeView->gender())
            QA_CHECK(G5::genderFromPid5(pidAfter, ratioAfter) == afterView->gender());
    }

    // Export: the staged state must reach the candidate exactly, or be refused cleanly.
    const auto candidate = G5::buildStagedSaveCandidateForObservedSource(ws, sav, &error);
    if (!candidate) {
        QA_CHECK(!error.empty());
        return;
    }
    QA_CHECK(candidate->bytes.size() == sav.size());
    const auto reparsed = G5::Gen5ReadOnlySave::parse(candidate->bytes, parsed->exactGameId(), &error,
                                                      G5::SaveCopySelection::Primary);
    QA_CHECK(reparsed.has_value());
    QA_CHECK(reparsed->partyCount() == parsed->partyCount());
    for (const auto& [key, _] : touched) {
        const bool party = key.first == 0;
        const size_t box = key.second / 100, slot = key.second % 100;
        const Slot s{party ? Region::Party : Region::Box, box, slot};
        const auto staged = viewBytes(ws, s);
        const auto exported = party ? reparsed->partyPokemon(slot) : reparsed->boxPokemon(box, slot);
        if (!exported) continue;
        const auto bytes = exported->originalEncryptedBytes();
        if (party && slot >= parsed->partyCount()) continue;
        QA_CHECK(staged.empty() || std::equal(bytes.begin(), bytes.end(), staged.begin(), staged.end()));
    }
    const size_t copy = qasynth::gen5::copySize(parsed->family());
    for (size_t i = copy; i < sav.size(); ++i) QA_CHECK(candidate->bytes[i] == sav[i]);
}

std::vector<qafuzz::Bytes> seeds() {
    std::vector<qafuzz::Bytes> out;
    for (uint8_t family = 0; family < 4; ++family) {
        qafuzz::Bytes tape{family};
        auto op = [&](uint8_t kind, uint8_t regionBox, uint8_t slot, Field f, uint8_t stat, uint32_t v) {
            const uint8_t bytes[8] = {kind, regionBox, slot, static_cast<uint8_t>(f), stat,
                                      static_cast<uint8_t>(v), static_cast<uint8_t>(v >> 8),
                                      static_cast<uint8_t>((v >> 16) & 0x0F)};
            tape.insert(tape.end(), std::begin(bytes), std::end(bytes));
        };
        op(0, 0, 0, Field::Gender, 0, 1);         // Pikachu -> female via PID
        op(0, 0, 0, Field::Shiny, 0, 1);          // -> shiny, gender kept
        op(0, 0, 0, Field::AbilitySlot, 0, 1);
        op(0, 0, 0, Field::Level, 0, 40);
        op(0, 0, 1, Field::Gender, 0, 0);         // Gen IV origin: must be refused
        op(0, 0, 1, Field::Nature, 0, 7);         // Gen IV origin: must be refused
        op(0, 1, 0, Field::TrainerId, 0, 999);
        op(5, 0, 0, Field::NicknameText, 0, 1);
        op(6, 1, 1, Field::CloneBox, 0, 1);        // clone party 0 into box 0 slot 1
        op(7, 1, 2, Field::RemoveBox, 0, 0);       // remove a blank: refused
        op(0, 0, 0, static_cast<Field>(32), 0, 1);  // forged field
        out.push_back(tape);
    }
    return out;
}

} // namespace

const qafuzz::Target& qaTarget() {
    static const qafuzz::Target target{
        "gen5-edits", runOne, seeds, nullptr, {0, 3, 4, 5}, {},
    };
    return target;
}
