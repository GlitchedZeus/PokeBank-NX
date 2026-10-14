// Verified classic (Gen I / Gen II) Bag export: real strict parsers as validators, exact
// original backup, checksum-valid edited save, refusal of corrupt staged bytes, fault cleanup.
#include "Integration/Common/ClassicStrictValidators.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace PokeVault::Integration;

namespace {

uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value = 0xFF;
    for (uint8_t b : bytes) value = static_cast<uint8_t>(value - b);
    return value;
}

// International Red/Blue main-data block with a valid checksum (same layout as test_rby_discovery).
std::vector<uint8_t> gen1Save(uint32_t moneyBcd) {
    constexpr size_t kMain = 0x2598, kLen = 0x0F8B, kSum = 0x3523;
    std::vector<uint8_t> d(Gen1::kRawSaveSize, 0);
    const uint8_t will[] = {0x96, 0x88, 0x8B, 0x8B, 0x50};
    std::copy(std::begin(will), std::end(will), d.begin() + kMain);
    d[0x25F3] = static_cast<uint8_t>(moneyBcd >> 16);
    d[0x25F4] = static_cast<uint8_t>(moneyBcd >> 8);
    d[0x25F5] = static_cast<uint8_t>(moneyBcd);
    d[0x2605] = 0x12; d[0x2606] = 0x34;
    d[0x29C3] = 0x99;
    d[0x284C] = 2;
    d[0x2F2C] = 0; d[0x2F2D] = 0xFF;
    d[0x30C0] = 0; d[0x30C1] = 0xFF;
    d[0x2ED5] = 0xFF;
    d[kSum] = diff8(std::span<const uint8_t>(d).subspan(kMain, kLen));
    return d;
}

// International Gold/Silver with one boxed and one party Pokemon (same layout as test_gsc_discovery).
std::vector<uint8_t> gen2Save(uint8_t moneyLow) {
    constexpr size_t money = 0x23DB, idx = 0x2724, names = 0x2727, party = 0x288A,
                     current = 0x2D6C, end = 0x2D68, sum = 0x2D69;
    auto listLen = [](size_t cap, size_t body, size_t str) { return 1 + (cap + 1) + cap * body + 2 * cap * str; };
    const size_t stride = listLen(20, 32, 11) + 2;
    auto boxStart = [&](size_t b) { return b < 7 ? 0x4000 + b * stride : 0x6000 + (b - 7) * stride; };
    auto name = [](std::vector<uint8_t>& d, size_t ofs, size_t len, char c) {
        std::fill_n(d.begin() + static_cast<std::ptrdiff_t>(ofs), static_cast<std::ptrdiff_t>(len), 0x50);
        d[ofs] = static_cast<uint8_t>(0x80 + c - 'A');
    };
    auto pokemonList = [&](std::vector<uint8_t>& d, size_t ofs, size_t cap, size_t bodySize) {
        d[ofs] = 1; d[ofs + 1] = 25; d[ofs + 2] = 0xFF;
        const size_t body = ofs + 1 + (cap + 1);
        d[body] = 25; d[body + 6] = 0x12; d[body + 7] = 0x34; d[body + 21] = 0x7A;
        d[body + 22] = 0xAA; d[body + 31] = 15;
        const size_t ot = body + cap * bodySize;
        name(d, ot, 11, 'O');
        name(d, ot + cap * 11, 11, 'M');
    };
    std::vector<uint8_t> d(0x8000);
    d[0x2009] = 0x12; d[0x200A] = 0x34; name(d, 0x200B, 11, 'A');
    d[money] = 0x01; d[money + 1] = 0xE2; d[money + 2] = moneyLow; d[idx] = 0;
    for (size_t b = 0; b < 14; ++b) {
        d[boxStart(b)] = 0; d[boxStart(b) + 1] = 0xFF;
        name(d, names + b * 9, 9, static_cast<char>('A' + b));
    }
    pokemonList(d, boxStart(0), 20, 32);
    d[current] = 0; d[current + 1] = 0xFF;
    pokemonList(d, party, 6, 48);
    uint16_t s = 0;
    for (size_t i = 0x2009; i <= end; ++i) s = static_cast<uint16_t>(s + d[i]);
    d[sum] = static_cast<uint8_t>(s); d[sum + 1] = static_cast<uint8_t>(s >> 8);
    return d;
}

std::vector<uint8_t> slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

void exportCase(const fs::path& root, const std::string& gameId,
                const std::vector<uint8_t>& original, const std::vector<uint8_t>& edited,
                Common::StrictSaveValidator validator) {
    std::string error;
    assert(validator(original, error) && validator(edited, error));

    Common::VerifiedExportRequest request;
    request.rootDirectory = root.string();
    request.directoryStem = gameId + "_20261014-120000";
    request.generationLabel = "Classic Bag";
    request.fileExtension = ".srm";
    request.gameId = gameId;
    request.originalBytes = original;
    request.editedBytes = edited;
    request.changeLines = {"Money: before -> after"};
    request.validator = validator;

    // Corrupt staged bytes never reach the filesystem.
    auto corrupt = edited;
    corrupt[0x2600] ^= 0x5A;
    auto bad = request;
    bad.editedBytes = corrupt;
    const auto refused = Common::publishVerifiedStagedExport(bad);
    assert(!refused.success && refused.error.find("Strict staged reparse failed") == 0);
    assert(!fs::exists(root / request.directoryStem));

    auto faulty = request;
    faulty.failurePoint = Common::ExportFailurePoint::AfterManifestWrite;
    assert(!Common::publishVerifiedStagedExport(faulty).success);
    assert(fs::is_empty(root));

    const auto result = Common::publishVerifiedStagedExport(request);
    assert(result.success && result.changedBytes > 0);
    const fs::path dir(result.directory);
    assert(slurp(dir / "original_backup.srm") == original);
    assert(slurp(dir / "edited.srm") == edited);
    assert(validator(slurp(dir / "edited.srm"), error));
    fs::remove_all(dir);
}

} // namespace

int main() {
    const fs::path root = fs::temp_directory_path() / ("pokebank-classic-export-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);

    // Gen I Red: edited money keeps a valid main-data checksum.
    exportCase(root, "red_gb", gen1Save(0x123456), gen1Save(0x654321),
               Common::gen1StrictValidator(Gen1::SourceGame::Red));
    // Gen II Gold: edited money keeps a valid primary checksum.
    exportCase(root, "gold_gbc", gen2Save(0x40), gen2Save(0x41),
               Common::gen2StrictValidator(Gen2::SourceGame::Gold));

    fs::remove_all(root);
    std::cout << "Classic verified Bag export (Gen I/II strict validators): PASS\n";
    return 0;
}
