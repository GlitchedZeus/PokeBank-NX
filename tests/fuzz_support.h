// PokeBank NX application QA: shared fuzz driver (Thread 4, QA-owned).
//
// One translation unit per fuzz target includes this header after defining a
// `qafuzz::Target` via `const qafuzz::Target& qaTarget()`. The same target is
// driven two ways:
//
//  * libFuzzer (clang -fsanitize=fuzzer,address,undefined, -DPOKEBANK_QA_LIBFUZZER):
//    LLVMFuzzerTestOneInput runs the target on the raw input AND on a copy whose
//    checksums were re-forged, so mutations reach past the first CRC gate.
//  * A deterministic corpus/mutation runner (any compiler, GCC included): every
//    built-in synthetic seed plus any corpus files are mutated with fixed
//    strategies (truncation, extension, bit flips, interesting bytes,
//    out-of-range counts, swapped primary/backup regions, block splices), each
//    with and without checksum forging.
//
// Contract every target enforces: an input is either rejected cleanly or it is
// accepted and round-trips (re-parse of the accepted bytes is identical, the
// caller buffer is never mutated, a no-op staged editor finalizes to the same
// bytes). Sanitizers catch crashes, UB and out-of-bounds reads.
//
// No real saves, ROMs or keys are used: all seeds are synthetic and built in code.
#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace qafuzz {

using Bytes = std::vector<uint8_t>;

