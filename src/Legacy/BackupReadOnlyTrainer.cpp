#include "Legacy/BackupReadOnlyTrainer.h"
#include "Games/GameIdentity.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include <algorithm>

namespace PokeVault::Legacy {
std::unique_ptr<Trainer::Trainer> openBackupTrainer(
    const Source::ImmutableSnapshot::File& file,const std::string& gameId,std::string& error,
    Integration::Gen5::SaveCopySelection copy) {
    namespace G1=Integration::Gen1;namespace G2=Integration::Gen2;
    namespace G3=Integration::Gen3;namespace G4=Integration::Gen4;namespace G5=Integration::Gen5;
    const auto* identity=Games::findGame(gameId);
    if(!identity || identity->dataGeneration<1 || identity->dataGeneration>5 ||
       identity->platform==Games::Platform::NintendoSwitch || file.bytes.empty()) {
        error="Backup requires an exact supported Gen I-V emulator game";return {};
    }
    if(!Source::ImmutableSnapshot::unchanged(file,error))return {};
    std::unique_ptr<Trainer::Trainer> trainer;
    if(gameId=="red_gb" || gameId=="blue_gb" || gameId=="yellow_gb") {
        const auto parsed=G1::parse(file.bytes,gameId=="red_gb"?G1::SourceGame::Red:
            gameId=="blue_gb"?G1::SourceGame::Blue:G1::SourceGame::Yellow);
        if(!parsed.save){error=parsed.detail;return {};}
        trainer=RBYReadOnlyTrainer::create(*parsed.save,error);
    } else if(gameId=="gold_gbc" || gameId=="silver_gbc" || gameId=="crystal_gbc") {
        const auto parsed=G2::parse(file.bytes,gameId=="gold_gbc"?G2::SourceGame::Gold:
            gameId=="silver_gbc"?G2::SourceGame::Silver:G2::SourceGame::Crystal);
        if(!parsed.save){error=parsed.detail;return {};}
        trainer=GSCReadOnlyTrainer::create(*parsed.save,error);
    } else if(identity->platform==Games::Platform::GameBoyAdvance) {
        const auto game=gameId=="ruby_gba"?G3::SourceGame::RubyGBA:
            gameId=="sapphire_gba"?G3::SourceGame::SapphireGBA:
            gameId=="emerald_gba"?G3::SourceGame::EmeraldGBA:
            gameId=="firered_gba"?G3::SourceGame::FireRedGBA:G3::SourceGame::LeafGreenGBA;
        const auto parsed=G3::parse(file.bytes,game);
        if(!parsed.save){error=parsed.detail;return {};}
        trainer=FRLGReadOnlyTrainer::create(*parsed.save,error);
    } else if(identity->dataGeneration==4) {
        const auto payload=G4::readSourcePayloadReadOnly(file.path);
        if(!payload.ready() || payload.bytes.size()>file.bytes.size() ||
           !std::equal(payload.bytes.begin(),payload.bytes.end(),file.bytes.begin())) {
            error="Backup container failed validation or changed during read";return {};
        }
        const auto layout=gameId=="platinum_nds"?G4::Layout::Platinum:
            (gameId=="heartgold_nds" || gameId=="soulsilver_nds")?G4::Layout::HeartGoldSoulSilver:
            G4::Layout::DiamondPearl;
        const auto parsed=G4::Gen4ReadOnlySave::parse(payload.bytes,layout,gameId,&error);
        if(!parsed || parsed->assignmentStatus()!=G4::AssignmentStatus::Match)return {};
        trainer=Gen4ReadOnlyTrainer::create(*parsed,gameId,error);
    } else if(identity->dataGeneration==5) {
        std::vector<uint8_t> raw;
        if(!G5::readNormalizedSourceReadOnly(file.path,raw,error) || raw.size()>file.bytes.size() ||
           !std::equal(raw.begin(),raw.end(),file.bytes.begin())) {
            if(error.empty())error="Backup container changed during validation";
            return {};
        }
        const auto parsed=G5::Gen5ReadOnlySave::parse(raw,gameId,&error,copy);
        if(!parsed || parsed->exactGameId()!=gameId)return {};
        trainer=Gen5ReadOnlyTrainer::create(*parsed,gameId,error);
    }
    if(!trainer){if(error.empty())error="Backup failed strict native validation";return {};}
    if(!Source::ImmutableSnapshot::unchanged(file,error))return {};
    error.clear();return trainer;
}
}
