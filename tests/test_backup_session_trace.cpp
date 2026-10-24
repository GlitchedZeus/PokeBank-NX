#include "test_backup_session_trace.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Integration/Gen1/Gen1ExportTransaction.h"
#include "Integration/Gen2/Gen2ExportTransaction.h"
#include "Integration/Gen3/Gen3ExportTransaction.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Safety/SourceMutationPolicy.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

bool verifyBackupSessionTrace(const std::vector<uint8_t>& bytes,const std::string& game,std::string& error) {
    namespace fs=std::filesystem;
    namespace S=PokeVault::Source::ImmutableSnapshot;
    namespace L=PokeVault::Legacy;
    namespace G1=PokeVault::Integration::Gen1;namespace G2=PokeVault::Integration::Gen2;
    namespace G3=PokeVault::Integration::Gen3;namespace G4=PokeVault::Integration::Gen4;
    namespace G5=PokeVault::Integration::Gen5;
    char pattern[]="/tmp/pokebank-archived-session-XXXXXX";
    const char* made=::mkdtemp(pattern);
    if(!made){error="Cannot create backup fixture directory";return false;}
    const fs::path root=made;
    const auto need=[&](bool ok,const std::string& context) {
        if(!ok)throw std::runtime_error(context+" / "+error);
    };
    try {
        const auto history=(root/"history").string();fs::create_directories(history);
        const auto path=history+"/archived.sav";
        {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());need(f.good(),"Write synthetic backup");}
        {std::ofstream f(root/"live.sav");f<<"a different invalid live source must never be substituted";}
        S::File snapshot;need(S::read(path,history,{},snapshot,error),"Read selected historical copy");
        auto trainer=L::openBackupTrainer(snapshot,game,error);need(bool(trainer),"Open selected historical copy as "+game);
        const bool twoRegions=game=="gold_gbc" || game=="silver_gbc" || game=="crystal_gbc" ||
            game=="heartgold_nds" || game=="soulsilver_nds";
        need(trainer->nativeBadges.total==(twoRegions?16:8),"Trainer supports all native badge slots");
        const uint32_t money=trainer->money==123456?123457:123456;
        const auto exports=(root/"exports").string();
        std::string directory;
        if(auto* t=dynamic_cast<L::RBYReadOnlyTrainer*>(trainer.get())) {
            need(t->stagedTrainer() && t->stagedPokemon(),"Gen I staged workspace");
            need(t->stagedTrainer()->stageMoney(money,error),"Gen I stage from backup");
            G1::Gen1ExportRequest request;request.rootDirectory=exports;request.directoryStem="edited-backup";
            request.sourcePath=path;request.timestamp="20261024_120000";
            const auto result=G1::publishVerifiedGen1Export(*t->stagedPokemon(),request,nullptr,t->stagedTrainer());
            error=result.error;need(result.success,"Gen I separate verified export");directory=result.directory;
            t->stagedTrainer()->discard();
        } else if(auto* t=dynamic_cast<L::GSCReadOnlyTrainer*>(trainer.get())) {
            need(t->stagedEditor()!=nullptr,"Gen II staged workspace");
            need(t->stagedEditor()->stageMoney(money,error),"Gen II stage from backup");
            G2::StagedEditorExportRequest request;request.rootDirectory=exports;request.directoryStem="edited-backup";
            request.sourcePath=path;request.gameId=game;request.timestamp="20261024_120000";
            const auto result=G2::publishVerifiedStagedEditorExport(*t->stagedEditor(),request);
            error=result.error;need(result.success,"Gen II separate verified export");directory=result.directory;
            t->stagedEditor()->discard();
        } else if(auto* t=dynamic_cast<L::FRLGReadOnlyTrainer*>(trainer.get())) {
            need(t->stagedTrainer() && t->stagedPokemon(),"Gen III staged workspace");
            need(t->stagedTrainer()->stageMoney(money,error),"Gen III stage from backup");
            G3::Gen3ExportRequest request;request.rootDirectory=exports;request.directoryStem="edited-backup";
            request.sourcePath=path;request.timestamp="20261024_120000";
            const auto result=G3::publishVerifiedGen3Export(*t->stagedPokemon(),request,nullptr,t->stagedTrainer());
            error=result.error;need(result.success,"Gen III separate verified export");directory=result.directory;
            t->stagedTrainer()->discard();
        } else if(auto* t=dynamic_cast<L::Gen4ReadOnlyTrainer*>(trainer.get())) {
            need(t->stagedPokemon()!=nullptr,"Gen IV staged workspace");
            need(t->stagedPokemon()->stageTrainerMoney(money,&error),"Gen IV stage from backup");
            G4::Gen4ExportRequest request;request.rootDirectory=exports;request.directoryStem="edited-backup";
            request.sourcePath=path;request.timestamp="20261024_120000";
            const auto result=G4::publishVerifiedGen4Export(*t->stagedPokemon(),request);
            error=result.error;need(result.success,"Gen IV separate verified export");directory=result.directory;
            t->stagedPokemon()->discard();
        } else if(auto* t=dynamic_cast<L::Gen5ReadOnlyTrainer*>(trainer.get())) {
            auto& workspace=t->stagedPokemon();
            auto* trainerEdit=workspace.stagedTrainer(&error);need(trainerEdit!=nullptr,"Gen V staged workspace");
            need(trainerEdit->setMoney(money,&error),"Gen V stage from backup");
            G5::WorkingCopyExportRequest request;request.rootDirectory=exports;
            request.profileIdentity="0123456789abcdef0123456789abcdef";request.timestamp="20261024_120000";
            request.sourcePath=path;request.observedSourceBytes=bytes;
            const auto result=G5::publishGen5WorkingCopy(workspace,request);
            error=result.error;need(result.success,"Gen V separate verified export");directory=result.directory;
            workspace.discardAll();
        } else need(false,"Unsupported backup Trainer bridge");
        need(!trainer->hasStagedChanges(),"Discard restores the archived baseline");
        const auto edited=fs::exists(fs::path(directory)/"edited.srm")?fs::path(directory)/"edited.srm":fs::path(directory)/"edited.sav";
        S::File output;need(S::read(edited.string(),exports,{},output,error),"Read verified separate output");
        const auto reopened=L::openBackupTrainer(output,game,error);
        need(reopened && reopened->money==money,"Reparse exported native money");
        need(S::unchanged(snapshot,error),"Original backup bytes/mtime/inode unchanged");
        const auto restored=L::openBackupTrainer(snapshot,game,error);
        need(restored && restored->money==trainer->money,"Reopening backup keeps original money");
        need(!L::openBackupTrainer(snapshot,game=="black_nds"?"red_gb":"black_nds",error),"Cross-generation mismatch refuses");
        need(!PokeVault::Safety::canPerform(PokeVault::Safety::SourceKind::ExternalLegacy,
            PokeVault::Safety::SourceMutation::SaveChanges),"Historical source never grants direct SaveChanges");
        fs::remove_all(root);error.clear();return true;
    } catch(const std::exception& e) {
        error=e.what();fs::remove_all(root);return false;
    }
}