[[noreturn]] inline void failInvariant(const char* expr, const char* file, int line) {
    std::fprintf(stderr, "QA FUZZ INVARIANT FAILED: %s (%s:%d)\n", expr, file, line);
    std::fflush(stderr);
    std::abort();
}
#define QA_CHECK(cond) \
    do { if (!(cond)) ::qafuzz::failInvariant(#cond, __FILE__, __LINE__); } while (0)

// Targets call noteAccepted() whenever an input passed the strict loader, so the runner can
// prove the synthetic seeds are genuinely accepted (and fuzzing reaches the deep paths).
inline size_t& acceptedCounter() {
    static size_t count = 0;
    return count;
}
inline void noteAccepted() { ++acceptedCounter(); }

struct SwapRegion {
    size_t first = 0;
    size_t second = 0;
    size_t length = 0;
};

struct Target {
    const char* name = "unnamed";
    // Runs every entry point on the bytes. Must never crash; asserts invariants with QA_CHECK.
    std::function<void(std::span<const uint8_t>)> run;
    // Synthetic, structurally valid seeds (built in code, never real saves).
    std::function<std::vector<Bytes>()> seeds;
    // Optional: rewrite format checksums in place (block CRCs, sector sums, diff8...).
    std::function<void(Bytes&)> forge;
    // Offsets of count/length bytes worth driving out of range (party counts, list counts...).
    std::vector<size_t> countOffsets;
    // Region pairs to swap (primary/backup copies, footers, partitions).
    std::vector<SwapRegion> swaps;
    // Every built-in seed must be accepted by at least one entry point, except this many
    // deliberately-invalid skeleton seeds placed at the END of seeds().
    size_t rejectedSeedTail = 0;
};

// Runs the target on the bytes and, if a forger exists, on a forged copy as well.
inline void runWithForge(const Target& target, std::span<const uint8_t> data) {
    const Bytes before(data.begin(), data.end());
    target.run(data);
    QA_CHECK(std::equal(before.begin(), before.end(), data.begin(), data.end()));
    if (target.forge) {
        Bytes forged(data.begin(), data.end());
        target.forge(forged);
        target.run(forged);
    }
}

class Rng {
public:
    explicit Rng(uint64_t seed) : state_(seed ? seed : 0x9E3779B97F4A7C15ull) {}
    uint64_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 7;
        state_ ^= state_ << 17;
        return state_;
    }
    size_t below(size_t n) { return n == 0 ? 0 : static_cast<size_t>(next() % n); }

private:
    uint64_t state_;
};

enum class Strategy : uint8_t {
    Identity, Truncate, Extend, BitFlip, InterestingByte, CountOutOfRange, SwapRegions,
    SpliceBlock, Count_
};

inline const char* strategyName(Strategy s) {
    switch (s) {
        case Strategy::Identity: return "identity";
        case Strategy::Truncate: return "truncate";
        case Strategy::Extend: return "extend";
        case Strategy::BitFlip: return "bitflip";
        case Strategy::InterestingByte: return "interesting-byte";
        case Strategy::CountOutOfRange: return "count-out-of-range";
        case Strategy::SwapRegions: return "swap-regions";
        case Strategy::SpliceBlock: return "splice-block";
        case Strategy::Count_: break;
    }
    return "?";
}

inline Bytes mutate(const Target& target, const Bytes& seed, Strategy strategy, Rng& rng) {
    static constexpr uint8_t kInteresting[] = {0x00, 0x01, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17,
                                               0x1E, 0x1F, 0x20, 0x7F, 0x80, 0xFE, 0xFF};
    Bytes out = seed;
    switch (strategy) {
        case Strategy::Identity:
            break;
        case Strategy::Truncate: {
            static constexpr size_t kFixed[] = {0, 1, 2, 0x2C, 0x7FFF, 0x8000, 0x1FFFF, 0x7FFFF};
            const size_t pick = rng.below(4);
            size_t len = pick == 0 ? out.size() - std::min<size_t>(out.size(), 1 + rng.below(64))
                       : pick == 1 ? out.size() / 2
                       : pick == 2 ? rng.below(out.size() + 1)
                                   : kFixed[rng.below(std::size(kFixed))];
            out.resize(std::min(len, out.size()));
            break;
        }
        case Strategy::Extend: {
            static constexpr size_t kGrow[] = {1, 2, 0x10, 0x2C, 0x7A, 0x100, 0x2000};
            const size_t grow = kGrow[rng.below(std::size(kGrow))];
            const uint8_t fill = rng.below(2) ? 0xFF : 0x00;
            out.insert(out.end(), grow, fill);
            if (rng.below(2) && !out.empty()) out[out.size() - 1 - rng.below(grow)] ^= 0x5A;
            break;
        }
        case Strategy::BitFlip: {
            if (out.empty()) break;
            const size_t flips = 1 + rng.below(8);
            for (size_t i = 0; i < flips; ++i)
                out[rng.below(out.size())] ^= static_cast<uint8_t>(1u << rng.below(8));
            break;
        }
        case Strategy::InterestingByte: {
            if (out.empty()) break;
            const size_t writes = 1 + rng.below(4);
            for (size_t i = 0; i < writes; ++i)
                out[rng.below(out.size())] = kInteresting[rng.below(std::size(kInteresting))];
            break;
        }
        case Strategy::CountOutOfRange: {
            if (target.countOffsets.empty()) {
                if (!out.empty()) out[rng.below(out.size())] = 0xFF;
                break;
            }
            static constexpr uint8_t kCounts[] = {0, 6, 7, 8, 20, 21, 30, 31, 0x7F, 0x80, 0xFE, 0xFF};
            const size_t at = target.countOffsets[rng.below(target.countOffsets.size())];
            if (at < out.size()) out[at] = kCounts[rng.below(std::size(kCounts))];
            break;
        }
        case Strategy::SwapRegions: {
            if (target.swaps.empty()) break;
            const auto& swap = target.swaps[rng.below(target.swaps.size())];
            if (swap.first + swap.length <= out.size() && swap.second + swap.length <= out.size())
                std::swap_ranges(out.begin() + static_cast<std::ptrdiff_t>(swap.first),
                                 out.begin() + static_cast<std::ptrdiff_t>(swap.first + swap.length),
                                 out.begin() + static_cast<std::ptrdiff_t>(swap.second));
            break;
        }
        case Strategy::SpliceBlock: {
            if (out.size() < 8) break;
            const size_t len = 1 + rng.below(std::min<size_t>(out.size() / 2, 0x400));
            const size_t from = rng.below(out.size() - len + 1);
            const size_t to = rng.below(out.size() - len + 1);
            Bytes chunk(out.begin() + static_cast<std::ptrdiff_t>(from),
                        out.begin() + static_cast<std::ptrdiff_t>(from + len));
            std::copy(chunk.begin(), chunk.end(), out.begin() + static_cast<std::ptrdiff_t>(to));
            break;
        }
        case Strategy::Count_:
            break;
    }
    return out;
}

inline Bytes slurp(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), {});
}

