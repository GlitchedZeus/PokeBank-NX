#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Integration/Gen5/Gen5SaveDiagnostics.h"

#include "Utils/SHA256.h"
#include "Games/GameIdentity.h"
#include "Integration/Gen5/Gen5SourcePathIdentity.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <set>
#include <sys/stat.h>
#include <utility>

namespace PokeVault::Integration::Gen5 {
namespace {
constexpr size_t SaveSize = LayoutInfo::FullSaveSize;
constexpr size_t DsvFooterSize = 40;
constexpr std::array<char,16> DsvMarker{
    '|','-','D','E','S','M','U','M','E',' ','S','A','V','E','-','|'
};

std::string normalized(std::string path) {
    std::replace(path.begin(),path.end(),'\\','/');
    std::string out; out.reserve(path.size());
    bool slash=false;
    for(char ch : path) {
        if(ch=='/') { if(slash) continue; slash=true; }
        else slash=false;
        out.push_back(ch);
    }
    while(out.size()>1 && out.back()=='/') out.pop_back();
    // The Switch SD card is FAT/exFAT: a spelling-only case change must NOT
    // produce a second profile owner for the exact same physical file when
    // its devoptab inode is zero. POSIX test paths remain case-sensitive.
    if(out.size()>=6 &&
       std::tolower(static_cast<unsigned char>(out[0]))=='s' &&
       std::tolower(static_cast<unsigned char>(out[1]))=='d' &&
       std::tolower(static_cast<unsigned char>(out[2]))=='m' &&
       std::tolower(static_cast<unsigned char>(out[3]))=='c' &&
       out[4]==':' && out[5]=='/')
        std::transform(out.begin(),out.end(),out.begin(),
            [](unsigned char c){return static_cast<char>(std::tolower(c));});
    return out;
}
bool hasDotPathComponent(std::string_view path) {
    const auto value=normalized(std::string(path));
    const std::string wrapped="/"+value+"/";
    return wrapped.find("/./")!=std::string::npos ||
           wrapped.find("/../")!=std::string::npos;
}
bool safeScanRoot(std::string_view root) {
    // A malformed or broad RetroArch savefile_directory must never turn a
    // bounded emulator scan into a scan of the full Switch SD card.
    const std::string normalizedRoot=normalized(std::string(root));
    if(normalizedRoot.empty() || normalizedRoot=="/" ||
       normalizedRoot=="." || normalizedRoot=="sdmc:" ||
       normalizedRoot=="sdmc:/" || normalizedRoot==".." ||
       normalizedRoot=="sdmc")return false;
    const std::string wrapped="/"+normalizedRoot+"/";
    return wrapped.find("/../")==std::string::npos &&
           wrapped.find("/./")==std::string::npos;
}
std::string extension(std::string_view path) {
    const size_t slash=path.find_last_of("/\\");
    const size_t dot=path.find_last_of('.');
    if(dot==std::string_view::npos ||
       (slash!=std::string_view::npos && dot<slash)) return {};
    std::string ext(path.substr(dot));
    std::transform(ext.begin(),ext.end(),ext.begin(),
        [](unsigned char ch){return static_cast<char>(std::tolower(ch));});
    return ext;
}
std::string leaf(std::string_view path) {
    const size_t slash=path.find_last_of("/\\");
    return std::string(path.substr(slash==std::string_view::npos?0:slash+1));
}
std::string join(const std::string& root, const std::string& child) {
    return root.empty() || root.back()=='/' ? root+child : root+"/"+child;
}
std::string identityFor(const struct stat& st,const std::string& path) {
    if(st.st_ino != 0) return "inode:" +
        std::to_string(static_cast<unsigned long long>(st.st_dev)) + ":" +
        std::to_string(static_cast<unsigned long long>(st.st_ino));
    return normalized(path);
}
std::string sha256Hex(std::span<const uint8_t> data) {
    Utils::SHA256 hasher;
    hasher.update(data.data(),data.size());
    std::array<uint8_t,Utils::PKSE_SHA256_HASH_SIZE> digest{};
    hasher.finalize(digest.data());
    std::string out; out.reserve(digest.size()*2);
    constexpr char digits[]="0123456789abcdef";
    for(uint8_t b:digest) { out.push_back(digits[b>>4]); out.push_back(digits[b&15]); }
    return out;
}

// Some Switch SD devoptab implementations support stat() but not lstat().
// On POSIX filesystems always use lstat() so a link cannot escape a scan root.
// SD card FAT/exFAT has no POSIX symlinks; ONLY on the sdmc: device and ONLY
// for an unsupported lstat syscall may we use the same stat() as Gen IV.
bool safeSourceStat(const std::string& path,struct stat& st) {
    if(::lstat(path.c_str(),&st)==0)return !S_ISLNK(st.st_mode);
    const int failure=errno;
    if(path.compare(0,6,"sdmc:/")!=0 ||
       (failure!=ENOSYS && failure!=EOPNOTSUPP && failure!=EINVAL))
        return false;
    return ::stat(path.c_str(),&st)==0;
}
bool safeRegular(const std::string& path,struct stat* dst=nullptr) {
    struct stat st{};
    if(!safeSourceStat(path,st) || !S_ISREG(st.st_mode))return false;
    if(dst)*dst=st;
    return true;
}
bool safeDirectory(const std::string& path,struct stat* dst=nullptr) {
    struct stat st{};
    if(!safeSourceStat(path,st) || !S_ISDIR(st.st_mode))return false;
    if(dst)*dst=st;
    return true;
}
uint32_t le32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
        (static_cast<uint32_t>(p[1])<<8) |
        (static_cast<uint32_t>(p[2])<<16) |
        (static_cast<uint32_t>(p[3])<<24);
}
std::string trim(std::string value) {
    const auto notSpace=[](unsigned char ch){return !std::isspace(ch);};
    value.erase(value.begin(),std::find_if(value.begin(),value.end(),notSpace));
    value.erase(std::find_if(value.rbegin(),value.rend(),notSpace).base(),value.end());
    return value;
}
std::string retroArchConfiguredRoot(const std::string& config) {
    FILE* f=std::fopen(config.c_str(),"rb");
    if(!f)return {};
    char buffer[2048];
    std::string root;
    while(std::fgets(buffer,sizeof(buffer),f)) {
        std::string line(buffer);
        const size_t eq=line.find('=');
        if(eq==std::string::npos || trim(line.substr(0,eq))!="savefile_directory")continue;
        root=trim(line.substr(eq+1));
        if(root.size()>=2 && root.front()=='"' && root.back()=='"')
            root=root.substr(1,root.size()-2);
        if(root=="default")root.clear();
        if(!root.empty() && root.front()!='/' && root.find(":/")==std::string::npos) {
            const size_t slash=config.find_last_of("/\\");
            root=join(slash==std::string::npos?".":config.substr(0,slash),root);
        }
        break;
    }
    std::fclose(f);
    return root;
}
bool eligible(std::string_view filename) {
    const auto ext=extension(filename);
    return ext==".sav" || ext==".srm" || ext==".dsv" || ext==".dss";
}
Source::ValidationStatus statusForFailure(const std::string& msg) {
    if(msg.find("symlink")!=std::string::npos || msg.find("unsupported")!=std::string::npos ||
       msg.find("savestate")!=std::string::npos ||
       msg.find("two valid but different")!=std::string::npos)
        return Source::ValidationStatus::Unsupported;
    if(msg.find("read")!=std::string::npos || msg.find("regular")!=std::string::npos)
        return Source::ValidationStatus::ReadError;
    return Source::ValidationStatus::Invalid;
}

struct ScanState {
    DiscoveryLimits limits;
    DiscoveryLimits stopAt;
    bool rootLimitReached = false;
    DiscoveryResult result;
    std::set<std::string> visitedDirs;
    std::set<std::string> visitedFiles;
};
void scan(const DiscoveryRoot& root,const std::string& dir,size_t depth,ScanState& s) {
    struct stat dirSt{};
    if(s.rootLimitReached || depth>root.maxDepth ||
       !safeScanRoot(dir) || !safeDirectory(dir,&dirSt)) return;
    const auto dirIdentity=identityFor(dirSt,dir);
    if(s.visitedDirs.contains(dirIdentity))return;
    if(s.result.directoriesExamined>=s.stopAt.maxDirectories) {
        s.rootLimitReached=true;
        return;
    }
    // Count attempted opens too: repeated unreadable folders are bounded.
    ++s.result.directoriesExamined;
    s.visitedDirs.insert(dirIdentity);
    DIR* d=::opendir(dir.c_str());
    if(!d) {++s.result.unreadableDirectories;return;}
    std::vector<std::string> names;
    bool entryLimitReached=false;
    for(;;) {
        errno=0;
        const auto* item=::readdir(d);
        if(!item) {
            if(errno!=0)++s.result.unreadableDirectories;
            break;
        }
        std::string name(item->d_name);
        if(name=="." || name=="..")continue;
        if(s.result.entriesExamined>=s.stopAt.maxEntries) {
            entryLimitReached=true;
            break;
        }
        ++s.result.entriesExamined;
        names.push_back(std::move(name));
    }
    if(::closedir(d)!=0)++s.result.unreadableDirectories;
    std::sort(names.begin(),names.end());
    for(const auto& name:names) {
        if(s.rootLimitReached)break;
        const std::string file=join(dir,name);
        struct stat st{};
        // lstat never follows outside-directory symlinks.
        if(!safeSourceStat(file,st))continue;
        if(S_ISDIR(st.st_mode)) {
            if(depth<root.maxDepth)scan(root,file,depth+1,s);
            continue;
        }
        if(!S_ISREG(st.st_mode) || !eligible(name))continue;
        const auto fileIdentity=identityFor(st,file);
        if(s.visitedFiles.contains(fileIdentity))continue;
        if(s.result.filesExamined>=s.stopAt.maxFiles) {
            s.rootLimitReached=true; break;
        }
        s.visitedFiles.insert(fileIdentity);
        ++s.result.filesExamined;
        auto choices=inspectSourceChoices(file,root.providerLabel);
        for(auto& row:choices) {
            // Hide unrelated RetroArch raw saves while retaining actionable
            // wrapper/savestate failures and BOTH verified native copy rows.
            // A failed Gen V parser is normally hidden, EXCEPT when a normalized
            // raw battery image carries a recognizable, explicitly UNVERIFIED
            // Gen V game-version byte. Keep that failure for diagnostics only.
            // It is NEVER an assignable Save Instance (validation != Ready).
            if(row.ready() || root.providerLabel!="RetroArch" ||
               extension(file)==".dsv" || extension(file)==".dss" ||
               row.sourceVariant=="gen5-unverified-version-hint")
                s.result.instances.push_back(std::move(row));
        }
    }
    // Process all names already enumerated under the hard memory cap, then
    // report incomplete discovery rather than pretending this catalog is
    // exhaustive. Subdirectory scans may have hit another limit first.
    if(entryLimitReached)s.rootLimitReached=true;
}
} // namespace

