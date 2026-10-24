// Gen V native Trainer: cited PlayerData (27) / Misc (52) decode, staged
// Name/Money/Play Time through the shared Trainer panel model, read-only
// ownership/story rows, Review rows and verified export with a byte whitelist.
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5TrainerScreen.h"
#include "Integration/Gen5/Gen5HubRouting.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace T = PokeVault::Integration::Gen5::TrainerLayout5;
using Screen = G::Gen5TrainerScreen;
using Row = Screen::Row;

namespace {

std::span<const L::SaveBlock> blocksFor(G::SaveFamily family) {
    return family == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                               : std::span<const L::SaveBlock>(L::Black2White2);
}

// Synthetic SAV5 (primary copy) with trainer "NX", TID/SID, 3 badges,
// 123456 money, 12:34:56 play time, one party PK5 and one bag stack.
std::vector<uint8_t> fixture(G::SaveFamily family, uint8_t version) {
    std::vector<uint8_t> sav(L::FullSaveSize, 0);
    const size_t player = G::trainerBlock5(family, T::PlayerBlockIndex).offset;
    const size_t misc = G::trainerBlock5(family, T::MiscBlockIndex).offset;
    C::write16(sav, player + 0x04, 'N');
    C::write16(sav, player + 0x06, 'X');
    C::write16(sav, player + 0x08, 0xFFFF);
    C::write16(sav, player + 0x14, 12345);
    C::write16(sav, player + 0x16, 54321);
    sav[player + 0x1E] = 2;
    sav[player + 0x1F] = version;
    sav[player + 0x21] = 1;
    C::write16(sav, player + 0x24, 12);
    sav[player + 0x26] = 34;
    sav[player + 0x27] = 56;
    sav[player + 0x30] = 0x5A;   // unrelated PlayerData byte (must survive)
    C::write16(sav, misc + 0x00, static_cast<uint16_t>(123456 & 0xFFFF));
    C::write16(sav, misc + 0x02, static_cast<uint16_t>(123456 >> 16));
    sav[misc + 0x04] = 0x07;      // three badges
    sav[misc + 0x40] = 0xA5;      // unrelated Misc byte (must survive)
    std::vector<uint8_t> pk(C::PartySize, 0);
    C::write16(pk, 0, 0x5678);
    C::write16(pk, 8, 25);
    pk[0x8C] = 30;
    const auto party = C::encryptCandidate(pk);
    sav[L::PartyOffset + 4] = 1;
    std::copy(party.begin(), party.end(), sav.begin() + static_cast<std::ptrdiff_t>(L::PartyOffset + 8));
    // Pokedex flags (Zukan5): Pikachu caught + seen (region 0), Eevee seen
    // only as female shiny (region 3), Victini (494) caught only.
    const size_t dex = family == G::SaveFamily::BlackWhite ? 0x21600 : 0x21400;
    sav[dex + 0x08 + (24 >> 3)] |= uint8_t(1u << (24 & 7));
    sav[dex + 0x5C + (24 >> 3)] |= uint8_t(1u << (24 & 7));
    sav[dex + 0x5C + 3 * 0x54 + (132 >> 3)] |= uint8_t(1u << (132 & 7));
    sav[dex + 0x08 + (493 >> 3)] |= uint8_t(1u << (493 & 7));
    const size_t items = G::itemsBlockOffset(family);
    C::write16(sav, items, 4);
    C::write16(sav, items + 2, 10);
    const size_t copySize = family == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto copy = std::span<uint8_t>(sav).subspan(0, copySize);
    for (const auto& b : blocksFor(family)) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

const G::Gen5StagedPokemonWorkspace& ws_probe(const G::Gen5ReadOnlySave& save) {
    static std::optional<G::Gen5StagedPokemonWorkspace> probe;
    probe.emplace(save);
    return *probe;
}

bool hasGlyph(const std::vector<Screen::FooterEntry>& footer, std::string_view glyph) {
    return std::any_of(footer.begin(), footer.end(), [&](const auto& e) { return glyph == e.glyph; });
}

void exercise(G::SaveFamily family, std::string_view game, uint8_t version) {
    const auto sav = fixture(family, version);
    const auto pristine = sav;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, game, &error, G::SaveCopySelection::Primary);
    assert(parsed);
    const auto decoded = G::decodeTrainer5(*parsed);
    assert(decoded && decoded->rawName == u"NX" && decoded->tid == 12345 && decoded->sid == 54321);
    assert(decoded->money == 123456 && decoded->badges == 7 && decoded->badgeCount() == 3);
    assert(decoded->hours == 12 && decoded->minutes == 34 && decoded->seconds == 56);
    assert(decoded->gender == 1 && decoded->language == 2 && decoded->game == version);
    // Agrees with the existing strict reader's trainer view.
    assert(parsed->trainer().rawName == decoded->rawName && parsed->trainer().playedHours == 12);

    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, std::string(game), error);
    assert(trainer && trainer->trainerName == "NX" && trainer->money == 123456);
    assert(trainer->getMaxMoney() == 9999999 && trainer->getMaxTrainerNameLength() == 7);
    auto& ws = trainer->stagedPokemon();
    auto& screen = trainer->trainerScreen();
    auto& shared = trainer->sharedScreen();
    const auto refresh = [&](std::string& why) { return trainer->refreshStagedPokemonPresentation(why); };
    const auto refuse = [](std::string& why) { why = "refused"; return false; };

