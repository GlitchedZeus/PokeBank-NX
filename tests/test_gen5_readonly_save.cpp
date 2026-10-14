#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Pokemon/Pokemon5ReadOnlyView.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5GameSourceCatalog.h"
#include "Integration/Gen5/Gen5GameCardPreview.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"
#include "Integration/Gen5/Gen5SharedEditorBridge.h"
#include "Integration/Gen5/Gen5SharedReview.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Games/GameIdentity.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;

void write32(std::vector<uint8_t>& dst,size_t off,uint32_t value) {
    C::write16(dst,off,static_cast<uint16_t>(value));
    C::write16(dst,off+2,static_cast<uint16_t>(value>>16));
}
template<size_t N>
void stamp(std::vector<uint8_t>& sav,size_t base,
           const std::array<L::SaveBlock,N>& blocks) {
    const size_t copySize = N == 70 ? L::BlackWhiteCopySize : L::Black2White2CopySize;
    auto partition = std::span<uint8_t>(sav).subspan(base,copySize);
    for(const auto& b : blocks) {
        const uint16_t crc=Utils::crc16ccitt(partition.data()+b.offset,b.length);
        C::write16(partition,b.checksumOffset,crc);
        C::write16(partition,b.mirrorOffset,crc);
    }
}
std::vector<uint8_t> make(G::SaveFamily family,uint8_t version,size_t base=0) {
    std::vector<uint8_t> sav(L::FullSaveSize,0);
    auto tr=base+0x19400;
    sav[tr+4]='N'; sav[tr+6]='X';
    C::write16(sav,tr+0x14,12345);
    C::write16(sav,tr+0x16,54321);
    sav[tr+0x1E]=2;
    sav[tr+0x1F]=version;
    sav[tr+0x21]=1;
    C::write16(sav,tr+0x24,23);
    sav[tr+0x26]=11;
    // Party Pikachu
    std::vector<uint8_t> pk(C::PartySize,0);
    write32(pk,0,0x12345678);
    C::write16(pk,8,25);
    C::write16(pk,12,12345);
    C::write16(pk,14,54321);
    pk[0x8C]=50; // native PK5 party level; boxed PK5 has no level byte
    const auto encrypted=C::encryptCandidate(pk);
    sav[base+L::PartyOffset+4]=1;
    std::copy(encrypted.begin(),encrypted.end(),
              sav.begin()+static_cast<std::ptrdiff_t>(base+L::PartyOffset+8));
    // Box Eevee
    std::vector<uint8_t> boxed(C::StoredSize,0);
    write32(boxed,0,0xABCDEF01);
    C::write16(boxed,8,133);
    const auto boxEncrypted=C::encryptCandidate(boxed);
    std::copy(boxEncrypted.begin(),boxEncrypted.end(),
              sav.begin()+static_cast<std::ptrdiff_t>(base+L::BoxOffset));
    const size_t dex=base+(family==G::SaveFamily::BlackWhite ? 0x21600 : 0x21400);
    sav[dex+8+3]|=1u; // species 25: bit 24
    sav[dex+8+0x54+3]|=1u;
    if(family==G::SaveFamily::BlackWhite)stamp(sav,base,L::BlackWhite);
    else stamp(sav,base,L::Black2White2);
    return sav;
}
void fileFixture(const std::string& path,const std::vector<uint8_t>& bytes) {
    FILE* f=std::fopen(path.c_str(),"wb");
    assert(f);
    assert(std::fwrite(bytes.data(),1,bytes.size(),f)==bytes.size());
    assert(std::fclose(f)==0);
}
std::vector<uint8_t> dsvFixture(const std::vector<uint8_t>& raw) {
    auto wrapped=raw;
    wrapped.resize(raw.size()+40,0);
    wrapped[raw.size()+4]=0; wrapped[raw.size()+5]=0;
    wrapped[raw.size()+6]=8; wrapped[raw.size()+7]=0;
    constexpr std::array<char,16> marker{
        '|','-','D','E','S','M','U','M','E',' ','S','A','V','E','-','|'
    };
    std::copy(marker.begin(),marker.end(),wrapped.begin()+static_cast<std::ptrdiff_t>(raw.size()+24));
    return wrapped;
}
void rejectedRetroArchGen5DiagnosticContracts() {
    char pattern[]="/tmp/pokebank_gen5_invalid_retroarch_XXXXXX";
    const char* temp=::mkdtemp(pattern);
    assert(temp);
    const std::string root=temp;
    const std::string invalid=root+"/black_rejected.srm";
    const std::string unrelated=root+"/not_gen5.sav";
    auto candidate=make(G::SaveFamily::BlackWhite,21);
    candidate[L::BoxOffset+7]^=uint8_t{0x80}; // checksum mismatch, retain native version byte
    fileFixture(invalid,candidate);
    fileFixture(unrelated,std::vector<uint8_t>(L::FullSaveSize,0));
    const auto inspected=G::inspectSourceFile(invalid,"RetroArch");
    assert(!inspected.ready());
    assert(inspected.gameId.empty()); // no trusted game identity from corrupt save
    assert(inspected.validation==PokeVault::Source::ValidationStatus::Invalid);
    assert(inspected.sourceVariant=="gen5-unverified-version-hint");
    assert(inspected.diagnostic.find("unverified Gen V header suggests black_nds")!=std::string::npos);
    assert(!G::reopenValidatedSource(inspected).ready());
    const G::DiscoveryRoot source{root,"RetroArch",0};
    const auto discovered=G::discoverSources(std::span<const G::DiscoveryRoot>(&source,1));
    assert(discovered.filesExamined==2);
    assert(discovered.instances.size()==1); // unrelated old-gen raw images remain hidden
    assert(discovered.instances[0].sourcePath==invalid);
    assert(!discovered.instances[0].ready());
    PokeVault::Legacy::LegacySourceBindings claims(root+"/claims");
    assert(claims.load());
    const auto catalog=G::forGameAndProfile(discovered.instances,claims,"profile-one","black_nds");
    assert(catalog.validQuery);
    assert(catalog.rows.empty() && catalog.invalidOrAmbiguous==1);
    // Black 2 backup-only candidates are diagnosed without becoming valid
    // after the checksum is damaged; no inferred release/assignment occurs.
    auto b2=make(G::SaveFamily::Black2White2,23,L::Black2White2CopySize);
    b2[L::Black2White2CopySize+L::BoxOffset+7]^=uint8_t{0x40};
    const std::string backup=root+"/black2_rejected.sav";
    fileFixture(backup,b2);
    const auto rejectedBackup=G::inspectSourceFile(backup,"RetroArch");
    assert(!rejectedBackup.ready());
    assert(rejectedBackup.gameId.empty());
    assert(rejectedBackup.sourceVariant=="gen5-unverified-version-hint");
    assert(rejectedBackup.diagnostic.find("black2_nds")!=std::string::npos);
    assert(std::remove(backup.c_str())==0);
    assert(std::remove(invalid.c_str())==0);
    assert(std::remove(unrelated.c_str())==0);
    assert(::rmdir(root.c_str())==0);
}
void extendedDsvContainerContracts() {
    char pattern[]="/tmp/pokebank_gen5_dsv_XXXXXX";
    const char* root=::mkdtemp(pattern);
    assert(root);
    const std::string path=std::string(root)+"/white_extended.dsv";
    const auto raw=make(G::SaveFamily::BlackWhite,20);
    auto extended=dsvFixture(raw);
    // Gen IV accepts this same emulator wrapper: 82 metadata bytes before
    // the fixed footer; a Gen V reader must not reject it by total size.
    extended.insert(extended.end()-40,82,uint8_t{0xA5});
    assert(extended.size()==L::FullSaveSize+122);
    fileFixture(path,extended);
    std::vector<uint8_t> normalized;
    std::string diagnostic,container;
    assert(G::readNormalizedSourceReadOnly(path,normalized,diagnostic,&container));
    assert(normalized==raw && container=="dsv-footer");
    const auto ready=G::inspectSourceFile(path,"DraStic");
    assert(ready.ready() && ready.gameId=="white_nds");
    assert(ready.fileSize==extended.size());
    assert(G::reopenValidatedSource(ready).ready());
    // The original Gen IV adapter also permits a longer metadata trailer.
    // Rejecting it solely for exceeding an invented 4096-byte cap made
    // the same emulator work for Gen IV but fail for Gen V.
    extended=dsvFixture(raw);
    extended.insert(extended.end()-40,8192,uint8_t{0xA6});
    fileFixture(path,extended);
    assert(G::readNormalizedSourceReadOnly(path,normalized,diagnostic,&container));
    assert(normalized==raw && container=="dsv-footer");
    const auto longFooter=G::inspectSourceFile(path,"DraStic");
    assert(longFooter.ready() && longFooter.gameId=="white_nds");
    assert(longFooter.fileSize==extended.size());
    assert(G::reopenValidatedSource(longFooter).ready());
    assert(!G::reopenValidatedSource(ready).ready()); // old snapshot invalidated
    // Invalid footer cannot become a battery save merely by having the
    // correct 0x80000-byte prefix.
    extended.back()^=0x01;
    fileFixture(path,extended);
    assert(!G::readNormalizedSourceReadOnly(path,normalized,diagnostic));
    assert(!G::inspectSourceFile(path,"DraStic").ready());
    assert(!G::reopenValidatedSource(ready).ready());
    assert(std::remove(path.c_str())==0);
    assert(::rmdir(root)==0);
}
void discoveryContracts() {
    char pattern[]="/tmp/pokebank_gen5_discovery_XXXXXX";
    const char* tmp=::mkdtemp(pattern);
    assert(tmp);
    const std::string dir=tmp;
    const std::string nested=dir+"/nested";
    assert(::mkdir(nested.c_str(),0700)==0);
    const std::string black=dir+"/a_black.sav";
    const std::string white=dir+"/w_white.SRM";
    const std::string dsv=dir+"/d_white.dsv";
    const std::string state=dir+"/b_state.dss";
    const std::string symlinkPath=dir+"/c_outside.sav";
    const std::string tooDeep=dir+"/nested/not_gen5.sav";
    const auto blackRaw=make(G::SaveFamily::BlackWhite,21);
    const auto whiteRaw=make(G::SaveFamily::BlackWhite,20);
    fileFixture(black,blackRaw);
    fileFixture(white,whiteRaw);
    fileFixture(dsv,dsvFixture(whiteRaw));
    fileFixture(state,blackRaw); // Even a valid-looking .dss is NOT a battery save.
    fileFixture(tooDeep,std::vector<uint8_t>(L::FullSaveSize,0));
    assert(::symlink(black.c_str(),symlinkPath.c_str())==0);

    const auto probe=G::inspectSourceFile(black,"RetroArch");
    assert(probe.ready() && probe.gameId=="black_nds");
    assert(probe.trainerName=="NX");
    assert(probe.readOnly() && probe.providerId=="retroarch");
    assert(probe.contentFingerprint.size()==64);
    assert(probe.physicalIdentity.find("inode:")==0);
    assert(!G::inspectSourceFile(black,"RetroArch","white_nds").ready());
    assert(G::inspectSourceFile(black,"RetroArch","white_nds").validation==
           PokeVault::Source::ValidationStatus::AssignmentMismatch);
    assert(!G::inspectSourceFile(symlinkPath,"RetroArch").ready());
    auto direct=G::reopenValidatedSource(probe);
    assert(direct.ready() && direct.save->exactGameId()=="black_nds");
    assert(direct.instance.sourceIdentity==probe.sourceIdentity);
    // The existing persistent profile/game binding is used only when a
    // human explicitly associates this exact physical source with a game.
    const std::string database=dir+"/gen5_bindings";
    PokeVault::Legacy::LegacySourceBindings bindings(database);
    assert(bindings.load());
    assert(bindings.assignFileAndSave(probe.sourceIdentity,
        {"profile-one","black_nds",black,"RetroArch","BW"}));
    const auto owned=G::openAssignedSource(bindings,"profile-one","black_nds");
    assert(owned.ready());
    assert(owned.save->exactGameId()=="black_nds");
    assert(owned.instance.sourceIdentity==probe.sourceIdentity);
    assert(owned.instance.claimedProfile=="profile-one");
    assert(owned.instance.readOnly() && owned.instance.rememberedSource);
    // Final Open accepts only the exact normalized bytes chosen in Save
    // Instances, not merely a matching inode/path or game title.
    assert(G::openAssignedSource(bindings,"profile-one","black_nds",
        G::SaveCopySelection::Automatic,probe.contentFingerprint).ready());
    assert(!G::openAssignedSource(bindings,"profile-one","black_nds",
        G::SaveCopySelection::Automatic,std::string(64,'0')).ready());
    assert(owned.instance.claimedProfile=="profile-one");
    assert(owned.instance.providerId=="retroarch");
    const auto gamePreview=G::previewAssignedGame(owned);
    assert(gamePreview);
    assert(gamePreview->exactGameId=="black_nds");
    assert(gamePreview->providerLabel=="RetroArch");
    assert(gamePreview->sourceIdentity==probe.sourceIdentity);
    assert(gamePreview->trainerName=="NX");
    assert(gamePreview->partyCount==1);
    assert(gamePreview->party[0].occupied && gamePreview->party[0].species==25);
    assert(gamePreview->party[0].level==50);
    assert(!gamePreview->party[1].occupied && !gamePreview->backupCopySelected);
    assert(gamePreview->dexSeen==1 && gamePreview->dexCaught==1);
    assert(gamePreview->dexTotal==649);
    const G::AssignedSourceReadOnly unassigned{};
    assert(!G::previewAssignedGame(unassigned));
    // Re-parsing native bytes must preserve non-native assignment metadata.
    auto remembered=probe;
    remembered.rememberedSource=true;
    remembered.mostRecentlyModified=true;
    remembered.sourceAliases={"second-location-same-file"};
    const auto reopenedRemembered=G::reopenValidatedSource(remembered);
    assert(reopenedRemembered.ready());
    assert(reopenedRemembered.instance.rememberedSource);
    assert(reopenedRemembered.instance.mostRecentlyModified);
    assert(reopenedRemembered.instance.sourceAliases==remembered.sourceAliases);
    assert(G::openAssignedSource(bindings,"other-profile","black_nds").status==
        G::AssignedOpenStatus::Unassigned);
    assert(G::openAssignedSource(bindings,"profile-one","white_nds").status==
        G::AssignedOpenStatus::Unassigned);
    assert(G::openAssignedSource(bindings,"profile-one","platinum_nds").status==
        G::AssignedOpenStatus::AssignmentMismatch);
    PokeVault::Legacy::LegacySourceBindings restored(database);
    assert(restored.load());
    assert(G::openAssignedSource(restored,"profile-one","black_nds").ready());
    // A malicious/stale family claim does not reinterpret native contents.
    const std::string badDatabase=dir+"/gen5_bad_bindings";
    PokeVault::Legacy::LegacySourceBindings badBindings(badDatabase);
    assert(badBindings.load());
    assert(badBindings.assignFileAndSave(probe.sourceIdentity,
        {"profile-one","black_nds",black,"RetroArch","B2W2"}));
    assert(G::openAssignedSource(badBindings,"profile-one","black_nds").status==
        G::AssignedOpenStatus::AssignmentMismatch);
    // Keep an in-place emulator update assigned; refuse a different physical
    // file moved into the same exact path despite valid checksum and title.
    const std::string replacementPath=dir+"/physical_black.sav";
    const std::string rotated=dir+"/physical_black.old";
    const std::string replacementDb=dir+"/physical_bindings";
    fileFixture(replacementPath,blackRaw);
    const auto originalPhysical=G::inspectSourceFile(replacementPath,"RetroArch");
    assert(originalPhysical.ready());
    PokeVault::Legacy::LegacySourceBindings physicalBindings(replacementDb);
    assert(physicalBindings.load());
    assert(physicalBindings.assignFileAndSave(originalPhysical.sourceIdentity,
        {"owner-one","black_nds",replacementPath,"RetroArch","BW"}));
    assert(G::openAssignedSource(physicalBindings,"owner-one","black_nds").ready());
    auto updatedPhysical=blackRaw;
    updatedPhysical[0x19400+4]='Y';
    stamp(updatedPhysical,0,L::BlackWhite);
    fileFixture(replacementPath,updatedPhysical);
    const auto inPlace=G::inspectSourceFile(replacementPath,"RetroArch");
    assert(inPlace.ready() && inPlace.sourceIdentity==originalPhysical.sourceIdentity);
    assert(inPlace.contentFingerprint!=originalPhysical.contentFingerprint);
    assert(G::openAssignedSource(physicalBindings,"owner-one","black_nds").ready());
    assert(::rename(replacementPath.c_str(),rotated.c_str())==0);
    fileFixture(replacementPath,updatedPhysical);
    const auto replacement=G::inspectSourceFile(replacementPath,"RetroArch");
    assert(replacement.ready() && replacement.gameId=="black_nds");
    assert(replacement.sourceIdentity!=originalPhysical.sourceIdentity);
    assert(G::openAssignedSource(physicalBindings,"owner-one","black_nds").status==
           G::AssignedOpenStatus::AssignmentMismatch);
    assert(std::remove(replacementPath.c_str())==0);
    assert(std::remove(rotated.c_str())==0);
    assert(std::remove(replacementDb.c_str())==0);
    const auto wrapped=G::inspectSourceFile(dsv,"DraStic");
    assert(wrapped.ready() && wrapped.gameId=="white_nds");
    assert(wrapped.containerType=="dsv-footer");
    assert(wrapped.sourceLabel.find("Boxed 1")!=std::string::npos);
    assert(G::reopenValidatedSource(wrapped).ready());
    assert(bindings.assignFileAndSave(wrapped.sourceIdentity,
        {"other-profile","white_nds",dsv,"DraStic","BW"}));
    auto samePhysical=probe;
    samePhysical.sourceIdentity="alias-from-same-physical-file";
    std::array<PokeVault::Source::SaveInstance,4> found{
        samePhysical,probe,wrapped,G::inspectSourceFile(state,"DraStic")
    };
    const auto blackRows=G::forGameAndProfile(found,bindings,"profile-one","black_nds");
    assert(blackRows.validQuery && blackRows.rows.size()==1);
    assert(blackRows.rows[0].rememberedSource);
    assert(blackRows.rows[0].sourceAliases.size()==1);
    assert(blackRows.rows[0].sourceAliases[0]==probe.sourceIdentity);
    assert(blackRows.rows[0].claimedProfile=="profile-one");
    assert(blackRows.rows[0].mostRecentlyModified);
    const auto cannotSeeBlack=G::forGameAndProfile(found,bindings,"other-profile","black_nds");
    assert(cannotSeeBlack.rows.empty());
    assert(cannotSeeBlack.ownedByOtherProfile==1);
    const auto cannotSeeWhite=G::forGameAndProfile(found,bindings,"profile-one","white_nds");
    assert(cannotSeeWhite.rows.empty());
    assert(cannotSeeWhite.ownedByOtherProfile==1);
    const auto whiteRows=G::forGameAndProfile(found,bindings,"other-profile","white_nds");
    assert(whiteRows.validQuery && whiteRows.rows.size()==1);
    assert(whiteRows.rows[0].rememberedSource);
    assert(whiteRows.rows[0].gameId=="white_nds");
    assert(!G::forGameAndProfile(found,bindings,"profile-one","platinum_nds").validQuery);
    const auto stateRow=G::inspectSourceFile(state,"DraStic");
    assert(!stateRow.ready() && stateRow.kind==PokeVault::Source::SaveInstanceKind::SaveState);
    const auto original=G::inspectSourceFile(white,"melonDS");
    assert(original.ready() && original.gameId=="white_nds");
    assert(original.containerType=="raw-nds-battery");

    const G::DiscoveryRoot one{dir,"RetroArch",0};
    const auto results=G::discoverSources(std::span<const G::DiscoveryRoot>(&one,1));
    assert(results.filesExamined==4); // Invalid symlink skipped; nested excluded.
    assert(results.instances.size()==4); // Three valid plus explicit DSV/DSS diagnostics.
    assert(results.instances[0].mostRecentlyModified);
    assert(!results.instances[1].mostRecentlyModified);
    for(const auto& row:results.instances) {
        assert(row.readOnly());
        assert(row.sourcePath.find(dir)==0);
        assert(row.sourcePath!=symlinkPath);
    }
    const auto limited=G::discoverSources(std::span<const G::DiscoveryRoot>(&one,1),{1});
    assert(limited.filesExamined==1 && limited.limitReached);
    assert(limited.instances.size()==1 && limited.instances[0].gameId=="black_nds");
    // A maliciously broad RetroArch configured save root must never
    // trigger an SD-card/root scan, even with an explicit config file.
    const G::DiscoveryRoot fsRoot{"/","RetroArch",2};
    const G::DiscoveryRoot sdRoot{"sdmc:/","RetroArch",2};
    const G::DiscoveryRoot dotdotRoot{dir+"/../","RetroArch",2};
    for(const auto& forbidden : {fsRoot,sdRoot,dotdotRoot}) {
        const auto blocked=G::discoverSources(
            std::span<const G::DiscoveryRoot>(&forbidden,1),{4});
        assert(blocked.filesExamined==0 && blocked.instances.empty());
    }
    const std::string retroarchCfg=dir+"/retroarch.cfg";
    {
        FILE* f=std::fopen(retroarchCfg.c_str(),"wb");
        assert(f);
        constexpr std::string_view setting="savefile_directory = \"/\"\n";
        assert(std::fwrite(setting.data(),1,setting.size(),f)==setting.size());
        assert(std::fclose(f)==0);
    }
    const auto safeDefault=G::discoverKnownSources(
        {4},retroarchCfg,dir+"/nonexistent_default_root");
    assert(safeDefault.filesExamined==0 && safeDefault.instances.empty());
    assert(std::remove(retroarchCfg.c_str())==0);
    const auto roots=G::defaultDraSticRoots();
    assert(roots.size()==4 && roots[0].providerLabel=="DraStic");

    // File changes must invalidate an earlier fingerprint. No stale reopen.
    auto modified=blackRaw;
    modified[0x400+4]^=0x40;
    fileFixture(black,modified);
    assert(!G::reopenValidatedSource(probe).ready());
    assert(!G::openAssignedSource(restored,"profile-one","black_nds").ready());

    // Two valid but differing copies must not be silently ranked by mtime.
    auto twoCopies=blackRaw;
    const auto secondary=make(G::SaveFamily::BlackWhite,21,L::BlackWhiteCopySize);
    std::copy(secondary.begin()+static_cast<std::ptrdiff_t>(L::BlackWhiteCopySize),
              secondary.begin()+static_cast<std::ptrdiff_t>(2*L::BlackWhiteCopySize),
              twoCopies.begin()+static_cast<std::ptrdiff_t>(L::BlackWhiteCopySize));
    twoCopies[L::BlackWhiteCopySize+0x19400+6]='Z';
    stamp(twoCopies,L::BlackWhiteCopySize,L::BlackWhite);
    fileFixture(black,twoCopies);
    assert(!G::inspectSourceFile(black,"RetroArch").ready());
    // Discovery must display both independently validated copies without
    // silently choosing their freshness or scanning the same file twice.
    const std::array<G::DiscoveryRoot,1> dualRoots{{{dir,"RetroArch",0}}};
    const auto dualDiscovery=G::discoverSources(dualRoots,{32});
    size_t shownCopies=0;
    for(const auto& entry:dualDiscovery.instances)
        if(entry.sourcePath==black) {
            assert(entry.ready() && entry.readOnly());
            assert(entry.sourceVariant=="gen5-copy-0-primary" ||
                   entry.sourceVariant=="gen5-copy-1-backup");
            ++shownCopies;
        }
    assert(shownCopies==2);
    const auto first=G::inspectSourceFile(
        black,"RetroArch",{},G::SaveCopySelection::Primary);
    const auto second=G::inspectSourceFile(
        black,"RetroArch",{},G::SaveCopySelection::Backup);
    assert(first.ready() && second.ready());
    assert(first.sourceVariant=="gen5-copy-0-primary" &&
           second.sourceVariant=="gen5-copy-1-backup");
    assert(first.sourceIdentity==second.sourceIdentity &&
           first.physicalIdentity==second.physicalIdentity);
    assert(first.gameId==second.gameId && first.gameId=="black_nds");
    const std::array<PokeVault::Source::SaveInstance,2> twoSelections{{first,second}};
    const auto catalogWithCopies=G::forGameAndProfile(
        twoSelections,restored,"profile-one","black_nds");
    assert(catalogWithCopies.rows.size()==2);
    assert(catalogWithCopies.rows[0].sourceVariant=="gen5-copy-0-primary");
    assert(catalogWithCopies.rows[1].sourceVariant=="gen5-copy-1-backup");
    assert(catalogWithCopies.rows[0].claimedProfile=="profile-one" &&
           catalogWithCopies.rows[1].claimedProfile=="profile-one");
    assert(first.sourceLabel.find("Primary copy")!=std::string::npos);
    assert(second.sourceLabel.find("Backup copy")!=std::string::npos);
    // Selected-copy rows display their independently verified PC occupancy
    // alongside Party count, so a legitimate but empty backup isn't
    // indistinguishable from a populated primary on Switch hardware.
    assert(first.sourceLabel.find("Boxed 1")!=std::string::npos);
    assert(second.sourceLabel.find("Boxed 1")!=std::string::npos);
    const auto openedPrimary=G::reopenValidatedSource(first,G::SaveCopySelection::Primary);
    const auto openedBackup=G::reopenValidatedSource(second,G::SaveCopySelection::Backup);
    assert(openedPrimary.ready() && openedBackup.ready());
    assert(openedPrimary.save->trainer().rawName==u"NX");
    assert(openedBackup.save->trainer().rawName==u"NZ");
    // Reopening an explicitly chosen row must preserve that exact copy,
    // even when the ordinary caller passes Automatic. There is no guess:
    // its saved sourceVariant was selected by the user and revalidated.
    assert(G::reopenValidatedSource(first).ready());
    assert(G::reopenValidatedSource(second).ready());
    assert(!G::reopenValidatedSource(second,G::SaveCopySelection::Primary).ready());
    assert(!G::reopenValidatedSource(first,G::SaveCopySelection::Backup).ready());
    assert(G::openAssignedSource(
        restored,"profile-one","black_nds",G::SaveCopySelection::Primary).ready());
    assert(G::openAssignedSource(
        restored,"profile-one","black_nds",G::SaveCopySelection::Backup).ready());
    assert(!G::openAssignedSource(restored,"profile-one","black_nds").ready());
    // This is an in-place emulator update, not a replacement path/inode.
    // Both copies remain checksum-valid; old selection must be refused,
    // while a newly selected exact snapshot can reopen.
    assert(G::openAssignedSource(restored,"profile-one","black_nds",
        G::SaveCopySelection::Primary,first.contentFingerprint).ready());
    auto rewritten=twoCopies;
    rewritten[0x19400+6]='R';
    stamp(rewritten,0,L::BlackWhite);
    fileFixture(black,rewritten);
    const auto updated=G::inspectSourceFile(
        black,"RetroArch",{},G::SaveCopySelection::Primary);
    assert(updated.ready() && updated.sourceIdentity==first.sourceIdentity);
    assert(updated.contentFingerprint!=first.contentFingerprint);
    const auto staleOpen=G::openAssignedSource(restored,"profile-one","black_nds",
        G::SaveCopySelection::Primary,first.contentFingerprint);
    assert(!staleOpen.ready());
    assert(staleOpen.diagnostic.find("source contents changed after selection")!=std::string::npos);
    assert(G::openAssignedSource(restored,"profile-one","black_nds",
        G::SaveCopySelection::Primary,updated.contentFingerprint).ready());
    assert(G::openAssignedSource(restored,"profile-one","black_nds",
        G::SaveCopySelection::Primary).ready()); // older passive-preview callers
    fileFixture(black,twoCopies);
    // A DSV with a malformed padding/version/footer is unsupported.
    auto malformed=dsvFixture(whiteRaw);
    malformed[whiteRaw.size()+7]=1;
    fileFixture(dsv,malformed);
    assert(!G::inspectSourceFile(dsv,"DraStic").ready());

    // File-count limits alone do not bound readdir, sorting, or allocations:
    // an emulator folder can contain thousands of unrelated ROM/temp files.
    // Total directory ENTRIES (even non-.sav files) and directory traversal
    // must be independently budgeted, with a visible incomplete-scan flag.
    const std::string flood=dir+"/many_unrelated_entries";
    assert(::mkdir(flood.c_str(),0700)==0);
    std::vector<std::string> junkFiles;
    for(int i=0;i<12;++i) {
        const auto path=flood+"/unrelated_"+std::to_string(i)+".txt";
        fileFixture(path,std::vector<uint8_t>{1});
        junkFiles.push_back(path);
    }
    const std::array<G::DiscoveryRoot,1> floodRoots{{{flood,"RetroArch",0}}};
    const auto budgeted=G::discoverSources(floodRoots,G::DiscoveryLimits{32,4,8});
    assert(budgeted.limitReached);
    assert(budgeted.directoriesExamined==1);
    assert(budgeted.entriesExamined==4);
    assert(budgeted.filesExamined==0 && budgeted.instances.empty());
    // Exhaustion of a separate total-directory budget must not scan
    // the second root silently or misreport an exhaustive source list.
    const std::array<G::DiscoveryRoot,2> twoRoots{{{dir,"RetroArch",0},
                                                    {flood,"RetroArch",0}}};
    const auto directoryBudget=G::discoverSources(
        twoRoots,G::DiscoveryLimits{32,4096,1});
    assert(directoryBudget.limitReached);
    assert(directoryBudget.directoriesExamined==1);
    for(const auto& path:junkFiles)assert(std::remove(path.c_str())==0);
    assert(::rmdir(flood.c_str())==0);

    assert(std::remove((database+".bak").c_str())==0);
    assert(std::remove(database.c_str())==0);
    assert(std::remove(badDatabase.c_str())==0);
    assert(std::remove(black.c_str())==0);
    assert(std::remove(white.c_str())==0);
    assert(std::remove(dsv.c_str())==0);
    assert(std::remove(state.c_str())==0);
    assert(std::remove(symlinkPath.c_str())==0);
    assert(std::remove(tooDeep.c_str())==0);
    assert(::rmdir(nested.c_str())==0);
    assert(::rmdir(dir.c_str())==0);
}

