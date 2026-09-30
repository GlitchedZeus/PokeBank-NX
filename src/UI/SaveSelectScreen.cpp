#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>

#include "Globals.h"
#include "UI/SaveSelectScreen.h"
#include "UI/Common.h"
#include "UI/ScreenChrome.h"
#include "UI/ProductChrome.h"
#include "UI/SystemIcons.h"
#include "UI/SpriteManager.h"
#include "UI/SpriteLayout.h"
#include "UI/GameLauncher.h"
#include "UI/TouchInput.h"
#include "Enums/GameVersion.h"
#include "Save/GetSaveFileContents.h"
#include "Trainer/Trainer.h"
#include "Games/GameIdentity.h"
#include "Integration/Gen4/Gen4AssignedSource.h"
#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Utils/Keyboard.h"
#include "Utils/Logger.h"
#include "Utils/Settings.h"
#include "Utils/StringHelpers.h"

using namespace Utils;
using namespace Enums;

namespace UI {
    namespace {
        std::string sourceLeafName(const std::string& path) {
            const size_t slash = path.find_last_of("/\\");
            std::string leaf = slash == std::string::npos ? path : path.substr(slash + 1);
            constexpr size_t maxChars = 22;
            if (leaf.size() > maxChars) leaf = leaf.substr(0, maxChars - 3) + "...";
            return leaf;
        }

        std::string profileIdentity(AccountUid uid) {
            std::ostringstream output;
            output << std::hex << std::setfill('0')
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[0])
                   << std::setw(16) << static_cast<unsigned long long>(uid.uid[1]);
            return output.str();
        }

        std::string shortValue(const std::string& value, size_t length = 12) {
            return value.substr(0, std::min(length, value.size()));
        }

        std::string productSourceLabel(std::string_view raw) {
            if (raw == "LOCAL SAVE") return "System save";
            if (raw == "REMEMBERED") return "Linked save";
            if (raw == "CHOOSE SAVE") return "Choose save";
            if (raw == "MISSING") return "Missing source";
            if (raw == "INVALID") return "Invalid source";
            if (raw == "AMBIGUOUS") return "Needs attention";
            return std::string(raw);
        }

        std::string providerSummary(
            const std::vector<PokeVault::Legacy::FRLGSaveInstance>& instances) {
            std::vector<std::string> providers;
            for (const auto& instance : instances) {
                const std::string provider =
                    instance.providerLabel.empty() ? std::string("Source") : instance.providerLabel;
                if (std::find(providers.begin(), providers.end(), provider) == providers.end())
                    providers.push_back(provider);
            }
            if (providers.empty()) return "No validated providers";
            if (providers.size() == 1) return "Provider: " + providers.front();

            std::string summary = "Providers: ";
            for (size_t i = 0; i < providers.size(); ++i) {
                if (i != 0) summary += " + ";
                summary += providers[i];
            }
            return summary;
        }

        int preferredLegacySourceIndex(
            const std::vector<PokeVault::Legacy::FRLGSaveInstance>& instances,
            const PokeVault::Legacy::LegacySourceBindings* bindings,
            std::string_view profileIdentity, std::string_view gameIdentity) {
            if (!bindings) return -1;
            int preferred = -1;
            for (size_t index = 0; index < instances.size(); ++index) {
                if (!bindings->isPreferredGameSource(
                        instances[index], profileIdentity, gameIdentity))
                    continue;
                if (preferred >= 0) return -2; // Corrupt/ambiguous metadata: never guess.
                preferred = static_cast<int>(index);
            }
            return preferred;
        }

        std::string joinBrowsePath(const std::string& root, const std::string& name) {
            if (root.empty() || root.back() == '/') return root + name;
            return root + "/" + name;
        }

        std::string parentBrowsePath(std::string path) {
            while (path.size() > 6 && path.back() == '/') path.pop_back();
            const size_t slash = path.find_last_of("/\\");
            if (slash == std::string::npos) return {};
            if (path.rfind("sdmc:/", 0) == 0 && slash <= 5) return "sdmc:/";
            return path.substr(0, slash);
        }

        bool browseDirectory(const std::string& path) {
            struct stat st{};
            return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
        }

        void drawSaveInstanceRows(
            PKSEFramebuffer& fb,
            const std::vector<PokeVault::Source::SaveInstance>& instances,
            int selectedIndex, int first, int x, int rowY, int width,
            int rowHeight, int visibleRows, bool showOlderLabel) {
            const int last = std::min<int>(
                static_cast<int>(instances.size()), first + visibleRows);
            for (int i = first; i < last; ++i) {
                const auto& instance = instances[static_cast<size_t>(i)];
                drawFocusedCard(fb, x + 24, rowY, width - 48, rowHeight - 6,
                                i == selectedIndex, 10);
                fb.drawText(x + 44, rowY + 8, instance.label,
                            i == selectedIndex ? Colors::TextPrimary : Colors::TextSecondary,
                            TextStyle::Body);
                if (instance.mostRecentlyModified || showOlderLabel) {
                    const std::string recency = instance.mostRecentlyModified
                        ? "MOST RECENTLY MODIFIED" : "OLDER FILE";
                    int fw = 0, fh = 0;
                    fb.measureText(recency, fw, fh, TextStyle::Caption);
                    fb.drawText(x + width - 44 - fw, rowY + 11, recency, Colors::TextMuted,
                                TextStyle::Caption);
                }
                const std::string sourceLine =
                    (instance.providerLabel.empty() ? std::string("Source") : instance.providerLabel) +
                    " / " + instance.sourceLabel;
                fb.drawText(x + 44, rowY + 34, sourceLine, Colors::TextMuted,
                            TextStyle::Caption);
                rowY += rowHeight;
            }
        }

        void drawProductHelpOverlay(
            PKSEFramebuffer& fb, const std::string& title,
            std::initializer_list<ControllerHint> rows,
            const std::string& note = {}) {
            constexpr int w = 850;
            constexpr int rowH = 48;
            const int count = static_cast<int>(rows.size());
            const int h = std::min(580, 126 + count * rowH + (note.empty() ? 0 : 42));
            const int x = (fb.getWidth() - w) / 2;
            const int y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);