std::string persistentGen5SourceIdentity(
    std::string_view path,uint64_t device,uint64_t inode) {
    // Retain the preexisting v2 key for inode-backed files. For a Switch SD
    // file with st_ino==0 use the normalized path alone: an ordinary emulator
    // save should NOT invalidate profile ownership just because its contents
    // changed. A source must still be freshly parsed when selected to open.
    std::string label="gen5-physical-v2:"+normalized(std::string(path));
    if(inode!=0)label+=":inode:"+std::to_string(device)+":"+
                       std::to_string(inode);
    return sha256Hex({reinterpret_cast<const uint8_t*>(label.data()),label.size()});
}

bool readNormalizedSourceReadOnly(const std::string& path,
                                  std::vector<uint8_t>& bytes,
                                  std::string& diagnostic,
                                  std::string* containerType) {
    bytes.clear();
    diagnostic.clear();
    if(containerType)containerType->clear();
    if(hasDotPathComponent(path)) {
        diagnostic="unsupported source path with dot or parent segments";
        return false;
    }
    std::string prefix=path.substr(0,5);
    std::transform(prefix.begin(),prefix.end(),prefix.begin(),
        [](unsigned char ch){return static_cast<char>(std::tolower(ch));});
    if(path.find('\0')!=std::string::npos ||
       (prefix=="sdmc:" && sdOwnershipPath(path).empty())) {
        diagnostic="unsupported source path; choose a full path within sdmc:/";
        return false;
    }
#ifdef __SWITCH__
    // A remembered relative path changes meaning with the homebrew loader's
    // working directory. Gen V emulator assignments are absolute SD paths.
    if(!path.starts_with("sdmc:/")) {
        diagnostic="unsupported source path; choose a full sdmc:/ save path";
        return false;
    }
#endif
    if(extension(path)==".dss") {
        diagnostic="savestate .dss is unsupported; use cartridge backup";
        return false;
    }
    struct stat before{};
    if(!safeRegular(path,&before)) {
        diagnostic="source missing, unreadable, symlink, or not a regular file";
        return false;
    }
    const bool raw=before.st_size==static_cast<off_t>(SaveSize);
    // Gen IV's established emulator adapter accepts a raw 0x80000 payload
    // followed by variable-size metadata with a documented final 40-byte
    // DeSmuME footer. Use exactly that boundary here: read at most 0x80000
    // bytes into memory, seek to EOF-40, and validate footer plus native
    // Gen V checksums. No write access or unbounded allocation is possible.
    const bool dsv=extension(path)==".dsv" &&
        before.st_size>=static_cast<off_t>(SaveSize+DsvFooterSize);
    if(!raw && !dsv) {
        diagnostic=extension(path)==".dsv" ?
            "unsupported .dsv wrapper size" : "unsupported NDS save size";
        return false;
    }
    FILE* f=std::fopen(path.c_str(),"rb");
    if(!f) {diagnostic="source read failed";return false;}
    struct stat opened{};
    if(::fstat(::fileno(f),&opened)!=0 || !S_ISREG(opened.st_mode) ||
       before.st_dev!=opened.st_dev || before.st_ino!=opened.st_ino ||
       before.st_size!=opened.st_size) {
        std::fclose(f);diagnostic="source changed during read";return false;
    }
    std::vector<uint8_t> candidate(SaveSize);
    bool ok=std::fread(candidate.data(),1,SaveSize,f)==SaveSize;
    if(dsv) {
        std::array<uint8_t,DsvFooterSize> footer{};
        // The footer marker is at the END of the container, not necessarily
        // immediately after the 0x80000-byte save payload.
        ok=ok && std::fseek(f,
            static_cast<long>(before.st_size-static_cast<off_t>(DsvFooterSize)),
            SEEK_SET)==0;
        ok=ok && std::fread(footer.data(),1,footer.size(),f)==footer.size();
        ok=ok && std::equal(DsvMarker.begin(),DsvMarker.end(),
                           footer.begin()+24);
        ok=ok && le32(footer.data()+4)==SaveSize &&
              le32(footer.data())<=SaveSize &&
              le32(footer.data()+20)==0;
    }
    ok=ok && std::fgetc(f)==EOF && std::ferror(f)==0;
    struct stat after{};
    ok=ok && ::fstat(::fileno(f),&after)==0 &&
       before.st_size==after.st_size && before.st_mtime==after.st_mtime &&
       before.st_ino==after.st_ino;
    const bool closed=std::fclose(f)==0;
    if(!ok || !closed) {
        diagnostic=dsv?"unsupported .dsv footer or source changed during read":
                       "source read failed or changed";
        return false;
    }
    bytes=std::move(candidate);
    if(containerType)*containerType=dsv?"dsv-footer":"raw-nds-battery";
    diagnostic=dsv?"strict DeSmuME-compatible .dsv footer validated":
                   "exact 0x80000 read-only battery payload";
    return true;
}

