#ifndef POKEBANK_GEN5_SOURCE_SELECTION_H
#define POKEBANK_GEN5_SOURCE_SELECTION_H

#include "Source/SaveInstance.h"
#include "Integration/Gen5/Gen5SaveInstanceAdapter.h"
#include <algorithm>
#include <span>
#include <string>
#include <unordered_map>

namespace PokeVault::Integration::Gen5 {
// Remember navigation only for profiles whose last focused game was Gen V.
// No disk writes, trainer inference or source-open authority belongs here.
class ProfileGameFocus {
public:
    void remember(std::string_view profile,std::string_view game) {
        if(profile.empty())return;
        if(isExactGen5Id(game))games_[std::string(profile)]=game;
        else games_.erase(std::string(profile));
    }
    [[nodiscard]] std::string gameFor(std::string_view profile) const {
        const auto found=games_.find(std::string(profile));
        return found==games_.end()?std::string():found->second;
    }
private:
    std::unordered_map<std::string,std::string> games_;
};
// UI focus only, never authorization to open cached source bytes.
struct SourceSelectionFocus {
    std::string gameId;
    std::string physicalIdentity;
    std::string variant;
};
inline SourceSelectionFocus captureSourceFocus(
    std::span<const Source::SaveInstance> rows,int index) {
    if(index<0)return {};
    for(const auto& row:rows) {
        if(index--==0)return {row.gameId,row.physicalIdentity,row.sourceVariant};
    }
    return {};
}
inline void restoreSourceFocus(std::span<const Source::SaveInstance> rows,
    const SourceSelectionFocus& previous,int& index,int& scroll,int visibleRows=5) {
    visibleRows=std::max(1,visibleRows);
    const int count=static_cast<int>(rows.size());
    if(!previous.physicalIdentity.empty()) {
        for(int i=0;i<count;++i) {
            const auto& row=rows[static_cast<size_t>(i)];
            if(row.gameId==previous.gameId &&
               row.physicalIdentity==previous.physicalIdentity &&
               row.sourceVariant==previous.variant) {
                index=i;
                scroll=std::clamp(scroll,0,std::max(0,count-visibleRows));
                return; // Preserve an independently finger-scrolled viewport.
            }
        }
    }
    // On initial entry or a removed selection, focus the explicitly remembered
    // copy if available. Never choose a copy based on modification timestamps.
    index=0;
    for(int i=0;i<count;++i)if(rows[static_cast<size_t>(i)].rememberedSource) {
        index=i;
        break;
    }
    scroll=std::clamp(index-visibleRows+1,0,std::max(0,count-visibleRows));
}
} // namespace PokeVault::Integration::Gen5
#endif
