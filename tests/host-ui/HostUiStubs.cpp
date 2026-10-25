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
void PKSEFramebuffer::drawFilledRect(int, int, int, int, Color) {}
void PKSEFramebuffer::drawText(int, int, const char* text, Color, TextStyle) { HostUi::texts.emplace_back(text); }
void PKSEFramebuffer::drawText(int, int, const std::string& text, Color, TextStyle) { HostUi::texts.push_back(text); }
void PKSEFramebuffer::drawSymbol(int, int, const std::string&, Color, TextStyle) {}
void PKSEFramebuffer::drawImage(int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::drawImageScaled(int, int, int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::flush() {}
void PKSEFramebuffer::invalidateImage(const unsigned char*) {}
void PKSEFramebuffer::onSpriteEvicted(const unsigned char*) {}
void PKSEFramebuffer::setClipRect(int, int, int, int) {}
void PKSEFramebuffer::clearClip() {}
void PKSEFramebuffer::pushTranslation(float, float) {}
void PKSEFramebuffer::popTransform() {}
void PKSEFramebuffer::drawCard(int, int, int, int) {}
void PKSEFramebuffer::drawSelectionHighlight(int, int, int, int) {}
void PKSEFramebuffer::drawHDivider(int, int, int) {}
void PKSEFramebuffer::drawFilledEllipse(int, int, int, int, Color) {}
void PKSEFramebuffer::drawFilledRoundedRect(int, int, int, int, int, Color) {}
void PKSEFramebuffer::drawRoundedRect(int, int, int, int, int, Color, int) {}
void PKSEFramebuffer::drawFilledCircle(int, int, int, Color) {}
void PKSEFramebuffer::drawCircle(int, int, int, Color, int) {}
void PKSEFramebuffer::drawEgg(int, int, int) {}
void PKSEFramebuffer::drawShinyMark(int, int, int, Color) {}
void PKSEFramebuffer::drawBackspaceIcon(int, int, int, Color) {}
void PKSEFramebuffer::drawShiftIcon(int, int, int, Color, bool) {}
void PKSEFramebuffer::drawPointerCursor(int, int, int, Color) {}
void PKSEFramebuffer::drawPill(int, int, int, int, Color) {}
void PKSEFramebuffer::drawPillBorder(int, int, int, int, Color, int) {}
void PKSEFramebuffer::drawSoftShadow(int, int, int, int, int) {}
void PKSEFramebuffer::drawVerticalGradient(int, int, int, int, Color, Color) {}
void PKSEFramebuffer::drawStatHexagon(int, int, int, const float*, int, float, Color, Color, Color) {}
int PKSEFramebuffer::drawTypeBadge(int, int, const std::string& name, Color) { return static_cast<int>(name.size()) * 9 + 30; }
void PKSEFramebuffer::drawSpriteStaticContained(int, int, int, int, int, int, const unsigned char*, int) {}
void PKSEFramebuffer::drawSpriteIdle(int, int, int, int, int, int, const unsigned char*, int, float) {}
void PKSEFramebuffer::startFade() {}
void PKSEFramebuffer::drawFadeOverlay() {}
void PKSEFramebuffer::measureText(const std::string& text, int& w, int& h, TextStyle) {
    w = static_cast<int>(text.size()) * 9;
    h = 18;
}
int PKSEFramebuffer::lineHeight(TextStyle) const { return 18; }
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
