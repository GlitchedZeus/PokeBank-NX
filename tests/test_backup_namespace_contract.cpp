#include "Games/GameIdentity.h"
#include "Utils/PokeBankPaths.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    using namespace PokeBank::Paths;
    using PokeVault::Games::findGame;
    using PokeVault::Games::findSwitchGame;

    const AccountUid profileA{{0x1111111111111111ULL, 0x2222222222222222ULL}};
    const AccountUid profileB{{0x3333333333333333ULL, 0x4444444444444444ULL}};
    const AccountUid invalidProfile{};

    assert(accountUidHex(profileA) == "11111111111111112222222222222222");
    assert(backupProfileComponent(profileA) ==
           "account-11111111111111112222222222222222");
    assert(accountUidHex(invalidProfile).empty());
    assert(backupProfileComponent(invalidProfile).empty());

    const auto* sword = findSwitchGame(0x0100ABF008968000ULL);
    const auto* shield = findSwitchGame(0x01008DB008C2C000ULL);
    const auto* letsGoPikachu = findSwitchGame(0x010003F003A34000ULL);
    const auto* letsGoEevee = findSwitchGame(0x0100187003A36000ULL);
    const auto* fireRedSwitch = findSwitchGame(0x0100554023408000ULL);
    const auto* leafGreenSwitch = findSwitchGame(0x010034D02340E000ULL);
    const auto* fireRedGba = findGame("firered_gba");

    assert(sword && shield && letsGoPikachu && letsGoEevee &&
           fireRedSwitch && leafGreenSwitch && fireRedGba);
    assert(sword->id == "sword_switch");
    assert(shield->id == "shield_switch");
    assert(letsGoPikachu->id == "letsgo_pikachu_switch");
    assert(letsGoEevee->id == "letsgo_eevee_switch");
    assert(fireRedSwitch->id == "firered_switch");
    assert(leafGreenSwitch->id == "leafgreen_switch");
    assert(fireRedGba->id == "firered_gba");
    assert(fireRedSwitch->id != fireRedGba->id);

    const std::string swordA = exactGameBackupsRoot(profileA, sword->id);
    assert(swordA ==
           backupsRoot() +
           "/account-11111111111111112222222222222222/sword_switch");

    // Same account + same exact release is deterministic.
    assert(swordA == exactGameBackupsRoot(profileA, sword->id));

    // The same release under two Switch accounts can never share a writable namespace.
    assert(swordA != exactGameBackupsRoot(profileB, sword->id));

    // Related releases under one account remain distinct.
    assert(swordA != exactGameBackupsRoot(profileA, shield->id));
    assert(exactGameBackupsRoot(profileA, letsGoPikachu->id) !=
           exactGameBackupsRoot(profileA, letsGoEevee->id));
    assert(exactGameBackupsRoot(profileA, fireRedSwitch->id) !=
           exactGameBackupsRoot(profileA, leafGreenSwitch->id));
    assert(exactGameBackupsRoot(profileA, fireRedSwitch->id) !=
           exactGameBackupsRoot(profileA, fireRedGba->id));

    // Working/history folders live below profile + exact-game identity.
    assert(workspaceBackupPath(profileA, sword->id, "Working") ==
           swordA + "/Working");
    assert(workspaceBackupPath(profileA, sword->id, "20260923_041500") ==
           swordA + "/20260923_041500");

    // Invalid identity never falls back to a shared writable directory.
    assert(profileBackupsRoot(invalidProfile).empty());
    assert(exactGameBackupsRoot(invalidProfile, sword->id).empty());
    assert(exactGameBackupsRoot(profileA, "../sword").empty());
    assert(exactGameBackupsRoot(profileA, "sword/switch").empty());
    assert(workspaceBackupPath(profileA, sword->id, "../Working").empty());
    assert(workspaceBackupPath(profileA, sword->id, "/tmp").empty());

    // The historical title-based layout is represented explicitly as legacy/unscoped.
    assert(legacyUnscopedGameBackupsRoot("Pokemon Sword") ==
           backupsRoot() + "/Pokemon_Sword");

    // Integration contracts: title ID -> exact release identity, never display title -> writable path.
    const std::string utilities = read("src/Utils/FileUtilities.cpp");
    const auto backupBegin = utilities.find("std::string backupSaveData(");
    const auto backupEnd = utilities.find("static bool backupHasFile", backupBegin);
    assert(backupBegin != std::string::npos);
    assert(backupEnd != std::string::npos && backupEnd > backupBegin);
    const std::string backupBody = utilities.substr(backupBegin, backupEnd - backupBegin);
    assert(backupBody.find("PokeVault::Games::findSwitchGame(titleId)") != std::string::npos);
    assert(backupBody.find("exactGameBackupsRoot(userUid, identity->id)") != std::string::npos);
    assert(backupBody.find("workspaceBackupPath(userUid, identity->id, folderName)") != std::string::npos);
    assert(backupBody.find("sanitizeComponent(titleName)") == std::string::npos);

    const std::string screen = read("src/UI/BackupSelectionScreen.cpp");
    assert(screen.find("exactGameBackupsRoot(userUid, identity->id)") != std::string::npos);
    assert(screen.find("legacyUnscopedGameBackupsRoot(titleName)") != std::string::npos);
    assert(screen.find("listBackupDirectories(legacyGameDirectory.c_str(), true)") != std::string::npos);
    assert(screen.find("LEGACY UNSCOPED / OWNERSHIP UNKNOWN") != std::string::npos);
    assert(screen.find("if (chosen.legacyUnscoped)") != std::string::npos);
    // Legacy folder names came from readdir under the already-owned title directory. Preserve
    // historical custom names verbatim rather than sanitizing them into a different/nonexistent path.
    assert(screen.find("legacyGameDirectory + \"/\" + name") != std::string::npos);
    assert(screen.find("child(legacyGameDirectory, name)") == std::string::npos);

    const std::string ui = read("src/UI/UI.cpp");
    assert(ui.find("BackupSelectionScreen backupScreen(userUid, titleId, titleName)") !=
           std::string::npos);

    const std::string trainerView = read("src/UI/TrainerViewScreenBase.inc");
    const auto namedBegin = trainerView.find("TrainerViewScreen::createNamedBackupDir");
    const auto namedEnd = trainerView.find("std::vector<int> TrainerViewScreen::visibleItemIndices", namedBegin);
    assert(namedBegin != std::string::npos);
    assert(namedEnd != std::string::npos && namedEnd > namedBegin);
    const std::string namedBody = trainerView.substr(namedBegin, namedEnd - namedBegin);
    assert(namedBody.find("PokeVault::Games::findSwitchGame(titleId)") != std::string::npos);
    assert(namedBody.find("exactGameBackupsRoot(userUid, identity->id)") != std::string::npos);
    assert(namedBody.find("BASE_SAVE_DIRECTORY + \"/\" + titleName") == std::string::npos);
    assert(namedBody.find("backupDir.rfind(gameDir + \"/\", 0)") != std::string::npos);

    const std::string fileHeader = read("include/Utils/FileUtilities.h");
    assert(fileHeader.find(
        "listBackupDirectories(const char* gameDirectory, bool includeWorking = false)") !=
        std::string::npos);

    std::cout << "Profile/exact-game backup namespace contract: PASS\n";
}