namespace {
struct SourceSnapshot {
    Source::SaveInstance row;
    std::vector<uint8_t> bytes;
    std::string message;
};
SourceSnapshot readSourceSnapshot(const std::string& path,
    std::string_view providerLabel,std::string_view assignedExactGame) {
    SourceSnapshot snapshot;
    auto& row=snapshot.row;
    row.generation=5;
    row.platformLabel="Nintendo DS";
    row.providerLabel=std::string(providerLabel);
    row.providerId=Source::providerIdFor(providerLabel);
    row.label=leaf(path);
    row.sourcePath=path;
    row.location=path;
    row.normalizedPath=normalized(path);
    row.sourceIdentity=persistentGen5SourceIdentity(path,0,0);
    row.access=Source::AccessMode::ReadOnly;
    row.kind=extension(path)==".dsv"?Source::SaveInstanceKind::Backup:
        Source::SaveInstanceKind::BatterySave;
    row.gameId=std::string(assignedExactGame);
    // File paths with /./ or /../ can alias an already-owned SD file under
    // a different spelling on a no-inode filesystem. Never accept them from
    // the manual source picker, even if the OS can follow the path.
    if(hasDotPathComponent(path)) {
        row.validation=Source::ValidationStatus::Unsupported;
        row.diagnostic="unsupported source path with dot or parent segments";
        return snapshot;
    }
    struct stat st{};
    if(!safeRegular(path,&st)) {
        row.validation=Source::ValidationStatus::ReadError;
        row.diagnostic="source is missing, symlink, or not a regular file";
        return snapshot;
    }
    row.fileSize=static_cast<uint64_t>(st.st_size);
    row.modifiedTime=static_cast<int64_t>(st.st_mtime);
    row.physicalIdentity=identityFor(st,path);
    if(extension(path)==".dss") {
        row.kind=Source::SaveInstanceKind::SaveState;
        row.validation=Source::ValidationStatus::Unsupported;
        row.diagnostic="DraStic .dss savestate unsupported; use in-game backup";
        return snapshot;
    }

    auto& bytes=snapshot.bytes;
    auto& message=snapshot.message;
    std::string container;
    if(!readNormalizedSourceReadOnly(path,bytes,message,&container)) {
        row.validation=statusForFailure(message);
        row.diagnostic=message;
        return snapshot;
    }
    struct stat after{};
    if(!safeRegular(path,&after) || st.st_dev!=after.st_dev ||
       st.st_ino!=after.st_ino || st.st_size!=after.st_size ||
       st.st_mtime!=after.st_mtime) {
        bytes.clear();
        row.validation=Source::ValidationStatus::Invalid;
        row.diagnostic="source changed during inspection; choose it again";
        return snapshot;
    }
    row.contentFingerprint=sha256Hex(bytes);
    // Profile ownership keys the normalized path (+inode where usable), not the
    // changing content hash; every open still revalidates the exact snapshot.
    row.sourceIdentity=persistentGen5SourceIdentity(
        path,static_cast<uint64_t>(st.st_dev),static_cast<uint64_t>(st.st_ino));
    row.containerType=container;
    return snapshot;
}

Source::SaveInstance inspectSnapshot(const SourceSnapshot& snapshot,
    std::string_view assignedExactGame,SaveCopySelection copy) {
    auto row=snapshot.row;
    if(snapshot.bytes.empty())return row;
    std::string error;
    const auto save=Gen5ReadOnlySave::parse(snapshot.bytes,{},&error,copy);
    if(!save) {
        row.validation=statusForFailure(error);
        row.diagnostic=error;
        // A real Gen V battery file with one damaged checksum was previously
        // dropped from mixed RetroArch folders, hiding the actual rejection.
        // An unchecked header is not proof of game identity: keep gameId
        // empty, leave validation Invalid/Unsupported, and mark the hint only
        // to improve failure reporting (never profile assignment).
        for(const size_t offset:{size_t{0},LayoutInfo::BlackWhiteCopySize,
                                 LayoutInfo::Black2White2CopySize}) {
            const size_t versionAt=offset+0x19400+0x1F;
            if(versionAt>=snapshot.bytes.size())continue;
            const auto hinted=Gen5ReadOnlySave::gameIdFromVersion(snapshot.bytes[versionAt]);
            if(!isExactGen5Id(hinted))continue;
            row.sourceVariant="gen5-unverified-version-hint";
            row.diagnostic+="; unverified Gen V header suggests "+std::string(hinted);
            break;
        }
        return row;
    }
    row.gameId=std::string(save->exactGameId());
    // Even Automatic must pin the actually validated partition. Otherwise a
    // later corruption could silently reopen the other copy of the same file.
    row.sourceVariant=save->selectedBackupPartition()?"gen5-copy-1-backup":
                      "gen5-copy-0-primary";
    row.partyCount=save->partyCount();
    row.trainerName=displayTrainerName(save->trainer().rawName).value_or("");
    // Both retail save copies may have valid checksums yet vastly different
    // PC contents. Real Black/White/B2W2 saves can contain a populated
    // primary and an empty backup. Make this distinction visible BEFORE the
    // user explicitly chooses a copy; never silently pick a "newer" save.
    row.sourceLabel=std::string("Gen V / ")+
        (save->selectedBackupPartition()?"Backup copy":"Primary copy")+
        " / Party "+std::to_string(save->partyCount())+
        " / Boxed "+std::to_string(save->diagnostics().occupiedBoxRecords);
    if(!row.trainerName.empty())row.sourceLabel+=" / "+row.trainerName;
    row.validation=Source::ValidationStatus::Ready;
    row.diagnostic=snapshot.message;
    if(save->selectedBackupPartition())row.diagnostic+="; validated backup copy";
    if(save->diagnostics().invalidBoxRecords)
        row.diagnostic+="; boxed PK5 quarantined: "+
            std::to_string(save->diagnostics().invalidBoxRecords);
    if(!assignedExactGame.empty() && row.gameId!=assignedExactGame) {
        row.validation=Source::ValidationStatus::AssignmentMismatch;
        const auto* actual=Games::findGame(row.gameId);
        const auto* expected=Games::findGame(assignedExactGame);
        row.diagnostic="Save is Pokemon "+(actual?std::string(actual->title):row.gameId)+
            "; choose a save for Pokemon "+
            (expected?std::string(expected->title):std::string(assignedExactGame));
    }
    return row;
}
} // namespace

