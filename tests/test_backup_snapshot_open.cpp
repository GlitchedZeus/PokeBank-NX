#include "UI/SourceBackupOpen.h"
#include "UI/BackupHistoryPresentation.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
namespace S=PokeVault::Source::ImmutableSnapshot;
namespace B=PokeBank::UIModel::SourceBackups;
static void write(const fs::path& path,const std::string& bytes) {
    std::ofstream f(path,std::ios::binary);f<<bytes;assert(f.good());
}
int main() {
    char pattern[]="/tmp/pokebank-backup-open-XXXXXX";
    const char* made=::mkdtemp(pattern);assert(made);
    const fs::path root=made;
    const auto source=(root/"current.sav").string();write(source,"original immutable save fixture");
    const auto before=S::fileStamp(source);
    const auto history=(root/"history").string(),exports=(root/"exports").string();
    for(const auto* date:{"20261024_010203","20261022_010203","20261023_010203","20261024_010203","20261021_010203"})
        assert(B::createBackup(history,"crystal_gbc",source,date).ok);
    auto list=B::list(exports,source,history);
    assert(list.entries.size()==7 && S::fileStamp(source)==before);
    for(size_t i=1;i+2<list.entries.size();++i)
        assert(list.entries[i].timestamp>=list.entries[i+1].timestamp);
    std::string error;S::File selected;
    assert(B::openSnapshot(list,1,"crystal_gbc",selected,error));
    assert(std::string(selected.bytes.begin(),selected.bytes.end())=="original immutable save fixture");
    assert(S::unchanged(selected,error) && S::fileStamp(source)==before);
    assert(!B::openSnapshot(list,1,"gold_gbc",selected,error));
    assert(!B::openSnapshot(list,0,"crystal_gbc",selected,error));
    assert(!B::openSnapshot(list,list.entries.size()-1,"crystal_gbc",selected,error));
    assert(!B::openSnapshot(list,999,"crystal_gbc",selected,error));
    const auto path=list.entries[1].path;
    write(path,"replacement with different bytes");
    assert(!B::openSnapshot(list,1,"crystal_gbc",selected,error));
    assert(B::canDelete(list,1)==B::Refusal::Missing);
    list=B::list(exports,source,history);
    assert(B::openSnapshot(list,1,"crystal_gbc",selected,error));
    fs::remove(source); // Archived recovery remains readable when the live source disappears.
    assert(B::historicalSources(exports,history,"crystal_gbc")==std::vector<std::string>{source});
    list.recoveryReadOnly=true;
    assert(B::canDelete(list,1)!=B::Refusal::None);
    assert(B::openSnapshot(list,1,"crystal_gbc",selected,error));
    list.recoveryReadOnly=false;
    const auto originalRecord=list.entries[1].recordDir;
    fs::rename(originalRecord,originalRecord+"-moved");
    fs::create_directory_symlink(originalRecord+"-moved",originalRecord);
    assert(!B::openSnapshot(list,1,"crystal_gbc",selected,error));
    assert(B::canDelete(list,1)!=B::Refusal::None);
    fs::remove(originalRecord);fs::rename(originalRecord+"-moved",originalRecord);
    S::Directory native;
    assert(S::readDirectory(originalRecord,history,native,error));
    assert(S::unchanged(native,error));
    write(fs::path(originalRecord)/"unexpected", "changed after preview");
    assert(!S::unchanged(native,error));
    fs::create_directory(root/"escape");write(root/"escape"/"main","outside");
    fs::create_symlink(root/"escape"/"main",fs::path(originalRecord)/"linked");
    assert(!S::readDirectory(originalRecord,history,native,error));
    assert(!S::safePath(originalRecord+"/../escape/main",history));
    assert(!S::safePath((root/"escape"/"main").string(),history));
    using PokeBank::UIModel::BackupHistory::date;
    assert(date("20261024_030405",0).order=="20261024030405");
    assert(date("2026-10-24 03:04:05 manual",0).label=="2026-10-24 03:04:05");
    assert(date("arbitrary named workspace",0).label=="Date unknown");
    assert(date("20269999_999999",0).order.empty());
    fs::remove_all(root);
    std::cout<<"Backup snapshots: exact copy, chronological duplicate dates, lost originals, stale files, traversal/symlink refusals and immutable native directory preview PASS\n";
}
