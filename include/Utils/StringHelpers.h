#ifndef UTILS_STRING_HELPERS_H
#define UTILS_STRING_HELPERS_H

#include <cstdint>
#include <cstddef>
#include <string>

namespace Utils {
    constexpr uint16_t TerminatorNull = 0u;

    enum class CheckedTextResult : uint8_t {
        Accepted,
        Overlength,
        MalformedUtf16,
    };

    // Equivalent to C# LoadString
    /// Loads characters into the result buffer and returns the count of characters loaded.
    int loadString(const uint8_t* data, size_t data_size, char16_t* result, size_t result_capacity);

    // Equivalent to C# GetString
    /// Converts Generation 7-Beluga encoded data to a decoded string.
    std::u16string getString(const uint8_t* data, size_t data_size);

    /// Converts a UTF-16 string to UTF-8
    std::string utf16ToUtf8(const std::u16string& utf16str);

    /// Converts a UTF-8 string to UTF-16. Covers the full BMP; astral code points are emitted as
    /// surrogate pairs. Malformed/truncated byte sequences are skipped. The inverse of utf16ToUtf8.
    std::u16string utf8ToUtf16(const std::string& utf8str);

    /// LOW-LEVEL, TRUNCATING: writes up to maxChars code units, then zero-fills the rest of the
    /// data_size bytes. It cuts silently (and can split a surrogate pair), so it must not be used
    /// for save data: every save-data name writer goes through setStringChecked (Round 14 P0-3).
    void setString(uint8_t* dest, size_t data_size, const std::u16string& value, size_t maxChars);

    /// The check setStringChecked makes, without writing: the text must be well-formed UTF-16 with
    /// no NUL, at most maxUnits code units, and fit the field with its terminator.
    CheckedTextResult checkStringFits(size_t data_size, const std::u16string& value, size_t maxUnits);
    inline bool stringFits(size_t data_size, const std::u16string& value, size_t maxUnits) {
        return checkStringFits(data_size, value, maxUnits) == CheckedTextResult::Accepted;
    }

    /// Checked modern-text writer. maxUnits is a count of UTF-16 code units, not Unicode code points.
    /// Rejected input (over maxUnits or over the field, malformed UTF-16, embedded NUL) leaves dest
    /// unchanged; it is never cut, so a surrogate pair is never split.
    CheckedTextResult setStringChecked(uint8_t* dest, size_t data_size,
                                       const std::u16string& value, size_t maxUnits);
}

#endif