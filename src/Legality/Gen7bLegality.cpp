// Gen7bLegality.cpp - PB7 (Let's Go Pikachu/Eevee) native legality (see
// include/Legality/Gen7bLegality.h).
//
// Every Invalid below names the PKHeX.Core verifier it mirrors (pinned commit in
// .github/workflows/legality-gen67-pkhex-diff.yml). Rules that need an encounter match, a
// learnset or a distribution record are not implemented here; their coverage stays
// None/Partial so the verdict stays Incomplete.
#include "Legality/Gen7bLegality.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "Integration/Gen6/Gen6PK6.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen7b {
namespace {

namespace PK6 = PokeVault::Integration::Gen6::PK6;
using Modern::Game;
using Modern::Slot;

constexpr uint16_t Pikachu = 25, Eevee = 133, Meltan = 808, Melmetal = 809;
constexpr uint8_t RatioMale = 0, RatioFemale = 254, RatioGenderless = 255;
constexpr uint8_t VersionGO = 34, VersionGP = 42, VersionGE = 43;
constexpr uint16_t LocationGO7 = 50;  // Locations.GO7: GO Park
constexpr uint8_t InitialSpiritMood = 100;
constexpr uint8_t AwakeningMax = 200;
// IV index (HP Atk Def Spe SpA SpD) -> PB7 Hyper Training bit (HP Atk Def SpA SpD Spe).
constexpr std::array<uint8_t, 6> HyperTrainBit = {0, 1, 2, 5, 3, 4};

float readFloat(std::span<const uint8_t> b, size_t o) noexcept {
    const uint32_t bits = PK6::read32(b, o);
    float f;
    std::memcpy(&f, &bits, sizeof f);
    return f;
}

// DateUtil.IsValidDate(2000 + year, month, day) and EncounterDate.IsValidDateSwitch.
bool validDate(uint8_t year, uint8_t month, uint8_t day) noexcept {
    if (month < 1 || month > 12 || day < 1) return false;
    static constexpr std::array<uint8_t, 12> days = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    const unsigned y = 2000u + year;
    const bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return day <= days[month - 1] + (month == 2 && leap ? 1 : 0);
}

class Analyzer {
public:
    Analyzer(const PK6::Pokemon6& pk, const Modern::Input& input) : pk_(pk), in_(input), plain_(pk.plain()) {}

