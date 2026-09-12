#include <algorithm>
#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "UI/Panels/ItemsPanel.h"
#include "UI/TrainerViewScreen.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "Trainer/Trainer.h"
#include "Trainer/Inventory9LZA.h"
#include "Trainer/Inventory9SV.h"
#include "Trainer/Inventory8LA.h"
#include "Trainer/Inventory8BDSP.h"
#include "Trainer/Inventory8SWSH.h"
#include "Trainer/Inventory7LGPE.h"
#include "Trainer/Inventory3FRLG.h"
#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Enums/GameVersion.h"
#include "Utils/HelperUtilities.h"
#include "Names/MoveNames.h"
#include "Names/TMMoves.h"
#include "Names/ItemNames.h"

using namespace Trainer;
using namespace Enums;
using namespace Utils;

namespace UI {
namespace Panels {
    namespace {
        bool isRBYSource(std::string_view id) noexcept {
            return id == "red_gb" || id == "blue_gb" || id == "yellow_gb";
        }

        bool isGSCSource(std::string_view id) noexcept {
            return id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc";
        }

        bool isRSESource(std::string_view id) noexcept {
            return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba";
        }

        const char* gscPouchDisplayName(int category) noexcept {
            constexpr std::array<const char*, 5> names = {
                "TM/HM", "Items", "Key Items", "Balls", "PC Items",
            };
            return category >= 0 && category < static_cast<int>(names.size())
                ? names[static_cast<size_t>(category)] : "?";
        }

        const char* rsePouchDisplayName(int category) noexcept {
            constexpr std::array<const char*, 6> names = {
                "Items", "Key Items", "Poké Balls", "TM/HM", "Berries", "PC Items",
            };
            return category >= 0 && category < static_cast<int>(names.size())
                ? names[static_cast<size_t>(category)] : "?";
        }
    }

    const char* pouchDisplayName(GameVersion gameGroup, int category) {
        const char* name = nullptr;
        if (gameGroup == GameVersion::ZA)        name = getPouchInfo9LZA(static_cast<PouchType9LZA>(category)).name;
        else if (gameGroup == GameVersion::SV)   name = getPouchInfo9SV(static_cast<PouchType9SV>(category)).name;
        else if (gameGroup == GameVersion::PLA)  name = getPouchInfo8LA(static_cast<PouchType8LA>(category)).name;
        else if (gameGroup == GameVersion::BDSP) name = getPouchInfo8BDSP(static_cast<PouchType8BDSP>(category)).name;
        else if (gameGroup == GameVersion::GG)   name = getPouchInfo7LGPE(static_cast<PouchType7LGPE>(category)).name;
        else if (gameGroup == GameVersion::GSC)  name = gscPouchDisplayName(category);
        else if (gameGroup == GameVersion::RBY)  name = PokeVault::Integration::Gen1::inventoryCategoryName(static_cast<size_t>(category));
        else if (gameGroup == GameVersion::FRLG) name = getPouchInfo3FRLG(static_cast<PouchType3FRLG>(category)).name;
        else                                     name = getPouchInfo8SWSH(static_cast<PouchType8SWSH>(category)).name;
        return (name != nullptr && name[0] != '\0') ? name : "?";
    }

