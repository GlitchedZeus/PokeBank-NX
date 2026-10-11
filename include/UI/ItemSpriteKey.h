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
    // Historically named items have no dedicated sprite in the pinned PNG
    // archive. Use a clearly classified modern REPRESENTATIVE icon for the
    // same item role; never misreport these as exact original-era artwork.
    if (key == "x-special") return "x-sp-atk";
    if (key == "exp-all") return "exp-share";
    // Gen II's ten original berry identities lack distinct sprite art in
    // the pinned archive. These later-generation successor berries have the
    // documented corresponding effects, and are labelled as REPRESENTATIVE
    // in the audit (not original GSC item graphics). Important: Ice Berry
    // cures BURN while Burnt Berry cures FREEZE, despite their names.
    if (key == "poison-cure-berry") return "pecha-berry";
    if (key == "paralyze-cure-berry") return "cheri-berry";
    if (key == "mint-berry") return "chesto-berry";
    if (key == "ice-berry") return "rawst-berry";
    if (key == "burnt-berry") return "aspear-berry";
    if (key == "mystery-berry") return "leppa-berry";
    if (key == "berry") return "oran-berry";
    if (key == "bitter-berry") return "persim-berry";
    if (key == "miracle-berry") return "lum-berry";
    if (key == "gold-berry") return "sitrus-berry";
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
    // Gen IV ItemNames display synonyms for these EXACT native item IDs,
    // cross-checked against the Diamond/Pearl/HGSS index table. Sprite
    // remapping only; do not reinterpret a different generation's raw ID.
    if (key == "guidebook") return "journal";          // Gen IV ID 433
    if (key == "sticker-case") return "seal-case";      // ID 434
    if (key == "sticker-bag") return "seal-bag";        // ID 436
    if (key == "bike") return "bicycle";                // ID 450
    if (key == "lunar-feather") return "lunar-wing";    // ID 453
    if (key == "secret-medicine") return "secret-potion"; // ID 464
    if (key == "silver-feather") return "silver-wing"; // ID 482
    if (key == "rainbow-feather") return "rainbow-wing"; // ID 483
    if (key == "itemfinder") return "itemfinder"; // verified extra asset

    // Gen II's historical mail labels sometimes differ from the source
    // screenshot's exact filename. Preserve each DISTINCT stationery key.
    if (key == "blue-sky-mail") return "bluesky-mail";
    if (key == "lite-blue-mail" || key == "litebluemail") return "liteblue-mail";
    if (key == "portrait-mail") return "portraitmail";
    return key;
}

inline bool isGen2Mail(const std::string& key) {
    return key == "bluesky-mail" || key == "eon-mail" ||
           key == "flower-mail" || key == "liteblue-mail" ||
           key == "lovely-mail" || key == "mirage-mail" ||
           key == "morph-mail" || key == "music-mail" ||
           key == "portraitmail" || key == "surf-mail";
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