Source::SaveInstance inspectSourceFile(const std::string& path,
    std::string_view providerLabel,std::string_view assignedExactGame,
    SaveCopySelection copy) {
    return inspectSnapshot(readSourceSnapshot(path,providerLabel,assignedExactGame),
                           assignedExactGame,copy);
}

std::vector<Source::SaveInstance> inspectSourceChoices(const std::string& path,
    std::string_view providerLabel,std::string_view assignedExactGame) {
    const auto snapshot=readSourceSnapshot(path,providerLabel,assignedExactGame);
    auto row=inspectSnapshot(snapshot,assignedExactGame,SaveCopySelection::Automatic);
    if(row.diagnostic.find("two valid but different")==std::string::npos)
        return {std::move(row)};
    std::vector<Source::SaveInstance> choices;
    for(const auto copy:{SaveCopySelection::Primary,SaveCopySelection::Backup})
        choices.push_back(inspectSnapshot(snapshot,assignedExactGame,copy));
    return choices;
}

ReadOnlyProbe reopenValidatedSource(const Source::SaveInstance& selected,
                                   SaveCopySelection selection) {
    ReadOnlyProbe out;
    out.instance=selected;
    out.instance.validation=Source::ValidationStatus::Invalid;
    out.instance.access=Source::AccessMode::ReadOnly;
    out.instance.diagnostic="source revalidation failed";
    if(!selected.ready() || selected.generation!=5 || !isExactGen5Id(selected.gameId))
        return out;
    // The explicit copy is pinned in the chosen Save Instance. A later
    // generic reopen must not silently fall back to a different copy.
    SaveCopySelection pinned=SaveCopySelection::Automatic;
    if(selected.sourceVariant=="gen5-copy-0-primary")
        pinned=SaveCopySelection::Primary;
    else if(selected.sourceVariant=="gen5-copy-1-backup")
        pinned=SaveCopySelection::Backup;
    else if(!selected.sourceVariant.empty()) {
        out.instance.diagnostic="unsupported Gen V source copy identity";
        return out;
    }
    if(selection!=SaveCopySelection::Automatic &&
       pinned!=SaveCopySelection::Automatic && selection!=pinned) {
        out.instance.diagnostic="Gen V copy choice conflicts with validated Save Instance";
        return out;
    }
    if(pinned!=SaveCopySelection::Automatic)selection=pinned;
    const auto fresh=inspectSourceFile(selected.path(),selected.providerLabel,
                                      selected.gameId,selection);
    if(!Source::sameValidatedSnapshot(selected,fresh) ||
       selected.sourceLabel!=fresh.sourceLabel) {
        out.instance.diagnostic="source changed since discovery; reselect/revalidate";
        return out;
    }
    std::vector<uint8_t> bytes;
    std::string diagnostic,container;
    if(!readNormalizedSourceReadOnly(selected.path(),bytes,diagnostic,&container) ||
       container!=selected.containerType ||
       sha256Hex(bytes)!=selected.contentFingerprint) {
        out.instance.diagnostic="source bytes or wrapper changed since validation";
        return out;
    }
    const SourceContext context{selected.gameId,selected.providerId,
        selected.providerLabel,selected.sourcePath,selected.normalizedPath,
        selected.physicalIdentity,selected.sourceIdentity,selected.claimedProfile,
        selected.sourceIndex};
    out=probeNormalizedBattery(bytes,context,selection);
    if(out.ready()) {
        out.instance.fileSize=selected.fileSize;
        out.instance.modifiedTime=selected.modifiedTime;
        out.instance.contentFingerprint=selected.contentFingerprint;
        out.instance.containerType=selected.containerType;
        out.instance.sourceVariant=selected.sourceVariant;
        out.instance.label=selected.label;
        out.instance.kind=selected.kind;
        out.instance.sourceLabel=selected.sourceLabel;
        // Provenance flags belong to the selected Save Instance. A strict
        // parse re-probes native bytes, not profile metadata or UI choices.
        out.instance.rememberedSource=selected.rememberedSource;
        out.instance.mostRecentlyModified=selected.mostRecentlyModified;
        out.instance.sourceAliases=selected.sourceAliases;
        out.instance.claimConflict=selected.claimConflict;
    }
    return out;
}

