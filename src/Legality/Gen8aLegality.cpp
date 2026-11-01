// Gen8aLegality.cpp - PA8 (Legends: Arceus) legality (see include/Legality/Gen8aLegality.h).
//
// Every Invalid below names the PKHeX.Core verifier it mirrors (pinned commit in
// .github/workflows/legality-gen67-pkhex-diff.yml). Rules that need an encounter match, a
// learnset or HOME history are not implemented here; their coverage stays None/Partial so the
// verdict stays Incomplete.
#include "Legality/Gen8aLegality.h"

#include <algorithm>
#include <cstring>
#include <array>
#include <string>

#include "Integration/Gen8/Gen8PK8.h"
#include "Legality/ModernNickname.h"
#include "Legality/Gen67Legality.h"
#include "Legality/ModernEncounter.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen8a {

namespace PK8 = PokeVault::Integration::Gen8::PK8;

namespace Codec {
namespace {
constexpr size_t HeaderSize = 8;
constexpr size_t BlockCount = 4;

void shuffle(std::span<uint8_t> blocks, uint32_t sv) {
    std::array<uint8_t, BlockSize * BlockCount> copy{};
    std::copy_n(blocks.begin(), copy.size(), copy.begin());
    for (size_t i = 0; i < BlockCount; ++i) {
        const size_t from = PK8::BlockPosition[sv * BlockCount + i];
        std::copy_n(copy.begin() + static_cast<std::ptrdiff_t>(from * BlockSize), BlockSize,
                    blocks.begin() + static_cast<std::ptrdiff_t>(i * BlockSize));
    }
}

bool supported(size_t size) noexcept { return size == StoredSize || size == PartySize; }
} // namespace

std::vector<uint8_t> decrypt(std::span<const uint8_t> encrypted) {
    if (!supported(encrypted.size())) return {};
    std::vector<uint8_t> out(encrypted.begin(), encrypted.end());
    const uint32_t ec = PK8::read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    PK8::cryptArray(body, ec);
    if (out.size() > StoredSize) PK8::cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    shuffle(body, PK8::shuffleValue(ec));
    return out;
}

std::vector<uint8_t> encrypt(std::span<const uint8_t> plain) {
    if (!supported(plain.size())) return {};
    std::vector<uint8_t> out(plain.begin(), plain.end());
    const uint32_t ec = PK8::read32(out, 0);
    std::span<uint8_t> body(out.data() + HeaderSize, StoredSize - HeaderSize);
    shuffle(body, PK8::BlockPositionInvert[PK8::shuffleValue(ec)]);
    PK8::cryptArray(body, ec);
    if (out.size() > StoredSize) PK8::cryptArray(std::span<uint8_t>(out).subspan(StoredSize), ec);
    return out;
}

uint16_t checksum(std::span<const uint8_t> plain) noexcept {
    uint16_t sum = 0;
    for (size_t i = HeaderSize; i + 1 < StoredSize && i + 1 < plain.size(); i += 2)
        sum = static_cast<uint16_t>(sum + PK8::read16(plain, i));
    return sum;
}
} // namespace Codec

namespace {

using Modern::Game;
using Modern::Slot;

constexpr uint16_t Enamorus = 905;
constexpr uint8_t RatioMale = 0, RatioFemale = 254, RatioGenderless = 255;
constexpr uint8_t VersionSW = 44, VersionSH = 45, VersionPLA = 47;
// IV index (HP Atk Def Spe SpA SpD) -> Hyper Training bit (HP Atk Def SpA SpD Spe).
constexpr std::array<uint8_t, 6> HyperTrainBit = {0, 1, 2, 5, 3, 4};

// GanbaruExtensions.GetMaxGanbaru: 10 minus the IV's bias.
uint8_t maxGanbaru(uint8_t iv) noexcept { return static_cast<uint8_t>(10 - (iv >= 31 ? 3 : iv >= 26 ? 2 : iv >= 20 ? 1 : 0)); }

class Analyzer {
public:
    Analyzer(std::span<const uint8_t> plain, const Modern::Input& input) : p_(plain), in_(input) {}

