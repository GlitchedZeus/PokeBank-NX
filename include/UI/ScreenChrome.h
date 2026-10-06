#ifndef UI_SCREEN_CHROME_H
#define UI_SCREEN_CHROME_H

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "UI/PKSEFramebuffer.h"
#include "UI/Common.h"
#include "UI/ControllerModel.h"
#include "UI/TouchInput.h"

namespace UI {
    // ---------------------------------------------------------------------------------------------
    // HOME-style screen chrome.
    //
    // Both bars are rounded SHEETS whose far edge runs off-screen, so only the edge facing the
    // content is curved: the header curves along its bottom, the nav bar along its top. A soft
    // shadow does the separating, which is why neither carries a hard divider rule any more.
    // ---------------------------------------------------------------------------------------------

    constexpr int kChromeRadius = 18;   // curve on the content-facing edge of both bars
    constexpr int kHeaderH      = 64;
    constexpr int kNavBarH      = 46;

    struct TouchGlyphHit {
        int x, y, w, h;
        std::string glyph;
    };

    // Visible controller glyph buttons collect geometry while the screen is rendered.
    // Content cards/rows are owned by the screen that draws them.
    inline std::vector<TouchGlyphHit> g_touchGlyphAccum;
    inline std::vector<TouchGlyphHit> g_touchGlyphHits;

    // Product Home's hero is rendered with many absolute coordinates. These two values let the
    // shared card helper place the entire already-established hero subtree under one NanoVG
    // translation while a finger is dragging it, then restore before the right-hand feature cards.
    inline bool g_productHeroTransformActive = false;
    inline bool g_productHeroSwipeRegistered = false;
    inline int g_productHeroDragXForDraw = 0;

    inline Color withAlpha(Color color, std::uint8_t alpha) {
        return Color(color.r, color.g, color.b, alpha);
    }

    inline void finishProductHeroTranslation(PKSEFramebuffer& fb) {
        if (!g_productHeroTransformActive) return;
        fb.popTransform();
        g_productHeroTransformActive = false;
    }

    // Shared PokeBank NX backdrop. It leaves the OLED theme genuinely black and keeps only the
    // low-alpha archive rings; the background now extends cleanly to the left edge.
    inline void drawAppBackdrop(PKSEFramebuffer& fb) {
        // A NanoVG frame resets transforms, so reset the matching bookkeeping at the frame boundary.
        g_productHeroTransformActive = false;
        // Input is consumed before draw. Begin each rendered frame with fresh direct-touch geometry;
        // controls drawn later in this same frame republish the targets used by the next update.
        g_touchGlyphAccum.clear();
        g_touchGlyphHits.clear();
        const int w = fb.getWidth();
        fb.clear(Colors::Background);
        fb.drawCircle(w - 58, 126, 112, withAlpha(Colors::BrandAccent, 24), 18);
        fb.drawCircle(w - 58, 126, 76, withAlpha(Colors::AccentSecondary, 20), 10);
    }

    struct ControllerHint {
        std::string button;
        std::string label;
    };

    inline std::string controllerGlyph(PokeBank::UIModel::ControllerButton button) {
        using PokeBank::UIModel::ControllerButton;
        switch (button) {
            case ControllerButton::DPad:  return "Up/Down";
            case ControllerButton::A:     return "A";
            case ControllerButton::B:     return "B";
            case ControllerButton::X:     return "X";
            case ControllerButton::Y:     return "Y";
            case ControllerButton::LR:    return "L/R";
            case ControllerButton::ZLZR:  return "ZL/ZR";
            case ControllerButton::Plus:  return "+";
            case ControllerButton::Minus: return "-";
        }
        return {};
    }

    inline std::vector<ControllerHint> controllerHints(PokeBank::UIModel::ControllerContext context) {
        std::vector<ControllerHint> result;
        for (const auto& binding : PokeBank::UIModel::controllerBindings(context))
            result.push_back({controllerGlyph(binding.button), std::string(binding.label)});
        return result;
    }

    // Reusable semantic surfaces. New UI code chooses purpose/elevation here rather than selecting
    // literal colors; the active OLED Black, Dark, or Light palette supplies the appearance.
    inline void drawPanelSurface(PKSEFramebuffer& fb, int x, int y, int w, int h,
                                 bool raised = false, int radius = 14) {
        if (raised) fb.drawSoftShadow(x, y, w, h, radius);
        fb.drawFilledRoundedRect(x, y, w, h, radius,
                                 raised ? Colors::SurfaceRaised : Colors::Surface);
        fb.drawRoundedRect(x, y, w, h, radius, Colors::Divider, 1);
    }

    inline void drawFocusedCard(PKSEFramebuffer& fb, int x, int y, int w, int h,
                                bool focused, int radius = 14) {
        // Exact Product Home hero geometry. Start one scoped translation here so all subsequent
        // hero artwork/text/buttons inherit the same finger displacement. The first right-side
        // feature card ends it below; drawNavHints also provides a defensive restore.
        const bool productHero = g_productHeroSwipeRegistered && x == 24 && y == 78 && w == 720 && h == 548;
        if (productHero && g_productHeroDragXForDraw != 0 && !g_productHeroTransformActive) {
            const int dx = std::clamp(g_productHeroDragXForDraw, -360, 360);
            fb.pushTranslation(static_cast<float>(dx), 0.0f);
            g_productHeroTransformActive = true;
        } else if (g_productHeroTransformActive && x >= 764) {
            finishProductHeroTranslation(fb);
        }

        if (focused) fb.drawSoftShadow(x, y, w, h, radius);
        // Focus never changes the card fill: teal outline + readable text carries selection.
        fb.drawFilledRoundedRect(x, y, w, h, radius, Colors::Surface);
        fb.drawRoundedRect(x, y, w, h, radius,
                           focused ? Colors::FocusBorder : Colors::Divider, focused ? 3 : 1);
    }