DiscoveryResult discoverSources(std::span<const DiscoveryRoot> roots,
                                DiscoveryLimits limits) {
    ScanState state;
    state.limits.maxFiles=std::min<size_t>(limits.maxFiles,4096);
    state.limits.maxEntries=std::min<size_t>(limits.maxEntries,32768);
    state.limits.maxDirectories=std::min<size_t>(limits.maxDirectories,512);
    std::vector<DiscoveryRoot> activeRoots;
    std::set<std::string> rootIdentities;
    for(const auto& root:roots) {
        // Every root reached here was attempted, including one refused only
        // because the directory budget is already spent.
        ++state.result.rootsConsidered;
        if(state.result.rootsExamined>=state.limits.maxDirectories) {
            state.result.limitReached=true;
            break;
        }
        ++state.result.rootsExamined;
        struct stat st{};
        if(!safeScanRoot(root.path) || !safeDirectory(root.path,&st) ||
           !rootIdentities.insert(identityFor(st,root.path)).second)continue;
        auto bounded=root;
        bounded.maxDepth=std::min<size_t>(bounded.maxDepth,2);
        activeRoots.push_back(std::move(bounded));
    }
    for(size_t i=0;i<activeRoots.size();++i) {
        const size_t remainingRoots=activeRoots.size()-i;
        // Reserve a fair share for every remaining provider/root. A huge
        // RetroArch folder must not consume the entire file/entry/directory
        // budget before DraStic or melonDS is ever searched. Unused budget
        // carries forward; global hard caps remain unchanged.
        const auto share=[remainingRoots](size_t used,size_t cap) {
            const size_t remaining=cap-used;
            return used+remaining/remainingRoots+(remaining%remainingRoots!=0);
        };
        state.stopAt={share(state.result.filesExamined,state.limits.maxFiles),
                      share(state.result.entriesExamined,state.limits.maxEntries),
                      share(state.result.directoriesExamined,state.limits.maxDirectories)};
        state.rootLimitReached=false;
        // Count actual root-directory opens rather than guessing from the
        // existence of a provider config path. Permission and mount failures
        // must not be presented to the user as "no save files found".
        const size_t openedBefore=state.result.directoriesExamined;
        scan(activeRoots[i],activeRoots[i].path,0,state);
        if(state.result.directoriesExamined>openedBefore)
            ++state.result.rootsOpened;
        state.result.limitReached|=state.rootLimitReached;
    }
    Source::sortNewestFirst(state.result.instances);
    return std::move(state.result);
}

