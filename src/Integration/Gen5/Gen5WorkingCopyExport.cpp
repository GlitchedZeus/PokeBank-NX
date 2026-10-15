#include "Integration/Gen5/Gen5WorkingCopyExport.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"

#include "Utils/PokeBankPaths.h"
#include "Utils/SHA256.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {
namespace {

std::string join(const std::string& parent,const std::string& child) {
    return parent.empty()?child:(parent.back()=='/'?parent+child:parent+"/"+child);
}
bool exists(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(),&st)==0 || errno!=ENOENT;
}
bool safeDirectory(const std::string& path) {
    struct stat st{};
    return ::lstat(path.c_str(),&st)==0 && S_ISDIR(st.st_mode);
}
bool ensuredDirectory(const std::string& path,std::string& error) {
    if(!PokeBank::Paths::ensureDirectoryTree(path,&error))return false;
    if(!safeDirectory(path)) {
        error="Gen V export destination is not a real directory";
        return false;
    }
    return true;
}
bool validProfile(std::string_view profile) noexcept {
    if(profile.size()!=32)return false;
    // All-zero is PokeBank NX's deliberate offline "Pokemon Saves"
    // profile identity, not a missing/empty identity. Its exports still
    // occupy a separate account-000... namespace and never touch sources.
    for(const char c:profile)
        if(!((c>='0' && c<='9') || (c>='a' && c<='f')))return false;
    return true;
}
bool safeEvidence(std::string_view value) noexcept {
    return value.find('\n')==std::string_view::npos &&
           value.find('\r')==std::string_view::npos &&
           value.find('\0')==std::string_view::npos;
}
std::string hash(std::span<const uint8_t> bytes) {
    Utils::SHA256 sha;
    sha.update(bytes.data(),bytes.size());
    std::array<uint8_t,Utils::PKSE_SHA256_HASH_SIZE> digest{};
    sha.finalize(digest.data());
    constexpr char hex[]="0123456789abcdef";
    std::string out;
    out.reserve(digest.size()*2);
    for(uint8_t byte:digest) {
        out.push_back(hex[byte>>4]);
        out.push_back(hex[byte&15]);
    }
    return out;
}
bool writeSynced(const std::string& path,
                 std::span<const uint8_t> bytes,std::string& error) {
    FILE* file=std::fopen(path.c_str(),"wb");
    if(!file) {
        error="Gen V export could not create a working-copy file";
        return false;
    }
    bool ok=std::fwrite(bytes.data(),1,bytes.size(),file)==bytes.size();
    if(ok)ok=std::fflush(file)==0;
    if(ok) {
        const int fd=::fileno(file);
        ok=fd>=0 && ::fsync(fd)==0;
    }
    const bool closed=std::fclose(file)==0;
    if(!ok || !closed) {
        error="Gen V working-copy write/flush/fsync/close failed";
        return false;
    }
    return true;
}
bool readExact(const std::string& path,size_t expected,
               std::vector<uint8_t>& bytes,std::string& error) {
    bytes.clear();
    struct stat st{};
    if(::lstat(path.c_str(),&st)!=0 || !S_ISREG(st.st_mode) ||
       static_cast<uint64_t>(st.st_size)!=expected) {
        error="Gen V export read-back file is absent, symlinked or wrong size";
        return false;
    }
    FILE* file=std::fopen(path.c_str(),"rb");
    if(!file) {
        error="Gen V export read-back cannot open file";
        return false;
    }
    struct stat opened{};
    bool ok=::fstat(::fileno(file),&opened)==0 &&
            opened.st_dev==st.st_dev && opened.st_ino==st.st_ino &&
            opened.st_size==st.st_size;
    if(ok) {
        bytes.resize(expected);
        ok=std::fread(bytes.data(),1,expected,file)==expected;
        ok=ok && std::fgetc(file)==EOF && std::ferror(file)==0;
    }
    const bool closed=std::fclose(file)==0;
    if(!ok || !closed) {
        bytes.clear();
        error="Gen V export read-back failed or file changed";
        return false;
    }
    return true;
}
void cleanTemporary(const std::string& root) noexcept {
    if(root.empty())return;
    ::remove(join(root,"original_backup.sav").c_str());
    ::remove(join(root,"original_backup.dsv").c_str());
    ::remove(join(root,"edited.sav").c_str());
    ::remove(join(root,"EXPORT_MANIFEST.txt").c_str());
    ::rmdir(root.c_str());
}
struct BackupFacts {
    std::string_view container;
    std::string_view name;
    size_t size = 0;
    std::string_view sha256;
};
std::string manifest(const WorkingCopyExportRequest& request,
                     const Gen5StagedSaveCandidate& candidate,
                     std::string_view originalHash,
                     std::string_view editedHash,
                     const BackupFacts& backup) {
    std::string out="PokeBank NX Generation V verified working-copy export\n";
    out+="GAME_ID="+candidate.exactGameId+"\n";
    out+="SOURCE_FORMAT=BW_B2W2_NATIVE_SAV5_0x80000\n";
    out+="SOURCE_PATH="+request.sourcePath+"\n";
    out+="SOURCE_SHA256="+std::string(originalHash)+"\n";
    out+="SOURCE_SHA256_SCOPE=0x80000_SAV5_PAYLOAD\n";
    out+="SOURCE_CONTAINER="+std::string(backup.container)+"\n";
    out+="SOURCE_FILE_SIZE="+std::to_string(backup.size)+"\n";
    out+="SOURCE_FILE_SHA256="+std::string(backup.sha256)+"\n";
    out+="EDITED_SHA256="+std::string(editedHash)+"\n";
    out+="SAVE_COPY="+std::string(candidate.selectedCopyOffset==0?
                              "Primary":"Backup")+"\n";
    out+="SELECTED_COPY_OFFSET="+std::to_string(candidate.selectedCopyOffset)+"\n";
    out+="CHANGED_RECORDS="+std::to_string(candidate.changedRecords)+"\n";
    out+="BAG_CHANGED="+std::string(candidate.bagChanged?"YES":"NO")+"\n";
    out+="TRAINER_CHANGED="+std::string(candidate.trainerChanged?"YES":"NO")+"\n";
    out+="TIMESTAMP="+request.timestamp+"\n";
    out+="APPLICATION_SHA="+request.applicationSha+"\n";
    out+="ORIGINAL_BACKUP="+std::string(backup.name)+"\n";
    out+="ORIGINAL_BACKUP_SHA256="+std::string(backup.sha256)+"\n";
    out+="ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE\n";
    out+="EDITED_SAVE=edited.sav\n";
    out+="EDITED_CONTAINER=raw-nds-battery\n";
    out+="WRITE_READBACK=VERIFIED\n";
    out+="FULL_SAV_REPARSE=VERIFIED\n";
    out+="EMULATOR_SAVE_WRITE=DISABLED\n";
    out+="INSTALLED_GAME_WRITE=DISABLED\n";
    out+="INJECT_SAVE=DISABLED\n";
    return out;
}
} // namespace