    // Read-only Pokedex from the cited Zukan5 block, consistent with the
    // existing card preview counter.
    const auto dex = G::decodePokedex5(*parsed);
    assert(dex && dex->caught == 2 && dex->seen == 2);
    assert(dex->entries[24].caught && dex->entries[24].seen);
    assert(dex->entries[132].seen && !dex->entries[132].caught);
    assert(dex->entries[493].caught && !dex->entries[493].seen);
    assert(parsed->dexProgress().caught == dex->caught && parsed->dexProgress().seen == dex->seen);
    assert(Screen::value(ws_probe(*parsed), Row::Pokedex) == "Seen 2 / Caught 2");

    // Closed: A opens; open: Up/Down/tap focus, A edits only editable rows.
    assert(hasGlyph(screen.footer(), "A") && !screen.open());
    assert(!screen.beginEdit(ws, error));
    assert(screen.openEditor(ws, error) && screen.open() && screen.selectedRow() == Row::Name);
    assert(Screen::value(ws, Row::Name) == "NX" && Screen::value(ws, Row::Money) == "123456");
    assert(Screen::value(ws, Row::PlayTime) == "12:34:56" && Screen::value(ws, Row::Badges) == "3 / 8");
    assert(Screen::value(ws, Row::Gender) == "Female");
    // Gen I-IV Trainer convention: read-only rows are shown but never focused;
    // Up/Down wrap over Name, Money, Play Time and the Pokedex list.
    for (const Row ro : {Row::TrainerId, Row::SecretId, Row::Gender, Row::Badges, Row::Language})
        assert(!Screen::editable(ro) && !Screen::focusable(ro));
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::Money);
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::PlayTime);
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::Pokedex);  // skips the read-only rows
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::Name);     // wraps
    screen.moveRow(-1);
    assert(screen.selectedRow() == Row::Pokedex);
    for (size_t i = 0; i < Screen::kRows.size(); ++i)
        if (!Screen::focusable(Screen::kRows[i])) assert(!screen.tapRow(i));
    assert(screen.selectedRow() == Row::Pokedex);
    // Pokedex row: A opens a read-only list; nothing becomes staged.
    assert(hasGlyph(screen.footer(), "A") && !screen.beginEdit(ws, error));
    assert(screen.openPokedex(ws, error) && screen.dex().open() && screen.dex().cursor() == 0);
    screen.dex().jumpRegistered(*dex, 1);
    assert(screen.dex().cursor() == 24);
    screen.dex().jumpRegistered(*dex, 1);
    assert(screen.dex().cursor() == 132);
    screen.dex().jumpRegistered(*dex, -1);
    assert(screen.dex().cursor() == 24);
    screen.dex().move(-1000);
    assert(screen.dex().cursor() == 0);
    screen.dex().move(1000);
    assert(screen.dex().cursor() == 648);
    assert(!hasGlyph(screen.footer(), "A") && !ws.hasChanges());
    screen.dex().close();
    // A completed tap on an editable row selects it and acts like A.
    assert(screen.tapRow(1) && screen.selectedRow() == Row::Money);
    assert(screen.beginEdit(ws, error));
    // Wrong-row apply is refused (stale selection).
    assert(!screen.applyName(ws, "RED", refresh, error) && !ws.hasChanges());

    // Money: bounds, rollback on refused refresh, staged value presented.
    assert(!screen.applyMoney(ws, 10000000, refresh, error) && !ws.hasChanges());
    assert(!screen.applyMoney(ws, 500, refuse, error) && !ws.hasChanges() && trainer->money == 123456);
    assert(screen.applyMoney(ws, 9999999, refresh, error) && error.empty());
    assert(trainer->money == 9999999 && ws.trainerDirty() && trainer->hasStagedChanges());

    // Name: printable ASCII 1..7, presented after staging.
    screen.moveRow(-1);
    assert(screen.selectedRow() == Row::Name);
    assert(!screen.applyName(ws, "", refresh, error));
    assert(!screen.applyName(ws, "TOOLONGX", refresh, error));
    assert(!screen.applyName(ws, std::string("A\x01"), refresh, error));
    assert(!screen.applyName(ws, " LEAD", refresh, error));
    assert(screen.applyName(ws, "Hilda7", refresh, error));
    assert(trainer->trainerName == "Hilda7");

    // Play time: 0..999:59:59.
    screen.moveRow(2);
    assert(screen.selectedRow() == Row::PlayTime);
    assert(!screen.applyPlayTime(ws, 1000, 0, 0, refresh, error));
    assert(!screen.applyPlayTime(ws, 1, 60, 0, refresh, error));
    assert(!screen.applyPlayTime(ws, 1, 0, 60, refresh, error));
    assert(screen.applyPlayTime(ws, 999, 59, 59, refresh, error));
    assert(Screen::value(ws, Row::PlayTime) == "999:59:59");

    // Review lists the three Trainer changes; the discoverable Y button opens it.
    const auto changes = ws.trainerReviewChanges();
    assert(changes.size() == 3);
    assert(changes[0].field == G::TrainerChange5::Field::Name && changes[0].before == "NX" &&
           changes[0].after == "Hilda7");
    assert(changes[1].before == "123456" && changes[1].after == "9999999");
    assert(changes[2].before == "12:34:56" && changes[2].after == "999:59:59");
    assert(G::Gen5SharedScreenState::reviewRowCount(ws) == 3);
    assert(hasGlyph(screen.footer(), "Y"));
    assert(shared.openReviewFromBrowse(ws));
    (void)shared.back();

    // Export: only the cited ranges plus block/terminal CRCs differ.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && out->trainerChanged && !out->bagChanged && out->changedRecords == 0);
    const auto& playerBlock = G::trainerBlock5(family, T::PlayerBlockIndex);
    const auto& miscBlock = G::trainerBlock5(family, T::MiscBlockIndex);
    const auto& terminal = blocksFor(family).back();
    const auto crcByte = [&](size_t i, const L::SaveBlock& b) {
        return i == b.checksumOffset || i == b.checksumOffset + 1 || i == b.mirrorOffset || i == b.mirrorOffset + 1;
    };
    for (size_t i = 0; i < sav.size(); ++i) {
        if (out->bytes[i] == sav[i]) continue;
        const bool name = i >= playerBlock.offset + 0x04 && i < playerBlock.offset + 0x14;
        const bool time = i >= playerBlock.offset + 0x24 && i < playerBlock.offset + 0x28;
        const bool money = i >= miscBlock.offset && i < miscBlock.offset + 4;
        assert(name || time || money || crcByte(i, playerBlock) || crcByte(i, miscBlock) || crcByte(i, terminal));
    }
    const auto reopened = G::Gen5ReadOnlySave::parse(out->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(reopened);
    const auto exported = G::decodeTrainer5(*reopened);
    assert(exported && exported->rawName == u"Hilda7" && exported->money == 9999999);
    assert(exported->hours == 999 && exported->minutes == 59 && exported->seconds == 59);
    assert(exported->tid == 12345 && exported->sid == 54321 && exported->badges == 7);
    assert(reopened->sourceBytes()[playerBlock.offset + 0x30] == 0x5A);
    assert(reopened->sourceBytes()[miscBlock.offset + 0x40] == 0xA5);
    // Name field is ClearZero + terminator per StringConverter5.
    assert(C::read16(reopened->sourceBytes(), playerBlock.offset + 0x04 + 6 * 2) == 0xFFFF);
    assert(C::read16(reopened->sourceBytes(), playerBlock.offset + 0x04 + 7 * 2) == 0);
    // Stale source refused.
    auto stale = sav;
    stale[miscBlock.offset + 0x40] ^= 1;
    assert(!G::buildStagedSaveCandidateForObservedSource(ws, stale, &error));
    assert(G::buildStagedSaveCandidateForObservedSource(ws, sav, &error));

    // Combined with Bag + PK5 edits.
    auto* bag = ws.stagedBag(&error);
    assert(bag && bag->setQuantity(0, 0, 77, &error));
    using Field = G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0, Field::Friendship, 0, 9, &error));
    const auto all = G::buildStagedSaveCandidate(ws, &error);
    assert(all && all->trainerChanged && all->bagChanged && all->changedRecords == 1);
    const auto all2 = G::Gen5ReadOnlySave::parse(all->bytes, game, &error, G::SaveCopySelection::Primary);
    assert(all2 && G::decodeTrainer5(*all2)->money == 9999999 && (*G::decodeBag5(*all2))[0][0].count == 77);
    assert(all2->partyPokemon(0)->friendship() == 9);

    // Source immutable; Discard All restores presentation.
    assert(sav == pristine);
    ws.discardAll();
    std::string why;
    assert(trainer->refreshStagedPokemonPresentation(why));
    assert(trainer->trainerName == "NX" && trainer->money == 123456 && !ws.hasChanges());
    screen.close();
    assert(!screen.open());
}

