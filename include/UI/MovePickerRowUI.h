#ifndef POKEBANK_UI_MOVE_PICKER_ROW_H
#define POKEBANK_UI_MOVE_PICKER_ROW_H

#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include "UI/MovePickerPresentation.h"

#include <cstdint>
#include <string>

namespace UI::MovePickerRowUI {

// Draw actual ALIGNED columns. Separate cells keep the bold move name and
// accuracy/power/PP metadata legible without spacing assumptions in fonts.
// The shared model's generation-specific battle-stat table is authoritative.
inline void draw(PKSEFramebuffer& fb,int x,int y,
                 uint16_t move,Enums::GameVersion game) {
    namespace Presentation=PokeBank::UIModel::MovePickerPresentation;
    if(move==0) {
        fb.drawText(x,y+4,"000 - Empty move slot",Colors::Text,TextStyle::Body);
        return;
    }
    fb.drawText(x,y,Presentation::numberedName(move),
                Colors::Text,TextStyle::Heading);
    const auto* stats=Names::getMoveBattleStats(move,game);
    const std::string acc=stats&&stats->accuracy?std::to_string(stats->accuracy):"--";
    const std::string pwr=stats&&stats->power?std::to_string(stats->power):"--";
    const std::string pp=stats?std::to_string(stats->pp):"--";
    fb.drawText(x+308,y+9,"| Acc "+acc,Colors::TextSecondary,TextStyle::Caption);
    fb.drawText(x+423,y+9,"| Pwr "+pwr,Colors::TextSecondary,TextStyle::Caption);
    fb.drawText(x+538,y+9,"| PP "+pp,Colors::TextSecondary,TextStyle::Caption);
}

} // namespace UI::MovePickerRowUI
#endif