    inline void drawModalSurface(PKSEFramebuffer& fb, int x, int y, int w, int h,
                                 int radius = 18) {
        finishProductHeroTranslation(fb);
        fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight(), Color(0, 0, 0, 150));
        drawPanelSurface(fb, x, y, w, h, true, radius);
    }

    // --- Controller-button badges -----------------------------------------------------------------

    // Draw (or, with measureOnly, just measure) one controller badge, shaped like the real button:
    // face buttons are round, shoulders are rounded rectangles, +/- are round with a drawn bar, and
    // the d-pad is a cross with the unused axis dimmed. `cy` is the badge's vertical CENTRE.
    // Returns the width consumed, or 0 if the token isn't a button PKSE knows how to draw.
    inline int buttonGlyph(PKSEFramebuffer& fb, int x, int cy, const std::string& btn, bool measureOnly) {
        // Neutral controller glyphs follow the theme. Face buttons use stable familiar colors so
        // A/B/X/Y remain instantly distinguishable in Light, Dark and OLED Black.
        const Color fill = Colors::TextPrimary;
        const Color ink  = Colors::Panel;
        constexpr int kR = 12;              // face-button radius

        auto centred = [&](const std::string& s, int bx, int bw, Color textColor) {
            int tw, th; fb.measureText(s, tw, th, TextStyle::Caption);
            fb.drawText(bx + (bw - tw) / 2, cy - th / 2, s, textColor, TextStyle::Caption);
        };

        // Face buttons. These colors are presentation-only and never imply danger/success state.
        if (btn.size() == 1 && (btn[0] == 'A' || btn[0] == 'B' || btn[0] == 'X' || btn[0] == 'Y')) {
            Color faceFill = fill;
            Color faceInk = Colors::White;
            if (btn[0] == 'A') faceFill = Color(62, 166, 96);
            else if (btn[0] == 'B') faceFill = Color(210, 72, 78);
            else if (btn[0] == 'X') faceFill = Color(70, 132, 214);
            else {
                faceFill = Color(226, 176, 54);
                faceInk = Color(40, 32, 12);
            }
            if (!measureOnly) {
                fb.drawFilledCircle(x + kR, cy, kR, faceFill);
                centred(btn, x, kR * 2, faceInk);
            }
            return kR * 2;
        }

        // Plus / Minus. The bars are drawn rather than typed -- Nunito's '+' and '-' are far too
        // thin to read at badge size, and '-' sits at x-height instead of centred.
        if (btn == "+" || btn == "Plus" || btn == "-" || btn == "Minus") {
            if (!measureOnly) {
                fb.drawFilledCircle(x + kR, cy, kR, fill);
                fb.drawFilledRoundedRect(x + kR - 6, cy - 1, 13, 3, 1, ink);
                if (btn == "+" || btn == "Plus")
                    fb.drawFilledRoundedRect(x + kR - 1, cy - 6, 3, 13, 1, ink);
            }
            return kR * 2;
        }

        // Shoulders, and the slash pairs the hints use ("L/R", "ZL/ZR"): rounded rects sized to text.
        if (btn == "L" || btn == "R" || btn == "ZL" || btn == "ZR" || btn == "L/R" || btn == "ZL/ZR") {
            int tw, th; fb.measureText(btn, tw, th, TextStyle::Caption);
            const int w = tw + 14, h = 22;
            if (!measureOnly) { fb.drawFilledRoundedRect(x, cy - h / 2, w, h, 7, fill); centred(btn, x, w, ink); }
            return w;
        }

        // D-pad + Left Stick. NavigationRepeat feeds both through the same directional contract, so
        // the shared legend shows that parity instead of implying D-pad-only navigation.
        if (btn == "Arrows" || btn == "D-Pad" || btn == "D-pad" || btn == "D-pad/Stick" ||
            btn == "Up/Down" || btn == "Left/Right") {
            constexpr int s = 24, a = 9, gap = 5, lsW = 26, lsH = 22;
            if (!measureOnly) {
                const Color dim(fill.r, fill.g, fill.b, 70);
                const bool vOnly = (btn == "Up/Down"), hOnly = (btn == "Left/Right");
                const int vx = x + (s - a) / 2, vy = cy - s / 2;
                const int hx = x,               hy = cy - a / 2;
                if (vOnly) {
                    fb.drawFilledRoundedRect(hx, hy, s, a, 3, dim);
                    fb.drawFilledRoundedRect(vx, vy, a, s, 3, fill);
                } else if (hOnly) {
                    fb.drawFilledRoundedRect(vx, vy, a, s, 3, dim);
                    fb.drawFilledRoundedRect(hx, hy, s, a, 3, fill);
                } else {
                    fb.drawFilledRoundedRect(vx, vy, a, s, 3, fill);
                    fb.drawFilledRoundedRect(hx, hy, s, a, 3, fill);
                }
                const int lsX = x + s + gap;
                fb.drawFilledRoundedRect(lsX, cy - lsH / 2, lsW, lsH, 7, fill);
                centred("LS", lsX, lsW, ink);
            }
            return s + gap + lsW;
        }

        // A single d-pad direction: a rounded-square badge with a geometric triangle arrow. Used by
        // the edit dialogs' -1 / +1 steps (the shoulders take the +/-10 and +/-100 steps).
        if (btn == "Left" || btn == "Right" || btn == "Up" || btn == "Down") {
            constexpr int s = 22;
            if (!measureOnly) {
                fb.drawFilledRoundedRect(x, cy - s / 2, s, s, 6, fill);
                const std::string tri = btn == "Left"  ? "\xE2\x97\x80"    // ◀
                                      : btn == "Right" ? "\xE2\x96\xB6"    // ▶
                                      : btn == "Up"    ? "\xE2\x96\xB2"    // ▲
                                      :                  "\xE2\x96\xBC";   // ▼
                // Measure AND draw the arrow at Caption size -- drawSymbol otherwise defaults to Body,
                // which overflowed this 22px badge and mis-centred the glyph (the ◀ was clipping away).
                int tw, th; fb.measureText(tri, tw, th, TextStyle::Caption);
                fb.drawSymbol(x + (s - tw) / 2, cy - th / 2, tri, ink, TextStyle::Caption);
            }
            return s;
        }
        return 0;   // not a button we have a badge for
    }

    inline int buttonGlyphWidth(PKSEFramebuffer& fb, const std::string& btn) {
        return buttonGlyph(fb, 0, 0, btn, true);
    }

    // A pressable button that carries its controller badge ON the button (glyph + label, centred).
    // Geometry is published immediately as well as accumulated for the next footer commit. This is
    // important for modal buttons drawn after a footer: they must still be real direct-touch targets.
    inline void drawGlyphButton(PKSEFramebuffer& fb, int bx, int by, int bw, int bh,
                                const std::string& glyph, const std::string& label,
                                Color fill = Colors::PanelAlt, Color textColor = Colors::Text) {
        const TouchGlyphHit hit{bx, by, bw, bh, glyph};
        g_touchGlyphAccum.push_back(hit);
        g_touchGlyphHits.push_back(hit);
        fb.drawFilledRoundedRect(bx, by, bw, bh, 8, fill);
        fb.drawRoundedRect(bx, by, bw, bh, 8, Colors::Border, 1);
        const int gw = buttonGlyphWidth(fb, glyph);
        int lw, lh; fb.measureText(label, lw, lh);
        const int gx = bx + (bw - (gw + 10 + lw)) / 2;
        buttonGlyph(fb, gx, by + bh / 2, glyph, false);
        fb.drawText(gx + gw + 10, by + (bh - lh) / 2, label, textColor);
    }

    // --- Tappable badges --------------------------------------------------------------------
    //
    // Every screen already publishes contextual controller hints. Single-button hints use a
    // release-confirmed tap; paired/directional hints resolve on release as well. Existing controller
    // handling remains the single action path, so touch does not grow a second set of save/editor
    // behaviors.
    struct NavHit { int x, y, w, h; uint64_t button; };
    enum class NavGestureKind : std::uint8_t { None, UpDown, LeftRight, DPad, LR, ZLZR };
    struct NavGestureHit { int x, y, w, h, glyphW; NavGestureKind kind; };
    inline std::vector<NavHit> g_navHits;
    inline std::vector<NavGestureHit> g_navGestureHits;
    inline int g_navSurfaceX = 0;
    inline int g_navSurfaceW = 0;
    inline int g_navContentBottom = 0;
    inline uint64_t g_contentSwipeMask = 0;
    inline bool g_contentSwipeUsesShoulders = false;
    inline bool g_contentSwipeReleaseOnly = false;
    inline bool g_quickGamesDrawerSwipe = false;
    inline uint64_t g_rightEdgeSwipeButton = 0;
    inline bool g_contentDragActive = false;
    inline bool g_contentDragMoved = false;
    inline int g_contentDragLastX = 0;
    inline int g_contentDragLastY = 0;
    inline int g_contentDragVisualX = 0;
    inline int g_contentDragVisualY = 0;
    inline int g_contentSwipeX = 0;
    inline int g_contentSwipeY = kHeaderH;
    inline int g_contentSwipeW = 0;
    inline int g_contentSwipeH = 0;

    inline int contentDragVisualX() noexcept { return g_contentDragActive ? g_contentDragVisualX : 0; }
    inline int contentDragVisualY() noexcept { return g_contentDragActive ? g_contentDragVisualY : 0; }

    inline uint64_t navButtonFor(const std::string& btn) {
        if (btn == "A") return HidNpadButton_A;
        if (btn == "B") return HidNpadButton_B;
        if (btn == "X") return HidNpadButton_X;
        if (btn == "Y") return HidNpadButton_Y;
        if (btn == "+"  || btn == "Plus")  return HidNpadButton_Plus;
        if (btn == "-"  || btn == "Minus") return HidNpadButton_Minus;
        if (btn == "L")  return HidNpadButton_L;
        if (btn == "R")  return HidNpadButton_R;
        if (btn == "ZL") return HidNpadButton_ZL;
        if (btn == "ZR") return HidNpadButton_ZR;
        if (btn == "Left")  return HidNpadButton_Left;
        if (btn == "Right") return HidNpadButton_Right;
        if (btn == "Up")    return HidNpadButton_Up;
        if (btn == "Down")  return HidNpadButton_Down;
        return 0;
    }

    inline NavGestureKind navGestureFor(const std::string& btn) {
        if (btn == "Up/Down") return NavGestureKind::UpDown;
        if (btn == "Left/Right") return NavGestureKind::LeftRight;
        if (btn == "Arrows" || btn == "D-Pad" || btn == "D-pad" || btn == "D-pad/Stick")
            return NavGestureKind::DPad;
        if (btn == "L/R") return NavGestureKind::LR;
        if (btn == "ZL/ZR") return NavGestureKind::ZLZR;
        return NavGestureKind::None;
    }

    inline bool navContains(int px, int py, int x, int y, int w, int h) {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    inline bool contentSwipeContains(int px, int py) noexcept {
        return g_contentSwipeW > 0 && g_contentSwipeH > 0 &&
               px >= g_contentSwipeX && px < g_contentSwipeX + g_contentSwipeW &&
               py >= g_contentSwipeY && py < g_contentSwipeY + g_contentSwipeH;
    }

    inline uint64_t horizontalContentButton(bool towardNext) noexcept {
        if (g_contentSwipeUsesShoulders)
            return towardNext ? HidNpadButton_R : HidNpadButton_L;
        return towardNext ? HidNpadButton_Right : HidNpadButton_Left;
    }

    // Hit-test the badges captured during the PREVIOUS frame's draw. Footer buttons resolve on
    // release, browser/list content steps while the finger is moving, and a stationary tap on a
    // visible card follows the same spatial navigation path before pressing A. Screen-owned editor
    // and storage drag surfaces remain excluded from this generic registry.
    inline uint64_t navTouchButton(const TouchInput& touch) {
        const bool startsInContent = g_contentSwipeMask != 0 && contentSwipeContains(touch.startX(), touch.startY());

        if (touch.justTouchedDown()) {
            g_contentDragActive = false;
            g_contentDragMoved = false;
            g_contentDragLastX = touch.x();
            g_contentDragLastY = touch.y();
            g_contentDragVisualX = 0;
            g_contentDragVisualY = 0;

            bool quickCloseEdge = false;
            if (g_quickGamesDrawerSwipe && g_navSurfaceW > 0) {
                const int drawerLeft = g_navSurfaceX + g_navSurfaceW - 520;
                constexpr int kDrawerEdgeCapture = 112;
                quickCloseEdge = touch.startX() >= drawerLeft &&
                                 touch.startX() < drawerLeft + kDrawerEdgeCapture;
            }
            if (startsInContent && !quickCloseEdge)
                g_contentDragActive = true;
            return 0;
        }

        if (touch.isDown()) {
            if (!g_contentDragActive) return 0;

            g_contentDragVisualX = touch.x() - touch.startX();
            g_contentDragVisualY = touch.y() - touch.startY();
            g_productHeroDragXForDraw = g_contentSwipeUsesShoulders
                ? g_contentDragVisualX : 0;

            // Product Home's hero is a carousel: it follows the finger continuously, but commits
            // exactly one previous/next game only on release rather than cycling titles mid-drag.
            if (g_contentSwipeReleaseOnly) return 0;

            const int dx = touch.x() - g_contentDragLastX;
            const int dy = touch.y() - g_contentDragLastY;
            const int ax = dx < 0 ? -dx : dx;
            const int ay = dy < 0 ? -dy : dy;
            // Twenty-four physical pixels keeps focus tracking a finger instead of lagging by a
            // whole card/row, while leaving actual pixel translation to screen-owned renderers.
            constexpr int kLiveDragStep = 24;

            if (ax >= kLiveDragStep && ax * 3 >= ay * 4) {
                const uint64_t direction = horizontalContentButton(dx < 0);
                if (g_contentSwipeMask & direction) {
                    g_contentDragLastX = touch.x();
                    g_contentDragLastY = touch.y();
                    g_contentDragMoved = true;
                    return direction;
                }
            }
            if (ay >= kLiveDragStep && ay * 3 >= ax * 4) {
                const uint64_t direction = dy < 0 ? HidNpadButton_Down : HidNpadButton_Up;
                if (g_contentSwipeMask & direction) {
                    g_contentDragLastX = touch.x();
                    g_contentDragLastY = touch.y();
                    g_contentDragMoved = true;
                    return direction;
                }
            }
            return 0;
        }

        if (!touch.justReleased()) return 0;

        const bool contentDragMoved = g_contentDragMoved;
        const bool releaseOnly = g_contentSwipeReleaseOnly;
        g_contentDragActive = false;
        g_contentDragMoved = false;
        g_contentDragVisualX = 0;
        g_contentDragVisualY = 0;
        g_productHeroDragXForDraw = 0;
        if (contentDragMoved && !releaseOnly) return 0;

        if (!touch.dragged()) {
            // Any controller-glyph button visibly drawn by the active surface is a real touch button.
            for (const auto& button : g_touchGlyphHits) {
                if (navContains(touch.startX(), touch.startY(), button.x, button.y, button.w, button.h) &&
                    navContains(touch.x(), touch.y(), button.x, button.y, button.w, button.h)) {
                    const uint64_t mapped = navButtonFor(button.glyph);
                    if (mapped) return mapped;
                }
            }

            for (const NavHit& h : g_navHits) {
                if (navContains(touch.startX(), touch.startY(), h.x, h.y, h.w, h.h) &&
                    navContains(touch.x(), touch.y(), h.x, h.y, h.w, h.h))
                    return h.button;
            }
        }

        // Product Home exposes Y = Quick Games. When that exact action is present, a
        // deliberate swipe in from the physical right edge maps to the same Y press.
        // Keeping this semantic registration in the footer means overlays that replace
        // the footer automatically disable the gesture instead of leaving it live behind them.
        if (g_rightEdgeSwipeButton != 0 && g_navSurfaceW > 0 && touch.dragged()) {
            const int dx = touch.x() - touch.startX();
            const int dy = touch.y() - touch.startY();
            const int ay = dy < 0 ? -dy : dy;
            const int rightEdge = g_navSurfaceX + g_navSurfaceW;
            constexpr int kEdgeCapture = 128;
            constexpr int kOpenDistance = 120;
            if (touch.startX() >= rightEdge - kEdgeCapture && touch.startX() < rightEdge &&
                dx <= -kOpenDistance && -dx > ay * 2)
                return g_rightEdgeSwipeButton;
        }

        // Quick Games is a right-side drawer. A deliberate push from its inner edge back toward
        // the physical right edge closes it through the existing B path. This is intentionally
        // narrower than normal grid swiping so moving left/right between covers remains easy.
        if (g_quickGamesDrawerSwipe && g_navSurfaceW > 0 && touch.dragged()) {
            const int dx = touch.x() - touch.startX();
            const int dy = touch.y() - touch.startY();
            const int ay = dy < 0 ? -dy : dy;
            const int drawerLeft = g_navSurfaceX + g_navSurfaceW - 520;
            constexpr int kDrawerEdgeCapture = 112;
            constexpr int kCloseDistance = 120;
            if (touch.startX() >= drawerLeft && touch.startX() < drawerLeft + kDrawerEdgeCapture &&
                dx >= kCloseDistance && dx > ay * 2)
                return HidNpadButton_B;
        }

        // A very fast flick can travel from touch-down to release between two rendered frames and
        // therefore never cross a live step while held. Product Home also intentionally resolves
        // its carousel here so exactly one L/R action is committed after the visual drag.
        if (g_contentSwipeMask != 0 && touch.dragged() &&
            contentSwipeContains(touch.startX(), touch.startY())) {
            const int dx = touch.x() - touch.startX();
            const int dy = touch.y() - touch.startY();
            const int ax = dx < 0 ? -dx : dx;
            const int ay = dy < 0 ? -dy : dy;
            constexpr int kContentSwipeDistance = 72;
            if (ax >= kContentSwipeDistance && ax * 3 >= ay * 4) {
                const uint64_t direction = horizontalContentButton(dx < 0);
                if (g_contentSwipeMask & direction) return direction;
            }
            if (ay >= kContentSwipeDistance && ay * 3 >= ax * 4) {
                const uint64_t direction = dy < 0 ? HidNpadButton_Down : HidNpadButton_Up;
                if (g_contentSwipeMask & direction) return direction;
            }
        }

        for (const NavGestureHit& h : g_navGestureHits) {
            if (!navContains(touch.startX(), touch.startY(), h.x, h.y, h.w, h.h)) continue;

            const int dx = touch.x() - touch.startX();
            const int dy = touch.y() - touch.startY();
            const int ax = dx < 0 ? -dx : dx;
            const int ay = dy < 0 ? -dy : dy;

            if (h.kind == NavGestureKind::UpDown) {
                if (touch.dragged() && ay >= ax) return dy < 0 ? HidNpadButton_Up : HidNpadButton_Down;
                return touch.startY() < h.y + h.h / 2 ? HidNpadButton_Up : HidNpadButton_Down;
            }
            if (h.kind == NavGestureKind::LeftRight) {
                if (touch.dragged() && ax >= ay) return dx < 0 ? HidNpadButton_Left : HidNpadButton_Right;
                return touch.startX() < h.x + h.w / 2 ? HidNpadButton_Left : HidNpadButton_Right;
            }
            if (h.kind == NavGestureKind::LR || h.kind == NavGestureKind::ZLZR) {
                const bool left = touch.dragged() && ax >= ay
                    ? dx < 0
                    : touch.startX() < h.x + h.w / 2;
                if (h.kind == NavGestureKind::LR)
                    return left ? HidNpadButton_L : HidNpadButton_R;
                return left ? HidNpadButton_ZL : HidNpadButton_ZR;
            }
            if (h.kind == NavGestureKind::DPad) {
                if (touch.dragged()) {
                    if (ax >= ay) return dx < 0 ? HidNpadButton_Left : HidNpadButton_Right;
                    return dy < 0 ? HidNpadButton_Up : HidNpadButton_Down;
                }

                // A no-drag tap is meaningful only on the actual d-pad glyph. The rest of a
                // "D-pad/Stick: Navigate" segment is a generous drag surface, not an invisible
                // right-arrow button just because the label sits to the glyph's right.
                constexpr int dpadW = 24;
                if (touch.startX() >= h.x && touch.startX() < h.x + std::min(dpadW, h.glyphW)) {
                    const int rx = touch.startX() - (h.x + dpadW / 2);
                    const int ry = touch.startY() - (h.y + h.h / 2);
                    const int arx = rx < 0 ? -rx : rx;
                    const int ary = ry < 0 ? -ry : ry;
                    if (arx >= ary) return rx < 0 ? HidNpadButton_Left : HidNpadButton_Right;
                    return ry < 0 ? HidNpadButton_Up : HidNpadButton_Down;
                }
                return 0;
            }
        }
        return 0;
    }

    // --- Hint layout ------------------------------------------------------------------------------

    // Lay out a "Btn: Label  |  Btn: Label" hint as badge+label pairs, centred within [x, x+w] on
    // `cy`. A segment with no colon (e.g. "HOLDING") is a state marker and renders as accent text.
    // Shared by the screen nav bar and the dialog footer so the two always match.
    inline void drawNavHints(PKSEFramebuffer& fb, int x, int w, int cy, const std::string& hint) {
        // Never let a hero translation leak into footer/overlay chrome even if a future Product Home
        // layout stops drawing the right-hand feature cards.
        finishProductHeroTranslation(fb);

        struct Seg { std::string btn, label; int glyphW, labelW; };

        // The footer commits glyph buttons drawn before it; buttons rendered later publish directly
        // from drawGlyphButton(). Content cards/rows remain owned by their screen.
        g_touchGlyphHits = g_touchGlyphAccum;
        g_touchGlyphAccum.clear();

        g_navHits.clear();
        g_navGestureHits.clear();
        g_navSurfaceX = x;
        g_navSurfaceW = w;
        g_navContentBottom = cy - TouchTargetMin / 2 - 8;
        g_contentSwipeMask = 0;
        g_contentSwipeUsesShoulders = false;
        g_contentSwipeReleaseOnly = false;
        g_quickGamesDrawerSwipe = false;
        g_rightEdgeSwipeButton = 0;
        g_contentSwipeX = x;
        g_contentSwipeY = kHeaderH;
        g_contentSwipeW = w;
        g_contentSwipeH = std::max(0, g_navContentBottom - kHeaderH);

        const uint64_t allDirections = HidNpadButton_Up | HidNpadButton_Down |
                                       HidNpadButton_Left | HidNpadButton_Right;
        const uint64_t verticalDirections = HidNpadButton_Up | HidNpadButton_Down;
        const uint64_t horizontalShoulders = HidNpadButton_L | HidNpadButton_R;

        // Product Home's selected-game hero card is a real touch carousel. Limit capture to that
        // exact card, keep the card physically attached to the finger while held, and resolve one
        // existing L/R change-game action only after release.
        const bool productHomeHero =
            hint.find("L/R: Change Game") != std::string::npos &&
            hint.find("A: Open") != std::string::npos &&
            hint.find("Y: Quick Games") != std::string::npos;
        g_productHeroSwipeRegistered = productHomeHero;
        if (productHomeHero) {
            g_contentSwipeMask = horizontalShoulders;
            g_contentSwipeUsesShoulders = true;
            g_contentSwipeReleaseOnly = true;
            g_contentSwipeX = 24;
            g_contentSwipeY = 78;
            g_contentSwipeW = 720;
            g_contentSwipeH = 548;
        }

        // Quick Games: three-column drawer with no competing content drag handler.
        if (hint.find("D-pad/Stick: Choose") != std::string::npos &&
            hint.find("X: Save / Source") != std::string::npos &&
            hint.find("B: Close") != std::string::npos) {
            g_contentSwipeMask = allDirections;
            g_contentSwipeUsesShoulders = false;
            g_contentSwipeReleaseOnly = false;
            g_quickGamesDrawerSwipe = true;
        }

        // Save/browser overlays in SaveSelectScreen have no content drag handlers, so generic
        // swipes and direct card taps are safe here. Exact labels keep this away from TrainerView
        // storage/editor surfaces, which own their own drag and direct-row semantics.
        const bool classicGamesBrowser =
            hint.find("Y: Sort") != std::string::npos &&
            hint.find("+: Favorite") != std::string::npos &&
            hint.find("ZR: Launch") != std::string::npos &&
            hint.find("B: Back") != std::string::npos;
        const bool currentGameGrid =
            hint.find("ZR: Launch") != std::string::npos &&
            hint.find("+: Close Menu") != std::string::npos &&
            hint.find("B: Home") != std::string::npos;
        if (classicGamesBrowser || currentGameGrid) {
            g_contentSwipeMask = allDirections;
            g_contentSwipeUsesShoulders = false;
            g_contentSwipeReleaseOnly = false;
        }

        const bool profilePicker = hint.find("Choose Profile") != std::string::npos &&
                                   hint.find("Use Profile") != std::string::npos;
        const bool gameFilePicker = hint.find("Open / Link") != std::string::npos &&
                                    hint.find("Up Folder") != std::string::npos &&
                                    hint.find("Start Folder") != std::string::npos;
        const bool saveInstanceList = hint.find("Choose Save") != std::string::npos &&
            (hint.find("Source Details") != std::string::npos ||
             hint.find("Source Setup") != std::string::npos ||
             hint.find("Refresh Saves") != std::string::npos);
        const bool saveAssignmentList = hint.find("Assign to This Profile") != std::string::npos &&
                                        hint.find("X: Refresh") != std::string::npos;
        if (profilePicker || gameFilePicker || saveInstanceList || saveAssignmentList) {
            g_contentSwipeMask = verticalDirections;
            g_contentSwipeUsesShoulders = false;
            g_contentSwipeReleaseOnly = false;
        }

        auto trim = [](const std::string& s) {
            const size_t a = s.find_first_not_of(" \t");
            if (a == std::string::npos) return std::string();
            return s.substr(a, s.find_last_not_of(" \t") - a + 1);
        };

        std::vector<Seg> segs;
        for (size_t i = 0; i <= hint.size(); ) {
            const size_t bar = hint.find('|', i);
            const std::string tok =
                trim(hint.substr(i, bar == std::string::npos ? std::string::npos : bar - i));
            if (!tok.empty()) {
                Seg s{};
                const size_t colon = tok.find(':');
                if (colon == std::string::npos) {
                    s.label = tok;
                } else {
                    s.btn   = trim(tok.substr(0, colon));
                    s.label = trim(tok.substr(colon + 1));
                }
                s.glyphW = s.btn.empty() ? 0 : buttonGlyphWidth(fb, s.btn);
                // A button we have no badge for still has to be readable: fall back to plain text
                // rather than silently dropping the button name and leaving a bare verb.
                if (s.glyphW == 0 && !s.btn.empty()) s.label = s.btn + ": " + s.label;
                int th; fb.measureText(s.label, s.labelW, th, TextStyle::Caption);
                segs.push_back(s);
            }
            if (bar == std::string::npos) break;
            i = bar + 1;
        }
        if (segs.empty()) return;

        constexpr int kGapGlyph = 8, kGapItemMax = 26, kGapItemMin = 10, kPadX = 20;
        int fixed = 0;
        for (const Seg& s : segs) fixed += s.glyphW + (s.glyphW ? kGapGlyph : 0) + s.labelW;

        const int n = static_cast<int>(segs.size());
        int gap = kGapItemMax;
        if (n > 1) gap = std::clamp((w - kPadX * 2 - fixed) / (n - 1), kGapItemMin, kGapItemMax);

        int cx = x + std::max(kPadX, (w - (fixed + gap * (n - 1))) / 2);
        for (const Seg& s : segs) {
            const int segX = cx;
            if (s.glyphW) { buttonGlyph(fb, cx, cy, s.btn, false); cx += s.glyphW + kGapGlyph; }
            int tw, th; fb.measureText(s.label, tw, th, TextStyle::Caption);
            fb.drawText(cx, cy - th / 2, s.label, s.glyphW ? Colors::Text : Colors::Accent,
                        TextStyle::Caption);
            cx += s.labelW;

            // Badge + label is one fingertip-sized target. All footer actions resolve on release so
            // touch-down never steals a gesture from the content above it.
            const int hitY = cy - TouchTargetMin / 2;
            const int hitW = cx - segX;
            const uint64_t button = s.glyphW ? navButtonFor(s.btn) : 0;
            if (button) {
                g_navHits.push_back({segX, hitY, hitW, TouchTargetMin, button});
                if (s.btn == "Y" && s.label == "Quick Games")
                    g_rightEdgeSwipeButton = button;
            } else if (s.glyphW) {
                const NavGestureKind kind = navGestureFor(s.btn);
                if (kind != NavGestureKind::None)
                    g_navGestureHits.push_back({segX, hitY, hitW, TouchTargetMin, s.glyphW, kind});
            }
            cx += gap;
        }
    }

    // --- Bars -------------------------------------------------------------------------------------

    // PokeBank NX identity bar. The compact vector archive-ball mark avoids a required image asset,
    // keeps every theme crisp, and removes inherited PKSE product branding from the normal path.
    inline void drawTitleBar(PKSEFramebuffer& fb, const std::string& subtitle) {
        fb.drawSoftShadow(0, -40, fb.getWidth(), kHeaderH + 40, kChromeRadius);
        fb.drawFilledRoundedRect(0, -kChromeRadius, fb.getWidth(), kHeaderH + kChromeRadius,
                                 kChromeRadius, Colors::SurfaceRaised);

        constexpr int cx = 34, cy = 30, r = 19;
        constexpr Color ballWhite(248, 248, 248);
        constexpr Color ballBand(24, 25, 29);

        // Classic Poké Ball: white base, clipped red top, dark centre band and white button.
        // Drawing from semantic AccentPrimary keeps the red consistent across all three themes.
        fb.drawFilledCircle(cx, cy, r, ballWhite);
        fb.setClipRect(cx - r, cy - r, r * 2, r);
        fb.drawFilledCircle(cx, cy, r, Colors::BrandAccent);
        fb.clearClip();
        fb.drawFilledRect(cx - r, cy - 3, r * 2, 6, ballBand);
        fb.drawFilledCircle(cx, cy, 8, ballBand);
        fb.drawFilledCircle(cx, cy, 5, ballWhite);
        fb.drawCircle(cx, cy, r, ballBand, 2);
        fb.drawCircle(cx, cy, 5, ballBand, 1);

        constexpr int brandX = 62;
        fb.drawText(brandX, 8, "PokeBank", Colors::TextPrimary, TextStyle::Title);
        int brandW, brandH; fb.measureText("PokeBank", brandW, brandH, TextStyle::Title);
        const int nxX = brandX + brandW + 8;
        fb.drawFilledRoundedRect(nxX, 15, 40, 28, 9, Colors::BrandAccent);
        int nxW, nxH; fb.measureText("NX", nxW, nxH, TextStyle::Caption);
        fb.drawText(nxX + (40 - nxW) / 2, 15 + (28 - nxH) / 2, "NX",
                    Colors::Surface, TextStyle::Caption);

        if (!subtitle.empty())
            fb.drawText(nxX + 56, 23, subtitle, Colors::TextSecondary, TextStyle::Caption);

        constexpr int badgeW = 112, badgeH = 26;
        const int badgeX = fb.getWidth() - badgeW - 22;
        fb.drawFilledRoundedRect(badgeX, 18, badgeW, badgeH, 10,
                                 withAlpha(Colors::Info, 45));
        fb.drawRoundedRect(badgeX, 18, badgeW, badgeH, 10, Colors::Info, 1);
        int roW, roH; fb.measureText("LIVE LOCKED", roW, roH, TextStyle::Caption);
        fb.drawText(badgeX + (badgeW - roW) / 2, 18 + (badgeH - roH) / 2,
                    "LIVE LOCKED", Colors::Info, TextStyle::Caption);
    }

    // Bottom nav bar: a sheet that curves along its top edge, carrying the controller badges.
    inline void drawNavBar(PKSEFramebuffer& fb, const std::string& hint) {
        finishProductHeroTranslation(fb);
        const int W = fb.getWidth(), barY = fb.getHeight() - kNavBarH;
        fb.drawSoftShadow(0, barY, W, kNavBarH + 40, kChromeRadius);
        fb.drawFilledRoundedRect(0, barY, W, kNavBarH + kChromeRadius, kChromeRadius, Colors::Panel);
        drawNavHints(fb, 0, W, barY + kNavBarH / 2, hint);
    }

    inline void drawNavBar(PKSEFramebuffer& fb, const std::vector<ControllerHint>& hints) {
        std::string serialized;
        for (const ControllerHint& hint : hints) {
            if (hint.button.empty() || hint.label.empty()) continue;
            if (!serialized.empty()) serialized += "  |  ";
            serialized += hint.button + ": " + hint.label;
        }
        drawNavBar(fb, serialized);
    }

    inline void drawNavBar(PKSEFramebuffer& fb, std::initializer_list<ControllerHint> hints) {
        drawNavBar(fb, std::vector<ControllerHint>(hints));
    }

    inline void drawInfoOverlay(PKSEFramebuffer& fb, const std::string& title,
                                const std::vector<std::string>& lines) {
        constexpr int w = 700;
        const int h = std::min(520, 140 + static_cast<int>(lines.size()) * 38);
        const int x = (fb.getWidth() - w) / 2;
        const int y = (fb.getHeight() - h) / 2;
        drawModalSurface(fb, x, y, w, h);
        fb.drawText(x + 28, y + 18, "POKEBANK NX  /  HELP", Colors::BrandAccent,
                    TextStyle::Caption);
        fb.drawText(x + 28, y + 44, title, Colors::TextPrimary, TextStyle::Heading);
        fb.drawFilledRoundedRect(x + 28, y + 86, w - 56, 3, 2, Colors::BrandAccent);
        int ly = y + 108;
        for (const std::string& line : lines) {
            fb.drawText(x + 30, ly, line, Colors::TextSecondary, TextStyle::Body);
            ly += 38;
        }
        drawNavBar(fb, {{"B", "Close"}});
    }

    // A HOME-style selectable list tile. Selection never tints the tile: the normal dark surface
    // stays put, a teal outline identifies focus, and selected text remains bright/readable.
    inline void drawHomeTile(PKSEFramebuffer& fb, int x, int y, int w, int h,
                             const std::string& label, bool selected, bool accent = false, bool enabled = true) {
        fb.drawSoftShadow(x, y, w, h, h / 2);
        fb.drawPill(x, y, w, h, Colors::PanelAlt);
        if (selected) fb.drawPillBorder(x, y, w, h, Colors::FocusBorder, 2);
        else if (accent) fb.drawPillBorder(x, y, w, h, Colors::Accent, 2);
        const Color txt = !enabled ? Colors::TextDim
                        : selected  ? Colors::SelectedText
                        : accent    ? Colors::Accent
                        :             Colors::Text;
        int lx = x + 28;
        if (selected) { fb.drawSymbol(x + 20, y + h / 2 - 12, "\xE2\x96\xB6", Colors::FocusBorder); lx = x + 48; }
        int lw, lh; fb.measureText(label, lw, lh, TextStyle::Body);
        fb.drawText(lx, y + (h - lh) / 2, label, txt, TextStyle::Body);
    }

    // A thin scrollbar thumb on a scrolling viewport's right edge, drawn ONLY when the content
    // overflows. All pixels: `x` is the thumb's left edge, [trackY, trackY + trackH] the viewport,
    // contentH the full content height, scroll the current offset. Item lists pass contentH =
    // totalItems * rowH and scroll = firstItem * rowH. One helper so every scrolling surface in the
    // app gets the same thumb the details editor uses.
    inline void drawScrollbar(PKSEFramebuffer& fb, int x, int trackY, int trackH, int contentH, int scroll) {
        if (contentH <= trackH || trackH <= 0) return;
        const int maxS = contentH - trackH;
        int s = scroll; if (s < 0) s = 0; if (s > maxS) s = maxS;
        const int thumbH = std::max(24, trackH * trackH / contentH);
        const int thumbY = trackY + (trackH - thumbH) * s / maxS;
        fb.drawFilledRoundedRect(x, thumbY, 3, thumbH, 2, Colors::Border);
    }
}

#endif