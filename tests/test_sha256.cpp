#include "Utils/SHA256.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

static std::string digest(const std::vector<uint8_t>& bytes) {
    Utils::SHA256 sha;
    if (!bytes.empty()) sha.update(bytes.data(), bytes.size());
    std::array<uint8_t, Utils::PKSE_SHA256_HASH_SIZE> out{};
    sha.finalize(out.data());

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (uint8_t b : out) hex << std::setw(2) << static_cast<unsigned>(b);
    return hex.str();
}

static std::vector<uint8_t> bytes(std::string_view text) {
    return std::vector<uint8_t>(text.begin(), text.end());
}

int main() {
    assert(digest({}) ==
           "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(digest(bytes("abc")) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(digest(bytes("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
           "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    std::vector<uint8_t> highBit(64);
    for (std::size_t i = 0; i < highBit.size(); ++i)
        highBit[i] = static_cast<uint8_t>(0x80u + i);
    assert(digest(highBit) ==
           "c39e13bbb05726a3c0747d3ca54c27e3f86bc10a1d3754cd031bd1ca7256c8ed");

    std::cout << "SHA-256 known-answer + high-bit decode: PASS\n";
    return 0;
}
