// Gen8Legality.cpp - PK8 (Sword/Shield), PB8 (Brilliant Diamond/Shining Pearl) and PK9
// (Scarlet/Violet) native legality (see include/Legality/Gen8Legality.h).
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
#include "Legality/ModernNickname.h"
#include "Legality/ModernEncounter.h"
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
    Zacian = 888, Zamazenta = 889, Eternatus = 890, Calyrex = 898, Dialga = 483, Palkia = 484,
    Ogerpon = 1017, Terapagos = 1024,
    // PKHeX SpeciesCategory.IsFixedGenderFromDual for species that exist in PK8.
    Shedinja = 292, Wormadam = 413, Vespiquen = 416, Gallade = 475, Froslass = 478, Salazzle = 758,
};
enum : uint16_t {
    MoveSketch = 166, MoveSecretSword = 548, MoveFusionFlare = 558, MoveFusionBolt = 559,
    MoveHyperspaceHole = 593, MoveHyperspaceFury = 621, MoveRevivalBlessing = 863,
};
constexpr uint16_t ItemGriseousOrb = 112, ItemAdamantCrystal = 1777, ItemLustrousGlobe = 1778, ItemGriseousCore = 1779;
// TeraTypeUtil: types 0-17, the "no override" marker and Stellar.
constexpr uint8_t TeraMaxType = 17, TeraOverrideNone = 19, TeraStellar = 99;
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
    Analyzer(Context ctx, const PK8::Pokemon8& pk, const Modern::Input& input)
        : ctx_(ctx), pk_(pk), in_(input), plain_(pk.plain()) {}

    Report run() {
        report_.coverage.structure = CoverageLevel::Complete;
        report_.coverage.internal = CoverageLevel::Partial;
        report_.coverage.moves = CoverageLevel::Partial;
        report_.coverage.pidRng = CoverageLevel::Partial;   // EC/PID transfer and nature correlation only
        report_.coverage.transfer = CoverageLevel::Partial; // presence in the game and transfer markers only
        checkSource();
        const uint16_t species = this->species();
        if (species > Data::maxSpecies(ctx_)) {
            invalid(CheckIdentifier::Species, "Species " + std::to_string(species) + " does not exist in " + formatName() +
                    " (PKHeX: species above the format's maximum)");
            return finish();
        }
        version_ = sv() ? plain_[0xCE] : pk_.originVersion();
        met_ = PK8::read16(plain_, 0x122);
        // Only Sword/Shield remaps a side transfer's version and met location (LocationsHOME).
        side_ = ctx_ == Context::SwordShield ? sideTransferOrigin(met_) : 0;
        originGen_ = originGeneration(version_, met_);
        if (ctx_ != Context::SwordShield && (version_ == 50 || version_ == 51)) originGen_ = 9;  // Scarlet/Violet keeps its version
        if (sv() && version_ == 0 && pk_.isEgg()) originGen_ = 9;  // PK9.IsUnhatchedEgg: an egg has no version until it hatches
        personal_ = Data::personal(species, form(), ctx_);
        checkOrigin();
        checkPresence();
        checkForm();
        // Base-form personal entry for a form outside the table (already Invalid above).
        if (!personal_) personal_ = Data::personal(species, 0, ctx_);
        // Species/forms absent from the game have empty personal rows; their checks are moot.
        if (personal_ && !personal_->present) personal_.reset();
        checkLevel();
        checkEffortValues();
        checkMoves();
        checkPidEc();
        checkGender();
        checkAbility();
        checkNature();
        checkBall();
        checkFateful(sv() ? Modern::Format::PK9 : bdsp() ? Modern::Format::PB8 : Modern::Format::PK8, species, (plain_[0x22] & 1) != 0);
        checkLanguage();
        checkNickname();
        checkUnhatchedEgg();
        checkHatchedEgg();
        checkRelearn();
        checkHeldItem();
        checkTransferMetLocation();
        if (originGen_ == 1 || originGen_ == 2) checkVirtualConsole();
        checkHyperTraining();
        if (bdsp()) checkBdspFields();
        else if (sv()) checkScarletVioletFields();
        else {
            checkSwordShieldFields();
            checkTechRecords();
        }
        checkPartyStats();
        checkTrainer();
        report_.issues.push_back({Severity::Info,
            std::string(gameName()) + " encounter, learnset, event, breeding, ribbon, memory and HOME-history rules "
            "are not implemented yet; a record without a contradiction stays Incomplete", CheckIdentifier::Misc});
        return finish();
    }

