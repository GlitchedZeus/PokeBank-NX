#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <cstdint>

#include "Enums/SCTypeCode.h"

// Simple struct to hold a block
struct Block {
    uint32_t key;
    SCTypeCode type;
    SCTypeCode sub_type = SCTypeCode::None; // Only for Array
    std::vector<uint8_t> data; // Empty for Bool types
};

bool tryReadBlock(const uint8_t* data, size_t data_size, uint32_t key, size_t& offset, Block& result);
std::vector<Block> parseAllBlocks(const uint8_t* data, size_t data_size);

// Serialization functions
size_t writeBlock(const Block& block, std::vector<uint8_t>& output);
std::vector<uint8_t> serializeAllBlocks(const std::vector<Block>& blocks);

#endif