void unrecognizedTrainerIsViewOnly() {
    // Eight name code units without a terminator: the trainer blocks are not
    // a recognized native name field, so editing is unavailable.
    auto sav = fixture(G::SaveFamily::BlackWhite, 21);
    const size_t player = G::trainerBlock5(G::SaveFamily::BlackWhite, T::PlayerBlockIndex).offset;
    for (size_t i = 0; i < 8; ++i) C::write16(sav, player + 0x04 + i * 2, 'A');
    auto copy = std::span<uint8_t>(sav).subspan(0, L::BlackWhiteCopySize);
    for (const auto& b : L::BlackWhite) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, "black_nds", &error, G::SaveCopySelection::Primary);
    assert(parsed && !G::decodeTrainer5(*parsed));
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    Screen screen;
    assert(!Screen::available(ws) && !screen.openEditor(ws, error) && !error.empty());
    assert(!ws.stagedTrainer(&error) && !ws.hasChanges());
    assert(Screen::value(ws, Row::Money) == "Unavailable");
}

} // namespace

// Hardware failure fa9cfcea: Gen V opened forced into Party and used a
// separate Trainer pop-up. It now opens on the shared hub; these are the
// route decisions the Gen V surface applies on top of the base hub.
void hubRoutes() {
    using G::HubCommand5;
    using G::HubTile5;
    constexpr auto t = [](HubTile5 tile) { return static_cast<int>(tile); };
    // Focus movement and help stay with the base hub.
    static_assert(G::routeHub5(0, false, false, false, -1).command == HubCommand5::PassToBase);
    // A on each tile.
    static_assert(G::routeHub5(t(HubTile5::Pokemon), true, false, false, -1).command == HubCommand5::Enter);
    static_assert(G::routeHub5(t(HubTile5::Party), true, false, false, -1).tile == t(HubTile5::Party));
    static_assert(G::routeHub5(t(HubTile5::Items), true, false, false, -1).command == HubCommand5::Enter);
    static_assert(G::routeHub5(t(HubTile5::Trainer), true, false, false, -1).command == HubCommand5::Enter);
    // Legacy Storage (Bank) never opens for Gen V: no PK5 conversion exists.
    static_assert(G::routeHub5(t(HubTile5::Storage), true, false, false, -1).command ==
                  HubCommand5::StorageRefused);
    // A completed tap focuses AND activates, exactly like A; Storage taps refused too.
    static_assert(G::routeHub5(0, false, false, false, 104).command == HubCommand5::Enter);
    static_assert(G::routeHub5(0, false, false, false, 104).tile == t(HubTile5::Trainer));
    static_assert(G::routeHub5(4, false, false, false, 102).command == HubCommand5::StorageRefused);
    static_assert(G::routeHub5(0, false, false, false, 105).command == HubCommand5::PassToBase);
    static_assert(G::routeHub5(0, false, false, false, 99).command == HubCommand5::PassToBase);
    // B exits through the Gen V confirmation; + opens Review; out-of-range focus clamps.
    static_assert(G::routeHub5(3, false, true, false, -1).command == HubCommand5::Exit);
    static_assert(G::routeHub5(3, false, false, true, -1).command == HubCommand5::Review);
    static_assert(G::routeHub5(9, true, false, false, -1).tile == 0);
    // Panel B returns to the hub except for the Product Home Items shortcut.
    static_assert(G::panelBack5(false) == G::PanelBack5::ToHub);
    static_assert(G::panelBack5(true) == G::PanelBack5::ExitSession);
}

