// Gen8Legality.cpp - PK8 (Sword/Shield) native legality (see include/Legality/Gen8Legality.h).
//
// Every Invalid below names the PKHeX.Core verifier it mirrors (pinned commit in
// .github/workflows/legality-gen67-pkhex-diff.yml). Rules that need an encounter match, an
// evolution chain, a learnset, a distribution record or the HOME server are not implemented
// here; their coverage stays None/Partial so the verdict stays Incomplete.
#include "Legality/Gen8Legality.h"

#include <algorithm>
#include <string>
#include <vector>

#include "Integration/Gen8/Gen8PK8.h"
#include "Legality/Gen67Legality.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen8 {
namespace {

namespace PK8 = PokeVault::Integration::Gen8::PK8;
using Modern::Game;
using Modern::Slot;

// Species, move and item ids used by species-specific rules (PKHeX Species / Move enums).
enum : uint16_t {
    Pikachu = 25, Unown = 201, Marill = 183, Azumarill = 184, Giratina = 487, Arceus = 493, Rotom = 479,
    Mothim = 414, Keldeo = 647, Genesect = 649, Kyurem = 646, Scatterbug = 664, Spewpa = 665,
    Furfrou = 676, Meowstic = 678, Hoopa = 720, Rockruff = 744, Lycanroc = 745, Silvally = 773,
    Zacian = 888, Zamazenta = 889, Eternatus = 890, Calyrex = 898,
    // PKHeX SpeciesCategory.IsFixedGenderFromDual for species that exist in PK8.
    Shedinja = 292, Wormadam = 413, Vespiquen = 416, Gallade = 475, Froslass = 478, Salazzle = 758,
};
enum : uint16_t {
    MoveSketch = 166, MoveSecretSword = 548, MoveFusionFlare = 558, MoveFusionBolt = 559,
    MoveHyperspaceHole = 593, MoveHyperspaceFury = 621, MoveRevivalBlessing = 863,
};
constexpr uint16_t ItemGriseousOrb = 112;
constexpr uint8_t RatioMale = 0, RatioFemale = 254, RatioGenderless = 255;
constexpr uint8_t VersionSW = 44, VersionSH = 45;

// LocationsHOME: the met location a side transfer into Sword/Shield writes.
enum : uint16_t { SHLG = 59994, SWFR = 59995, SHVL = 59996, SWSL = 59997, SHSP = 59998, SWBD = 59999, SWLA = 60000 };
constexpr uint16_t LocationGO7 = 50, LocationGO8 = 30012, LocationTransfer4 = 30001;
constexpr uint16_t LocationTransfer1 = 30013, LocationTransfer2 = 30017;

bool fixedGenderFromDual(uint16_t species) noexcept {
    switch (species) {
        case Shedinja: case Wormadam: case Mothim: case Vespiquen: case Gallade: case Froslass:
        case Meowstic: case Salazzle: return true;
        default: return false;
    }
}

// PKHeX Legal.GetMaxLanguageID(generation, EntityContext.Gen8).
uint8_t maxLanguage(uint8_t generation) noexcept {
    switch (generation) {
        case 1: case 3: return 7;                  // Spanish
        case 2: case 4: case 5: case 6: return 8;  // Korean
        case 7: case 8: case 9: return 10;         // Chinese (Traditional)
        default: return 0;
    }
}

constexpr bool isSunMoon(uint8_t version) noexcept { return version == 30 || version == 31; }

class Analyzer {
public:
    Analyzer(const PK8::Pokemon8& pk, const Modern::Input& input) : pk_(pk), in_(input), plain_(pk.plain()) {}