std::vector<DiscoveryRoot> defaultDraSticRoots() {
    return {
        {"sdmc:/switch/drastic/user/backup","DraStic",1},
        {"sdmc:/switch/drastic/backup","DraStic",1},
        {"sdmc:/switch/drastic/user/savestates","DraStic",1},
        {"sdmc:/switch/drastic/savestates","DraStic",1},
    };
}

DiscoveryResult discoverKnownSources(DiscoveryLimits limits,
                                    const std::string& retroArchConfig,
                                    const std::string& retroArchFallback) {
    std::vector<DiscoveryRoot> roots;
    const std::string configured=retroArchConfiguredRoot(retroArchConfig);
    if(safeScanRoot(configured) && safeDirectory(configured))
        roots.push_back({configured,"RetroArch",2});
    else if(safeScanRoot(retroArchFallback) && safeDirectory(retroArchFallback))
        roots.push_back({retroArchFallback,"RetroArch",2});
    for(const auto& root:defaultDraSticRoots())
        if(safeDirectory(root.path))roots.push_back(root);
    for(const auto& root:{
            DiscoveryRoot{"sdmc:/switch/melonds","melonDS",2},
            DiscoveryRoot{"sdmc:/melonds","melonDS",2}})
        if(safeDirectory(root.path))roots.push_back(root);
    return discoverSources(roots,limits);
}