            fb.drawText(x + 28, y + 18, "POKEBANK NX / HELP",
                        Colors::Info, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, title,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawFilledRoundedRect(x + 28, y + 80, w - 56, 2, 1, Colors::Divider);

            int rowY = y + 100;
            for (const auto& row : rows) {
                const int glyphW = buttonGlyphWidth(fb, row.button);
                if (glyphW > 0) {
                    buttonGlyph(fb, x + 34, rowY + rowH / 2, row.button, false);
                    fb.drawText(x + 34 + glyphW + 18, rowY + 12, row.label,
                                Colors::TextPrimary, TextStyle::Body);
                } else {
                    fb.drawText(x + 34, rowY + 12,
                                row.button.empty() ? row.label : row.button + "   " + row.label,
                                Colors::TextSecondary, TextStyle::Body);
                }
                rowY += rowH;
            }

            if (!note.empty())
                fb.drawText(x + 34, y + h - 38, note,
                            Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"B", "Close"}});
        }
    }

        void drawHubDockIcon(PKSEFramebuffer& fb, int index,
                             int x, int y, int size, bool focused) {
            const Color ink = focused ? Colors::SelectedText : Colors::TextPrimary;
            const int cx = x + size / 2;
            const int cy = y + size / 2;
            if (index == 0) {
                // Games: compact controller mark.
                fb.drawRoundedRect(cx - 17, cy - 11, 34, 22, 10, ink, 2);
                fb.drawFilledRoundedRect(cx - 11, cy - 1, 10, 3, 1, ink);
                fb.drawFilledRoundedRect(cx - 7, cy - 5, 3, 10, 1, ink);
                fb.drawFilledCircle(cx + 8, cy - 3, 2, ink);
                fb.drawFilledCircle(cx + 12, cy + 4, 2, ink);
            } else if (index == 1) {
                // Banks.
                fb.drawRoundedRect(cx - 17, cy - 13, 34, 22, 5, ink, 2);
                fb.drawRoundedRect(cx - 12, cy - 5, 24, 22, 5, ink, 2);
            } else if (index == 2) {
                // Backpack / Items replaces the old Backups root shortcut.
                fb.drawRoundedRect(cx - 15, cy - 10, 30, 24, 7, ink, 2);
                fb.drawRoundedRect(cx - 9, cy - 17, 18, 12, 7, ink, 2);
                fb.drawFilledRoundedRect(cx - 10, cy - 2, 20, 4, 2, ink);
            } else if (index == 3) {
                // Search.
                fb.drawCircle(cx - 3, cy - 3, 9, ink, 2);
                fb.drawFilledRoundedRect(cx + 5, cy + 5, 13, 4, 2, ink);
            } else if (index == 4) {
                // More.
                constexpr int tile = 8;
                constexpr int gap = 5;
                fb.drawFilledRoundedRect(cx - tile - gap / 2, cy - tile - gap / 2, tile, tile, 2, ink);
                fb.drawFilledRoundedRect(cx + gap / 2, cy - tile - gap / 2, tile, tile, 2, ink);
                fb.drawFilledRoundedRect(cx - tile - gap / 2, cy + gap / 2, tile, tile, 2, ink);
                fb.drawFilledRoundedRect(cx + gap / 2, cy + gap / 2, tile, tile, 2, ink);
            } else {
                // The only visible Settings gear lives in the bottom dock.
                fb.drawFilledCircle(cx, cy, 7, ink);
                fb.drawFilledRoundedRect(cx - 2, cy - 18, 4, 8, 2, ink);
                fb.drawFilledRoundedRect(cx - 2, cy + 10, 4, 8, 2, ink);
                fb.drawFilledRoundedRect(cx - 18, cy - 2, 8, 4, 2, ink);
                fb.drawFilledRoundedRect(cx + 10, cy - 2, 8, 4, 2, ink);
            }
        }

    struct TrainerPortraitPresentation {
        const char* label = "Trainer";
        const char* assetKey = "";
        bool female = false;
        bool specific = false;
    };

    TrainerPortraitPresentation trainerPortraitForGame(
        const std::string& gameId, bool genderKnown, uint8_t gender) {
        const bool female = genderKnown && gender == 1;

        if (gameId == "red_gb" || gameId == "blue_gb" || gameId == "yellow_gb")
            return {"Red", "red", false, true};
        if (gameId == "gold_gbc" || gameId == "silver_gbc")
            return {"Gold", "gold", false, true};
        if (gameId == "crystal_gbc")
            return genderKnown
                ? TrainerPortraitPresentation{female ? "Kris" : "Gold", female ? "kris" : "gold", female, true}
                : TrainerPortraitPresentation{"Crystal Trainer", "", false, false};

        if (gameId == "ruby_gba" || gameId == "sapphire_gba" || gameId == "emerald_gba")
            return genderKnown
                ? TrainerPortraitPresentation{female ? "May" : "Brendan", female ? "may" : "brendan", female, true}
                : TrainerPortraitPresentation{"Hoenn Trainer", "", false, false};
        if (gameId == "firered_gba" || gameId == "leafgreen_gba")
            return genderKnown
                ? TrainerPortraitPresentation{female ? "Leaf" : "Red", female ? "leaf" : "red", female, true}
                : TrainerPortraitPresentation{"Kanto Trainer", "", false, false};

        if (gameId == "diamond_nds" || gameId == "pearl_nds" || gameId == "platinum_nds" ||
            gameId == "brilliant_diamond_switch" || gameId == "shining_pearl_switch")
            return genderKnown
                ? TrainerPortraitPresentation{female ? "Dawn" : "Lucas", female ? "dawn" : "lucas", female, true}
                : TrainerPortraitPresentation{"Sinnoh Trainer", "", false, false};
        if (gameId == "heartgold_nds" || gameId == "soulsilver_nds")
            return genderKnown
                ? TrainerPortraitPresentation{female ? "Lyra" : "Ethan", female ? "lyra" : "ethan", female, true}
                : TrainerPortraitPresentation{"Johto Trainer", "", false, false};

        // Later customizable protagonists deliberately stay generic until appearance reconstruction
        // is backed by the exact save-format model. Never pretend a base portrait is exact.
        return {"Trainer", "", false, false};
    }

    void drawTrainerPortrait(PKSEFramebuffer& fb, int x, int y, int w, int h,
                             const TrainerPortraitPresentation& portrait,
                             bool showLabel) {
        const Color accent = Colors::Info;
        drawPanelSurface(fb, x, y, w, h, false, std::min(14, w / 5));

        const IconImage& art = SystemIcons::trainerPortrait(portrait.assetKey);
        const int labelReserve = showLabel ? 24 : 4;
        if (art.valid()) {
            const auto rect = PokeBank::UIModel::containSprite(
                x + 4, y + 4, w - 8, h - labelReserve - 4, art.width, art.height);
            if (rect.width > 0 && rect.height > 0)
                fb.drawImageScaled(rect.x, rect.y, art.width, art.height,
                                   rect.width, rect.height, art.data, 4);
        } else {
            // Truthful fallback: a Poké Ball identity badge, never a fake "character portrait".
            const int cx = x + w / 2;
            const int cy = y + (h - labelReserve) / 2;
            const int r = std::max(13, std::min(w, h - labelReserve) / 4);
            fb.drawCircle(cx, cy, r, withAlpha(accent, 190), 3);
            fb.drawFilledRect(cx - r, cy - 2, r * 2, 4, withAlpha(accent, 150));
            fb.drawFilledCircle(cx, cy, std::max(4, r / 4), accent);
        }

        if (showLabel) {
            std::string label = portrait.label;
            if (label.size() > 14) label = label.substr(0, 13) + "…";
            int tw = 0, th = 0;
            fb.measureText(label, tw, th, TextStyle::Caption);
            fb.drawText(x + std::max(4, (w - tw) / 2), y + h - 20,
                        label, portrait.specific ? Colors::TextPrimary : Colors::TextMuted,
                        TextStyle::Caption);
        }
    }

    // Approved product-home layout (1280x720): selected game is the visual anchor,
    // Master Vault / Pokédex sit beside it, and the persistent dock stays compact.
    constexpr int HUB_Y = 78;
    constexpr int HUB_H = 548;
    constexpr int PROFILE_W = 342;       // retained for legacy list/scroll helpers
    constexpr int DETAIL_X = 24;
    constexpr int DETAIL_W = 720;
    constexpr int RIGHT_X = 764;
    constexpr int RIGHT_W = 492;
    constexpr int PROFILE_AVATAR = 44;
    constexpr int GAME_ROW_H = 56;       // retained for legacy list/scroll helpers
    constexpr int HUB_VISIBLE_TITLES = 7;
    constexpr int DETAIL_ART = 210;
    constexpr int CLASSIC_TILE_W = 184;
    constexpr int CLASSIC_TILE_H = 208;
    constexpr int CLASSIC_ICON = 126;
    constexpr int CLASSIC_GAP = 16;
    constexpr int CLASSIC_MAX_COLS = 5;
    constexpr int CLASSIC_GRID_Y = 216;
    constexpr int CLASSIC_VISIBLE_ROWS = 2;

    SaveSelectScreen::SaveSelectScreen(
        PokeVault::Legacy::FRLGDiscoveryResult& legacySources,
        PokeVault::Legacy::LegacySourceBindings& bindings,
        const NavigationState* resumeState)
        : legacyCatalog(&legacySources), legacyBindings(&bindings) {
        loadUsers();
        loadLegacySources(legacySources);
        loadGen4Cards();

        if (resumeState && !users.empty()) {
            userIndex = std::clamp(resumeState->userIndex, 0,
                                   static_cast<int>(users.size()) - 1);
            const auto* resumedUser = currentUser();
            const int titleCount = resumedUser
                ? static_cast<int>(resumedUser->titles.size()) : 0;
            titleIndex = titleCount > 0
                ? std::clamp(resumeState->titleIndex, 0, titleCount - 1) : 0;
            hubDockIndex = std::clamp(resumeState->hubDockIndex, 0, 4);
            classicGamesActive = resumeState->classicGamesActive;
            hubFeatureIndex = std::clamp(resumeState->hubFeatureIndex, -1, 1);
            scrollRow = std::max(0, resumeState->scrollRow);
            hubDockFocused = resumeState->hubDockFocused;
            scrollSelectionIntoView();
        }
        refreshHubPreview();
    }

    void SaveSelectScreen::loadLegacySources(
        const PokeVault::Legacy::FRLGDiscoveryResult& legacySources) {
        for (auto& user : users) {
            user.titles.erase(std::remove_if(user.titles.begin(), user.titles.end(),
                [](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::RetroArchFRLG;
                }), user.titles.end());
        }
        if (users.size() == 1 && users.front().titles.empty() &&
            users.front().name == "No users found") {
            users.front().name = "Pokémon Saves";
        }

        // Discovery is app-global; normal visibility is not. A filesystem source has no intrinsic
        // Nintendo-account owner, so it appears only after an explicit persistent assignment.
        for (auto& user : users) {
            const auto cards = legacyBindings
                ? PokeVault::Legacy::buildFRLGSourceCardsForProfile(
                    legacySources, *legacyBindings, profileIdentity(user.uid))
                : std::vector<PokeVault::Legacy::FRLGSourceCard>{};
            user.titles.reserve(user.titles.size() + cards.size());
            for (const auto& card : cards) {
                TitleEntry entry;
                entry.name = "Pokemon " + card.title;
                entry.label = card.title;
                entry.gameId = card.gameId;
                entry.platformLabel = card.platformLabel;
                entry.sourceLabel = card.sourceLabel;
                entry.locationLabel = std::to_string(card.instances.size()) +
                    (card.instances.size() == 1 ? " SAVE" : " SAVES");
                entry.artworkKey = card.artworkKey;
                entry.legacyInstances = card.instances;
                entry.sourceKind = SelectedSourceKind::RetroArchFRLG;
                if (card.instances.size() == 1) {
                    const auto& instance = card.instances.front();
                    entry.trainerName = instance.trainerName;
                    const size_t handle = instance.sourceIndex;
                    if (handle < legacySources.sources.size()) {
                        const auto& source = legacySources.sources[handle];
                        if (source.isGen1() && source.gen1Save) {
                            const auto dex = source.gen1Save->dexProgress();
                            entry.dexSeen = dex.seen;
                            entry.dexCaught = dex.caught;
                            entry.dexTotal = dex.total;
                            // Gen I has no selectable player gender; Red is fixed by the game.
                            entry.trainerGender = 0;
                            entry.trainerGenderKnown = true;
                        } else if (source.isGen2() && source.gen2Save) {
                            const auto dex = source.gen2Save->dexProgress();
                            entry.dexSeen = dex.seen;
                            entry.dexCaught = dex.caught;
                            entry.dexTotal = dex.total;
                            const auto gender = source.gen2Save->trainer().gender;
                            if (gender) {
                                entry.trainerGender = *gender;
                                entry.trainerGenderKnown = true;
                            } else if (entry.gameId == "gold_gbc" || entry.gameId == "silver_gbc") {
                                // Gold/Silver have a fixed male player character.
                                entry.trainerGender = 0;
                                entry.trainerGenderKnown = true;
                            }
                        } else if (source.isGen3() && source.save) {
                            const auto dex = source.save->dexProgress();
                            entry.dexSeen = dex.seen;
                            entry.dexCaught = dex.caught;
                            entry.dexTotal = dex.total;
                            entry.trainerGender = source.save->trainer().gender;
                            entry.trainerGenderKnown = true;
                        }
                    }
                }
                user.titles.push_back(std::move(entry));
            }
        }
        rebuildUnassignedLegacySources();
    }

    void SaveSelectScreen::loadGen4Cards() {
        for (auto& user : users) {
            user.titles.erase(std::remove_if(user.titles.begin(), user.titles.end(),
                [](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::Gen4AssignedFile;
                }), user.titles.end());

            const std::string profile = profileIdentity(user.uid);
            for (const auto& game : PokeVault::Games::allGameDescriptors()) {
                if (game.platform != PokeVault::Games::Platform::NintendoDS ||
                    game.dataGeneration != 4 ||
                    game.support != PokeVault::Games::SourceSupport::ReadOnly) continue;

                TitleEntry entry;
                entry.name = "Pokemon " + std::string(game.title);
                entry.label = std::string(game.title);
                entry.gameId = std::string(game.id);
                entry.platformLabel = std::string(PokeVault::Games::platformName(game.platform));
                entry.artworkKey = entry.gameId;
                entry.sourceKind = SelectedSourceKind::Gen4AssignedFile;

                if (!legacyBindings) {
                    entry.sourceLabel = "CHOOSE SAVE";
                } else {
                    const auto assigned = legacyBindings->resolveFileForGame(profile, game.id);
                    switch (assigned.status) {
                        case PokeVault::Legacy::AssignedFileStatus::Ready: {
                            entry.sourceLabel = "REMEMBERED";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            const auto opened = PokeVault::Integration::Gen4::openAssignedSource(
                                *legacyBindings, profile, game.id);
                            if (opened.status == PokeVault::Integration::Gen4::OpenStatus::Ready &&
                                opened.save) {
                                entry.trainerName = Utils::utf16ToUtf8(opened.save->trainer().name);
                                entry.trainerGender = opened.save->trainer().gender;
                                entry.trainerGenderKnown = true;
                                const auto dex = opened.save->dexProgress();
                                entry.dexSeen = dex.seen;
                                entry.dexCaught = dex.caught;
                                entry.dexTotal = dex.total;
                            }
                            break;
                        }
                        case PokeVault::Legacy::AssignedFileStatus::Missing:
                            entry.sourceLabel = "MISSING";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Unreadable:
                            entry.sourceLabel = "INVALID";
                            entry.locationLabel = sourceLeafName(assigned.binding.sourcePath);
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Ambiguous:
                            entry.sourceLabel = "AMBIGUOUS";
                            break;
                        case PokeVault::Legacy::AssignedFileStatus::Unassigned:
                            entry.sourceLabel = "CHOOSE SAVE";
                            break;
                    }
                }
                user.titles.push_back(std::move(entry));
            }
        }
    }

    void SaveSelectScreen::rebuildUnassignedLegacySources() {
        unassignedLegacySources.clear();
        if (!legacyCatalog || !legacyBindings) return;
        for (const auto& card : PokeVault::Legacy::buildFRLGSourceCards(*legacyCatalog)) {
            for (auto instance : card.instances) {
                legacyBindings->applyClaims(instance);
                if (instance.claimConflict || !instance.claimedProfile.empty()) continue;
                unassignedLegacySources.push_back({card.gameId, card.title, instance});
            }
        }
        if (legacyAssignmentIndex >= static_cast<int>(unassignedLegacySources.size()))
            legacyAssignmentIndex = std::max(0,
                static_cast<int>(unassignedLegacySources.size()) - 1);
    }

    std::string SaveSelectScreen::currentProfileIdentity() const {
        const UserEntry* user = currentUser();
        return user ? profileIdentity(user->uid) : std::string{};
    }

    const PokeVault::Legacy::FRLGSaveInstance* SaveSelectScreen::currentLegacyInstance() const {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size()))
            return nullptr;
        const auto& title = user->titles[titleIndex];
        if (title.sourceKind != SelectedSourceKind::RetroArchFRLG || legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(title.legacyInstances.size())) return nullptr;
        return &title.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
    }

    bool SaveSelectScreen::assignCurrentLegacySource() {
        if (!legacyBindings || legacyAssignmentIndex < 0 ||
            legacyAssignmentIndex >= static_cast<int>(unassignedLegacySources.size())) return false;
        const std::string profile = currentProfileIdentity();
        if (profile.empty()) return false;
        const auto& entry = unassignedLegacySources[static_cast<size_t>(legacyAssignmentIndex)];
        const std::string assignedGameId = entry.gameId;
        const std::string assignedLeaf = sourceLeafName(entry.instance.location);
        if (!legacyBindings->claimInstanceAndSave(entry.instance, profile)) {
            logErrorToFile("Legacy binding assignment failed", legacyBindings->lastError().c_str());
            legacyNotice = "Assignment could not be saved; source remains unassigned.";
            return false;
        }
        loadLegacySources(*legacyCatalog);
        const UserEntry* user = currentUser();
        if (user) {
            const auto found = std::find_if(user->titles.begin(), user->titles.end(),
                [&](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                           title.gameId == assignedGameId;
                });
            if (found != user->titles.end())
                titleIndex = static_cast<int>(std::distance(user->titles.begin(), found));
        }
        gamesDrawerIndex = titleIndex;
        gamesDrawerScroll = std::max(0, gamesDrawerIndex - 3);
        legacyNotice = assignedLeaf + " assigned to this profile.";
        hubNotice = legacyNotice;
        overlay = Overlay::GamesDrawer;
        scrollSelectionIntoView();
        refreshHubPreview();
        return true;
    }

    bool SaveSelectScreen::refreshLegacySources(
        const std::string& gameId, const std::string& preferredSourceIdentity,
        bool requirePreferred) {
        if (!legacyCatalog) return false;
        auto refreshed = PokeVault::Legacy::discoverConfiguredLegacySaves();
        *legacyCatalog = std::move(refreshed);
        loadLegacySources(*legacyCatalog);

        logInfoToFile("Legacy active battery-save root",
            legacyCatalog->activeRoot.empty() ? "(none)" : legacyCatalog->activeRoot.c_str());
        const UserEntry* user = currentUser();
        if (!user) return false;
        const auto parent = std::find_if(user->titles.begin(), user->titles.end(),
            [&](const auto& title) {
                return title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                       title.gameId == gameId;
            });
        if (parent == user->titles.end()) {
            overlay = Overlay::None;
            legacyNotice = "No validated saves remain for this game.";
            titleIndex = std::min<int>(titleIndex,
                std::max<int>(0, static_cast<int>(user->titles.size()) - 1));
            scrollSelectionIntoView();
            refreshHubPreview();
            return false;
        }

        titleIndex = static_cast<int>(std::distance(user->titles.begin(), parent));
        legacyInstanceIndex = 0;
        if (!preferredSourceIdentity.empty()) {
            const auto instance = std::find_if(parent->legacyInstances.begin(),
                parent->legacyInstances.end(), [&](const auto& candidate) {
                    return candidate.sourceIdentity == preferredSourceIdentity;
                });
            if (instance == parent->legacyInstances.end()) {
                legacyNotice = "That save changed location or was removed; nothing was opened.";
                overlay = Overlay::LegacyInstances;
                legacyInstanceScroll = 0;
                refreshHubPreview();
                return !requirePreferred;
            }
            legacyInstanceIndex = static_cast<int>(
                std::distance(parent->legacyInstances.begin(), instance));
        }
        legacyInstanceScroll = std::max(0, legacyInstanceIndex - 5);
        legacyNotice = "Save list refreshed from configured emulator roots.";
        overlay = Overlay::LegacyInstances;
        scrollSelectionIntoView();
        refreshHubPreview();
        return true;
    }

    void SaveSelectScreen::loadUsers() {
        users.clear();

        AccountUid userIds[ACC_USER_LIST_SIZE];
        s32 userCount = 0;
        Result rc = accountListAllUsers(userIds, ACC_USER_LIST_SIZE, &userCount);
        if (R_FAILED(rc)) {
            logErrorToFile("SaveSelect: failed to list users");
            userCount = 0;
        }

        for (s32 i = 0; i < userCount; i++) {
            UserEntry entry;
            entry.uid = userIds[i];

            AccountProfile profile;
            AccountProfileBase base;
            if (R_SUCCEEDED(accountGetProfile(&profile, userIds[i]))) {
                if (R_SUCCEEDED(accountProfileGet(&profile, NULL, &base))) {
                    entry.name = std::string(base.nickname);
                } else {
                    entry.name = "Unknown User";
                }
                accountProfileClose(&profile);
            } else {
                entry.name = "Unknown User";
            }

            loadTitlesForUser(entry);
            users.push_back(std::move(entry));
        }

        if (users.empty()) {
            UserEntry def;
            def.name = "No users found";
            memset(&def.uid, 0, sizeof(AccountUid));
            users.push_back(std::move(def));
        }
    }

    /**
     * List the Pokemon saves this user has, by enumerating SAVE DATA -- not installed titles.
     *
     * The distinction is the whole point. This used to walk `nsListApplicationRecord` and, for each
     * Pokemon title found, probe whether a save could be mounted. That asks "which games are
     * installed, and do they have saves?" when the only question a save editor cares about is
     * "which saves exist?" -- and the two differ in a case that is not rare at all:
     *
     *   A game played from a CARTRIDGE has no application record when the cart is out. Its save
     *   lives on internal storage and is perfectly editable, but the title vanishes from the picker
     *   the moment the cart is swapped for another game. An archived or partly-uninstalled title
     *   does the same.
     *
     * Enumerating save data finds those, because the OS lists the save whether or not anything is
     * currently installed to play it. It is also cheaper: no mount/unmount probe per title, which
     * was both slow and a devoptab slot churn.
     */
    // Scan one save-data space and append this user's Pokemon saves. Returns false only if the
    // space could not be opened at all; an empty space is a perfectly normal success.
    bool SaveSelectScreen::scanSaveSpace(UserEntry& user, int spaceId, int& scanned, int& forUser) {
        FsSaveDataInfoReader reader;
        Result rc = fsOpenSaveDataInfoReader(&reader, static_cast<FsSaveDataSpaceId>(spaceId));
        if (R_FAILED(rc)) {
            char m[112];
            snprintf(m, sizeof(m), "SaveSelect: cannot read save space %d (rc=0x%08X)", spaceId, (unsigned)rc);
            logInfoToFile(m);
            return false;
        }

        FsSaveDataInfo info[24];
        s64 readCount = 0;
        while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&reader, info, 24, &readCount)) && readCount > 0) {
            for (s64 i = 0; i < readCount; i++) {
                scanned++;
                // Account saves only -- system/temporary/cache entries are not a player's save file.
                if (info[i].save_data_type != FsSaveDataType_Account) continue;
                if (memcmp(&info[i].uid, &user.uid, sizeof(AccountUid)) != 0) continue;
                forUser++;

                const u64 titleId = info[i].application_id;
                GameVersion gv = getGameVersion(titleId);
                if (gv == GameVersion::Invalid) continue;   // not a Pokemon title PKSE knows
                const auto* identity = PokeVault::Games::findSwitchGame(titleId);
                if (!identity) continue;                    // supported parser without a stable release identity

                // One tile per game. A title can report more than one save entry (save_data_index),
                // and scanning two spaces can see the same save twice -- listing a game twice would
                // be worse than useless.
                bool dup = false;
                for (const auto& t : user.titles) {
                    if (t.titleId == titleId) { dup = true; break; }
                }
                if (dup) continue;

                char m[128];
                snprintf(m, sizeof(m), "SaveSelect: %s (%016llX) -> listed",
                         getGameVersionName(gv).c_str(), (unsigned long long)titleId);
                logInfoToFile(m);

                TitleEntry t;
                t.titleId = titleId;
                t.label = getGameVersionName(gv);       // short: "Shield", "Legends: Z-A", ...
                t.name  = "Pokemon " + t.label;         // full name (backup dir + downstream compat)
                t.gameId = std::string(identity->id);
                t.platformLabel = std::string(PokeVault::Games::platformName(identity->platform));
                user.titles.push_back(std::move(t));
            }
        }
        fsSaveDataInfoReaderClose(&reader);
        return true;
    }

    void SaveSelectScreen::loadTitlesForUser(UserEntry& user) {
        int scanned = 0, forUser = 0;

        // `All` is the pseudo-space the header blesses for this reader, and it is what should
        // normally answer. Fall back to the concrete spaces if it is refused, because "found
        // nothing" is precisely the failure this function exists to stop producing.
        if (!scanSaveSpace(user, FsSaveDataSpaceId_All, scanned, forUser)) {
            scanSaveSpace(user, FsSaveDataSpaceId_User,   scanned, forUser);
            scanSaveSpace(user, FsSaveDataSpaceId_SdUser, scanned, forUser);
        }

        // Counts make a missing title diagnosable from the log alone: how many saves the console
        // reported in total, how many belong to this user, and how many were Pokemon titles.
        char summary[192];
        snprintf(summary, sizeof(summary),
                 "SaveSelect: %d save entries on console, %d for %s, %d Pokemon titles listed",
                 scanned, forUser, user.name.c_str(), (int)user.titles.size());
        logInfoToFile(summary);
    }

    const SaveSelectScreen::UserEntry* SaveSelectScreen::currentUser() const {
        if (users.empty()) return nullptr;
        return &users[userIndex];
    }

    int SaveSelectScreen::titleColumns() const { return 1; }

    int SaveSelectScreen::titleRows() const {
        const UserEntry* u = currentUser();
        return u ? static_cast<int>(u->titles.size()) : 0;
    }

    int SaveSelectScreen::classicTitleColumns() const {
        return std::max(1, std::min(CLASSIC_MAX_COLS, titleRows()));
    }

    int SaveSelectScreen::classicTitleRows() const {
        const int count = titleRows();
        const int cols = classicTitleColumns();
        return count <= 0 ? 0 : (count + cols - 1) / cols;
    }

    void SaveSelectScreen::scrollClassicSelectionIntoView() {
        const int rows = classicTitleRows();
        if (rows <= CLASSIC_VISIBLE_ROWS) { scrollRow = 0; return; }
        const int selectedRow = titleIndex / classicTitleColumns();
        if (selectedRow < scrollRow) scrollRow = selectedRow;
        else if (selectedRow >= scrollRow + CLASSIC_VISIBLE_ROWS)
            scrollRow = selectedRow - CLASSIC_VISIBLE_ROWS + 1;
        scrollRow = std::clamp(scrollRow, 0, std::max(0, rows - CLASSIC_VISIBLE_ROWS));
    }

    /**
     * Move the scroll window as LITTLE as possible to keep the selected tile on screen.
     *
     * `scrollRow` is deliberately STATE, not something recomputed from `titleIndex` each frame. The
     * derived version centred the selection -- `first = selRow - VISIBLE_ROWS/2` -- which reads as
     * paging: with three rows, selecting the bottom row shows rows 1-2, and moving back up to the
     * middle row snapped the view to rows 0-1 even though the middle row was *already visible*.
     * Every vertical move repainted the whole grid.
     *
     * Scrolling only when the selection would otherwise fall outside the window means the view
     * holds still while the cursor moves inside it, and shifts by exactly one row at the edges.
     * With three rows that is: nothing at all between the top two rows, one row down when you enter
     * the bottom row, and one row back up only when you leave the top row.
     */
    void SaveSelectScreen::scrollSelectionIntoView() {
        const int count = titleRows();
        if (count <= HUB_VISIBLE_TITLES) { scrollRow = 0; return; }
        if (titleIndex < scrollRow) scrollRow = titleIndex;
        else if (titleIndex >= scrollRow + HUB_VISIBLE_TITLES)
            scrollRow = titleIndex - HUB_VISIBLE_TITLES + 1;
        scrollRow = std::clamp(scrollRow, 0, std::max(0, count - HUB_VISIBLE_TITLES));
    }

    void SaveSelectScreen::setUser(int idx) {
        if (users.empty()) return;
        userIndex = (idx % (int)users.size() + (int)users.size()) % (int)users.size();
        titleIndex = 0;
        scrollRow  = 0;
        hubDockFocused = false;
        headerActionIndex = -1;
        hubFeatureIndex = -1;
        refreshHubPreview();
    }

    void SaveSelectScreen::refreshHubPreview() {
        partyPreview = {};
        partyPreviewStatus.clear();
        previewTrainerName.clear();
        previewTrainerGender = 0;
        previewTrainerGenderKnown = false;
        previewDexSeen = 0;
        previewDexCaught = 0;
        previewDexTotal = 0;
        hubNotice.clear();
        launchDescriptor = {};

        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size())) {
            partyPreviewStatus = "No game selected";
            return;
        }
        const auto& title = user->titles[static_cast<size_t>(titleIndex)];
        previewTrainerName = title.trainerName;
        previewTrainerGender = title.trainerGender;
        previewTrainerGenderKnown = title.trainerGenderKnown;
        previewDexSeen = title.dexSeen;
        previewDexCaught = title.dexCaught;
        previewDexTotal = title.dexTotal;

        std::string providerId;
        std::string sourcePath;
        std::string bindingKey;
        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (title.legacyInstances.size() == 1) {
                const auto& instance = title.legacyInstances.front();
                providerId = instance.providerId.empty()
                    ? PokeVault::Source::providerIdFor(instance.providerLabel)
                    : instance.providerId;
                sourcePath = instance.path();
                bindingKey = gameLaunchBindingKey(
                    currentProfileIdentity(), title.gameId, instance.sourceIdentity);
                previewTrainerName = instance.trainerName;
            }
        } else if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile && legacyBindings) {
            const auto assigned = legacyBindings->resolveFileForGame(
                currentProfileIdentity(), title.gameId);
            if (assigned.status == PokeVault::Legacy::AssignedFileStatus::Ready) {
                providerId = PokeVault::Source::providerIdFor(assigned.binding.sourceType);
                sourcePath = assigned.binding.sourcePath;
                bindingKey = gameLaunchBindingKey(
                    currentProfileIdentity(), title.gameId, assigned.sourceIdentity);
            }
        }

        launchDescriptor = resolveGameLaunch(
            title.titleId, title.gameId, providerId, sourcePath, bindingKey);

        auto addParty = [&](size_t index, uint16_t species, uint8_t level,
                            uint8_t form, bool shiny) {
            if (index >= partyPreview.size() || species == 0) return;
            partyPreview[index].species = species;
            partyPreview[index].level = level;
            partyPreview[index].form = form;
            partyPreview[index].shiny = shiny;
            const char* speciesName = Trainer::getSpeciesName(species);
            partyPreview[index].name = speciesName ? speciesName : "Unknown";
        };

        if (title.sourceKind == SelectedSourceKind::SwitchTitle && title.titleId != 0) {
            const Result mount = fsdevMountSaveData("pbpreview", title.titleId, user->uid);
            if (R_FAILED(mount)) {
                partyPreviewStatus = "Party preview unavailable while this save cannot be mounted.";
                return;
            }

            std::string error;
            if (Save::validateTrainerSaveForOpen("pbpreview:", title.titleId, error)) {
                auto trainer = Save::readTrainerInfo("pbpreview:", title.titleId);
                std::visit([&](auto& parsed) {
                    previewTrainerName = parsed.trainerName;
                    previewTrainerGender = parsed.trainerGender;
                    previewTrainerGenderKnown = true;
                    const auto dex = parsed.pokedexProgress();
                    previewDexSeen = dex.seen;
                    previewDexCaught = dex.caught;
                    previewDexTotal = dex.total;
                    const size_t count = std::min<size_t>(parsed.party.size(), partyPreview.size());
                    for (size_t i = 0; i < count; ++i) {
                        if (parsed.party[i]) {
                            const auto* pokemon = parsed.party[i].get();
                            addParty(i, pokemon->speciesID(), pokemon->level(), pokemon->form(),
                                     pokemon->isShiny(pokemon->id32(), pokemon->species()));
                        }
                    }
                    partyPreviewStatus = count == 0 ? "No active party Pokémon." : "Current save party";
                }, trainer);
            } else {
                partyPreviewStatus = error.empty() ? "Party preview could not validate this save." : error;
            }
            fsdevUnmountDevice("pbpreview");
            return;
        }

        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (title.legacyInstances.size() != 1) {
                partyPreviewStatus = title.legacyInstances.empty()
                    ? "No validated save instance."
                    : "Choose a Save Instance to preview its active party.";
                if (title.legacyInstances.size() > 1) {
                    launchDescriptor.backend = GameLaunchBackend::HomebrewNro;
                    launchDescriptor.state = GameLaunchState::ChooseSource;
                    launchDescriptor.providerId = "source-choice";
                    launchDescriptor.detail = "Choose the exact validated save/source to launch.";
                }
                return;
            }
            const size_t handle = title.legacyInstances.front().sourceIndex;
            if (!legacyCatalog || handle >= legacyCatalog->sources.size()) {
                partyPreviewStatus = "Party preview source is stale.";
                return;
            }
            const auto& source = legacyCatalog->sources[handle];
            if (!source.ready() || source.gameId != title.gameId) {
                partyPreviewStatus = "Party preview source no longer validates.";
                return;
            }
            if (source.isGen1()) {
                const auto& party = source.gen1Save->party();
                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)
                    addParty(i, party[i].species, party[i].level, 0, false);
            } else if (source.isGen2()) {
                const auto& party = source.gen2Save->party();
                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)
                    addParty(i, party[i].species, party[i].level, 0, false);
            } else if (source.isGen3()) {
                const auto party = source.save->party();
                // The strict Gen III read-only record intentionally exposes experience rather than
                // a cached level. Do not invent a growth-curve conversion in presentation code:
                // species is authoritative here and the level line stays omitted for this preview.
                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)
                    addParty(i, party[i].species, 0, 0, false);
            }
            partyPreviewStatus = "Validated read-only source party";
            return;
        }

        if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile && legacyBindings) {
            const auto opened = PokeVault::Integration::Gen4::openAssignedSource(
                *legacyBindings, currentProfileIdentity(), title.gameId);
            if (opened.status == PokeVault::Integration::Gen4::OpenStatus::Ready && opened.save) {
                previewTrainerName = Utils::utf16ToUtf8(opened.save->trainer().name);
                previewTrainerGender = opened.save->trainer().gender;
                previewTrainerGenderKnown = true;
                const auto dex = opened.save->dexProgress();
                previewDexSeen = dex.seen;
                previewDexCaught = dex.caught;
                previewDexTotal = dex.total;
                const auto party = opened.save->party();
                for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i) {
                    if (party[i].valid())
                        addParty(i, party[i].species(), party[i].partyLevel(),
                                 party[i].form(), false);
                }
                partyPreviewStatus = "Remembered read-only source party";
            } else {
                partyPreviewStatus = "Choose a validated Save Instance to preview the active party.";
            }
        }
    }

    void SaveSelectScreen::openGameFilePicker(
        const std::string& gameId, const std::string& providerId,
        const std::string& sourcePath, const std::string& bindingKey,
        bool returnToLegacy) {
        launchLinkGameId = gameId;
        launchLinkProviderId = providerId;
        launchLinkSourcePath = sourcePath;
        launchLinkBindingKey = bindingKey;
        launchFileReturnToLegacy = returnToLegacy;
        launchBrowseRoot = suggestedGameLaunchBrowseRoot(gameId, providerId, sourcePath);
        launchBrowsePath = launchBrowseRoot;
        launchFileIndex = 0;
        launchFileScroll = 0;
        launchFileNotice.clear();
        refreshGameFilePicker();
        overlay = Overlay::GameFilePicker;
    }

    void SaveSelectScreen::refreshGameFilePicker() {
        launchFileEntries.clear();
        launchFileIndex = 0;
        launchFileScroll = 0;

        DIR* dir = ::opendir(launchBrowsePath.c_str());
        if (!dir) {
            launchFileNotice = "This folder could not be opened.";
            return;
        }

        std::vector<LaunchFileEntry> dirs;
        std::vector<LaunchFileEntry> files;
        size_t examined = 0;
        while (const dirent* entry = ::readdir(dir)) {
            if (++examined > 512) {
                launchFileNotice = "Folder limit reached; narrow the folder and try again.";
                break;
            }
            const std::string name(entry->d_name);
            if (name == "." || name == "..") continue;
            const std::string path = joinBrowsePath(launchBrowsePath, name);
            struct stat st{};
            if (::stat(path.c_str(), &st) != 0) continue;
            if (S_ISDIR(st.st_mode))
                dirs.push_back({name, path, true});
            else if (S_ISREG(st.st_mode) &&
                     gameLaunchContentSupported(launchLinkGameId, path))
                files.push_back({name, path, false});
        }
        ::closedir(dir);

        auto byName = [](const LaunchFileEntry& a, const LaunchFileEntry& b) {
            return a.name < b.name;
        };
        std::sort(dirs.begin(), dirs.end(), byName);
        std::sort(files.begin(), files.end(), byName);
        launchFileEntries.reserve(dirs.size() + files.size());
        launchFileEntries.insert(launchFileEntries.end(), dirs.begin(), dirs.end());
        launchFileEntries.insert(launchFileEntries.end(), files.begin(), files.end());
        if (launchFileEntries.empty() && launchFileNotice.empty())
            launchFileNotice = "No compatible game files in this folder.";
    }

    void SaveSelectScreen::activateGameFilePicker() {
        if (launchFileIndex < 0 ||
            launchFileIndex >= static_cast<int>(launchFileEntries.size())) return;
        const auto selected = launchFileEntries[static_cast<size_t>(launchFileIndex)];
        if (selected.directory) {
            launchBrowsePath = selected.path;
            refreshGameFilePicker();
            return;
        }

        std::string error;
        if (!saveGameLaunchBinding(
                launchLinkBindingKey, launchLinkGameId, launchLinkProviderId,
                selected.path, error)) {
            launchFileNotice = error.empty() ? "That game file could not be linked." : error;
            return;
        }

        const bool backToLegacy = launchFileReturnToLegacy;
        launchFileNotice.clear();
        refreshHubPreview();
        if (backToLegacy) {
            legacyNotice = "Game file linked. Press A to launch this source.";
            overlay = Overlay::LegacyInstances;
            launchLegacyMode = true;
        } else {
            hubNotice = "Game file linked. Launch is ready.";
            overlay = Overlay::None;
        }
    }

    void SaveSelectScreen::browseGameFileParent() {
        const std::string parent = parentBrowsePath(launchBrowsePath);
        if (parent.empty() || parent == launchBrowsePath) return;
        launchBrowsePath = parent;
        refreshGameFilePicker();
    }

    bool SaveSelectScreen::beginLaunchLinkForCurrentTitle() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 ||
            titleIndex >= static_cast<int>(user->titles.size())) return false;
        const auto& title = user->titles[static_cast<size_t>(titleIndex)];

        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
            title.legacyInstances.size() == 1) {
            const auto& instance = title.legacyInstances.front();
            const std::string provider = instance.providerId.empty()
                ? PokeVault::Source::providerIdFor(instance.providerLabel)
                : instance.providerId;
            const std::string key = gameLaunchBindingKey(
                currentProfileIdentity(), title.gameId, instance.sourceIdentity);
            openGameFilePicker(title.gameId, provider, instance.path(), key, false);
            return true;
        }

        if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile && legacyBindings) {
            const auto assigned = legacyBindings->resolveFileForGame(
                currentProfileIdentity(), title.gameId);
            if (assigned.status != PokeVault::Legacy::AssignedFileStatus::Ready) {
                hubNotice = "Choose a validated save source before linking a game file.";
                return false;
            }
            const std::string provider =
                PokeVault::Source::providerIdFor(assigned.binding.sourceType);
            const std::string key = gameLaunchBindingKey(
                currentProfileIdentity(), title.gameId, assigned.sourceIdentity);
            openGameFilePicker(
                title.gameId, provider, assigned.binding.sourcePath, key, false);
            return true;
        }

        hubNotice = "This game does not need a separate game-file link.";
        return false;
    }

    bool SaveSelectScreen::launchCurrentLegacyInstance() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size()))
            return false;
        const auto& title = user->titles[static_cast<size_t>(titleIndex)];
        if (legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(title.legacyInstances.size())) return false;

        const auto& instance = title.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
        const std::string provider = instance.providerId.empty()
            ? PokeVault::Source::providerIdFor(instance.providerLabel) : instance.providerId;
        const std::string key = gameLaunchBindingKey(
            currentProfileIdentity(), title.gameId, instance.sourceIdentity);
        const auto descriptor = resolveGameLaunch(
            0, title.gameId, provider, instance.path(), key);
        if (descriptor.state == GameLaunchState::NeedsContentLink) {
            openGameFilePicker(title.gameId, provider, instance.path(), key, true);
            return false;
        }
        if (!descriptor.ready()) {
            legacyNotice = descriptor.detail;
            return false;
        }
        std::string error;
        if (!requestGameLaunch(descriptor, error)) {
            legacyNotice = error;
            return false;
        }
        appExitRequested = true;
        exitRequested = true;
        return true;
    }

    bool SaveSelectScreen::launchCurrentTitle() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size()))
            return false;
        const auto& title = user->titles[static_cast<size_t>(titleIndex)];

        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
            title.legacyInstances.size() > 1) {
            launchLegacyMode = true;
            legacyInstanceIndex = 0;
            legacyInstanceScroll = 0;
            legacyNotice = "Choose which validated source to launch.";
            overlay = Overlay::LegacyInstances;
            return false;
        }

        if (launchDescriptor.state == GameLaunchState::NeedsContentLink)
            return beginLaunchLinkForCurrentTitle();

        if (!launchDescriptor.ready()) {
            hubNotice = launchDescriptor.detail.empty()
                ? std::string(gameLaunchActionLabel(launchDescriptor.state))
                : launchDescriptor.detail;
            return false;
        }

        std::string error;
        if (!requestGameLaunch(launchDescriptor, error)) {
            hubNotice = error;
            return false;
        }
        appExitRequested = true;
        exitRequested = true;
        return true;
    }

    void SaveSelectScreen::openGen4Setup(const std::string& gameId, std::string notice,
                                         bool returnToGamesDrawer) {
        gen4TargetGameId = gameId;
        gen4Notice = std::move(notice);
        gen4SetupFromGamesDrawer = returnToGamesDrawer;
        gen4Candidates.clear();
        gen4Instances.clear();
        gen4SetupIndex = 0;
        gen4CandidateIndex = 0;
        gen4CandidateScroll = 0;
        overlay = Overlay::Gen4Setup;
    }

    void SaveSelectScreen::discoverGen4Candidates() {
        gen4Candidates.clear();
        gen4Instances.clear();
        auto discovered = PokeVault::Integration::Gen4::discoverKnownSources();
        size_t wrappers = 0;
        size_t savestates = 0;
        std::string rememberedDiagnostic;

        auto appendReady = [&](PokeVault::Integration::Gen4::SourceCandidate candidate,
                               bool rememberedSource = false) {
            if (!candidate.ready() ||
                !PokeVault::Integration::Gen4::candidateMatchesGame(candidate, gen4TargetGameId))
                return;
            auto instance = PokeVault::Integration::Gen4::toSaveInstance(
                candidate, gen4TargetGameId, gen4Candidates.size(), rememberedSource,
                {});
            if (legacyBindings) legacyBindings->applyClaims(instance);
            if (!PokeVault::Source::visibleToProfile(instance, currentProfileIdentity()))
                return;
            if (!PokeVault::Source::appendDeduplicated(gen4Instances, std::move(instance)))
                return;
            gen4Candidates.push_back(std::move(candidate));
        };

        for (auto& candidate : discovered.candidates) {
            if (candidate.status == PokeVault::Integration::Gen4::CandidateStatus::UnsupportedWrapper)
                ++wrappers;
            else if (candidate.status == PokeVault::Integration::Gen4::CandidateStatus::UnsupportedSavestate)
                ++savestates;
            appendReady(std::move(candidate));
        }

        // A manually chosen source may live outside every known emulator root. Keep the remembered
        // assignment available in the provider-neutral chooser for source setup/replacement. Normal
        // Product Home open may use the exact remembered binding directly, but only through the
        // strict read-only openAssignedSource() revalidation path.
        if (legacyBindings) {
            const auto remembered = legacyBindings->resolveFileForGame(
                currentProfileIdentity(), gen4TargetGameId);
            if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Ready) {
                auto candidate = PokeVault::Integration::Gen4::inspectSourceFile(
                    remembered.binding.sourcePath,
                    remembered.binding.sourceType.empty() ? "Remembered" : remembered.binding.sourceType,
                    gen4TargetGameId);
                if (!candidate.ready())
                    rememberedDiagnostic = candidate.diagnostic;
                appendReady(std::move(candidate), true);
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Missing) {
                rememberedDiagnostic = "Remembered save is missing; choose or discover another source.";
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Unreadable) {
                rememberedDiagnostic = "Remembered save is unreadable; choose or discover another source.";
            } else if (remembered.status == PokeVault::Legacy::AssignedFileStatus::Ambiguous) {
                rememberedDiagnostic = "Remembered source assignment is ambiguous; choose a replacement.";
            }
        }

        // Match the classic chooser through the same provider-neutral ordering rule.
        PokeVault::Source::sortNewestFirst(gen4Instances);
        gen4CandidateIndex = 0;
        gen4CandidateScroll = 0;
        if (gen4Instances.empty()) {
            gen4Notice = !rememberedDiagnostic.empty() ? rememberedDiagnostic
                : discovered.limitReached
                    ? "No compatible save found before the bounded scan limit."
                : savestates > 0
                    ? "DraStic savestate found (.dss). PokeBank needs the cartridge save in /switch/drastic/user/backup/."
                : wrappers > 0
                    ? "Unsupported .dsv wrapper found. Valid footer-declared 0x80000 containers are supported read-only."
                    : "No compatible Gen IV cartridge save found in known emulator locations.";
            overlay = Overlay::Gen4Setup;
        } else {
            gen4Notice = std::to_string(gen4Instances.size()) +
                (gen4Instances.size() == 1 ? " validated save instance." : " validated save instances.");
            overlay = Overlay::Gen4Candidates;
        }
    }

    bool SaveSelectScreen::assignGen4Candidate(
        const PokeVault::Integration::Gen4::SourceCandidate& candidate) {
        if (!legacyBindings || !candidate.ready() ||
            !PokeVault::Integration::Gen4::candidateMatchesGame(candidate, gen4TargetGameId))
            return false;
        const std::string profile = currentProfileIdentity();
        if (profile.empty()) return false;
        const auto fresh = PokeVault::Integration::Gen4::inspectSourceFile(
            candidate.path, candidate.sourceType, gen4TargetGameId);
        const auto shownInstance = PokeVault::Integration::Gen4::toSaveInstance(candidate, gen4TargetGameId);
        auto freshInstance = PokeVault::Integration::Gen4::toSaveInstance(fresh, gen4TargetGameId);
        legacyBindings->applyClaims(freshInstance);
        if (!PokeVault::Source::sameValidatedSnapshot(shownInstance, freshInstance) ||
            !PokeVault::Source::visibleToProfile(freshInstance, profile)) {
            discoverGen4Candidates();
            gen4Notice = "That save changed or is no longer available to this profile. Review the refreshed list.";
            return false;
        }

        PokeVault::Legacy::BindingRecord binding;
        binding.profileIdentity = profile;
        binding.gameIdentity = gen4TargetGameId;
        binding.sourcePath = candidate.path;
        binding.sourceType = candidate.sourceType;
        binding.expectedRawFamily = candidate.expectedRawFamily;
        if (!legacyBindings->replaceFileAssignmentAndSave(candidate.sourceIdentity, std::move(binding))) {
            gen4Notice = legacyBindings->lastError().find("conflict") != std::string::npos
                ? "That physical save is already remembered by another profile or game. Forget it there first."
                : "The save assignment could not be stored safely.";
            overlay = Overlay::Gen4Setup;
            return false;
        }

        loadGen4Cards();
        gen4Notice.clear();
        const UserEntry* user = currentUser();
        if (user) {
            const auto found = std::find_if(user->titles.begin(), user->titles.end(),
                [&](const auto& title) {
                    return title.sourceKind == SelectedSourceKind::Gen4AssignedFile &&
                           title.gameId == gen4TargetGameId;
                });
            if (found != user->titles.end())
                titleIndex = static_cast<int>(std::distance(user->titles.begin(), found));
        }
        scrollSelectionIntoView();
        refreshHubPreview();
        if (gen4SetupFromGamesDrawer) {
            gamesDrawerIndex = titleIndex;
            gamesDrawerScroll = std::max(0, gamesDrawerIndex - 3);
            hubNotice = "Save linked to this game.";
            overlay = Overlay::GamesDrawer;
            gen4SetupFromGamesDrawer = false;
            return true;
        }
        overlay = Overlay::None;
        selectAssignedGen4Title();
        return titleSelected;
    }

    void SaveSelectScreen::chooseGen4ManualFile() {
        const auto chosen = Utils::promptText(
            "Choose Generation IV Save",
            "Full SD path to a raw cartridge save (.sav/.srm/.dsv; not .dss)",
            "", 240);
        if (!chosen.accepted) return;

        auto candidate = PokeVault::Integration::Gen4::inspectSourceFile(
            chosen.text, "Manual", gen4TargetGameId);
        if (!candidate.ready()) {
            gen4Notice = candidate.diagnostic.empty()
                ? "That file is not a compatible save for this game card."
                : candidate.diagnostic;
            overlay = Overlay::Gen4Setup;
            return;
        }
        assignGen4Candidate(candidate);
    }

    bool SaveSelectScreen::unassignCurrentGen4Game() {
        if (!legacyBindings) return false;
        const std::string profile = currentProfileIdentity();
        if (!legacyBindings->unassignGameAndSave(profile, gen4TargetGameId)) {
            gen4Notice = "No remembered save could be removed for this game.";
            return false;
        }
        loadGen4Cards();
        const UserEntry* user = currentUser();
        if (user && !user->titles.empty())
            titleIndex = std::min<int>(titleIndex, static_cast<int>(user->titles.size()) - 1);
        scrollSelectionIntoView();
        refreshHubPreview();
        gen4Notice = "Remembered save removed. The source file itself was not changed.";
        overlay = Overlay::Gen4Setup;
        return true;
    }

    void SaveSelectScreen::selectAssignedGen4Title() {
        const UserEntry* user = currentUser();
        if (!user) return;
        const auto found = std::find_if(user->titles.begin(), user->titles.end(),
            [&](const auto& title) {
                return title.sourceKind == SelectedSourceKind::Gen4AssignedFile &&
                       title.gameId == gen4TargetGameId;
            });
        if (found == user->titles.end()) return;
        titleIndex = static_cast<int>(std::distance(user->titles.begin(), found));
        const auto opened = legacyBindings
            ? PokeVault::Integration::Gen4::openAssignedSource(
                *legacyBindings, currentProfileIdentity(), found->gameId)
            : PokeVault::Integration::Gen4::AssignedSourceReadOnly{};
        if (opened.status != PokeVault::Integration::Gen4::OpenStatus::Ready) {
            openGen4Setup(found->gameId, opened.diagnostic);
            return;
        }
        selectedUserUid = user->uid;
        selectedTitleId = 0;
        selectedTitleName = found->name;
        selectedGameId = found->gameId;
        selectedSourceKind = found->sourceKind;
        titleSelected = true;
    }

    void SaveSelectScreen::selectCurrentTitle() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size())) return;

        // Snapshot the selected identity before any source refresh. Never keep a list index across
        // discovery: discovery can legitimately reorder cards. Hardware also showed the old grid
        // visibly reshuffling while an A-open blocked, so identity must survive any refresh.
        const TitleEntry selected = user->titles[static_cast<size_t>(titleIndex)];
        const std::string selectedGameId = selected.gameId;
        const std::string profile = currentProfileIdentity();

        if (selected.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (!legacyCatalog || selected.legacyInstances.empty()) {
                hubNotice = "No validated save is available for this game.";
                return;
            }

            int sourceIndex = 0;
            if (selected.legacyInstances.size() > 1) {
                sourceIndex = preferredLegacySourceIndex(
                    selected.legacyInstances, legacyBindings, profile, selectedGameId);
                if (sourceIndex < 0) {
                    legacyInstanceIndex = 0;
                    legacyInstanceScroll = 0;
                    legacyNotice = sourceIndex == -2
                        ? "The remembered save choice is ambiguous. Choose the exact save again."
                        : "Choose the exact validated save for " + selected.label + " once.";
                    overlay = Overlay::LegacyInstances;
                    return;
                }
            }

            const auto shown = selected.legacyInstances[static_cast<size_t>(sourceIndex)];
            auto refreshed = PokeVault::Legacy::discoverConfiguredLegacySaves();
            *legacyCatalog = std::move(refreshed);
            loadLegacySources(*legacyCatalog);

            user = currentUser();
            if (!user) {
                hubNotice = "The selected save is no longer available.";
                return;
            }

            const auto parent = std::find_if(user->titles.begin(), user->titles.end(),
                [&](const auto& candidate) {
                    return candidate.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                           candidate.gameId == selectedGameId;
                });
            if (parent == user->titles.end()) {
                hubNotice = "The selected game no longer has a validated save.";
                refreshHubPreview();
                return;
            }

            // Re-map by stable game/source identity after refresh; never by the old titleIndex.
            titleIndex = static_cast<int>(std::distance(user->titles.begin(), parent));
            const auto instance = std::find_if(parent->legacyInstances.begin(),
                parent->legacyInstances.end(), [&](const auto& candidate) {
                    return candidate.sourceIdentity == shown.sourceIdentity;
                });
            if (instance == parent->legacyInstances.end() ||
                !PokeVault::Source::sameValidatedSnapshot(shown, *instance)) {
                hubNotice = "That save changed while opening. Nothing was opened.";
                refreshHubPreview();
                return;
            }

            selectedUserUid = user->uid;
            selectedTitleId = 0;
            selectedTitleName = parent->name;
            this->selectedGameId = parent->gameId;
            selectedSourceKind = parent->sourceKind;
            selectedLegacySourceIndex = instance->sourceIndex;
            titleSelected = true;
            return;
        }

        if (selected.sourceKind == SelectedSourceKind::Gen4AssignedFile) {
            if (!legacyBindings) {
                openGen4Setup(selectedGameId, "Choose a validated read-only save for this game.");
                return;
            }
            const auto opened = PokeVault::Integration::Gen4::openAssignedSource(
                *legacyBindings, profile, selectedGameId);
            if (opened.status != PokeVault::Integration::Gen4::OpenStatus::Ready || !opened.save) {
                openGen4Setup(selectedGameId,
                    opened.diagnostic.empty()
                        ? "The remembered save no longer validates."
                        : opened.diagnostic);
                return;
            }

            // The remembered binding already names the exact source. Open that exact game directly;
            // do not re-enter the old candidate grid and do not let a refreshed index pick a neighbor.
            selectedUserUid = user->uid;
            selectedTitleId = 0;
            selectedTitleName = selected.name;
            this->selectedGameId = selectedGameId;
            selectedSourceKind = selected.sourceKind;
            titleSelected = true;
            return;
        }

        selectedUserUid  = user->uid;
        selectedTitleId  = selected.titleId;
        selectedTitleName = selected.name;
        this->selectedGameId = selectedGameId;
        selectedSourceKind = selected.sourceKind;
        titleSelected = true;
    }

    void SaveSelectScreen::selectCurrentTitleForItems() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size())) {
            hubNotice = "Choose a game before opening Items.";
            return;
        }

        const auto title = user->titles[static_cast<size_t>(titleIndex)];
        openIntent = OpenIntent::Items;

        if (title.sourceKind == SelectedSourceKind::SwitchTitle) {
            selectedUserUid = user->uid;
            selectedTitleId = title.titleId;
            selectedTitleName = title.name;
            selectedGameId = title.gameId;
            selectedSourceKind = title.sourceKind;
            titleSelected = true;
            return;
        }

        if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile) {
            if (!legacyBindings) {
                hubNotice = "Items needs a remembered validated save for this game.";
                return;
            }
            const auto opened = PokeVault::Integration::Gen4::openAssignedSource(
                *legacyBindings, currentProfileIdentity(), title.gameId);
            if (opened.status != PokeVault::Integration::Gen4::OpenStatus::Ready || !opened.save) {
                hubNotice = "The remembered save no longer validates. Re-link it from Source / Game File.";
                return;
            }
            selectedUserUid = user->uid;
            selectedTitleId = 0;
            selectedTitleName = title.name;
            selectedGameId = title.gameId;
            selectedSourceKind = title.sourceKind;
            titleSelected = true;
            return;
        }

        if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (!legacyCatalog || title.legacyInstances.empty()) {
                hubNotice = "Items needs one validated save for this game.";
                return;
            }

            int sourceIndex = 0;
            if (title.legacyInstances.size() > 1) {
                sourceIndex = preferredLegacySourceIndex(
                    title.legacyInstances, legacyBindings, currentProfileIdentity(), title.gameId);
                if (sourceIndex < 0) {
                    hubNotice = sourceIndex == -2
                        ? "The remembered save choice is ambiguous. Re-select it from Source / Game File."
                        : "Multiple saves exist. Open Source / Game File once to choose the exact save.";
                    return;
                }
            }

            const auto shown = title.legacyInstances[static_cast<size_t>(sourceIndex)];
            auto refreshed = PokeVault::Legacy::discoverConfiguredLegacySaves();
            *legacyCatalog = std::move(refreshed);
            loadLegacySources(*legacyCatalog);

            user = currentUser();
            if (!user) {
                hubNotice = "The selected save is no longer available.";
                return;
            }
            const auto parent = std::find_if(user->titles.begin(), user->titles.end(),
                [&](const auto& candidate) {
                    return candidate.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                           candidate.gameId == title.gameId;
                });
            if (parent == user->titles.end()) {
                hubNotice = "The selected save is no longer available.";
                refreshHubPreview();
                return;
            }
            titleIndex = static_cast<int>(std::distance(user->titles.begin(), parent));
            const auto instance = std::find_if(parent->legacyInstances.begin(), parent->legacyInstances.end(),
                [&](const auto& candidate) {
                    return candidate.sourceIdentity == shown.sourceIdentity;
                });
            if (instance == parent->legacyInstances.end() ||
                !PokeVault::Source::sameValidatedSnapshot(shown, *instance)) {
                hubNotice = "That save changed. Nothing was opened.";
                refreshHubPreview();
                return;
            }

            selectedUserUid = user->uid;
            selectedTitleId = 0;
            selectedTitleName = parent->name;
            selectedGameId = parent->gameId;
            selectedSourceKind = parent->sourceKind;
            selectedLegacySourceIndex = instance->sourceIndex;
            titleSelected = true;
            return;
        }

        hubNotice = "Items is unavailable for this selected source.";
    }

    void SaveSelectScreen::selectCurrentLegacyInstance() {
        const UserEntry* u = currentUser();
        if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) return;
        const auto& title = u->titles[titleIndex];
        if (title.sourceKind != SelectedSourceKind::RetroArchFRLG ||
            legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(title.legacyInstances.size())) return;
        // Reread the active physical root at the selection boundary. If this child was replaced or
        // deleted while the picker was open, the stale instance can no longer resolve to anything.
        const std::string gameId = title.gameId;
        const auto shownInstance = title.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
        const std::string sourceIdentity = shownInstance.sourceIdentity;
        if (!refreshLegacySources(gameId, sourceIdentity, true)) return;
        u = currentUser();
        if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) return;
        const auto& refreshedTitle = u->titles[titleIndex];
        if (legacyInstanceIndex < 0 ||
            legacyInstanceIndex >= static_cast<int>(refreshedTitle.legacyInstances.size())) return;

        const auto& freshInstance = refreshedTitle.legacyInstances[static_cast<size_t>(legacyInstanceIndex)];
        if (!PokeVault::Source::sameValidatedSnapshot(shownInstance, freshInstance)) {
            legacyNotice = "That save changed. Review the refreshed list before opening.";
            overlay = Overlay::LegacyInstances;
            return;
        }
        if (!legacyBindings || !legacyBindings->preferGameSourceAndSave(
                freshInstance, currentProfileIdentity(), refreshedTitle.gameId)) {
            legacyNotice = "Couldn't remember that exact save choice. Nothing was opened.";
            overlay = Overlay::LegacyInstances;
            return;
        }
        selectedUserUid = u->uid;
        selectedTitleId = 0;
        selectedTitleName = refreshedTitle.name;
        selectedGameId = refreshedTitle.gameId;
        selectedSourceKind = refreshedTitle.sourceKind;
        selectedLegacySourceIndex =
            refreshedTitle.legacyInstances[static_cast<size_t>(legacyInstanceIndex)].sourceIndex;
        titleSelected = true;
    }

    void SaveSelectScreen::activateHubDock() {
        const UserEntry* user = currentUser();
        const bool hasGame = user && titleIndex >= 0 &&
            titleIndex < static_cast<int>(user->titles.size());

        if (hubDockIndex == 0) {
            classicGamesActive = true;
            overlay = Overlay::None;
            hubNotice.clear();
            hubDockFocused = false;
            hubFeatureIndex = -1;
            scrollClassicSelectionIntoView();
        } else if (hubDockIndex == 1) {
            requestedMainMenuDestination = MainMenuDestination::Banks;
            exitRequested = true;
        } else if (hubDockIndex == 2) {
            if (!hasGame) {
                hubNotice = "Choose a game before opening Items.";
                return;
            }
            selectCurrentTitleForItems();
        } else if (hubDockIndex == 3) {
            requestedMainMenuDestination = MainMenuDestination::Search;
            exitRequested = true;
        } else {
            requestedMainMenuDestination = MainMenuDestination::More;
            exitRequested = true;
        }
    }

    void SaveSelectScreen::activateGameWorkspace() {
        const UserEntry* user = currentUser();
        if (!user || titleIndex < 0 || titleIndex >= static_cast<int>(user->titles.size())) {
            hubNotice = "Choose a game before opening a workspace destination.";
            overlay = Overlay::None;
            return;
        }

        const auto& title = user->titles[static_cast<size_t>(titleIndex)];
        switch (gameWorkspaceIndex) {
            case 0: // Overview
            case 1: // Party
            case 2: // Boxes
            case 4: // Trainer
            case 5: // Editor / Create
                // These destinations already live inside the validated existing game workspace.
                // Open the exact selected source; the editor/back-end remains owned by MAIN.
                overlay = Overlay::None;
                selectCurrentTitle();
                return;
            case 3: // Pokédex
                requestedMainMenuDestination = MainMenuDestination::Pokedex;
                exitRequested = true;
                return;
            case 6: // Backups
                if (title.sourceKind == SelectedSourceKind::SwitchTitle) {
                    openIntent = OpenIntent::Backups;
                    overlay = Overlay::None;
                    selectCurrentTitle();
                } else {
                    hubNotice = "External emulator sources stay read-only; staged/export history is managed inside the game workspace.";
                }
                return;
            case 7: // Source / Game File
                if (title.sourceKind == SelectedSourceKind::RetroArchFRLG) {
                    legacyInstanceIndex = 0;
                    legacyInstanceScroll = 0;
                    legacyNotice.clear();
                    overlay = Overlay::LegacyInstances;
                } else if (title.sourceKind == SelectedSourceKind::Gen4AssignedFile) {
                    openGen4Setup(title.gameId,
                        "Review or change this game's remembered read-only save source.");
                } else if (launchDescriptor.state == GameLaunchState::NeedsContentLink) {
                    beginLaunchLinkForCurrentTitle();
                } else {
                    hubNotice = "Nintendo save data is console-managed. Use Launch if a separate game file needs linking.";
                }
                return;
            default:
                return;
        }
    }

    void SaveSelectScreen::update(const PadState& pad, const TouchInput& touch) {
        // A tap on a nav-bar badge becomes that button's press, so every handler below is
        // reached identically whether the user pressed the button or tapped its on-screen badge.
        const HidAnalogStickState stick = padGetStickPos(&pad, 0);
        u64 kDown = controllerNavigation.apply(
            padGetButtonsDown(&pad), padGetButtons(&pad), stick.x, stick.y,
            HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right)
            | navTouchButton(touch);

        if (overlay == Overlay::GamesDrawer) {
            const UserEntry* drawerUser = currentUser();
            const int count = drawerUser ? static_cast<int>(drawerUser->titles.size()) : 0;
            if (kDown & (HidNpadButton_B | HidNpadButton_Y)) {
                overlay = Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_X) {
                if (drawerUser && gamesDrawerIndex >= 0 && gamesDrawerIndex < count) {
                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubPreview();
                    const auto& game = drawerUser->titles[static_cast<size_t>(gamesDrawerIndex)];
                    if (game.sourceKind == SelectedSourceKind::Gen4AssignedFile) {
                        openGen4Setup(game.gameId, "Assign, repair, or change this game's save source.", true);
                    } else if (game.sourceKind == SelectedSourceKind::RetroArchFRLG) {
                        legacyInstanceIndex = 0;
                        legacyInstanceScroll = 0;
                        legacyNotice.clear();
                        overlay = Overlay::LegacyInstances;
                    } else if (launchDescriptor.state == GameLaunchState::NeedsContentLink) {
                        beginLaunchLinkForCurrentTitle();
                    } else if (!unassignedLegacySources.empty()) {
                        legacyAssignmentIndex = 0;
                        legacyAssignmentScroll = 0;
                        legacyNotice.clear();
                        overlay = Overlay::LegacyAssignment;
                    } else {
                        hubNotice = "This save source is already managed by the console.";
                    }
                }
                return;
            }
            if (count > 0) {
                constexpr int cols = 3;
                const int row = gamesDrawerIndex / cols;
                const int col = gamesDrawerIndex % cols;
                if (kDown & HidNpadButton_Left) {
                    if (gamesDrawerIndex > 0) --gamesDrawerIndex;
                }
                if (kDown & HidNpadButton_Right) {
                    if (gamesDrawerIndex + 1 < count) ++gamesDrawerIndex;
                }
                if (kDown & HidNpadButton_Up) {
                    if (row > 0) gamesDrawerIndex -= cols;
                }
                if (kDown & HidNpadButton_Down) {
                    const int candidate = gamesDrawerIndex + cols;
                    if (candidate < count) gamesDrawerIndex = candidate;
                    else {
                        const int lastRowFirst = ((count - 1) / cols) * cols;
                        gamesDrawerIndex = std::min(count - 1, lastRowFirst + col);
                    }
                }
                constexpr int visibleRows = 4;
                const int selectedRow = gamesDrawerIndex / cols;
                if (selectedRow < gamesDrawerScroll)
                    gamesDrawerScroll = selectedRow;
                else if (selectedRow >= gamesDrawerScroll + visibleRows)
                    gamesDrawerScroll = selectedRow - visibleRows + 1;
                if (kDown & HidNpadButton_A) {
                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubPreview();
                    overlay = Overlay::None;
                }
            }
            return;
        }

        if (overlay == Overlay::ProfilePicker) {
            const int count = static_cast<int>(users.size());
            if (kDown & HidNpadButton_B) {
                overlay = Overlay::None;
                return;
            }
            if (count > 0) {
                if (kDown & HidNpadButton_Up)
                    profilePickerIndex = (profilePickerIndex - 1 + count) % count;
                if (kDown & HidNpadButton_Down)
                    profilePickerIndex = (profilePickerIndex + 1) % count;
                if (kDown & HidNpadButton_A) {
                    setUser(profilePickerIndex);
                    gamesDrawerIndex = titleIndex;
                    overlay = Overlay::None;
                    headerActionIndex = -1;
                }
            }
            return;
        }

        if (overlay == Overlay::GameWorkspace) {
            constexpr int count = 8;
            constexpr int columns = 2;
            if (kDown & HidNpadButton_B) {
                overlay = Overlay::None;
                hubNotice.clear();
                return;
            }
            if (kDown & HidNpadButton_ZR) {
                launchCurrentTitle();
                return;
            }
            if (kDown & HidNpadButton_Plus) {
                overlay = Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_Minus) {
                helpReturnOverlay = Overlay::GameWorkspace;
                overlay = Overlay::Help;
                return;
            }
            const int row = gameWorkspaceIndex / columns;
            const int col = gameWorkspaceIndex % columns;
            if (kDown & HidNpadButton_Left)
                gameWorkspaceIndex = row * columns + (col + columns - 1) % columns;
            if (kDown & HidNpadButton_Right)
                gameWorkspaceIndex = row * columns + (col + 1) % columns;
            if (kDown & HidNpadButton_Up)
                gameWorkspaceIndex = ((row + 3) % 4) * columns + col;
            if (kDown & HidNpadButton_Down)
                gameWorkspaceIndex = ((row + 1) % 4) * columns + col;
            gameWorkspaceIndex = std::clamp(gameWorkspaceIndex, 0, count - 1);
            if (kDown & HidNpadButton_A) activateGameWorkspace();
            return;
        }

        if (overlay == Overlay::GameFilePicker) {
            const int count = static_cast<int>(launchFileEntries.size());
            if (kDown & HidNpadButton_B) {
                overlay = launchFileReturnToLegacy ? Overlay::LegacyInstances : Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_Y) {
                browseGameFileParent();
                return;
            }
            if (kDown & HidNpadButton_X) {
                launchBrowsePath = launchBrowseRoot;
                refreshGameFilePicker();
                return;
            }
            if (count > 0) {
                if (kDown & HidNpadButton_Up)
                    launchFileIndex = (launchFileIndex - 1 + count) % count;
                if (kDown & HidNpadButton_Down)
                    launchFileIndex = (launchFileIndex + 1) % count;
                constexpr int visibleRows = 7;
                if (launchFileIndex < launchFileScroll)
                    launchFileScroll = launchFileIndex;
                else if (launchFileIndex >= launchFileScroll + visibleRows)
                    launchFileScroll = launchFileIndex - visibleRows + 1;
                if (kDown & HidNpadButton_A) activateGameFilePicker();
            }
            return;
        }
        if (overlay == Overlay::Help) {
            if (kDown & (HidNpadButton_B | HidNpadButton_Minus)) {
                overlay = helpReturnOverlay;
                helpReturnOverlay = Overlay::None;
            }
            return;
        }
        if (overlay == Overlay::LegacyDetails) {
            if (kDown & (HidNpadButton_B | HidNpadButton_Y)) overlay = Overlay::LegacyInstances;
            return;
        }
        if (overlay == Overlay::LegacyAssignment) {
            const int count = static_cast<int>(unassignedLegacySources.size());
            if (kDown & HidNpadButton_B) { overlay = Overlay::GamesDrawer; return; }
            if (kDown & HidNpadButton_X) {
                if (legacyCatalog) {
                    *legacyCatalog = PokeVault::Legacy::discoverConfiguredLegacySaves();
                    loadLegacySources(*legacyCatalog);
                    refreshHubPreview();
                    legacyNotice = "Unassigned source list refreshed.";
                }
                if (unassignedLegacySources.empty()) overlay = Overlay::GamesDrawer;
                return;
            }
            if (count == 0) { overlay = Overlay::GamesDrawer; return; }
            if (kDown & HidNpadButton_Up)
                legacyAssignmentIndex = (legacyAssignmentIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                legacyAssignmentIndex = (legacyAssignmentIndex + 1) % count;
            constexpr int visibleRows = 5;
            if (legacyAssignmentIndex < legacyAssignmentScroll)
                legacyAssignmentScroll = legacyAssignmentIndex;
            else if (legacyAssignmentIndex >= legacyAssignmentScroll + visibleRows)
                legacyAssignmentScroll = legacyAssignmentIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) assignCurrentLegacySource();
            return;
        }
        if (overlay == Overlay::Gen4Setup) {
            if (kDown & HidNpadButton_B) {
                overlay = gen4SetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;
                if (overlay == Overlay::GamesDrawer) {
                    gamesDrawerIndex = titleIndex;
                    gamesDrawerScroll = std::max(0, gamesDrawerIndex - 3);
                }
                gen4SetupFromGamesDrawer = false;
                return;
            }
            if (kDown & HidNpadButton_Up) gen4SetupIndex = (gen4SetupIndex + 3) % 4;
            if (kDown & HidNpadButton_Down) gen4SetupIndex = (gen4SetupIndex + 1) % 4;
            if (kDown & HidNpadButton_A) {
                if (gen4SetupIndex == 0) discoverGen4Candidates();
                else if (gen4SetupIndex == 1) chooseGen4ManualFile();
                else if (gen4SetupIndex == 2) unassignCurrentGen4Game();
                else {
                    overlay = gen4SetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;
                    if (overlay == Overlay::GamesDrawer) {
                        gamesDrawerIndex = titleIndex;
                        gamesDrawerScroll = std::max(0, gamesDrawerIndex - 3);
                    }
                    gen4SetupFromGamesDrawer = false;
                }
            }
            return;
        }
        if (overlay == Overlay::Gen4Candidates) {
            const int count = static_cast<int>(gen4Instances.size());
            if (kDown & HidNpadButton_B) {
                overlay = gen4SetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None;
                if (overlay == Overlay::GamesDrawer) {
                    gamesDrawerIndex = titleIndex;
                    gamesDrawerScroll = std::max(0, gamesDrawerIndex - 3);
                }
                gen4SetupFromGamesDrawer = false;
                return;
            }
            if (kDown & HidNpadButton_X) { discoverGen4Candidates(); return; }
            if (kDown & HidNpadButton_Y) {
                openGen4Setup(gen4TargetGameId,
                    "Add, repair, or forget a remembered source. Opening still happens from Save Instances.",
                    gen4SetupFromGamesDrawer);
                return;
            }
            if (count == 0) { overlay = Overlay::Gen4Setup; return; }
            if (kDown & HidNpadButton_Up)
                gen4CandidateIndex = (gen4CandidateIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                gen4CandidateIndex = (gen4CandidateIndex + 1) % count;
            constexpr int visibleRows = 5;
            if (gen4CandidateIndex < gen4CandidateScroll)
                gen4CandidateScroll = gen4CandidateIndex;
            else if (gen4CandidateIndex >= gen4CandidateScroll + visibleRows)
                gen4CandidateScroll = gen4CandidateIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) {
                const size_t handle =
                    gen4Instances[static_cast<size_t>(gen4CandidateIndex)].sourceIndex;
                if (handle < gen4Candidates.size())
                    assignGen4Candidate(gen4Candidates[handle]);
            }
            return;
        }

        if (overlay == Overlay::Options) {
            if (kDown & HidNpadButton_Up)   optionsIndex = (optionsIndex + 2) % 3;
            if (kDown & HidNpadButton_Down) optionsIndex = (optionsIndex + 1) % 3;
            if (kDown & HidNpadButton_B) { overlay = Overlay::None; return; }
            if (kDown & HidNpadButton_A) {
                if (optionsIndex == 0) {
                    applyTheme(nextThemeMode(g_themeMode));
                    Utils::saveSettings();
                } else if (optionsIndex == 1) {
                    appExitRequested = true;
                    exitRequested = true;
                } else {
                    overlay = Overlay::None;
                }
            }
            return;
        }
        if (overlay == Overlay::LegacyInstances) {
            const UserEntry* u = currentUser();
            if (!u || titleIndex < 0 || titleIndex >= static_cast<int>(u->titles.size())) {
                overlay = Overlay::None;
                return;
            }
            const auto& instances = u->titles[titleIndex].legacyInstances;
            const int count = static_cast<int>(instances.size());
            if (kDown & HidNpadButton_B) {
                overlay = Overlay::None;
                launchLegacyMode = false;
                return;
            }
            if (kDown & HidNpadButton_X) {
                const std::string gameId = u->titles[titleIndex].gameId;
                std::string preferred;
                if (legacyInstanceIndex >= 0 && legacyInstanceIndex < count)
                    preferred = instances[static_cast<size_t>(legacyInstanceIndex)].sourceIdentity;
                refreshLegacySources(gameId, preferred, false);
                return;
            }
            if (kDown & HidNpadButton_Y) {
                const auto* instance = currentLegacyInstance();
                if (instance) {
                    legacyDetailsInstance = *instance;
                    legacyDetailsGameId = u->titles[titleIndex].gameId;
                    overlay = Overlay::LegacyDetails;
                }
                return;
            }
            if (count == 0) {
                overlay = Overlay::None;
                return;
            }
            if (kDown & HidNpadButton_Up)
                legacyInstanceIndex = (legacyInstanceIndex - 1 + count) % count;
            if (kDown & HidNpadButton_Down)
                legacyInstanceIndex = (legacyInstanceIndex + 1) % count;
            constexpr int visibleRows = 6;
            if (legacyInstanceIndex < legacyInstanceScroll)
                legacyInstanceScroll = legacyInstanceIndex;
            else if (legacyInstanceIndex >= legacyInstanceScroll + visibleRows)
                legacyInstanceScroll = legacyInstanceIndex - visibleRows + 1;
            if (kDown & HidNpadButton_A) {
                if (launchLegacyMode) launchCurrentLegacyInstance();
                else selectCurrentLegacyInstance();
            }
            return;
        }

        if (classicGamesActive) {
            if (kDown & HidNpadButton_B) {
                classicGamesActive = false;
                scrollRow = 0;
                refreshHubPreview();
                return;
            }
            if (kDown & HidNpadButton_Minus) {
                helpReturnClassicGames = true;
                helpReturnOverlay = Overlay::None;
                overlay = Overlay::Help;
                return;
            }
            if (kDown & HidNpadButton_ZR) {
                launchCurrentTitle();
                return;
            }
            const int beforeUser = userIndex;
            const int beforeTitle = titleIndex;
            if (users.size() > 1) {
                if (kDown & HidNpadButton_L) setUser(userIndex - 1);
                if (kDown & HidNpadButton_R) setUser(userIndex + 1);
            }

            const UserEntry* classicUser = currentUser();
            const int classicCount = classicUser ? static_cast<int>(classicUser->titles.size()) : 0;
            if (classicCount > 0) {
                const int cols = classicTitleColumns();
                if (kDown & HidNpadButton_Left)
                    titleIndex = (titleIndex - 1 + classicCount) % classicCount;
                if (kDown & HidNpadButton_Right)
                    titleIndex = (titleIndex + 1) % classicCount;
                if ((kDown & HidNpadButton_Up) && titleIndex - cols >= 0)
                    titleIndex -= cols;
                if ((kDown & HidNpadButton_Down) && titleIndex + cols < classicCount)
                    titleIndex += cols;

                if (kDown & HidNpadButton_A) {
                    openIntent = OpenIntent::Default;
                    selectCurrentTitle();
                    // Do not mount/reparse the selected save again on the same input frame.
                    // The outer UI loop must observe titleSelected/overlay immediately.
                    return;
                }

                if (userIndex == beforeUser && titleIndex != beforeTitle) {
                    scrollClassicSelectionIntoView();
                    refreshHubPreview();
                }
            }
            return;
        }

        if (kDown & HidNpadButton_B) {
            // Games is now the app root, so Back exits just as the former root Home did.
            appExitRequested = true;
            exitRequested = true;
            return;
        }
        if (kDown & HidNpadButton_ZR) {
            launchCurrentTitle();
            return;
        }
        if (kDown & HidNpadButton_Plus) {
            const UserEntry* current = currentUser();
            if (current && titleIndex >= 0 &&
                titleIndex < static_cast<int>(current->titles.size())) {
                gameWorkspaceIndex = 0;
                hubNotice.clear();
                overlay = Overlay::GameWorkspace;
            } else {
                hubNotice = "Choose a game before opening the Current Game menu.";
            }
            return;
        }
        if (kDown & HidNpadButton_Y) {
            const UserEntry* quickUser = currentUser();
            const int quickCount = quickUser ? static_cast<int>(quickUser->titles.size()) : 0;
            gamesDrawerIndex = quickCount > 0 ? titleIndex : 0;
            gamesDrawerScroll = std::max(0, gamesDrawerIndex / 3 - 1);
            hubNotice.clear();
            overlay = Overlay::GamesDrawer;
            return;
        }
        if (kDown & HidNpadButton_Minus) {
            helpReturnOverlay = Overlay::None;
            overlay = Overlay::Help;
            return;
        }
        // Touch uses the same actions as controller focus. Fine-grained app-wide touch parity is
        // intentionally a later tranche; these existing primary hit targets remain safe now.
        if (touch.justPressed()) {
            const int tx = touch.x(), ty = touch.y();
            for (const auto& r : headerRects) {
                if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h) {
                    headerActionIndex = r.idx;
                    hubDockFocused = false;
                    hubFeatureIndex = -1;
                    if (headerActionIndex == 0) {
                        profilePickerIndex = userIndex;
                        overlay = Overlay::ProfilePicker;
                    } else {
                        requestedMainMenuDestination = MainMenuDestination::Settings;
                        exitRequested = true;
                    }
                    return;
                }
            }
            for (const auto& r : dockRects) {
                if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h) {
                    hubDockFocused = true;
                    hubFeatureIndex = -1;
                    hubDockIndex = r.idx;
                    activateHubDock();
                    return;
                }
            }
            for (const auto& r : titleRects) {
                if (tx >= r.x && tx < r.x + r.w && ty >= r.y && ty < r.y + r.h) {
                    hubDockFocused = false;
                    hubFeatureIndex = -1;
                    kDown |= HidNpadButton_A;
                    break;
                }
            }
        }

        // L/R changes the selected game from anywhere on Product Home, matching the footer.
        const UserEntry* homeUser = currentUser();
        const int homeGameCount = homeUser ? static_cast<int>(homeUser->titles.size()) : 0;
        if (homeGameCount > 0) {
            const int before = titleIndex;
            if (kDown & HidNpadButton_L)
                titleIndex = (titleIndex - 1 + homeGameCount) % homeGameCount;
            if (kDown & HidNpadButton_R)
                titleIndex = (titleIndex + 1) % homeGameCount;
            if (titleIndex != before) {
                scrollSelectionIntoView();
                refreshHubPreview();
            }
        }

        if (headerActionIndex >= 0) {
            if (kDown & (HidNpadButton_Left | HidNpadButton_Right))
                headerActionIndex = headerActionIndex == 0 ? 1 : 0;
            if (kDown & HidNpadButton_Down) {
                hubFeatureIndex = headerActionIndex == 0 ? 0 : 1;
                headerActionIndex = -1;
                return;
            }
            if (kDown & HidNpadButton_A) {
                if (headerActionIndex == 0) {
                    profilePickerIndex = userIndex;
                    overlay = Overlay::ProfilePicker;
                } else {
                    requestedMainMenuDestination = MainMenuDestination::Settings;
                    exitRequested = true;
                }
            }
            return;
        }

        if (hubDockFocused) {
            if (kDown & HidNpadButton_Left) {
                if (hubDockIndex == 0) hubDockIndex = 2;
                else if (hubDockIndex == 3) hubDockIndex = 4;
                else --hubDockIndex;
            }
            if (kDown & HidNpadButton_Right) {
                if (hubDockIndex == 2) hubDockIndex = 0;
                else if (hubDockIndex == 4) hubDockIndex = 3;
                else ++hubDockIndex;
            }
            if (kDown & HidNpadButton_Up) {
                if (hubDockIndex >= 3) hubDockIndex -= 3;
                else {
                    hubFeatureIndex = hubDockIndex <= 1 ? 0 : 1;
                    hubDockFocused = false;
                }
                return;
            }
            if (kDown & HidNpadButton_Down) {
                if (hubDockIndex < 3)
                    hubDockIndex = hubDockIndex <= 1 ? hubDockIndex + 3 : 4;
                return;
            }
            if (kDown & HidNpadButton_A) activateHubDock();
            return;
        }

        if (hubFeatureIndex >= 0) {
            if (kDown & HidNpadButton_Left) {
                if (hubFeatureIndex == 1) hubFeatureIndex = 0;
                else hubFeatureIndex = -1;
                return;
            }
            if (kDown & HidNpadButton_Right) {
                if (hubFeatureIndex == 0) hubFeatureIndex = 1;
                return;
            }
            if (kDown & HidNpadButton_Up) {
                headerActionIndex = hubFeatureIndex == 0 ? 0 : 1;
                hubFeatureIndex = -1;
                return;
            }
            if (kDown & HidNpadButton_Down) {
                hubDockFocused = true;
                hubDockIndex = hubFeatureIndex == 0 ? 0 : 3;
                hubFeatureIndex = -1;
                return;
            }
            if (kDown & HidNpadButton_A) {
                requestedMainMenuDestination = hubFeatureIndex == 0
                    ? MainMenuDestination::MasterVault
                    : MainMenuDestination::Pokedex;
                exitRequested = true;
            }
            return;
        }

        if (kDown & HidNpadButton_Up) {
            headerActionIndex = 0;
            return;
        }
        if (kDown & HidNpadButton_Right) {
            hubFeatureIndex = 0;
            return;
        }
        if (kDown & HidNpadButton_Down) {
            hubDockFocused = true;
            hubDockIndex = 0;
            return;
        }

        if (homeGameCount > 0 && (kDown & HidNpadButton_A)) {
            openIntent = OpenIntent::Default;
            selectCurrentTitle();
        }
    }

    void SaveSelectScreen::drawClassicGameSources(PKSEFramebuffer& fb) {
        drawAppBackdrop(fb);
        drawTitleBar(fb, "Pokémon Games");
        const UserEntry* u = currentUser();
        drawPanelSurface(fb, 24, 82, fb.getWidth() - 48, 104, true);

        if (u) {
            const IconImage* avatar = u->name == "Pokémon Saves" ? nullptr : &SystemIcons::userIcon(u->uid);
            if (avatar && avatar->valid())
                fb.drawImageScaled(44, 92, avatar->width, avatar->height, 84, 84, avatar->data, 4);
            else
                fb.drawFilledRoundedRect(44, 92, 84, 84, 12, Colors::PanelAlt);
            fb.drawRoundedRect(44, 92, 84, 84, 12, Colors::Info, 2);
            fb.drawText(148, 103, u->name, Colors::TextPrimary, TextStyle::Title);
            fb.drawText(148, 144, std::to_string(u->titles.size()) + " available save sources",
                        Colors::TextMuted, TextStyle::Caption);
        }

        const int count = u ? static_cast<int>(u->titles.size()) : 0;
        const int cols = classicTitleColumns();
        const int gridW = cols * CLASSIC_TILE_W + (cols - 1) * CLASSIC_GAP;
        const int startX = std::max(40, (fb.getWidth() - gridW) / 2);
        const int first = scrollRow * cols;
        const int last = std::min(count, first + CLASSIC_VISIBLE_ROWS * cols);
        for (int i = first; i < last; ++i) {
            const int col = i % cols;
            const int row = (i / cols) - scrollRow;
            const int x = startX + col * (CLASSIC_TILE_W + CLASSIC_GAP);
            const int y = CLASSIC_GRID_Y + row * (CLASSIC_TILE_H + CLASSIC_GAP);
            const bool focused = i == titleIndex;
            const auto& title = u->titles[static_cast<size_t>(i)];

            drawFocusedCard(fb, x, y, CLASSIC_TILE_W, CLASSIC_TILE_H, focused, 16);
            const IconImage& art = SystemIcons::gameCardIcon(
                title.sourceKind == SelectedSourceKind::RetroArchFRLG ? title.artworkKey : title.gameId,
                title.titleId);
            if (art.valid())
                fb.drawImageScaled(x + 29, y + 14, art.width, art.height,
                                   CLASSIC_ICON, CLASSIC_ICON, art.data, 4);
            else
                fb.drawFilledRoundedRect(x + 29, y + 14, CLASSIC_ICON, CLASSIC_ICON, 12, Colors::PanelAlt);

            const auto portrait = trainerPortraitForGame(
                title.gameId, title.trainerGenderKnown, title.trainerGender);
            drawTrainerPortrait(fb, x + CLASSIC_TILE_W - 58, y + 18, 48, 58,
                                portrait, false);

            std::string label = title.label;
            if (label.size() > 18) label = label.substr(0, 17) + "…";
            int lw=0,lh=0; fb.measureText(label,lw,lh,TextStyle::Caption);
            fb.drawText(x + (CLASSIC_TILE_W-lw)/2, y + 146, label,
                        focused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Caption);

            std::string meta = title.trainerName.empty()
                ? productSourceLabel(title.sourceLabel) : title.trainerName;
            if (title.dexTotal > 0)
                meta += "  •  " + std::to_string(title.dexCaught) + "/" + std::to_string(title.dexTotal);
            if (meta.size() > 24) meta = meta.substr(0, 23) + "…";
            int mw=0,mh=0; fb.measureText(meta,mw,mh,TextStyle::Caption);
            fb.drawText(x + (CLASSIC_TILE_W-mw)/2, y + 172, meta,
                        Colors::TextMuted, TextStyle::Caption);
        }

        drawNavBar(fb, {{"D-pad/Stick","Choose Game"},{"A","Open"},
                        {"L/R","Switch User"},{"ZR","Launch"},
                        {"-","Help"},{"B","Back"}});
    }

    void SaveSelectScreen::draw(PKSEFramebuffer& fb) {
        titleRects.clear();
        userRects.clear();
        dockRects.clear();
        headerRects.clear();

        if (classicGamesActive) {
            const UserEntry* classicUser = currentUser();
            if (classicUser && !classicUser->titles.empty())
                titleIndex = std::clamp(titleIndex, 0, static_cast<int>(classicUser->titles.size()) - 1);
            else
                titleIndex = 0;
            scrollClassicSelectionIntoView();
            drawClassicGameSources(fb);
            if (overlay == Overlay::Help) {
                drawProductHelpOverlay(fb, "Pokémon Games", {
                    {"D-pad/Stick", "Choose a game"},
                    {"A", "Open the selected game"},
                    {"L/R", "Switch profile"},
                    {"ZR", "Launch the selected game or emulator"},
                    {"-", "Close Help / Controls"},
                    {"B", "Back to Product Home"}
                }, "This is the full artwork game browser. Press Y on Product Home for the quick drawer.");
            }
            return;
        }

        drawAppBackdrop(fb);
        drawProductTitleBar(fb);

        const UserEntry* u = currentUser();
        const int count = u ? static_cast<int>(u->titles.size()) : 0;

        // Header right owns the only Profile and Settings destinations.
        if (u) {
            const int avatarX = 1000, avatarY = 10;
            const bool profileFocused = headerActionIndex == 0;
            const bool settingsFocused = headerActionIndex == 1;
            const IconImage* avatar =
                u->name == "Pokémon Saves" ? nullptr : &SystemIcons::userIcon(u->uid);

            if (profileFocused)
                fb.drawFilledRoundedRect(avatarX - 10, 6, 176, 54, 24, Colors::PanelAlt);
            if (avatar && avatar->valid())
                fb.drawImageScaled(avatarX, avatarY, avatar->width, avatar->height,
                                   PROFILE_AVATAR, PROFILE_AVATAR, avatar->data, 4);
            else
                fb.drawFilledRoundedRect(avatarX, avatarY, PROFILE_AVATAR, PROFILE_AVATAR,
                                         PROFILE_AVATAR / 2, Colors::PanelAlt);
            fb.drawRoundedRect(avatarX, avatarY, PROFILE_AVATAR, PROFILE_AVATAR,
                               PROFILE_AVATAR / 2,
                               profileFocused ? Colors::FocusBorder : Colors::Divider,
                               profileFocused ? 3 : 2);

            std::string profileName = u->name;
            if (profileName.size() > 12) profileName = profileName.substr(0, 11) + "…";
            fb.drawText(avatarX + PROFILE_AVATAR + 10, 19, profileName,
                        profileFocused ? Colors::SelectedText : Colors::TextPrimary,
                        TextStyle::Heading);
            headerRects.push_back({avatarX - 10, 6, 176, 54, 0});

            const int settingsX = 1200, settingsY = 10;
            drawFocusedCard(fb, settingsX, settingsY, PROFILE_AVATAR, PROFILE_AVATAR,
                            settingsFocused, PROFILE_AVATAR / 2);
            drawHubDockIcon(fb, 5, settingsX, settingsY, PROFILE_AVATAR, settingsFocused);
            headerRects.push_back({settingsX, settingsY, PROFILE_AVATAR, PROFILE_AVATAR, 1});
        }

        // Right: selected-game hero card.
        // Historical wording is retained because the cross-lane polish contract uses this boundary
        // to prove that physical source diagnostics stay out of the normal product presentation.
        const bool gameFocused = !hubDockFocused && hubFeatureIndex < 0 && headerActionIndex < 0;
        drawFocusedCard(fb, DETAIL_X, HUB_Y, DETAIL_W, HUB_H, gameFocused, 18);
        if (gameFocused)
            fb.drawRoundedRect(DETAIL_X, HUB_Y, DETAIL_W, HUB_H, 18, Colors::Info, 3);

        if (u && titleIndex >= 0 && titleIndex < count) {
            const auto& title = u->titles[static_cast<size_t>(titleIndex)];
            const bool legacy = title.sourceKind == SelectedSourceKind::RetroArchFRLG;
            const int artX = DETAIL_X + 22;
            const int artY = HUB_Y + 24;
            const IconImage& art = SystemIcons::gameCardIcon(
                legacy ? title.artworkKey : title.gameId, title.titleId);
            if (art.valid())
                fb.drawImageScaled(artX, artY, art.width, art.height,
                                   DETAIL_ART, DETAIL_ART, art.data, 4);
            else {
                fb.drawFilledRoundedRect(artX, artY, DETAIL_ART, DETAIL_ART, 16, Colors::PanelAlt);
                fb.drawCircle(artX + DETAIL_ART / 2, artY + DETAIL_ART / 2, 48,
                              withAlpha(Colors::AccentPrimary, 120), 8);
                fb.drawFilledCircle(artX + DETAIL_ART / 2, artY + DETAIL_ART / 2,
                                    14, Colors::AccentPrimary);
            }
            fb.drawRoundedRect(artX, artY, DETAIL_ART, DETAIL_ART, 16, Colors::Divider, 1);

            const int infoX = artX + DETAIL_ART + 26;
            std::string gameTitle = title.name.empty() ? title.label : title.name;
            if (gameTitle.size() > 27) gameTitle = gameTitle.substr(0, 26) + "…";
            fb.drawText(infoX, HUB_Y + 26, gameTitle, Colors::TextPrimary, TextStyle::Title);

            fb.drawText(infoX, HUB_Y + 76, "Trainer", Colors::TextMuted, TextStyle::Caption);
            fb.drawText(infoX, HUB_Y + 99,
                        previewTrainerName.empty() ? "—" : previewTrainerName,
                        previewTrainerName.empty() ? Colors::TextMuted : Colors::TextPrimary,
                        TextStyle::Heading);
            const auto portrait = trainerPortraitForGame(
                title.gameId, previewTrainerGenderKnown, previewTrainerGender);
            drawTrainerPortrait(fb, infoX + 334, HUB_Y + 66, 94, 108, portrait, true);

            std::string sourceLine = title.platformLabel;
            if (!title.sourceLabel.empty()) sourceLine += "  •  " + productSourceLabel(title.sourceLabel);
            if (sourceLine.size() > 38) sourceLine = sourceLine.substr(0, 37) + "…";
            fb.drawText(infoX, HUB_Y + 139, sourceLine,
                        Colors::TextSecondary, TextStyle::Body);

            fb.drawFilledRoundedRect(infoX, HUB_Y + 172, DETAIL_W - (infoX - DETAIL_X) - 24,
                                     2, 1, Colors::Divider);
            fb.drawText(infoX, HUB_Y + 191, "Pokédex Progress",
                        Colors::TextSecondary, TextStyle::Body);
            if (previewDexTotal > 0) {
                const std::string dexLine =
                    "Seen " + std::to_string(previewDexSeen) + " / " +
                    std::to_string(previewDexTotal) + "   •   Owned " +
                    std::to_string(previewDexCaught) + " / " +
                    std::to_string(previewDexTotal);
                fb.drawText(infoX, HUB_Y + 218, dexLine,
                            Colors::TextPrimary, TextStyle::Caption);
                const int barW = 360;
                const int fillW = static_cast<int>(
                    (static_cast<uint32_t>(barW) * previewDexCaught) / previewDexTotal);
                fb.drawFilledRoundedRect(infoX, HUB_Y + 246, barW, 7, 3,
                                         withAlpha(Colors::TextMuted, 38));
                if (fillW > 0)
                    fb.drawFilledRoundedRect(infoX, HUB_Y + 246, fillW, 7, 3, Colors::Info);
            } else {
                fb.drawText(infoX, HUB_Y + 218, "Progress unavailable for this save format.",
                            Colors::TextMuted, TextStyle::Caption);
            }

            const int partyX = DETAIL_X + 22;
            const int partyY = HUB_Y + 278;
            fb.drawText(partyX, partyY, "PARTY", Colors::Info, TextStyle::Caption);
            if (!partyPreviewStatus.empty()) {
                std::string status = partyPreviewStatus;
                if (status.size() > 62) status = status.substr(0, 61) + "…";
                fb.drawText(partyX + 70, partyY, status, Colors::TextMuted, TextStyle::Caption);
            }

            const int slotY = partyY + 26;
            const int slotGap = 8;
            const int slotW = (DETAIL_W - 44 - slotGap * 5) / 6;
            for (int i = 0; i < 6; ++i) {
                const int sx = partyX + i * (slotW + slotGap);
                drawPanelSurface(fb, sx, slotY, slotW, 118, false, 10);
                const auto& p = partyPreview[static_cast<size_t>(i)];
                if (p.species != 0) {
                    Sprite* sprite = SpriteManager::getIconSprite(p.species, p.form, p.shiny);
                    if (sprite && sprite->data) {
                        const auto rect = PokeBank::UIModel::containSprite(
                            sx + 6, slotY + 4, slotW - 12, 66, sprite->width, sprite->height);
                        if (rect.width > 0 && rect.height > 0)
                            fb.drawImageScaled(rect.x, rect.y, sprite->width, sprite->height,
                                               rect.width, rect.height, sprite->data, sprite->channels);
                    } else {
                        const int cx = sx + slotW / 2;
                        const int cy = slotY + 34;
                        fb.drawCircle(cx, cy, 17, withAlpha(Colors::Info, 150), 2);
                        fb.drawFilledRect(cx - 17, cy - 2, 34, 4, withAlpha(Colors::Info, 110));
                        fb.drawFilledCircle(cx, cy, 6, Colors::Info);
                    }
                    if (p.shiny)
                        fb.drawShinyMark(sx + slotW - 18, slotY + 5, 12, Colors::ShinyStar);
                    std::string name = p.name;
                    if (name.size() > 10) name = name.substr(0, 9) + "…";
                    int nw = 0, nh = 0;
                    fb.measureText(name, nw, nh, TextStyle::Caption);
                    fb.drawText(sx + std::max(5, (slotW - nw) / 2), slotY + 73,
                                name, Colors::TextPrimary, TextStyle::Caption);
                    if (p.level > 0) {
                        const std::string level = "Lv. " + std::to_string(p.level);
                        int lw = 0, lh = 0;
                        fb.measureText(level, lw, lh, TextStyle::Caption);
                        fb.drawText(sx + (slotW - lw) / 2, slotY + 96,
                                    level, Colors::TextSecondary, TextStyle::Caption);
                    }
                } else {
                    fb.drawFilledCircle(sx + slotW / 2, slotY + 43, 18, Colors::PanelAlt);
                    int ew = 0, eh = 0;
                    fb.measureText("Empty", ew, eh, TextStyle::Caption);
                    fb.drawText(sx + (slotW - ew) / 2, slotY + 82,
                                "Empty", Colors::TextMuted, TextStyle::Caption);
                }
            }

            const std::string launchLabel = gameLaunchActionLabel(launchDescriptor.state);
            const bool launchActionable = launchDescriptor.ready() ||
                launchDescriptor.state == GameLaunchState::NeedsContentLink ||
                launchDescriptor.state == GameLaunchState::ChooseSource;
            const int buttonY = HUB_Y + 432;
            drawGlyphButton(fb, DETAIL_X + 22, buttonY, 322, 58, "A", "OPEN",
                            Colors::PanelAlt, Colors::TextPrimary);
            if (gameFocused)
                fb.drawRoundedRect(DETAIL_X + 22, buttonY, 322, 58, 8,
                                   Colors::Info, 3);
            drawGlyphButton(fb, DETAIL_X + 366, buttonY, 332, 58, "ZR", launchLabel,
                            launchActionable ? Colors::Info : Colors::PanelAlt,
                            launchActionable ? Colors::White : Colors::TextMuted);

            if (!hubNotice.empty()) {
                std::string notice = hubNotice;
                if (notice.size() > 92) notice = notice.substr(0, 91) + "…";
                fb.drawText(DETAIL_X + 24, HUB_Y + 505, notice, Colors::Info, TextStyle::Caption);
            }

            // The whole selected-game panel remains one safe A/Open touch target for now.
            titleRects.push_back({DETAIL_X, HUB_Y, DETAIL_W, HUB_H, titleIndex});
        } else {
            const std::string emptyHeading =
                (!u || u->name == "Pokémon Saves")
                    ? "No validated Pokémon game sources found"
                    : "No Pokémon saves found for this profile";
            fb.drawText(DETAIL_X + 34, HUB_Y + 42, emptyHeading,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(DETAIL_X + 34, HUB_Y + 82,
                        "Assign an emulator save or create a supported Switch save to begin.",
                        Colors::TextMuted, TextStyle::Body);
        }

        // Right side: compact Vault/Pokédex row above the five primary navigation buttons.
        // The selected game remains the large left anchor; the old full-width bottom dock is gone.
        constexpr int featureGap = 14;
        constexpr int featureH = 190;
        const int featureW = (RIGHT_W - featureGap) / 2;
        const bool vaultFocused = !hubDockFocused && hubFeatureIndex == 0;
        const bool dexFocused = !hubDockFocused && hubFeatureIndex == 1;
        const Color vaultAccent(72, 194, 238);
        const Color dexAccent(244, 132, 74);

        const int vaultX = RIGHT_X;
        const int dexX = RIGHT_X + featureW + featureGap;
        drawFocusedCard(fb, vaultX, HUB_Y, featureW, featureH, vaultFocused, 18);
        drawFocusedCard(fb, dexX, HUB_Y, featureW, featureH, dexFocused, 18);
        if (vaultFocused)
            fb.drawRoundedRect(vaultX, HUB_Y, featureW, featureH, 18, vaultAccent, 3);
        if (dexFocused)
            fb.drawRoundedRect(dexX, HUB_Y, featureW, featureH, 18, dexAccent, 3);

        fb.drawRoundedRect(vaultX + 18, HUB_Y + 18, 38, 38, 10, vaultAccent, 2);
        fb.drawFilledRect(vaultX + 25, HUB_Y + 35, 24, 4, withAlpha(vaultAccent, 160));
        fb.drawFilledCircle(vaultX + 37, HUB_Y + 37, 7, vaultAccent);
        fb.drawText(vaultX + 68, HUB_Y + 20, "MASTER VAULT",
                    vaultFocused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Body);
        fb.drawText(vaultX + 18, HUB_Y + 70, "Pokémon storage",
                    Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(vaultX + 18, HUB_Y + 91, "Transfer & lineage",
                    Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(vaultX + 18, HUB_Y + 126, "Coming Soon",
                    Colors::TextMuted, TextStyle::Caption);

        fb.drawRoundedRect(dexX + 18, HUB_Y + 18, 40, 38, 9, dexAccent, 2);
        fb.drawFilledRoundedRect(dexX + 36, HUB_Y + 23, 4, 28, 2, dexAccent);
        fb.drawText(dexX + 70, HUB_Y + 20, "POKÉDEX",
                    dexFocused ? Colors::SelectedText : Colors::TextPrimary, TextStyle::Body);
        fb.drawText(dexX + 18, HUB_Y + 70, "Species & forms",
                    Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(dexX + 18, HUB_Y + 91, "Research & collection",
                    Colors::TextSecondary, TextStyle::Caption);
        fb.drawText(dexX + 18, HUB_Y + 126, "Coming Soon",
                    Colors::TextMuted, TextStyle::Caption);

        // Five primary destinations use the compact round logo treatment from the old bottom dock.
        // No individual rectangular cards: the selected-game hero remains the visual anchor.
        const int navY = HUB_Y + featureH + 18;
        static constexpr const char* dockLabels[5] =
            {"Games", "Banks", "Items", "Search", "More"};
        constexpr int buttonD = 74;
        constexpr int hitW = 126;
        constexpr int hitH = 112;
        const int topCellW = RIGHT_W / 3;
        const int bottomCellW = RIGHT_W / 2;
        const int topCy = navY + 54;
        const int bottomCy = navY + 182;

        for (int i = 0; i < 5; ++i) {
            const bool topRow = i < 3;
            const int slot = topRow ? i : i - 3;
            const int cellW = topRow ? topCellW : bottomCellW;
            const int cx = RIGHT_X + slot * cellW + cellW / 2;
            const int cy = topRow ? topCy : bottomCy;
            const bool focused = hubDockFocused && hubDockIndex == i;

            fb.drawFilledCircle(cx, cy, buttonD / 2, Colors::SurfaceRaised);
            fb.drawCircle(cx, cy, buttonD / 2,
                          focused ? Colors::FocusBorder : Colors::Divider,
                          focused ? 3 : 2);
            if (focused)
                fb.drawCircle(cx, cy, buttonD / 2 + 6, Colors::FocusBorder, 2);

            const int iconSize = 46;
            drawHubDockIcon(fb, i, cx - iconSize / 2, cy - iconSize / 2,
                            iconSize, focused);
            dockRects.push_back({cx - hitW / 2, cy - buttonD / 2 - 8,
                                 hitW, hitH, i});

            int lw = 0, lh = 0;
            fb.measureText(dockLabels[i], lw, lh, TextStyle::Caption);
            fb.drawText(cx - lw / 2, cy + buttonD / 2 + 10, dockLabels[i],
                        focused ? Colors::SelectedText
                                : i == 0 ? Colors::Info : Colors::TextSecondary,
                        TextStyle::Caption);
        }

        // The hero card already carries the big Launch button, so the global footer does not
        // repeat a ZR Launch hint.
        drawNavBar(fb, {{"L/R", "Change Game"}, {"A", "Open"}, {"Y", "Quick Games"},
                        {"+", "Current Game"}, {"-", "Help"}, {"B", "Exit"}});

        if (overlay == Overlay::GamesDrawer) {
            constexpr int w = 520;
            const int x = fb.getWidth() - w;
            const int h = fb.getHeight();
            constexpr int cols = 3;
            constexpr int visibleRows = 4;
            if (u && !u->titles.empty()) {
                gamesDrawerIndex = std::clamp(
                    gamesDrawerIndex, 0, static_cast<int>(u->titles.size()) - 1);
                const int totalRows =
                    (static_cast<int>(u->titles.size()) + cols - 1) / cols;
                gamesDrawerScroll = std::clamp(
                    gamesDrawerScroll, 0, std::max(0, totalRows - visibleRows));
                const int selectedRow = gamesDrawerIndex / cols;
                if (selectedRow < gamesDrawerScroll)
                    gamesDrawerScroll = selectedRow;
                else if (selectedRow >= gamesDrawerScroll + visibleRows)
                    gamesDrawerScroll = selectedRow - visibleRows + 1;
            } else {
                gamesDrawerIndex = 0;
                gamesDrawerScroll = 0;
            }
            constexpr int gap = 7;
            constexpr int margin = 10;
            constexpr int tileH = 124;
            const int tileW = (w - margin * 2 - gap * 2) / cols;

            // Full-height right-edge sheet with no top/right/bottom gutter.
            fb.drawFilledRect(0, 0, x, h, Color(0, 0, 0, 92));
            fb.drawFilledRect(x, 0, w, h, Colors::SurfaceRaised);
            fb.drawFilledRect(x, 0, 2, h, Colors::FocusBorder);

            fb.drawText(x + 22, 18, "QUICK GAMES",
                        Colors::Info, TextStyle::Caption);
            fb.drawText(x + 22, 44, "Choose a Pokémon Game",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 22, 70,
                        "Artwork + game + save  •  Games opens the full browser",
                        Colors::TextSecondary, TextStyle::Caption);

            if (u && !u->titles.empty()) {
                const int first = gamesDrawerScroll * cols;
                const int last = std::min<int>(
                    static_cast<int>(u->titles.size()), first + visibleRows * cols);
                const int gridY = 88;
                for (int i = first; i < last; ++i) {
                    const int local = i - first;
                    const int col = local % cols;
                    const int row = local / cols;
                    const int bx = x + margin + col * (tileW + gap);
                    const int by = gridY + row * (tileH + gap);
                    const bool selected = i == gamesDrawerIndex;
                    const auto& title = u->titles[static_cast<size_t>(i)];

                    drawFocusedCard(fb, bx, by, tileW, tileH, selected, 14);
                    const IconImage& art = SystemIcons::gameCardIcon(
                        title.sourceKind == SelectedSourceKind::RetroArchFRLG
                            ? title.artworkKey : title.gameId,
                        title.titleId);
                    constexpr int artSize = 78;
                    if (art.valid())
                        fb.drawImageScaled(bx + (tileW - artSize) / 2, by + 6,
                                           art.width, art.height, artSize, artSize, art.data, 4);
                    else {
                        fb.drawFilledRoundedRect(bx + (tileW - artSize) / 2, by + 6,
                                                 artSize, artSize, 10, Colors::PanelAlt);
                        fb.drawCircle(bx + tileW / 2, by + 45, 22,
                                      withAlpha(Colors::Info, 120), 3);
                    }

                    std::string label = title.label;
                    if (label.size() > 16) label = label.substr(0, 15) + "…";
                    int lw = 0, lh = 0;
                    fb.measureText(label, lw, lh, TextStyle::Caption);
                    fb.drawText(bx + std::max(6, (tileW - lw) / 2), by + 87, label,
                                selected ? Colors::SelectedText : Colors::TextPrimary,
                                TextStyle::Caption);

                    std::string saveName;
                    if (title.sourceKind == SelectedSourceKind::RetroArchFRLG &&
                        title.legacyInstances.size() == 1) {
                        saveName = sourceLeafName(title.legacyInstances.front().path());
                    } else if (!title.locationLabel.empty()) {
                        saveName = title.locationLabel;
                    } else if (title.sourceKind == SelectedSourceKind::SwitchTitle) {
                        saveName = "System save";
                    } else {
                        saveName = productSourceLabel(title.sourceLabel);
                    }
                    if (saveName.size() > 18) saveName = saveName.substr(0, 17) + "…";
                    int mw = 0, mh = 0;
                    fb.measureText(saveName, mw, mh, TextStyle::Caption);
                    fb.drawText(bx + std::max(6, (tileW - mw) / 2), by + 105, saveName,
                                Colors::TextMuted, TextStyle::Caption);
                }

                const int totalRows = (static_cast<int>(u->titles.size()) + cols - 1) / cols;
                if (totalRows > visibleRows)
                    drawScrollbar(fb, x + w - 8, gridY, visibleRows * (tileH + gap) - gap,
                                  totalRows * (tileH + gap) - gap,
                                  gamesDrawerScroll * (tileH + gap));
            } else {
                fb.drawText(x + 28, 150, "No Pokémon games found for this profile.",
                            Colors::TextMuted, TextStyle::Body);
            }

            fb.drawText(x + 22, h - 72,
                        "X: Save / Source   •   Y or B: Close",
                        Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose"}, {"A", "Select"},
                            {"X", "Save / Source"}, {"Y", "Close"}, {"B", "Close"}});
        } else if (overlay == Overlay::ProfilePicker) {
            constexpr int w = 780;
            constexpr int h = 560;
            constexpr int rowH = 88;
            constexpr int visibleRows = 4;
            const int x = (fb.getWidth() - w) / 2;
            const int y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);

            fb.drawText(x + 30, y + 20, "SWITCH PROFILE",
                        Colors::Info, TextStyle::Caption);
            fb.drawText(x + 30, y + 48, "Choose Profile",
                        Colors::TextPrimary, TextStyle::Title);
            fb.drawText(x + 30, y + 86,
                        "Select the Switch user whose Pokémon saves PokeBank NX should show.",
                        Colors::TextSecondary, TextStyle::Caption);
            fb.drawFilledRect(x + 28, y + 118, w - 56, 1, Colors::Divider);

            const int count = static_cast<int>(users.size());
            const int first = std::clamp(profilePickerIndex - 1, 0, std::max(0, count - visibleRows));
            const int last = std::min(count, first + visibleRows);
            int rowY = y + 136;
            for (int i = first; i < last; ++i) {
                const auto& user = users[static_cast<size_t>(i)];
                const bool selected = i == profilePickerIndex;
                const bool current = i == userIndex;
                drawFocusedCard(fb, x + 24, rowY, w - 48, rowH - 8, selected, 14);

                const IconImage* avatar =
                    user.name == "Pokémon Saves" ? nullptr : &SystemIcons::userIcon(user.uid);
                if (avatar && avatar->valid())
                    fb.drawImageScaled(x + 42, rowY + 9, avatar->width, avatar->height,
                                       62, 62, avatar->data, 4);
                else {
                    fb.drawFilledCircle(x + 73, rowY + 40, 30, Colors::PanelAlt);
                    fb.drawCircle(x + 73, rowY + 40, 30, Colors::Divider, 2);
                }

                fb.drawText(x + 126, rowY + 16, user.name,
                            selected ? Colors::SelectedText : Colors::TextPrimary,
                            TextStyle::Heading);
                fb.drawText(x + 126, rowY + 48,
                            std::to_string(user.titles.size()) + " Pokémon games",
                            Colors::TextMuted, TextStyle::Caption);

                if (current) {
                    constexpr int pillW = 92;
                    fb.drawPill(x + w - 142, rowY + 24, pillW, 30,
                                withAlpha(Colors::Info, 38));
                    fb.drawPillBorder(x + w - 142, rowY + 24, pillW, 30,
                                      Colors::Info, 1);
                    fb.drawText(x + w - 124, rowY + 32, "CURRENT",
                                Colors::Info, TextStyle::Caption);
                }
                rowY += rowH;
            }

            fb.drawText(x + 30, y + h - 42,
                        "Changing profile changes only which saves are shown; it never modifies save data.",
                        Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose Profile"},
                            {"A", "Use Profile"}, {"B", "Cancel"}});
        } else if (overlay == Overlay::GameWorkspace && u && titleIndex >= 0 &&
            titleIndex < static_cast<int>(u->titles.size())) {
            const auto& title = u->titles[static_cast<size_t>(titleIndex)];
            constexpr int x = 86, y = 82, w = 1108, h = 536;
            constexpr int art = 210;
            constexpr int gridX = x + 322;
            constexpr int gridY = y + 116;
            constexpr int cardW = 352;
            constexpr int cardH = 82;
            constexpr int gapX = 18;
            constexpr int gapY = 16;
            static constexpr const char* labels[8] = {
                "Overview", "Party", "Boxes", "Pokédex",
                "Trainer", "Editor / Create", "Backups", "Source / Game File"
            };
            static constexpr const char* subtitles[8] = {
                "Open the existing game workspace",
                "Open the real parsed party view",
                "Open the real storage / boxes view",
                "Game research and collection",
                "Open trainer information",
                "Use the existing safe staged editor",
                "Backup / staged history where supported",
                "Save instances, source and launch link"
            };

            fb.drawFilledRect(0, 0, fb.getWidth(), fb.getHeight() - kNavBarH,
                              Color(0, 0, 0, 118));
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18, "GAME WORKSPACE",
                        Colors::Info, TextStyle::Caption);
            fb.drawText(x + 28, y + 45,
                        title.name.empty() ? title.label : title.name,
                        Colors::TextPrimary, TextStyle::Title);

            const IconImage& artImage = SystemIcons::gameCardIcon(
                title.sourceKind == SelectedSourceKind::RetroArchFRLG
                    ? title.artworkKey : title.gameId,
                title.titleId);
            if (artImage.valid())
                fb.drawImageScaled(x + 30, y + 102, artImage.width, artImage.height,
                                   art, art, artImage.data, 4);
            else {
                fb.drawFilledRoundedRect(x + 30, y + 102, art, art, 18, Colors::PanelAlt);
                fb.drawCircle(x + 30 + art / 2, y + 102 + art / 2, 54,
                              withAlpha(Colors::Info, 120), 8);
                fb.drawFilledCircle(x + 30 + art / 2, y + 102 + art / 2, 15, Colors::Info);
            }
            fb.drawRoundedRect(x + 30, y + 102, art, art, 18, Colors::Divider, 1);
            const auto workspacePortrait = trainerPortraitForGame(
                title.gameId, previewTrainerGenderKnown, previewTrainerGender);
            drawTrainerPortrait(fb, x + 30 + art - 78, y + 102 + art - 90,
                                72, 84, workspacePortrait, false);

            const int metaY = y + 332;
            fb.drawText(x + 30, metaY, "Trainer", Colors::TextMuted, TextStyle::Caption);
            fb.drawText(x + 122, metaY,
                        previewTrainerName.empty() ? "—" : previewTrainerName,
                        previewTrainerName.empty() ? Colors::TextMuted : Colors::TextPrimary,
                        TextStyle::Body);
            fb.drawText(x + 30, metaY + 34, "Source", Colors::TextMuted, TextStyle::Caption);
            fb.drawText(x + 122, metaY + 34,
                        title.sourceLabel.empty() ? "Validated source" : productSourceLabel(title.sourceLabel),
                        Colors::TextSecondary, TextStyle::Body);
            fb.drawText(x + 30, metaY + 68, "Save", Colors::TextMuted, TextStyle::Caption);
            fb.drawText(x + 122, metaY + 68,
                        title.locationLabel.empty() ? "Ready" : title.locationLabel,
                        Colors::TextSecondary, TextStyle::Body);
            fb.drawText(x + 30, metaY + 102, "Profile", Colors::TextMuted, TextStyle::Caption);
            fb.drawText(x + 122, metaY + 102, u->name,
                        Colors::TextSecondary, TextStyle::Body);

            for (int i = 0; i < 8; ++i) {
                const int col = i % 2;
                const int row = i / 2;
                const int cx = gridX + col * (cardW + gapX);
                const int cy = gridY + row * (cardH + gapY);
                const bool focused = gameWorkspaceIndex == i;
                const bool unavailableBackup =
                    i == 6 && title.sourceKind != SelectedSourceKind::SwitchTitle;
                drawFocusedCard(fb, cx, cy, cardW, cardH, focused, 14);
                fb.drawText(cx + 20, cy + 14, labels[i],
                            focused ? Colors::SelectedText
                                    : unavailableBackup ? Colors::TextMuted
                                                        : Colors::TextPrimary,
                            TextStyle::Heading);
                fb.drawText(cx + 20, cy + 47,
                            unavailableBackup ? "READ-ONLY SOURCE" : subtitles[i],
                            unavailableBackup ? Colors::TextMuted : Colors::TextSecondary,
                            TextStyle::Caption);
                if (i == 3)
                    fb.drawText(cx + cardW - 104, cy + 16, "COMING SOON",
                                Colors::TextMuted, TextStyle::Caption);
            }

            if (!hubNotice.empty()) {
                std::string notice = hubNotice;
                if (notice.size() > 104) notice = notice.substr(0, 103) + "…";
                fb.drawText(gridX, y + h - 42, notice, Colors::Info, TextStyle::Caption);
            }
            drawNavBar(fb, {{"D-pad/Stick", "Navigate"}, {"A", "Open"},
                            {"ZR", "Launch"}, {"+", "Close Menu"}, {"B", "Home"}});
        } else if (overlay == Overlay::GameFilePicker) {
            constexpr int w = 900, h = 560, rowH = 54, visibleRows = 7;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18, "GAME LAUNCH / LINK GAME FILE",
                        Colors::AccentPrimary, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Choose the ROM / game file",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 78,
                        "Read-only browser — linking stores only PokeBank-owned launch metadata.",
                        Colors::TextSecondary, TextStyle::Caption);
            fb.drawText(x + 28, y + 102,
                        launchBrowsePath.substr(0, 100), Colors::TextMuted, TextStyle::Caption);

            const int first = launchFileScroll;
            const int last = std::min<int>(
                static_cast<int>(launchFileEntries.size()), first + visibleRows);
            int rowY = y + 132;
            for (int i = first; i < last; ++i) {
                const auto& entry = launchFileEntries[static_cast<size_t>(i)];
                const bool selected = i == launchFileIndex;
                drawFocusedCard(fb, x + 24, rowY, w - 48, rowH - 6, selected, 10);
                fb.drawText(x + 44, rowY + 13,
                            entry.directory ? "[Folder]  " + entry.name : entry.name,
                            selected ? Colors::SelectedText : Colors::TextSecondary,
                            TextStyle::Body);
                rowY += rowH;
            }
            if (!launchFileNotice.empty())
                fb.drawText(x + 28, y + h - 42, launchFileNotice.substr(0, 108),
                            Colors::Info, TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose"}, {"A", "Open / Link"},
                            {"Y", "Up Folder"}, {"X", "Start Folder"}, {"B", "Cancel"}});
        } else if (overlay == Overlay::LegacyInstances && u && titleIndex >= 0 &&
            titleIndex < static_cast<int>(u->titles.size())) {
            const auto& parent = u->titles[titleIndex];
            constexpr int w = 780, h = 530, rowH = 66, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18,
                        "SAVE INSTANCES / " + parent.platformLabel + " / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + parent.label + " — Save Instances",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Choose a validated battery save. The source file will not be modified.",
                        Colors::TextSecondary, TextStyle::Caption);
            fb.drawText(x + 28, y + 96, providerSummary(parent.legacyInstances),
                        Colors::TextMuted, TextStyle::Caption);

            const int first = legacyInstanceScroll;
            drawSaveInstanceRows(fb, parent.legacyInstances, legacyInstanceIndex, first,
                                 x, y + 122, w, rowH, visibleRows, true);
            if (!legacyNotice.empty())
                fb.drawText(x + 28, y + h - 38, legacyNotice, Colors::TextMuted,
                            TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose Save"},
                            {"A", launchLegacyMode ? "Launch / Link" : "Open Read Only"},
                            {"Y", "Source Details"}, {"X", "Refresh Saves"}, {"B", "Back"}});
        } else if (overlay == Overlay::LegacyAssignment && u) {
            constexpr int w = 800, h = 530, rowH = 70, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 28, y + 18, "SAVE SOURCE / EXPLICIT PROFILE ASSIGNMENT",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Assign a Legacy Save to " + u->name,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Assigning claims this save for this profile and hides it from other profiles.",
                        Colors::TextSecondary, TextStyle::Caption);
            const int first = legacyAssignmentScroll;
            const int last = std::min<int>(static_cast<int>(unassignedLegacySources.size()),
                                           first + visibleRows);
            int rowY = y + 108;
            for (int index = first; index < last; ++index) {
                const auto& entry = unassignedLegacySources[static_cast<size_t>(index)];
                drawFocusedCard(fb, x + 24, rowY, w - 48, rowH - 6,
                                index == legacyAssignmentIndex, 10);
                fb.drawText(x + 44, rowY + 7, "Pokémon " + entry.title + " — " +
                            entry.instance.label,
                            index == legacyAssignmentIndex ? Colors::TextPrimary
                                                           : Colors::TextSecondary,
                            TextStyle::Body);
                fb.drawText(x + 44, rowY + 35,
                            (entry.instance.providerLabel.empty() ? std::string("Source")
                                                                 : entry.instance.providerLabel) +
                                " / " + entry.instance.sourceLabel,
                            Colors::TextMuted, TextStyle::Caption);
                rowY += rowH;
            }
            if (!legacyNotice.empty())
                fb.drawText(x + 28, y + h - 36, legacyNotice, Colors::TextMuted,
                            TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose"}, {"A", "Assign to This Profile"},
                            {"X", "Refresh"}, {"B", "Cancel"}});
        } else if (overlay == Overlay::LegacyDetails) {
            constexpr int w = 900, h = 520;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const std::string detailsProvider = legacyDetailsInstance.providerLabel.empty()
                ? std::string("SOURCE") : legacyDetailsInstance.providerLabel;
            fb.drawText(x + 28, y + 18,
                        detailsProvider + " / SOURCE DIAGNOSTICS / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, legacyDetailsInstance.label,
                        Colors::TextPrimary, TextStyle::Heading);
            int lineY = y + 88;
            auto drawLine = [&](const std::string& label, const std::string& value) {
                fb.drawText(x + 32, lineY, label, Colors::TextMuted, TextStyle::Caption);
                fb.drawText(x + 214, lineY, value, Colors::TextPrimary, TextStyle::Caption);
                lineY += 30;
            };
            drawLine("Provider", legacyDetailsInstance.providerLabel.empty()
                ? "Source" : legacyDetailsInstance.providerLabel);
            drawLine("Game identity", legacyDetailsGameId);
            drawLine("Trainer", legacyDetailsInstance.trainerName.empty()
                ? "Unknown" : legacyDetailsInstance.trainerName);
            drawLine("Party count", std::to_string(legacyDetailsInstance.partyCount));
            drawLine("File size", std::to_string(legacyDetailsInstance.fileSize) + " bytes");
            drawLine("Modified state", std::to_string(legacyDetailsInstance.modifiedTime));
            drawLine("Source identity", shortValue(legacyDetailsInstance.sourceIdentity, 24));
            drawLine("Content SHA-256", shortValue(legacyDetailsInstance.contentFingerprint, 24));
            const std::string& path = legacyDetailsInstance.normalizedPath;
            drawLine("Physical path", path.substr(0, std::min<size_t>(70, path.size())));
            for (size_t offset = 70; offset < path.size() && offset < 210; offset += 70)
                drawLine("", path.substr(offset, 70));
            drawNavBar(fb, {{"B", "Back to Save List"}});
        } else if (overlay == Overlay::Gen4Setup && u) {
            constexpr int w = 760, h = 430, rowH = 64;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const auto* identity = PokeVault::Games::findGame(gen4TargetGameId);
            const std::string title = identity ? std::string(identity->title) : std::string("Generation IV");
            fb.drawText(x + 28, y + 18, "NINTENDO DS / SOURCE SETUP / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + title,
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Add or repair sources here. Opening always happens from Save Instances.",
                        Colors::TextSecondary, TextStyle::Caption);
            const std::string rows[4] = {
                "Refresh Known Emulator Saves",
                "Choose Save File Manually",
                "Forget Remembered Save",
                "Cancel"
            };
            int ry = y + 112;
            for (int i = 0; i < 4; ++i) {
                drawFocusedCard(fb, x + 24, ry, w - 48, rowH - 8, i == gen4SetupIndex, 10);
                fb.drawText(x + 44, ry + 15, rows[i],
                            i == gen4SetupIndex ? Colors::TextPrimary : Colors::TextSecondary);
                ry += rowH;
            }
            if (!gen4Notice.empty())
                fb.drawText(x + 28, y + h - 42, gen4Notice.substr(0, 100),
                            Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose"}, {"A", "Select"}, {"B", "Back"}});
        } else if (overlay == Overlay::Gen4Candidates && u) {
            constexpr int w = 900, h = 540, rowH = 76, visibleRows = 5;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            const auto* identity = PokeVault::Games::findGame(gen4TargetGameId);
            const std::string title = identity ? std::string(identity->title) : std::string("Generation IV");
            fb.drawText(x + 28, y + 18, "NINTENDO DS / SAVE INSTANCES / READ ONLY",
                        Colors::Accent, TextStyle::Caption);
            fb.drawText(x + 28, y + 44, "Pokémon " + title + " — Save Instances",
                        Colors::TextPrimary, TextStyle::Heading);
            fb.drawText(x + 28, y + 76,
                        "Choose a validated cartridge save. The source file will not be modified.",
                        Colors::TextSecondary, TextStyle::Caption);

            const int first = gen4CandidateScroll;
            drawSaveInstanceRows(fb, gen4Instances, gen4CandidateIndex, first,
                                 x, y + 108, w, rowH, visibleRows, false);
            if (!gen4Notice.empty())
                fb.drawText(x + 28, y + h - 34, gen4Notice, Colors::TextMuted, TextStyle::Caption);
            drawNavBar(fb, {{"D-pad/Stick", "Choose Save"}, {"A", "Open Read Only"},
                            {"Y", "Source Setup"}, {"X", "Refresh Saves"}, {"B", "Back"}});
        } else if (overlay == Overlay::Help) {
            if (helpReturnOverlay == Overlay::GameWorkspace) {
                drawProductHelpOverlay(fb, "Current Game Controls", {
                    {"D-pad/Stick", "Navigate Overview / Party / Boxes / Trainer / Backups"},
                    {"A", "Open the focused game tool"},
                    {"ZR", "Launch the selected game"},
                    {"+", "Close the Current Game menu"},
                    {"-", "Close Help / Controls"},
                    {"B", "Back to Product Home"}
                }, "Backup history appears only when you explicitly choose Backups.");
            } else {
                drawProductHelpOverlay(fb, "PokeBank NX Controls", {
                    {"D-pad/Stick", "Navigate Product Home"},
                    {"A", "Open the selected game or focused control"},
                    {"L/R", "Previous / next game"},
                    {"Y", "Open the quick right-side artwork browser"},
                    {"+", "Current Game tools: Overview / Party / Boxes / Trainer / Backups"},
                    {"-", "Help / Controls"},
                    {"B", "Exit PokeBank NX from Product Home"}
                }, "Games opens the full artwork browser. Profile and Settings use the top-right controls.");
            }
        } else if (overlay == Overlay::Options) {
            constexpr int w = 560, h = 326, rowH = 64;
            const int x = (fb.getWidth() - w) / 2, y = (fb.getHeight() - h) / 2;
            drawModalSurface(fb, x, y, w, h);
            fb.drawText(x + 26, y + 16, "POKEBANK NX  /  OPTIONS", Colors::AccentPrimary,
                        TextStyle::Caption);
            fb.drawText(x + 26, y + 40, "Quick Options", Colors::TextPrimary, TextStyle::Heading);
            const std::string rows[3] = {
                "Theme: " + std::string(themeModeName(g_themeMode)),
                "Exit PokeBank NX",
                "Cancel"
            };
            int ry = y + 92;
            for (int i = 0; i < 3; ++i) {
                drawFocusedCard(fb, x + 22, ry, w - 44, rowH - 8, i == optionsIndex, 12);
                fb.drawText(x + 44, ry + 15, rows[i],
                            i == optionsIndex ? Colors::TextPrimary : Colors::TextSecondary);
                ry += rowH;
            }
            drawNavBar(fb, {{"D-pad/Stick", "Choose"}, {"A", "Select"}, {"B", "Cancel"}});
        }
    }
}
