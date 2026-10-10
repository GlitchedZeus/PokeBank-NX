#ifndef POKEBANK_UI_ITEM_PICKER_ARTWORK_H
#define POKEBANK_UI_ITEM_PICKER_ARTWORK_H

#include "UI/SpriteManager.h"
#include "UI/PKSEFramebuffer.h"
#include <string>
#include <cctype>

namespace UI::ItemPickerArtwork {

// Draw the TRUE, matching, pinned upstream item icon when bundled in RomFS.
// Do not use a generic ball or another item's picture as a fake substitute.
// This is shared by Gen II/III/IV Held Item pickers and Gen III/IV Ball
// pickers. Generation I has no native held-item/ball field to expose.
inline void draw(PKSEFramebuffer& fb, int right, int y, int px,
                 const std::string& exactName) {
    if(exactName.empty())return;
    auto* art=SpriteManager::getItemSprite(exactName);
    if(!art || !art->data || art->width<=0 || art->height<=0) {
        // Exact numbered TM artwork is absent from the pinned source.
        // Draw a neutral machine disc rather than assigning an incorrect type.
        const bool machine = exactName.size() >= 3 &&
            ((exactName[0] == 'T' && exactName[1] == 'M') ||
             (exactName[0] == 'H' && exactName[1] == 'M')) &&
            std::isdigit(static_cast<unsigned char>(exactName[2]));
        if (machine) {
            const int cx = right - px / 2, cy = y + px / 2;
            fb.drawCircle(cx, cy, px / 2 - 2, Colors::TextSecondary, 3);
            fb.drawCircle(cx, cy, px / 4, Colors::Divider, 2);
            fb.drawCircle(cx, cy, 2, Colors::Text, 2);
        }
        return;
    }
    fb.drawSpriteStaticContained(right-px,y,px,px,
                                 art->width,art->height,art->data,art->channels);
}

} // namespace UI::ItemPickerArtwork
#endif