private:
    Context ctx_;
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

    bool bdsp() const noexcept { return ctx_ == Context::BDSP; }
    bool sv() const noexcept { return ctx_ == Context::ScarletViolet; }
    const char* formatName() const noexcept { return sv() ? "PK9" : bdsp() ? "PB8" : "PK8"; }
    const char* gameName() const noexcept {
        return sv() ? "Scarlet/Violet" : bdsp() ? "Brilliant Diamond/Shining Pearl" : "Sword/Shield";
    }
    // Fields PK9 stores elsewhere: an internal species id (SpeciesConverter.GetNational9), gender
    // in bits 1-2 of 0x22, battle version at 0xCF and language at 0xD5.
    uint16_t species() const noexcept { return sv() ? Data::nationalSpecies9(pk_.species()) : pk_.species(); }
    uint8_t gender() const noexcept { return sv() ? static_cast<uint8_t>((plain_[0x22] >> 1) & 3) : pk_.gender(); }
    uint8_t language() const noexcept { return sv() ? plain_[0xD5] : pk_.language(); }
    uint8_t battleVersion() const noexcept { return plain_[sv() ? 0xCF : 0xDF]; }
    // A Scarlet/Violet origin (or a Scarlet/Violet egg, which has no version yet).
    bool svOrigin() const noexcept { return version_ == 50 || version_ == 51 || (sv() && version_ == 0 && pk_.isEgg()); }

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
    // PKM.VC3: a Switch Virtual Console FireRed/LeafGreen record moved in through HOME (PK8
    // overrides it with its HOME met location; PB8 keeps the version).
    bool vc3() const noexcept { return !swsh() ? (version_ == 56 || version_ == 57) : (met_ == SWFR || met_ == SHLG); }
    bool swsh() const noexcept { return ctx_ == Context::SwordShield; }

    void checkSource() {
        if (in_.game == Game::Unknown) {
            report_.coverage.sourceGame = CoverageLevel::Partial;
            return;
        }
        if (Modern::nativeFormat(in_.game) != (sv() ? Modern::Format::PK9 : bdsp() ? Modern::Format::PB8 : Modern::Format::PK8)) {
            report_.coverage.sourceGame = CoverageLevel::None;
            warning(CheckIdentifier::SourceGame, std::string("The source game does not store ") + formatName() +
                    " records; game-specific checks were skipped");
            return;
        }
        report_.coverage.sourceGame = in_.integrity == Modern::Integrity::Verified ? CoverageLevel::Complete
                                                                                 : CoverageLevel::Partial;
        if (in_.integrity == Modern::Integrity::Failed)
            warning(CheckIdentifier::SourceGame, "The source container failed its own integrity check");
    }

    // EncounterFinder: a version no game writes into the format has no encounter at all, and
    // Legends: Z-A records cannot be moved into Sword/Shield (whether HOME moves them into
    // BD/SP is not decided here).
    void checkOrigin() {
        if (originGen_ == 0) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version_) + " cannot be stored in a " + formatName());
            return;
        }
        if (originGen_ > 8 && swsh()) {
            invalid(CheckIdentifier::Origin, "Origin game " + std::to_string(version_) + " is newer than " + std::string(formatName()));
            return;
        }
        if (side_ != 0) {
            info(CheckIdentifier::Transfer, "Moved into Sword/Shield through HOME from game " + std::to_string(side_));
            // LocationsHOME.GetVersionSWSH: HOME writes Sword for Legends: Arceus, Brilliant Diamond,
            // Scarlet and FireRed, and Shield for Shining Pearl, Violet and LeafGreen.
            const uint8_t expected = (side_ == 47 || side_ == 48 || side_ == 50 || side_ == 56) ? VersionSW : VersionSH;
            if (version_ != expected)
                invalid(CheckIdentifier::Origin, "A record moved in from game " + std::to_string(side_) + " through HOME has version " +
                                                     std::to_string(expected) + ", not " + std::to_string(version_));
            if (!ModernEncounter::lineInGame(species(), side_))
                invalid(CheckIdentifier::Origin, "No species of the line is in game " + std::to_string(side_) +
                                                     ", which the HOME met location names as its origin");
        }
        else if (originGen_ != 8)
            info(CheckIdentifier::Transfer, "Transferred from generation " + std::to_string(originGen_));
    }

    // TransferVerifier.VerifyTransferLegalityG8: Sword/Shield and BD/SP hold a selection of
    // species and forms; any other species/form cannot be in the format.
    void checkPresence() {
        if (!Data::presentInGame(species(), form(), ctx_))
            invalid(CheckIdentifier::Transfer, std::string("Species/form is not present in ") + gameName());
    }

    // FormVerifier (format 8 branches).
    void checkForm() {
        const uint16_t species = this->species();
        const uint8_t f = form();
        const uint8_t count = Data::formCount(species, ctx_);
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
                // Format 9 uses the Griseous Core; earlier formats the Griseous Orb.
                if ((f == 1) != (item == (sv() ? ItemGriseousCore : ItemGriseousOrb)))
                    return invalid(CheckIdentifier::Items, "Giratina's form does not match its Origin item");
                break;
            case Dialga:
                if (sv() && (f == 1) != (item == ItemAdamantCrystal))
                    return invalid(CheckIdentifier::Items, "Dialga's form does not match the Adamant Crystal");
                break;
            case Palkia:
                if (sv() && (f == 1) != (item == ItemLustrousGlobe))
                    return invalid(CheckIdentifier::Items, "Palkia's form does not match the Lustrous Globe");
                break;
            case Ogerpon:
                if ((f & 3) != Data::ogerponForm(item))
                    return invalid(CheckIdentifier::Items, "Ogerpon's form does not match its held mask");
                break;
            case Arceus:
                if (Data::arceusForm(item, ctx_) != f)
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
                // A Scarlet/Violet encounter has its own patterns (fixed per encounter, not checked).
                if (!svOrigin() && f > Gen67::Data::vivillonMaxWildForm())
                    return invalid(CheckIdentifier::Species, "Vivillon pattern is only obtainable as an event");
                break;
            case Meowstic:
                if ((f & 1) != gender())
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
        if (Data::battleOnlyForm(species, f, ctx_))
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
            if (m[i] > Data::maxMove(ctx_)) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " does not exist");
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
            if (species() == Kyurem) {
                if (f != 1 && hasMove(MoveFusionFlare)) invalid(CheckIdentifier::Moves, "Only White Kyurem knows Fusion Flare");
                if (f != 2 && hasMove(MoveFusionBolt)) invalid(CheckIdentifier::Moves, "Only Black Kyurem knows Fusion Bolt");
            }
            if (species() == Hoopa && hasMove(f != 0 ? MoveHyperspaceHole : MoveHyperspaceFury))
                invalid(CheckIdentifier::Moves, "Hoopa's form does not match its signature move");
        }
        if (pk_.isEgg()) {
            for (size_t i = 0; i < 4; ++i) {
                if (movePPUps(i) != 0) { invalid(CheckIdentifier::Egg, "An egg cannot have PP Ups"); break; }
            }
            for (size_t i = 0; i < 4; ++i) {
                if (movePP(i) != Data::basePP(m[i], ctx_)) { invalid(CheckIdentifier::Egg, "An egg's PP is not its moves' base PP"); break; }
            }
            return;
        }
        for (size_t i = 0; i < 4; ++i) {
            if (movePPUps(i) != 0 && (m[i] == 0 || m[i] == MoveSketch || m[i] == MoveRevivalBlessing))
                invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
        }
        for (size_t i = 0; i < 4; ++i) {
            const uint32_t healed = Data::basePP(m[i], ctx_) * (5u + movePPUps(i)) / 5u;
            if (movePP(i) > healed) invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
        }
    }

    // PIDVerifier.VerifyEC (format >= 6). Gen III-V origins carry the Poke Transporter PID;
    // Switch Virtual Console records carry HOME's PID (not checked here); everything else
    // has PID != EC, except fixed-EC Legends: Arceus / Brilliant Diamond / Shining Pearl
    // gifts of the line (ModernEncounter::fixedEcGiftPossible), which a record from those games
    // may be, and Sword/Shield HOME gifts.
    void checkPidEc() {
        if (vc3()) return;
        const uint32_t pid = pk_.pid(), ec = pk_.encryptionConstant();
        if (originGen_ >= 3 && originGen_ <= 5) {
            if (pid != Gen67::transferPid(ec, id32()))
                invalid(CheckIdentifier::PidRng, "PID does not match the Poke Transporter PID for its encryption constant");
            return;
        }
        const bool fixedEcGift = side_ != 0 || ModernEncounter::fixedEcGiftPossible(species(), version_);
        // A Sword/Shield-origin record with EC 0 may be a HOME gift (WC8.IsHOMEGift), whose
        // EC PKHeX does not check; the encounter is not resolved here.
        const bool homeGift = ec == 0 && (version_ == 44 || version_ == 45);
        if (originGen_ != 0 && pid == ec && !fixedEcGift && !homeGift)
            invalid(CheckIdentifier::PidRng, "PID equals the encryption constant");
    }

    // GenderVerifier and PKM.IsGenderValid.
    void checkGender() {
        if (!personal_) return;
        const uint8_t gender = this->gender();
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
                const bool azurill = (species() == Marill || species() == Azumarill) &&
                                     gender == 1 && lowEc >= 127 && lowEc < 191;
                if (!azurill) invalid(CheckIdentifier::Misc, "Gender does not match the PID of a Generation III-V origin");
            } else if (fixedGenderFromDual(species())) {
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

    // MiscFatefulEncounterVerifier: the fateful flag in a state no encounter of the species takes.
    void checkFateful(Modern::Format format, uint16_t species, bool fateful) {
        if (ModernEncounter::fatefulPossible(format, species, fateful)) return;
        invalid(CheckIdentifier::Encounter, fateful ? "Fateful encounter flag is set but no encounter of the species is fateful"
                                                    : "Fateful encounter flag is clear but every encounter of the species is fateful");
    }

    // BallVerifier: no Sword/Shield record holds ball 0 or a ball above Beast Ball (Legends:
    // Arceus balls are replaced by a Poke Ball when the record moves to Sword/Shield); BD/SP
    // keeps Legends: Arceus balls, up to the Origin Ball.
    void checkBall() {
        const uint8_t ball = pk_.ball();
        if (ball == 0 || ball > Data::maxBall(ctx_))
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball) + " cannot be held by a " + formatName());
        else if (!ModernEncounter::ballPossible(sv() ? Modern::Format::PK9 : bdsp() ? Modern::Format::PB8 : Modern::Format::PK8, species(), ball))
            invalid(CheckIdentifier::Origin, "Ball " + std::to_string(ball) + " is not possible for any encounter of the species");
    }

    Modern::Format format() const noexcept { return sv() ? Modern::Format::PK9 : bdsp() ? Modern::Format::PB8 : Modern::Format::PK8; }

    // EncounterVerifier.VerifyUnhatchedEgg and NicknameVerifier.VerifyNicknameEgg, made
    // encounter-independent over every egg encounter of the format's own games (ModernEncounter.h).
    // BD/SP stores "no location" as 65535 and an unhatched egg's trade as 30001 (LinkTrade6NPC).
    void checkUnhatchedEgg() {
        if (!pk_.isEgg()) return;
        const bool own = sv() ? (version_ == 0 || version_ == 50 || version_ == 51)
                       : bdsp() ? (version_ == 48 || version_ == 49)
                                : (side_ == 0 && (version_ == VersionSW || version_ == VersionSH));
        if (!own) {
            invalid(CheckIdentifier::Egg, std::string("An unhatched egg cannot leave the game it is in (HOME refuses eggs), so it must be a ") +
                                              gameName() + " egg");
            return;
        }
        const auto lang = language();
        if (lang != 0 && lang != 6 && !ModernNickname::eggNameMatches(sv() ? ModernNickname::Family::Switch9 : ModernNickname::Family::Switch8,
                                                                      plain_.subspan(0x58, 26), lang))
            invalid(CheckIdentifier::Egg, "An unhatched egg's name is not the egg name of its language");
        const auto f = ModernEncounter::eggFacts(format(), species());
        if (!f.decided) return;
        if (!f.exists) {
            invalid(CheckIdentifier::Egg, "No egg of the species exists in the format's games");
            return;
        }
        const uint16_t none = bdsp() ? 65535 : 0, trade = bdsp() ? 30001 : 30002;
        const uint16_t eggLocation = PK8::read16(plain_, 0x120);
        if (eggLocation == none && !f.eggLocationZero)
            invalid(CheckIdentifier::Egg, "An unhatched egg has no egg location");
        // A gift egg's (mystery gift or in-game) met level and locations follow its template; bred eggs:
        if (!f.gift) {
            if (pk_.metLevel() != 1) invalid(CheckIdentifier::Egg, "An unhatched egg's met level is not 1");
            if (met_ != none && met_ != trade) invalid(CheckIdentifier::Egg, "An unhatched egg has a met location other than a link trade");
            if (eggLocation == trade) invalid(CheckIdentifier::Egg, "An unhatched egg cannot have the link-trade egg location");
        }
        if (pk_.isNicknamed() ? !f.nicknameFlagSet : !f.nicknameFlagClear)
            invalid(CheckIdentifier::Egg, pk_.isNicknamed() ? "An unhatched egg's nickname flag is set, but no egg of the species sets it"
                                                            : "An unhatched egg's nickname flag is clear, but every egg of the species sets it");
    }

    // EncounterVerifier.VerifyEncounterEgg8/8BDSP/9 for a record of the format's own games. No non-egg
    // encounter there carries an egg location (other than BD/SP's 65535; ModernEncounter.h checks it
    // per species), so a hatched record with one hatched from an egg: met level 1, at a hatching
    // location of its game.
    void checkHatchedEgg() {
        const bool own = sv() ? (version_ == 50 || version_ == 51) : bdsp() ? (version_ == 48 || version_ == 49)
                                                                           : (side_ == 0 && (version_ == VersionSW || version_ == VersionSH));
        const uint16_t eggLocation = PK8::read16(plain_, 0x120);
        if (pk_.isEgg() || !own || eggLocation == 0 || eggLocation == 65535) return;
        const auto f = ModernEncounter::eggFacts(format(), species());
        if (!f.decided || f.nonEggEggLocation) return;
        if (!f.lineHasEgg) {
            invalid(CheckIdentifier::Egg, "Egg location is set but no egg of the species' line exists in its origin games");
            return;
        }
        if (pk_.metLevel() != 1) invalid(CheckIdentifier::Egg, "A hatched egg's met level is not 1");
        if (!ModernEncounter::hatchLocation(version_, met_))
            invalid(CheckIdentifier::Egg, "A hatched egg's met location is not a hatching location of its origin game");
    }

    // LearnVerifierRelearn: no relearn moves for a Generation 1-5 origin or once the original moveset
    // was deleted (Legal.IsOriginalMovesetDeleted: a record from another game moved in through HOME,
    // or a Sword/Shield record that entered a battle version); no repeated relearn move.
    void checkRelearn() {
        std::array<uint16_t, 4> moves{};
        for (size_t i = 0; i < 4; ++i) moves[i] = PK8::read16(plain_, 0x82 + 2 * i);
        for (size_t i = 0; i < 4; ++i)
            for (size_t j = 0; j < i; ++j)
                if (moves[i] != 0 && moves[i] == moves[j]) {
                    invalid(CheckIdentifier::Moves, "Relearn move " + std::to_string(moves[i]) + " is repeated");
                    return;
                }
        if (moves == std::array<uint16_t, 4>{}) return;
        bool deleted;
        if (sv()) deleted = !(version_ == 50 || version_ == 51 || (pk_.isEgg() && version_ == 0));
        else if (bdsp()) deleted = !(version_ == 48 || version_ == 49);
        else deleted = battleVersion() != 0 || (side_ != 0 && !(species() == 251 && (side_ == 56 || side_ == 57)));
        if (deleted)
            invalid(CheckIdentifier::Moves, std::string("Relearn moves are set but the original moveset was deleted (moved in through HOME") +
                                                (!sv() && !bdsp() && battleVersion() != 0 ? " or battle version set)" : ")"));
        else if (originGen_ != 0 && originGen_ < 6)
            invalid(CheckIdentifier::Moves, "A Generation " + std::to_string(originGen_) + " origin has no relearn moves");
    }

    // NicknameVerifier: NickLengthShort, and VerifyUnNicknamed (ModernNickname.h).
    void checkNickname() {
        namespace N = ModernNickname;
        const N::Input n{sv() ? N::Family::Switch9 : N::Family::Switch8, plain_.subspan(0x58, 26), species(), language(), pk_.isEgg(),
                         pk_.isNicknamed(), 0};
        if (const char* m = N::message(N::check(n))) invalid(CheckIdentifier::Trainer, m);
    }

    // LanguageVerifier.
    void checkLanguage() {
        const uint8_t lang = language();
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
        if (!Data::heldItemAllowed(item, ctx_))
            invalid(CheckIdentifier::Items, "Held item " + std::to_string(item) + " is not obtainable in " + gameName());
    }

    // TransferVerifier.VerifyTransferLegalityG3 / G4: a Gen III/IV origin (by version) moved
    // with Poke Transporter keeps met location 30001 (Lock Capsule / Crown Beast exceptions).
    void checkTransferMetLocation() {
        if (originGen_ != 3 && originGen_ != 4) return;
        if (met_ == LocationTransfer4) return;
        if (originGen_ == 4) {
            const uint16_t s = species();
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
    // a PK8 between 50 and 99 is decided only when no species of its line is in those games
    // (ModernEncounter::visitNinePossible). BD/SP always needs level 100 (HOME refuses lower trained records).
    void checkHyperTraining() {
        const uint8_t flags = plain_[0x126];
        if ((flags & 0x3F) == 0 || !personal_) return;
        if (bdsp() && level() < 100) {
            invalid(CheckIdentifier::Stats, "Hyper Training in Brilliant Diamond/Shining Pearl needs level 100");
            return;
        }
        if (level() < 50) {
            invalid(CheckIdentifier::Stats, "Hyper Training needs level 50 or more");
            return;
        }
        if (level() < 100 && swsh()) {
            if (!ModernEncounter::visitNinePossible(species())) {
                invalid(CheckIdentifier::Stats, "Hyper Training below level 100 needs a Scarlet/Violet or Z-A visit, and no species "
                                                "of its line is in those games");
                return;
            }
            info(CheckIdentifier::Stats, "Hyper Training below level 100 needs a Scarlet/Violet or Z-A visit (not checked)");
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

    // MiscVerifierPK8: sociability, battle version and Dynamax level; NicknameVerifier
    // (EggStateLegality.IsNicknameFlagSet): a Sword/Shield egg carries the nickname flag.
    void checkSwordShieldFields() {
        if (pk_.isEgg() && side_ == 0 && (version_ == VersionSW || version_ == VersionSH) && !pk_.isNicknamed())
            invalid(CheckIdentifier::Egg, "A Sword/Shield egg must have the nickname flag set");
        const uint32_t social = PK8::read32(plain_, 0x48);
        if (pk_.isEgg() ? social != 0 : social > 255)
            invalid(CheckIdentifier::Misc, pk_.isEgg() ? "An egg cannot have sociability" : "Sociability is above 255");
        checkBattleVersion();
        // MiscVerifierPK8.VerifyGigantamax: the flag state must be the encounter's own, or one
        // Gigantamax Soup can toggle (ModernEncounter.h; no encounter of the species and form allows it).
        const bool gmax = (plain_[0x16] & 0x10) != 0;
        if (!ModernEncounter::gigantamaxPossible(species(), form(), gmax, pk_.isEgg()))
            invalid(CheckIdentifier::Misc, std::string("No encounter of the species and form allows the Gigantamax flag ") +
                                               (gmax ? "set" : "clear") + (pk_.isEgg() ? " on an egg" : ", and Gigantamax Soup cannot change it"));
        checkRaidSeed();
        // IDynamaxLevel.CanHaveDynamaxLevel: no egg and no Zacian/Zamazenta/Eternatus; at most 10.
        const uint8_t dynamax = pk_.dynamaxLevel();
        if (dynamax != 0) {
            const uint16_t s = species();
            if (pk_.isEgg() || s == Zacian || s == Zamazenta || s == Eternatus || dynamax > 10)
                invalid(CheckIdentifier::Misc, "Dynamax level " + std::to_string(dynamax) + " is not possible");
        }
    }

    // Overworld8RNG and EncounterStatic8Nest.TryGetSeed: a Sword/Shield record met where every
    // encounter of its line is seed-bound (overworld-correlated wild slots and statics, raids at the
    // Pokemon Den 162, Max Lair 244 or their Wild Area locations) must come from one of those seeds
    // (ModernEncounter::raidSeed). Curry-marked records, Shedinja, and shiny records that could only
    // be raids are not checked, as in PKHeX. HOME keeps every field these seeds fix, so the same
    // rule holds for a Sword/Shield record in Scarlet/Violet (PK9, whose sizes sit at 0x48/0x49);
    // HOME's size and Ability Patch quirks are allowed for any record with a HOME tracker.
    void checkRaidSeed() {
        if (pk_.isEgg() || side_ != 0 || (version_ != VersionSW && version_ != VersionSH) || PK8::read16(plain_, 0x120) != 0) return;
        ModernEncounter::RaidRecord r;
        r.species = species();
        r.form = form();
        r.metLocation = PK8::read16(plain_, 0x122);
        r.ec = pk_.encryptionConstant();
        r.pid = pk_.pid();
        r.id32 = PK8::read32(plain_, 0x0C);
        r.ivs = {pk_.iv(0), pk_.iv(1), pk_.iv(2), pk_.iv(4), pk_.iv(5), pk_.iv(3)};
        r.abilityNumber = pk_.abilityNumber();
        r.gender = gender();
        r.nature = pk_.nature();
        const size_t size = sv() ? 0x48 : 0x50, tracker = sv() ? 0x127 : 0x135;
        r.height = plain_[size];
        r.weight = plain_[size + 1];
        r.homeTracker = sv() || (PK8::read32(plain_, tracker) | PK8::read32(plain_, tracker + 4)) != 0;
        r.curryMark = (plain_[0x40] & 0x08) != 0;  // G8PKM.RibbonMarkCurry
        if (ModernEncounter::raidSeed(r) == ModernEncounter::RaidResult::NoSeed)
            invalid(CheckIdentifier::PidRng, "No overworld or Max Raid seed of the species' line gives this EC, PID, IVs, ability, "
                                          "gender, nature and size, and every encounter of its line met here is seed-bound");
    }

    // MiscVerifierPB8: BD/SP has no sociability, Dynamax, Gigantamax or TR flags (a
    // non-egg record with any TR flag is Invalid), the Dpr-illegal flag is never set, Spinda
    // and Nincada cannot come in from another game, a Spinda has no HOME tracker, and the
    // battle version follows the same rule as Sword/Shield.
    void checkBdspFields() {
        if (PK8::read32(plain_, 0x48) != 0) invalid(CheckIdentifier::Misc, "Brilliant Diamond/Shining Pearl has no sociability");
        if (plain_[0x52] != 0) invalid(CheckIdentifier::Transfer, "The BD/SP illegal-record flag is set");
        const uint16_t s = species();
        const bool native = version_ == 48 || version_ == 49;
        if ((s == 327 || s == 290) && !native) invalid(CheckIdentifier::Transfer, "Spinda and Nincada cannot be moved into BD/SP");
        // G8PKM.Tracker: 64 bits at 0x135.
        if (s == 327 && (PK8::read32(plain_, 0x135) | PK8::read32(plain_, 0x139)) != 0)
            invalid(CheckIdentifier::Transfer, "A BD/SP Spinda has no HOME tracker");
        checkBattleVersion();
        if ((plain_[0x16] & 0x10) != 0) invalid(CheckIdentifier::Misc, "BD/SP records cannot Gigantamax");
        if (pk_.dynamaxLevel() != 0) invalid(CheckIdentifier::Misc, "BD/SP records have no Dynamax level");
        if (!pk_.isEgg()) {
            for (size_t i = 0; i < 14; ++i) {
                if (plain_[0x127 + i] != 0) { invalid(CheckIdentifier::Moves, "BD/SP records cannot hold TR flags"); break; }
            }
        }
    }

    // MiscVerifierPK9: battle version (as Sword/Shield), obedience level
    // (MiscVerifierHelpers.IsObedienceLevelValid) and Tera types (VerifyTeraType, and the
    // non-Scarlet/Violet-encounter branch of VerifyEncounter: a record from another game always
    // has a Tera type override).
    void checkScarletVioletFields() {
        checkBattleVersion();
        checkRaidSeed();
        if (personal_) {
            const uint8_t obey = plain_[0x11F], met = pk_.metLevel();
            // PK9.IsUntraded: no handling trainer name and a Gen IX origin (or an unhatched egg).
            const bool untraded = plain_[0xA8] == 0 && plain_[0xA9] == 0 && (originGen_ == 9 || (version_ == 0 && pk_.isEgg()));
            if (untraded ? obey != met : (obey < met || level() < obey))
                invalid(CheckIdentifier::Transfer, "Obedience level " + std::to_string(obey) + " is not possible for this record");
        }
        const uint8_t original = plain_[0x94], override_ = plain_[0x95];
        const uint16_t s = species();
        if (pk_.isEgg()) {
            if (override_ != TeraOverrideNone) invalid(CheckIdentifier::Egg, "An egg cannot have a Tera type override");
        } else if (s == Terapagos) {
            if (override_ != TeraOverrideNone) invalid(CheckIdentifier::Misc, "Terapagos cannot have a Tera type override");
        } else if (s == Ogerpon) {
            static constexpr std::array<uint8_t, 4> maskType = {11, 10, 9, 5};  // Grass, Water, Fire, Rock
            const uint8_t f = form() & 3;
            if (override_ != maskType[f] && !(f == 0 && override_ == TeraOverrideNone))
                invalid(CheckIdentifier::Misc, "Ogerpon's Tera type does not match its mask");
        } else if (!(original <= TeraMaxType || original == TeraOverrideNone || original == TeraStellar)) {
            invalid(CheckIdentifier::Misc, "Tera type " + std::to_string(original) + " does not exist");
        }
        if (!pk_.isEgg() && !svOrigin() && override_ == TeraOverrideNone)
            invalid(CheckIdentifier::Misc, "A record from another game must have a Tera type override");
        // MiscVerifierPK9.VerifyEncounter: a record from another game arrives with its line's HOME
        // import type as the original Tera type; a bred Scarlet/Violet egg hatches with one of its
        // species' types (gift eggs and lines with any gift egg are not decided).
        if (!pk_.isEgg() && s != Terapagos && s != Ogerpon && original <= TeraMaxType) {
            if (!svOrigin()) {
                if (!ModernEncounter::teraImportPossible(s, original))
                    invalid(CheckIdentifier::Misc, "Original Tera type " + std::to_string(original) +
                                                       " is not the HOME import type of any species of the line");
            } else if (PK8::read16(plain_, 0x120) != 0) {
                const auto eggs = ModernEncounter::eggFacts(Modern::Format::PK9, s);
                if (eggs.decided && !eggs.gift && !eggs.nonEggEggLocation && !ModernEncounter::teraEggPossible(s, original))
                    invalid(CheckIdentifier::Egg, "Original Tera type " + std::to_string(original) +
                                                      " is not a type of any species of the line, so no egg could hatch with it");
            }
        }
        checkTechRecords9();
        // NicknameVerifier.VerifyNicknameEgg (EggStateLegality.IsNicknameFlagSet): a Scarlet/Violet
        // egg carries the nickname flag, except Jacq's gift egg (egg location 60005).
        if (pk_.isEgg() && !pk_.isNicknamed() && PK8::read16(plain_, 0x120) != 60005)
            invalid(CheckIdentifier::Egg, "A Scarlet/Violet egg must have the nickname flag set");
        // MiscScaleVerifier.VerifyScale: a record HOME has touched (tracker set) has its height
        // scalar equal to its scale. Legends: Arceus alphas and Z-A records have their own rule.
        const uint64_t tracker = uint64_t{PK8::read32(plain_, 0x127)} | (uint64_t{PK8::read32(plain_, 0x12B)} << 32);
        if (tracker != 0 && version_ != 47 && version_ != 52 && plain_[0x48] != plain_[0x4A])
            invalid(CheckIdentifier::Misc, "Height scalar does not match the scale of a record HOME has touched");
    }

    // MiscVerifierPK9.VerifyTechRecordFlags: a TM flag that no form of the species or of its
    // pre-evolutions can learn. TM flags 0-199 are at 0x12F, 200 and up (DLC) at 0x4B.
    void checkTechRecords9() {
        const uint16_t s = species();
        if (!Data::presentInGame(s, form(), ctx_)) return;  // already Invalid
        for (uint16_t i = 0; i < Data::tmCount9(); ++i) {
            const size_t offset = i < 200 ? 0x12F + (i >> 3) : 0x4B + ((i - 200) >> 3);
            if (((plain_[offset] >> (i & 7)) & 1) == 0 || Data::tmLineCanLearn9(s, i)) continue;
            invalid(CheckIdentifier::Moves, "TM index " + std::to_string(i) + " is flagged but the species' line cannot learn it");
            return;
        }
    }

    // IBattleVersion.IsBattleVersionValid: 0, or Sword/Shield for a record that did not
    // originate in (or side-transfer into) Sword/Shield (LocationsHOME.GetVersionSWSH).
    void checkBattleVersion() {
        const uint8_t battle = battleVersion();
        if (battle == 0) return;
        const bool swordShieldOrigin = version_ == VersionSW || version_ == VersionSH || version_ == 47 || version_ == 48 ||
                                       version_ == 49 || version_ == 50 || version_ == 51 || version_ == 56 || version_ == 57;
        if ((battle != VersionSW && battle != VersionSH) || swordShieldOrigin)
            invalid(CheckIdentifier::Misc, "Battle version " + std::to_string(battle) + " is not possible for this record");
    }

    // MiscVerifierPK8.VerifyTechRecordFlags: a TR flag the species/form cannot learn
    // (Calyrex keeps flags its fused forms learned).
    void checkTechRecords() {
        const uint16_t s = species();
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

Report analyze(Context ctx, std::span<const uint8_t> record, const Modern::Input& input) {
    Report report{};
    const std::string name = ctx == Context::ScarletViolet ? "PK9" : ctx == Context::BDSP ? "PB8" : "PK8";
    if (!PK8::supportedSize(record.size())) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 328-byte or 344-byte " + name + " record", CheckIdentifier::Structure});
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
            report.issues.push_back({Severity::Invalid, name + " checksum does not match its data", CheckIdentifier::Checksum});
        else if (status == PK8::DecodeStatus::BadSanity)
            report.issues.push_back({Severity::Invalid, name + " sanity field is set (bad egg)", CheckIdentifier::Structure});
        else
            report.issues.push_back({Severity::Warning, name + " record could not be decoded", CheckIdentifier::Structure});
        return report;
    }
    if (pk->species() == 0) {
        if (!pk->allZero()) {
            report.coverage.structure = CoverageLevel::Partial;
            report.issues.push_back({Severity::Warning, "Species is zero but the " + name + " payload is not a native blank slot",
                                     CheckIdentifier::Structure});
        }
        return report;
    }
    return Analyzer(ctx, *pk, input).run();
}

} // namespace Legality::Gen8