    Report run() {
        report_.coverage.structure = CoverageLevel::Complete;
        report_.coverage.internal = CoverageLevel::Partial;
        report_.coverage.moves = CoverageLevel::Partial;
        report_.coverage.pidRng = CoverageLevel::Partial;
        report_.coverage.transfer = CoverageLevel::Partial;
        checkSource();
        const uint16_t species = pk_.species();
        if (species > Data::maxSpecies()) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species) +
                    " does not exist in PB7 (PKHeX: species above the format's maximum)");
            return finish();
        }
        version_ = pk_.originVersion();
        checkOrigin();
        checkForm();
        personal_ = Data::personal(species, pk_.form());
        if (!personal_) personal_ = Data::personal(species, 0);
        if (personal_ && !personal_->present) personal_.reset();
        if (pk_.isEgg()) invalid(CheckIdentifier::Egg, "Let's Go Pikachu/Eevee has no eggs");
        checkLevel();
        checkEffortAndAwakening();
        checkMoves();
        if (pk_.pid() == pk_.encryptionConstant()) invalid(CheckIdentifier::PidRng, "PID equals the encryption constant");
        checkGender();
        checkAbility();
        if (pk_.nature() >= 25) invalid(CheckIdentifier::Misc, "Nature is out of range");
        checkBall();
        checkLanguage();
        if (!Data::heldItemAllowed(pk_.heldItem()))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(pk_.heldItem()) + " is not obtainable in Let's Go Pikachu/Eevee");
        checkHyperTraining();
        checkCombatPower();
        checkSpiritMood();
        checkReceivedDate();
        checkSizes();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            "Let's Go Pikachu/Eevee encounter, learnset, event and minimum Awakening Value rules are not implemented "
            "yet; a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    const PK6::Pokemon6& pk_;
    const Modern::Input& in_;
    std::span<const uint8_t> plain_;
    Report report_{};
    std::optional<Data::Personal> personal_;
    uint8_t version_ = 0;

    Report finish() { return std::move(report_); }
    void invalid(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warning(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Warning, std::move(text), id}); }

    uint8_t level() const noexcept { return personal_ ? Pokemon::getLevelFromExp(pk_.experience(), personal_->growth) : 0; }
    uint8_t av(size_t i) const noexcept { return plain_[0x24 + i]; }  // HP Atk Def Spe SpA SpD
    // PB7.IsStarter: the partner Pikachu (form 8) or Eevee (form 1).
    bool starter() const noexcept {
        return (pk_.species() == Pikachu && pk_.form() == 8) || (pk_.species() == Eevee && pk_.form() == 1);
    }
    // G6PKM.IsUntraded: no handling trainer name (every PB7 origin is generation 7).
    bool untraded() const noexcept { return plain_[0x78] == 0 && plain_[0x79] == 0; }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (Modern::nativeFormat(in_.game) != Modern::Format::PB7) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, "The source game does not store PB7 records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
    }

    // EncounterFinder: a PB7 comes from Let's Go Pikachu/Eevee or from GO Park (version GO,
    // met location GO7); nothing else moves into Let's Go. TransferVerifier: Let's Go holds
    // only its own species and forms (PersonalTable.GG.IsPresentInGame).
    void checkOrigin() {
        const bool native = version_ == VersionGP || version_ == VersionGE;
        const bool goPark = version_ == VersionGO && pk_.metLocation() == LocationGO7;
        if (!native && !goPark) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version_) + " with met location " +
                    std::to_string(pk_.metLocation()) + " cannot be stored in a PB7");
        }
        const auto p = Data::personal(pk_.species(), pk_.form());
        if (p && !p->present) invalid(CheckIdentifier::Transfer, "Species/form is not present in Let's Go Pikachu/Eevee");
    }

    // FormVerifier: the form is within the species' Let's Go forms and not battle-only.
    void checkForm() {
        const auto base = Data::personal(pk_.species(), 0);
        if (!base) return;
        const uint8_t f = pk_.form();
        if (f >= base->formCount && !(base->formCount <= 1 && f == 0)) {
            invalid(CheckIdentifier::Species, "Form " + std::to_string(f) + " is outside the species' forms");
            return;
        }
        if (Data::battleOnlyForm(pk_.species(), f)) invalid(CheckIdentifier::Species, "Battle-only form outside battle");
    }

    // LevelVerifier.
    void checkLevel() {
        if (!personal_) return;
        const uint8_t lvl = level();
        if (lvl >= 100 && pk_.experience() != Pokemon::getExpForLevel(100, personal_->growth))
            invalid(CheckIdentifier::Stats, "Experience is above the level-100 total");
        if (lvl < pk_.metLevel()) invalid(CheckIdentifier::Stats, "Current level is below the met level");
    }

    // AwakenedValueVerifier: Let's Go has no EVs, and each Awakening Value is at most 200.
    void checkEffortAndAwakening() {
        for (size_t i = 0; i < 6; ++i) {
            if (pk_.ev(i) != 0) { invalid(CheckIdentifier::Stats, "Let's Go Pikachu/Eevee records have no EVs"); break; }
        }
        for (size_t i = 0; i < 6; ++i) {
            if (av(i) > AwakeningMax) { invalid(CheckIdentifier::Stats, "An Awakening Value is above 200"); break; }
        }
    }

    // LearnVerifier.Finalize and MovePPVerifier.
    void checkMoves() {
        std::array<uint16_t, 4> m{};
        for (size_t i = 0; i < 4; ++i) m[i] = pk_.move(i);
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
            if (pk_.movePPUps(i) != 0 && m[i] == 0) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
            const uint32_t healed = Data::basePP(m[i]) * (5u + pk_.movePPUps(i)) / 5u;
            if (pk_.movePP(i) > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
        }
    }

    // GenderVerifier and PKM.IsGenderValid (a Gen VII origin: ratio only).
    void checkGender() {
        if (!personal_) return;
        const uint8_t gender = pk_.gender(), ratio = personal_->gender;
        if ((ratio == RatioGenderless) != (gender == 2) || (ratio == RatioFemale && gender != 1) || (ratio == RatioMale && gender != 0))
            invalid(CheckIdentifier::Misc, "Gender does not match the species' gender ratio");
    }

    // AbilityVerifier (format >= 6 structural part).
    void checkAbility() {
        if (!personal_) return;
        const uint16_t ability = pk_.ability();
        const std::array<uint16_t, 3> slots = {personal_->ability1, personal_->ability2, personal_->abilityHidden};
        if (std::find(slots.begin(), slots.end(), ability) == slots.end()) {
            invalid(CheckIdentifier::Ability, "Ability is not one of the species' abilities");
            return;
        }
        const uint8_t number = pk_.abilityNumber() & 7;
        if (number != 1 && number != 2 && number != 4) {
            invalid(CheckIdentifier::Ability, "Ability number is not 1, 2 or 4");
            return;
        }
        if (slots[number >> 1] != ability) invalid(CheckIdentifier::Ability, "Ability does not match the ability number");
    }

    // BallVerifier: ball 0 or above Beast Ball.
    void checkBall() {
        const uint8_t ball = pk_.ball();
        if (ball == 0 || ball > Data::maxBall()) invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball) + " cannot be held by a PB7");
    }

    // LanguageVerifier (generation 7: up to Chinese (Traditional), no language 6).
    void checkLanguage() {
        const uint8_t lang = pk_.language();
        if (lang == 0 || lang == 6) invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " is not a language");
        else if (lang > 10) invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " does not exist in the origin game");
    }

    // HyperTrainingVerifier: Let's Go trains at level 100, never a perfect IV.
    void checkHyperTraining() {
        const uint8_t flags = plain_[0xDE];
        if ((flags & 0x3F) == 0 || !personal_) return;
        if (level() < 100) {
            invalid(CheckIdentifier::Stats, "Hyper Training in Let's Go Pikachu/Eevee needs level 100");
            return;
        }
        int total = 0;
        for (size_t i = 0; i < 6; ++i) total += pk_.iv(i);
        if (total == 31 * 6) {
            invalid(CheckIdentifier::Stats, "Hyper Training with six perfect IVs");
            return;
        }
        // Let's Go's gold bottle cap trains all six, perfect IVs included (PKHeX skips Gen7b here).
        if ((flags & 0x3F) == 0x3F) return;
        for (size_t i = 0; i < 6; ++i) {
            if (pk_.iv(i) == 31 && ((flags >> HyperTrainBit[i]) & 1)) {
                invalid(CheckIdentifier::Stats, "Hyper Training on a perfect IV");
                return;
            }
        }
    }

    // MiscVerifierPB7: the stored CP is PB7.CalcCP, except for the partner Pikachu/Eevee.
    void checkCombatPower() {
        if (!personal_ || starter()) return;
        std::array<uint8_t, 6> ivs{}, avs{};
        const uint8_t flags = plain_[0xDE];
        for (size_t i = 0; i < 6; ++i) {
            ivs[i] = ((flags >> HyperTrainBit[i]) & 1) ? 31 : pk_.iv(i);
            avs[i] = av(i);
        }
        const uint8_t friendship = plain_[0x93] == 0 ? plain_[0xCA] : plain_[0xA2];
        const uint16_t expect = combatPower(*personal_, level(), ivs, avs, pk_.nature(), friendship);
        const uint16_t stored = PK6::read16(plain_, 0xFE);
        if (stored != expect)
            invalid(CheckIdentifier::Stats, "Stored CP " + std::to_string(stored) + " is not the calculated CP " + std::to_string(expect));
    }

    // MiscVerifierPB7.VerifyMoodSpirit: at rest in a box, spirit and mood are 100 (the partner
    // Pikachu/Eevee keeps its own). A party record or an unknown slot is not decided.
    void checkSpiritMood() {
        if (in_.slot != Slot::Box || starter()) return;
        if (plain_[0xAC] != InitialSpiritMood) invalid(CheckIdentifier::Misc, "Spirit is not 100 at rest in a box");
        if (plain_[0xAD] != InitialSpiritMood) invalid(CheckIdentifier::Misc, "Mood is not 100 at rest in a box");
    }

    // MiscVerifierPB7.VerifyReceivedDateTime: a valid time and a Switch-era date, equal to the
    // met date while untraded (GO Park records keep their GO met date).
    void checkReceivedDate() {
        if (plain_[0xCE] >= 24 || plain_[0xCF] >= 60 || plain_[0xD0] >= 60)
            invalid(CheckIdentifier::Misc, "Received time is not a time of day");
        const uint8_t y = plain_[0xCB], m = plain_[0xCC], d = plain_[0xCD];
        if (!validDate(y, m, d) || y > 60) {
            invalid(CheckIdentifier::Misc, "Received date is not a valid date");
            return;
        }
        if (untraded() && version_ != VersionGO && (y != pk_.metYear() || m != pk_.metMonth() || d != pk_.metDay()))
            invalid(CheckIdentifier::Misc, "Received date differs from the met date of an untraded record");
    }

    // MiscVerifierPB7.VerifyCalculatedSizes: stored height and weight are the values PB7
    // calculates from the scalars (Meltan's fixed-size gift and its evolution excepted).
    void checkSizes() {
        if (!personal_ || pk_.species() == Meltan || pk_.species() == Melmetal) return;
        const uint8_t hs = plain_[0x3A], ws = plain_[0x3B];
        if (readFloat(plain_, 0x2C) != heightAbsolute(personal_->height, hs))
            invalid(CheckIdentifier::Stats, "Stored height is not the height its scalar gives");
        if (readFloat(plain_, 0xE4) != weightAbsolute(personal_->weight, hs, ws))
            invalid(CheckIdentifier::Stats, "Stored weight is not the weight its scalars give");
    }

    void checkTrainer() {
        const auto& t = in_.trainer;
        if (!t.present || t.tid != pk_.tid() || t.sid != pk_.sid()) return;
        std::u16string saveOt;
        for (uint16_t ch : t.ot) {
            if (ch == 0) break;
            saveOt.push_back(static_cast<char16_t>(ch));
        }
        if (saveOt != pk_.originalTrainerName() || t.female != (pk_.originalTrainerGender() == 1))
            warning(CheckIdentifier::Trainer, "Trainer ID matches the save but the OT name or gender does not");
    }
};

