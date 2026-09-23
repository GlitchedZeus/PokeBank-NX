#include "Utils/DurableFile.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>
#include <unistd.h>

namespace {
std::vector<uint8_t> readFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
void writeFile(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    assert(out.good());
}
}

int main() {
    using PokeBank::Storage::DurableFile::replace;

    const std::string target = "/tmp/pokebank-durable-" + std::to_string(static_cast<long long>(getpid())) + ".dat";
    const std::vector<uint8_t> oldBytes{1,2,3,4,5};
    const std::vector<uint8_t> newBytes{9,8,7,6,5,4};

    std::remove(target.c_str());

    // Success: authoritative target advances only after temp validation and the previous
    // known-good generation remains independently recoverable.
    writeFile(target, oldBytes);
    auto exact = [&](std::span<const uint8_t> bytes, std::string& error) {
        if (!std::equal(bytes.begin(), bytes.end(), newBytes.begin(), newBytes.end())) {
            error = "unexpected bytes";
            return false;
        }
        return true;
    };
    auto ok = replace(target, newBytes, exact);
    assert(ok.ok);
    assert(readFile(target) == newBytes);
    assert(!ok.previousPath.empty());
    assert(readFile(ok.previousPath) == oldBytes);

    // Pre-promotion validation failure: original target must remain byte-identical.
    writeFile(target, oldBytes);
    auto reject = [](std::span<const uint8_t>, std::string& error) {
        error = "intentional validator rejection";
        return false;
    };
    auto rejected = replace(target, newBytes, reject);
    assert(!rejected.ok);
    assert(readFile(target) == oldBytes);

    // Post-promotion validation failure: restore the exact prior generation and preserve
    // the failed candidate separately for diagnosis.
    writeFile(target, oldBytes);
    int validationCalls = 0;
    auto failAfterPromotion = [&](std::span<const uint8_t> bytes, std::string& error) {
        ++validationCalls;
        if (bytes.size() != newBytes.size()) {
            error = "wrong size";
            return false;
        }
        if (validationCalls == 1) return true;
        error = "intentional promoted-image rejection";
        return false;
    };
    auto rolledBack = replace(target, newBytes, failAfterPromotion);
    assert(!rolledBack.ok);
    assert(readFile(target) == oldBytes);
    assert(!rolledBack.failedPath.empty());
    assert(readFile(rolledBack.failedPath) == newBytes);

    std::remove(target.c_str());
    if (!ok.previousPath.empty()) std::remove(ok.previousPath.c_str());
    if (!rolledBack.previousPath.empty()) std::remove(rolledBack.previousPath.c_str());
    if (!rolledBack.failedPath.empty()) std::remove(rolledBack.failedPath.c_str());

    std::cout << "Durable file replacement + rollback: PASS\n";
}
