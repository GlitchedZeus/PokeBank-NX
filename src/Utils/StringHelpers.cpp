#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include "Utils/HelperUtilities.h"
#include "Utils/StringHelpers.h"

namespace Utils {
    // Equivalent to C# LoadString
    /// Loads characters into the result buffer and returns the count of characters loaded.
    int loadString(const uint8_t* data, size_t data_size, char16_t* result, size_t result_capacity) {
        size_t i = 0;
        for (; i < data_size; i += 2) {
            uint16_t value = readUInt32LittleEndian(&data[i]);
            if (value == TerminatorNull) {
                break;
            }
            result[i / 2] = static_cast<char16_t>(value);
        }
        return static_cast<int>(i / 2);
    }

    // Equivalent to C# GetString
    /// Converts Generation 7-Beluga encoded data to a decoded string.
    std::u16string getString(const uint8_t* data, size_t data_size) {
        // Allocate a vector for the result (heap-based, as dynamic stack allocation is non-standard in C++17)
        // Size is data_size / 2 to account for byte pairs, plus some padding if needed; adjust based on expected max.
        std::vector<char16_t> result(data_size / 2 + 1);  // +1 for potential null terminator, though not used here
        
        int length = loadString(data, data_size, result.data(), result.size());
        
        // Construct the u16string from the loaded portion
        return std::u16string(result.data(), static_cast<size_t>(length));
    }

    /// Converts a UTF-16 string to UTF-8
    std::string utf16ToUtf8(const std::u16string& utf16str) {
        std::string utf8str;
        utf8str.reserve(utf16str.length() * 3);

        for (size_t i = 0; i < utf16str.length(); ++i) {
            char32_t codepoint = utf16str[i];

            // Handle surrogate pairs for characters outside the Basic Multilingual Plane
            if (codepoint >= 0xD800 && codepoint <= 0xDBFF && i + 1 < utf16str.length()) {
                char16_t high = utf16str[i];
                char16_t low = utf16str[i + 1];
                if (low >= 0xDC00 && low <= 0xDFFF) {
                    codepoint = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                    ++i;
                }
            }

            // Convert to UTF-8
            if (codepoint <= 0x7F) {
                utf8str.push_back(static_cast<char>(codepoint));
            } else if (codepoint <= 0x7FF) {
                utf8str.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                utf8str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint <= 0xFFFF) {
                utf8str.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                utf8str.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                utf8str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint <= 0x10FFFF) {
                utf8str.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
                utf8str.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                utf8str.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                utf8str.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

        return utf8str;
    }
}