// NatureAmp.AmplifyStat for a stat in visual order (Atk Def SpA SpD Spe). Natures order
// their boosted/lowered stat as Atk Def Spe SpA SpD; an out-of-range nature lowers every stat.
int amplify(uint8_t nature, size_t visual, int initial) noexcept {
    static constexpr std::array<uint8_t, 5> natureIndex = {0, 1, 3, 4, 2};
    if (nature >= 25) return 90 * initial / 100;
    const uint8_t up = nature / 5, down = nature % 5, idx = natureIndex[visual];
    if (up == down) return initial;
    if (up == idx) return 110 * initial / 100;
    if (down == idx) return 90 * initial / 100;
    return initial;
}

} // namespace

uint16_t combatPower(const Data::Personal& p, uint8_t level, const std::array<uint8_t, 6>& ivs,
                     const std::array<uint8_t, 6>& avs, uint8_t nature, uint8_t friendship) noexcept {
    // PB7.CPScalar: friendship raises stats by up to 10%. volatile keeps each single-precision
    // step separate (no fused multiply-add), as PKHeX's NoOptimization code does.
    volatile float scalarF = static_cast<float>(friendship) / 255.0f;
    scalarF = scalarF / 10.0f;
    scalarF = scalarF + 1.0f;
    scalarF = scalarF * 100.0f;
    const int scalar = static_cast<int>(scalarF);
    auto stat = [&](size_t i) { return (ivs[i] + 2 * p.baseStats[i]) * level / 100; };
    auto amped = [&](size_t i, size_t visual) {
        return static_cast<uint16_t>(amplify(nature, visual, stat(i) + 5) * scalar / 100);
    };
    // HP Atk Def Spe SpA SpD -> visual Atk 0, Def 1, SpA 2, SpD 3, Spe 4.
    const int statSum = static_cast<uint16_t>(stat(0)) + 10 + level + amped(1, 0) + amped(2, 1) + amped(3, 4) +
                        amped(4, 2) + amped(5, 3);
    volatile float base = static_cast<float>(statSum) * 6.0f;
    base = base * static_cast<float>(level);
    base = base / 100.0f;
    const int baseCp = static_cast<int>(base);
    int awakeSum = 0;
    for (uint8_t v : avs) awakeSum += v;
    int awakeCp = 0;
    if (awakeSum != 0) {
        volatile float s = static_cast<float>(level) * 4.0f;
        s = s / 100.0f;
        s = s + 2.0f;
        volatile float r = static_cast<float>(awakeSum) * s;
        awakeCp = static_cast<int>(r);
    }
    return static_cast<uint16_t>(std::min(10000, awakeCp + baseCp));
}

