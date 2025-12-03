#ifndef ENUMS_ENUMS_H
#define ENUMS_ENUMS_H

namespace Enums {
    /// Contiguous series Game Language IDs
    enum class LanguageID
    {
        /// Undefined Language ID, usually indicative of a value not being set.
        /// Gen5 Japanese In-game Trades happen to not have their Language value set, and express Language=0.
        Hacked,

        /// Japanese (日本語)
        Japanese,

        /// English (US/UK/AU)
        English,

        /// French (Français)
        French,

        /// Italian (Italiano)
        Italian,

        /// German (Deutsch)
        German,

        /// Unused Language ID
        /// Was reserved for Korean in Gen3 but never utilized.
        UNUSED_6,

        /// Spanish (Español)
        Spanish,

        /// Korean (한국어)
        Korean,

        /// Chinese Simplified (简体中文)
        ChineseSimplified,

        /// Chinese Traditional (繁體中文)
        ChineseTraditional
    };
}

#endif