    Report run() {
        report_.coverage.structure = CoverageLevel::Complete;
        report_.coverage.internal = CoverageLevel::Partial;
        report_.coverage.moves = CoverageLevel::Partial;
        report_.coverage.pidRng = CoverageLevel::Partial;   // EC/PID transfer and nature correlation only
        report_.coverage.transfer = CoverageLevel::Partial; // presence in Sword/Shield and transfer markers only
        checkSource();
        const uint16_t species = pk_.species();
        if (species > Data::maxSpecies()) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species) +
                    " does not exist in PK8 (PKHeX: species above the format's maximum)");
            return finish();
        }
        version_ = pk_.originVersion();
        met_ = PK8::read16(plain_, 0x122);
        side_ = sideTransferOrigin(met_);
        originGen_ = originGeneration(version_, met_);
        personal_ = Data::personal(species, form());
        checkOrigin();
        checkPresence();
        checkForm();
        // Base-form personal entry for a form outside the table (already Invalid above).
        if (!personal_) personal_ = Data::personal(species, 0);
        // Species/forms absent from Sword/Shield have empty personal rows; their checks are moot.
        if (personal_ && !personal_->present) personal_.reset();
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
        checkTransferMetLocation();
        if (originGen_ == 1 || originGen_ == 2) checkVirtualConsole();
        checkHyperTraining();
        checkSwordShieldFields();
        checkTechRecords();
        checkPartyStats();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            "Sword/Shield encounter, learnset, event, breeding, ribbon, memory, Gigantamax and HOME-history rules "
            "are not implemented yet; a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    const PK8::Pokemon8& pk_;
    const Modern::Input& in_;
    std::span<const uint8_t> plain_;
    Report report_{};
    std::optional<Data::Personal> personal_;
    uint8_t version_ = 0;
    uint16_t met_ = 0;
    uint8_t side_ = 0;
    uint8_t originGen_ = 0;

    Report finish() { return std::move(report_); }
    void invalid(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warning(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Warning, std::move(text), id}); }
    void info(CheckIdentifier id, std::string text) { report_.issues.push_back({Severity::Info, std::move(text), id}); }

    // G8PKM.Form reads one byte; the high byte of the 16-bit field is not part of the form.
    uint8_t form() const noexcept { return plain_[0x24]; }
    bool partySlot() const noexcept { return in_.slot == Slot::Party || pk_.size() == PK8::PartySize; }
    uint8_t level() const noexcept {
        return personal_ ? Pokemon::getLevelFromExp(pk_.experience(), personal_->growth) : 0;
    }
    uint32_t id32() const noexcept { return uint32_t{pk_.tid()} | (uint32_t{pk_.sid()} << 16); }
    uint8_t movePP(size_t i) const noexcept { return plain_[0x7A + i]; }
    uint8_t movePPUps(size_t i) const noexcept { return plain_[0x7E + i]; }
    bool hasMove(uint16_t move) const noexcept {
        for (size_t i = 0; i < 4; ++i)
            if (pk_.move(i) == move) return true;
        return false;
    }
    // PK8.VC3: a Switch Virtual Console FireRed/LeafGreen record moved in through HOME.
    bool vc3() const noexcept { return met_ == SWFR || met_ == SHLG; }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (Modern::nativeFormat(in_.game) != Modern::Format::PK8) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, "The source game does not store PK8 records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
    }

    // EncounterFinder: a version no game writes into a PK8 has no encounter at all, and
    // Legends: Z-A records cannot be moved into Sword/Shield.
    void checkOrigin() {
        if (originGen_ == 0) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version_) + " cannot be stored in a PK8");
            return;
        }
        if (originGen_ > 8) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version_) + " is newer than PK8");
            return;
        }
        if (side_ != 0)
            info(CheckIdentifier::Transfer, "Moved into Sword/Shield through HOME from game " + std::to_string(side_));
        else if (originGen_ < 8)
            info(CheckIdentifier::Transfer, "Transferred from generation " + std::to_string(originGen_));
    }

    // TransferVerifier.VerifyTransferLegalityG8: Sword/Shield holds a selection of species
    // and forms; any other species/form cannot be in a PK8.
    void checkPresence() {
        if (!Data::presentInGame(pk_.species(), form()))
            invalid(CheckIdentifier::Transfer, "Species/form is not present in Sword/Shield");
    }

    // FormVerifier (format 8 branches).
    void checkForm() {
        const uint16_t species = pk_.species();
        const uint8_t f = form();
        const uint8_t count = Data::formCount(species);
        if (count <= 1 && f == 0) return;
        bool inRange = f < count;
        if (!inRange) {
            switch (species) {
                case Unown: inRange = f < 28; break;
                case Mothim: inRange = f < 3; break;
                case Scatterbug: case Spewpa: inRange = f <= Gen67::Data::vivillonMaxWildForm(); break;
                default: break;
            }
        }
        if (!inRange) {
            invalid(CheckIdentifier::Species, "Form " + std::to_string(f) + " is outside the species' forms");
            return;
        }
        const uint16_t item = pk_.heldItem();
        switch (species) {
            case Pikachu:
                if (originGen_ == 6 && f != 0)
                    return invalid(CheckIdentifier::Species, "Cosplay Pikachu cannot leave Generation VI");
                break;
            case Unown:
                if (originGen_ == 2 && f >= 26)
                    return invalid(CheckIdentifier::Species, "Unown form is outside the Generation II letters");
                if (originGen_ == 3 && Gen67::unownForm3(pk_.encryptionConstant()) != f)
                    return invalid(CheckIdentifier::Species, "Unown letter does not match its Generation III PID");
                break;
            case Giratina:
                if ((f == 1) != (item == ItemGriseousOrb))
                    return invalid(CheckIdentifier::Items, "Giratina's form does not match the Griseous Orb");
                break;
            case Arceus:
                if (Data::arceusForm(item) != f)
                    return invalid(CheckIdentifier::Items, "Arceus' form does not match its held Plate");
                return;
            case Keldeo:
                // Generation VIII forces Keldeo's form to match Secret Sword for every origin.
                if ((f == 1) != hasMove(MoveSecretSword))
                    return invalid(CheckIdentifier::Moves, "Keldeo's form does not match Secret Sword");
                break;
            case Genesect:
                if (Data::genesectForm(item) != f)
                    return invalid(CheckIdentifier::Items, "Genesect's form does not match its held Drive");
                return;
            case Scatterbug: case Spewpa:
                if (f > Gen67::Data::vivillonMaxWildForm())
                    return invalid(CheckIdentifier::Species, "Vivillon pattern is only obtainable as an event");
                break;
            case Meowstic:
                if ((f & 1) != pk_.gender())
                    return invalid(CheckIdentifier::Species, "Meowstic's form does not match its gender");
                break;
            case Silvally:
                if (Data::silvallyForm(item) != f)
                    return invalid(CheckIdentifier::Items, "Silvally's form does not match its held Memory");
                return;
            case Rockruff:
                if (originGen_ == 7 && f == 1 && isSunMoon(version_))
                    return invalid(CheckIdentifier::Species, "Own Tempo Rockruff does not exist in Sun/Moon");
                break;
            case Lycanroc:
                if (originGen_ == 7 && f == 2 && isSunMoon(version_))
                    return invalid(CheckIdentifier::Species, "Dusk Form Lycanroc does not exist in Sun/Moon");
                break;
            case Rotom: case Furfrou:
                if (pk_.isEgg() && f != 0) return invalid(CheckIdentifier::Egg, "An egg cannot have this form");
                break;
            default: break;
        }
        if (Data::battleOnlyForm(species, f))
            invalid(CheckIdentifier::Species, "Battle-only form outside battle");
    }

    // LevelVerifier (non-gift, non-egg branch).
    void checkLevel() {
        if (!personal_ || pk_.isEgg()) return;
        const uint8_t lvl = level();
        if (lvl >= 100 && pk_.experience() != Pokemon::getExpForLevel(100, personal_->growth))
            invalid(CheckIdentifier::Stats, "Experience is above the level-100 total");
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

    // LearnVerifier.Finalize, FlagFormExclusiveHoopa/Kyurem (format 8) and MovePPVerifier.
    void checkMoves() {
        std::array<uint16_t, 4> m{};
        for (size_t i = 0; i < 4; ++i) m[i] = pk_.move(i);
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] > Data::maxMove()) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " does not exist");
        }
        if (m[0] == 0) invalid(CheckIdentifier::Moves, "The first move slot is empty");
        bool empty = false;
        for (size_t i = 0; i < 4; ++i) {
            if (m[i] == 0) { empty = true; continue; }
            if (empty) { invalid(CheckIdentifier::Moves, "An empty move slot comes before a move"); empty = false; continue; }
            for (size_t j = i + 1; j < 4; ++j)
                if (m[j] == m[i]) { invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " is duplicated"); break; }
        }
        if (!pk_.isEgg()) {
            const uint8_t f = form();
            if (pk_.species() == Kyurem) {
                if (f != 1 && hasMove(MoveFusionFlare)) invalid(CheckIdentifier::Moves, "Only White Kyurem knows Fusion Flare");
                if (f != 2 && hasMove(MoveFusionBolt)) invalid(CheckIdentifier::Moves, "Only Black Kyurem knows Fusion Bolt");
            }
            if (pk_.species() == Hoopa && hasMove(f != 0 ? MoveHyperspaceHole : MoveHyperspaceFury))
                invalid(CheckIdentifier::Moves, "Hoopa's form does not match its signature move");
        }
        if (pk_.isEgg()) {
            for (size_t i = 0; i < 4; ++i) {
                if (movePPUps(i) != 0) { invalid(CheckIdentifier::Egg, "An egg cannot have PP Ups"); break; }
            }
            for (size_t i = 0; i < 4; ++i) {
                if (movePP(i) != Data::basePP(m[i])) { invalid(CheckIdentifier::Egg, "An egg's PP is not its moves' base PP"); break; }
            }
            return;
        }
        for (size_t i = 0; i < 4; ++i) {
            if (movePPUps(i) != 0 && (m[i] == 0 || m[i] == MoveSketch || m[i] == MoveRevivalBlessing))
                invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
        }
        for (size_t i = 0; i < 4; ++i) {
            const uint32_t healed = Data::basePP(m[i]) * (5u + movePPUps(i)) / 5u;
            if (movePP(i) > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
        }
    }

    // PIDVerifier.VerifyEC (format >= 6). Gen III-V origins carry the Poke Transporter PID;
    // Switch Virtual Console records carry HOME's PID (not checked here); everything else
    // has PID != EC, except fixed-EC Legends: Arceus / Brilliant Diamond / Shining Pearl
    // gifts, which a record moved in from those games may be.
    void checkPidEc() {
        if (vc3()) return;
        const uint32_t pid = pk_.pid(), ec = pk_.encryptionConstant();
        if (originGen_ >= 3 && originGen_ <= 5) {
            if (pid != Gen67::transferPid(ec, id32()))
                invalid(CheckIdentifier::PidRng, "PID does not match the Poke Transporter PID for its encryption constant");
            return;
        }
        const bool fixedEcGift = side_ != 0 || version_ == 47 || version_ == 48 || version_ == 49;
        if (originGen_ != 0 && pid == ec && !fixedEcGift)
            invalid(CheckIdentifier::PidRng, "PID equals the encryption constant");
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

    // PIDVerifier (Nature.IsFixed), GenderVerifier.VerifyNaturePID (Gen III/IV origins) and
    // MiscVerifierHelpers.VerifyStatAlignment (the stat nature is the nature or a mint).
    void checkNature() {
        const uint8_t nature = pk_.nature();
        if (nature >= 25) {
            invalid(CheckIdentifier::Misc, "Nature is out of range");
            return;
        }
        if ((originGen_ == 3 || originGen_ == 4) && pk_.encryptionConstant() % 25 != nature)
            invalid(CheckIdentifier::PidRng, "Nature does not match the PID of a Generation III/IV origin");
        const uint8_t stat = pk_.statNature();
        if (stat != nature && !Data::mintNature(stat))
            invalid(CheckIdentifier::Misc, "Stat nature is neither the nature nor a mint");
    }

    // BallVerifier: no Sword/Shield record holds ball 0 or a ball above Beast Ball (Legends:
    // Arceus balls are replaced by a Poke Ball when the record moves to Sword/Shield).
    void checkBall() {
        const uint8_t ball = pk_.ball();
        if (ball == 0 || ball > Data::maxBall())
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball) + " cannot be held by a PK8");
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
            if (lang == 8 && version_ != 39 && version_ != 40)
                invalid(CheckIdentifier::Trainer, "Korean exists only for Gold/Silver Virtual Console");
            if (lang != 1 && version_ == 37)
                invalid(CheckIdentifier::Trainer, "Blue Virtual Console exists only in Japanese");
        }
    }

    // ItemVerifier.
    void checkHeldItem() {
        const uint16_t item = pk_.heldItem();
        if (pk_.isEgg() && item != 0) invalid(CheckIdentifier::Egg, "An egg cannot hold an item");
        if (!Data::heldItemAllowed(item))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(item) + " is not obtainable in Sword/Shield");
    }

    // TransferVerifier.VerifyTransferLegalityG3 / G4: a Gen III/IV origin (by version) moved
    // with Poke Transporter keeps met location 30001 (Lock Capsule / Crown Beast exceptions).
    void checkTransferMetLocation() {
        if (originGen_ != 3 && originGen_ != 4) return;
        if (met_ == LocationTransfer4) return;
        if (originGen_ == 4) {
            const uint16_t s = pk_.species();
            if (s == 251 && (met_ == 30010 || met_ == 30011)) return;
            if ((s == 243 || s == 244 || s == 245) && (met_ == 30012 || met_ == 30013)) return;
        }
        invalid(CheckIdentifier::Transfer, "Met location is not Poke Transporter's for a Generation " +
                std::to_string(originGen_) + " origin");
    }

    // TransferVerifier.VerifyVCEncounter / VerifyTransferLegalityG12 (the console-region
    // part is PK7-only). Poke Transporter writes met location 30013 for a Red/Green/Blue/
    // Yellow record (30017 when its family is Gen II-only) and 30017 for Gold/Silver/Crystal.
    void checkVirtualConsole() {
        const bool vc1 = version_ >= 35 && version_ <= 38;
        if (met_ != LocationTransfer2 && !(vc1 && met_ == LocationTransfer1))
            invalid(CheckIdentifier::Transfer, "Met location is not Poke Transporter's for a Virtual Console origin");
        if (pk_.originalTrainerGender() == 1 && version_ != 41)
            invalid(CheckIdentifier::Trainer, "Only Crystal has a female OT among the Virtual Console games");
        const uint8_t met = pk_.metLevel();
        if (met == 100) {
            if (pk_.experience() % 25 != pk_.nature())
                invalid(CheckIdentifier::Transfer, "Nature does not match the experience at transfer (EXP % 25)");
        } else if (met <= 2 && personal_ && pk_.nature() < 25 &&
                   !Gen67::Data::vcNatureValidAtMetLevel2(personal_->growth, pk_.nature())) {
            invalid(CheckIdentifier::Transfer, "No transferred level-2 experience gives this nature");
        }
    }

    // HyperTrainingVerifier. Sword/Shield trains at level 100, Scarlet/Violet and Z-A at 50;
    // whether this record visited those games needs its evolution history, so only a level
    // below 50 is decided here.
    void checkHyperTraining() {
        const uint8_t flags = plain_[0x126];
        if ((flags & 0x3F) == 0 || !personal_) return;
        if (level() < 50) {
            invalid(CheckIdentifier::Stats, "Hyper Training needs level 50 or more");
            return;
        }
        if (level() < 100)
            info(CheckIdentifier::Stats, "Hyper Training below level 100 needs a Scarlet/Violet or Z-A visit (not checked)");
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

    // MiscVerifierPK8: sociability, battle version and Dynamax level; NicknameVerifier
    // (EggStateLegality.IsNicknameFlagSet): a Sword/Shield egg carries the nickname flag.
    void checkSwordShieldFields() {
        if (pk_.isEgg() && side_ == 0 && (version_ == VersionSW || version_ == VersionSH) && !pk_.isNicknamed())
            invalid(CheckIdentifier::Egg, "A Sword/Shield egg must have the nickname flag set");
        const uint32_t social = PK8::read32(plain_, 0x48);
        if (pk_.isEgg() ? social != 0 : social > 255)
            invalid(CheckIdentifier::Misc, pk_.isEgg() ? "An egg cannot have sociability" : "Sociability is above 255");
        // IBattleVersion.IsBattleVersionValid: 0, or Sword/Shield for a record that did not
        // originate in (or side-transfer into) Sword/Shield.
        const uint8_t battle = plain_[0xDF];
        if (battle != 0) {
            const bool swordShieldOrigin = version_ == VersionSW || version_ == VersionSH || version_ == 47 ||
                                           version_ == 48 || version_ == 49 || version_ == 50 || version_ == 51 ||
                                           version_ == 56 || version_ == 57;
            if ((battle != VersionSW && battle != VersionSH) || swordShieldOrigin)
                invalid(CheckIdentifier::Misc, "Battle version " + std::to_string(battle) + " is not possible for this record");
        }
        // IDynamaxLevel.CanHaveDynamaxLevel: no egg and no Zacian/Zamazenta/Eternatus; at most 10.
        const uint8_t dynamax = pk_.dynamaxLevel();
        if (dynamax != 0) {
            const uint16_t s = pk_.species();
            if (pk_.isEgg() || s == Zacian || s == Zamazenta || s == Eternatus || dynamax > 10)
                invalid(CheckIdentifier::Misc, "Dynamax level " + std::to_string(dynamax) + " is not possible");
        }
    }

    // MiscVerifierPK8.VerifyTechRecordFlags: a TR flag the species/form cannot learn
    // (Calyrex keeps flags its fused forms learned).
    void checkTechRecords() {
        const uint16_t s = pk_.species();
        const uint8_t f = form();
        if (!Data::presentInGame(s, f)) return;  // already Invalid; its TR row is empty
        for (uint8_t i = 0; i < Data::techRecordCount(); ++i) {
            if (((plain_[0x127 + (i >> 3)] >> (i & 7)) & 1) == 0) continue;
            if (Data::canLearnTechRecord(s, f, i)) continue;
            if (s == Calyrex && ((f != 1 && Data::canLearnTechRecord(s, 1, i)) || (f != 2 && Data::canLearnTechRecord(s, 2, i))))
                continue;
            invalid(CheckIdentifier::Moves, "TR" + std::to_string(i) + " is flagged but the species cannot learn it");
            return;
        }
    }

    // Not a PKHeX rule: a party record's cached level and stats are rewritten by the game
    // whenever it enters the party, so a mismatch means the bytes were edited. Warning.
    void checkPartyStats() {
        if (pk_.size() != PK8::PartySize || pk_.isEgg() || !personal_) return;
        const uint8_t lvl = level();
        if (pk_.statLevel() != lvl) {
            warning(CheckIdentifier::Stats, "Party level byte does not match the level from experience");
            return;
        }
        const auto& base = personal_->baseStats;
        const uint8_t ht = plain_[0x126];
        static constexpr std::array<uint8_t, 6> bitForIv = {0, 1, 2, 5, 3, 4};
        std::array<uint32_t, 6> out{};
        for (size_t i = 0; i < 6; ++i) {
            const uint32_t iv = ((ht >> bitForIv[i]) & 1) ? 31u : pk_.iv(i);
            const uint32_t core = iv + 2u * base[i] + pk_.ev(i) / 4u;
            out[i] = i == 0 ? (base[0] == 1 ? 1 : (core + 100) * lvl / 100 + 10) : core * lvl / 100 + 5;
        }
        // Gen VIII stats use the stat nature (mints).
        const uint8_t nature = pk_.statNature();
        if (nature < 25) {
            const size_t up = nature / 5 + 1, down = nature % 5 + 1;
            if (up != down) {
                out[up] = out[up] * 11 / 10;
                out[down] = out[down] * 9 / 10;
            }
        }
        for (size_t i = 0; i < 6; ++i) {
            if (pk_.stat(i) != out[i]) {
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

uint8_t sideTransferOrigin(uint16_t metLocation) noexcept {
    switch (metLocation) {
        case SWLA: return 47;
        case SWBD: return 48;
        case SHSP: return 49;
        case SWSL: return 50;
        case SHVL: return 51;
        case SWFR: return 56;
        case SHLG: return 57;
        default: return 0;
    }
}

uint8_t originGeneration(uint8_t version, uint16_t metLocation) noexcept {
    switch (version) {
        case 42: case 43: return 7;                    // Let's Go
        case 44: case 45: case 47: case 48: case 49: return 8;
        case 34:                                       // GO: by the location HOME or Let's Go wrote
            return metLocation == LocationGO8 ? 8 : metLocation == LocationGO7 ? 7 : 0;
        case 52: return 9;                             // Legends: Z-A
        case 56: case 57: return 3;                    // Switch Virtual Console FireRed/LeafGreen
        default: return Gen67::originGeneration(version);
    }
}

Report analyze(std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    if (!PK8::supportedSize(record.size())) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 328-byte or 344-byte PK8 record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    // The codec decodes stored bytes; a decrypted input is re-encrypted first so both
    // paths share one decoder (encryption is a bijection, so nothing is lost).
    std::vector<uint8_t> stored = input.encrypted ? std::vector<uint8_t>(record.begin(), record.end()) : PK8::encrypt(record);
    PK8::DecodeStatus status{};
    const auto pk = PK8::Pokemon8::decode(stored, &status);
    if (!pk) {
        report.coverage.structure = CoverageLevel::Partial;
        if (status == PK8::DecodeStatus::BadChecksum)
            report.issues.push_back({Severity::Invalid, "PK8 checksum does not match its data", CheckIdentifier::Checksum});
        else if (status == PK8::DecodeStatus::BadSanity)
            report.issues.push_back({Severity::Invalid, "PK8 sanity field is set (bad egg)", CheckIdentifier::Structure});
        else
            report.issues.push_back({Severity::Warning, "PK8 record could not be decoded", CheckIdentifier::Structure});
        return report;
    }
    if (pk->species() == 0) {
        if (!pk->allZero()) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the PK8 payload is not a native blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(*pk, input).run();
}

} // namespace Legality::Gen8