float heightAbsolute(uint16_t height, uint8_t heightScalar) noexcept {
    volatile float r = static_cast<float>(heightScalar) / 255.0f;
    r = r * 0.79999995f;
    r = r + 0.6f;
    volatile float out = r * static_cast<float>(height);
    return out;
}

float weightAbsolute(uint16_t weight, uint8_t heightScalar, uint8_t weightScalar) noexcept {
    volatile float h = static_cast<float>(heightScalar) / 255.0f;
    h = h * 0.79999995f;
    h = h + 0.6f;
    volatile float w = static_cast<float>(weightScalar) / 255.0f;
    w = w * 0.40000004f;
    w = w + 0.8f;
    volatile float weighted = w * static_cast<float>(weight);
    volatile float out = h * weighted;
    return out;
}

Report analyze(std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    if (record.size() != PK6::PartySize) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 260-byte PB7 record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    // The codec decodes stored bytes; a decrypted input is re-encrypted first so both paths
    // share one decoder (encryption is a bijection, so nothing is lost).
    std::vector<uint8_t> stored = input.encrypted ? std::vector<uint8_t>(record.begin(), record.end()) : PK6::encrypt(record);
    PK6::DecodeStatus status{};
    const auto pk = PK6::Pokemon6::decode(stored, &status);
    if (!pk) {
        report.coverage.structure = CoverageLevel::Partial;
        if (status == PK6::DecodeStatus::BadChecksum)
            report.issues.push_back({Severity::Invalid, "PB7 checksum does not match its data", CheckIdentifier::Checksum});
        else if (status == PK6::DecodeStatus::BadSanity)
            report.issues.push_back({Severity::Invalid, "PB7 sanity field is set (bad egg)", CheckIdentifier::Structure});
        else
            report.issues.push_back({Severity::Warning, "PB7 record could not be decoded", CheckIdentifier::Structure});
        return report;
    }
    if (pk->species() == 0) {
        if (!pk->allZero()) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the PB7 payload is not a native blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(*pk, input).run();
}

} // namespace Legality::Gen7b
