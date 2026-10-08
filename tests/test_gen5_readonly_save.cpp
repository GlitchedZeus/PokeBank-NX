#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5AssignedSource.h"
#include "Integration/Gen5/Gen5GameSourceCatalog.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"
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
        }
        // Same Gen I-IV shared exit decision model: local Edit changes are
        // NOT committed to the staged workspace until explicit Keep.
        G::Gen5SharedPokemonSession viewer;
        using GSlot=G::Gen5StagedPokemonWorkspace::Slot;
        using GRegion=G::Gen5StagedPokemonWorkspace::Region;
        using GMode=G::Gen5SharedPokemonSession::Mode;
        using GField=G::StagedPokemon5Record::Field;
        assert(viewer.begin(workspace,GSlot{GRegion::Party,0,0},GMode::View,error));
        assert(!viewer.stage(GField::Nature,0,15,error));
        assert(viewer.back() && viewer.mode()==GMode::None);
        assert(!viewer.begin(workspace,GSlot{GRegion::Box,24,0},GMode::Edit,error));
        G::Gen5SharedPokemonSession editor;
        assert(editor.begin(workspace,GSlot{GRegion::Party,0,0},GMode::Edit,error));
        assert(editor.stage(GField::Nature,0,10,error));
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
        assert(editor.stage(GField::Friendship,0,87,error));
        assert(editor.dirty());
        editor.discardDraft();
        assert(workspace.viewBox(0,0)->friendship()==255);
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
