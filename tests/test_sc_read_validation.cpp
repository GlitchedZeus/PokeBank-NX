#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Save/SCReadValidation.h"

namespace {

using Enums::SCTypeCode;
using Save::Block;
using namespace Save::SCReadValidation;

Block block(uint32_t key, SCTypeCode type, std::size_t size) {
    Block result{};
    result.key = key;
    result.type = type;
    result.data.resize(size, 0);
    return result;
}

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
}

Block& currentBox(std::vector<Block>& blocks) {
    for (auto& value : blocks) {
        if (value.key == GEN9_CURRENT_BOX || value.key == SWSH_CURRENT_BOX)
            return value;
    }
    std::cerr << "FAIL: Current Box fixture block missing\n";
    std::exit(1);
}

std::vector<Block> makeSWSH() {
    return {
        block(SWSH_MY_STATUS, SCTypeCode::Object, 0xB0 + 0x1A),
        block(SWSH_PARTY, SCTypeCode::Object, 6 * Encryption::SIZE_PARTY8_SWSH),
        block(SWSH_MONEY, SCTypeCode::Object, 8),
        block(SWSH_ITEMS, SCTypeCode::Object, SWSH_ITEM_BLOCK_BYTES),
        block(SWSH_BOX, SCTypeCode::Object,
              BOX_COUNT * BOX_SLOTS * Encryption::SIZE_PARTY8_SWSH),
        block(SWSH_BOX_LAYOUT, SCTypeCode::Object, BOX_COUNT * BOX_NAME_BYTES),
        block(SWSH_CURRENT_BOX, SCTypeCode::Byte, 1),
    };
}

std::vector<Block> makeSV() {
    return {
        block(GEN9_MY_STATUS, SCTypeCode::Object, 0x10 + 0x1A),
        block(GEN9_PARTY, SCTypeCode::Object, 6 * Encryption::SIZE_PARTY9_SV),
        block(GEN9_MONEY, SCTypeCode::UInt32, 4),
        block(GEN9_ITEMS, SCTypeCode::Object, GEN9_ITEM_BLOCK_BYTES),
        block(GEN9_BOX, SCTypeCode::Object,
              BOX_COUNT * BOX_SLOTS * Encryption::SIZE_PARTY9_SV),
        block(GEN9_BOX_LAYOUT, SCTypeCode::Object, BOX_COUNT * BOX_NAME_BYTES),
        block(GEN9_CURRENT_BOX, SCTypeCode::Byte, 1),
    };
}

std::vector<Block> makeZA() {
    return {
        block(GEN9_MY_STATUS, SCTypeCode::Object, 0x10 + 0x1A),
        block(GEN9_PARTY, SCTypeCode::Object, 6 * Encryption::PARTY_SLOT_SIZE9_LZA),
        block(GEN9_MONEY, SCTypeCode::UInt32, 4),
        block(GEN9_ITEMS, SCTypeCode::Object, GEN9_ITEM_BLOCK_BYTES),
        block(GEN9_BOX, SCTypeCode::Object,
              BOX_COUNT * BOX_SLOTS * Encryption::BOX_SLOT_SIZE9_LZA),
        block(GEN9_BOX_LAYOUT, SCTypeCode::Object, BOX_COUNT * BOX_NAME_BYTES),
        block(GEN9_CURRENT_BOX, SCTypeCode::Byte, 1),
        block(ZA_SAVE_REVISION, SCTypeCode::UInt64, 8),
    };
}

template <typename Validator>
void verifyCurrentBoxContract(std::vector<Block> blocks, Validator validator, const char* game) {
    std::string diagnostic;
    require(validator(blocks, &diagnostic).empty(), game);

    auto& box = currentBox(blocks);
    box.type = SCTypeCode::UInt32;
    box.data.resize(4);
    diagnostic.clear();
    const auto error = validator(blocks, &diagnostic);
    require(!error.empty(), "four-byte Current Box must fail closed");
    require(diagnostic.find("key=0x017C3CBB") != std::string::npos,
            "Current Box rejection must identify the exact block key");
    require(diagnostic.find("wrong type") != std::string::npos,
            "Current Box rejection must identify a scalar type mismatch");
}

} // namespace

int main() {
    verifyCurrentBoxContract(makeSWSH(), Save::SCReadValidation::validateSWSH,
                             "SWSH one-byte Current Box should pass preflight");
    verifyCurrentBoxContract(makeSV(), Save::SCReadValidation::validateSV,
                             "SV one-byte Current Box should pass preflight");
    verifyCurrentBoxContract(makeZA(), Save::SCReadValidation::validateZA,
                             "Z-A one-byte Current Box should pass preflight");
    std::cout << "Native SC Current Box validation: PASS\n";
    return 0;
}
