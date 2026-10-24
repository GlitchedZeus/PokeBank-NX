#pragma once
#include "UI/SourceBackupClassification.h"

namespace PokeBank::UIModel::SourceBackups {
inline std::vector<std::string> historicalSources(const std::string& exportsRoot,
    const std::string& manualRoot,const std::string& game) {
    std::vector<std::string> paths;
    if(!detail::plainName(game))return paths;
    for(const auto& name:detail::children(manualRoot+"/"+game)) {
        if(!detail::plainName(name))continue;
        const auto manifest=manualRoot+"/"+game+"/"+name+"/"+detail::kBackupManifestName;
        if(!PokeVault::Source::ImmutableSnapshot::safePath(manifest,manualRoot))continue;
        const auto m=detail::readBackupManifest(manifest);
        if(m.ok && m.gameId==game)paths.push_back(m.sourcePath);
    }
    for(int gen=1;gen<=5;++gen)for(const auto& name:detail::children(exportsRoot+"/gen"+std::to_string(gen))) {
        if(!detail::plainName(name))continue;
        const auto manifest=exportsRoot+"/gen"+std::to_string(gen)+"/"+name+"/"+detail::kManifestName;
        if(!PokeVault::Source::ImmutableSnapshot::safePath(manifest,exportsRoot))continue;
        const auto m=detail::readManifest(manifest);
        if(m.ok && m.gameId==game)paths.push_back(m.sourcePath);
    }
    std::sort(paths.begin(),paths.end());paths.erase(std::unique(paths.begin(),paths.end()),paths.end());
    return paths;
}
// Separate from canDelete: a last surviving backup is readable, never deletable.
// Revalidate the exact manifest and file before handing any bytes to a parser.
inline bool openSnapshot(const Listing& listing,size_t index,const std::string& game,
    PokeVault::Source::ImmutableSnapshot::File& file,std::string& error) {
    using namespace PokeVault::Source;
    if(index>=listing.entries.size()){error="Backup selection is stale";return false;}
    const auto& e=listing.entries[index];
    if(!e.exists || e.gameId!=game || e.physicalStamp.empty() ||
       e.kind==Kind::OriginalSource || e.kind==Kind::StagedWorkingCopy) {
        error="Select an existing backup for this exact game";return false;
    }
    const bool manual=e.kind==Kind::ManualBackup;
    const auto root=manual?listing.manualRoot:listing.exportsRoot;
    const std::string manifest=e.recordDir+"/"+(manual?detail::kBackupManifestName:detail::kManifestName);
    ImmutableSnapshot::File metadata;
    if(!ImmutableSnapshot::read(manifest,root,{},metadata,error,64*1024))return false;
    std::string expected;
    if(manual) {
        const auto m=detail::readBackupManifest(manifest);
        if(!m.ok || m.gameId!=game || m.sourcePath!=listing.sourcePath || m.timestamp!=e.timestamp) {
            error="Backup manifest no longer matches this source and game";return false;
        }
        const auto prefix=root+"/"+game+"/";
        if(!e.recordDir.starts_with(prefix) || !detail::plainName(e.recordDir.substr(prefix.size()))) {
            error="Backup is outside its exact-game history";return false;
        }
        expected=e.recordDir+"/"+m.backupFile;
    } else {
        const auto m=detail::readManifest(manifest);
        if(!m.ok || m.gameId!=game || m.sourcePath!=listing.sourcePath || m.timestamp!=e.timestamp) {
            error="Export manifest no longer matches this source and game";return false;
        }
        bool scoped=false;
        for(int gen=1;gen<=5;++gen) {
            const auto prefix=root+"/gen"+std::to_string(gen)+"/";
            scoped|=e.recordDir.starts_with(prefix) && detail::plainName(e.recordDir.substr(prefix.size()));
        }
        if(!scoped){error="Backup is outside an application export record";return false;}
        expected=e.recordDir+"/"+(e.kind==Kind::AppBackup?m.originalBackup:m.editedSave);
    }
    if(expected!=e.path){error="Backup file no longer matches its selected record";return false;}
    if(!ImmutableSnapshot::read(e.path,root,e.physicalStamp,file,error))return false;
    if(!ImmutableSnapshot::unchanged(metadata,error))return false;
    return true;
}
}
