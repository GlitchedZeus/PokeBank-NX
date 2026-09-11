#include "Legacy/RetroArchGSCDiscovery.h"

#include "Utils/SHA256.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <dirent.h>
#include <set>
#include <sys/stat.h>

namespace PokeVault::Legacy {
namespace {
using Integration::Gen2::SourceGame;

enum class IdentityHint { None, Gold, Silver, Crystal, Ambiguous };

std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}
std::string dirname(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string(".") : path.substr(0, slash);
}
std::string join(const std::string& root, const std::string& name) {
    if (root.empty() || root.back() == '/' || root.back() == '\\') return root + name;
    return root + "/" + name;
}
bool isAbsoluteOrDevicePath(const std::string& path) {
    return !path.empty() && (path.front() == '/' || path.find(":/") != std::string::npos);
}
bool isRegularFile(const std::string& path, struct stat* metadata = nullptr) {
    struct stat info{};
    if (stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) return false;
    if (metadata) *metadata = info;
    return true;
}
bool isDirectory(const std::string& path) {
    struct stat info{};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}
std::string normalizedPath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    std::string result; result.reserve(path.size()); bool slash = false;
    for (char c : path) {
        if (c == '/') { if (slash) continue; slash = true; } else slash = false;
        result.push_back(c);
    }
    while (result.size() > 1 && result.back() == '/') result.pop_back();
    return result;
}
std::string sha256Hex(const uint8_t* bytes, std::size_t size) {
    Utils::SHA256 hash; hash.update(bytes, size);
    std::array<uint8_t, Utils::PKSE_SHA256_HASH_SIZE> digest{}; hash.finalize(digest.data());
    constexpr char digits[] = "0123456789abcdef"; std::string result; result.reserve(digest.size()*2);
    for (uint8_t byte : digest) { result.push_back(digits[byte>>4]); result.push_back(digits[byte&0x0F]); }
    return result;
}
std::string filesystemIdentity(const std::string& path) {
    struct stat info{};
    if (stat(path.c_str(), &info) == 0 && info.st_ino != 0) {
        return "inode:" + std::to_string(static_cast<unsigned long long>(info.st_dev)) + ":" +
               std::to_string(static_cast<unsigned long long>(info.st_ino));
    }
    return normalizedPath(path);
}
std::string sourceIdentity(const std::string& path) {
    const std::string identity = "retroarch-gsc:" + normalizedPath(path);
    return sha256Hex(reinterpret_cast<const uint8_t*>(identity.data()), identity.size());
}
bool supportedExtension(const std::string& name) {
    const std::size_t dot = name.find_last_of('.'); if (dot == std::string::npos) return false;
    std::string ext = name.substr(dot); std::transform(ext.begin(), ext.end(), ext.begin(),
        [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return ext == ".sav" || ext == ".srm";
}
std::string compactLower(const std::string& path) {
    std::string compact; compact.reserve(path.size());
    for (unsigned char c : path) if (std::isalnum(c)) compact.push_back(static_cast<char>(std::tolower(c)));
    return compact;
}
IdentityHint identityHint(const std::string& path) {
    const std::string compact = compactLower(path);
    const bool gold = compact.find("pokemongold") != std::string::npos || compact.find("pokegold") != std::string::npos;
    const bool silver = compact.find("pokemonsilver") != std::string::npos || compact.find("pokesilver") != std::string::npos;
    const bool crystal = compact.find("pokemoncrystal") != std::string::npos || compact.find("pokecrystal") != std::string::npos;
    const int matches = static_cast<int>(gold)+static_cast<int>(silver)+static_cast<int>(crystal);
    if (matches > 1) return IdentityHint::Ambiguous;
    if (gold) return IdentityHint::Gold; if (silver) return IdentityHint::Silver; if (crystal) return IdentityHint::Crystal;
    std::string token;
    for (std::size_t i=0;i<=path.size();++i) {
        const unsigned char c = i<path.size()?static_cast<unsigned char>(path[i]):'/';
        if (std::isalnum(c)) { token.push_back(static_cast<char>(std::tolower(c))); continue; }
        if (token=="gold") return IdentityHint::Gold;
        if (token=="silver") return IdentityHint::Silver;
        if (token=="crystal") return IdentityHint::Crystal;
        token.clear();
    }
    return IdentityHint::None;
}
SourceGame gameForHint(IdentityHint hint) {
    switch(hint) { case IdentityHint::Silver:return SourceGame::Silver; case IdentityHint::Crystal:return SourceGame::Crystal; default:return SourceGame::Gold; }
}
bool plausibleSize(std::size_t size) noexcept {
    if (size == Integration::Gen2::kRawSaveSize32K || size == Integration::Gen2::kRawSaveSize64K) return true;
    return size > Integration::Gen2::kRawSaveSize64K &&
        Integration::Gen2::isKnownRTCFooterSize(size - Integration::Gen2::kRawSaveSize64K);
}
bool readExactly(const std::string& path, std::size_t expected, std::vector<uint8_t>& bytes) {
    FILE* file = std::fopen(path.c_str(), "rb"); if (!file) return false;
    bytes.resize(expected); const std::size_t count = std::fread(bytes.data(),1,bytes.size(),file);
    const int trailing = std::fgetc(file); const bool closeOk = std::fclose(file)==0;
    if (count!=expected || trailing!=EOF || !closeOk) { bytes.clear(); return false; }
    return true;
}

bool validateAny(std::span<const uint8_t> bytes, Integration::Gen2::SaveError& error, std::string& detail,
                 bool& crystal, bool& gs) {
    crystal=false; gs=false;
    auto c=Integration::Gen2::parse(bytes,SourceGame::Crystal); if(c) crystal=true;
    auto g=Integration::Gen2::parse(bytes,SourceGame::Gold); if(g) gs=true;
    if(crystal||gs) return true;
    if(c.error!=Integration::Gen2::SaveError::GameHintMismatch){error=c.error;detail=c.detail;}
    else {error=g.error;detail=g.detail;}
    return false;
}

GSCSource inspectFile(const std::string& path,const struct stat& metadata,IdentityHint hint) {
    GSCSource source; source.path=path; source.normalizedPath=normalizedPath(path); source.sourceIdentity=sourceIdentity(path);
    source.fileSize=static_cast<uint64_t>(metadata.st_size); source.modifiedTime=static_cast<int64_t>(metadata.st_mtime);
    if (metadata.st_size < 0 || !plausibleSize(static_cast<std::size_t>(metadata.st_size))) {
        source.status=GSCSourceStatus::InvalidSave; source.parseError=Integration::Gen2::SaveError::WrongSize;
        source.detail="candidate is not a supported 32/64 KiB Generation II battery save or known RTC wrapper"; return source;
    }
    std::vector<uint8_t> bytes;
    if(!readExactly(path,static_cast<std::size_t>(metadata.st_size),bytes)){source.status=GSCSourceStatus::ReadError;source.detail="candidate could not be read completely";return source;}
    source.contentFingerprint=sha256Hex(bytes.data(),bytes.size());

    if(hint==IdentityHint::None || hint==IdentityHint::Ambiguous){
        bool crystal=false,gs=false;
        if(!validateAny(bytes,source.parseError,source.detail,crystal,gs)){source.status=GSCSourceStatus::InvalidSave;return source;}
        if(crystal && !gs && hint==IdentityHint::None){
            auto parsed=Integration::Gen2::parse(bytes,SourceGame::Crystal); source.status=GSCSourceStatus::Ready;
            source.gameId=Integration::Gen2::sourceGameId(SourceGame::Crystal); source.save=std::move(parsed.save);
            source.detail="validated read-only RetroArch Crystal source from structural evidence"; return source;
        }
        source.status=GSCSourceStatus::AmbiguousIdentity;
        source.detail=hint==IdentityHint::Ambiguous
            ? "valid Generation II save path contains multiple Gold/Silver/Crystal identity hints"
            : "valid Gold/Silver-family save needs an explicit Gold or Silver source hint; the two releases are not structurally distinguishable";
        return source;
    }

    const SourceGame assumed=gameForHint(hint); auto parsed=Integration::Gen2::parse(bytes,assumed);
    if(!parsed){
        source.status=parsed.error==Integration::Gen2::SaveError::GameHintMismatch?GSCSourceStatus::AmbiguousIdentity:GSCSourceStatus::InvalidSave;
        source.parseError=parsed.error; source.detail=parsed.detail; return source;
    }
    source.status=GSCSourceStatus::Ready; source.gameId=Integration::Gen2::sourceGameId(assumed); source.save=std::move(parsed.save);
    source.detail=std::string("validated read-only RetroArch Generation II source (")+Integration::Gen2::regionName(source.save->metadata().region)+")";
    return source;
}

struct ScanState { GSCScanLimits limits; GSCDiscoveryResult result; std::set<std::string> visitedDirectories,visitedFiles; };
void scanDirectory(const std::string& root,std::size_t depth,ScanState& state){
    const std::string dirId=filesystemIdentity(root);
    if(state.result.limitReached||depth>state.limits.maxDepth||!state.visitedDirectories.insert(dirId).second)return;
    DIR* directory=opendir(root.c_str()); if(!directory)return; std::vector<std::string> names;
    while(const dirent* entry=readdir(directory)) if(std::string(entry->d_name)!="."&&std::string(entry->d_name)!="..") names.emplace_back(entry->d_name);
    closedir(directory); std::sort(names.begin(),names.end());
    for(const auto& name:names){
        if(state.result.limitReached)break; const std::string path=join(root,name);
        if(isDirectory(path)){if(depth<state.limits.maxDepth)scanDirectory(path,depth+1,state);continue;}
        if(!supportedExtension(name))continue; struct stat metadata{}; if(!isRegularFile(path,&metadata))continue;
        const std::string fileId=filesystemIdentity(path); if(!state.visitedFiles.insert(fileId).second)continue;
        if(state.result.filesExamined>=state.limits.maxFiles){state.result.limitReached=true;break;} ++state.result.filesExamined;
        const IdentityHint hint=identityHint(path); GSCSource source=inspectFile(path,metadata,hint); source.canonicalPath=fileId;
        if(source.ready()||source.status==GSCSourceStatus::AmbiguousIdentity||hint!=IdentityHint::None) state.result.sources.push_back(std::move(source));
    }
}
} // namespace

std::vector<std::string> retroArchGSCSaveRootsFromConfig(const std::string& configPath){
    std::vector<std::string> roots; FILE* file=std::fopen(configPath.c_str(),"rb"); if(!file)return roots; char line[2048];
    while(std::fgets(line,sizeof(line),file)){std::string value(line);const std::size_t equals=value.find('=');if(equals==std::string::npos||trim(value.substr(0,equals))!="savefile_directory")continue;value=trim(value.substr(equals+1));if(value.size()>=2&&value.front()=='"'&&value.back()=='"')value=value.substr(1,value.size()-2);if(!value.empty()&&value!="default")roots.push_back(isAbsoluteOrDevicePath(value)?value:join(dirname(configPath),value));break;}
    std::fclose(file); return roots;
}
GSCDiscoveryResult discoverGSCSaves(std::span<const std::string> approvedRoots,GSCScanLimits limits){
    if(limits.maxFiles==0)limits.maxFiles=1; ScanState state{limits,{},{},{}};
    for(const auto& root:approvedRoots){if(state.result.limitReached)break;if(isDirectory(root)){if(state.result.activeRoot.empty())state.result.activeRoot=normalizedPath(root);scanDirectory(root,0,state);}}
    return std::move(state.result);
}
GSCDiscoveryResult discoverConfiguredRetroArchGSCSaves(GSCScanLimits limits,const std::string& configPath,const std::string& conventionalRoot){
    const auto configured=retroArchGSCSaveRootsFromConfig(configPath);std::vector<std::string> selected;GSCDiscoveryResult::RootKind kind=GSCDiscoveryResult::RootKind::None;
    if(!configured.empty()&&isDirectory(configured.front())){selected.push_back(configured.front());kind=GSCDiscoveryResult::RootKind::Configured;}
    else if(isDirectory(conventionalRoot)){selected.push_back(conventionalRoot);kind=GSCDiscoveryResult::RootKind::ConventionalFallback;}
    auto result=discoverGSCSaves(selected,limits);result.activeRootKind=kind;return result;
}

} // namespace PokeVault::Legacy
