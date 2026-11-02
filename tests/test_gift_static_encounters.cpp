// Round 13 owner decisions 7 and 8 (verbatim intent):
//  7. "Gift/static encounters require proper source-backed data before v1.0 acceptance. Markers
//     alone are provisional."
//  8. "Crystal time rows can be grouped visually, but do not lose encounter-time distinctions.
//     Preserve accurate ancestry labels."
//
// 1. Data spot checks against the sources (tools/gen_gift_static_encounters.py: PKHeX pinned
//    tables, every Crystal row matched to its pret/pokecrystal map script, Gen III pret spot checks).
// 2. The Met Location pickers of Crystal, Gen III, Gen IV and Gen V offer a gift / static / trade /
//    fossil / prize / roaming row of the species or a pre-evolution (level-checked like a wild row)
//    as a VERIFIED row labelled with its kind; markers without data stay unverified; event
//    distributions have no rows.
// 3. Crystal time: rows grouped per place + level show every time they occur ("morn/day"), a place
//    whose times differ in level keeps one row per level, and the caught time written for a grouped
//    row is the pending time when the row allows it, otherwise a STATED default; it round-trips
//    through Add -> export -> strict reparse.
// 4. Owner decision (Nov 2): "Do not label a Pokémon's complete acquisition history Verified simply
//    because its encounter exists in the game's scripts." A ticket / event-Pokémon / other-game
//    dependent encounter is listed as "encounter in the game" but is never a verified encounter, and
//    its editor / Review note reads Incomplete; every row carries its source confidence, and a Gen IV
//    / V (single-source) data row says so in its evidence note.
#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Encounter/EncounterContext45.h"
#include "Integration/Encounter/GiftStaticEncounters.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "Integration/Gen5/Gen5EncounterLocations.h"
#include "Names/LocationNames.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen3EncounterPickerModel.h"
#include "UI/SharedMenuGeometry.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace GS = PokeVault::Integration::GiftStaticEncounters;
namespace E = PokeVault::Integration::EncounterGuardrails;
namespace C45 = PokeVault::Integration::EncounterContext45;
namespace Picker = PokeBank::UIModel::Gen2PokemonPicker;
namespace Native = PokeBank::UIModel::Gen2Native;
namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
namespace G3 = PokeBank::UIModel::Gen3EncounterPicker;
namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;

