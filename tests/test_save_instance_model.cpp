#include "Source/SaveInstance.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using namespace PokeVault::Source;

    std::vector<SaveInstance> instances;

    SaveInstance retro;
    retro.gameId = "crystal_gbc";
    retro.generation = 2;
    retro.platformLabel = "GBC";
    retro.providerLabel = "RetroArch";
    retro.providerId = providerIdFor(retro.providerLabel);
    retro.label = "Pokemon Crystal.srm";
    retro.sourcePath = "/retroarch/cores/savefiles/Pokemon Crystal.srm";
    retro.location = retro.sourcePath;
    retro.normalizedPath = retro.sourcePath;
    retro.sourceIdentity = "retro-crystal";
    retro.physicalIdentity = "dev1:inode10";
    retro.containerType = "Battery save";
    retro.modifiedTime = 100;
    retro.trainerName = "WILL";
    retro.partyCount = 3;
    retro.validation = ValidationStatus::Ready;
    retro.access = AccessMode::ReadOnly;
    assert(appendDeduplicated(instances, retro));

    // The same physical file discovered through an overlapping provider/root stays one row.
    SaveInstance alias = retro;
    alias.providerLabel = "Tico";
    alias.providerId = providerIdFor(alias.providerLabel);
    alias.sourcePath = "/tico/saves/gbc/Pokemon Crystal.sav";
    alias.location = alias.sourcePath;
    alias.normalizedPath = alias.sourcePath;
    alias.sourceIdentity = "tico-alias";
    assert(!appendDeduplicated(instances, alias));
    assert(instances.size() == 1);
    assert(instances.front().providerLabel == "RetroArch");

    SaveInstance mgba = retro;
    mgba.providerLabel = "mGBA";
    mgba.providerId = providerIdFor(mgba.providerLabel);
    mgba.label = "Crystal.sav";
    mgba.sourcePath = "/mGBA/battery/Crystal.sav";
    mgba.location = mgba.sourcePath;
    mgba.normalizedPath = mgba.sourcePath;
    mgba.sourceIdentity = "mgba-crystal";
    mgba.physicalIdentity = "dev1:inode11";
    mgba.modifiedTime = 200;
    mgba.claimedProfile = "profile-a";
    assert(appendDeduplicated(instances, mgba));

    SaveInstance manual = retro;
    manual.kind = SaveInstanceKind::ManualImport;
    manual.providerLabel = "Manual";
    manual.providerId = providerIdFor(manual.providerLabel);
    manual.label = "MyOldCrystal.sav";
    manual.sourcePath = "/archive/old-saves/MyOldCrystal.sav";
    manual.location = manual.sourcePath;
    manual.normalizedPath = manual.sourcePath;
    manual.sourceIdentity = "manual-crystal";
    manual.physicalIdentity = "dev2:inode50";
    manual.modifiedTime = 300;
    manual.rememberedSource = true;
    assert(appendDeduplicated(instances, manual));

    assert(instances.size() == 3);
    assert(instances[0].gameId == "crystal_gbc");
    assert(instances[1].gameId == "crystal_gbc");
    assert(instances[2].gameId == "crystal_gbc");
    assert(instances[0].providerId == "retroarch");
    assert(instances[1].providerId == "mgba");
    assert(instances[2].providerId == "manual");

    sortNewestFirst(instances);
    assert(instances[0].providerLabel == "Manual");
    assert(instances[0].mostRecentlyModified);
    assert(!instances[1].mostRecentlyModified);
    assert(!instances[2].mostRecentlyModified);
    assert(instances[0].rememberedSource);
    assert(instances[0].sourcePath == "/archive/old-saves/MyOldCrystal.sav");

    const auto& claimed = *std::find_if(instances.begin(), instances.end(),
        [](const auto& instance) { return instance.providerLabel == "mGBA"; });
    assert(visibleToProfile(claimed, "profile-a"));
    assert(!visibleToProfile(claimed, "profile-b"));
    const auto& unassigned = *std::find_if(instances.begin(), instances.end(),
        [](const auto& instance) { return instance.providerLabel == "RetroArch"; });
    assert(visibleToProfile(unassigned, "profile-a"));
    assert(visibleToProfile(unassigned, "profile-b"));

    for (const auto& instance : instances) {
        assert(instance.ready());
        assert(instance.readOnly());
        assert(instance.access != AccessMode::StagedWorkspace);
    }

    std::cout << "Provider-neutral SaveInstance model: dedupe/order/provider/profile/manual/read-only PASS\n";
}
