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
