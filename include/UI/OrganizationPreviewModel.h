#ifndef UI_ORGANIZATION_PREVIEW_MODEL_H
#define UI_ORGANIZATION_PREVIEW_MODEL_H

#include <array>
#include <cstddef>
#include <string_view>

namespace PokeBank::UIModel {

    enum class OrganizationPreviewKind {
        Banks,
        Search,
        Collections,
    };

    struct PreviewBox {
        std::string_view title;
        int count;
        int capacity;
    };

    struct SearchFilterPreview {
        std::string_view label;
        std::string_view value;
    };

    struct CollectionPreview {
        std::string_view title;
        std::string_view subtitle;
    };

    inline constexpr std::array<PreviewBox, 6> BANK_BOX_PREVIEW{{
        {"Box 1", 0, 30}, {"Box 2", 0, 30}, {"Box 3", 0, 30},
        {"Box 4", 0, 30}, {"Box 5", 0, 30}, {"Box 6", 0, 30},
    }};

    inline constexpr std::array<SearchFilterPreview, 7> SEARCH_FILTER_PREVIEW{{
        {"Species", "Any"},
        {"Generation", "Any"},
        {"Game / Source", "Any"},
        {"Shiny", "Any"},
        {"Box", "Any"},
        {"Favorites", "Any"},
        {"Recent", "Any"},
    }};

    inline constexpr std::array<CollectionPreview, 4> COLLECTION_PREVIEW{{
        {"Living Dex", "Species completion view"},
        {"Shiny Living Dex", "Shiny species completion view"},
        {"Favorites", "Pinned Pokémon across Banks"},
        {"Recent", "Recently viewed / added Pokémon"},
    }};

    constexpr int previewWrapIndex(int current, int delta, int count) {
        if (count <= 0) return 0;
        if (current < 0 || current >= count) current = 0;
        const int normalized = delta % count;
        return (current + normalized + count) % count;
    }

    constexpr int previewColumns(OrganizationPreviewKind kind) {
        switch (kind) {
            case OrganizationPreviewKind::Banks:       return 3;
            case OrganizationPreviewKind::Search:      return 2;
            case OrganizationPreviewKind::Collections: return 2;
        }
        return 1;
    }

    constexpr int previewCount(OrganizationPreviewKind kind) {
        switch (kind) {
            case OrganizationPreviewKind::Banks:
                return static_cast<int>(BANK_BOX_PREVIEW.size());
            case OrganizationPreviewKind::Search:
                return static_cast<int>(SEARCH_FILTER_PREVIEW.size());
            case OrganizationPreviewKind::Collections:
                return static_cast<int>(COLLECTION_PREVIEW.size());
        }
        return 0;
    }

    constexpr int previewMoveSelection(OrganizationPreviewKind kind, int current, int dx, int dy) {
        const int count = previewCount(kind);
        if (count <= 0) return 0;
        if (current < 0 || current >= count) current = 0;
        const int cols = previewColumns(kind);
        if (dx != 0) return previewWrapIndex(current, dx, count);

        if (dy != 0) {
            const int col = current % cols;
            const int row = current / cols;
            const int rowsInColumn = ((count - 1 - col) / cols) + 1;
            const int normalized = dy % rowsInColumn;
            const int nextRow = (row + normalized + rowsInColumn) % rowsInColumn;
            return nextRow * cols + col;
        }
        return current;
    }

} // namespace PokeBank::UIModel

#endif
