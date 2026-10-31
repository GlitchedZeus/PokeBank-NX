// Gen67Legality.cpp - PK6 / PK7 native legality (see include/Legality/Gen67Legality.h).
//
// Every Invalid below names the PKHeX.Core verifier it mirrors (pinned commit in
// .github/workflows/legality-gen67-pkhex-diff.yml). Rules that need an encounter match,
// an evolution chain, a learnset, a distribution record or a transfer receipt are not
// implemented here; their coverage stays None/Partial so the verdict stays Incomplete.
#include "Legality/Gen67Legality.h"

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "Integration/Gen6/Gen6PK6.h"
#include "Integration/Gen6/Gen6PersonalData.h"
#include "Integration/Gen7/Gen7PersonalData.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen67 {
namespace {

namespace PK6 = PokeVault::Integration::Gen6::PK6;
using Modern::Game;
using Modern::Slot;

// Species and move ids used by species-specific rules (PKHeX Species / Move enums).
enum : uint16_t {
    Pikachu = 25, Unown = 201, Marill = 183, Azumarill = 184, Giratina = 487, Arceus = 493,
    Rotom = 479, Shaymin = 492, Mothim = 414, Keldeo = 647, Genesect = 649, Kyurem = 646,
    Scatterbug = 664, Spewpa = 665, Vivillon = 666, Furfrou = 676, Meowstic = 678, Hoopa = 720,
    Rockruff = 744, Lycanroc = 745, Silvally = 773,
    // PKHeX SpeciesCategory.IsFixedGenderFromDual for species that exist in Gen VI/VII.
    Shedinja = 292, Wormadam = 413, Vespiquen = 416, Gallade = 475, Froslass = 478, Salazzle = 758,
};
enum : uint16_t {
    MoveScaryFace = 184, MoveSketch = 166, MoveSecretSword = 548, MoveFusionFlare = 558, MoveFusionBolt = 559,
};
constexpr uint16_t ItemGriseousOrb = 112;

constexpr uint8_t RatioMale = 0, RatioFemale = 254, RatioGenderless = 255;

bool fixedGenderFromDual(uint16_t species) noexcept {
    switch (species) {
        case Shedinja: case Wormadam: case Mothim: case Vespiquen: case Gallade: case Froslass:
        case Meowstic: case Salazzle: return true;
        default: return false;
    }
}

// PKHeX Legal.GetMaxLanguageID by origin generation.
uint8_t maxLanguage(uint8_t generation) noexcept {
    switch (generation) {
        case 1: case 3: return 7;           // Spanish
        case 2: case 4: case 5: case 6: return 8;  // Korean
        case 7: return 10;                  // Chinese (Traditional)
        default: return 0;
    }
}

constexpr bool isSunMoon(uint8_t version) noexcept { return version == 30 || version == 31; }

bool gameMatchesFormat(Game game, uint8_t format) noexcept {
    return Modern::nativeFormat(game) == (format == 6 ? Modern::Format::PK6 : Modern::Format::PK7);
}

/// Base stats for the party-stat consistency warning, from the existing generated
/// personal tables (#309 Gen6PersonalData.h, #311 Gen7PersonalData.h).
struct BaseStats {
    std::array<uint32_t, 6> stats;  // HP Atk Def Spe SpA SpD
};
template <typename Row, size_t N>
std::optional<BaseStats> baseStats(const std::array<Row, N>& table, uint16_t maxSpecies, uint16_t species, uint8_t form) noexcept {
    if (species == 0 || species > maxSpecies) return std::nullopt;
    const auto& base = table[species];
    const uint8_t count = base.formCount == 0 ? 1 : base.formCount;
    if (form >= count) return std::nullopt;
    const Row* row = &base;
    if (form != 0 && base.formStatsIndex != 0) {
        const size_t index = size_t{base.formStatsIndex} + form - 1;
        if (index >= table.size()) return std::nullopt;
        row = &table[index];
    }
    return BaseStats{{row->hp, row->atk, row->def, row->spe, row->spa, row->spd}};
}

class Analyzer {
public:
    Analyzer(uint8_t format, const PK6::Pokemon6& pk, const Modern::Input& input)
        : format_(format), pk_(pk), in_(input), plain_(pk.plain()) {}

