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
    const auto first=G::inspectSourceFile(
        black,"RetroArch",{},G::SaveCopySelection::Primary);
    const auto second=G::inspectSourceFile(
        black,"RetroArch",{},G::SaveCopySelection::Backup);
    assert(first.ready() && second.ready());
    assert(first.gameId==second.gameId && first.gameId=="black_nds");
    assert(first.sourceLabel.find("Primary copy")!=std::string::npos);
    assert(second.sourceLabel.find("Backup copy")!=std::string::npos);
    const auto openedPrimary=G::reopenValidatedSource(first,G::SaveCopySelection::Primary);
    const auto openedBackup=G::reopenValidatedSource(second,G::SaveCopySelection::Backup);
    assert(openedPrimary.ready() && openedBackup.ready());
    assert(openedPrimary.save->trainer().rawName==u"NX");
    assert(openedBackup.save->trainer().rawName==u"NZ");
    assert(!G::reopenValidatedSource(first).ready()); // No automatic guessing.
    assert(!G::reopenValidatedSource(second,G::SaveCopySelection::Primary).ready());
    assert(!G::reopenValidatedSource(first,G::SaveCopySelection::Backup).ready());
    // A DSV with a malformed padding/version/footer is unsupported.
    auto malformed=dsvFixture(whiteRaw);
    malformed[whiteRaw.size()+7]=1;
    fileFixture(dsv,malformed);
    assert(!G::inspectSourceFile(dsv,"DraStic").ready());

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
        assert(!stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Species));
        assert(!stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::Ability));
        assert(!stagedDescriptor->fieldIsEditorTarget(Shared::FieldIdentity::HeldItem));
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
        assert(presented->party[0]->nature()==0);
        assert(presented->party[0]->getData().empty());
        assert(!presented->party[0]->clone());
        assert(presented->party[0]->getGameGroup()==
            (family==G::SaveFamily::BlackWhite?Enums::GameVersion::BW:
                                                Enums::GameVersion::B2W2));
        assert(presented->boxes.size()==24 && presented->boxes[0][0]);
        assert(presented->boxes[0][0]->speciesID()==133);
        assert(presented->boxes[0][0]->level()==0);
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
        assert(staleActions.count==2 && staleActions[0]==GA::Review &&
               staleActions[1]==GA::Close);
        assert(G::fieldAccess(GF::IV)==GAccess::Editable);
        assert(G::fieldAccess(GF::Nature)==GAccess::Editable);
        assert(G::fieldAccess(GF::Species)!=GAccess::Editable);
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
        assert(!ui.requestExit(localUiWorkspace));
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::ConfirmExit);
        ui.reviewFromExit(localUiWorkspace);
        assert(ui.surface()==G::Gen5SharedScreenState::Surface::Review);
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
        auto wrongFamily=make(family,static_cast<uint8_t>(
            family==G::SaveFamily::BlackWhite ? 22 : 20));
        assert(!G::Gen5ReadOnlySave::parse(wrongFamily,{},&error));
        assert(error.find("disagrees")!=std::string::npos);
        // Valid backup-only copy, primary is invalid.
        const size_t copySize = family == G::SaveFamily::BlackWhite ?
            L::BlackWhiteCopySize : L::Black2White2CopySize;
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
    assert(!G::Gen5ReadOnlySave::parse(empty));
    assert(!G::Gen5ReadOnlySave::parse(std::span<const uint8_t>(empty.data(),0x40000)));
    discoveryContracts();
    std::cout<<"Gen V strict BW/B2W2 save reader synthetic contracts PASS\n";
}
