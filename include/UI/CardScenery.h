#pragma once
// Scenery band at the top of a rounded card (Round 11, HOME game card).
//
// The artwork and its readability scrim are each drawn as ONE rounded path (top corners concentric
// with the card, bottom edge square) filled with an image pattern / colour. Nothing is drawn through
// a rectangular clip, so the image never protrudes past the card's rounded corners or its focus ring.

#include "UI/PKSEFramebuffer.h"
#include "UI/RoundedArtwork.h"

namespace UI::CardScenery {

// Card corner radius shared by the HOME selected-game card, its focus ring and its scenery band.
inline constexpr int kHomeCardRadius = 18;
// The scenery sits just inside the card border.
inline constexpr int kHomeSceneryInset = 2;
// Readability scrim over the artwork (dark glass).
inline constexpr Color kHomeSceneryScrim{5, 14, 30, 40};

// Draws `image` scale-to-cover into the top `bandHeight` pixels of the card, masked by the card's
// rounded top corners, then the scrim with the same mask. Returns the band geometry that was used.
inline RoundedArtwork::Band drawTopBand(PKSEFramebuffer& fb, int cardX, int cardY, int cardW, int bandHeight,
                                        int imgWidth, int imgHeight, const unsigned char* imageData,
                                        int cardRadius = kHomeCardRadius, int inset = kHomeSceneryInset,
                                        Color scrim = kHomeSceneryScrim) {
    const RoundedArtwork::Band band =
        RoundedArtwork::topBand(cardX, cardY, cardW, bandHeight, static_cast<float>(cardRadius), inset);
    if (band.rect.w <= 0 || band.rect.h <= 0 || !imageData || imgWidth <= 0 || imgHeight <= 0) return band;
    fb.drawImageRoundedCover(band.rect.x, band.rect.y, band.rect.w, band.rect.h, band.radii,
                             imgWidth, imgHeight, imageData, 4);
    fb.drawFilledRoundedRectVarying(band.rect.x, band.rect.y, band.rect.w, band.rect.h, band.radii, scrim);
    return band;
}

} // namespace UI::CardScenery