namespace {

int failures = 0;
void require(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

bool contains(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

const GS::Row* row(std::string_view game, uint16_t species, uint16_t location, uint8_t level) {
    for (const auto& r : GS::kRows)
        if (r.game == game && r.species == species && r.location == location && r.minLevel == level) return &r;
    return nullptr;
}

void dataSpotChecks() {
    // Crystal (pret/pokecrystal maps/*.asm; PKHeX Encounters2.cs).
    const auto* eevee = row("crystal_gbc", 133, 16, 20);   // BillsFamilysHouse: givepoke EEVEE, 20
    require(eevee && eevee->kind == GS::Kind::Gift && eevee->maxLevel == 20, "Crystal Eevee gift Goldenrod Lv 20");
    const auto* lapras = row("crystal_gbc", 131, 10, 20);  // UnionCaveB2F: loadwildmon LAPRAS, 20
    require(lapras && lapras->kind == GS::Kind::Static, "Crystal Lapras static Union Cave Lv 20");
    const auto* larvitar = row("crystal_gbc", 246, 71, 40);
    require(larvitar && larvitar->kind == GS::Kind::Prize, "Crystal Larvitar Game Corner prize Lv 40");
    const auto* machop = row("crystal_gbc", 66, 0x7E, 5);  // npc_trades.asm; SetGiftMonCaughtData
    require(machop && machop->kind == GS::Kind::Trade && (machop->flags & GS::kCaughtLevelUnrecorded),
            "Crystal Machop in-game trade at LANDMARK_GIFT, caught level unrecorded");
    const auto* spearow = row("crystal_gbc", 21, 0x7E, 10);
    require(spearow && spearow->kind == GS::Kind::Gift, "Crystal Kenya Spearow OT gift at LANDMARK_GIFT");
    bool rhydon = false, raikouRoute29 = false, raikouNewBark = false;
    for (const auto& r : GS::kRows) {
        if (r.game != "crystal_gbc") continue;
        rhydon |= r.species == 112 && r.kind == GS::Kind::Trade;   // Gold / Silver-only trade
        raikouRoute29 |= r.species == 243 && r.location == 2 && r.kind == GS::Kind::Roaming;
        raikouNewBark |= r.species == 243 && r.location == 1;
    }
    require(!rhydon, "Crystal has no Rhydon trade (Gold / Silver only)");
    require(raikouRoute29 && !raikouNewBark, "Crystal Raikou roams Route 29, never New Bark Town");

    // Gen III.
    for (const auto game : {"firered_gba", "leafgreen_gba"}) {
        const auto* r = row(game, 131, 134, 25);               // SilphCo_7F: givemon SPECIES_LAPRAS, 25
        require(r && r->kind == GS::Kind::Gift, std::string(game) + " Lapras gift Silph Co. Lv 25");
    }
    const auto* mime = row("firered_gba", 122, 254, 5);
    require(mime && mime->kind == GS::Kind::Trade && mime->maxLevel == 100, "FireRed Mr. Mime in-game trade at 254");
    for (const auto game : {"ruby_gba", "sapphire_gba", "emerald_gba"}) {
        const auto* r = row(game, 374, 13, 5);                 // MossdeepCity_StevensHouse: givemon BELDUM, 5
        require(r && r->kind == GS::Kind::Gift, std::string(game) + " Beldum gift Mossdeep Lv 5");
    }
    const auto* lugia = row("emerald_gba", 249, 211, 70);
    require(lugia && lugia->kind == GS::Kind::EventStatic && (lugia->flags & GS::kFateful),
            "Emerald Navel Rock Lugia: in-game event static, fateful");

    // Gen IV.
    for (const auto game : {"heartgold_nds", "soulsilver_nds"}) {
        for (const uint16_t s : {152, 155, 158}) {
            const auto* r = row(game, s, 126, 5);
            require(r && r->kind == GS::Kind::Gift, std::string(game) + " Johto starter gift New Bark Lv 5");
        }
        for (const uint16_t s : {1, 4, 7}) require(row(game, s, 138, 5), std::string(game) + " Kanto starter gift");
        for (const uint16_t s : {252, 255, 258}) require(row(game, s, 148, 5), std::string(game) + " Hoenn starter gift");
    }
    const auto* rotom = row("platinum_nds", 479, 70, 20);
    require(rotom && rotom->kind == GS::Kind::Static, "Platinum Rotom static Old Chateau Lv 20");
    require(row("diamond_nds", 479, 70, 15), "Diamond Rotom static Old Chateau Lv 15");
    const auto* giratina = row("platinum_nds", 487, 62, 47);
    const auto* origin = row("platinum_nds", 487, 117, 47);
    require(giratina && giratina->kind == GS::Kind::Static && giratina->form == 0 && origin && origin->form == 1,
            "Platinum Giratina static Turnback Cave (Altered) and Distortion World (Origin) Lv 47");
    const auto* abra = row("diamond_nds", 63, 2001, 1);
    require(abra && abra->kind == GS::Kind::Trade, "Diamond Abra in-game trade at 2001");
    const auto* togepiEgg = row("platinum_nds", 175, 0, 1);
    require(togepiEgg && togepiEgg->kind == GS::Kind::EggGift && togepiEgg->eggLocation == 2011 &&
            !GS::metLocationRow(*togepiEgg), "Platinum Togepi egg: egg data only, met where hatched");

    // Gen V.
    for (const auto game : {"black_nds", "white_nds"}) {
        const auto* zorua = row(game, 570, 8, 10);
        require(zorua && zorua->kind == GS::Kind::EventStatic, std::string(game) + " Zorua Castelia City Lv 10");
        const auto* emolga = row(game, 587, 30002, 30);
        require(emolga && emolga->kind == GS::Kind::Trade, std::string(game) + " Emolga in-game trade Lv 30");
    }
    for (const auto game : {"black2_nds", "white2_nds"}) {
        const auto* eevee5 = row(game, 133, 8, 10);
        require(eevee5 && eevee5->kind == GS::Kind::Gift, std::string(game) + " Eevee gift Castelia City Lv 10");
        const auto* nZorua = row(game, 570, 10, 25);
        require(nZorua && nZorua->kind == GS::Kind::Static && (nZorua->flags & GS::kNsPokemon),
                std::string(game) + " N's Zorua Driftveil City Lv 25");
    }

    // Every row: a covered game, a level range, a kind; no event-distribution locations.
    for (const auto game : {"crystal_gbc", "ruby_gba", "sapphire_gba", "emerald_gba", "firered_gba",
                            "leafgreen_gba", "diamond_nds", "pearl_nds", "platinum_nds", "heartgold_nds",
                            "soulsilver_nds", "black_nds", "white_nds", "black2_nds", "white2_nds"})
        require(GS::covers(game), std::string(game) + " covered");
    for (const auto& r : GS::kRows) {
        require(r.minLevel >= 1 && r.minLevel <= r.maxLevel && r.maxLevel <= 100, "level range");
        require(!(r.location >= 3000 && r.location < 4000) && !(r.location >= 40000 && r.location < 50000),
                "no event-distribution rows (no data)");
        require(std::string(GS::kindLabel(r.kind)) != "encounter", "every row names its kind");
    }
}

std::vector<uint16_t> gen45Native(uint8_t origin) {
    std::vector<uint16_t> native;
    if (C45::generation(origin) == 5) return {PokeVault::Integration::Gen5::NativeMetLocations5.begin(),
                                              PokeVault::Integration::Gen5::NativeMetLocations5.end()};
    const auto table = Names::getLocationTable(origin);
    for (size_t i = 1; i < table.count; ++i)
        if (table.names[i] && table.names[i][0] && Names::isGen4NativeMetLocation(origin, static_cast<uint16_t>(i)))
            native.push_back(static_cast<uint16_t>(i));
    return native;
}

// The Gen IV / V default list offers `location` as a verified row whose detail contains `label`.
void gen45Verified(uint8_t origin, uint16_t species, uint8_t level, uint16_t location, const std::string& label,
                   const std::string& what, bool checkBelow = true) {
    const C45::Catalogue context(origin, species, 0, gen45Native(origin), {}, level);
    const auto& choices = context.choices();
    const bool listed = std::find(choices.begin(), choices.end(), location) != choices.end();
    const auto detail = context.detail(location, [](uint16_t s) { return std::string(Names::getSpeciesName(s)); });
    require(listed && context.verifiedEncounter(location) && contains(detail, label) && !contains(detail, "unverified"),
            what + ": verified default row \"" + detail + "\"");
    // One level below the encounter: not verified, says the level is too low.
    if (checkBelow && level > 1) {
        const C45::Catalogue low(origin, species, 0, gen45Native(origin), {}, static_cast<uint8_t>(level - 1));
        // (a trade location stays in the default list as the game's marker, unverified)
        const auto& lowChoices = low.choices();
        const bool marker = PokeVault::Integration::MetLocationPolicy::gen45DefaultMarker(C45::generation(origin), location);
        require((marker || std::find(lowChoices.begin(), lowChoices.end(), location) == lowChoices.end()) &&
                !low.verifiedEncounter(location), what + ": not verified below its level");
    }
}

void gen45PickerRows() {
    gen45Verified(7, 152, 5, 126, "Gift Lv. 5", "HeartGold Chikorita New Bark Town");
    gen45Verified(8, 158, 5, 126, "Gift Lv. 5", "SoulSilver Totodile New Bark Town");
    gen45Verified(7, 154, 40, 126, "Gift Lv. 5 via Chikorita", "HeartGold Meganium via Chikorita", false);
    gen45Verified(12, 479, 20, 70, "Static Lv. 20", "Platinum Rotom Old Chateau");
    gen45Verified(10, 479, 15, 70, "Static Lv. 15", "Diamond Rotom Old Chateau");
    gen45Verified(12, 487, 47, 62, "Static Lv. 47", "Platinum Giratina Turnback Cave");
    gen45Verified(12, 487, 47, 117, "Static Lv. 47", "Platinum Giratina Distortion World");
    gen45Verified(10, 63, 1, 2001, "Trade Lv. 1+", "Diamond Abra in-game trade (2001)");
    gen45Verified(12, 138, 20, 94, "Fossil Lv. 20", "Platinum Omanyte fossil Oreburgh");
    // (Black / White Zorua / Zoroark need an event Pokémon in the party: Incomplete, acquisitionTiers)
    gen45Verified(23, 133, 10, 8, "Gift Lv. 10", "Black 2 Eevee Castelia City");
    gen45Verified(22, 133, 10, 8, "Gift Lv. 10", "White 2 Eevee Castelia City");
    gen45Verified(23, 570, 25, 10, "Static Lv. 25", "Black 2 N's Zorua Driftveil City");
    gen45Verified(21, 587, 30, 30002, "Trade Lv. 30", "Black Emolga in-game trade (30002)");
    // A marker without data stays unverified: Diamond Pikachu has no trade, so 2001 is not verified.
    const C45::Catalogue pikachu(10, 25, 0, gen45Native(10), {}, 30);
    require(!pikachu.verifiedEncounter(2001), "Diamond Pikachu: no trade data, the trade marker stays unverified");
    // An event distribution (Gen IV 3000+) has no rows: never verified.
    const C45::Catalogue manaphy(12, 490, 3000, gen45Native(12), {}, 50);
    require(!manaphy.verifiedEncounter(3000) && contains(C45::unverifiedMetNote45(12, 490, 3000, 50), "unverified"),
            "Gen IV event distribution stays unverified");
}

void gen3PickerRows() {
    const auto check = [](uint8_t origin, uint16_t species, uint8_t level, uint16_t location, const std::string& label,
                          const std::string& what) {
        const auto choices = G3::templatesForOrigin(origin, species);
        const auto rows = G3::defaultRows(choices, 0, origin, level);
        bool found = false;
        for (const auto value : rows) {
            const auto* choice = G3::choiceAt(choices, value);
            if (!choice || choice->encounter.location != location) continue;
            const auto text = G3::rowText(choices, value, origin, 0, 0, level);
            found |= text.verifiedCatch && contains(text.detail, label) && !contains(text.detail, "unverified");
        }
        require(found, what);
        const auto low = G3::defaultRows(choices, 0, origin, static_cast<uint8_t>(level - 1));
        for (const auto value : low)
            if (const auto* choice = G3::choiceAt(choices, value))
                require(!(choice->encounter.location == location && choice->encounter.minLevel == level),
                        what + ": not in the default list below its level");
    };
    check(4, 131, 25, 134, "Gift Lv. 25", "FireRed Lapras gift Silph Co.");
    check(5, 131, 25, 134, "Gift Lv. 25", "LeafGreen Lapras gift Silph Co.");
    check(4, 133, 25, 94, "Gift Lv. 25", "FireRed Eevee gift Celadon City");
    check(4, 136, 30, 94, "Gift Lv. 25 via Eevee", "FireRed Flareon via Eevee (gift Celadon City)");
    check(4, 122, 5, 254, "Trade Lv. 5+", "FireRed Mr. Mime in-game trade (254) verified");
    check(3, 374, 5, 13, "Gift Lv. 5", "Emerald Beldum gift Mossdeep City");
    check(3, 375, 20, 13, "Gift Lv. 5 via Beldum", "Emerald Metang via Beldum");
    check(2, 374, 5, 13, "Gift Lv. 5", "Ruby Beldum gift Mossdeep City");
    check(4, 138, 5, 96, "Fossil Lv. 5", "FireRed Omanyte fossil Cinnabar Island");
    check(4, 243, 50, 101, "Roaming Lv. 50", "FireRed Raikou roaming Route 1");
    // Without data, 254 stays a marker row (unverified): FireRed Pikachu has no in-game trade.
    const auto pikachu = G3::templatesForOrigin(4, 25);
    const auto rows = G3::defaultRows(pikachu, 0, 4, 30);
    require(std::find(rows.begin(), rows.end(), Rows::otherNativeRow(254)) != rows.end() &&
            contains(G3::rowText(pikachu, Rows::otherNativeRow(254), 4, 0, 0, 30).detail, "unverified"),
            "FireRed Pikachu: the trade marker stays an unverified marker");
}

int crystalRow(const Picker::Model& model, uint16_t location, const std::string& detailPart) {
    for (int row = 0; row < static_cast<int>(model.encounterChoices.size()); ++row)
        if (model.encounterChoices[static_cast<std::size_t>(row)].location == location &&
            contains(Picker::locationRowTextAt(model, row).detail, detailPart))
            return row;
    return -1;
}

void crystalPickerRows() {
    Picker::Model eevee;
    eevee.openLocation("crystal_gbc", 133, 0, 0, 20);
    require(crystalRow(eevee, 16, "Gift · Lv. 20") >= 0, "Crystal Eevee: Goldenrod City gift Lv 20 is a verified row");
    Picker::Model eeveeLow;
    eeveeLow.openLocation("crystal_gbc", 133, 0, 0, 19);
    require(crystalRow(eeveeLow, 16, "Lv. 20") < 0, "Crystal Lv 19 Eevee: the Lv 20 gift is not verified");
    Picker::Model espeon;
    espeon.openLocation("crystal_gbc", 196, 0, 0, 30);
    require(crystalRow(espeon, 16, "Gift · Lv. 20 · via Eevee") >= 0, "Crystal Espeon: Eevee gift via Eevee");
    Picker::Model machop;
    machop.openLocation("crystal_gbc", 66, 0, 0, 10);
    require(crystalRow(machop, 0x7E, "Trade · Lv. 5+") >= 0, "Crystal Machop: in-game trade verified at LANDMARK_GIFT");
    Picker::Model machamp;   // via Machop: the trade is the Machoke / Machamp ancestry row
    machamp.openLocation("crystal_gbc", 68, 0, 0, 40);
    require(crystalRow(machamp, 0x7E, "Trade · Lv. 5+ · via Machop") >= 0, "Crystal Machamp: trade via Machop");
    Picker::Model raikou;
    raikou.openLocation("crystal_gbc", 243, 0, 0, 40);
    require(crystalRow(raikou, 2, "Roaming · Lv. 40") >= 0, "Crystal Raikou: roaming row on Route 29");
    // Without data the Gift landmark stays a marker: Pikachu has no trade or OT gift in Crystal.
    Picker::Model pikachu;
    pikachu.openLocation("crystal_gbc", 25, 0, 0, 30);
    require(crystalRow(pikachu, 0x7E, "") < 0, "Crystal Pikachu: no verified Gift-landmark row");
}

// ---- Crystal time (owner decision 8) ----
void crystalTimeRows() {
    // Rattata at Route 29: morning / day Lv 2 and night Lv 2-3 -- two rows, each naming its times.
    Picker::Model rattata;
    rattata.openLocation("crystal_gbc", 19, 0, 0, 5);
    const int mornDay = crystalRow(rattata, 2, "Lv. 2 · morn/day");
    const int night = crystalRow(rattata, 2, "Lv. 2–3 · night");
    require(mornDay >= 0 && night >= 0 && mornDay != night,
            "Crystal Rattata Route 29: 'morn/day Lv 2' and 'night Lv 2-3' stay distinct rows");
    // Raticate's ancestry rows keep the same time distinctions and say "via Rattata".
    Picker::Model raticate;
    raticate.openLocation("crystal_gbc", 20, 0, 0, 30);
    require(crystalRow(raticate, 2, "Lv. 2 · morn/day · via Rattata") >= 0 &&
            crystalRow(raticate, 2, "Lv. 2–3 · night · via Rattata") >= 0,
            "Crystal Raticate: ancestry rows keep their times and 'via Rattata'");
    // A slot met at all three times says so (no silent omission).
    bool allThree = false;
    for (int row = 0; row < static_cast<int>(rattata.encounterChoices.size()); ++row)
        allThree |= contains(Picker::locationRowTextAt(rattata, row).detail, "any time");
    require(allThree, "Crystal Rattata: a slot met morning, day and night reads 'any time'");

    // Writing a grouped row: the pending time is kept when the row allows it; otherwise the first
    // time of the row is written and the status STATES it (never a silent change).
    const auto& md = rattata.encounterChoices[static_cast<std::size_t>(mornDay)];
    const auto keep = Picker::caughtForTemplate(md, 2, false);
    require(keep.caught.timeOfDay == 2 && keep.caught.location == 2 && keep.caught.levelCode == 2 &&
            contains(keep.status, "Day") && contains(keep.status, "morn/day"),
            "grouped morn/day row keeps a pending Day and says which time was written: " + keep.status);
    const auto def = Picker::caughtForTemplate(md, 3, false);
    require(def.caught.timeOfDay == 1 && def.timeDefaulted && contains(def.status, "Morning") &&
            contains(def.status, "morn/day"),
            "grouped morn/day row with a pending Night writes Morning and states it: " + def.status);
    const auto unset = Picker::caughtForTemplate(md, 0, true);
    require(unset.caught.timeOfDay == 1 && unset.timeDefaulted && unset.caught.originalTrainerFemale &&
            !unset.status.empty(), "no pending time: the stated default (Morning)");
    const auto& n = rattata.encounterChoices[static_cast<std::size_t>(night)];
    const auto nightWrite = Picker::caughtForTemplate(n, 0, false);
    require(nightWrite.caught.timeOfDay == 3, "a night-only row writes Night");
    // Crystal's gift / trade caught data (pret SetGiftMonCaughtData): LANDMARK_GIFT, level 0, time 0.
    Picker::Model machop;
    machop.openLocation("crystal_gbc", 66, 0, 0, 10);
    const int trade = crystalRow(machop, 0x7E, "Trade");
    require(trade >= 0, "Crystal Machop trade row");
    if (trade >= 0) {
        const auto t = Picker::caughtForTemplate(machop.encounterChoices[static_cast<std::size_t>(trade)], 2, false);
        require(t.caught.location == 0x7E && t.caught.levelCode == 0 && t.caught.timeOfDay == 0 && t.caught.present,
                "Crystal trade row writes LANDMARK_GIFT with caught level and time unrecorded");
        require(Rows::crystalUnverifiedNote("crystal_gbc", 66, 0x7E, 0, 10, 0).empty(),
                "Crystal Machop at LANDMARK_GIFT, level 0: a verified trade (no note)");
    }
    // The editor / Review note judges the stored time too: Rattata Lv 2 at Route 29 met at Night is
    // the night slot (verified); met in the Morning at Lv 3 is not a listed slot (unverified).
    require(Rows::crystalUnverifiedNote("crystal_gbc", 19, 2, 2, 5, 3).empty(), "Rattata Lv 2 night verified");
    require(Rows::crystalUnverifiedNote("crystal_gbc", 19, 2, 2, 5, 1).empty(), "Rattata Lv 2 morning verified");
    require(contains(Rows::crystalUnverifiedNote("crystal_gbc", 19, 2, 3, 5, 1), "time of day"),
            "Rattata Lv 3 in the morning at Route 29 is not a listed slot: unverified (time of day)");
}

// Every Crystal row (both lists, every species, at Lv 100 and Lv 5) fits its 640 px chooser on one
// line: place (Body) at listX + 14, detail (Caption) right-aligned at listX + listWidth - 14, the
// scrollbar gutter (28) and a 12 px gap between them.
void crystalRowsFit() {
    namespace G = PokeBank::UIModel::SharedMenuGeometry;
    const auto geometry = Picker::locationPickerGeometry(9);
    const int room = geometry.listWidth - 14 - 14 - 28 - 12;
    int widest = 0;
    std::string widestText;
    for (uint16_t species = 1; species <= 251; ++species)
        for (const uint8_t level : {5, 100})
            for (const bool unverified : {false, true}) {
                Picker::Model model;
                model.openLocation("crystal_gbc", species, 0, 0, level, unverified);
                for (int row = 0; row < model.locationCount(); ++row) {
                    const auto text = Picker::locationRowTextAt(model, row);
                    const int w = G::textWidth(text.place, G::Text::Body) + G::textWidth(text.detail, G::Text::Caption);
                    if (w > widest) { widest = w; widestText = text.place + " | " + text.detail; }
                }
            }
    require(widest <= room, "widest Crystal row fits (" + std::to_string(widest) + " / " + std::to_string(room) +
                                "): " + widestText);
    std::cout << "widest Crystal Met Location row " << widest << " / " << room << " px: " << widestText << '\n';
}

// Caught time write -> Add -> export -> strict reparse keeps the chosen row's time (Crystal fixture).
void crystalTimeRoundTrip() {
    const auto raw = fixture(C, true);
    auto parsed = parse(raw, SourceGame::Crystal);
    require(static_cast<bool>(parsed), "Crystal fixture parses");
    if (!parsed) return;
    std::string error;
    auto base = StagedEditor::create(*parsed.save, error);
    auto entry = *base->boxedPokemon(1, 0, error);
    Picker::Model rattata;
    rattata.openLocation("crystal_gbc", 19, 0, 0, 5);
    Picker::Model machop;
    machop.openLocation("crystal_gbc", 66, 0, 0, 10);
    struct Case { uint16_t species; uint8_t level; const E::EncounterTemplate* encounter; uint8_t pending; uint8_t time; };
    const int mornDay = crystalRow(rattata, 2, "morn/day");
    const int night = crystalRow(rattata, 2, "night");
    const int trade = crystalRow(machop, 0x7E, "Trade");
    require(mornDay >= 0 && night >= 0 && trade >= 0, "round-trip rows");
    if (mornDay < 0 || night < 0 || trade < 0) return;
    const Case cases[] = {
        {19, 5, &rattata.encounterChoices[static_cast<std::size_t>(mornDay)], 2, 2},
        {19, 5, &rattata.encounterChoices[static_cast<std::size_t>(mornDay)], 3, 1},
        {19, 5, &rattata.encounterChoices[static_cast<std::size_t>(night)], 0, 3},
        {66, 10, &machop.encounterChoices[static_cast<std::size_t>(trade)], 2, 0},
    };
    for (const auto& c : cases) {
        const std::string what = "Crystal " + std::to_string(c.species) + " time " + std::to_string(c.time);
        auto editor = StagedEditor::create(*parsed.save, error);
        PokemonRecord fresh = entry;
        fresh.level = 5;
        fresh.moves = {};
        fresh.pp = {};
        fresh.ppUps = {};
        fresh.pokerus = 0;
        fresh.caughtData = 0;
        Rules::Session create;
        create.begin(fresh, Rules::SessionMode::Create);
        uint8_t move = 0;
        for (uint16_t m = 1; m <= 251 && !move; ++m)
            if (PokeVault::Integration::Gen2::MoveCompatibility::canLearnMove(SourceGame::Crystal, c.species, m))
                move = static_cast<uint8_t>(m);
        require(create.setSpecies(c.species, "crystal_gbc") && create.setMove(0, move) && create.setLevel(c.level),
                what + ": Create");
        const auto choice = Picker::caughtForTemplate(*c.encounter, c.pending, false);
        require(choice.caught.timeOfDay == c.time, what + ": written time");
        create.working.caughtData = Native::encodeCrystalCaughtData(choice.caught);
        create.noteMetLocationChosen();   // what the picker does on this row (Round 14, owner R09)
        std::size_t slot = 99;
        require(create.add(*editor, 2, slot, error), what + ": add " + error);
        bool unverifiedNote = false;
        for (const auto& change : editor->pendingChanges())
            unverifiedNote |= contains(change.label, "Met evidence");
        require(!unverifiedNote, what + ": a verified row carries no unverified Review note");
        const auto out = editor->finalizedBytes(error);
        require(!out.empty() && error.empty(), what + ": export " + error);
        auto reread = parse(out, SourceGame::Crystal);
        require(static_cast<bool>(reread), what + ": strict reparse");
        if (!reread) continue;
        auto back = StagedEditor::create(*reread.save, error)->boxedPokemon(2, slot, error);
        require(back && back->caughtData == Native::encodeCrystalCaughtData(choice.caught),
                what + ": caught data (time) round-trips byte for byte");
    }
}

// ---- Owner decision (Nov 2): script-backed encounter vs access prerequisite vs Incomplete ----
void acquisitionTiers() {
    std::size_t total = 0, prereqRows = 0;
    for (const auto& r : GS::kRows) {
        ++total;
        if (r.prereq != GS::Prerequisite::None) {
            ++prereqRows;
            require(!GS::prerequisiteReason(r).empty(), "every prerequisite row names its reason");
        }
        const bool gen45 = r.game.find("_nds") != std::string_view::npos;
        require(!gen45 || r.confidence == GS::Confidence::SingleSource,
                "a Gen IV / V row is single source (PKHeX), never independently verified");
        require(r.game == "crystal_gbc" || r.confidence != GS::Confidence::PretChecked,
                "only Crystal rows are fully pret cross-checked");
    }
    require(total == 1332, "all 1332 rows preserved");
    require(prereqRows > 0, "prerequisite rows present");
    const auto prereq = [](std::string_view game, uint16_t species, uint16_t location, uint8_t level) {
        const auto* r = row(game, species, location, level);
        return r ? r->prereq : GS::Prerequisite::None;
    };
    require(prereq("emerald_gba", 249, 211, 70) == GS::Prerequisite::TicketOrItem, "Emerald Navel Rock: MysticTicket");
    require(prereq("emerald_gba", 386, 200, 30) == GS::Prerequisite::TicketOrItem, "Emerald Birth Island: AuroraTicket");
    require(prereq("emerald_gba", 151, 201, 30) == GS::Prerequisite::TicketOrItem, "Emerald Faraway Island: Old Sea Map");
    require(prereq("emerald_gba", 380, 73, 50) == GS::Prerequisite::TicketOrItem, "Emerald Southern Island: Eon Ticket");
    require(prereq("ruby_gba", 380, 73, 50) == GS::Prerequisite::TicketOrItem, "Ruby Southern Island: Eon Ticket");
    require(prereq("firered_gba", 386, 187, 30) == GS::Prerequisite::TicketOrItem, "FireRed Birth Island: AuroraTicket");
    require(prereq("platinum_nds", 492, 63, 30) == GS::Prerequisite::TicketOrItem, "Platinum Shaymin: Oak's Letter");
    require(prereq("platinum_nds", 491, 79, 50) == GS::Prerequisite::TicketOrItem, "Platinum Darkrai: Member Card");
    require(prereq("heartgold_nds", 483, 231, 1) == GS::Prerequisite::EventPokemon, "HeartGold Sinjoh Dialga: event Arceus");
    require(prereq("soulsilver_nds", 380, 140, 40) == GS::Prerequisite::TicketOrItem, "SoulSilver Latias: Enigma Stone");
    require(prereq("black_nds", 494, 62, 15) == GS::Prerequisite::TicketOrItem, "Black Victini: Liberty Pass");
    require(prereq("white_nds", 570, 8, 10) == GS::Prerequisite::EventPokemon, "White Zorua: event Celebi");
    require(prereq("crystal_gbc", 251, 14, 30) == GS::Prerequisite::TicketOrItem, "Crystal Celebi: GS Ball");
    require(prereq("crystal_gbc", 133, 16, 20) == GS::Prerequisite::None, "Crystal Eevee: no prerequisite");
    const auto* eevee = row("crystal_gbc", 133, 16, 20);
    const auto* lapras = row("firered_gba", 131, 134, 25);
    const auto* hitmonlee = row("firered_gba", 106, 98, 25);
    require(eevee && eevee->confidence == GS::Confidence::PretChecked, "Crystal Eevee: pret cross-checked");
    require(lapras && lapras->confidence == GS::Confidence::PretSpotChecked, "FireRed Lapras: pret spot-checked");
    require(hitmonlee && hitmonlee->confidence == GS::Confidence::SingleSource, "FireRed Hitmonlee: single source");

    // Gen IV / V: listed as the encounter, never verified; the note reads Incomplete.
    const auto incomplete45 = [](uint8_t origin, uint16_t species, uint8_t level, uint16_t location,
                                 const std::string& prerequisite, const std::string& what) {
        const C45::Catalogue context(origin, species, 0, gen45Native(origin), {}, level);
        const auto& choices = context.choices();
        const auto detail = context.detail(location, [](uint16_t s) { return std::string(Names::getSpeciesName(s)); });
        const auto note = C45::unverifiedMetNote45(origin, species, location, level);
        require(std::find(choices.begin(), choices.end(), location) != choices.end() &&
                !context.verifiedEncounter(location) && contains(detail, GS::kAccessUnverified) &&
                contains(note, "Incomplete") && contains(note, prerequisite),
                what + ": encounter listed, never verified (" + detail + " / " + note + ")");
    };
    incomplete45(12, 492, 30, 63, "ticket / distribution", "Platinum Shaymin (Oak's Letter)");
    incomplete45(12, 491, 50, 79, "ticket / distribution", "Platinum Darkrai (Member Card)");
    incomplete45(7, 483, 1, 231, "event Pokémon", "HeartGold Sinjoh Dialga");
    incomplete45(21, 494, 15, 62, "ticket / distribution", "Black Victini (Liberty Pass)");
    incomplete45(20, 570, 10, 8, "event Pokémon", "White Zorua (event Celebi)");
    incomplete45(21, 571, 25, 72, "event Pokémon", "Black Zoroark (event beasts)");
    incomplete45(10, 486, 70, 64, "another game", "Diamond Regigigas");
    // Coordinator follow-up (owner rule "unknown ... prerequisites -> Incomplete"; conservative):
    // B2/W2 Key System Regis and Regigigas, HG/SS Embedded Tower Kyogre / Groudon, D/P/Pt Spiritomb.
    for (const uint8_t origin : {22, 23}) {
        incomplete45(origin, 377, 65, 150, "link play", "B2/W2 Regirock (Key System)");
        incomplete45(origin, 378, 65, 151, "link play", "B2/W2 Regice (Key System)");
        incomplete45(origin, 379, 65, 152, "link play", "B2/W2 Registeel (Key System)");
        incomplete45(origin, 486, 68, 38, "another game", "B2/W2 Regigigas (the three Regis)");
    }
    incomplete45(7, 382, 50, 232, "not confirmed", "HeartGold Kyogre (Embedded Tower)");
    incomplete45(8, 383, 50, 232, "not confirmed", "SoulSilver Groudon (Embedded Tower)");
    for (const uint8_t origin : {10, 11, 12})
        incomplete45(origin, 442, 25, 24, "link play", "D/P/Pt Spiritomb (32 Underground players)");
    // Prerequisite-free statics stay verified (no blanket downgrade).
    gen45Verified(23, 488, 68, 68, "Static Lv. 68", "Black 2 Cresselia Marvelous Bridge");
    // A Gen IV / V verified data row says it is single source; a wild row keeps "" (unchanged).
    require(C45::unverifiedMetNote45(7, 152, 126, 5) == "Encounter data: single source (PKHeX)",
            "HeartGold Chikorita gift: verified, single source: " + C45::unverifiedMetNote45(7, 152, 126, 5));

    // Gen III: in the default list as the encounter, not a verified catch; Incomplete origin line.
    const auto incomplete3 = [](uint8_t origin, uint16_t species, uint8_t level, uint16_t location, const std::string& what) {
        const auto choices = G3::templatesForOrigin(origin, species);
        const auto rows = G3::defaultRows(choices, 0, origin, level);
        bool listed = false;
        for (const auto value : rows)
            if (const auto* choice = G3::choiceAt(choices, value); choice && choice->encounter.location == location) {
                const auto text = G3::rowText(choices, value, origin, 0, 0, level);
                listed = !text.verifiedCatch && contains(text.detail, GS::kAccessUnverified);
            }
        require(listed && !G3::verifiedTemplate(choices, location, level, level) &&
                contains(G3::encounterOriginText(choices, species, location, level, level), "Incomplete") &&
                contains(G3::evidenceNote(choices, species, location, level, level), "Incomplete"),
                what + ": listed as the encounter, never verified, Incomplete");
    };
    incomplete3(3, 249, 70, 211, "Emerald Lugia Navel Rock");
    incomplete3(3, 151, 30, 201, "Emerald Mew Faraway Island");
    incomplete3(2, 380, 50, 73, "Ruby Latias Southern Island");
    incomplete3(4, 386, 30, 187, "FireRed Deoxys Birth Island");
    const auto hitmonleeChoices = G3::templatesForOrigin(4, 106);
    require(G3::evidenceNote(hitmonleeChoices, 106, 98, 25, 25) == "Encounter data: single source (PKHeX)",
            "FireRed Hitmonlee gift: verified, single source");
    const auto laprasChoices = G3::templatesForOrigin(4, 131);
    require(G3::evidenceNote(laprasChoices, 131, 134, 25, 25).empty(), "FireRed Lapras gift: verified (pret spot-checked)");

    // Crystal: Celebi (GS Ball) is listed as the encounter with "access unverified"; the note is Incomplete.
    Picker::Model celebi;
    celebi.openLocation("crystal_gbc", 251, 0, 0, 30);
    require(crystalRow(celebi, 14, GS::kAccessUnverified) >= 0, "Crystal Celebi Ilex Forest: access unverified");
    require(contains(Rows::crystalUnverifiedNote("crystal_gbc", 251, 14, 30, 30, 1), "Incomplete"),
            "Crystal Celebi: Incomplete acquisition evidence");
    require(Rows::crystalUnverifiedNote("crystal_gbc", 133, 16, 20, 20, 1).empty(), "Crystal Eevee gift: verified");
}

} // namespace

int main() {
    dataSpotChecks();
    gen45PickerRows();
    gen3PickerRows();
    crystalPickerRows();
    crystalTimeRows();
    crystalRowsFit();
    crystalTimeRoundTrip();
    acquisitionTiers();
    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "Gift / static / trade data (Crystal, Gen III-V) verified rows, Crystal time rows: PASS\n";
    return 0;
}