inline void writeFile(const std::filesystem::path& path, const Bytes& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

struct RunnerOptions {
    size_t iterations = 40;     // per seed per strategy
    double seconds = 0;         // 0 = no time box; otherwise stop when exhausted
    uint64_t rngSeed = 0x5EED0C7A11ull;
    std::vector<std::filesystem::path> corpus;   // extra seed files or directories
    std::filesystem::path writeSeeds;            // write built-in seeds for libFuzzer and exit
};

inline int deterministicMain(const Target& target, int argc, char** argv) {
    RunnerOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);
        auto value = [&](const char* flag) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s needs a value\n", flag);
                std::exit(2);
            }
            return argv[++i];
        };
        if (arg == "--iterations") options.iterations = std::strtoull(value("--iterations"), nullptr, 10);
        else if (arg == "--seconds") options.seconds = std::strtod(value("--seconds"), nullptr);
        else if (arg == "--rng-seed") options.rngSeed = std::strtoull(value("--rng-seed"), nullptr, 0);
        else if (arg == "--write-seeds") options.writeSeeds = value("--write-seeds");
        else options.corpus.emplace_back(argv[i]);
    }

    auto seeds = target.seeds ? target.seeds() : std::vector<Bytes>{};
    if (!options.writeSeeds.empty()) {
        std::filesystem::create_directories(options.writeSeeds);
        for (size_t i = 0; i < seeds.size(); ++i)
            writeFile(options.writeSeeds / (std::string(target.name) + "-seed-" + std::to_string(i) + ".bin"),
                      seeds[i]);
        std::printf("%s: wrote %zu synthetic seeds to %s\n", target.name, seeds.size(),
                    options.writeSeeds.string().c_str());
        return 0;
    }
    for (const auto& entry : options.corpus) {
        if (std::filesystem::is_directory(entry)) {
            std::vector<std::filesystem::path> files;
            for (const auto& file : std::filesystem::directory_iterator(entry))
                if (file.is_regular_file()) files.push_back(file.path());
            std::sort(files.begin(), files.end());
            for (const auto& file : files) seeds.push_back(slurp(file));
        } else {
            seeds.push_back(slurp(entry));
        }
    }
    if (seeds.empty()) {
        std::fprintf(stderr, "%s: no seeds\n", target.name);
        return 2;
    }

    const auto start = std::chrono::steady_clock::now();
    auto outOfTime = [&] {
        if (options.seconds <= 0) return false;
        const std::chrono::duration<double> spent = std::chrono::steady_clock::now() - start;
        return spent.count() >= options.seconds;
    };
    Rng rng(options.rngSeed);
    size_t executions = 0;
    bool timedOut = false;
    // Seeds themselves must be accepted-or-rejected cleanly first; built-in seeds must be accepted.
    const size_t builtIn = target.seeds ? target.seeds().size() : 0;
    for (size_t i = 0; i < seeds.size(); ++i) {
        const size_t before = acceptedCounter();
        target.run(seeds[i]);
        if (i + target.rejectedSeedTail < builtIn && acceptedCounter() == before) {
            std::fprintf(stderr, "%s: built-in synthetic seed %zu was not accepted by any entry point\n",
                         target.name, i);
            return 1;
        }
        runWithForge(target, seeds[i]);
        ++executions;
    }
    const size_t acceptedBaseline = acceptedCounter();
    // Round-robin over strategies so a time box still covers every strategy.
    for (size_t round = 0; round < options.iterations && !timedOut; ++round) {
        for (size_t s = 1; s < static_cast<size_t>(Strategy::Count_) && !timedOut; ++s) {
            for (const auto& seed : seeds) {
                const auto mutated = mutate(target, seed, static_cast<Strategy>(s), rng);
                runWithForge(target, mutated);
                ++executions;
                if (outOfTime()) {
                    timedOut = true;
                    break;
                }
            }
        }
    }
    const std::chrono::duration<double> spent = std::chrono::steady_clock::now() - start;
    std::printf("%s: %zu seeds, %zu executions (x2 with checksum forging), %zu mutant acceptances, "
                "in %.1fs%s - no crash, no invariant failure\n",
                target.name, seeds.size(), executions, acceptedCounter() - acceptedBaseline,
                spent.count(), timedOut ? " (time box)" : "");
    return 0;
}

} // namespace qafuzz

const qafuzz::Target& qaTarget();

#if defined(POKEBANK_QA_LIBFUZZER)
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    qafuzz::runWithForge(qaTarget(), std::span<const uint8_t>(data, size));
    return 0;
}
#else
int main(int argc, char** argv) { return qafuzz::deterministicMain(qaTarget(), argc, argv); }
#endif
