#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5WorkingCopyExport.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;

static void write32(std::span<uint8_t> s, size_t o, uint32_t v) {
    C::write16(s,o,static_cast<uint16_t>(v));
    C::write16(s,o+2,static_cast<uint16_t>(v>>16));
}
static void stamp(std::span<uint8_t> copy,
                  std::span<const L::SaveBlock> blocks) {
    for (const auto& b : blocks) {
        assert(b.offset+b.length<=copy.size());
        const uint16_t crc=Utils::crc16ccitt(copy.data()+b.offset,b.length);
        C::write16(copy,b.checksumOffset,crc);
        C::write16(copy,b.mirrorOffset,crc);
    }
}
static std::vector<uint8_t> fixture(G::SaveFamily family,uint8_t version,
                                     uint32_t pid,bool backup) {
    const size_t copySize=family==G::SaveFamily::BlackWhite
        ? L::BlackWhiteCopySize:L::Black2White2CopySize;
    const size_t base=backup?copySize:0;
    std::vector<uint8_t> sav(L::FullSaveSize,0);
    sav[base+0x19400+0x1F]=version;
    sav[base+L::PartyOffset+4]=1;
    std::vector<uint8_t> party(C::PartySize,0);
    write32(party,0,pid);
    C::write16(party,8,25);
    C::write16(party,0x0C,4321);
    party[0x8C]=54;
    party[0x42]=1;       // Gen V hidden-ability flag is untouched.
    party[0x82]=0x21;    // Native Pokerus strain/days are untouched.
    party[0x84]=0x80|35; // Female original trainer + met level.
    const auto encrypted=C::encryptCandidate(party);
    assert(encrypted.size()==C::PartySize);
    std::copy(encrypted.begin(),encrypted.end(),
        sav.begin()+static_cast<std::ptrdiff_t>(base+L::PartyOffset+8));
    std::vector<uint8_t> box(C::StoredSize,0);
    write32(box,0,pid ^ 0x12340000u);
    C::write16(box,8,133);
    const auto boxed=C::encryptCandidate(box);
    std::copy(boxed.begin(),boxed.end(),
        sav.begin()+static_cast<std::ptrdiff_t>(base+L::BoxOffset));
    const auto blocks=family==G::SaveFamily::BlackWhite
        ? std::span<const L::SaveBlock>(L::BlackWhite)
        : std::span<const L::SaveBlock>(L::Black2White2);
    stamp(std::span<uint8_t>(sav).subspan(base,copySize),blocks);
    return sav;
}

