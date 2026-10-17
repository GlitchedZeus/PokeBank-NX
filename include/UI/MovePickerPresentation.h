#ifndef UI_MOVE_PICKER_PRESENTATION_H
#define UI_MOVE_PICKER_PRESENTATION_H
#include "Enums/GameVersion.h"
#include "Names/MoveBattleStats.h"
#include "Names/MoveNames.h"
#include <cstdio>
#include <string>
namespace PokeBank::UIModel::MovePickerPresentation {
struct CompactPickerLayout {
    int width;
    int height;
    int visibleRows;
    int rowStart;
    int rowStep;
    int highlightHeight;
};
// Phase 9 (owner round 1: "the moves selector could have 1 or 2 more rows instead of that blank
// space", all gens): the same 520 px frame now shows 10 rows instead of 9. The last row's highlight
// still ends inside the frame with a margin, so nothing is clipped and the footer stays clear.
inline constexpr CompactPickerLayout compactPickerLayout() noexcept {
    return {760, 520, 10, 86, 42, 38};
}
inline constexpr int kCompactPickerBottomMargin = 12;
static_assert(compactPickerLayout().rowStart +
                  (compactPickerLayout().visibleRows - 1) * compactPickerLayout().rowStep +
                  compactPickerLayout().highlightHeight + kCompactPickerBottomMargin <=
                  compactPickerLayout().height,
              "every visible move row, last one included, fits the chooser frame");
static_assert(compactPickerLayout().height - compactPickerLayout().rowStart -
                  compactPickerLayout().visibleRows * compactPickerLayout().rowStep < compactPickerLayout().rowStep,
              "no room for another whole row is left blank");
inline std::string numberedName(uint16_t move){if(move==0)return "000 - Empty move slot";char p[16]{};std::snprintf(p,sizeof(p),"%03u - ",static_cast<unsigned>(move));return std::string(p)+Names::getMoveName(move);}
// Shared Gen II/III/IV native move metadata, plus any Gen I caller using
// this presentation contract. Keep game-specific stats untouched; only the
// visual separators and missing-value placeholders are standardized.
inline std::string statsLabel(uint16_t move,Enums::GameVersion game) {
    if(move==0)return {};
    const auto* s=Names::getMoveBattleStats(move,game);
    if(!s)return "Acc --  |  Pwr --  |  PP --";
    const std::string accuracy=s->accuracy==0?"--":std::to_string(s->accuracy);
    const std::string power=s->power==0?"--":std::to_string(s->power);
    return "Acc "+accuracy+"  |  Pwr "+power+
           "  |  PP "+std::to_string(s->pp);
}
inline std::string rowLabel(uint16_t move,Enums::GameVersion game) {
    const auto name=numberedName(move);
    if(move==0)return name;
    return name+"  |  "+statsLabel(move,game);
}
}
#endif