// Real profile/game assignment exercise for the first Gen V hardware NRO.
// Uses the same permanent bindings, strict file reopening and Save Instances
// catalog as Product Home, without any source save writes by production code.
void fourTitleProfileAssignmentContracts() {
    char pattern[]="/tmp/pokebank_gen5_profiles_XXXXXX";
    const char* temp=::mkdtemp(pattern);
    assert(temp);
    const std::string dir=temp;
    const std::string db=dir+"/assigned-profiles";
    struct Case {
        const char* id;
        const char* profile;
        const char* file;
        G::SaveFamily family;
        uint8_t version;
        const char* provider;
        bool wrapped;
        bool twoCopies;
    };
    const std::array<Case,4> games{{
        {"black_nds","profile-a","black.sav",G::SaveFamily::BlackWhite,21,
            "RetroArch",false,false},
        {"white_nds","profile-b","white.dsv",G::SaveFamily::BlackWhite,20,
            "DraStic",true,false},
        {"black2_nds","profile-a","black2.srm",G::SaveFamily::Black2White2,23,
            "melonDS",false,false},
        {"white2_nds","profile-b","white2.sav",G::SaveFamily::Black2White2,22,
            "Manual",false,true}
    }};
    struct Entry {
        std::string path;
        std::vector<uint8_t> disk;
        PokeVault::Source::SaveInstance chosen;
        G::SaveCopySelection selection;
    };
    std::array<Entry,4> entries{};
    std::vector<PokeVault::Source::SaveInstance> discoveries;
    PokeVault::Legacy::LegacySourceBindings bindings(db);
    assert(bindings.load());
    for(size_t i=0;i<games.size();++i) {
        const auto& g=games[i];
        const size_t copySize=g.family==G::SaveFamily::BlackWhite?
            L::BlackWhiteCopySize:L::Black2White2CopySize;
        auto sav=make(g.family,g.version);
        if(g.twoCopies) {
            auto second=make(g.family,g.version,copySize);
            // Two complete, individually valid copies with different source
            // state; no heuristic may guess which was played most recently.
            second[copySize+0x19400+6]='Y';
            if(g.family==G::SaveFamily::BlackWhite)
                stamp(second,copySize,L::BlackWhite);
            else stamp(second,copySize,L::Black2White2);
            std::copy(second.begin()+static_cast<std::ptrdiff_t>(copySize),
                      second.begin()+static_cast<std::ptrdiff_t>(2*copySize),
                      sav.begin()+static_cast<std::ptrdiff_t>(copySize));
        }
        auto& entry=entries[i];
        entry.path=dir+"/"+g.file;
        entry.disk=g.wrapped?dsvFixture(sav):sav;
        fileFixture(entry.path,entry.disk);
        if(g.twoCopies) {
            assert(!G::inspectSourceFile(entry.path,g.provider,g.id).ready());
            auto primary=G::inspectSourceFile(entry.path,g.provider,g.id,
                G::SaveCopySelection::Primary);
            auto backup=G::inspectSourceFile(entry.path,g.provider,g.id,
                G::SaveCopySelection::Backup);
            assert(primary.ready() && backup.ready());
            assert(primary.sourceIdentity==backup.sourceIdentity);
            assert(primary.physicalIdentity==backup.physicalIdentity);
            assert(primary.sourceVariant=="gen5-copy-0-primary");
            assert(backup.sourceVariant=="gen5-copy-1-backup");
            discoveries.push_back(primary);
            discoveries.push_back(backup);
            entry.chosen=backup;
            entry.selection=G::SaveCopySelection::Backup;
        } else {
            entry.chosen=G::inspectSourceFile(entry.path,g.provider,g.id);
            entry.selection=G::SaveCopySelection::Automatic;
            assert(entry.chosen.ready());
            discoveries.push_back(entry.chosen);
        }
        const auto& source=entry.chosen;
        assert(source.ready() && source.readOnly() && source.gameId==g.id);
        assert(source.providerLabel==g.provider && !source.sourceIdentity.empty());
        assert(bindings.replaceFileAssignmentAndSave(source.sourceIdentity,
            {g.profile,g.id,entry.path,g.provider,
             g.family==G::SaveFamily::BlackWhite?"BW":"B2W2"}));
        const auto selected=G::openAssignedSource(bindings,g.profile,g.id,
            entry.selection,source.contentFingerprint);
        assert(selected.ready() && selected.instance.gameId==g.id);
        assert(selected.instance.sourceIdentity==source.sourceIdentity);
        assert(selected.instance.contentFingerprint==source.contentFingerprint);
        assert(selected.instance.providerLabel==g.provider);
        assert(selected.instance.readOnly() && selected.save);
        assert(selected.save->selectedBackupPartition()==g.twoCopies);
        assert(selected.save->trainer().rawName==
               (g.twoCopies?u"NY":u"NX"));
        assert(!G::openAssignedSource(bindings,
            std::string(g.profile)=="profile-a"?"profile-b":"profile-a",
            g.id).ready());
        // The exact title remains authoritative even for identical-size raw
        // files. A source assigned to Black must never open for White.
        const char* wrong=i==0?"white_nds":
            i==1?"black_nds":i==2?"white2_nds":"black2_nds";
        const auto wrongTitle=G::inspectSourceFile(entry.path,g.provider,wrong,
            entry.selection);
        assert(!wrongTitle.ready() &&
               wrongTitle.validation==
                   PokeVault::Source::ValidationStatus::AssignmentMismatch);
    }
    PokeVault::Legacy::LegacySourceBindings restored(db);
    assert(restored.load()); // Persistent user profile associations survive restart.
    for(size_t i=0;i<games.size();++i) {
        const auto& g=games[i];
        const auto& entry=entries[i];
        const auto opened=G::openAssignedSource(restored,g.profile,g.id,
            entry.selection,entry.chosen.contentFingerprint);
        assert(opened.ready() && opened.instance.claimedProfile==g.profile);
        const auto catalog=G::forGameAndProfile(discoveries,restored,g.profile,g.id);
        assert(catalog.validQuery && catalog.rows.size()==(g.twoCopies?2u:1u));
        for(const auto& row:catalog.rows) {
            assert(row.gameId==g.id && row.claimedProfile==g.profile);
            assert(row.readOnly() && row.ready());
        }
        const auto other=G::forGameAndProfile(discoveries,restored,
            std::string(g.profile)=="profile-a"?"profile-b":"profile-a",g.id);
        assert(other.validQuery && other.rows.empty());
    }
    // The selected White 2 copy is a session choice, not a permanent guess.
    const auto& dual=entries[3];
    assert(!G::openAssignedSource(restored,"profile-b","white2_nds",
        G::SaveCopySelection::Automatic,dual.chosen.contentFingerprint).ready());
    const auto primary=G::openAssignedSource(restored,"profile-b","white2_nds",
        G::SaveCopySelection::Primary,dual.chosen.contentFingerprint);
    const auto backup=G::openAssignedSource(restored,"profile-b","white2_nds",
        G::SaveCopySelection::Backup,dual.chosen.contentFingerprint);
    assert(primary.ready() && backup.ready());
    assert(primary.save->trainer().rawName==u"NX");
    assert(backup.save->trainer().rawName==u"NY");
    // Same physical source changed after the selection: the original
    // fingerprint cannot authorize an unrelated/newer read.
    auto changed=make(G::SaveFamily::BlackWhite,21);
    changed[0x19400+6]='Z';
    stamp(changed,0,L::BlackWhite);
    fileFixture(entries[0].path,changed);
    assert(!G::openAssignedSource(restored,"profile-a","black_nds",
        G::SaveCopySelection::Automatic,
        entries[0].chosen.contentFingerprint).ready());
    assert(G::openAssignedSource(restored,"profile-a","black_nds").ready());
    // Restore test fixture; a real emulator's original files are never
    // mutated by the application nor by this host-only test.
    fileFixture(entries[0].path,entries[0].disk);
    assert(G::openAssignedSource(restored,"profile-a","black_nds",
        G::SaveCopySelection::Automatic,
        entries[0].chosen.contentFingerprint).ready());
    for(const auto& entry:entries)assert(std::remove(entry.path.c_str())==0);
    assert(std::remove(db.c_str())==0);
    (void)std::remove((db+".bak").c_str());
    (void)std::remove((db+".tmp").c_str());
    assert(::rmdir(dir.c_str())==0);
}

