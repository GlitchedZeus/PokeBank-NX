#pragma once
#include <algorithm>
#include <cctype>
#include <ctime>
#include <string>
#include <sys/stat.h>

namespace PokeBank::UIModel::BackupHistory {
struct Date { std::string order,label; bool fromName=false; };
inline std::string format(const std::string& digits) {
    return digits.substr(0,4)+"-"+digits.substr(4,2)+"-"+digits.substr(6,2)+" "+
        digits.substr(8,2)+":"+digits.substr(10,2)+":"+digits.substr(12,2);
}
inline Date date(const std::string& name,std::time_t modified) {
    // Historical PKSE used both compact and separated timestamps, often with
    // custom prefixes/suffixes. Do not sort by a title or an ownership badge.
    for(size_t start=0;start+14<=name.size();++start) {
        if(name.compare(start,2,"20")!=0)continue;
        std::string digits;
        size_t i=start;
        for(;i<name.size() && digits.size()<14;++i) {
            const auto c=static_cast<unsigned char>(name[i]);
            if(std::isdigit(c))digits+=static_cast<char>(c);
            else if(c!='-' && c!='_' && c!=' ' && c!=':')break;
        }
        if(digits.size()!=14)continue;
        const int month=std::stoi(digits.substr(4,2)),day=std::stoi(digits.substr(6,2));
        const int hour=std::stoi(digits.substr(8,2)),minute=std::stoi(digits.substr(10,2));
        const int second=std::stoi(digits.substr(12,2));
        if(month<1 || month>12 || day<1 || day>31 || hour>23 || minute>59 || second>59)continue;
        return {digits,format(digits),true};
    }
    if(modified>0) {
        std::tm t{};
        if(localtime_r(&modified,&t)) {
            char digits[32]{};
            if(std::strftime(digits,sizeof(digits),"%Y%m%d%H%M%S",&t))
                return {digits,"Modified "+format(digits),false};
        }
    }
    return {{},"Date unknown",false};
}
inline Date dateForDirectory(const std::string& name,const std::string& path) {
    struct stat st{};
    return date(name,::lstat(path.c_str(),&st)==0?st.st_mtime:0);
}
}
