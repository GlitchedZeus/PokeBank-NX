#ifndef UTILS_UTILITIES_H
#define UTILS_UTILITIES_H

#include <cstdint>
#include <string>
#include <switch.h>
#include <cstdint>

#include "Utils/StringHelpers.h"
#include "Utils/Logger.h"

/// Helper to read little-endian uint16_t from a byte pointer.
uint16_t readUInt16LittleEndian(const uint8_t* ptr);

/// Helper to read little-endian int32_t from a byte pointer.
int32_t readInt32LittleEndian(const uint8_t* ptr);

/// Helper to read little-endian uint32_t from a byte pointer.
uint32_t readUInt32LittleEndian(const uint8_t* ptr);

/// Helper to write little-endian uint16_t to a byte pointer.
void writeUInt16LittleEndian(uint8_t* ptr, uint16_t value);

/// Helper to write little-endian int32_t to a byte pointer.
void writeInt32LittleEndian(uint8_t* ptr, int32_t value);

/// Helper to write little-endian uint32_t to a byte pointer.
void writeUInt32LittleEndian(uint8_t* ptr, uint32_t value);

uint16_t reverseEndianness(uint16_t value);

/// Sanitize the file name as some titles have unsupported UTF-8 characters
std::string sanitizeTitleName(std::string input);
/// Format game title to Pascal Case
std::string formatPascalCase(std::string input);

std::string getTitleName(u64 titleId);

#endif