// Q04 (independent QA 2026-10-27; upstream PKSE include/Enums/GameVersion.h:163-174 at a79fe61a):
// all twelve FireRed/LeafGreen Switch releases are recognised with the right game, language and
// family, each keeps its own identity and backup namespace, and unknown title ids stay refused.
#include "Enums/GameVersion.h"
#include "Games/GameIdentity.h"
#include "Utils/MoveTransactionProduction.h"
#include "Utils/PokeBankPaths.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>

using namespace PokeVault::Games;
using Enums::GameVersion;

namespace {
struct Release {
    uint64_t titleId;
    GameVersion version;
    const char* id;
    uint8_t language;  // PKHeX LanguageID
};

// The exact table upstream PKSE pins (title id, game, language).
constexpr Release kReleases[] = {
    {0x01006FA0233F8000ULL, GameVersion::FR, "firered_switch_jpn", 1},
    {0x0100554023408000ULL, GameVersion::FR, "firered_switch", 2},
    {0x01004B3023412000ULL, GameVersion::FR, "firered_switch_fra", 3},
    {0x010092302342A000ULL, GameVersion::FR, "firered_switch_ita", 4},
    {0x01007F8023416000ULL, GameVersion::FR, "firered_switch_deu", 5},
    {0x0100EB702342C000ULL, GameVersion::FR, "firered_switch_esp", 7},
    {0x0100F1E0233FA000ULL, GameVersion::LG, "leafgreen_switch_jpn", 1},
    {0x010034D02340E000ULL, GameVersion::LG, "leafgreen_switch", 2},
    {0x010087C02342E000ULL, GameVersion::LG, "leafgreen_switch_fra", 3},
    {0x01005C7023432000ULL, GameVersion::LG, "leafgreen_switch_ita", 4},
    {0x0100FD6023430000ULL, GameVersion::LG, "leafgreen_switch_deu", 5},
    {0x01002B5023434000ULL, GameVersion::LG, "leafgreen_switch_esp", 7},
};
}  // namespace

int main() {
    AccountUid profile{{0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL}};
    AccountUid otherProfile{{0x1111111111111111ULL, 0x2222222222222222ULL}};
    std::set<std::string> ids, backupRoots;
    for (const auto& r : kReleases) {
        // Discovery: SaveSelectScreen keeps a tile only when both lookups succeed.
        assert(Enums::getGameVersion(r.titleId) == r.version);
        assert(Enums::getGameGroup(r.version) == GameVersion::FRLG);
        const auto* game = findSwitchGame(r.titleId);
        assert(game && game->id == r.id);
        assert(game->platform == Platform::NintendoSwitch && game->dataGeneration == 3);
        assert(game->support == SourceSupport::NativeSwitch);
        // Exact identity and language; shared behaviour family.
        assert(ids.insert(std::string(game->id)).second);
        assert(switchReleaseLanguage(game->id) == r.language);
        const std::string_view family = switchReleaseFamily(game->id);
        assert(family == (r.version == GameVersion::FR ? "firered_switch" : "leafgreen_switch"));
        assert(gameRegionBackdropKey(game->id) == "kanto");
        // Storage never shares a namespace: one exact-game backup root per release and profile.
        const auto root = PokeBank::Paths::exactGameBackupsRoot(profile, game->id);
        assert(!root.empty() && root.find(std::string(game->id)) != std::string::npos);
        assert(backupRoots.insert(root).second);
        assert(PokeBank::Paths::exactGameBackupsRoot(otherProfile, game->id) != root);
        // Writes stay as narrow as before: durable true Move only for the original English pair,
        // the only releases validated for it. Localized releases are refused, not guessed.
        const bool english = r.language == 2;
        assert(PokeBank::Storage::MoveTx::Production::supportsSingleFileTrueMove(game->id) == english);
    }
    assert(ids.size() == 12 && backupRoots.size() == 12);

    // The English ids are unchanged, so existing backups keep their folders.
    assert(findGame("firered_switch")->switchTitleId == 0x0100554023408000ULL);
    assert(findGame("leafgreen_switch")->switchTitleId == 0x010034D02340E000ULL);
    // Non-FRLG ids are untouched by the family/language helpers.
    assert(switchReleaseFamily("sword_switch") == "sword_switch");
    assert(switchReleaseLanguage("sword_switch") == 0 && switchReleaseLanguage("firered_gba") == 0);
    assert(switchReleaseFamily("firered_switch_xyz") == "firered_switch_xyz");  // not a registered id

    // Unknown title ids, including near-misses of the FRLG ids, remain unrecognised.
    for (const uint64_t unknown : {0x0100554023409000ULL, 0x01006FA0233F8001ULL, 0x0ULL, 0xFFFFFFFFFFFFFFFFULL}) {
        assert(Enums::getGameVersion(unknown) == GameVersion::Invalid);
        assert(findSwitchGame(unknown) == nullptr);
    }
    std::cout << "FireRed/LeafGreen Switch releases (12 exact identities, unique backup namespaces): PASS\n";
}
