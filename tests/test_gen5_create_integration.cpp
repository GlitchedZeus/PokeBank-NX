#include "Integration/Gen5/Gen5NativeCreate.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"

#include <cstdlib>
#include <iostream>

namespace G = PokeVault::Integration::Gen5;
namespace C = G::Crypto;
namespace L = G::LayoutInfo;
static size_t checks = 0;
#define CHECK(...) do { ++checks; if (!(__VA_ARGS__)) { \
    std::cerr << "FAIL line " << __LINE__ << ": " << #__VA_ARGS__ << '\n'; std::exit(1); } } while (false)

// A generated PK5 embedded in an entirely synthetic input SAV5 is a test
// fixture, NOT new-slot staging authority. Production Create wiring is gated.
static void exercise(uint32_t version, bool backup, uint32_t species) {
    const bool b2w2 = version >= 22;
    const auto blocks = b2w2 ? std::span<const L::SaveBlock>(L::Black2White2)
                            : std::span<const L::SaveBlock>(L::BlackWhite);
    const size_t copySize = b2w2 ? L::Black2White2CopySize : L::BlackWhiteCopySize;
    const size_t base = backup ? copySize : 0;
    constexpr std::array<std::string_view,4> ids{"white_nds","black_nds","white2_nds","black2_nds"};
    const auto id = ids[version - 20];
    const auto selection = backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
    G::CreateRequest5 r;
    r.game = static_cast<G::CreateGame5>(version); r.species=species; r.level=70;
    r.nature=3; r.gender=0; r.abilitySlot=0; r.pid=0x123400ff; r.friendship=70;
    r.trainer=G::CreateTrainer5{100,200,0,2,"TEST"}; r.nickname="CREATED";
    r.encounter={G::CreateHistory5::CallerSuppliedNative,1,1,4,2024,1,1};
    r.moves[0]={1,0,std::nullopt};
    const auto created=G::createNative5(r);
    CHECK(created);
    std::vector<uint8_t> input(L::FullSaveSize,0);
    input[base+0x19400+0x1f]=static_cast<uint8_t>(version);
    std::copy(created.encrypted.begin(),created.encrypted.end(),input.begin()+static_cast<std::ptrdiff_t>(base+L::BoxOffset));
    auto partition=std::span<uint8_t>(input).subspan(base,copySize);
    for (const auto& block:blocks) {
        const auto crc=Utils::crc16ccitt(partition.data()+block.offset,block.length);
        C::write16(partition,block.checksumOffset,crc);
        C::write16(partition,block.mirrorOffset,crc);
    }
    const auto immutable=input;
    std::string error;
    const auto save=G::Gen5ReadOnlySave::parse(input,id,&error,selection);
    CHECK(save && error.empty());
    CHECK(save->boxPokemon(0,0)->species()==species);
    G::Gen5StagedPokemonWorkspace workspace(*save);
    G::Gen5SharedPokemonSession session;
    using Region=G::Gen5StagedPokemonWorkspace::Region;
    using Field=G::StagedPokemon5Record::Field;
    using Mode=G::Gen5SharedPokemonSession::Mode;
    CHECK(session.begin(workspace,{Region::Box,0,0},Mode::Edit,error));
    CHECK(session.stage(Field::Nature,0,24,error));
    CHECK(session.stageName(Field::NicknameText,"REVIEWED",error));
    CHECK(session.keep(workspace,error));
    CHECK(workspace.pendingReview().size()==1);
    CHECK(G::verifyStagedReview(workspace.pendingReview().front(),&error));
    const auto candidate=G::buildStagedSaveCandidate(workspace,&error);
    CHECK(candidate && candidate->changedRecords==1);
    const auto reparsed=G::Gen5ReadOnlySave::parse(candidate->bytes,id,&error,selection);
    CHECK(reparsed && reparsed->boxPokemon(0,0)->nature()==24);
    CHECK(reparsed->boxPokemon(0,0)->experience()==save->boxPokemon(0,0)->experience());
    // Exact whitelist: only this boxed record and owning/terminal CRCs differ.
    std::vector<bool> allowed(input.size(),false);
    for (size_t i=0;i<136;++i) allowed[base+L::BoxOffset+i]=true;
    for (const auto& block:blocks) {
        if ((L::BoxOffset>=block.offset && L::BoxOffset+136<=block.offset+block.length) ||
            &block==&blocks.back()) {
            allowed[base+block.checksumOffset]=allowed[base+block.checksumOffset+1]=true;
            allowed[base+block.mirrorOffset]=allowed[base+block.mirrorOffset+1]=true;
        }
    }
    for (size_t i=0;i<input.size();++i) if(!allowed[i]) CHECK(candidate->bytes[i]==immutable[i]);
    CHECK(input==immutable);
    CHECK(std::equal(immutable.begin(),immutable.end(),workspace.sourceSave().sourceBytes().begin()));
    // Tamper with an unreviewable reserved byte; refresh checksum to ensure the
    // review rejects unsupported semantics, not just invalid crypto.
    auto forged=workspace.pendingReview().front();
    auto d=C::decrypt(forged.after); d[0x43]=1; forged.after=C::encryptCandidate(d);
    CHECK(!G::verifyStagedReview(forged,&error));
    workspace.discardAll();
    CHECK(!workspace.hasChanges());
    CHECK(workspace.cloneToEmptyBox({Region::Box,0,0},0,1,&error));
    const auto clone=G::buildStagedSaveCandidate(workspace,&error);
    CHECK(clone);
    const auto cloneSave=G::Gen5ReadOnlySave::parse(clone->bytes,id,&error,selection);
    CHECK(cloneSave && cloneSave->boxPokemon(0,1)->species()==species);
    CHECK(workspace.undoBoxReplacement(0,1));
    CHECK(workspace.removeBox(0,0,&error));
    const auto removed=G::buildStagedSaveCandidate(workspace,&error);
    CHECK(removed);
    CHECK(G::Gen5ReadOnlySave::parse(removed->bytes,id,&error,selection)->boxPokemon(0,0)->empty());
    CHECK(input==immutable);
    // Create must not masquerade as FieldEdit or CloneBox for an empty slot.
    auto unauthorized=forged;
    unauthorized.location={Region::Box,0,1};
    unauthorized.before.assign(136,0); unauthorized.after=created.encrypted;
    CHECK(!G::verifyStagedReview(unauthorized,&error));
    // Coordinator progression patch is required before exposing Create to the
    // existing editor: native Erratic level 70 must not be displayed as 69.
    if(species==290) CHECK(G::levelForExperience5(1,save->boxPokemon(0,0)->experience())==70);
}
int main() {
    for(uint32_t version=20;version<=23;++version)
        for(bool backup:{false,true}) for(uint32_t species:{25,290}) exercise(version,backup,species);
    std::cout<<"Gen V Create integration: 16 save scenarios, "<<checks<<" checks passed\n";
}