    Report run() {
        report_.coverage.structure = CoverageLevel::Complete;
        report_.coverage.internal = CoverageLevel::Partial;
        report_.coverage.moves = CoverageLevel::Partial;
        report_.coverage.pidRng = CoverageLevel::Partial;   // EC/PID transfer and nature correlation only
        report_.coverage.transfer = CoverageLevel::Partial; // origin-format admission and Transporter PID only
        checkSource();
        const auto species = pk_.species();
        if (species > Data::maxSpecies(format_)) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species) + " does not exist in PK" +
                    std::to_string(format_) + " (PKHeX: species above the format's maximum)");
            return finish();
        }
        personal_ = Data::personal(format_, species, 0);
        originGen_ = originGeneration(pk_.originVersion());
        checkOrigin();
        checkForm();
        // Form-specific personal entry (abilities, gender ratio) when the form is in range.
        if (auto pf = Data::personal(format_, species, pk_.form())) personal_ = pf;
        checkLevel();
        checkEffortValues();
        checkMoves();
        checkPidEc();
        checkGender();
        checkAbility();
        checkNature();
        checkBall();
        checkLanguage();
        checkHeldItem();
        checkConsoleRegion();
        checkTransferMetLocation();
        if (originGen_ == 1 || originGen_ == 2) checkVirtualConsole();
        if (format_ == 7) checkHyperTraining();
        checkPartyStats();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            "Gen VI/VII encounter, learnset, event, breeding, ribbon, memory and transfer-history rules are not "
            "implemented yet; a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    uint8_t format_;
    const PK6::Pokemon6& pk_;
    const Modern::Input& in_;
    std::span<const uint8_t> plain_;
    Report report_{};
    std::optional<Data::Personal> personal_;
    uint8_t originGen_ = 0;

    Report finish() { return std::move(report_); }
    void invalid(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warning(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Warning, std::move(text), id}); }
    void info(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Info, std::move(text), id}); }

    bool boxSlot() const noexcept { return in_.slot == Slot::Box; }
    bool partySlot() const noexcept { return in_.slot == Slot::Party || pk_.partyForm(); }
    uint8_t level() const noexcept {
        return personal_ ? Pokemon::getLevelFromExp(pk_.experience(), personal_->growth) : 0;
    }
    uint32_t id32() const noexcept { return uint32_t{pk_.tid()} | (uint32_t{pk_.sid()} << 16); }
    uint8_t consoleRegion() const noexcept { return plain_[0xE2]; }
    bool hasMove(uint16_t move) const noexcept {
        for (size_t i = 0; i < 4; ++i)
            if (pk_.move(i) == move) return true;
        return false;
    }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (!gameMatchesFormat(in_.game, format_)) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, "The source game does not store PK" + std::to_string(format_) +
                    " records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
        // Game-native limits below the format's (X/Y below OR/AS, Sun/Moon below Ultra Sun/Ultra Moon).
        // Not a PKHeX rule (PKHeX checks the format, not the save), so a Warning, not Invalid.
        const uint16_t maxSpecies = Data::gameMaxSpecies(in_.game);
        if (maxSpecies && pk_.species() > maxSpecies)
            warning(CheckIdentifier::SourceGame, "Species " + std::to_string(pk_.species()) +
                    " is not in the source game's data");
        const uint16_t maxMove = Data::gameMaxMove(in_.game);
        for (size_t i = 0; i < 4; ++i) {
            if (maxMove && pk_.move(i) > maxMove && pk_.move(i) <= Data::maxMove(format_))
                warning(CheckIdentifier::SourceGame, "Move " + std::to_string(i + 1) + " is not in the source game's data");
        }
        const uint16_t maxItem = Data::gameMaxItem(in_.game);
        if (maxItem && pk_.heldItem() > maxItem)
            warning(CheckIdentifier::SourceGame, "Held item is not in the source game's data");
    }

    // EncounterFinder: an origin game id no game writes, or a game whose records cannot
    // reach this format, has no encounter at all.
    void checkOrigin() {
        const uint8_t version = pk_.originVersion();
        if (originGen_ == 0) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version) + " is not a game");
            return;
        }
        if (originGen_ > format_) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version) + " is newer than PK" +
                    std::to_string(format_));
            return;
        }
        if (format_ == 6 && originGen_ < 3) {
            invalid(CheckIdentifier::Origin, "Virtual Console origin cannot be stored in a PK6");
            return;
        }
        if (originGen_ < format_)
            info(CheckIdentifier::Transfer, "Transferred from generation " + std::to_string(originGen_));
    }

    // FormVerifier (format 6/7 branches).
    void checkForm() {
        const uint16_t species = pk_.species();
        const uint8_t form = pk_.form();
        const uint8_t count = Data::formCount(format_, species);
        if (count <= 1 && form == 0) return;
        bool inRange = form < count;
        if (!inRange) {
            switch (species) {
                case Unown: inRange = form < (originGen_ == 2 ? 26 : 28); break;
                case Mothim: inRange = form < 3; break;
                case Scatterbug: case Spewpa: inRange = form <= Data::vivillonMaxWildForm(); break;
                default: break;
            }
        }
        if (!inRange) {
            invalid(CheckIdentifier::Species, "Form " + std::to_string(form) + " is outside the species' forms");
            return;
        }
        const uint16_t item = pk_.heldItem();
        switch (species) {
            case Pikachu:
                if (originGen_ == 6 && form != 0 && format_ != 6)
                    return invalid(CheckIdentifier::Species, "Cosplay Pikachu cannot leave Generation VI");
                break;
            case Unown:
                if (originGen_ == 2 && form >= 26)
                    return invalid(CheckIdentifier::Species, "Unown form is outside the Generation II letters");
                if (originGen_ == 3 && unownForm3(pk_.encryptionConstant()) != form)
                    return invalid(CheckIdentifier::Species, "Unown letter does not match its Generation III PID");
                break;
            case Giratina:
                if ((form == 1) != (item == ItemGriseousOrb))
                    return invalid(CheckIdentifier::Items, "Giratina's form does not match the Griseous Orb");
                break;
            case Arceus:
                if (Data::arceusForm(format_, item) != form)
                    return invalid(CheckIdentifier::Items, "Arceus' form does not match its held Plate/Z-Crystal");
                return;
            case Keldeo:
                if (originGen_ != 5 && (form == 1) != hasMove(MoveSecretSword))
                    return invalid(CheckIdentifier::Moves, "Keldeo's form does not match Secret Sword");
                break;
            case Genesect:
                if (Data::genesectForm(item) != form)
                    return invalid(CheckIdentifier::Items, "Genesect's form does not match its held Drive");
                return;
            case Furfrou:
                if (format_ == 6 && form != 0 && boxSlot())
                    return invalid(CheckIdentifier::Species, "Trimmed Furfrou reverts when stored in a box");
                break;
            case Scatterbug: case Spewpa:
                if (form > Data::vivillonMaxWildForm())
                    return invalid(CheckIdentifier::Species, "Vivillon pattern is only obtainable as an event");
                if (!Data::vivillonPatternValid(form, consoleRegion()))
                    return invalid(CheckIdentifier::Species, "Vivillon pattern does not match the console region");
                break;
            case Vivillon:
                if (form > Data::vivillonMaxWildForm()) {
                    info(CheckIdentifier::EventGift, "Event-only Vivillon pattern: valid only for a matching gift");
                    break;
                }
                if (!Data::vivillonPatternValid(form, consoleRegion()))
                    return invalid(CheckIdentifier::Species, "Vivillon pattern does not match the console region");
                break;
            case Meowstic:
                if ((form & 1) != pk_.gender())
                    return invalid(CheckIdentifier::Species, "Meowstic's form does not match its gender");
                break;
            case Silvally:
                if (Data::silvallyForm(item) != form)
                    return invalid(CheckIdentifier::Items, "Silvally's form does not match its held Memory");
                return;
            case Rockruff:
                if (originGen_ == 7 && form == 1 && isSunMoon(pk_.originVersion()))
                    return invalid(CheckIdentifier::Species, "Own Tempo Rockruff does not exist in Sun/Moon");
                break;
            case Lycanroc:
                if (originGen_ == 7 && form == 2 && isSunMoon(pk_.originVersion()))
                    return invalid(CheckIdentifier::Species, "Dusk Form Lycanroc does not exist in Sun/Moon");
                break;
            case Rotom:
                if (pk_.isEgg() && form != 0) return invalid(CheckIdentifier::Egg, "An egg cannot have this form");
                break;
            default: break;
        }
        if (species == Furfrou && pk_.isEgg() && form != 0)
            return invalid(CheckIdentifier::Egg, "An egg cannot have this form");
        if ((species == Shaymin || species == Furfrou || species == Hoopa) && form != 0 && boxSlot() && format_ <= 6)
            return invalid(CheckIdentifier::Species, "This form reverts when stored in a box");
        if (Data::battleOnlyForm(format_, species, form))
            invalid(CheckIdentifier::Species, "Battle-only form outside battle");
    }

    // LevelVerifier (non-gift, non-egg branch).
    void checkLevel() {
        if (!personal_ || pk_.isEgg()) return;
        const uint8_t lvl = level();
        if (lvl >= 100) {
            const uint32_t expect = Pokemon::getExpForLevel(100, personal_->growth);
            if (pk_.experience() != expect)
                invalid(CheckIdentifier::Stats, "Experience is above the level-100 total");
        }
        if (lvl < pk_.metLevel())
            invalid(CheckIdentifier::Stats, "Current level is below the met level");
    }

    // EffortValueVerifier (format >= 6).
    void checkEffortValues() {
        int sum = 0;
        bool above252 = false;
        for (size_t i = 0; i < 6; ++i) {
            sum += pk_.ev(i);
            above252 = above252 || pk_.ev(i) > 252;
        }
        if (pk_.isEgg()) {
            if (sum != 0) invalid(CheckIdentifier::Egg, "An egg cannot have EVs");
            return;
        }
        if (sum > 510) invalid(CheckIdentifier::Stats, "EV total is above 510");
        else if (above252) invalid(CheckIdentifier::Stats, "An EV is above 252");
    }

    // LearnVerifier.Finalize, FlagFormExclusiveKyurem and MovePPVerifier.
    void checkMoves() {
        std::array<uint16_t, 4> m{};
        for (size_t i = 0; i < 4; ++i) m[i] = pk_.move(i);
        const uint16_t maxMove = Data::maxMove(format_);
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] > maxMove) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " does not exist");
        }
        if (m[0] == 0) invalid(CheckIdentifier::Moves, "The first move slot is empty");
        bool empty = false;
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] == 0) { empty = true; continue; }
            if (empty) { invalid(CheckIdentifier::Moves, "An empty move slot comes before a move"); empty = false; continue; }
            for (size_t j = i + 1; j < 4; ++j)
                if (m[j] == m[i]) { invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " is duplicated"); break; }
        }
        if (pk_.species() == Kyurem && !pk_.isEgg()) {
            const uint8_t form = pk_.form();
            if (form != 1 && hasMove(MoveFusionFlare)) invalid(CheckIdentifier::Moves, "Only White Kyurem knows Fusion Flare");
            if (form != 2 && hasMove(MoveFusionBolt)) invalid(CheckIdentifier::Moves, "Only Black Kyurem knows Fusion Bolt");
            if (form != 0 && hasMove(MoveScaryFace)) invalid(CheckIdentifier::Moves, "Fused Kyurem cannot know Scary Face");
        }
        // PP.
        if (pk_.isEgg()) {
            for (size_t i = 0; i < 4; ++i) {
                if (pk_.movePPUps(i) != 0) { invalid(CheckIdentifier::Egg, "An egg cannot have PP Ups"); break; }
            }
            for (size_t i = 0; i < 4; ++i) {
                if (pk_.movePP(i) != Data::basePP(format_, m[i])) { invalid(CheckIdentifier::Egg, "An egg's PP is not its moves' base PP"); break; }
            }
            return;
        }
        for (size_t i = 0; i < 4; ++i) {
            if (pk_.movePPUps(i) != 0 && (m[i] == 0 || m[i] == MoveSketch))
                invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
        }
        // PK7 Virtual Console records keep their Gen I/II PP until they leave the box
        // (MovePPVerifier.IsVirtualConsoleUntouched); that exception needs the VC PP
        // tables, so a boxed VC record's PP is left undecided here.
        const bool vcBoxed = format_ == 7 && originGen_ <= 2 && !partySlot();
        for (size_t i = 0; i < 4; ++i) {
            const uint32_t ups = pk_.movePPUps(i);
            const uint32_t healed = Data::basePP(format_, m[i]) * (5 + ups) / 5;
            if (pk_.movePP(i) > healed) {
                if (vcBoxed) info(CheckIdentifier::Moves, "Virtual Console PP above the 3DS value (allowed while untouched in a box)");
                else invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
            }
        }
    }

    // PIDVerifier.VerifyEC (format >= 6): Gen III-V origins carry the Transporter PID;
    // Gen I/II (Virtual Console) and Gen VI/VII origins have PID != EC.
    void checkPidEc() {
        const uint32_t pid = pk_.pid(), ec = pk_.encryptionConstant();
        if (originGen_ >= 3 && originGen_ <= 5) {
            if (pid != transferPid(ec, id32()))
                invalid(CheckIdentifier::PidRng, "PID does not match the Poke Transporter PID for its encryption constant");
        } else if (originGen_ != 0 && pid == ec) {
            invalid(CheckIdentifier::PidRng, "PID equals the encryption constant");
        }
    }

    // GenderVerifier and PKM.IsGenderValid.
    void checkGender() {
        if (!personal_) return;
        const uint8_t gender = pk_.gender();
        const uint8_t ratio = personal_->gender;
        if ((ratio == RatioGenderless) != (gender == 2)) {
            invalid(CheckIdentifier::Misc, "Gender does not match the species' gender ratio");
            return;
        }
        if (originGen_ >= 3 && originGen_ <= 5) {
            bool valid;
            if (ratio == RatioGenderless) valid = gender == 2;
            else if (ratio == RatioFemale) valid = gender == 1;
            else if (ratio == RatioMale) valid = gender == 0;
            else valid = gender < 2 && gender == (((pk_.pid() & 0xFF) < ratio) ? 1 : 0);
            if (!valid) {
                const uint8_t lowEc = static_cast<uint8_t>(pk_.encryptionConstant());
                // Azurill (3F:1M) evolved in Gen IV/V keeps its PID; Marill is 1:1.
                const bool azurill = (pk_.species() == Marill || pk_.species() == Azumarill) &&
                                     gender == 1 && lowEc >= 127 && lowEc < 191;
                if (!azurill) invalid(CheckIdentifier::Misc, "Gender does not match the PID of a Generation III-V origin");
            } else if (fixedGenderFromDual(pk_.species())) {
                info(CheckIdentifier::Misc, "Gender of the original (pre-evolution) species is not checked");
            }
            return;
        }
        if ((ratio == RatioFemale && gender != 1) || (ratio == RatioMale && gender != 0))
            invalid(CheckIdentifier::Misc, "Gender does not match the species' fixed gender");
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
        const uint8_t number = pk_.abilityNumber();
        if (number != 1 && number != 2 && number != 4) {
            invalid(CheckIdentifier::Ability, "Ability number is not 1, 2 or 4");
            return;
        }
        if (slots[number >> 1] != ability)
            invalid(CheckIdentifier::Ability, "Ability does not match the ability number");
    }

    // PIDVerifier (Nature.IsFixed) and GenderVerifier.VerifyNaturePID (Gen III/IV origins).
    void checkNature() {
        const uint8_t nature = pk_.nature();
        if (nature >= 25) {
            invalid(CheckIdentifier::Misc, "Nature is out of range");
            return;
        }
        if ((originGen_ == 3 || originGen_ == 4) && pk_.encryptionConstant() % 25 != nature)
            invalid(CheckIdentifier::PidRng, "Nature does not match the PID of a Generation III/IV origin");
    }

    // BallVerifier: no encounter uses ball 0 or a ball above the format's last ball.
    void checkBall() {
        const uint8_t ball = pk_.ball();
        if (ball == 0 || ball > Data::maxBall(format_))
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball) + " does not exist");
    }

    // LanguageVerifier.
    void checkLanguage() {
        const uint8_t lang = pk_.language();
        if (lang == 0 || lang == 6) {
            invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " is not a language");
            return;
        }
        const uint8_t max = maxLanguage(originGen_);
        if (max && lang > max) {
            invalid(CheckIdentifier::Trainer, "Language " + std::to_string(lang) + " does not exist in the origin game");
            return;
        }
        if (originGen_ == 1 || originGen_ == 2) {
            const uint8_t v = pk_.originVersion();
            if (lang == 8 && v != 39 && v != 40)
                invalid(CheckIdentifier::Trainer, "Korean exists only for Gold/Silver Virtual Console");
            if (lang != 1 && v == 37)
                invalid(CheckIdentifier::Trainer, "Blue Virtual Console exists only in Japanese");
        }
    }

    // ItemVerifier.
    void checkHeldItem() {
        const uint16_t item = pk_.heldItem();
        if (pk_.isEgg() && item != 0) invalid(CheckIdentifier::Egg, "An egg cannot hold an item");
        if (!Data::heldItemAllowed(format_, item))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(item) + " is not obtainable in this generation");
    }

    // ConsoleRegionVerifier: the 3DS region byte and the country it allows.
    void checkConsoleRegion() {
        const uint8_t region = consoleRegion(), country = plain_[0xE0];
        if (region >= 7) invalid(CheckIdentifier::Trainer, "Console region " + std::to_string(region) + " does not exist");
        else if (!Data::consoleRegionCountryValid(region, country))
            invalid(CheckIdentifier::Trainer, "Country is not in the console's region");
    }

    // TransferVerifier.VerifyTransferLegalityG3 / G4 (encounter generation 3 or 4, format >= 5):
    // Poke Transporter writes met location 30001, or a Lock Capsule / Crown Beast location.
    void checkTransferMetLocation() {
        if (originGen_ != 3 && originGen_ != 4) return;
        const uint16_t loc = pk_.metLocation();
        if (loc == 30001) return;
        if (originGen_ == 4) {
            const uint16_t s = pk_.species();
            if (s == 251 && (loc == 30010 || loc == 30011)) return;               // Celebi
            if ((s == 243 || s == 244 || s == 245) && (loc == 30012 || loc == 30013)) return;  // Raikou, Entei, Suicune
        }
        invalid(CheckIdentifier::Transfer, "Met location is not Poke Transporter's for a Generation " +
                std::to_string(originGen_) + " origin");
    }

    // TransferVerifier.VerifyVCEncounter / VerifyTransferLegalityG12 (PK7 Virtual Console
    // transfers). Poke Transporter writes met location 30013 for a Red/Green/Blue/Yellow
    // record (30017 when its family is Gen II-only) and 30017 for Gold/Silver/Crystal.
    void checkVirtualConsole() {
        if (format_ != 7) return;
        const uint8_t version = pk_.originVersion();
        const uint16_t metLocation = pk_.metLocation();
        if (metLocation != 30017 && !(version >= 35 && version <= 38 && metLocation == 30013))
            invalid(CheckIdentifier::Transfer, "Met location is not Poke Transporter's for a Virtual Console origin");
        if (pk_.originalTrainerGender() == 1 && pk_.originVersion() != 41)
            invalid(CheckIdentifier::Trainer, "Only Crystal has a female OT among the Virtual Console games");
        const uint8_t met = pk_.metLevel();
        if (met == 100) {
            if (pk_.experience() % 25 != pk_.nature())
                invalid(CheckIdentifier::Transfer, "Nature does not match the experience at transfer (EXP % 25)");
        } else if (met <= 2 && personal_ && pk_.nature() < 25 &&
                   !Data::vcNatureValidAtMetLevel2(personal_->growth, pk_.nature())) {
            invalid(CheckIdentifier::Transfer, "No transferred level-2 experience gives this nature");
        }
        if (!Data::vcLanguageValid(consoleRegion(), pk_.language()))
            invalid(CheckIdentifier::Trainer, "The Virtual Console game's language is not sold in the console's region");
    }

    // HyperTrainingVerifier (Gen VII: level 100). PK7 flags at 0xDE: bit0 HP, 1 Atk,
    // 2 Def, 3 SpA, 4 SpD, 5 Spe; storage IVs are HP Atk Def Spe SpA SpD.
    void checkHyperTraining() {
        const uint8_t flags = plain_[0xDE];
        if ((flags & 0x3F) == 0) return;
        if (!personal_) return;
        if (level() < 100) {
            invalid(CheckIdentifier::Stats, "Hyper Training needs level 100");
            return;
        }
        int total = 0;
        for (size_t i = 0; i < 6; ++i) total += pk_.iv(i);
        if (total == 31 * 6) {
            invalid(CheckIdentifier::Stats, "Hyper Training with six perfect IVs");
            return;
        }
        static constexpr std::array<uint8_t, 6> bitForIv = {0, 1, 2, 5, 3, 4};
        for (size_t i = 0; i < 6; ++i) {
            if (pk_.iv(i) == 31 && ((flags >> bitForIv[i]) & 1)) {
                invalid(CheckIdentifier::Stats, "Hyper Training on a perfect IV");
                return;
            }
        }
    }

    // Not a PKHeX rule: a party record's cached level and stats are rewritten by the game
    // whenever it enters the party, so a mismatch means the bytes were edited. Warning.
    void checkPartyStats() {
        if (!pk_.partyForm() || pk_.isEgg() || !personal_) return;
        const uint8_t lvl = level();
        if (pk_.statLevel() != lvl) {
            warning(CheckIdentifier::Stats, "Party level byte does not match the level from experience");
            return;
        }
        const auto base = format_ == 6
            ? baseStats(PokeVault::Integration::Gen6::PersonalAO, Data::maxSpecies(6), pk_.species(), pk_.form())
            : baseStats(PokeVault::Integration::Gen7::PersonalUSUM, Data::maxSpecies(7), pk_.species(), pk_.form());
        if (!base) return;
        const uint8_t ht = format_ == 7 ? plain_[0xDE] : 0;
        static constexpr std::array<uint8_t, 6> bitForIv = {0, 1, 2, 5, 3, 4};
        std::array<uint32_t, 6> out{};
        for (size_t i = 0; i < 6; ++i) {
            const uint32_t iv = ((ht >> bitForIv[i]) & 1) ? 31u : pk_.iv(i);
            const uint32_t core = iv + 2 * base->stats[i] + pk_.ev(i) / 4;
            out[i] = i == 0 ? (base->stats[0] == 1 ? 1 : (core + 100) * lvl / 100 + 10) : core * lvl / 100 + 5;
        }
        const uint8_t nature = pk_.nature();
        if (nature < 25) {
            const size_t up = nature / 5 + 1, down = nature % 5 + 1;
            if (up != down) {
                out[up] = out[up] * 11 / 10;
                out[down] = out[down] * 9 / 10;
            }
        }
        for (size_t i = 0; i < 6; ++i) {
            if (pk_.stat(i + 1) != out[i]) {
                warning(CheckIdentifier::Stats, "Party stats do not match the stats its level, IVs, EVs and nature give");
                return;
            }
        }
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

} // namespace

