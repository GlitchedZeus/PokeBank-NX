// Host UI harness stubs (never shipped). The real app's screens, overlays and staged editors are
// linked unchanged; only the platform edges are replaced:
//   * PKSEFramebuffer draws nothing (fixed 9 px / char text metrics), so draw() still publishes the
//     real touch targets the next frame's input hit-tests;
//   * keyboard prompts answer from HostUi::prompts (default: cancelled, i.e. "no change");
//   * file / title utilities that touch the console's storage refuse or return fixed values.
#include "HostUi.h"

#include "UI/PKSEFramebuffer.h"
#include "Utils/FileUtilities.h"
#include "Utils/HelperUtilities.h"
#include "Utils/Keyboard.h"

namespace UI {
PKSEFramebuffer::PKSEFramebuffer() : width(1280), height(720) {}
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::clear(Color) {}
void PKSEFramebuffer::drawPixel(int, int, Color) {}
void PKSEFramebuffer::drawRect(int, int, int, int, Color) {}
void PKSEFramebuffer::drawFilledRect(int x, int y, int w, int h, Color c) { HostUi::occlude(x, y, w, h, c.a); }
void PKSEFramebuffer::drawText(int x, int y, const char* text, Color c, TextStyle style) {
    HostUi::texts.emplace_back(text);
    HostUi::recordText(x, y, text, static_cast<int>(style), c.toRGBA8());
}
void PKSEFramebuffer::drawText(int x, int y, const std::string& text, Color c, TextStyle style) {
    HostUi::texts.push_back(text);
    HostUi::recordText(x, y, text, static_cast<int>(style), c.toRGBA8());
}
void PKSEFramebuffer::drawSymbol(int, int, const std::string&, Color, TextStyle) {}
void PKSEFramebuffer::drawImage(int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::drawImageScaled(int, int, int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::flush() {}
void PKSEFramebuffer::invalidateImage(const unsigned char*) {}
void PKSEFramebuffer::onSpriteEvicted(const unsigned char*) {}
void PKSEFramebuffer::setClipRect(int x, int y, int w, int h) {
    HostUi::clipOn = true; HostUi::clipX = x; HostUi::clipY = y; HostUi::clipW = w; HostUi::clipH = h;
}
void PKSEFramebuffer::clearClip() { HostUi::clipOn = false; }
void PKSEFramebuffer::pushTranslation(float, float) {}
void PKSEFramebuffer::popTransform() {}
void PKSEFramebuffer::drawCard(int x, int y, int w, int h) { HostUi::occlude(x, y, w, h, 255); }
void PKSEFramebuffer::drawSelectionHighlight(int, int, int, int) {}
void PKSEFramebuffer::drawHDivider(int, int, int) {}
void PKSEFramebuffer::drawFilledEllipse(int, int, int, int, Color) {}
void PKSEFramebuffer::drawFilledRoundedRect(int x, int y, int w, int h, int radius, Color c) {
    HostUi::occlude(x, y, w, h, c.a);
    HostUi::fills.push_back({x, y, w, h, radius, c.toRGBA8()});
    if (x == 0 && w == width && y == height - 56) ++HostUi::footerBars;   // ScreenChrome::drawNavBar band
}
void PKSEFramebuffer::drawRoundedRect(int, int, int, int, int, Color, int) {}
void PKSEFramebuffer::drawFilledCircle(int, int, int, Color) {}
void PKSEFramebuffer::drawCircle(int, int, int, Color, int) {}
void PKSEFramebuffer::drawEgg(int, int, int) {}
void PKSEFramebuffer::drawShinyMark(int, int, int, Color) {}
void PKSEFramebuffer::drawBackspaceIcon(int, int, int, Color) {}
void PKSEFramebuffer::drawShiftIcon(int, int, int, Color, bool) {}
void PKSEFramebuffer::drawPointerCursor(int, int, int, Color) {}
void PKSEFramebuffer::drawPill(int x, int y, int w, int h, Color c) {
    HostUi::occlude(x, y, w, h, c.a);
    HostUi::fills.push_back({x, y, w, h, -1, c.toRGBA8()});
}
void PKSEFramebuffer::drawPillBorder(int, int, int, int, Color, int) {}
void PKSEFramebuffer::drawSoftShadow(int, int, int, int, int) {}
void PKSEFramebuffer::drawVerticalGradient(int x, int y, int w, int h, Color top, Color bottom) {
    HostUi::occlude(x, y, w, h, std::min(top.a, bottom.a));
}
void PKSEFramebuffer::drawStatHexagon(int, int, int, const float*, int, float, Color, Color, Color) {}
int PKSEFramebuffer::drawTypeBadge(int, int, const std::string& name, Color) { return static_cast<int>(name.size()) * 9 + 30; }
void PKSEFramebuffer::drawSpriteStaticContained(int, int, int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::drawSpriteIdle(int, int, int, int, int, int, const unsigned char*, int, float) {}
void PKSEFramebuffer::startFade() {}
void PKSEFramebuffer::drawFadeOverlay() {}
// Per-style advance and line height near the device font (HostUi::charWidth, Nunito 1.364 em line).
void PKSEFramebuffer::measureText(const std::string& text, int& w, int& h, TextStyle style) {
    w = static_cast<int>(text.size()) * HostUi::charWidth(static_cast<int>(style));
    h = lineHeight(style);
}
int PKSEFramebuffer::lineHeight(TextStyle style) const {
    const int s = static_cast<int>(style);
    return s >= 0 && s < 4 ? (HostUi::kFontPx[s] * 1364 + 999) / 1000 : 18;
}
int PKSEFramebuffer::textYCenteredOn(int centerY, TextStyle) const { return centerY - 9; }
} // namespace UI

namespace Utils {
KeyboardResult promptText(const std::string&, const std::string&, const std::string&, int,
                          const std::function<void(const std::string&)>&) {
    return HostUi::nextText();
}
KeyboardResult promptSystemText(const std::string&, const std::string&, const std::string&, int) {
    return HostUi::nextText();
}
NumberResult promptNumber(const std::string&, int, int, int) { return HostUi::nextNumber(); }
bool copyDirectoryTransactional(const char*, const char*) { return false; }
bool deleteDirectoryRecursive(const char*) { return false; }
bool restoreBackupToTitle(AccountUid, u64, const char*, const std::string&, const std::vector<std::string>&) {
    return false;
}
std::string getTimestamp() { return "20261025-120000"; }
std::string getTitleVersion(u64) { return {}; }
} // namespace Utils