    Report run() {
        report_.coverage.structure = CoverageLevel::Complete;
        report_.coverage.internal = CoverageLevel::Partial;
        report_.coverage.moves = CoverageLevel::Partial;
        report_.coverage.pidRng = CoverageLevel::Partial;
        report_.coverage.transfer = CoverageLevel::Partial;
        checkSource();
        if (species() > Data::maxSpecies()) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species()) +
                    " does not exist in PA8 (PKHeX: species above the format's maximum)");
            return finish();
        }
        checkForm();
        personal_ = Data::personal(species(), form());
        if (personal_ && !personal_->present) {
            invalid(CheckIdentifier::Transfer, "Species/form is not present in Legends: Arceus");
            personal_.reset();
        }
        checkLevel();
        checkEffort();
        checkMoves();
        checkGender();
        checkAbility();
        checkPidEc();
        checkNature();
        if (ball() == 0 || ball() > Data::maxBall())
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball()) + " cannot be held by a PA8");
        else if (!ModernEncounter::ballPossible(Modern::Format::PA8, species(), ball()))
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball()) + " is not possible for any encounter of the species");
        checkFateful(Modern::Format::PA8, species(), (p_[0x22] & 1) != 0);
        checkLanguage();
        checkNickname();
        checkRelearn();
        if (!Data::heldItemAllowed(u16(0x0A)))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(u16(0x0A)) + " is not obtainable in Legends: Arceus");
        checkHyperTraining();
        checkLegendsArceus();
        checkMoveShop();
        checkMisc();
        checkSizes();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            "Legends: Arceus encounter, learnset, alpha, event and HOME history rules are not implemented yet; "
            "a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    std::span<const uint8_t> p_;
    const Modern::Input& in_;
    Report report_{};
    std::optional<Data::Personal> personal_;

    Report finish() { return std::move(report_); }
    void invalid(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warning(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Warning, std::move(text), id}); }

    // LearnVerifierRelearn (Legal.IsOriginalMovesetDeleted): a record from another game moved in through
    // HOME has no relearn moves; no repeated relearn move.
    void checkRelearn() {
        std::array<uint16_t, 4> moves{};
        for (size_t i = 0; i < 4; ++i) moves[i] = u16(0x8A + 2 * i);
        for (size_t i = 0; i < 4; ++i)
            for (size_t j = 0; j < i; ++j)
                if (moves[i] != 0 && moves[i] == moves[j]) {
                    invalid(CheckIdentifier::Moves, "Relearn move " + std::to_string(moves[i]) + " is repeated");
                    return;
                }
        if (moves != std::array<uint16_t, 4>{} && version() != VersionPLA)
            invalid(CheckIdentifier::Moves, "Relearn moves are set but the original moveset was deleted (moved into Legends: Arceus through HOME)");
    }

    uint16_t u16(size_t o) const noexcept { return PK8::read16(p_, o); }
    uint32_t u32(size_t o) const noexcept { return PK8::read32(p_, o); }
    float f32(size_t o) const noexcept {
        const uint32_t bits = u32(o);
        float f;
        std::memcpy(&f, &bits, sizeof f);
        return f;
    }
    uint16_t species() const noexcept { return u16(0x08); }
    uint8_t form() const noexcept { return p_[0x24]; }
    uint8_t gender() const noexcept { return (p_[0x22] >> 2) & 3; }
    uint8_t ball() const noexcept { return p_[0x137]; }
    uint8_t version() const noexcept { return p_[0xEE]; }
    uint8_t metLevel() const noexcept { return p_[0x13D] & 0x7F; }
    uint8_t iv(size_t i) const noexcept { return static_cast<uint8_t>((u32(0x94) >> (5 * i)) & 0x1F); }
    bool isEgg() const noexcept { return ((u32(0x94) >> 30) & 1) != 0; }
    bool alpha() const noexcept { return (p_[0x16] & 0x20) != 0; }
    uint64_t tracker() const noexcept { return static_cast<uint64_t>(u32(0x14D)) | (static_cast<uint64_t>(u32(0x151)) << 32); }
    bool flag(size_t base, size_t index) const noexcept { return ((p_[base + (index >> 3)] >> (index & 7)) & 1) != 0; }
    uint8_t level() const noexcept { return personal_ ? Pokemon::getLevelFromExp(u32(0x10), personal_->growth) : 0; }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (Modern::nativeFormat(in_.game) != Modern::Format::PA8) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, "The source game does not store PA8 records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
    }

    // FormVerifier: the form is within the species' forms and not battle-only (Dialga, Palkia,
    // Giratina and Arceus change form with key items in Legends: Arceus, so no form-item rule).
    void checkForm() {
        const auto base = Data::personal(species(), 0);
        if (!base) return;
        const uint8_t f = form();
        if (f >= base->formCount && !(base->formCount <= 1 && f == 0)) {
            invalid(CheckIdentifier::Species, "Form " + std::to_string(f) + " is outside the species' forms");
            return;
        }
        if (Data::battleOnlyForm(species(), f)) invalid(CheckIdentifier::Species, "Battle-only form outside battle");
    }

    // LevelVerifier.
    void checkLevel() {
        if (!personal_) return;
        const uint8_t lvl = level();
        if (lvl >= 100 && u32(0x10) != Pokemon::getExpForLevel(100, personal_->growth))
            invalid(CheckIdentifier::Stats, "Experience is above the level-100 total");
        if (lvl < metLevel()) invalid(CheckIdentifier::Stats, "Current level is below the met level");
    }

    // EffortValueVerifier (generation 6+ limits).
    void checkEffort() {
        unsigned sum = 0;
        bool above252 = false;
        for (size_t i = 0; i < 6; ++i) {
            sum += p_[0x26 + i];
            above252 = above252 || p_[0x26 + i] > 252;
        }
        if (sum > 510) invalid(CheckIdentifier::Stats, "EV total is above 510");
        else if (above252) invalid(CheckIdentifier::Stats, "An EV is above 252");
    }

    // LearnVerifier.Finalize and MovePPVerifier.
    void checkMoves() {
        std::array<uint16_t, 4> m{};
        for (size_t i = 0; i < 4; ++i) m[i] = u16(0x54 + 2 * i);
        for (size_t i = 0; i < 4; ++i)
            if (m[i] > Data::maxMove()) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " does not exist");
        if (m[0] == 0) invalid(CheckIdentifier::Moves, "The first move slot is empty");
        bool empty = false;
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] == 0) { empty = true; continue; }
            if (empty) { invalid(CheckIdentifier::Moves, "An empty move slot comes before a move"); empty = false; continue; }
            for (size_t j = i + 1; j < 4; ++j)
                if (m[j] == m[i]) { invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " is duplicated"); break; }
        }
        for (size_t i = 0; i < 4; ++i) {
            const uint8_t ups = p_[0x86 + i];
            // Legal.IsPPUpAvailable: Legends: Arceus has no PP Ups (HOME clears them).
            if (ups != 0) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " has PP Ups, which Legends: Arceus does not have");
            const uint32_t healed = Data::basePP(m[i]) * (5u + std::min<uint8_t>(ups, 3)) / 5u;
            if (p_[0x5C + i] > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
        }
    }

    // GenderVerifier: a fixed ratio fixes the gender, a dual ratio is male or female.
    void checkGender() {
        if (!personal_) return;
        const uint8_t g = gender(), ratio = personal_->gender;
        if ((ratio == RatioGenderless) != (g == 2) || (ratio == RatioFemale && g != 1) || (ratio == RatioMale && g != 0))
            invalid(CheckIdentifier::Misc, "Gender does not match the species' gender ratio");
    }

    // AbilityVerifier (format >= 6 structural part).
    void checkAbility() {
        if (!personal_) return;
        const uint16_t ability = u16(0x14);
        const std::array<uint16_t, 3> slots = {personal_->ability1, personal_->ability2, personal_->abilityHidden};
        if (std::find(slots.begin(), slots.end(), ability) == slots.end()) {
            invalid(CheckIdentifier::Ability, "Ability is not one of the species' abilities");
            return;
        }
        const uint8_t number = p_[0x16] & 7;
        if (number != 1 && number != 2 && number != 4) {
            invalid(CheckIdentifier::Ability, "Ability number is not 1, 2 or 4");
            return;
        }
        if (slots[number >> 1] != ability) invalid(CheckIdentifier::Ability, "Ability does not match the ability number");
    }

    // PIDVerifier (nature range) and MiscVerifierHelpers.VerifyStatAlignment.
    void checkNature() {
        const uint8_t nature = p_[0x20], stat = p_[0x21];
        if (nature >= 25) {
            invalid(CheckIdentifier::Misc, "Nature is out of range");
            return;
        }
        if (stat != nature && !Data::mintNature(stat))
            invalid(CheckIdentifier::Misc, "Stat nature is neither the nature nor a mint");
    }

    // MiscFatefulEncounterVerifier: the fateful flag in a state no encounter of the species takes.
    void checkFateful(Modern::Format format, uint16_t species, bool fateful) {
        if (ModernEncounter::fatefulPossible(format, species, fateful)) return;
        invalid(CheckIdentifier::Encounter, fateful ? "Fateful encounter flag is set but no encounter of the species is fateful"
                                                    : "Fateful encounter flag is clear but every encounter of the species is fateful");
    }

    // NicknameVerifier: NickLengthShort, and VerifyUnNicknamed (ModernNickname.h).
    void checkNickname() {
        namespace N = ModernNickname;
        const N::Input n{N::Family::Switch8, p_.subspan(0x60, 26), species(), p_[0xF2], ((p_[0x97] >> 6) & 1) != 0,
                         ((p_[0x97] >> 7) & 1) != 0, 0};
        if (const char* m = N::message(N::check(n))) invalid(CheckIdentifier::Trainer, m);
    }

    // LanguageVerifier (generation 8: up to Chinese (Traditional), no language 6).
    void checkLanguage() {
        const uint8_t lang = p_[0xF2];
        if (lang == 0 || lang == 6) invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " is not a language");
        else if (lang > 10) invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " does not exist in Legends: Arceus");
    }

    // PIDVerifier.VerifyEC: a Generation III-V origin carries the Poke Transporter PID
    // (PK5.GetTransferPID); any other origin has PID != EC (VerifyFutureOriginEC), except the
    // fixed-EC Legends: Arceus and BD/SP gifts (WA8/WB8.IsEquivalentFixedECPID), a Sword/Shield
    // HOME gift with EC 0 (WC8.IsHOMEGift), and the Switch Virtual Console records, whose HOME PID
    // is not checked here.
    void checkPidEc() {
        const uint8_t v = version();
        if (v == 0 || v == 56 || v == 57) return;
        const uint32_t pid = u32(0x1C), ec = u32(0x00), id32 = u32(0x0C);
        const uint8_t gen = Gen67::originGeneration(v);
        if (gen >= 3 && gen <= 5) {
            if (pid != Gen67::transferPid(ec, id32))
                invalid(CheckIdentifier::PidRng, "PID does not match the Poke Transporter PID for its encryption constant");
            return;
        }
        const bool fixedEcGift = v == 47 || v == 48 || v == 49;
        const bool homeGift = ec == 0 && (v == 44 || v == 45);
        if (pid == ec && !fixedEcGift && !homeGift)
            invalid(CheckIdentifier::PidRng, "PID equals the encryption constant");
    }

    // HyperTrainingVerifier: Legends: Arceus has no Hyper Training, so a trained record must have
    // left through HOME (IHyperTrain.IsHyperTrainingAvailable: a HOME tracker); the minimum level
    // is 50 after a Scarlet/Violet or Z-A visit and 100 otherwise (a level from 50 to 99 is decided
    // only when no species of the line is in those games); never with six perfect IVs, never on a
    // perfect IV.
    void checkHyperTraining() {
        const uint8_t flags = p_[0x13E];
        if ((flags & 0x3F) == 0) return;
        if (tracker() == 0) {
            invalid(CheckIdentifier::Stats, "Hyper Training on a Legends: Arceus record that never left through HOME");
            return;
        }
        if (personal_) {
            const uint8_t lvl = level();
            if (lvl < 50 || (lvl < 100 && !ModernEncounter::visitNinePossible(species()))) {
                invalid(CheckIdentifier::Stats, lvl < 50 ? "Hyper Training needs level 50 or more"
                                                         : "Hyper Training below level 100 needs a Scarlet/Violet or Z-A visit, and "
                                                           "no species of its line is in those games");
                return;
            }
        }
        int total = 0;
        for (size_t i = 0; i < 6; ++i) total += iv(i);
        if (total == 31 * 6) {
            invalid(CheckIdentifier::Stats, "Hyper Training with six perfect IVs");
            return;
        }
        for (size_t i = 0; i < 6; ++i) {
            if (iv(i) == 31 && ((flags >> HyperTrainBit[i]) & 1)) {
                invalid(CheckIdentifier::Stats, "Hyper Training on a perfect IV");
                return;
            }
        }
    }

    // LegendsArceusVerifier: no noble flag outside battle, the displayed scale is the height
    // scalar, and each Effort Level (Ganbaru value) is within its IV's cap.
    void checkLegendsArceus() {
        if ((p_[0x16] & 0x40) != 0) invalid(CheckIdentifier::Misc, "Noble flag is set outside battle");
        if (p_[0x50] != p_[0x52]) invalid(CheckIdentifier::Encounter, "Scale is not the height scalar");
        for (size_t i = 0; i < 6; ++i) {
            if (p_[0xA4 + i] > maxGanbaru(iv(i))) {
                invalid(CheckIdentifier::Stats, "An Effort Level is above the cap its IV allows");
                return;
            }
        }
    }

    // LegendsArceusVerifier.CheckMastery: an alpha move needs the alpha flag, must be a move
    // the species can master in the move shop, and must be mastered; a purchased move must be
    // one the species can buy.
    void checkMoveShop() {
        if (!personal_) return;
        const auto shop = Data::moveShop();
        const uint16_t alphaMove = u16(0x3E);
        if (alphaMove != 0) {
            const auto it = std::find(shop.begin(), shop.end(), alphaMove);
            const size_t index = static_cast<size_t>(it - shop.begin());
            if (!alpha()) invalid(CheckIdentifier::Moves, "Alpha move is set on a record that is not an alpha");
            else if (it == shop.end() || ((personal_->moveShop >> index) & 1) == 0)
                invalid(CheckIdentifier::Moves, "Alpha move is not a move shop move the species can master");
            else if (!flag(0x15D, index))
                invalid(CheckIdentifier::Moves, "Alpha move is not marked as mastered");
        }
        for (size_t i = 0; i < shop.size() && i < 64; ++i) {
            if (flag(0x155, i) && ((personal_->moveShop >> i) & 1) == 0) {
                invalid(CheckIdentifier::Moves, "Move shop move " + std::to_string(shop[i]) + " is purchased but the species cannot buy it");
                return;
            }
        }
    }

    // MiscVerifierPA8 and FullnessRules: no sociability, Gigantamax, Dynamax level or TR flags;
    // fullness and enjoyment are never set; battle version as Sword/Shield.
    void checkMisc() {
        if (u32(0x48) != 0) invalid(CheckIdentifier::Misc, "Sociability is not zero");
        if ((p_[0x16] & 0x10) != 0) invalid(CheckIdentifier::Misc, "Gigantamax flag is set");
        if (p_[0x98] != 0) invalid(CheckIdentifier::Misc, "Dynamax level is not zero");
        if (!isEgg() && std::any_of(p_.begin() + 0x13F, p_.begin() + 0x14D, [](uint8_t b) { return b != 0; }))
            invalid(CheckIdentifier::Moves, "TR flags are set");
        if (p_[0xEC] != 0)  // Legends: Arceus never sets it, even through HOME
            invalid(CheckIdentifier::Misc, "Fullness is not zero");
        if (p_[0xED] != 0) invalid(CheckIdentifier::Misc, "Enjoyment is not zero");
        const uint8_t battle = p_[0xEF], v = version();
        if (battle != 0) {
            const bool swordShieldOrigin = v == VersionSW || v == VersionSH || v == 47 || v == 48 || v == 49 ||
                                           v == 50 || v == 51 || v == 56 || v == 57;
            if ((battle != VersionSW && battle != VersionSH) || swordShieldOrigin)
                invalid(CheckIdentifier::Misc, "Battle version " + std::to_string(battle) + " is not possible for this record");
        }
    }

    // MiscVerifierPA8.VerifyAbsoluteSizes: a Legends: Arceus record that never left the game
    // (no HOME tracker) and is not an alpha or Enamorus stores the height and weight the game
    // calculates from its scalars. Alphas (127-scalar statics), Enamorus' gift, HOME-calculated
    // values and pre-evolution values need the encounter or history and are not decided.
    void checkSizes() {
        if (!personal_ || version() != VersionPLA || tracker() != 0 || alpha() || species() == Enamorus) return;
        const uint8_t hs = p_[0x50], ws = p_[0x51];
        if (f32(0xAC) != heightAbsolute(personal_->height, hs))
            invalid(CheckIdentifier::Stats, "Stored height is not the height its scalar gives");
        if (f32(0xB0) != weightAbsolute(personal_->weight, hs, ws))
            invalid(CheckIdentifier::Stats, "Stored weight is not the weight its scalars give");
    }

    void checkTrainer() {
        const auto& t = in_.trainer;
        if (!t.present || t.tid != u16(0x0C) || t.sid != u16(0x0E)) return;
        bool same = true;
        for (size_t i = 0; i < t.ot.size(); ++i) {
            const uint16_t mine = i < 13 ? u16(0x110 + 2 * i) : 0;
            if (t.ot[i] != mine) { same = false; break; }
            if (mine == 0) break;
        }
        if (!same || t.female != ((p_[0x13D] >> 7) == 1))
            warning(CheckIdentifier::Trainer, "Trainer ID matches the save but the OT name or gender does not");
    }
};

