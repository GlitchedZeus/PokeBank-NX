#include <cstdint>
#include <cstddef>

// Need to account for Region Variants and pokemon specific forms. For example, Pumpkaboo has 4 forms: small, medium, large and jumbo
namespace Names {
    // Form name lookup table - indexed by Form ID
    static const char* FORM_NAMES[] = {
        "Alolan",   // 0
        "Galarian", // 1
        "Hisuian"   // 2
    };

    constexpr size_t FORM_NAMES_COUNT = 3;

    const char* getFormName(uint8_t formId) {
        if (formId >= FORM_NAMES_COUNT) {
            return "Unknown";
        }
        return FORM_NAMES[formId];
    }
}