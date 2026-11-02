#pragma once

// Read-only, bounded physical snapshots for historical backup opens. No write
// authority is granted by the location of a file or by successful save parsing.
#include "Utils/SHA256.h"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <utility>
#include <dirent.h>
#include <functional>

namespace PokeVault::Source::ImmutableSnapshot {
inline std::string stamp(const struct stat& s) {
    std::string out=std::to_string(s.st_dev)+":"+std::to_string(s.st_ino)+":"+
        std::to_string(s.st_size)+":"+std::to_string(s.st_mtime)+":"+std::to_string(s.st_ctime);
#ifdef __linux__
    out+=":"+std::to_string(s.st_mtim.tv_nsec)+":"+std::to_string(s.st_ctim.tv_nsec);
#endif
    return out;
}
inline std::string fileStamp(const std::string& path) {
    struct stat s{};
    return ::lstat(path.c_str(),&s)==0 && S_ISREG(s.st_mode)?stamp(s):std::string();
}
inline std::string directoryStamp(const std::string& path) {
    struct stat s{};
    return ::lstat(path.c_str(),&s)==0 && S_ISDIR(s.st_mode)?stamp(s):std::string();
}
inline bool safePath(const std::string& path,const std::string& root,bool directory=false) {
    if(root.empty() || path.size()<=root.size() || path.compare(0,root.size(),root)!=0 ||
       (root.back()!='/' && path[root.size()]!='/'))return false;
    size_t start=path.starts_with('/')?1:path.starts_with("sdmc:/")?6:std::string::npos;
    if(start==std::string::npos || path.find('\\')!=std::string::npos)return false;
    while(start<path.size()) {
        const auto slash=path.find('/',start);
        const auto part=path.substr(start,slash==std::string::npos?path.size()-start:slash-start);
        if(part.empty() || part=="." || part=="..")return false;
        struct stat s{};
        const std::string prefix=slash==std::string::npos?path:path.substr(0,slash);
        if(::lstat(prefix.c_str(),&s)!=0 || S_ISLNK(s.st_mode) ||
           (slash==std::string::npos?(directory?!S_ISDIR(s.st_mode):!S_ISREG(s.st_mode)):!S_ISDIR(s.st_mode)))return false;
        if(slash==std::string::npos)break;
        start=slash+1;
    }
    return true;
}
inline std::string fingerprint(const std::vector<uint8_t>& bytes) {
    Utils::SHA256 sha;sha.update(bytes.data(),bytes.size());
    std::array<uint8_t,32> digest{};sha.finalize(digest.data());
    static constexpr char hex[]="0123456789abcdef";
    std::string out;out.reserve(64);
    for(auto b:digest){out+=hex[b>>4];out+=hex[b&15];}
    return out;
}
struct File {
    std::string path,root,physicalStamp,sha256;
    std::vector<uint8_t> bytes;
};
inline bool read(const std::string& path,const std::string& root,
                 const std::string& expectedStamp,File& out,std::string& error,
                 size_t cap=16*1024*1024,bool allowEmpty=false) {
    out={};
    if(!safePath(path,root)) {error="Backup path is missing, unsafe, or contains a link";return false;}
    const auto before=fileStamp(path);
    if(before.empty() || (!expectedStamp.empty() && before!=expectedStamp)) {
        error="Backup changed since it was listed; refresh the backup browser";return false;
    }
    std::FILE* f=std::fopen(path.c_str(),"rb");
    if(!f){error="Backup could not be opened read-only";return false;}
    struct stat opened{};
    bool ok=::fstat(::fileno(f),&opened)==0 && stamp(opened)==before &&
        (opened.st_size>0 || (allowEmpty && opened.st_size==0)) && static_cast<uint64_t>(opened.st_size)<=cap;
    std::vector<uint8_t> bytes;
    if(ok) {
        bytes.resize(static_cast<size_t>(opened.st_size));
        ok=std::fread(bytes.data(),1,bytes.size(),f)==bytes.size() && std::fgetc(f)==EOF && !std::ferror(f);
    }
    struct stat after{};
    ok=ok && ::fstat(::fileno(f),&after)==0 && stamp(after)==before;
    if(std::fclose(f)!=0)ok=false;
    if(!ok || !safePath(path,root) || fileStamp(path)!=before) {
        error="Backup changed during read or is empty, oversized, or incomplete";return false;
    }
    out={path,root,before,fingerprint(bytes),std::move(bytes)};
    error.clear();return true;
}
inline bool unchanged(const File& file,std::string& error) {
    File observed;
    if(!read(file.path,file.root,file.physicalStamp,observed,error))return false;
    if(observed.sha256!=file.sha256){error="Backup bytes changed during validation";return false;}
    return true;
}
struct Directory {
    std::string path,root;
    std::vector<std::pair<std::string,std::string>> directories;
    std::vector<File> files;
};
inline bool readDirectory(const std::string& path,const std::string& root,
                          Directory& out,std::string& error) {
    Directory result{path,root,{}, {}};
    size_t total=0,entries=0;
    std::function<bool(const std::string&,int)> scan=[&](const std::string& dir,int depth) {
        if(depth>4 || !safePath(dir,root,true))return false;
        struct stat before{};
        if(::lstat(dir.c_str(),&before)!=0)return false;
        result.directories.emplace_back(dir,stamp(before));
        DIR* d=::opendir(dir.c_str());if(!d)return false;
        std::vector<std::string> children;
        bool ok=true;
        while(const auto* e=::readdir(d)) {
            const std::string name=e->d_name;
            if(name=="." || name=="..")continue;
            if(++entries>256 || name.find('/')!=std::string::npos || name.find('\\')!=std::string::npos) {
                ok=false;break;
            }
            children.push_back(name);
        }
        ::closedir(d);
        if(!ok)return false;
        std::sort(children.begin(),children.end());
        for(const auto& name:children) {
            const auto child=dir+"/"+name;
            struct stat st{};
            if(::lstat(child.c_str(),&st)!=0 || S_ISLNK(st.st_mode))return false;
            if(S_ISDIR(st.st_mode)) {if(!scan(child,depth+1))return false;}
            else {
                File file;
                if(!read(child,root,stamp(st),file,error,64*1024*1024,true))return false;
                total+=file.bytes.size();if(total>128*1024*1024)return false;
                result.files.push_back(std::move(file));
            }
        }
        struct stat after{};
        return ::lstat(dir.c_str(),&after)==0 && stamp(after)==stamp(before);
    };
    if(!scan(path,0) || result.files.empty()) {
        out={};if(error.empty())error="Backup directory is empty, changed, unsafe, or exceeds snapshot limits";
        return false;
    }
    out=std::move(result);error.clear();return true;
}
inline bool unchanged(const Directory& source,std::string& error) {
    Directory observed;
    if(!readDirectory(source.path,source.root,observed,error))return false;
    if(observed.directories!=source.directories || observed.files.size()!=source.files.size()) {
        error="Backup directory changed during validation";return false;
    }
    for(size_t i=0;i<source.files.size();++i)
        if(source.files[i].path!=observed.files[i].path ||
           source.files[i].physicalStamp!=observed.files[i].physicalStamp ||
           source.files[i].sha256!=observed.files[i].sha256) {
            error="Backup file changed during validation";return false;
        }
    return true;
}
}