// PA8.GetScalarPercent: (scalar / 255) * 0.4 + 0.8, each step in single precision.
float scalarPercent(uint8_t scalar) noexcept {
    volatile float r = static_cast<float>(scalar) / 255.0f;
    r = r * 0.40000004f;
    r = r + 0.8f;
    return r;
}

} // namespace

float heightAbsolute(uint16_t height, uint8_t heightScalar) noexcept {
    volatile float out = static_cast<float>(height) * scalarPercent(heightScalar);
    return out;
}

float weightAbsolute(uint16_t weight, uint8_t heightScalar, uint8_t weightScalar) noexcept {
    volatile float ratio = scalarPercent(heightScalar) * scalarPercent(weightScalar);
    volatile float out = static_cast<float>(weight) * ratio;
    return out;
}

Report analyze(std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    if (record.size() != Codec::StoredSize && record.size() != Codec::PartySize) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 360- or 376-byte PA8 record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    const std::vector<uint8_t> plain = input.encrypted ? Codec::decrypt(record) : std::vector<uint8_t>(record.begin(), record.end());
    if (Codec::checksum(plain) != PK8::read16(plain, 0x06)) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PA8 checksum does not match its data", CheckIdentifier::Checksum});
        return report;
    }
    if (PK8::read16(plain, 0x04) != 0) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PA8 sanity field is set (bad egg)", CheckIdentifier::Structure});
        return report;
    }
    if (PK8::read16(plain, 0x08) == 0) {
        if (std::any_of(plain.begin() + 8, plain.end(), [](uint8_t b) { return b != 0; })) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the PA8 payload is not a blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(plain, input).run();
}

} // namespace Legality::Gen8a
