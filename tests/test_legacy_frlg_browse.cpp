#include "Games/GameIdentity.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Safety/SourceMutationPolicy.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
    std::vector<uint8_t> readFixture() {
        std::ifstream input("build-host/frlg_fixture.sav", std::ios::binary);
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }
}

int main() {
    using namespace PokeVault;
    using Integration::Gen3::SourceGame;

    auto fixture = readFixture();
    assert(fixture.size() == 0x20000);
    const auto sourceBefore = fixture;

    auto fireRedParsed = Integration::Gen3::parse(fixture, SourceGame::FireRedGBA);
    auto leafGreenParsed = Integration::Gen3::parse(fixture, SourceGame::LeafGreenGBA);
    assert(fireRedParsed && leafGreenParsed);

    Legacy::FRLGDiscoveryResult discovery;
    Legacy::FRLGSource fireRed;
    fireRed.path = "/retroarch/saves/Pokemon FireRed.sav";
    fireRed.canonicalPath = "/retroarch/saves/Pokemon FireRed.sav";
    fireRed.gameId = "firered_gba";
    fireRed.status = Legacy::LegacySourceStatus::Ready;
    fireRed.save = std::move(fireRedParsed.save);
    discovery.sources.push_back(std::move(fireRed));

    Legacy::FRLGSource ambiguous;
    ambiguous.path = "/retroarch/saves/FRLG.sav";
    ambiguous.status = Legacy::LegacySourceStatus::AmbiguousIdentity;
    discovery.sources.push_back(std::move(ambiguous));

    Legacy::FRLGSource leafGreen;
    leafGreen.path = "/retroarch/saves/Pokemon LeafGreen.srm";
    leafGreen.canonicalPath = "/retroarch/saves/Pokemon LeafGreen.srm";
    leafGreen.gameId = "leafgreen_gba";
    leafGreen.status = Legacy::LegacySourceStatus::Ready;
    leafGreen.save = std::move(leafGreenParsed.save);
    discovery.sources.push_back(std::move(leafGreen));

    // An alias of the first file must collapse inside the FireRed child list. A separately stored
    // file remains a distinct child even though this fixture deliberately has identical bytes.
    auto aliasParsed = Integration::Gen3::parse(fixture, SourceGame::FireRedGBA);
    auto separateParsed = Integration::Gen3::parse(fixture, SourceGame::FireRedGBA);
    assert(aliasParsed && separateParsed);
    Legacy::FRLGSource alias;
    alias.path = "/retroarch/cores/savefiles/Pokemon FireRed.sav";
    alias.canonicalPath = "/retroarch/saves/Pokemon FireRed.sav";
    alias.gameId = "firered_gba";
    alias.status = Legacy::LegacySourceStatus::Ready;
    alias.save = std::move(aliasParsed.save);
    discovery.sources.push_back(std::move(alias));
    Legacy::FRLGSource separate;
    separate.path = "/retroarch/saves/FireRed Backup.sav";
    separate.canonicalPath = "/retroarch/saves/FireRed Backup.sav";
    separate.gameId = "firered_gba";
    separate.status = Legacy::LegacySourceStatus::Ready;
    separate.save = std::move(separateParsed.save);
    discovery.sources.push_back(std::move(separate));

    const auto cards = Legacy::buildFRLGSourceCards(discovery);
    assert(cards.size() == 2);
    assert(cards[0].gameId == "firered_gba");
    assert(cards[0].title == "FireRed" && cards[0].platformLabel == "Game Boy Advance");
    assert(cards[0].sourceLabel == "RETROARCH");
    assert(cards[0].artworkKey == "firered_gba");
    assert(cards[0].instances.size() == 2);
    assert(cards[0].instances[0].sourceIndex == 0);
    assert(cards[0].instances[0].label == "Main Save");
    assert(cards[0].instances[1].sourceIndex == 4);
    assert(cards[0].instances[1].label == "Battery Save 2");
    assert(cards[1].gameId == "leafgreen_gba");
    assert(cards[1].artworkKey == "leafgreen_gba");
    assert(cards[1].instances.size() == 1 && cards[1].instances[0].sourceIndex == 2);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 0) == &discovery.sources[0]);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 1) == &discovery.sources[4]);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[1], 0) == &discovery.sources[2]);

    auto staleCard = cards[0];
    staleCard.gameId = "firered_switch";
    assert(Legacy::resolveFRLGSaveInstance(discovery, staleCard, 0) == nullptr);
    staleCard = cards[0];
    staleCard.instances[0].sourceIndex = 1;
    assert(Legacy::resolveFRLGSaveInstance(discovery, staleCard, 0) == nullptr);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 2) == nullptr);

    const auto strictParty = discovery.sources[0].save->party();
    const auto strictBoxes = discovery.sources[0].save->boxes();
    assert(strictParty.size() == 1 && strictBoxes.size() == 2);
    std::string error;
    auto trainer = Legacy::FRLGReadOnlyTrainer::create(*discovery.sources[0].save, error);
    assert(trainer && error.empty());
    assert(trainer->sourceGameId() == "firered_gba");
    assert(trainer->getPartySize() == 1);
    assert(trainer->getBoxCount() == 14 && trainer->getSlotsPerBox() == 30);
    assert(trainer->party[0]);
    assert(trainer->party[0]->speciesID() == strictParty[0].species);
    assert(trainer->party[0]->pid() == strictParty[0].pid);
    for (const auto& record : strictBoxes) {
        const auto& pokemon = trainer->boxes[record.location.box][record.location.slot];
        assert(pokemon);
        assert(pokemon->speciesID() == record.species);
        assert(pokemon->pid() == record.pid);
    }
    assert(trainer->createBlankPokemon() == nullptr);

    // Parsing and view-model construction retain source bytes exactly. The new source kind grants
    // View only; every inherited mutation category remains blocked.
    assert(fixture == sourceBefore);
    using Safety::SourceKind;
    using Safety::SourceMutation;
    assert(Safety::canPerform(SourceKind::RetroArchLegacy, SourceMutation::View));
    assert(!Safety::canPerform(SourceKind::RetroArchLegacy, SourceMutation::Edit));
    assert(!Safety::canPerform(SourceKind::RetroArchLegacy, SourceMutation::DirectMove));
    assert(!Safety::canPerform(SourceKind::RetroArchLegacy, SourceMutation::SaveChanges));

    const auto* gba = Games::findGame(cards[0].gameId);
    const auto* switchRelease = Games::findGame("firered_switch");
    assert(gba && switchRelease && gba->id != switchRelease->id);
    assert(gba->platform == Games::Platform::GameBoyAdvance);
    assert(switchRelease->platform == Games::Platform::NintendoSwitch);
    assert(Games::gameCardArtworkPath("firered_gba") ==
           "romfs:/game_cards/firered_gba.png");
    assert(Games::gameCardArtworkPath("leafgreen_gba") ==
           "romfs:/game_cards/leafgreen_gba.png");

    std::cout << "RetroArch FRLG selectable read-only browse tests passed\n";
    return 0;
}
