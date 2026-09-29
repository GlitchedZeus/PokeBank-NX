#ifndef UI_MOVE_PICKER_PRESENTATION_H
#define UI_MOVE_PICKER_PRESENTATION_H
#include "Enums/GameVersion.h"
#include "Names/MoveBattleStats.h"
#include "Names/MoveNames.h"
#include <cstdio>
#include <string>
namespace PokeBank::UIModel::MovePickerPresentation {
inline std::string numberedName(uint16_t move){if(move==0)return "000 - Empty move slot";char p[16]{};std::snprintf(p,sizeof(p),"%03u - ",static_cast<unsigned>(move));return std::string(p)+Names::getMoveName(move);}
inline std::string statsLabel(uint16_t move,Enums::GameVersion game){if(move==0)return {};const auto* s=Names::getMoveBattleStats(move,game);if(!s)return {};const std::string a=s->accuracy==0?"—":std::to_string(s->accuracy);const std::string p=s->power==0?"—":std::to_string(s->power);return "Acc "+a+"   Pwr "+p+"   PP "+std::to_string(s->pp);}
inline std::string rowLabel(uint16_t move,Enums::GameVersion game){auto out=numberedName(move);const auto stats=statsLabel(move,game);if(!stats.empty())out+="    "+stats;return out;}
}
#endif
