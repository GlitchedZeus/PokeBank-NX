#ifndef POKEBANK_UI_ITEM_PICKER_ARTWORK_H
#define POKEBANK_UI_ITEM_PICKER_ARTWORK_H

#include "UI/SpriteManager.h"
#include "UI/PKSEFramebuffer.h"
#include <string>

namespace UI::ItemPickerArtwork {

// Draw the TRUE, matching, pinned upstream item icon when bundled in RomFS.
// Do not use a generic ball or another item's picture as a fake substitute.
// This is shared by Gen II/III/IV Held Item pickers and Gen III/IV Ball
// pickers. Generation I has no native held-item/ball field to expose.
inline void draw(PKSEFramebuffer& fb, int right, int y, int px,
                 const std::string& exactName) {
    if(exactName.empty())return;
    auto* art=SpriteManager::getItemSprite(exactName);
    if(!art || !art->data || art->width<=0 || art->height<=0)return;
    fb.drawSpriteStaticContained(right-px,y,px,px,
                                 art->width,art->height,art->data,art->channels);
}

} // namespace UI::ItemPickerArtwork
#endif