WorkingCopyExportResult publishGen5WorkingCopy(
    const Gen5StagedPokemonWorkspace& workspace,
    const WorkingCopyExportRequest& request) {
    WorkingCopyExportResult out;
    if(!validProfile(request.profileIdentity) ||
       !PokeBank::Paths::isSafeTimestamp(request.timestamp) ||
       !safeEvidence(request.sourcePath) ||
       !safeEvidence(request.applicationSha) ||
       request.sourcePath.empty() || request.applicationSha.empty()) {
        out.error="Gen V working-copy export has invalid profile, timestamp or evidence";
        return out;
    }
    const std::string gameId(workspace.sourceSave().exactGameId());
    if(!isExactGen5Id(gameId)) {
        out.error="Gen V export source has no exact BW/B2W2 title";
        return out;
    }
    // Strict full-image equality + selected-copy reparse + exact PK5 record
    // whitelist occur before a single byte of filesystem output.
    std::string candidateError;
    const auto candidate=buildStagedSaveCandidateForObservedSource(
        workspace,request.observedSourceBytes,&candidateError);
    if(!candidate) {
        out.error=candidateError.empty()?"Gen V staged candidate invalid":candidateError;
        return out;
    }
    out.changedRecords=candidate->changedRecords;
    out.sourceSha256=hash(request.observedSourceBytes);
    out.editedSha256=hash(candidate->bytes);

    // The backup is the source file exactly as found. A provided file must
    // wrap exactly the validated payload: a raw file IS the payload, a .dsv
    // starts with it. Anything else is refused before filesystem output.
    std::span<const uint8_t> backupBytes=request.observedSourceBytes;
    std::string container="raw-nds-battery";
    if(!request.observedSourceFile.empty()) {
        const auto& file=request.observedSourceFile;
        const bool prefix=file.size()>=request.observedSourceBytes.size() &&
            std::equal(request.observedSourceBytes.begin(),
                       request.observedSourceBytes.end(),file.begin());
        const bool raw=request.sourceContainer=="raw-nds-battery" &&
            file.size()==request.observedSourceBytes.size();
        const bool dsv=request.sourceContainer=="dsv-footer" &&
            file.size()>request.observedSourceBytes.size();
        if(!prefix || !(raw || dsv)) {
            out.error="Gen V export source file does not wrap the validated SAV5 payload";
            return out;
        }
        backupBytes=file;
        container=request.sourceContainer;
    } else if(!request.sourceContainer.empty() &&
              request.sourceContainer!="raw-nds-battery") {
        out.error="Gen V export of a "+request.sourceContainer+
                  " source needs its verbatim file bytes";
        return out;
    }
    out.backupName=container=="dsv-footer"?"original_backup.dsv":"original_backup.sav";
    out.backupSha256=hash(backupBytes);

    const std::string fixedRoot=PokeBank::Paths::exportsRoot()+"/gen5";
    const std::string root=request.rootDirectory.empty()?
        fixedRoot:request.rootDirectory;
#ifdef __SWITCH__
    if(root!=fixedRoot) {
        out.error="Gen V export cannot leave app-owned exports/gen5";
        return out;
    }
#endif
    // No arbitrary components from a source filename enter the destination.
    const std::string account="account-"+request.profileIdentity;
    const std::string directory=join(root,account);
    std::string ioError;
    if(!ensuredDirectory(directory,ioError)) {
        out.error=ioError;
        return out;
    }
    const std::string stem=gameId+"_"+request.timestamp;
    std::string finalPath,temporaryPath;
    for(int suffix=1;suffix<=9999;++suffix) {
        const std::string label=stem+(suffix==1?"":"-"+std::to_string(suffix));
        const std::string final=join(directory,label);
        const std::string temporary=join(directory,"."+label+".pending");
        if(exists(final) || exists(temporary))continue;
        if(::mkdir(temporary.c_str(),0700)==0) {
            finalPath=final;
            temporaryPath=temporary;
            break;
        }
    }
    if(temporaryPath.empty()) {
        out.error="Gen V could not reserve a unique temporary export directory";
        return out;
    }
    const auto fail=[&](std::string reason) {
        out.error=std::move(reason);
        cleanTemporary(temporaryPath);
        return out;
    };
    const auto backupPath=join(temporaryPath,out.backupName);
    const auto editedPath=join(temporaryPath,"edited.sav");
    const auto manifestPath=join(temporaryPath,"EXPORT_MANIFEST.txt");
    if(!writeSynced(backupPath,backupBytes,ioError))
        return fail(ioError);
    if(request.failurePoint==WorkingCopyFailurePoint::AfterBackup)
        return fail("Injected failure after Gen V backup write");
    if(!writeSynced(editedPath,candidate->bytes,ioError))
        return fail(ioError);
    if(request.failurePoint==WorkingCopyFailurePoint::AfterEdited)
        return fail("Injected failure after Gen V edited write");

    std::vector<uint8_t> backupReadback,editedReadback;
    if(!readExact(backupPath,backupBytes.size(),backupReadback,ioError) ||
       !readExact(editedPath,LayoutInfo::FullSaveSize,editedReadback,ioError))
        return fail(ioError);
    const auto baseline=workspace.sourceSave().sourceBytes();
    if(!std::equal(backupReadback.begin(),backupReadback.end(),backupBytes.begin()) ||
       !std::equal(baseline.begin(),baseline.end(),backupReadback.begin()) ||
       editedReadback!=candidate->bytes ||
       hash(backupReadback)!=out.backupSha256 ||
       hash(editedReadback)!=out.editedSha256)
        return fail("Gen V working-copy read-back SHA-256 or full-byte mismatch");
    const auto selection=workspace.sourceSave().selectedBackupPartition()
        ? SaveCopySelection::Backup:SaveCopySelection::Primary;
    const auto reparsed=Gen5ReadOnlySave::parse(
        editedReadback,gameId,&ioError,selection);
    if(!reparsed ||
       reparsed->selectedCopyOffset()!=candidate->selectedCopyOffset)
        return fail("Gen V exported working copy failed strict full-SAV reparse");
    if(request.failurePoint==WorkingCopyFailurePoint::AfterReadback)
        return fail("Injected failure after Gen V readback");

    const auto text=manifest(request,*candidate,out.sourceSha256,out.editedSha256,
        BackupFacts{container,out.backupName,backupBytes.size(),out.backupSha256});
    if(!writeSynced(manifestPath,std::span<const uint8_t>(
        reinterpret_cast<const uint8_t*>(text.data()),text.size()),ioError))
        return fail(ioError);
    if(request.failurePoint==WorkingCopyFailurePoint::AfterManifest)
        return fail("Injected failure after Gen V manifest write");
    if(request.failurePoint==WorkingCopyFailurePoint::DestinationAppears)
        (void)::mkdir(finalPath.c_str(),0700);
    // Publication happens only inside the app-owned export tree and never
    // replaces anything: a directory rename would silently replace an empty
    // destination, so the destination is claimed with an exclusive mkdir
    // (EEXIST for any existing directory, file or link) and the verified
    // files move in, manifest last. Failure returns them and removes only
    // the directory this call created.
    if(::mkdir(finalPath.c_str(),0700)!=0)
        return fail("Gen V working-copy publication failed; existing export preserved");
    const auto publishedBackup=join(finalPath,out.backupName);
    const auto publishedEdited=join(finalPath,"edited.sav");
    const auto publishedManifest=join(finalPath,"EXPORT_MANIFEST.txt");
    const auto unpublish=[&](std::string reason) {
        (void)::rename(publishedBackup.c_str(),backupPath.c_str());
        (void)::rename(publishedEdited.c_str(),editedPath.c_str());
        (void)::rename(publishedManifest.c_str(),manifestPath.c_str());
        ::rmdir(finalPath.c_str());
        return fail(std::move(reason));
    };
    if(::rename(backupPath.c_str(),publishedBackup.c_str())!=0)
        return unpublish("Gen V working-copy publication failed");
    if(request.failurePoint==WorkingCopyFailurePoint::AfterFirstPublishedFile)
        return unpublish("Injected failure during Gen V publication");
    if(::rename(editedPath.c_str(),publishedEdited.c_str())!=0 ||
       ::rename(manifestPath.c_str(),publishedManifest.c_str())!=0)
        return unpublish("Gen V working-copy publication failed");
    ::rmdir(temporaryPath.c_str());
    out.success=true;
    out.directory=finalPath;
    out.error.clear();
    return out;
}

} // namespace PokeVault::Integration::Gen5
