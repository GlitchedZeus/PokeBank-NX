#include "Games/GameIdentity.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/FRLGSourceBrowser.h"
#include "Safety/SourceMutationPolicy.h"

#include <cassert>
#include <cstring>
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

    uint16_t read16(const std::vector<uint8_t>& bytes, size_t offset) {
        return static_cast<uint16_t>(bytes[offset]) |
               static_cast<uint16_t>(bytes[offset + 1] << 8);
    }

    void write16(std::vector<uint8_t>& bytes, size_t offset, uint16_t value) {
        bytes[offset] = static_cast<uint8_t>(value);
        bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
    }

    uint16_t sectorChecksum(const uint8_t* bytes) {
        uint32_t sum = 0;
        for (size_t offset = 0; offset < 0xF80; offset += 4) {
            sum += static_cast<uint32_t>(bytes[offset]) |
                   (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
                   (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
                   (static_cast<uint32_t>(bytes[offset + 3]) << 24);
        }
        return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
    }

    std::vector<uint8_t> makeTwoPokemonFixture(const std::vector<uint8_t>& fixture) {
        std::vector<uint8_t> current = fixture;
        constexpr size_t activeSlot = 0xE000;
        constexpr size_t sectorSize = 0x1000;
        size_t partySector = current.size();
        for (size_t physical = 0; physical < 14; ++physical) {
            const size_t offset = activeSlot + physical * sectorSize;
            if (read16(current, offset + 0xFF4) == 1) {
                partySector = offset;
                break;
            }
        }
        assert(partySector < current.size());
        current[partySector + 0x34] = 2;
        std::memcpy(current.data() + partySector + 0x38 + 100,
                    current.data() + partySector + 0x38, 100);
        write16(current, partySector + 0xFF6,
                sectorChecksum(current.data() + partySector));
        return current;
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
    fireRed.normalizedPath = fireRed.path;
    fireRed.canonicalPath = "/retroarch/saves/Pokemon FireRed.sav";
    fireRed.sourceIdentity = "source-firered-primary";
    fireRed.contentFingerprint = "11111111111111111111111111111111";
    fireRed.fileSize = 0x20000;
    fireRed.modifiedTime = 100;
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
    leafGreen.normalizedPath = leafGreen.path;
    leafGreen.canonicalPath = "/retroarch/saves/Pokemon LeafGreen.srm";
    leafGreen.sourceIdentity = "source-leafgreen";
    leafGreen.contentFingerprint = "22222222222222222222222222222222";
    leafGreen.fileSize = 0x20000;
    leafGreen.modifiedTime = 150;
    leafGreen.gameId = "leafgreen_gba";
    leafGreen.status = Legacy::LegacySourceStatus::Ready;
    leafGreen.save = std::move(leafGreenParsed.save);
    discovery.sources.push_back(std::move(leafGreen));

    // An alias of the first file must collapse inside the FireRed child list. A separately stored
    // file remains a distinct child even though this fixture deliberately has identical bytes.
    auto aliasParsed = Integration::Gen3::parse(fixture, SourceGame::FireRedGBA);
    const auto currentFixture = makeTwoPokemonFixture(fixture);
    auto separateParsed = Integration::Gen3::parse(currentFixture, SourceGame::FireRedGBA);
    assert(aliasParsed && separateParsed);
    Legacy::FRLGSource alias;
    alias.path = "/retroarch/cores/savefiles/Pokemon FireRed.sav";
    alias.normalizedPath = alias.path;
    alias.canonicalPath = "/retroarch/saves/Pokemon FireRed.sav";
    alias.sourceIdentity = "source-firered-primary";
    alias.contentFingerprint = "11111111111111111111111111111111";
    alias.fileSize = 0x20000;
    alias.modifiedTime = 200;
    alias.gameId = "firered_gba";
    alias.status = Legacy::LegacySourceStatus::Ready;
    alias.save = std::move(aliasParsed.save);
    discovery.sources.push_back(std::move(alias));
    Legacy::FRLGSource separate;
    separate.path = "/retroarch/saves/Pokemon FireRed Current.srm";
    separate.normalizedPath = separate.path;
    separate.canonicalPath = "/retroarch/saves/Pokemon FireRed Current.srm";
    separate.sourceIdentity = "source-firered-backup";
    separate.contentFingerprint = "33333333333333333333333333333333";
    separate.fileSize = 0x20000;
    separate.modifiedTime = 300;
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
    // The rejected build called the lexicographically first old copy "Main Save" and focused it.
    // The model now exposes actual filenames/contents and puts the newest mtime first without
    // claiming that recency alone proves which file RetroArch is actively using.
    assert(cards[0].instances[0].sourceIndex == 4);
    assert(cards[0].instances[0].label == "Pokemon FireRed Current.srm");
    assert(cards[0].instances[0].partyCount == 2);
    assert(cards[0].instances[0].contentFingerprint.starts_with("333333"));
    assert(cards[0].instances[0].mostRecentlyModified);
    assert(cards[0].instances[1].sourceIndex == 0);
    assert(cards[0].instances[1].label == "Pokemon FireRed.sav");
    assert(cards[0].instances[1].partyCount == 1);
    assert(!cards[0].instances[1].mostRecentlyModified);
    assert(cards[1].gameId == "leafgreen_gba");
    assert(cards[1].artworkKey == "leafgreen_gba");
    assert(cards[1].instances.size() == 1 && cards[1].instances[0].sourceIndex == 2);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 0) == &discovery.sources[4]);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 1) == &discovery.sources[0]);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[1], 0) == &discovery.sources[2]);

    // The physical catalog is shared, but normal cards are visible only to the explicitly bound
    // profile. Save contents and trainer identity never decide ownership.
    Legacy::LegacySourceBindings bindings;
    assert(bindings.assign("source-firered-primary", "profile-a"));
    assert(bindings.assign("source-firered-backup", "profile-a"));
    assert(bindings.assign("source-leafgreen", "profile-b"));
    const auto profileA = Legacy::buildFRLGSourceCardsForProfile(discovery, bindings, "profile-a");
    const auto profileB = Legacy::buildFRLGSourceCardsForProfile(discovery, bindings, "profile-b");
    assert(profileA.size() == 1 && profileA[0].gameId == "firered_gba");
    assert(profileA[0].instances.size() == 2);
    assert(profileB.size() == 1 && profileB[0].gameId == "leafgreen_gba");
    assert(profileB[0].instances.size() == 1);
    assert(Legacy::buildFRLGSourceCardsForProfile(discovery, bindings, "profile-c").empty());

    auto staleCard = cards[0];
    staleCard.gameId = "firered_switch";
    assert(Legacy::resolveFRLGSaveInstance(discovery, staleCard, 0) == nullptr);
    staleCard = cards[0];
    staleCard.instances[0].sourceIndex = 1;
    assert(Legacy::resolveFRLGSaveInstance(discovery, staleCard, 0) == nullptr);
    staleCard = cards[0];
    ++staleCard.instances[0].modifiedTime;
    assert(Legacy::resolveFRLGSaveInstance(discovery, staleCard, 0) == nullptr);
    assert(Legacy::resolveFRLGSaveInstance(discovery, cards[0], 2) == nullptr);

    const auto strictParty = discovery.sources[0].save->party();
    const auto strictBoxes = discovery.sources[0].save->boxes();
    assert(strictParty.size() == 1 && strictBoxes.size() == 2);
    std::string error;
    auto trainer = Legacy::FRLGReadOnlyTrainer::create(*discovery.sources[0].save, error);
    assert(trainer && error.empty());
    assert(trainer->sourceGameId() == "firered_gba");
    assert(trainer->trainerName == "WILL");
    assert(trainer->trainerGender == 1);
    assert(trainer->TID16 == 54321 && trainer->SID16 == 12345);
    assert(trainer->ID32 == (static_cast<uint32_t>(12345) << 16 | 54321));
    assert(trainer->TID == 54321 && trainer->SID == 12345);
    assert(trainer->money == 500000);
    assert(trainer->items.size() == 6);
    const uint16_t expectedItemIds[] = {13, 259, 4, 289, 133, 1};
    const uint16_t expectedItemCounts[] = {25, 1, 50, 3, 8, 7};
    for (size_t pouch = 0; pouch < trainer->items.size(); ++pouch) {
        assert(trainer->items[pouch].size() == 1);
        assert(trainer->items[pouch][0].itemId == expectedItemIds[pouch]);
        assert(trainer->items[pouch][0].count == expectedItemCounts[pouch]);
    }
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