static void exercise(G::SaveFamily family,std::string_view game,uint8_t version,
                     uint32_t pid,bool backup) {
    const auto input=fixture(family,version,pid,backup);
    std::string error;
    const auto selected=backup?G::SaveCopySelection::Backup:
                               G::SaveCopySelection::Primary;
    const auto parsed=G::Gen5ReadOnlySave::parse(input,game,&error,selected);
    assert(parsed && error.empty());
    assert(parsed->selectedBackupPartition()==backup);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    assert(!G::buildStagedSaveCandidate(ws,&error) && !error.empty());

    using Field=G::StagedPokemon5Record::Field;
    assert(ws.stageParty(0,Field::Nature,0,22,&error));
    assert(ws.stageParty(0,Field::Friendship,0,128,&error));
    assert(ws.stageParty(0,Field::Ball,0,25,&error));
    assert(ws.stageParty(0,Field::MetLevel,0,72,&error));
    assert(ws.stageParty(0,Field::TrainerId,0,65535,&error));
    assert(ws.stageParty(0,Field::SecretId,0,54321,&error));
    assert(ws.stageParty(0,Field::OriginalTrainerGender,0,0,&error));
    assert(ws.stageParty(0,Field::FatefulEncounter,0,1,&error));
    assert(ws.stageParty(0,Field::MetLocation,0,40001,&error));
    assert(ws.stageParty(0,Field::Pokerus,0,1,&error));
    assert(ws.stageParty(0,Field::Move,0,559,&error));
    assert(ws.stageParty(0,Field::MovePPUps,0,3,&error));
    assert(ws.stageParty(0,Field::MovePP,0,4,&error));
    assert(ws.stagePartyName(0,true,"PARTY",&error));
    assert(ws.stageParty(0,Field::Species,0,26,&error));
    assert(ws.stageBoxName(0,0,false,"NXT",&error));
    assert(ws.stageBox(0,0,Field::Level,0,40,&error));
    assert(ws.stageBox(0,0,Field::Experience,0,75000,&error));
    assert(ws.stageBox(0,0,Field::HeldItem,0,638,&error));
    assert(ws.stageBox(0,0,Field::AbilitySlot,0,2,&error));
    assert(ws.stageBox(0,0,Field::Gender,0,1,&error));
    assert(!ws.stageBox(0,0,Field::Form,0,1,&error)); // Eevee form 0 only
    assert(ws.stageBox(0,0,Field::Move,1,85,&error));
    assert(ws.stageBox(0,0,Field::EV,0,252,&error));
    assert(ws.stageBox(0,0,Field::IV,4,31,&error));
    assert(ws.changedRecordCount()==2);
    assert(!ws.stageParty(6,Field::Nature,0,22,&error));
    assert(!ws.stageBox(24,0,Field::EV,0,252,&error));
    const auto out=G::buildStagedSaveCandidate(ws,&error);
    assert(out && error.empty());
    assert(out->bytes.size()==input.size() && out->bytes!=input);
    assert(out->changedRecords==2 && out->exactGameId==game);
    const size_t copySize=family==G::SaveFamily::BlackWhite
        ? L::BlackWhiteCopySize:L::Black2White2CopySize;
    const size_t base=backup?copySize:0;
    assert(out->selectedCopyOffset==base);
    // Reopened source must match the *entire* observed battery image.
    // This includes unused regions and the other untouched save copy.
    const auto checkedExport=G::buildStagedSaveCandidateForObservedSource(
        ws,std::span<const uint8_t>(input),&error);
    assert(checkedExport && error.empty());
    assert(checkedExport->bytes==out->bytes);
    auto stale=input;
    stale[0x70000]^=1; // Outside retail selected copy, still a source change.
    assert(!G::buildStagedSaveCandidateForObservedSource(ws,stale,&error));
    assert(error.find("changed since staging")!=std::string::npos);
    stale=input;
    stale[base+L::PartyOffset+10]^=1; // Inside selected original PK5.
    assert(!G::buildStagedSaveCandidateForObservedSource(ws,stale,&error));
    assert(!G::buildStagedSaveCandidateForObservedSource(
        ws,std::span<const uint8_t>(input.data(),input.size()-1),&error));
    assert(std::equal(parsed->sourceBytes().begin(),
                      parsed->sourceBytes().end(),input.begin()));

    // Independently calculate every allowed output-byte region. Even a
    // checksum-correct candidate must never change inventories, trainer,
    // other boxes, copy-selection metadata or any byte in the other copy.
    std::vector<uint8_t> allowed(input.size(),0);
    const auto mark=[&](size_t offset,size_t size) {
        assert(offset+size<=allowed.size());
        std::fill_n(allowed.begin()+static_cast<std::ptrdiff_t>(offset),
                    size,static_cast<uint8_t>(1));
    };
    mark(base+L::BoxOffset,C::StoredSize);
    mark(base+L::PartyOffset+8,C::PartySize);
    const auto blocks=family==G::SaveFamily::BlackWhite
        ? std::span<const L::SaveBlock>(L::BlackWhite)
        : std::span<const L::SaveBlock>(L::Black2White2);
    for (const size_t recordOffset : {size_t{L::BoxOffset},
                                     size_t{L::PartyOffset+8}}) {
        const size_t recordSize=recordOffset==L::BoxOffset
            ? C::StoredSize:C::PartySize;
        bool found=false;
        for (size_t i=0;i+1<blocks.size();++i) {
            const auto& b=blocks[i];
            if (recordOffset>=b.offset &&
                recordOffset+recordSize<=b.offset+b.length) {
                assert(!found);
                found=true;
                mark(base+b.checksumOffset,2);
                mark(base+b.mirrorOffset,2);
            }
        }
        assert(found);
    }
    mark(base+blocks.back().checksumOffset,2);
    mark(base+blocks.back().mirrorOffset,2);
    for (size_t i=0;i<input.size();++i)
        if (!allowed[i])assert(out->bytes[i]==input[i]);

    const auto candidate=G::Gen5ReadOnlySave::parse(out->bytes,game,&error,selected);
    assert(candidate && error.empty());
    assert(candidate->exactGameId()==game);
    assert(candidate->partyPokemon(0)->nature()==22);
    assert(candidate->partyPokemon(0)->friendship()==128);
    assert(candidate->partyPokemon(0)->ball()==25);
    assert(candidate->partyPokemon(0)->metLevel()==72);
    assert(candidate->partyPokemon(0)->originalTrainerGender()==0);
    assert(candidate->partyPokemon(0)->metLevel()==72);
    assert(candidate->partyPokemon(0)->tid()==65535);
    assert(candidate->partyPokemon(0)->sid()==54321);
    assert(candidate->partyPokemon(0)->fatefulEncounter());
    assert(candidate->partyPokemon(0)->metLocation()==40001);
    assert(candidate->partyPokemon(0)->pokerusState()==0x20);
    assert(candidate->partyPokemon(0)->hiddenAbility());
    assert(candidate->partyPokemon(0)->moves()[0]==559);
    assert(candidate->partyPokemon(0)->ppUps()[0]==3);
    assert(candidate->partyPokemon(0)->pp()[0]==4);
    assert(candidate->boxPokemon(0,0)->heldItem()==638);
    assert(candidate->boxPokemon(0,0)->hiddenAbility());
    assert(candidate->boxPokemon(0,0)->ability()==107);
    assert(candidate->boxPokemon(0,0)->gender()==1);
    assert(candidate->boxPokemon(0,0)->form()==0);
    assert(candidate->boxPokemon(0,0)->moves()[1]==85);
    assert(candidate->boxPokemon(0,0)->ppUps()[1]==0);
    assert(candidate->boxPokemon(0,0)->pp()[1]==15);
    assert(candidate->partyPokemon(0)->partyLevel()==54);
    assert(candidate->boxPokemon(0,0)->evs()[0]==252);
    assert(candidate->boxPokemon(0,0)->ivs()[4]==31);
    assert(candidate->boxPokemon(0,1)->empty());
    assert(std::equal(parsed->sourceBytes().begin(),parsed->sourceBytes().end(),
                      input.begin()));
    // The output's game identity must remain exact; cross-title reuse fails.
    const auto wrong=version==20?"black_nds":"white_nds";
    assert(!G::Gen5ReadOnlySave::parse(out->bytes,wrong,&error,selected));
    auto damaged=out->bytes;
    damaged[base+L::PartyOffset+10]^=0x80;
    assert(!G::Gen5ReadOnlySave::parse(damaged,game,&error,selected));
    // Exercise real app-owned working-copy export on one native fixture
    // per BW/B2W2 family and selected-copy location. All writes must be
    // confined to a disposable host directory; the emulator source remains
    // intact. Fault injection exercises atomic publication/cleanup.
    if ((pid>>13)==0) {
        char pattern[]="/tmp/pokebank_gen5_working_copy_XXXXXX";
        const char* root=::mkdtemp(pattern);
        assert(root);
        const std::string exportRoot(root);
        const std::string sourcePath=exportRoot+"/original_source.sav";
        FILE* original=std::fopen(sourcePath.c_str(),"wb");
        assert(original);
        assert(std::fwrite(input.data(),1,input.size(),original)==input.size());
        assert(std::fclose(original)==0);
        G::WorkingCopyExportRequest req;
        req.rootDirectory=exportRoot;
        req.profileIdentity="00000000000000000000000000000001";
        req.timestamp="20261012_221500";
        req.sourcePath=sourcePath;
        req.applicationSha="host-synthetic-test";
        req.observedSourceBytes=input;
        assert(!G::publishGen5WorkingCopy(
            G::Gen5StagedPokemonWorkspace(*parsed),req).success);
        const auto exported=G::publishGen5WorkingCopy(ws,req);
        assert(exported.success && exported.error.empty());
        assert(exported.changedRecords==2);
        assert(exported.sourceSha256.size()==64 &&
               exported.editedSha256.size()==64 &&
               exported.sourceSha256!=exported.editedSha256);
        const auto verifyOnDisk=[&](const std::string& path,
                                     const std::vector<uint8_t>& expected) {
            FILE* file=std::fopen(path.c_str(),"rb");
            assert(file);
            std::vector<uint8_t> readback(expected.size(),0);
            assert(std::fread(readback.data(),1,readback.size(),file)==readback.size());
            assert(std::fgetc(file)==EOF);
            assert(std::fclose(file)==0);
            assert(readback==expected);
        };
        verifyOnDisk(exported.directory+"/original_backup.sav",input);
        verifyOnDisk(exported.directory+"/edited.sav",out->bytes);
        verifyOnDisk(sourcePath,input); // Physical source was never opened for write.
        {
            FILE* manifest=std::fopen(
                (exported.directory+"/EXPORT_MANIFEST.txt").c_str(),"rb");
            assert(manifest);
            char text[4096]{};
            const size_t n=std::fread(text,1,sizeof(text)-1,manifest);
            assert(std::fclose(manifest)==0 && n>0);
            const std::string manifestText(text,n);
            assert(manifestText.find("EMULATOR_SAVE_WRITE=DISABLED")!=std::string::npos);
            assert(manifestText.find("INJECT_SAVE=DISABLED")!=std::string::npos);
            assert(manifestText.find(exported.sourceSha256)!=std::string::npos);
            assert(manifestText.find(exported.editedSha256)!=std::string::npos);
        }
        auto second=G::publishGen5WorkingCopy(ws,req);
        assert(second.success && second.directory!=exported.directory);
        verifyOnDisk(exported.directory+"/original_backup.sav",input);
        const auto cleanupPublished=[&](const std::string& dir) {
            assert(std::remove((dir+"/original_backup.sav").c_str())==0);
            assert(std::remove((dir+"/edited.sav").c_str())==0);
            assert(std::remove((dir+"/EXPORT_MANIFEST.txt").c_str())==0);
            assert(::rmdir(dir.c_str())==0);
        };
        cleanupPublished(exported.directory);
        cleanupPublished(second.directory);
        const std::array<G::WorkingCopyFailurePoint,4> failures{
            G::WorkingCopyFailurePoint::AfterBackup,
            G::WorkingCopyFailurePoint::AfterEdited,
            G::WorkingCopyFailurePoint::AfterReadback,
            G::WorkingCopyFailurePoint::AfterManifest};
        for(size_t i=0;i<failures.size();++i) {
            req.timestamp="20261012_22150"+std::to_string(i);
            req.failurePoint=failures[i];
            const auto failed=G::publishGen5WorkingCopy(ws,req);
            assert(!failed.success && !failed.error.empty());
            const std::string path=exportRoot+"/account-"+req.profileIdentity+
                "/"+std::string(game)+"_"+req.timestamp;
            struct stat st{};
            assert(::lstat(path.c_str(),&st)!=0 && errno==ENOENT);
            const std::string temporary=exportRoot+"/account-"+req.profileIdentity+
                "/."+std::string(game)+"_"+req.timestamp+".pending";
            assert(::lstat(temporary.c_str(),&st)!=0 && errno==ENOENT);
        }
        {
            // QA-F3: a destination appearing just before publication is never
            // replaced (the racing empty directory survives untouched) and a
            // mid-publication failure leaves neither directory behind.
            req.timestamp="20261012_221510";
            req.failurePoint=G::WorkingCopyFailurePoint::DestinationAppears;
            const auto raced=G::publishGen5WorkingCopy(ws,req);
            assert(!raced.success && raced.error.find("existing export preserved")!=std::string::npos);
            const std::string base=exportRoot+"/account-"+req.profileIdentity+"/";
            const std::string racedPath=base+std::string(game)+"_"+req.timestamp;
            struct stat st{};
            assert(::lstat(racedPath.c_str(),&st)==0 && S_ISDIR(st.st_mode));
            assert(::rmdir(racedPath.c_str())==0); // proves it stayed empty
            const std::string racedTemp=base+"."+std::string(game)+"_"+req.timestamp+".pending";
            assert(::lstat(racedTemp.c_str(),&st)!=0 && errno==ENOENT);
            req.timestamp="20261012_221511";
            req.failurePoint=G::WorkingCopyFailurePoint::AfterFirstPublishedFile;
            assert(!G::publishGen5WorkingCopy(ws,req).success);
            const std::string midPath=base+std::string(game)+"_"+req.timestamp;
            assert(::lstat(midPath.c_str(),&st)!=0 && errno==ENOENT);
            const std::string midTemp=base+"."+std::string(game)+"_"+req.timestamp+".pending";
            assert(::lstat(midTemp.c_str(),&st)!=0 && errno==ENOENT);
        }
        req.failurePoint=G::WorkingCopyFailurePoint::None;
        req.timestamp="20261012_221520";
        auto stale=input;
        stale[0x70000]^=0x80;
        req.observedSourceBytes=stale;
        assert(!G::publishGen5WorkingCopy(ws,req).success);
        req.observedSourceBytes=input;
        req.profileIdentity=std::string(31,'0');
        assert(!G::publishGen5WorkingCopy(ws,req).success); // malformed UID
        // The existing offline "Pokemon Saves" profile is exactly zero UID.
        // It has a distinct owned export directory, never a source overwrite.
        req.profileIdentity=std::string(32,'0');
        const auto offline=G::publishGen5WorkingCopy(ws,req);
        assert(offline.success);
        verifyOnDisk(offline.directory+"/original_backup.sav",input);
        verifyOnDisk(offline.directory+"/edited.sav",out->bytes);
        cleanupPublished(offline.directory);
        const std::string offlineDir=exportRoot+
            "/account-00000000000000000000000000000000";
        assert(::rmdir(offlineDir.c_str())==0);
        const std::string profileDir=exportRoot+
            "/account-00000000000000000000000000000001";
        verifyOnDisk(sourcePath,input); // Still unchanged after every fault injection.
        assert(std::remove(sourcePath.c_str())==0);
        assert(::rmdir(profileDir.c_str())==0);
        assert(::rmdir(exportRoot.c_str())==0);
        // No original bytes or selection state were mutated by export.
        assert(std::equal(parsed->sourceBytes().begin(),
                          parsed->sourceBytes().end(),input.begin()));
    }
    // Gen IV-parity box operations: source-party/box native clones and
    // boxed removal are separately audited from field editing. The working
    // copy changes only one selected retail partition and verified CRCs.
    G::Gen5StagedPokemonWorkspace boxOps(*parsed);
    using Region=G::Gen5StagedPokemonWorkspace::Region;
    assert(boxOps.firstEmptyCloneDestination()==1);
    assert(!boxOps.cloneToEmptyBox({Region::Party,0,0},0,0,&error));
    assert(!boxOps.cloneToEmptyBox({Region::Party,0,4},0,1,&error));
    assert(boxOps.cloneToEmptyBox({Region::Party,0,0},0,1,&error));
    assert(!boxOps.cloneToEmptyBox({Region::Party,0,0},0,1,&error));
    assert(boxOps.isBoxReplacement(0,1));
    assert(boxOps.viewBox(0,1)->valid() && !boxOps.viewBox(0,1)->empty());
    assert(boxOps.viewBox(0,1)->species()==25);
    assert(boxOps.viewBox(0,1)->pid()==parsed->partyPokemon(0)->pid());
    assert(!boxOps.viewBox(0,1)->partyRecord());
    // A staged native clone remains editable through the SAME field model;
    // its immutable party-source certificate must survive each extra edit.
    assert(boxOps.stageBox(0,1,Field::Friendship,0,199,&error));
    assert(boxOps.stageBoxName(0,1,true,"CLONE",&error));
    assert(boxOps.viewBox(0,1)->friendship()==199);
    assert(!boxOps.removeBox(0,1,&error)); // Cannot remove newly staged clone.
    assert(boxOps.removeBox(0,0,&error));
    assert(!boxOps.removeBox(0,0,&error));
    assert(boxOps.viewBox(0,0)->valid() && boxOps.viewBox(0,0)->empty());
    assert(!boxOps.cloneToEmptyBox({Region::Box,0,0},0,2,&error));
    assert(boxOps.changedRecordCount()==2);
    bool sawClone=false,sawRemove=false;
    for(const auto& change:boxOps.pendingReview()) {
        const auto verified=G::verifyStagedReview(change,&error);
        assert(verified && !verified->fields.empty());
        if(change.kind==G::Gen5StagedPokemonWorkspace::ChangeKind::CloneBox) {
            assert(verified->fields.size()==3);
            assert(verified->fields[1].field==Field::NicknameText);
            assert(verified->fields[2].field==Field::Friendship);
            sawClone=true;
            assert(change.location.box==0 && change.location.slot==1);
            assert(verified->fields[0].field==Field::CloneBox);
            auto forged=change;
            forged.cloneSourceOriginal[0]^=0x1;
            assert(!G::verifyStagedReview(forged,&error));
            forged=change;
            forged.replacementOriginal[8]^=0x1;
            assert(!G::verifyStagedReview(forged,&error));
            forged=change;
            forged.after[8]^=0x1;
            assert(!G::verifyStagedReview(forged,&error));
        } else {
            sawRemove=true;
            assert(change.kind==G::Gen5StagedPokemonWorkspace::ChangeKind::RemoveBox);
            assert(verified->fields.size()==1);
            assert(verified->fields[0].field==Field::RemoveBox);
            auto forged=change;
            forged.after[10]=1;
            assert(!G::verifyStagedReview(forged,&error));
        }
    }
    assert(sawClone && sawRemove);
    const auto operations=G::buildStagedSaveCandidate(boxOps,&error);
    assert(operations && error.empty() && operations->changedRecords==2);
    const auto checked=G::Gen5ReadOnlySave::parse(operations->bytes,game,
        &error,selected);
    assert(checked && checked->boxPokemon(0,0)->empty());
    assert(checked->boxPokemon(0,1)->species()==25);
    assert(checked->boxPokemon(0,1)->friendship()==199);
    assert(checked->boxPokemon(0,1)->pid()==parsed->partyPokemon(0)->pid());
    assert(checked->partyPokemon(0)->species()==25);
    assert(std::equal(parsed->sourceBytes().begin(),
                      parsed->sourceBytes().end(),input.begin()));
    // Undo a single boxed operation without losing unrelated draft edits.
    assert(boxOps.undoBoxReplacement(0,1));
    assert(boxOps.changedRecordCount()==1);
    assert(!boxOps.undoBoxReplacement(0,1));
    assert(boxOps.undoBoxReplacement(0,0));
    assert(!boxOps.hasChanges());
    assert(boxOps.viewBox(0,0)->species()==133);
    assert(boxOps.viewBox(0,1)->empty());
    // Box-to-box exact copy uses the original 136-byte encrypted PK5,
    // never a party truncation and never an unrelated occupied target.
    G::Gen5StagedPokemonWorkspace boxClone(*parsed);
    assert(boxClone.cloneToEmptyBox({Region::Box,0,0},0,2,&error));
    assert(boxClone.viewBox(0,2)->species()==133);
    assert(boxClone.viewBox(0,2)->pid()==parsed->boxPokemon(0,0)->pid());
    assert(!boxClone.cloneToEmptyBox({Region::Box,0,2},0,3,&error));
    assert(boxClone.stageBox(0,2,Field::Friendship,0,180,&error));
    const auto boxedReview=boxClone.pendingReview();
    assert(boxedReview.size()==1 && boxedReview[0].replacementOriginal.size()==136);
    const auto boxedVerified=G::verifyStagedReview(boxedReview.front(),&error);
    assert(boxedVerified && boxedVerified->fields.size()==2);
    const auto boxedCandidate=G::buildStagedSaveCandidate(boxClone,&error);
    assert(boxedCandidate && boxedCandidate->changedRecords==1);
    const auto boxedParsed=G::Gen5ReadOnlySave::parse(
        boxedCandidate->bytes,game,&error,selected);
    assert(boxedParsed && boxedParsed->boxPokemon(0,2)->species()==133);
    assert(boxedParsed->boxPokemon(0,2)->friendship()==180);
    assert(boxedParsed->boxPokemon(0,0)->species()==133);
    auto staleClone=input;
    staleClone.back()^=0x80;
    assert(!G::buildStagedSaveCandidateForObservedSource(boxClone,staleClone,&error));
    assert(G::buildStagedSaveCandidateForObservedSource(boxClone,input,&error));
    assert(boxClone.undoBoxReplacement(0,2));
    assert(!boxClone.hasChanges());
    // A species-zero record can still hold data. It is *not* a physically
    // empty slot and must never be silently overwritten by Clone.
    auto contaminated=input;
    std::vector<uint8_t> ghost(C::StoredSize,0);
    C::write16(ghost,0x0A,1);
    const auto ghostEncrypted=C::encryptCandidate(ghost);
    std::copy(ghostEncrypted.begin(),ghostEncrypted.end(),
        contaminated.begin()+static_cast<std::ptrdiff_t>(
            base+L::BoxOffset+C::StoredSize));
    stamp(std::span<uint8_t>(contaminated).subspan(base,copySize),blocks);
    const auto ghostSave=G::Gen5ReadOnlySave::parse(
        contaminated,game,&error,selected);
    assert(ghostSave && ghostSave->boxPokemon(0,1)->valid() &&
           ghostSave->boxPokemon(0,1)->empty());
    G::Gen5StagedPokemonWorkspace ghostOps(*ghostSave);
    assert(!ghostOps.emptyCloneDestination(0,1));
    assert(!ghostOps.cloneToEmptyBox({Region::Party,0,0},0,1,&error));
    assert(!ghostOps.hasChanges());
    ws.discardAll();
    assert(!ws.hasChanges() && !G::buildStagedSaveCandidate(ws,&error));
}
int main() {
    const std::array<uint32_t,8> shuffleBits{0,1,6,11,17,23,24,31};
    for (const auto sv:shuffleBits) {
        const uint32_t pid=(sv<<13)|0x123u;
        exercise(G::SaveFamily::BlackWhite,"white_nds",20,pid,false);
        exercise(G::SaveFamily::BlackWhite,"black_nds",21,pid,true);
        exercise(G::SaveFamily::Black2White2,"white2_nds",22,pid,false);
        exercise(G::SaveFamily::Black2White2,"black2_nds",23,pid,true);
    }
    std::cout<<"Gen V SAV5 app-memory candidate: BW/B2W2, 8 shuffle codes,"
                " exact-copy CRC and immutable-source contracts PASS\n";
}