    void drawItems(TrainerViewScreen& screen, PKSEFramebuffer& fb, int x, int y, int width, int height) {
        fb.drawFilledRoundedRect(x, y, width, height, 16, Colors::Panel);
        fb.drawRoundedRect(x, y, width, height, 16, Colors::Border, 1);

        constexpr int hH = 46;
        fb.drawFilledRoundedRect(x, y, width, hH, 16, Colors::AccentDim);
        fb.drawFilledRect(x, y + hH - 16, width, 16, Colors::AccentDim);
        GameVersion gameGroup = screen.trainer.getGameGroup();
        const bool rbySource = screen.legacyReadOnlySource() && isRBYSource(screen.sourceGameId);
        const bool gscSource = screen.legacyReadOnlySource() && isGSCSource(screen.sourceGameId);
        const bool rseSource = screen.legacyReadOnlySource() && isRSESource(screen.sourceGameId);
        const char* pouchName = rbySource
            ? PokeVault::Integration::Gen1::inventoryCategoryName(static_cast<size_t>(screen.selectedCategory))
            : gscSource ? gscPouchDisplayName(screen.selectedCategory)
            : rseSource ? rsePouchDisplayName(screen.selectedCategory)
                        : pouchDisplayName(gameGroup, screen.selectedCategory);
        fb.drawText(x + 22, y + (hH - fb.lineHeight(TextStyle::Heading)) / 2,
                    std::string("Items - ") + pouchName, Colors::Text, TextStyle::Heading);

        screen.touchButtons.clear();

        // The strict legacy inventory decoders are optional submodels. A malformed inventory must
        // never turn an otherwise valid read-only save into "Invalid category" or hide its Pokemon.
        if (rbySource && screen.trainer.items.empty()) {
            fb.drawText(x + 24, y + hH + 30, "Inventory unavailable", Colors::Text, TextStyle::Body);
            fb.drawText(x + 24, y + hH + 58, "Generation I inventory validation failed", Colors::TextDim, TextStyle::Caption);
            fb.drawText(x + 24, y + hH + 82, "Trainer, party and boxes remain read-only and available.", Colors::TextDim, TextStyle::Caption);
            return;
        }
        if (gscSource && screen.trainer.items.empty()) {
            fb.drawText(x + 24, y + hH + 30, "Inventory unavailable", Colors::Text, TextStyle::Body);
            fb.drawText(x + 24, y + hH + 58, "Generation II inventory validation failed", Colors::TextDim, TextStyle::Caption);
            fb.drawText(x + 24, y + hH + 82, "Trainer, party, boxes and Pokémon details remain available.", Colors::TextDim, TextStyle::Caption);
            return;
        }
        if (rseSource && screen.trainer.items.empty()) {
            fb.drawText(x + 24, y + hH + 30, "Inventory unavailable", Colors::Text, TextStyle::Body);
            fb.drawText(x + 24, y + hH + 58, "RSE inventory validation failed", Colors::TextDim, TextStyle::Caption);
            fb.drawText(x + 24, y + hH + 82, "Trainer, party and boxes remain read-only and available.", Colors::TextDim, TextStyle::Caption);
            return;
        }

        if (screen.selectedCategory < 0 || screen.selectedCategory >= static_cast<int>(screen.trainer.items.size())) {
            fb.drawText(x + 24, y + hH + 30, "Invalid category", Colors::TextDim);
            return;
        }
        const auto& pouch = screen.trainer.items[screen.selectedCategory];
        std::vector<int> visible = screen.visibleItemIndices();
        if (visible.empty()) {
            fb.drawText(x + 24, y + hH + 30, "No items in this category", Colors::TextDim);
            return;
        }
        const int total = static_cast<int>(visible.size());

        constexpr int rowPitch = 52, tileH = 46;
        const int itemsPerPage = (height - 106) / rowPitch;
        const int totalPages = (total + itemsPerPage - 1) / itemsPerPage;
        const int tileW = std::min(width - 60, 860);
        const int tileX = x + (width - tileW) / 2;

        std::string countText = std::to_string(total) + " items";
        if (totalPages > 1)
            countText += "      Page " + std::to_string(screen.currentPage + 1) + " / " + std::to_string(totalPages);
        fb.drawText(tileX, y + hH + 10, countText, Colors::TextDim, TextStyle::Caption);

        const int startIdx = screen.currentPage * itemsPerPage;
        const int endIdx = std::min(startIdx + itemsPerPage, total);
        int ry = y + hH + 40;
        for (int i = startIdx; i < endIdx; ++i) {
            const InventoryItem& item = pouch[visible[i]];
            const bool selected = screen.detailViewActive && i == screen.selectedItemIndex;
            fb.drawSoftShadow(tileX, ry, tileW, tileH, tileH / 2);
            fb.drawFilledRoundedRect(tileX, ry, tileW, tileH, 12,
                                     selected ? Colors::Primary : Colors::PanelAlt);
            const Color nameCol = selected ? Colors::PrimaryText
                                           : (item.isNew ? Colors::Accent : Colors::Text);
            int nx = tileX + 22;
            if (item.isFavorite) {
                fb.drawSymbol(nx, ry + (tileH - 20) / 2, "\xE2\x98\x85", Colors::Yellow);
                nx += 24;
            }

            std::string gen2Name;
            const char* itemName = nullptr;
            if (gameGroup == GameVersion::RBY) {
                itemName = Names::getItemNameG1(item.itemId);
            } else if (gameGroup == GameVersion::GSC) {
                gen2Name = std::string(PokeVault::Integration::Gen2::gen2ItemName(
                    static_cast<uint8_t>(item.itemId)));
                itemName = gen2Name.c_str();
            } else if (gameGroup == GameVersion::FRLG) {
                itemName = Names::getItemNameG3(item.itemId);
            } else {
                itemName = getItemName(item.itemId);
            }
            fb.drawText(nx, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2,
                        itemName, nameCol, TextStyle::Body);

            // Raw Gen I/II machine ids have their own namespaces; never reinterpret them through a
            // later-generation TM table. Their exact TM01..TM50/HM01..HM07 labels come from gen2ItemName.
            if (gameGroup != GameVersion::RBY && gameGroup != GameVersion::GSC) {
                if (uint16_t tmMove = Names::getTMMove(gameGroup, item.itemId)) {
                    int iw, ih; fb.measureText(itemName, iw, ih, TextStyle::Body);
                    const Color moveCol = selected ? Colors::PrimaryText : Colors::TextDim;
                    fb.drawText(nx + iw + 14, ry + (tileH - fb.lineHeight(TextStyle::Body)) / 2,
                                Names::getMoveName(tmMove), moveCol, TextStyle::Body);
                }
            }

            std::string cnt = "\xC3\x97" + std::to_string(item.count);
            int cw, ch; fb.measureText(cnt, cw, ch, TextStyle::Body);
            fb.drawText(tileX + tileW - 24 - cw, ry + (tileH - ch) / 2,
                        cnt, selected ? Colors::PrimaryText : Colors::TextDim, TextStyle::Body);
            screen.touchButtons.push_back({ i, tileX, ry, tileW, tileH });
            ry += rowPitch;
        }
    }
}
}
