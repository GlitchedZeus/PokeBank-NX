#include <cstdint>

#include <switch.h>

#include "Utils/HelperUtilities.h"
#include "Utils/StringHelpers.h"
#include "Utils/Logger.h"

namespace Utils {
    uint16_t readUInt16LittleEndian(const uint8_t* ptr) {
        return
        static_cast<uint16_t>(ptr[0]) |
        (static_cast<uint16_t>(ptr[1]) << 8);
    }

    int32_t readInt32LittleEndian(const uint8_t* ptr) {
        return
            static_cast<uint32_t>(ptr[0]) |
            (static_cast<uint32_t>(ptr[1]) << 8) |
            (static_cast<uint32_t>(ptr[2]) << 16) |
            (static_cast<uint32_t>(ptr[3]) << 24);
    }

    uint32_t readUInt32LittleEndian(const uint8_t* ptr) {
        return
            static_cast<uint32_t>(ptr[0]) |
            (static_cast<uint32_t>(ptr[1]) << 8) |
            (static_cast<uint32_t>(ptr[2]) << 16) |
            (static_cast<uint32_t>(ptr[3]) << 24);
    }

    void writeUInt16LittleEndian(uint8_t* ptr, uint16_t value) {
        ptr[0] = static_cast<uint8_t>(value & 0xFF);
        ptr[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }

    void writeInt32LittleEndian(uint8_t* ptr, int32_t value) {
        uint32_t uvalue = static_cast<uint32_t>(value);
        ptr[0] = static_cast<uint8_t>(uvalue & 0xFF);
        ptr[1] = static_cast<uint8_t>((uvalue >> 8) & 0xFF);
        ptr[2] = static_cast<uint8_t>((uvalue >> 16) & 0xFF);
        ptr[3] = static_cast<uint8_t>((uvalue >> 24) & 0xFF);
    }

    void writeUInt32LittleEndian(uint8_t* ptr, uint32_t value) {
        ptr[0] = static_cast<uint8_t>(value & 0xFF);
        ptr[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        ptr[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        ptr[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    uint16_t reverseEndianness(uint16_t value) {
        // Swap the two bytes
        return static_cast<uint16_t>((value >> 8) | (value << 8));
    }

    std::string getTitleName(u64 titleId)
    {
        NsApplicationControlData controlData;
        u64 controlDataSize = 0;
        Result result = nsGetApplicationControlData(NsApplicationControlSource_Storage, titleId, &controlData, sizeof(controlData), &controlDataSize);
        if (R_FAILED(result))
        {
            printf("Failed to get application control data for TitleID: 0x%016lX (error: 0x%x)\n", titleId, result);
            return "UnknownGame";
        }

        const NacpStruct *nacp = (const NacpStruct *)&controlData.nacp;

        // Iterate over language entries to find the first valid name
        for (size_t i = 0; i < std::size(nacp->lang); i++)
        {
            if (nacp->lang[i].name[0] != '\0')
            {
                return nacp->lang[i].name;
            }
        }

        // Fallback if no name found
        return "UnknownGame";
    }

    /// Sanitize the file name as some titles have unsupported UTF-8 characters
    std::string sanitizeTitleName(std::string input) {
        std::string output;
        size_t i = 0;

        while (i < input.size()) {
            unsigned char c = static_cast<unsigned char>(input[i]);

            // Handle UTF-8 characters like é
            if (c == 0xC3 && i + 1 < input.size()) {
                unsigned char next = static_cast<unsigned char>(input[i + 1]);
                if (next == 0xA9) {  // é
                    output.push_back('e');
                    i += 2;
                    continue;
                } else if (next == 0x89) {  // É
                    output.push_back('E');
                    i += 2;
                    continue;
                }
            }

            // Handle valid alphanumeric characters
            if (std::isalnum(c)) {
                output.push_back(c);
            } else if (c == ' ') {
                output.push_back('_');  // Replace spaces with underscores
            }

            // Handle 3-byte UTF-8 (common in Japanese)
            if (c == 0xE3 && i + 2 < input.size()) {
                unsigned char c1 = static_cast<unsigned char>(input[i + 1]);
                unsigned char c2 = static_cast<unsigned char>(input[i + 2]);
                // Example: ポケモン (Pokémon) -> "Pokemon"
                if (c1 == 0x83 && c2 == 0xBB) {
                    output.push_back('P');
                    output.push_back('o');
                    output.push_back('k');
                    output.push_back('e');
                    output.push_back('m');
                    output.push_back('o');
                    i += 3;
                    continue;
                }
                if (c1 == 0x82 && c2 == 0xA4) {
                    output.push_back('V');
                    output.push_back('i');
                    output.push_back('o');
                    output.push_back('l');
                    output.push_back('e');
                    output.push_back('t');
                    i += 3;
                    continue;
                }
                // Add more mappings as needed (use online UTF-8 decoder for Violet's NACP)
            }

            ++i;
        }

        return output;
    }

    /// Format game title to Pascal Case
    std::string formatPascalCase(std::string input) {
        std::string output;
        bool capitalize = true;
        size_t i = 0;

        while (i < input.size()) {
            unsigned char c = static_cast<unsigned char>(input[i]);

            if (c == '_' || c == ' ') {
                capitalize = true;
                ++i;
                continue;
            }

            // Process alphanumeric or other characters (assumes sanitized input)
            char processed = static_cast<char>(
                capitalize ?
                std::toupper(static_cast<int>(c)) :
                std::tolower(static_cast<int>(c))
            );
            output.push_back(processed);
            capitalize = false;
            ++i;
        }

        return output;
    }
}
