#ifndef UTILS_STRING_HELPERS_H
#define UTILS_STRING_HELPERS_H

#include <cstdint>
#include <cstddef>
#include <string>

constexpr uint16_t TerminatorNull = 0u;

// Equivalent to C# LoadString
/// Loads characters into the result buffer and returns the count of characters loaded.
int loadString(const uint8_t* data, size_t data_size, char16_t* result, size_t result_capacity);

// Equivalent to C# GetString
/// Converts Generation 7-Beluga encoded data to a decoded string.
std::u16string getString(const uint8_t* data, size_t data_size);

/// Converts a UTF-16 string to UTF-8
std::string utf16ToUtf8(const std::u16string& utf16str);

#endif