int main() {
    for(const auto& [family,version,id] :
        {std::tuple{G::SaveFamily::BlackWhite,uint8_t{20},"white_nds"},
         std::tuple{G::SaveFamily::BlackWhite,uint8_t{21},"black_nds"},
         std::tuple{G::SaveFamily::Black2White2,uint8_t{22},"white2_nds"},
         std::tuple{G::SaveFamily::Black2White2,uint8_t{23},"black2_nds"}}) {
        const auto sav=make(family,version);
        // Preserve the existing ONE shared editor shell. Gen V can only
        // advertise audited staged field types, never a native SAV writer.
        namespace Format = PokeVault::Integration::Gen5EditorProvider;
        namespace Exact = PokeBank::UIModel::ExactFormatEditor;
        namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
        const auto* title=PokeVault::Games::findGame(id);
        assert(title && title->support==PokeVault::Games::SourceSupport::Planned);
        auto stagedDescriptor=Format::descriptorForSource(id,true);
        auto readOnlyDescriptor=Format::descriptorForSource(id,false);
        assert(stagedDescriptor && readOnlyDescriptor);
        assert(stagedDescriptor->exact.identity.gameId==id);
        assert(stagedDescriptor->exact.identity.generation==
               PokeBank::UIModel::PokemonEditorFoundation::Generation::Gen5);
        assert(stagedDescriptor->exact.identity.format==
               PokeBank::UIModel::PokemonEditorFoundation::SaveFormat::PK5);
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Nature));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Friendship));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::IV));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::EV));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Species));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Ability));
        assert(stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::HeldItem));
        assert(!stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Shiny));
        assert(!readOnlyDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Nature));
        assert(!stagedDescriptor->source.canWriteOriginalSource());
        assert(!stagedDescriptor->source.saveOperations.supports(
               PokeVault::SaveEdit::Capability::PokemonCreation));
        const Exact::MoveCompatibilityQuery unverifiedMove{id,25,0,1,false};
        assert(stagedDescriptor->moves.evaluate(unverifiedMove)==
               Exact::MoveCompatibilityResult::Unsupported);
        assert(!Format::descriptorForSource("platinum_nds",true));
        std::string error;
        const auto parsed=G::Gen5ReadOnlySave::parse(sav,id,&error);
        assert(parsed && error.empty());
        assert(parsed->family()==family && parsed->exactGameId()==id);
        auto presented=PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed,id,error);
        assert(presented && error.empty());
        assert(presented->getBoxCount()==24 && presented->getSlotsPerBox()==30);
        assert(presented->getPartySize()==1 && presented->party.size()==1);
        assert(presented->party[0] && presented->party[0]->speciesID()==25);
        assert(presented->party[0]->level()==50);
        assert(presented->party[0]->baseHP()==35);
        assert(presented->party[0]->baseATK()==55);
        assert(presented->party[0]->nature()==0);
        assert(presented->party[0]->getData().empty());
        assert(!presented->party[0]->clone());
        assert(presented->party[0]->getGameGroup()==
            (family==G::SaveFamily::BlackWhite?Enums::GameVersion::BW:
                                                Enums::GameVersion::B2W2));
        assert(presented->boxes.size()==24 && presented->boxes[0][0]);
        assert(presented->boxes[0][0]->speciesID()==133);
        assert(presented->boxes[0][0]->level()==1);
        assert(presented->boxes[0][0]->baseHP()==55);
        assert(presented->boxes[0][0]->baseATK()==55);
        assert(presented->boxes[0][0]->baseSPD()==65);
        assert(presented->boxes[0][0]->statHPMax()==0);
        assert(!presented->boxes[0][1]);
        assert(presented->trainerName=="NX" && presented->TID16==12345);
        assert(!presented->hasStagedChanges());
        presented->party[0]->setIV(0,31);
        presented->party[0]->setEV(0,252);
        presented->party[0]->setShiny(true,0);
        assert(!presented->hasStagedChanges());
        assert(presented->stagedPokemon().stageParty(
            0,G::StagedPokemon5Record::Field::Nature,0,9,&error));
        assert(presented->hasStagedChanges());
        assert(presented->refreshStagedPokemonPresentation(error) && error.empty());
        assert(presented->party[0]->nature()==9);
        assert(parsed->partyPokemon(0)->nature()==0);
        presented->stagedPokemon().discardAll();
        assert(presented->refreshStagedPokemonPresentation(error));
        assert(!presented->hasStagedChanges() && presented->party[0]->nature()==0);
        assert(!PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed,
            std::string_view(id)=="black_nds"?"white_nds":"black_nds",error));
        const G::SourceContext context{
            id, "retroarch", "RetroArch", "sdmc:/retroarch/cores/savefiles/test.srm",
            "sdmc:/retroarch/cores/savefiles/test.srm", "physical-save-fixture",
            "fixture-test", "test-profile", 17
        };
        auto probe = G::probeNormalizedBattery(sav,context);
        assert(probe.ready());
        assert(probe.instance.generation==5 && probe.instance.gameId==id);
        assert(probe.instance.providerId=="retroarch" && probe.instance.providerLabel=="RetroArch");
        assert(probe.instance.readOnly() && probe.instance.partyCount==1);
        assert(probe.instance.sourceIndex==17 && probe.instance.claimedProfile=="test-profile");
        assert(probe.instance.sourcePath=="sdmc:/retroarch/cores/savefiles/test.srm");
        assert(probe.instance.trainerName=="NX"); // Verified printable UTF-16 only.
        auto mismatched=context; mismatched.assignedExactGame = "platinum_nds";
        assert(!G::probeNormalizedBattery(sav,mismatched).ready());
        mismatched.assignedExactGame = id;
        mismatched.sourcePath = {};
        assert(!G::probeNormalizedBattery(sav,mismatched).ready());
        auto wrongAssignment=context;
        wrongAssignment.assignedExactGame = family==G::SaveFamily::BlackWhite ?
            "white2_nds" : "white_nds";
        const auto mismatchProbe=G::probeNormalizedBattery(sav,wrongAssignment);
        assert(!mismatchProbe.ready());
        assert(mismatchProbe.instance.validation==PokeVault::Source::ValidationStatus::AssignmentMismatch);
        assert(parsed->partyCount()==1 && parsed->selectedPartition()==0);
        assert(parsed->trainer().rawName==u"NX");
        assert(G::displayTrainerName(parsed->trainer().rawName).value()=="NX");
        assert(G::displayTrainerName(u"\u30CF").value()=="\xE3\x83\x8F");
        assert(!G::displayTrainerName(std::u16string(1,char16_t(0xD800))));
        assert(!G::displayTrainerName(std::u16string(1,char16_t(0xDC00))));
        assert(!G::displayTrainerName(std::u16string(1,char16_t(0xE000))));
        assert(!G::displayTrainerName(u""));
        assert(parsed->trainer().tid==12345 && parsed->trainer().sid==54321);
        assert(parsed->trainer().playedHours==23);
        assert(parsed->partyPokemon(0)->species()==25);
        assert(parsed->boxPokemon(0,0)->species()==133);
        assert(parsed->diagnostics().validPartyRecords==1);
        assert(parsed->diagnostics().occupiedBoxRecords==1);
        assert(parsed->diagnostics().invalidBoxRecords==0);
        G::Gen5StagedPokemonWorkspace workspace(*parsed);
        assert(!workspace.hasChanges() && workspace.changedRecordCount()==0);
        // Shared Clone/Remove actions are separate from Edit and never go
        // through a generic Trainer mutator. Y is required for removal.
        {
            using Slot=G::Gen5StagedPokemonWorkspace::Slot;
            using Region=G::Gen5StagedPokemonWorkspace::Region;
            using Action=PokeBank::UIModel::SharedPokemonEditor::Action;
            G::Gen5StagedPokemonWorkspace ops(*parsed);
            G::Gen5SharedScreenState actionUi;
            assert(actionUi.openActions(ops,Slot{Region::Party,0,0},error));
            const auto available=actionUi.availableActions(ops);
            bool canClone=false;
            for(size_t i=0;i<available.count;++i)
                if(available[i]==Action::Clone)canClone=true;
            assert(canClone);
            assert(actionUi.activate(ops,Action::Clone,error));
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::CloneDestination);
            assert(actionUi.cloneDestinationIndex()==1);
            assert(actionUi.cloneDestinationAllowed(ops));
            assert(!actionUi.confirmClone(ops,[&](std::string& why){
                why="Presentation intentionally failed";
                return false;
            },error));
            assert(!ops.hasChanges() && actionUi.cloneDestinationAllowed(ops));
            assert(actionUi.confirmClone(ops,[&](std::string&){
                return true;
            },error));
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::Browse);
            assert(ops.viewBox(0,1)->species()==25 && ops.changedRecordCount()==1);
            assert(actionUi.openActions(ops,Slot{Region::Box,0,0},error));
            const auto boxActions=actionUi.availableActions(ops);
            bool canRemove=false;
            for(size_t i=0;i<boxActions.count;++i)
                if(boxActions[i]==Action::Remove)canRemove=true;
            assert(canRemove);
            assert(actionUi.activate(ops,Action::Remove,error));
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::ConfirmRemoveBox);
            assert(!actionUi.confirmRemoveBox(ops,[&](std::string& why){
                why="Presentation intentionally failed";
                return false;
            },error));
            assert(!ops.viewBox(0,0)->empty() && ops.changedRecordCount()==1);
            actionUi.cancelBoxAction();
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::Actions);
            assert(actionUi.activate(ops,Action::Remove,error));
            assert(actionUi.confirmRemoveBox(ops,[&](std::string&){
                return true;
            },error));
            assert(ops.viewBox(0,0)->empty() && ops.changedRecordCount()==2);
            const auto output=G::buildStagedSaveCandidate(ops,&error);
            assert(output && output->changedRecords==2);
            assert(std::equal(parsed->sourceBytes().begin(),
                              parsed->sourceBytes().end(),sav.begin()));
            // Existing action screen Y undoes only the selected staged Box
            // transaction. Rejected view refresh must restore exact bytes.
            assert(actionUi.openActions(ops,Slot{Region::Box,0,1},error));
            assert(actionUi.selectedBoxUndoAvailable(ops));
            assert(!actionUi.undoSelectedBoxReplacement(
                ops,[&](std::string& why) {
                    why="Forced presentation failure for Undo regression";
                    return false;
                },error));
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::Actions);
            assert(ops.viewBox(0,1)->species()==25);
            assert(ops.changedRecordCount()==2);
            assert(actionUi.undoSelectedBoxReplacement(
                ops,[&](std::string&){return true;},error));
            assert(actionUi.surface()==G::Gen5SharedScreenState::Surface::Browse);
            assert(ops.viewBox(0,1)->empty() && ops.changedRecordCount()==1);
            assert(actionUi.openActions(ops,Slot{Region::Box,0,0},error));
            assert(actionUi.selectedBoxUndoAvailable(ops));
            assert(actionUi.undoSelectedBoxReplacement(
                ops,[&](std::string&){return true;},error));
            assert(ops.viewBox(0,0)->species()==133 && !ops.hasChanges());
            assert(actionUi.openActions(ops,Slot{Region::Box,0,0},error));
            assert(!actionUi.selectedBoxUndoAvailable(ops));
            assert(!actionUi.undoSelectedBoxReplacement(
                ops,[&](std::string&){return true;},error));
            assert(!ops.hasChanges());
            assert(!actionUi.back());
            ops.discardAll();
            assert(!ops.hasChanges() && ops.viewBox(0,0)->species()==133);
        }
        assert(workspace.stageParty(0,G::StagedPokemon5Record::Field::Nature,0,12,&error));
        assert(workspace.stageBox(0,0,G::StagedPokemon5Record::Field::Friendship,0,255,&error));
        assert(workspace.stageBox(0,0,G::StagedPokemon5Record::Field::IV,5,31,&error));
        assert(workspace.hasChanges() && workspace.changedRecordCount()==2);
        assert(workspace.viewParty(0)->nature()==12);
        assert(workspace.viewBox(0,0)->friendship()==255);
        assert(workspace.viewBox(0,0)->ivs()[5]==31);
        // The validated source save is still byte-identical.
        assert(parsed->partyPokemon(0)->nature()!=12);
        assert(parsed->boxPokemon(0,0)->friendship()!=255);
        assert(std::equal(parsed->sourceBytes().begin(),parsed->sourceBytes().end(),sav.begin()));
        assert(!workspace.stageParty(6,G::StagedPokemon5Record::Field::Nature,0,1,&error));
        assert(!workspace.stageBox(24,0,G::StagedPokemon5Record::Field::Nature,0,1,&error));
        assert(!workspace.stageBox(0,29,G::StagedPokemon5Record::Field::EV,0,300,&error));
        assert(!workspace.stageBox(0,1,G::StagedPokemon5Record::Field::Nature,0,1,&error));
        assert(workspace.changedRecordCount()==2);
        // Full-SAV candidate is produced ONLY in app-owned memory; it must
        // retain all retail block checksums, the exact game and all other bytes.
        const auto exported=G::buildStagedSaveCandidate(workspace,&error);
        assert(exported && error.empty() && exported->changedRecords==2);
        assert(exported->bytes.size()==sav.size());
        assert(exported->selectedCopyOffset==parsed->selectedCopyOffset());
        assert(exported->exactGameId==id);
        assert(exported->bytes!=sav);
        const auto reread=G::Gen5ReadOnlySave::parse(exported->bytes,id,&error,
            G::SaveCopySelection::Primary);
        assert(reread && error.empty() && reread->family()==family);
        assert(reread->partyPokemon(0)->nature()==12);
        assert(reread->boxPokemon(0,0)->friendship()==255);
        assert(reread->boxPokemon(0,0)->ivs()[5]==31);
        assert(reread->partyPokemon(0)->partyLevel()==50);
        assert(reread->trainer().rawName==parsed->trainer().rawName);
        assert(reread->dexProgress().seen==parsed->dexProgress().seen);
        const size_t copySize=family==G::SaveFamily::BlackWhite
            ? L::BlackWhiteCopySize:L::Black2White2CopySize;
        assert(std::equal(exported->bytes.begin()+static_cast<std::ptrdiff_t>(copySize),
                          exported->bytes.end(),
                          sav.begin()+static_cast<std::ptrdiff_t>(copySize)));
        assert(std::equal(parsed->sourceBytes().begin(),
                          parsed->sourceBytes().end(),sav.begin()));
        // A two-copy source whose backup is intentionally different must
        // preserve the PRIMARY unmodified when the owner chose Backup.
        auto dual=make(family,version);
        const auto backupImage=make(family,version,copySize);
        std::copy(backupImage.begin()+static_cast<std::ptrdiff_t>(copySize),
                  backupImage.begin()+static_cast<std::ptrdiff_t>(2*copySize),
                  dual.begin()+static_cast<std::ptrdiff_t>(copySize));
        dual[copySize+0x19400+0x26]=12;
        if(family==G::SaveFamily::BlackWhite)stamp(dual,copySize,L::BlackWhite);
        else stamp(dual,copySize,L::Black2White2);
        assert(!G::Gen5ReadOnlySave::parse(dual,id,&error)); // ambiguous freshness
        const auto selectedBackup=G::Gen5ReadOnlySave::parse(
            dual,id,&error,G::SaveCopySelection::Backup);
        assert(selectedBackup && selectedBackup->selectedBackupPartition());
        G::Gen5StagedPokemonWorkspace backupWorkspace(*selectedBackup);
        assert(backupWorkspace.stageParty(
            0,G::StagedPokemon5Record::Field::Nature,0,18,&error));
        const auto backupExport=G::buildStagedSaveCandidate(backupWorkspace,&error);
        assert(backupExport && backupExport->changedRecords==1);
        assert(backupExport->selectedCopyOffset==copySize);
        assert(std::equal(dual.begin(),
                          dual.begin()+static_cast<std::ptrdiff_t>(copySize),
                          backupExport->bytes.begin()));
        assert(std::equal(dual.begin()+static_cast<std::ptrdiff_t>(2*copySize),
                          dual.end(),
                          backupExport->bytes.begin()+static_cast<std::ptrdiff_t>(2*copySize)));
        const auto backupParsed=G::Gen5ReadOnlySave::parse(backupExport->bytes,id,
            &error,G::SaveCopySelection::Backup);
        assert(backupParsed && backupParsed->selectedBackupPartition());
        assert(backupParsed->partyPokemon(0)->nature()==18);
        assert(G::Gen5ReadOnlySave::parse(backupExport->bytes,id,&error,
            G::SaveCopySelection::Primary)->partyPokemon(0)->nature()!=18);
        assert(std::equal(dual.begin(),dual.end(),
                          selectedBackup->sourceBytes().begin()));
        // No edits means NO candidate, avoiding a misleading exported SAV.
        backupWorkspace.discardAll();
        assert(!G::buildStagedSaveCandidate(backupWorkspace,&error));
        assert(!error.empty());
        const auto pending=workspace.pendingReview();
        assert(pending.size()==2);
        for(const auto& change:pending) {
            assert(change.before.size()==change.after.size());
            assert(change.before!=change.after);
            assert(change.before.size()==(change.location.region==
                G::Gen5StagedPokemonWorkspace::Region::Party?
                C::PartySize:C::StoredSize));
            const auto verified=G::verifyStagedReview(change,&error);
            assert(verified && !verified->fields.empty());
            assert(verified->fields.size()==
                (change.location.region==G::Gen5StagedPokemonWorkspace::Region::Party?1u:2u));
            for(const auto& field:verified->fields)assert(field.before!=field.after);
            // A structurally valid but unrelated species edit is not a
            // permitted four-field PK5 stage, and cannot be misrepresented
            // as a verified review of our supported mutations.
            auto forged=change;
            auto plaintext=C::decrypt(forged.after);
            C::write16(plaintext,8,150); // Mewtwo: valid Gen V dex ID.
            forged.after=C::encryptCandidate(plaintext);
            assert(!G::verifyStagedReview(forged,&error));
            forged=change;
            forged.location.region=static_cast<G::Gen5StagedPokemonWorkspace::Region>(0xFF);
            assert(!G::verifyStagedReview(forged,&error));
        }
        // One shared action menu and exact Gen V field capabilities.
        using GA=PokeBank::UIModel::SharedPokemonEditor::Action;
        using GF=PokeBank::UIModel::SharedPokemonEditor::FieldIdentity;
        using GAccess=PokeBank::UIModel::SharedPokemonEditor::FieldAccess;
        const auto nativeParty=G::selectedSlot(workspace,
            {G::Gen5StagedPokemonWorkspace::Region::Party,0,0});
        assert(nativeParty.validated && nativeParty.occupied);
        const auto nativeActions=G::actions(workspace,nativeParty);
        assert(nativeActions.count==4);
        assert(nativeActions[0]==GA::View && nativeActions[1]==GA::Edit);
        assert(nativeActions[2]==GA::Review && nativeActions[3]==GA::Close);
        // A stale/forged slot flag must never surface the Edit action on
        // an actually empty native box cell, even with pending changes.
        auto staleSlot=nativeParty;
        staleSlot.location={G::Gen5StagedPokemonWorkspace::Region::Box,0,1};
        const auto staleActions=G::actions(workspace,staleSlot);
        // Empty verified cells use the very same shared Add/Review/Cancel
        // menu as Gen I-IV, not an invented Gen V-only Create UI.
        assert(staleActions.count==3 && staleActions[0]==GA::Add &&
               staleActions[1]==GA::Review && staleActions[2]==GA::Close);
        // No Add permission for unverified/occupied/party slots. The
        // existing shared action model never receives a write handle.
        G::Gen5StagedPokemonWorkspace emptyActions(*parsed);
        G::Gen5SharedScreenState addMenu;
        assert(addMenu.openActions(emptyActions,
            {G::Gen5StagedPokemonWorkspace::Region::Box,0,1},error));
        assert(addMenu.availableActions(emptyActions)[0]==GA::Add);
        assert(addMenu.activate(emptyActions,GA::Add,error));
        assert(addMenu.surface()==G::Gen5SharedScreenState::Surface::ConfirmCreateBox);
        G::CreateRequest5 partialCreate;
        partialCreate.game=static_cast<G::CreateGame5>(
            parsed->trainer().gameVersion);
        partialCreate.species=25;
        partialCreate.nickname="LOCALONLY";
        assert(addMenu.rememberCreateInput(emptyActions,partialCreate));
        assert(addMenu.pendingCreateInput() &&
               addMenu.pendingCreateInput()->nickname=="LOCALONLY");
        assert(!addMenu.back()); // Cancel never changes the staged workspace.
        assert(!addMenu.pendingCreateInput()); // No stale personal fields.
        assert(addMenu.surface()==G::Gen5SharedScreenState::Surface::Actions);
        assert(!emptyActions.hasChanges());
        assert(!addMenu.activate(emptyActions,GA::Remove,error));
        assert(!emptyActions.hasChanges());
        assert(!addMenu.back());
        assert(addMenu.surface()==G::Gen5SharedScreenState::Surface::Browse);
        assert(addMenu.openActions(emptyActions,
            {G::Gen5StagedPokemonWorkspace::Region::Party,0,0},error));
        const auto partyActions=addMenu.availableActions(emptyActions);
        for(size_t i=0;i<partyActions.count;++i)
            assert(partyActions[i]!=GA::Add);
        assert(!addMenu.activate(emptyActions,GA::Add,error));
        assert(!emptyActions.hasChanges());
        assert(!addMenu.back());
        assert(G::fieldAccess(GF::IV)==GAccess::Editable);
        assert(G::fieldAccess(GF::Nature)==GAccess::Editable);
        assert(G::fieldAccess(GF::HeldItem)==GAccess::Editable);
        assert(G::fieldAccess(GF::Ball)==GAccess::Editable);
        assert(G::fieldAccess(GF::MetLevel)==GAccess::Editable);
        assert(G::fieldAccess(GF::Level)==GAccess::Editable);
        assert(G::fieldAccess(GF::Experience)==GAccess::Editable);
        assert(G::fieldAccess(GF::Species)==GAccess::Editable);
        assert(G::fieldAccess(GF::Nickname)==GAccess::Editable);
        assert(G::fieldAccess(GF::OriginalTrainer)==GAccess::Editable);
        assert(G::fieldAccess(GF::Ability)==GAccess::Editable);
        assert(G::fieldAccess(GF::Form)==GAccess::Editable);
        assert(G::fieldAccess(GF::Gender)==GAccess::Editable);
        assert(G::fieldAccess(GF::Species)==GAccess::Editable);
        assert(G::fieldAccess(GF::Move)==GAccess::Editable);
        assert(G::fieldAccess(GF::MovePP)==GAccess::Editable);
        assert(G::fieldAccess(GF::MovePPUps)==GAccess::Editable);
        static_assert(G::Gen5SharedScreenState::kEditableRows.size()==43);
        assert(G::fieldAccess(GF::TrainerId)==GAccess::Editable);
        assert(G::fieldAccess(GF::SecretId)==GAccess::Editable);
        assert(G::fieldAccess(GF::OriginalTrainerGender)==GAccess::Editable);
        assert(G::fieldAccess(GF::Pokerus)==GAccess::Editable);
        assert(G::fieldAccess(GF::MetLocation)==GAccess::Editable);
        assert(G::fieldAccess(GF::FatefulEncounter)==GAccess::Editable);
        assert(G::exactDescriptor("black_nds",true));
        assert(!G::exactDescriptor("platinum_nds",true));
        G::Gen5SharedPokemonSession unsupported;
        assert(!G::openSharedDraft(unsupported,workspace,nativeParty,GA::Add,error));
        assert(!G::openSharedDraft(unsupported,workspace,staleSlot,GA::Edit,error));
        assert(unsupported.mode()==G::Gen5SharedPokemonSession::Mode::None);
        assert(G::openSharedDraft(unsupported,workspace,nativeParty,GA::View,error));
        assert(unsupported.back());
        // Same Gen I-IV shared exit decision model: local Edit changes are
        // NOT committed to the staged workspace until explicit Keep.
        G::Gen5SharedPokemonSession viewer;
        using GSlot=G::Gen5StagedPokemonWorkspace::Slot;
        using GRegion=G::Gen5StagedPokemonWorkspace::Region;
        using GMode=G::Gen5SharedPokemonSession::Mode;
        using GField=G::StagedPokemon5Record::Field;
        // Per-Trainer shared UI: this is a transient app-owned draft only.
        auto localUiWorkspace=workspace;
        G::Gen5SharedScreenState ui;
        assert(!ui.openActions(localUiWorkspace,GSlot{GRegion::Party,1,0},error));
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Actions);
        assert(ui.activateSelected(localUiWorkspace,error)); // View
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::View);
        assert(!ui.adjustField(1,error)); // View is immutable.
        assert(ui.back());
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Browse);
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        ui.moveAction(1,localUiWorkspace); // Edit
        assert(ui.activateSelected(localUiWorkspace,error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        const auto firstNature=ui.draft().current()->nature();
        assert(ui.adjustField(1,error));
        assert(ui.draft().current()->nature()==firstNature+1);
        assert(!ui.back()); // Cannot lose a dirty draft with B.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::ConfirmDraft);
        assert(!ui.back()); // B continues editing, preserving draft.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.keep(localUiWorkspace,error));
        assert(localUiWorkspace.viewParty(0)->nature()==firstNature+1);
        assert(localUiWorkspace.hasChanges());
        // Independent second draft exercises the additional PK5 field
        // transactions through the real shared-session Keep boundary.
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        ui.moveAction(1,localUiWorkspace); // Edit
        assert(ui.activateSelected(localUiWorkspace,error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        const auto& rows=G::Gen5SharedScreenState::kEditableRows;
        assert(rows[38].field==GF::Level &&
               rows[39].field==GF::Experience &&
               rows[40].field==GF::Nickname &&
               rows[41].field==GF::OriginalTrainer &&
               rows[42].field==GF::Species);
        assert(rows[2].field==GF::HeldItem && rows[3].field==GF::Ball);
        assert(rows[4].field==GF::MetLevel);
        static_assert(G::Gen5SharedScreenState::natureName(0)=="Hardy");
        static_assert(G::Gen5SharedScreenState::natureName(12)=="Serious");
        static_assert(G::Gen5SharedScreenState::natureName(24)=="Quirky");
        const auto sourceNature=ui.draft().current()->nature();
        assert(ui.openNaturePicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::NaturePicker);
        assert(ui.natureSelection()==sourceNature);
        ui.moveNatureCursor(30); // Bounded 24, never wraps.
        assert(ui.natureSelection()==24);
        assert(!ui.back()); // Cancel is lossless and keeps Edit cursor.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.draft().current()->nature()==sourceNature);
        assert(ui.openNaturePicker(error));
        ui.moveNatureCursor(12-static_cast<int>(sourceNature));
        assert(ui.natureSelection()==12);
        assert(ui.acceptNaturePicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.draft().current()->nature()==12);
        ui.moveField(2);
        assert(ui.adjustField(1,error));
        ui.moveField(1);
        assert(ui.adjustField(1,error));
        ui.moveField(1);
        assert(ui.adjustField(1,error));
        assert(ui.keep(localUiWorkspace,error));
        assert(localUiWorkspace.viewParty(0)->heldItem()==
               workspace.viewParty(0)->heldItem()+1);
        assert(localUiWorkspace.viewParty(0)->ball()==
               workspace.viewParty(0)->ball()+1);
        assert(localUiWorkspace.viewParty(0)->metLevel()==
               workspace.viewParty(0)->metLevel()+1);
        assert(localUiWorkspace.hasChanges());
        // A source-backed Move Picker belongs to the SAME draft/editor.
        // A cancelled picker cannot accidentally Keep or mutate the stage.
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        ui.moveAction(1,localUiWorkspace);
        assert(ui.activateSelected(localUiWorkspace,error));
        ui.moveField(11);
        assert(G::Gen5SharedScreenState::kEditableRows[11].field==GF::Move);
        assert(ui.openMovePicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::MovePicker);
        ui.movePickerCursor(559);
        assert(ui.movePickerSelection()==559);
        assert(!ui.back());
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.openMovePicker(error));
        ui.movePickerCursor(559);
        assert(ui.acceptMovePicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.draft().current()->moves()[0]==559);
        assert(ui.draft().current()->pp()[0]==5);
        assert(ui.draft().current()->ppUps()[0]==0);
        ui.moveField(4); // PP Ups 1
        assert(ui.adjustField(1,error));
        assert(ui.draft().current()->pp()[0]==6);
        ui.moveField(4); // current PP 1
        assert(ui.adjustField(-1,error));
        assert(ui.draft().current()->pp()[0]==5);
        assert(ui.keep(localUiWorkspace,error));
        assert(localUiWorkspace.viewParty(0)->moves()[0]==559);
        assert(localUiWorkspace.viewParty(0)->ppUps()[0]==1);
        assert(localUiWorkspace.viewParty(0)->pp()[0]==5);
        const auto moveReview=localUiWorkspace.pendingReview();
        assert(!moveReview.empty());
        const auto verifiedMoves=G::verifyStagedReview(moveReview.front(),&error);
        assert(verifiedMoves && error.empty());
        bool sawMove=false,sawPPUps=false,sawPP=false;
        for(const auto& f:verifiedMoves->fields) {
            if(f.field==GField::Move && f.stat==0)sawMove=true;
            if(f.field==GField::MovePPUps && f.stat==0)sawPPUps=true;
            if(f.field==GField::MovePP && f.stat==0)sawPP=true;
        }
        assert(sawMove && sawPPUps && sawPP);
        // Exercise all six new identity/encounter fields in one atomic Keep:
        // no mutable source access, no implicit trainer/name/PID coupling.
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        ui.moveAction(1,localUiWorkspace);
        assert(ui.activateSelected(localUiWorkspace,error));
        ui.moveField(5); // Trainer ID, after original met level.
        assert(G::Gen5SharedScreenState::kEditableRows[5].field==GF::TrainerId);
        assert(!ui.stageFieldValue(65536,error));
        assert(ui.stageFieldValue(1111,error));
        ui.moveField(1); // Secret ID
        assert(ui.stageFieldValue(2222,error));
        ui.moveField(1); // original trainer gender
        assert(ui.adjustField(1,error));
        ui.moveField(1); // Pokérus None -> Cured, never a raw byte editor
        assert(ui.adjustField(1,error));
        ui.moveField(1); // met location
        assert(ui.openLocationPicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::LocationPicker);
        ui.moveLocationCursor(1);
        assert(ui.back()==false); // picker cancel preserves original
        assert(ui.openLocationPicker(error));
        ui.moveLocationCursor(1);
        assert(ui.acceptLocationPicker(error));
        assert(ui.draft().current()->metLocation()==1);
        ui.moveField(1); // fateful bit in same byte as gender/form
        assert(ui.adjustField(1,error));
        assert(ui.keep(localUiWorkspace,error));
        const auto identity=localUiWorkspace.viewParty(0);
        assert(identity && identity->tid()==1111 && identity->sid()==2222);
        assert(identity->originalTrainerGender()==1);
        assert(identity->pokerusMode()==1 && identity->pokerusState()==0x10);
        assert(identity->metLocation()==1 && identity->fatefulEncounter());
        assert(identity->species()==25 && identity->pid()==workspace.viewParty(0)->pid());
        assert(identity->ability()==workspace.viewParty(0)->ability());
        assert(identity->moves()[0]==559);
        bool sawIdentity=false,sawLocation=false,sawFateful=false;
        for(const auto& pending:localUiWorkspace.pendingReview()) {
            const auto verified=G::verifyStagedReview(pending,&error);
            assert(verified && error.empty());
            for(const auto& f:verified->fields) {
                if(f.field==GField::TrainerId || f.field==GField::SecretId)
                    sawIdentity=true;
                if(f.field==GField::MetLocation)sawLocation=true;
                if(f.field==GField::FatefulEncounter)sawFateful=true;
            }
        }
        assert(sawIdentity && sawLocation && sawFateful);
        // Exact BW identity picker is part of the same staged editor;
        // cancelled choices cannot mutate the current local draft.
        assert(ui.openActions(localUiWorkspace,GSlot{GRegion::Party,0,0},error));
        ui.moveAction(1,localUiWorkspace);
        assert(ui.activateSelected(localUiWorkspace,error));
        ui.moveField(35);
        assert(rows[35].field==GF::Ability &&
               rows[36].field==GF::Form && rows[37].field==GF::Gender);
        assert(ui.openIdentityPicker(error));
        assert(ui.identityChoiceCount()==3);
        ui.moveIdentityCursor(2);
        assert(ui.identitySelection()==2);
        assert(!ui.back()); // Cancels without changing hidden ability.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(!ui.draft().current()->hiddenAbility());
        assert(ui.openIdentityPicker(error));
        ui.moveIdentityCursor(2);
        assert(ui.acceptIdentityPicker(error));
        assert(ui.draft().current()->hiddenAbility());
        assert(ui.draft().current()->ability()==31);
        ui.moveField(1);
        assert(ui.openIdentityPicker(error));
        assert(ui.identityChoiceCount()==1); // Pikachu has only base form.
        assert(!ui.identityChoiceAllowed(1));
        assert(!ui.back());
        ui.moveField(1);
        assert(ui.openIdentityPicker(error));
        assert(ui.identityChoiceCount()==3);
        assert(!ui.identityChoiceAllowed(2)); // Pikachu isn't genderless.
        ui.moveIdentityCursor(1);
        assert(ui.acceptIdentityPicker(error));
        assert(ui.draft().current()->gender()==1);
        ui.moveField(3); // Nickname follows Gender/Level/EXP.
        assert(rows[40].field==GF::Nickname);
        assert(!ui.stageNameValue("INVALID NAME LONG",error));
        assert(ui.stageNameValue("NXPIKA",error));
        ui.moveField(1);
        assert(rows[41].field==GF::OriginalTrainer);
        assert(ui.stageNameValue("NXT",error));
        ui.moveField(1);
        assert(rows[42].field==GF::Species);
        // Exact-native named picker preserves the local draft until A.
        assert(ui.openSpeciesPicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::SpeciesPicker);
        assert(ui.speciesChoiceCount()==649);
        assert(ui.chosenSpeciesId()==25);
        assert(ui.speciesChoiceAllowed(26));
        assert(!ui.speciesChoiceAllowed(1)); // Different growth group.
        assert(!ui.speciesChoiceAllowed(650));
        ui.moveSpeciesCursor(1);
        assert(ui.chosenSpeciesId()==26);
        ui.cancelSpeciesPicker();
        assert(ui.draft().current()->species()==25); // No mutation on B.
        assert(ui.openSpeciesPicker(error));
        ui.filterSpecies("rAi",[](uint16_t id)->const char* {
            return id==26?"Raichu":"Other";
        });
        assert(ui.speciesSearch()=="rAi");
        assert(ui.speciesChoiceCount()==1);
        assert(ui.chosenSpeciesId()==26);
        ui.filterSpecies("Not a Pokemon",[](uint16_t)->const char* {
            return "Other";
        });
        assert(ui.speciesChoiceCount()==0);
        assert(!ui.acceptSpeciesPicker(error)); // Never choose an absent match.
        assert(ui.draft().current()->species()==25);
        ui.filterSpecies("rAi",[](uint16_t id)->const char* {
            return id==26?"Raichu":"Other";
        });
        assert(ui.speciesChoiceCount()==1);
        assert(ui.acceptSpeciesPicker(error));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Edit);
        assert(ui.draft().current()->species()==26);
        assert(!ui.stageFieldValue(1,error)); // Growth differs: fail closed.
        assert(ui.keep(localUiWorkspace,error));
        assert(localUiWorkspace.viewParty(0)->hiddenAbility());
        assert(localUiWorkspace.viewParty(0)->gender()==1);
        assert(localUiWorkspace.viewParty(0)->species()==26);
        assert(G::asciiName5(localUiWorkspace.viewParty(0)->decodedBytes(),true)=="NXPIKA");
        assert(G::asciiName5(localUiWorkspace.viewParty(0)->decodedBytes(),false)=="NXT");
        const auto identityChanges=localUiWorkspace.pendingReview();
        assert(!identityChanges.empty());
        assert(G::verifyStagedReview(identityChanges.front(),&error));
        assert(error.empty());
        assert(!ui.requestExit(localUiWorkspace));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::ConfirmExit);
        ui.reviewFromExit(localUiWorkspace);
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Review);
        // Export is explicit and confirms separately from both Review and
        // Discard All. It never silently drops in-memory staged records.
        assert(ui.requestExport(localUiWorkspace));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::ConfirmExport);
        assert(!ui.requestExport(localUiWorkspace));
        assert(!ui.back()); // B cancels export; edits survive.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Review);
        assert(localUiWorkspace.hasChanges());
        assert(ui.requestExport(localUiWorkspace));
        ui.finishExport(); // Simulate a verified completed app-owned export.
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Review);
        assert(localUiWorkspace.hasChanges());
        ui.requestDiscardAll();
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::ConfirmDiscardAll);
        assert(!ui.back()); // Cancel never discards staged changes.
        assert(localUiWorkspace.hasChanges());
        ui.requestDiscardAll();
        assert(ui.confirmDiscardAll(localUiWorkspace));
        assert(!localUiWorkspace.hasChanges() && ui.requestExit(localUiWorkspace));
        auto exitWorkspace=workspace;
        G::Gen5SharedScreenState exitOnly;
        assert(!exitOnly.requestExit(exitWorkspace));
        assert(exitOnly.surface()==G::Gen5SharedScreenState::Surface::ConfirmExit);
        assert(!exitOnly.back()); // B keeps the app-memory workspace intact.
        assert(exitWorkspace.hasChanges());
        assert(!exitOnly.requestExit(exitWorkspace));
        assert(exitOnly.confirmDiscardAndExit(exitWorkspace)); // Y explicit exit path.
        assert(!exitWorkspace.hasChanges());
        assert(workspace.hasChanges()); // Separate editor/session unchanged.
        // Two-phase Keep must survive a failed native presentation refresh.
        // In particular A from the dirty-draft confirmation MUST be a Keep,
        // never a dead button or a silent draft-discard operation.
        auto transactionWorkspace=workspace;
        G::Gen5SharedScreenState transactionUi;
        assert(transactionUi.openActions(transactionWorkspace,
            GSlot{GRegion::Party,0,0},error));
        assert(transactionUi.activate(transactionWorkspace,GA::Edit,error));
        const auto transactionNature=transactionUi.draft().current()->nature();
        assert(transactionUi.adjustField(1,error));
        assert(!transactionUi.back());
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::ConfirmDraft);
        const auto untouched=transactionWorkspace.viewParty(0)->nature();
        assert(!transactionUi.keepWithPresentation(transactionWorkspace,
            [](std::string& why) {
                why="Simulated Gen V presentation refresh failure";
                return false;
            },error));
        assert(error=="Simulated Gen V presentation refresh failure");
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::ConfirmDraft);
        assert(transactionUi.draft().dirty() && transactionUi.draft().editable());
        assert(transactionUi.draft().current()->nature()==transactionNature+1);
        assert(transactionWorkspace.viewParty(0)->nature()==untouched);
        assert(transactionUi.keepWithPresentation(transactionWorkspace,
            [](std::string&) {return true;},error));
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::Browse);
        assert(transactionWorkspace.viewParty(0)->nature()==transactionNature+1);
        assert(transactionUi.openActions(transactionWorkspace,
            GSlot{GRegion::Party,0,0},error));
        assert(transactionUi.activate(transactionWorkspace,GA::Review,error));
        transactionUi.requestDiscardAll();
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::ConfirmDiscardAll);
        const auto preDiscard=transactionWorkspace.changedRecordCount();
        assert(!transactionUi.discardAllWithPresentation(transactionWorkspace,
            [](std::string& why) {
                why="Simulated discard presentation refresh failure";
                return false;
            },error));
        assert(error=="Simulated discard presentation refresh failure");
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::ConfirmDiscardAll);
        assert(transactionWorkspace.changedRecordCount()==preDiscard);
        assert(transactionUi.discardAllWithPresentation(transactionWorkspace,
            [](std::string&) {return true;},error));
        assert(transactionUi.surface()==G::Gen5SharedScreenState::Surface::Browse);
        assert(!transactionWorkspace.hasChanges());
        assert(workspace.hasChanges());
        assert(std::equal(parsed->sourceBytes().begin(),parsed->sourceBytes().end(),sav.begin()));
        const GSlot forgedRegion{static_cast<GRegion>(0xFF),0,0};
        const GSlot forgedPartyBox{GRegion::Party,1,0};
        assert(!G::Gen5StagedPokemonWorkspace::canonicalSlot(forgedRegion));
        assert(!G::Gen5StagedPokemonWorkspace::canonicalSlot(forgedPartyBox));
        assert(!G::selectedSlot(workspace,forgedRegion).validated);
        assert(!G::selectedSlot(workspace,forgedPartyBox).validated);
        assert(!viewer.begin(workspace,forgedRegion,GMode::Edit,error));
        assert(!viewer.begin(workspace,forgedPartyBox,GMode::View,error));
        assert(viewer.mode()==GMode::None);
        assert(viewer.begin(workspace,GSlot{GRegion::Party,0,0},GMode::View,error));
        assert(!viewer.stage(GField::Nature,0,15,error));
        assert(!G::stageSharedField(viewer,GF::Nature,0,15,error));
        assert(viewer.back() && viewer.mode()==GMode::None);
        assert(!viewer.begin(workspace,GSlot{GRegion::Box,24,0},GMode::Edit,error));
        G::Gen5SharedPokemonSession editor;
        assert(editor.begin(workspace,GSlot{GRegion::Party,0,0},GMode::Edit,error));
        assert(!G::stageSharedField(editor,GF::Species,0,133,error));
        assert(!G::stageSharedField(editor,GF::IV,0,32,error));
        // Unknown enum discriminants are forbidden at BOTH the shared
        // adapter and native PK5 record transaction boundary.
        const auto forbiddenField=static_cast<GField>(0xFF);
        const auto pristineNature=editor.current()->nature();
        assert(!editor.stage(forbiddenField,0,17,error));
        assert(editor.current()->nature()==pristineNature);
        assert(!G::stageSharedField(editor,static_cast<GF>(0xFF),0,17,error));
        assert(editor.current()->nature()==pristineNature);
        assert(G::stageSharedField(editor,GF::Nature,0,10,error));
        assert(editor.current()->nature()==10);
        // An attempted second Open cannot silently destroy a dirty draft.
        assert(!editor.begin(workspace,GSlot{GRegion::Box,0,0},GMode::Edit,error));
        assert(editor.mode()==GMode::Edit && editor.dirty());
        assert(editor.current()->nature()==10);
        assert(workspace.viewParty(0)->nature()==12);
        assert(!editor.back() && editor.confirmExit());
        assert(workspace.viewParty(0)->nature()==12);
        editor.continueEditing();
        assert(!editor.confirmExit());
        assert(editor.keep(workspace,error));
        assert(editor.mode()==GMode::None && workspace.viewParty(0)->nature()==10);
        assert(workspace.changedRecordCount()==2);
        // Local cancel never discards earlier accepted workspace edits.
        assert(editor.begin(workspace,GSlot{GRegion::Box,0,0},GMode::Edit,error));
        // Exercise all four native staged-field bridges in one local draft.
        assert(G::stageSharedField(editor,GF::Friendship,0,87,error));
        assert(G::stageSharedField(editor,GF::IV,0,31,error));
        assert(G::stageSharedField(editor,GF::EV,1,10,error));
        assert(editor.current()->friendship()==87);
        assert(editor.current()->ivs()[0]==31);
        assert(editor.current()->evs()[1]==10);
        assert(editor.dirty());
        editor.discardDraft();
        assert(workspace.viewBox(0,0)->friendship()==255);
        assert(workspace.viewBox(0,0)->evs()[1]!=10);
        // Concurrent edits are not overwritten with a stale draft.
        assert(editor.begin(workspace,GSlot{GRegion::Party,0,0},GMode::Edit,error));
        assert(editor.stage(GField::Nature,0,13,error));
        assert(workspace.stageParty(0,GField::Nature,0,14,&error));
        assert(!editor.keep(workspace,error));
        assert(workspace.viewParty(0)->nature()==14);
        editor.discardDraft();
        // A new box draft can be independently kept.
        assert(editor.begin(workspace,GSlot{GRegion::Box,0,0},GMode::Edit,error));
        assert(editor.stage(GField::Friendship,0,89,error));
        assert(editor.keep(workspace,error));
        assert(workspace.viewBox(0,0)->friendship()==89);
        assert(workspace.viewBox(0,0)->ivs()[5]==31);
        workspace.discardAll();
        assert(!workspace.hasChanges());
        assert(workspace.viewParty(0)->nature()==parsed->partyPokemon(0)->nature());
        assert(workspace.viewBox(0,0)->friendship()==parsed->boxPokemon(0,0)->friendship());

        // Sign the outer block after breaking an encrypted PK5. A good SAV
        // checksum must not make the damaged entity semantically valid.
        auto nestedDamage=sav;
        nestedDamage[L::BoxOffset+20]^=0x40;
        if(family==G::SaveFamily::BlackWhite)stamp(nestedDamage,0,L::BlackWhite);
        else stamp(nestedDamage,0,L::Black2White2);
        const auto nested=G::Gen5ReadOnlySave::parse(nestedDamage,id,&error);
        assert(nested);
        assert(nested->diagnostics().invalidBoxRecords==1);
        assert(nested->diagnostics().occupiedBoxRecords==0);
        assert(!nested->boxPokemon(0,0)->valid());
        assert(nested->boxPokemon(0,0)->species()==0);
        const auto damagedProbe=G::probeNormalizedBattery(nestedDamage,context);
        assert(damagedProbe.ready());
        assert(damagedProbe.instance.diagnostic.find("quarantined")!=std::string::npos);
        // A declared damaged party member remains a hard read failure.
        auto nestedPartyDamage=sav;
        nestedPartyDamage[L::PartyOffset+8+20]^=0x40;
        if(family==G::SaveFamily::BlackWhite)stamp(nestedPartyDamage,0,L::BlackWhite);
        else stamp(nestedPartyDamage,0,L::Black2White2);
        assert(!G::Gen5ReadOnlySave::parse(nestedPartyDamage,id,&error));
        assert(error.find("invalid or empty PK5")!=std::string::npos);
        assert(parsed->dexProgress().caught==1 && parsed->dexProgress().seen==1);
        assert(!parsed->partyPokemon(6) && !parsed->boxPokemon(24,0));
        assert(!G::Gen5ReadOnlySave::parse(sav,"wrong_nds",&error));
        assert(error.find("identity")!=std::string::npos);
        auto corrupt=sav; corrupt[L::BoxOffset+1]^=1;
        assert(!G::Gen5ReadOnlySave::parse(corrupt,id,&error));
        assert(error.find("checksum")!=std::string::npos);
        // A single bad game-data CRC reports the exact verified fraction
        // without changing fail-closed semantics or inferred source identity.
        const std::string copyDiagnostic=family==G::SaveFamily::BlackWhite
            ? "unverified BW primary 69/70 blocks"
            : "unverified B2W2 primary 73/74 blocks";
        assert(error.find(copyDiagnostic)!=std::string::npos);
        assert(!G::Gen5ReadOnlySave::parse(corrupt,id));
        auto wrongFamily=make(family,static_cast<uint8_t>(
            family==G::SaveFamily::BlackWhite ? 22 : 20));
        assert(!G::Gen5ReadOnlySave::parse(wrongFamily,{},&error));
        assert(error.find("disagrees")!=std::string::npos);
        // Valid backup-only copy, primary is invalid.
        const auto backup=make(family,version,copySize);
        const auto selected=G::Gen5ReadOnlySave::parse(backup,id,&error);
        assert(selected && selected->selectedBackupPartition());
        assert(selected->selectedCopyOffset()==copySize);
        auto backupProbe=G::probeNormalizedBattery(backup,context);
        assert(backupProbe.ready() && backupProbe.save->selectedBackupPartition());
        assert(!G::Gen5ReadOnlySave::parse(backup,id,&error,G::SaveCopySelection::Primary));
        assert(error.find("requested save copy")!=std::string::npos);
        // Two different valid copies must fail closed until recency is sourced.
        auto both=sav;
        auto different=backup;
        different[copySize+0x19400+6]='Y'; // valid but newer/older unknown
        if(family==G::SaveFamily::BlackWhite)stamp(different,copySize,L::BlackWhite);
        else stamp(different,copySize,L::Black2White2);
        std::copy(different.begin()+static_cast<std::ptrdiff_t>(copySize),
                  different.end(),both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(!G::Gen5ReadOnlySave::parse(both,id,&error));
        assert(error.find("select one explicitly")!=std::string::npos);
        const auto ambiguousProbe=G::probeNormalizedBattery(both,context);
        assert(!ambiguousProbe.ready());
        assert(ambiguousProbe.instance.validation==PokeVault::Source::ValidationStatus::Unsupported);
        const auto primary = G::Gen5ReadOnlySave::parse(
            both,id,&error,G::SaveCopySelection::Primary);
        const auto secondary = G::Gen5ReadOnlySave::parse(
            both,id,&error,G::SaveCopySelection::Backup);
        assert(primary && secondary);
        assert(primary->selectedCopyOffset()==0 && secondary->selectedCopyOffset()==copySize);
        assert(primary->trainer().rawName==u"NX" && secondary->trainer().rawName==u"NY");
        // Identical valid copies are deterministic and harmless.
        std::copy(sav.begin(),sav.begin()+static_cast<std::ptrdiff_t>(copySize),
                  both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(G::Gen5ReadOnlySave::parse(both,id,&error)->selectedPartition()==0);
        // Gen IV-sized 0x40000 "backup" must not be mistaken for a Gen V copy.
        auto wrongOffset=make(family,version,0x40000);
        assert(!G::Gen5ReadOnlySave::parse(wrongOffset,id,&error));
        // Exact title discrepancy between valid copies must fail closed.
        auto mismatch=make(family,static_cast<uint8_t>(version ^ 1),copySize);
        std::copy(mismatch.begin()+static_cast<std::ptrdiff_t>(copySize),
                  mismatch.begin()+static_cast<std::ptrdiff_t>(2*copySize),
                  both.begin()+static_cast<std::ptrdiff_t>(copySize));
        assert(!G::Gen5ReadOnlySave::parse(both,id,&error,G::SaveCopySelection::Primary));
        assert(error.find("disagree on exact game identity")!=std::string::npos);
    }
    std::vector<uint8_t> empty(L::FullSaveSize,0);
    std::string emptyError;
    assert(!G::Gen5ReadOnlySave::parse(empty,{},&emptyError));
    assert(emptyError.find("unverified") == std::string::npos); // no invented format hint
    assert(!G::Gen5ReadOnlySave::parse(std::span<const uint8_t>(empty.data(),0x40000)));
    rejectedRetroArchGen5DiagnosticContracts();
    extendedDsvContainerContracts();
    discoveryContracts();
    fourTitleProfileAssignmentContracts();
    std::cout<<"Gen V strict BW/B2W2 save reader synthetic contracts PASS\n";
}