uint8_t originGeneration(uint8_t version) noexcept {
    switch (version) {
        case 1: case 2: case 3: case 4: case 5: case 15: return 3;
        case 7: case 8: case 10: case 11: case 12: return 4;
        case 20: case 21: case 22: case 23: return 5;
        case 24: case 25: case 26: case 27: return 6;
        case 30: case 31: case 32: case 33: return 7;
        case 35: case 36: case 37: case 38: return 1;
        case 39: case 40: case 41: return 2;
        default: return 0;
    }
}

Report analyze(uint8_t format, std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    const std::string name = "PK" + std::to_string(format);
    if (!PK6::supportedSize(record.size())) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 232-byte or 260-byte " + name + " record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    // The codec decodes stored bytes; a decrypted input is re-encrypted first so both
    // paths share one decoder (encryption is a bijection, so nothing is lost).
    std::vector<uint8_t> stored = input.encrypted ? std::vector<uint8_t>(record.begin(), record.end()) : PK6::encrypt(record);
    PK6::DecodeStatus status{};
    const auto pk = PK6::Pokemon6::decode(stored, &status);
    if (!pk) {
        report.coverage.structure = CoverageLevel::Partial;
        if (status == PK6::DecodeStatus::BadChecksum)
            report.issues.push_back({Severity::Invalid, name + " checksum does not match its data", CheckIdentifier::Checksum});
        else if (status == PK6::DecodeStatus::BadSanity)
            report.issues.push_back({Severity::Invalid, name + " sanity field is set (bad egg)", CheckIdentifier::Structure});
        else
            report.issues.push_back({Severity::Warning, name + " record could not be decoded", CheckIdentifier::Structure});
        return report;
    }
    if (pk->species() == 0) {
        // A cleared slot decodes to all zero. A nonzero payload with species zero is
        // ambiguous, not evidence of a native blank slot.
        if (!pk->allZero()) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the " + name + " payload is not a native blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(format, *pk, input).run();
}

} // namespace Legality::Gen67
