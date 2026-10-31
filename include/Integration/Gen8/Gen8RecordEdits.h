#ifndef POKEBANK_GEN8_RECORD_EDITS_H
#define POKEBANK_GEN8_RECORD_EDITS_H

// Shared PK8-format record edits for every title that stores Gen VIII-format records (Sword/Shield
// PK8 and Brilliant Diamond/Shining Pearl PB8, which PKHeX derives from the same G8PKM layout).
// One whitelist, one set of bounds and one set of refusals, parameterised only by the game label,
// the held-item predicate and the ball list. Gen8EditorProvider's table drives the whitelist.

#include "Integration/Gen8/Gen8EditorProvider.h"
#include "Integration/Gen8/Gen8PK8.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen8Edits {

namespace PK8 = Gen8::PK8;
using Field = Gen8EditorProvider::Field;
using RecordRegion = Gen8EditorProvider::RecordRegion;

// Per-byte bitmask of what a field edit may change inside a decrypted PK8/PB8. The cached party
// block (0x148..0x157) is never whitelisted: no Editable field feeds a battle stat.
[[nodiscard]] inline std::array<uint8_t, PK8::PartySize> editMask(RecordRegion region) {
    std::array<uint8_t, PK8::PartySize> m{};
    const auto range = [&](size_t from, size_t to) { for (size_t i = from; i < to; ++i) m[i] = 0xFF; };
    using Gen8EditorProvider::editable;
    m[0x06] = m[0x07] = 0xFF;  // checksum is always recomputed
    if (editable(Field::HeldItem, region)) range(0x0A, 0x0C);
    if (editable(Field::TrainerId, region)) range(0x0C, 0x0E);
    if (editable(Field::SecretId, region)) range(0x0E, 0x10);
    if (editable(Field::FatefulEncounter, region)) m[0x22] |= 0x01;
    if (editable(Field::Pokerus, region)) m[0x32] = 0xFF;
    if (editable(Field::Nickname, region)) {
        range(PK8::NicknameOffset, PK8::NicknameOffset + PK8::NameBytes);
        m[0x8F] |= 0x80;  // IV32 bit 31: IsNicknamed
    }
    if (editable(Field::OriginalTrainer, region))
        range(PK8::OriginalTrainerOffset, PK8::OriginalTrainerOffset + PK8::NameBytes);
    if (editable(Field::Friendship, region)) m[0x112] = 0xFF;
    if (editable(Field::Ball, region)) m[0x124] = 0xFF;
    if (editable(Field::OriginalTrainerGender, region)) m[0x125] |= 0x80;
    return m;
}

[[nodiscard]] inline bool withinMask(std::span<const uint8_t> before, std::span<const uint8_t> after, RecordRegion region,
                                     size_t* firstBad = nullptr) {
    if (before.size() != after.size()) return false;
    const auto mask = editMask(region);
    for (size_t i = 0; i < after.size() && i < mask.size(); ++i)
        if ((after[i] ^ before[i]) & ~mask[i]) {
            if (firstBad) *firstBad = i;
            return false;
        }
    return true;
}

[[nodiscard]] inline bool allowedName(std::string_view text) {
    if (text.empty() || text.size() > PK8::NameCapacity) return false;
    bool visible = false;
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}

struct Rules {
    std::string_view game;                               // "Sword/Shield", "BD/SP"
    std::function<bool(uint32_t)> heldItemAllowed;       // e.g. Names::isHeldItemPresent for the group
    std::vector<uint8_t> balls;                          // the title's ball list
};

// Applies one numeric field edit to `rec` and appends the review note text. False (with error) when
// the field is not editable or the value is out of bounds; `rec` is then unchanged.
[[nodiscard]] inline bool applyValue(PK8::Pokemon8& rec, Field field, uint32_t value, const Rules& rules,
                                     std::string& note, std::string& error) {
    const std::string g(rules.game);
    if (!Gen8EditorProvider::editable(field, RecordRegion::Box)) {
        error = g + " field is not editable in this build";
        return false;
    }
    switch (field) {
        case Field::HeldItem:
            if (value > 0xFFFF || (value != 0 && !rules.heldItemAllowed(value))) {
                error = g + " held item is not a holdable item in this game";
                return false;
            }
            note += "held item " + std::to_string(rec.heldItem()) + " -> " + std::to_string(value);
            rec.setHeldItem(static_cast<uint16_t>(value));
            return true;
        case Field::TrainerId:
        case Field::SecretId: {
            if (value > 0xFFFF) {
                error = g + " trainer ID half out of range";
                return false;
            }
            const bool tid = field == Field::TrainerId;
            note += std::string(tid ? "TID16 " : "SID16 ") + std::to_string(tid ? rec.tid() : rec.sid()) + " -> " +
                    std::to_string(value);
            tid ? rec.setTid(static_cast<uint16_t>(value)) : rec.setSid(static_cast<uint16_t>(value));
            return true;
        }
        case Field::FatefulEncounter:
            if (value > 1) {
                error = g + " fateful encounter is a single bit";
                return false;
            }
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec.setFatefulEncounter(value != 0);
            return true;
        case Field::Pokerus:
            if (value > 0xFF) {
                error = g + " Pokerus byte out of range";
                return false;
            }
            note += "Pokerus " + std::to_string(rec.pokerus()) + " -> " + std::to_string(value);
            rec.setPokerus(static_cast<uint8_t>(value));
            return true;
        case Field::Friendship:
            if (value > 0xFF) {
                error = g + " friendship out of range";
                return false;
            }
            if (rec.currentHandler() != 0) {
                error = g + " handling-trainer friendship editing is unsupported";
                return false;
            }
            note += "friendship " + std::to_string(rec.originalTrainerFriendship()) + " -> " + std::to_string(value);
            rec.setOriginalTrainerFriendship(static_cast<uint8_t>(value));
            return true;
        case Field::Ball:
            if (std::find(rules.balls.begin(), rules.balls.end(), value) == rules.balls.end()) {
                error = g + " ball is not in this game's ball list";
                return false;
            }
            note += "ball " + std::to_string(rec.ball()) + " -> " + std::to_string(value);
            rec.setBall(static_cast<uint8_t>(value));
            return true;
        case Field::OriginalTrainerGender:
            if (value > 1) {
                error = g + " OT gender is a single bit";
                return false;
            }
            note += "OT gender -> " + std::to_string(value);
            rec.setOriginalTrainerGender(static_cast<uint8_t>(value));
            return true;
        default:
            error = g + " field has no numeric staged editor";
            return false;
    }
}

[[nodiscard]] inline bool applyText(PK8::Pokemon8& rec, Field field, std::string_view text, const Rules& rules,
                                    std::string& note, std::string& error) {
    const std::string g(rules.game);
    if (field != Field::Nickname && field != Field::OriginalTrainer) {
        error = g + " text editing supports Nickname and OT name only";
        return false;
    }
    if (!allowedName(text)) {
        error = g + " names must be 1-12 printable ASCII characters";
        return false;
    }
    const std::u16string wide(text.begin(), text.end());
    if (field == Field::Nickname) {
        if (rec.isEgg()) {
            error = g + " egg nicknames are unsupported";
            return false;
        }
        rec.setRawName(PK8::NicknameOffset, wide);
        rec.setNicknamedFlag(true);
        note += "nickname -> " + std::string(text);
    } else {
        rec.setRawName(PK8::OriginalTrainerOffset, wide);
        note += "OT name -> " + std::string(text);
    }
    return true;
}

} // namespace PokeVault::Integration::Gen8Edits

#endif
