from pathlib import Path


def replace_once(text, old, new, label):
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected 1 match, found {count}")
    return text.replace(old, new, 1)


chrome_path = Path("include/UI/ScreenChrome.h")
chrome = chrome_path.read_text()

chrome = replace_once(
    chrome,
    """    inline std::vector<NavHit> g_navHits;
    inline std::vector<NavGestureHit> g_navGestureHits;
    inline int g_navSurfaceX = 0;
    inline int g_navSurfaceW = 0;
    inline uint64_t g_rightEdgeSwipeButton = 0;
""",
    """    inline std::vector<NavHit> g_navHits;
    inline std::vector<NavGestureHit> g_navGestureHits;
    inline int g_navSurfaceX = 0;
    inline int g_navSurfaceW = 0;
    inline int g_navContentBottom = 0;
    inline uint64_t g_contentSwipeMask = 0;
    inline bool g_contentSwipePages = false;
    inline uint64_t g_rightEdgeSwipeButton = 0;
""",
    "shared content swipe state",
)

chrome = replace_once(
    chrome,
    "    inline uint64_t navTouchButton(const TouchInput& touch) {\n",
    "    inline uint64_t navTouchButton(const TouchInput& touch, bool allowContentSwipe = false) {\n",
    "navTouchButton signature",
)

chrome = replace_once(
    chrome,
    """            if (touch.startX() >= rightEdge - kEdgeCapture && touch.startX() < rightEdge &&
                dx <= -kOpenDistance && -dx > ay * 2)
                return g_rightEdgeSwipeButton;
        }

        for (const NavGestureHit& h : g_navGestureHits) {
""",
    """            if (touch.startX() >= rightEdge - kEdgeCapture && touch.startX() < rightEdge &&
                dx <= -kOpenDistance && -dx > ay * 2)
                return g_rightEdgeSwipeButton;
        }

        // Browser/navigation screens opt into natural content swipes. The active footer is the
        // capability contract: swipes can only synthesize directions that the current screen
        // already advertises. TrainerViewScreen deliberately does not opt in because storage
        // multi-select owns content dragging there.
        if (allowContentSwipe && g_navSurfaceW > 0 && g_navContentBottom > 0 && touch.dragged() &&
            touch.startX() >= g_navSurfaceX && touch.startX() < g_navSurfaceX + g_navSurfaceW &&
            touch.startY() >= 0 && touch.startY() < g_navContentBottom) {
            const int dx = touch.x() - touch.startX();
            const int dy = touch.y() - touch.startY();
            const int ax = dx < 0 ? -dx : dx;
            const int ay = dy < 0 ? -dy : dy;
            constexpr int kContentSwipeDistance = 72;

            if (ax >= kContentSwipeDistance && ax * 3 >= ay * 4) {
                const uint64_t direction = dx < 0 ? HidNpadButton_Right : HidNpadButton_Left;
                if (g_contentSwipeMask & direction) return direction;
                if (g_contentSwipePages) return dx < 0 ? HidNpadButton_R : HidNpadButton_L;
            }
            if (ay >= kContentSwipeDistance && ay * 3 >= ax * 4) {
                const uint64_t direction = dy < 0 ? HidNpadButton_Down : HidNpadButton_Up;
                if (g_contentSwipeMask & direction) return direction;
            }
        }

        for (const NavGestureHit& h : g_navGestureHits) {
""",
    "natural content swipe handling",
)

chrome = replace_once(
    chrome,
    """        g_navHits.clear();
        g_navGestureHits.clear();
        g_navSurfaceX = x;
        g_navSurfaceW = w;
        g_rightEdgeSwipeButton = 0;
""",
    """        g_navHits.clear();
        g_navGestureHits.clear();
        g_navSurfaceX = x;
        g_navSurfaceW = w;
        g_navContentBottom = cy - TouchTargetMin / 2 - 8;
        g_contentSwipeMask = 0;
        g_contentSwipePages = false;
        g_rightEdgeSwipeButton = 0;

        // Content swipes are semantic, not global. Infer only the directional actions published by
        // this footer, so a list cannot suddenly gain horizontal movement and a modal cannot inherit
        // navigation from the screen behind it.
        if (hint.find("D-pad/Stick") != std::string::npos ||
            hint.find("D-Pad") != std::string::npos || hint.find("D-pad") != std::string::npos ||
            hint.find("Arrows") != std::string::npos)
            g_contentSwipeMask |= HidNpadButton_Up | HidNpadButton_Down |
                                  HidNpadButton_Left | HidNpadButton_Right;
        if (hint.find("Up/Down") != std::string::npos)
            g_contentSwipeMask |= HidNpadButton_Up | HidNpadButton_Down;
        if (hint.find("Left/Right") != std::string::npos)
            g_contentSwipeMask |= HidNpadButton_Left | HidNpadButton_Right;
        if (hint.find("L/R") != std::string::npos)
            g_contentSwipePages = true;
""",
    "footer content swipe registration",
)
chrome_path.write_text(chrome)

for path_name in [
    "src/UI/AppShellScreen.cpp",
    "src/UI/BackupSelectionScreen.cpp",
    "src/UI/SaveSelectScreen.cpp",
]:
    path = Path(path_name)
    text = path.read_text()
    text = replace_once(
        text,
        "            | navTouchButton(touch);\n",
        "            | navTouchButton(touch, true);\n",
        f"content swipe opt-in: {path_name}",
    )
    path.write_text(text)

save_path = Path("src/UI/SaveSelectScreen.cpp")
save = save_path.read_text()
save = replace_once(
    save,
    """        if (overlay == Overlay::GamesDrawer) {
            const UserEntry* drawerUser = currentUser();
            const int count = drawerUser ? static_cast<int>(drawerUser->titles.size()) : 0;
            // Product Home owns Y = Open Quick Games. Once open, Y is deliberately inert;
""",
    """        if (overlay == Overlay::GamesDrawer) {
            const UserEntry* drawerUser = currentUser();
            const int count = drawerUser ? static_cast<int>(drawerUser->titles.size()) : 0;

            // Touch the cover itself to choose it. Resolve on release rather than touch-down so a
            // swipe that begins on artwork remains a navigation gesture instead of accidentally
            // selecting and closing the drawer.
            if (count > 0 && touch.justReleased() && !touch.dragged()) {
                constexpr int screenW = 1280;
                constexpr int drawerW = 520;
                constexpr int cols = 3;
                constexpr int visibleRows = 3;
                constexpr int gap = 7;
                constexpr int margin = 10;
                constexpr int tileH = 160;
                constexpr int gridY = 88;
                constexpr int drawerX = screenW - drawerW;
                constexpr int tileW = (drawerW - margin * 2 - gap * 2) / cols;
                const int first = gamesDrawerScroll * cols;
                const int last = std::min(count, first + visibleRows * cols);
                for (int i = first; i < last; ++i) {
                    const int local = i - first;
                    const int col = local % cols;
                    const int row = local / cols;
                    const int bx = drawerX + margin + col * (tileW + gap);
                    const int by = gridY + row * (tileH + gap);
                    if (touch.x() >= bx && touch.x() < bx + tileW &&
                        touch.y() >= by && touch.y() < by + tileH) {
                        gamesDrawerIndex = i;
                        kDown |= HidNpadButton_A;
                        break;
                    }
                }
            }

            // Product Home owns Y = Open Quick Games. Once open, Y is deliberately inert;
""",
    "Quick Games direct tile tap",
)
save_path.write_text(save)
