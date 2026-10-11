#ifndef POKEBANK_UI_ITEM_SPRITE_KEY_H
#define POKEBANK_UI_ITEM_SPRITE_KEY_H

#include <cctype>
#include <string>
#include <string_view>

namespace UI::ItemSpriteKey {

// The raw item IDs are generation-dependent.  Resolve *display names* to
// verified native ROMFS artwork; never reinterpret a Gen I/II ID as Gen IV.
inline std::string slug(std::string_view label) {
    std::string key;
    key.reserve(label.size());
    for (size_t i = 0; i < label.size(); ++i) {
        const unsigned char ch = static_cast<unsigned char>(label[i]);
        if (ch == 0xC3 && i + 1 < label.size() &&
            static_cast<unsigned char>(label[i + 1]) == 0xA9) {
            key.push_back('e'); ++i; continue; // Pokémon -> pokemon
        }
        if (ch == ' ' && i + 1 < label.size() && label[i + 1] == '(') break;
        if (ch >= 'A' && ch <= 'Z') key.push_back(static_cast<char>(ch - 'A' + 'a'));
        else if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))
            key.push_back(static_cast<char>(ch));
        else if (ch == ' ' || ch == '-' || ch == '_') {
            if (!key.empty() && key.back() != '-') key.push_back('-');
        }
    }
    while (!key.empty() && key.back() == '-') key.pop_back();
    // TM27 — Return and HM01 — Cut are display labels, not filenames.
    // Never try to load tm27-return.png. Keep the original numbered ID.
    if (key.size() > 3 &&
        ((key[0] == 't' && key[1] == 'm') || (key[0] == 'h' && key[1] == 'm')) &&
        std::isdigit(static_cast<unsigned char>(key[2]))) {
        size_t cursor = 2;
        while (cursor < key.size() &&
               std::isdigit(static_cast<unsigned char>(key[cursor]))) ++cursor;
        if (cursor < key.size() && key[cursor] == '-') key.resize(cursor);
    }
    if (key == "none" || key == "no-item" || key == "???" ||
        key == "teru-sama") return {};

    // Exact historical aliases for the SAME native object; these are sprite
    // lookup fixes only.  The inventory label and native IDs are untouched.
    if (key == "thunderstone") return "thunder-stone";
    if (key == "x-defend") return "x-defense";
    if (key == "parlyz-heal") return "paralyze-heal";
    if (key == "brightpowder") return "bright-powder";
    if (key == "elixer") return "elixir";
    if (key == "max-elixer") return "max-elixir";
    if (key == "energypowder") return "energy-powder";
    if (key == "tinymushroom") return "tiny-mushroom";
    if (key == "silverpowder") return "silver-powder";
    if (key == "deepseatooth") return "deep-sea-tooth";
    if (key == "deepseascale") return "deep-sea-scale";
    if (key == "blackglasses") return "black-glasses";
    if (key == "nevermeltice") return "never-melt-ice";
    if (key == "twistedspoon") return "twisted-spoon";
    if (key == "upgrade") return "up-grade";
    if (key == "leek") return "stick"; // renamed Gen IV held item
    if (key == "slowpoketail") return "slowpoke-tail";
    if (key == "ragecandybar") return "rage-candy-bar";
    if (key == "ss-ticket" || key == "ssticket") return "ss-ticket";
    if (key == "doll") return "poke-doll"; // Gen II retail "# DOLL"
    if (key == "itemfinder") return "itemfinder"; // verified extra asset
    return key;
}

inline bool isNumberedMachine(const std::string& key) {
    return key.size() >= 3 &&
        ((key[0] == 't' && key[1] == 'm') ||
         (key[0] == 'h' && key[1] == 'm')) &&
        std::isdigit(static_cast<unsigned char>(key[2])) != 0;
}

inline bool isTM(const std::string& key) {
    return isNumberedMachine(key) && key[0] == 't';
}

inline bool isHM(const std::string& key) {
    return isNumberedMachine(key) && key[0] == 'h';
}

} // namespace UI::ItemSpriteKey
#endif