bool writeGen5DiagnosticsReport(const std::string& sourcePath,std::string_view exactGame,
                                std::string_view timestamp,const std::string& diagnosticsRoot,
                                std::string& writtenPath,std::string& error) {
    writtenPath.clear();
    error.clear();
    const auto safe=[](std::string_view v) {
        return !v.empty() && std::all_of(v.begin(),v.end(),[](unsigned char c) {
            return std::isalnum(c) || c=='_' || c=='-';
        });
    };
    if(!safe(exactGame) || !safe(timestamp) || diagnosticsRoot.empty()) {
        error="diagnostics request has an unsafe game id, timestamp or root";
        return false;
    }
    std::vector<uint8_t> bytes;
    std::string container;
    std::string readError;
    SaveReport5 report;
    if(!readNormalizedSourceReadOnly(sourcePath,bytes,readError,&container)) {
        report.container=container.empty()?"unreadable":container;
        report.chosenReason="source read failed: "+readError;
    } else {
        report=inspectSave5(bytes,container.empty()?"raw-nds-battery":container);
    }
    if(::mkdir(diagnosticsRoot.c_str(),0777)!=0 && errno!=EEXIST) {
        error="could not create the diagnostics directory";
        return false;
    }
    const std::string path=diagnosticsRoot+"/gen5_"+std::string(exactGame)+"_"+
                           std::string(timestamp)+".txt";
    // O_EXCL: a new file only; never overwrite or touch any other file.
    const int fd=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0644);
    if(fd<0) {
        error="could not create a new diagnostics file";
        return false;
    }
    const auto text=formatSaveReport5(report,exactGame);
    bool ok=::write(fd,text.data(),text.size())==static_cast<ssize_t>(text.size());
    ok=::fsync(fd)==0 && ok;
    ok=::close(fd)==0 && ok;
    if(!ok) {
        ::unlink(path.c_str());
        error="could not write the diagnostics file";
        return false;
    }
    writtenPath=path;
    return true;
}

} // namespace PokeVault::Integration::Gen5