void trainerPanelFocusMemory() {
    const auto sav = fixture(G::SaveFamily::Black2White2, 23);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(sav, "black2_nds", &error, G::SaveCopySelection::Primary);
    assert(parsed);
    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black2_nds", error);
    assert(trainer);
    auto& ws = trainer->stagedPokemon();
    auto& screen = trainer->trainerScreen();
    // Hub -> Trainer: the shared panel opens on Name.
    assert(screen.enter(ws, error) && screen.open() && screen.selectedRow() == Row::Name);
    screen.moveRow(2);
    assert(screen.selectedRow() == Row::PlayTime);
    // B -> hub keeps the focused row; re-entering restores it.
    screen.leave();
    assert(!screen.open());
    assert(screen.enter(ws, error) && screen.selectedRow() == Row::PlayTime);
    assert(screen.beginEdit(ws, error));
    // The next focus skips the read-only identity rows to the Pokedex list,
    // which opens a view rather than an edit.
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::Pokedex && !screen.beginEdit(ws, error));
    screen.moveRow(-3);
    assert(screen.selectedRow() == Row::Name && screen.beginEdit(ws, error));
    // Scrolling keeps the focused row visible for any viewport size.
    for (size_t visible = 1; visible <= Screen::kRows.size() + 1; ++visible)
        for (size_t row = 0; row < Screen::kRows.size(); ++row) {
            assert(screen.enter(ws, error));
            screen.moveRow(static_cast<int>(row) - static_cast<int>(screen.row()));
            const size_t first = screen.firstVisible(visible);
            assert(screen.row() >= first && screen.row() < first + visible);
            assert(first + std::min(visible, Screen::kRows.size()) <= Screen::kRows.size());
        }
    // Wrap-around in both directions.
    while (screen.selectedRow() != Row::Name) screen.moveRow(1);
    screen.moveRow(-1);
    assert(screen.selectedRow() == Row::Pokedex);
    screen.moveRow(1);
    assert(screen.selectedRow() == Row::Name);
    // Parity footer: no X/Y mode shortcuts.
    screen.leave();
    assert(!hasGlyph(screen.footer(), "X") && !hasGlyph(screen.footer(), "Y"));
}

int main() {
    hubRoutes();
    trainerPanelFocusMemory();
    static_assert(G::trainerNameAccepted5("Hilda"));
    static_assert(!G::trainerNameAccepted5("12345678"));
    exercise(G::SaveFamily::BlackWhite, "white_nds", 20);
    exercise(G::SaveFamily::BlackWhite, "black_nds", 21);
    exercise(G::SaveFamily::Black2White2, "white2_nds", 22);
    exercise(G::SaveFamily::Black2White2, "black2_nds", 23);
    unrecognizedTrainerIsViewOnly();
    std::cout << "Gen V native Trainer: cited decode, staged Name/Money/Play Time, read-only rows, "
                 "Review and verified export PASS\n";
}
