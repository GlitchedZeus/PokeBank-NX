// Gen9aLegality.cpp - PA9 (Legends: Z-A) legality (see include/Legality/Gen9aLegality.h).
//
// Every Invalid below names the PKHeX.Core verifier it mirrors (pinned commit in
// .github/workflows/legality-gen67-pkhex-diff.yml). Rules that need an encounter match, a
// learnset or HOME history are not implemented here; their coverage stays None/Partial so the
// verdict stays Incomplete.
#include "Legality/Gen9aLegality.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "Integration/Gen8/Gen8PK8.h"
#include "Legality/Gen8Legality.h"
#include "Legality/ModernNickname.h"
#include "Legality/ModernEncounter.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen9a {
namespace {

namespace PK8 = PokeVault::Integration::Gen8::PK8;
using Modern::Game;

constexpr uint8_t RatioMale = 0, RatioFemale = 254, RatioGenderless = 255;
constexpr uint8_t VersionSW = 44, VersionSH = 45, VersionSL = 50, VersionVL = 51, VersionZA = 52;
constexpr uint8_t LanguageSpanishL = 11;  // LanguageID.SpanishL, Legends: Z-A only
// IV index (HP Atk Def Spe SpA SpD) -> Hyper Training bit (HP Atk Def SpA SpD Spe).
constexpr std::array<uint8_t, 6> HyperTrainBit = {0, 1, 2, 5, 3, 4};

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
        species_ = Gen8::Data::nationalSpecies9(u16(0x08));
        if (species_ > Data::maxSpecies()) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species_) +
                    " does not exist in PA9 (PKHeX: species above the format's maximum)");
            return finish();
        }
        checkForm();
        personal_ = Data::personal(species_, form());
        if (personal_ && !personal_->present) {
            invalid(CheckIdentifier::Transfer, "Species/form is not present in Legends: Z-A");
            personal_.reset();
        }
        checkLevel();
        checkEffort();
        checkMoves();
        checkGender();
        checkAbility();
        checkNature();
        if (p_[0x124] == 0 || p_[0x124] > Data::maxBall())
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(p_[0x124]) + " cannot be held by a PA9");
        else if (!ModernEncounter::ballPossible(Modern::Format::PA9, species_, p_[0x124]))
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(p_[0x124]) + " is not possible for any encounter of the species");
        checkFateful(Modern::Format::PA9, species_, (p_[0x22] & 1) != 0);
        checkLanguage();
        checkNickname();
        if (!Data::heldItemAllowed(u16(0x0A)))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(u16(0x0A)) + " is not obtainable in Legends: Z-A");
        checkHyperTraining();
        checkLegendsZA();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            "Legends: Z-A encounter, learnset, Plus move by level, alpha, event and HOME history rules are not implemented yet; "
            "a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    std::span<const uint8_t> p_;
    const Modern::Input& in_;
    Report report_{};
    std::optional<Data::Personal> personal_;
    uint16_t species_ = 0;

    Report finish() { return std::move(report_); }
    void invalid(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warning(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Warning, std::move(text), id}); }

    uint16_t u16(size_t o) const noexcept { return PK8::read16(p_, o); }
    uint32_t u32(size_t o) const noexcept { return PK8::read32(p_, o); }
    uint8_t form() const noexcept { return p_[0x24]; }
    uint8_t version() const noexcept { return p_[0xCE]; }
    uint8_t metLevel() const noexcept { return p_[0x125] & 0x7F; }
    uint8_t iv(size_t i) const noexcept { return static_cast<uint8_t>((u32(0x8C) >> (5 * i)) & 0x1F); }
    bool alpha() const noexcept { return p_[0x23] != 0; }
    bool hasTracker() const noexcept { return u32(0x127) != 0 || u32(0x12B) != 0; }
    bool gen9Origin() const noexcept { return version() == VersionSL || version() == VersionVL || version() == VersionZA; }
    uint8_t level() const noexcept { return personal_ ? Pokemon::getLevelFromExp(u32(0x10), personal_->growth) : 0; }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (Modern::nativeFormat(in_.game) != Modern::Format::PA9) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, "The source game does not store PA9 records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
    }

    // FormVerifier: the form is within the species' forms and not battle-only.
    void checkForm() {
        const auto base = Data::personal(species_, 0);
        if (!base) return;
        const uint8_t f = form();
        if (f >= base->formCount && !(base->formCount <= 1 && f == 0)) {
            invalid(CheckIdentifier::Species, "Form " + std::to_string(f) + " is outside the species' forms");
            return;
        }
        if (Data::battleOnlyForm(species_, f)) invalid(CheckIdentifier::Species, "Battle-only form outside battle");
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

    // LearnVerifier (duplicates) and MovePPVerifier: Legends: Z-A has no PP Ups and never uses PP,
    // so each move holds its full PP (or 0, which HOME writes on the way in).
    void checkMoves() {
        std::array<uint16_t, 4> m{};
        for (size_t i = 0; i < 4; ++i) m[i] = u16(0x72 + 2 * i);
        for (size_t i = 0; i < 4; ++i)
            if (m[i] > Data::maxMove()) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " does not exist");
        // Moves can be removed and rearranged (HOME, the Z-A move menu), so empty slots and gaps
        // are not decided here; PKHeX checks them against the encounter's minimum moveset.
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] == 0) continue;
            for (size_t j = i + 1; j < 4; ++j)
                if (m[j] == m[i]) { invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " is duplicated"); break; }
        }
        for (size_t i = 0; i < 4; ++i) {
            const uint8_t ups = p_[0x7E + i], pp = p_[0x7A + i];
            if (ups != 0) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " has PP Ups, which Legends: Z-A does not have");
            const uint32_t healed = Data::basePP(m[i]) * (5u + std::min<uint8_t>(ups, 3)) / 5u;
            if (pp > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
            else if (pp != healed && pp != 0)
                invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is neither full nor zero (Legends: Z-A never uses PP)");
        }
    }

    // GenderVerifier: a fixed ratio fixes the gender, a dual ratio is male or female.
    void checkGender() {
        if (!personal_) return;
        const uint8_t g = (p_[0x22] >> 1) & 3, ratio = personal_->gender;
        if ((ratio == RatioGenderless) != (g == 2) || (ratio == RatioFemale && g != 1) || (ratio == RatioMale && g != 0))
            invalid(CheckIdentifier::Misc, "Gender does not match the species' gender ratio");
    }

    // AbilityVerifier.VerifyBirthAbility: Legends: Z-A keeps the ability the encounter species
    // was born with (checked against the encounter, not the current species), so only the
    // ability number's bit pattern is decided here.
    void checkAbility() {
        const uint8_t number = p_[0x16] & 7;
        if (number != 1 && number != 2 && number != 4) invalid(CheckIdentifier::Ability, "Ability number is not 1, 2 or 4");
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
        const N::Input n{N::Family::Switch9, p_.subspan(0x58, 26), species_, p_[0xD5], ((p_[0x8F] >> 6) & 1) != 0,
                         ((p_[0x8F] >> 7) & 1) != 0, 0};
        if (const char* m = N::message(N::check(n))) invalid(CheckIdentifier::Trainer, m);
    }

    // LanguageVerifier: no language 0 or 6; Spanish (LATAM) only for a Legends: Z-A origin.
    void checkLanguage() {
        const uint8_t lang = p_[0xD5];
        if (lang == 0 || lang == 6) invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " is not a language");
        else if (lang > LanguageSpanishL || (lang == LanguageSpanishL && version() != VersionZA))
            invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " does not exist in the origin game");
    }

    // HyperTrainingVerifier: never with six perfect IVs, never on a perfect IV.
    void checkHyperTraining() {
        const uint8_t flags = p_[0x126];
        if ((flags & 0x3F) == 0) return;
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

    // LegendsZAVerifier and MiscVerifierPA9: no level boost at rest, no Plus flag past the 340
    // used, battle version as Sword/Shield, obedience level (MiscVerifierHelpers), and an alpha
    // that came in through HOME has 255 height and weight scalars.
    void checkLegendsZA() {
        if (p_[0x4B] != 0) invalid(CheckIdentifier::Misc, "Level boost is not zero");
        const auto tail = p_.subspan(0x94, 12);  // PlusFlags1: flags 264-359, 340 used
        if (tail[10] != 0 || tail[11] != 0 || (tail[9] & 0xF0) != 0)
            invalid(CheckIdentifier::Moves, "A Plus move flag past the last Plus move is set");
        // LegendsZAVerifier.GetInvalidPlusMove: a Plus flag for a move no form of the species'
        // evolution line can learn.
        if (personal_) {
            for (uint16_t i = 0; i < Data::plusCount(); ++i) {
                const bool set = i < 264 ? ((p_[0xD6 + (i >> 3)] >> (i & 7)) & 1) != 0
                                         : ((p_[0x94 + ((i - 264) >> 3)] >> ((i - 264) & 7)) & 1) != 0;
                if (!set || Data::plusLineCanLearn(species_, i)) continue;
                invalid(CheckIdentifier::Moves, "Plus move flag " + std::to_string(i) + " is set but the species' line cannot learn it");
                break;
            }
        }
        const uint8_t battle = p_[0xCF], v = version();
        if (battle != 0) {
            const bool swordShieldOrigin = v == VersionSW || v == VersionSH || v == 47 || v == 48 || v == 49 ||
                                           v == 50 || v == 51 || v == 56 || v == 57;
            if ((battle != VersionSW && battle != VersionSH) || swordShieldOrigin)
                invalid(CheckIdentifier::Misc, "Battle version " + std::to_string(battle) + " is not possible for this record");
        }
        if (personal_) {
            const uint8_t obey = p_[0x11F], met = metLevel();
            // PA9.IsUntraded: no handling trainer name and a Gen IX origin.
            const bool untraded = p_[0xA8] == 0 && p_[0xA9] == 0 && gen9Origin();
            if (untraded ? obey != met : (obey < met || level() < obey))
                invalid(CheckIdentifier::Transfer, "Obedience level " + std::to_string(obey) + " is not possible for this record");
        }
        if (alpha() && hasTracker() && (p_[0x48] != 255 || p_[0x49] != 255))
            invalid(CheckIdentifier::Misc, "An alpha moved through HOME must have 255 height and weight scalars");
    }

    void checkTrainer() {
        const auto& t = in_.trainer;
        if (!t.present || t.tid != u16(0x0C) || t.sid != u16(0x0E)) return;
        bool same = true;
        for (size_t i = 0; i < t.ot.size(); ++i) {
            const uint16_t mine = u16(0xF8 + 2 * i);
            if (t.ot[i] != mine) { same = false; break; }
            if (mine == 0) break;
        }
        if (!same || t.female != ((p_[0x125] >> 7) == 1))
            warning(CheckIdentifier::Trainer, "Trainer ID matches the save but the OT name or gender does not");
    }
};

} // namespace

Report analyze(std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    if (!PK8::supportedSize(record.size())) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 328- or 344-byte PA9 record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    const std::vector<uint8_t> plain = input.encrypted ? PK8::decrypt(record) : std::vector<uint8_t>(record.begin(), record.end());
    if (PK8::checksum(plain) != PK8::read16(plain, 0x06)) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PA9 checksum does not match its data", CheckIdentifier::Checksum});
        return report;
    }
    if (PK8::read16(plain, 0x04) != 0) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PA9 sanity field is set (bad egg)", CheckIdentifier::Structure});
        return report;
    }
    if (PK8::read16(plain, 0x08) == 0) {
        if (std::any_of(plain.begin() + 8, plain.end(), [](uint8_t b) { return b != 0; })) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the PA9 payload is not a blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(plain, input).run();
}

} // namespace Legality::Gen9a
