#include <cassert>
#include <string_view>

#include "UI/OrganizationPreviewModel.h"

int main() {
    using namespace PokeBank::UIModel;

    static_assert(BANK_BOX_PREVIEW.size() == 6);
    static_assert(SEARCH_FILTER_PREVIEW.size() == 7);
    static_assert(COLLECTION_PREVIEW.size() == 4);

    for (const auto& box : BANK_BOX_PREVIEW) {
        assert(box.count == 0);
        assert(box.capacity == 30);
    }

    assert(previewWrapIndex(0, 1, 6) == 1);
    assert(previewWrapIndex(0, -1, 6) == 5);
    assert(previewWrapIndex(5, 1, 6) == 0);
    assert(previewWrapIndex(-1, 0, 6) == 0);
    assert(previewWrapIndex(99, 0, 6) == 0);
    assert(previewWrapIndex(3, 2, 0) == 0);

    assert(SEARCH_FILTER_PREVIEW[0].label == std::string_view("Species"));
    assert(SEARCH_FILTER_PREVIEW[6].label == std::string_view("Recent"));
    assert(COLLECTION_PREVIEW[1].title == std::string_view("Shiny Living Dex"));
    return 0;
}
