// QA fuzz target: Save Instance identification from a file on disk, i.e. the read-only container
// adapters for NDS battery saves (raw 0x80000 and DeSmuME .dsv footers) used by Gen IV and Gen V
// discovery, manual linking and assigned-source reopen.
//
// Contract: the source file is never modified (bytes, size and mtime are identical afterwards);
// a payload is only reported ready when it is exactly 0x80000 bytes and equals the file's prefix;
// a row is only Ready when its exact title was validated from the save itself.
#include "fuzz_support.h"
#include "fuzz_synthetic_saves.h"

#include "Integration/Gen4/Gen4SourceDiscovery.h"
#include "Integration/Gen5/Gen5SourceDiscovery.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace G4 = PokeVault::Integration::Gen4;
namespace G5 = PokeVault::Integration::Gen5;
namespace fs = std::filesystem;

namespace {

const fs::path& scratchFile(const char* extension) {
    static const fs::path dir = [] {
        const fs::path d = fs::temp_directory_path() / ("pokebank-qa-src-" + std::to_string(::getpid()));
        fs::create_directories(d);
        return d;
    }();
    // Remove the scratch directory at exit (libFuzzer and the deterministic runner both exit normally).
    static const struct Cleanup {
        fs::path path;
        ~Cleanup() {
            std::error_code ignored;
            fs::remove_all(path, ignored);
        }
    } cleanup{dir};
    static fs::path raw = dir / "synthetic.sav";
    static fs::path dsv = dir / "synthetic.dsv";
    return std::string_view(extension) == ".dsv" ? dsv : raw;
}

struct FileState {
    qafuzz::Bytes bytes;
    struct stat st{};
};
FileState capture(const fs::path& path) {
    FileState state;
    state.bytes = qafuzz::slurp(path);
    QA_CHECK(::stat(path.c_str(), &state.st) == 0);
    return state;
}
void assertUnchanged(const fs::path& path, const FileState& before) {
    const auto after = capture(path);
    QA_CHECK(after.bytes == before.bytes);
    QA_CHECK(after.st.st_size == before.st.st_size);
    QA_CHECK(after.st.st_mtim.tv_sec == before.st.st_mtim.tv_sec &&
             after.st.st_mtim.tv_nsec == before.st.st_mtim.tv_nsec);
}

void runOne(std::span<const uint8_t> data) {
    for (const char* extension : {".sav", ".dsv"}) {
        const auto& path = scratchFile(extension);
        qafuzz::writeFile(path, qafuzz::Bytes(data.begin(), data.end()));
        // Back-date the mtime so a rewrite in the same clock tick would still be detected.
        struct timespec times[2] = {{1000000000, 0}, {1000000000, 0}};
        QA_CHECK(::utimensat(AT_FDCWD, path.c_str(), times, 0) == 0);
        const auto before = capture(path);

        const auto payload = G4::readSourcePayloadReadOnly(path.string());
        if (payload.ready()) {
            qafuzz::noteAccepted();
            QA_CHECK(payload.bytes.size() == 0x80000 && data.size() >= 0x80000);
            QA_CHECK(std::equal(payload.bytes.begin(), payload.bytes.end(), data.begin()));
        } else {
            QA_CHECK(payload.bytes.empty() || payload.bytes.size() == 0x80000);
        }
        for (const std::string_view game : {std::string_view{}, std::string_view{"platinum_nds"},
                                            std::string_view{"heartgold_nds"}}) {
            const auto candidate = G4::inspectSourceFile(path.string(), "manual", game);
            if (candidate.ready()) {
                QA_CHECK(payload.ready());
                if (!game.empty()) QA_CHECK(G4::candidateMatchesGame(candidate, game));
                const auto row = G4::toSaveInstance(candidate, game.empty() ? "platinum_nds" : game);
                QA_CHECK(row.readOnly());
            }
        }

        qafuzz::Bytes normalized;
        std::string diagnostic, container;
        const bool g5 = G5::readNormalizedSourceReadOnly(path.string(), normalized, diagnostic, &container);
        if (g5) {
            QA_CHECK(normalized.size() == 0x80000 && data.size() >= 0x80000);
            QA_CHECK(std::equal(normalized.begin(), normalized.end(), data.begin()));
        } else {
            QA_CHECK(!diagnostic.empty());
        }
        for (const std::string_view game : {std::string_view{}, std::string_view{"black_nds"},
                                            std::string_view{"white2_nds"}}) {
            for (const auto& row : G5::inspectSourceChoices(path.string(), "DraStic", game)) {
                QA_CHECK(row.readOnly());
                if (row.ready()) {
                    qafuzz::noteAccepted();
                    QA_CHECK(g5 && G5::isExactGen5Id(row.gameId));
                    if (!game.empty()) QA_CHECK(row.gameId == game);
                }
            }
        }
        assertUnchanged(path, before);
    }
}

qafuzz::Bytes dsv(const qafuzz::Bytes& raw, uint32_t padded = 0x80000u) {
    qafuzz::Bytes out = raw;
    out.insert(out.end(), 13, 0xA5);
    for (uint32_t v : {static_cast<uint32_t>(raw.size()), padded, 0u, 0u, 0u, 0u}) {
        out.push_back(static_cast<uint8_t>(v));
        out.push_back(static_cast<uint8_t>(v >> 8));
        out.push_back(static_cast<uint8_t>(v >> 16));
        out.push_back(static_cast<uint8_t>(v >> 24));
    }
    const std::string cookie = "|-DESMUME SAVE-|";
    out.insert(out.end(), cookie.begin(), cookie.end());
    return out;
}

std::vector<qafuzz::Bytes> seeds() {
    using qasynth::gen4::Kind;
    using qasynth::gen5::Family;
    const auto pt = qasynth::gen4::save(Kind::Platinum);
    const auto bw = qasynth::gen5::save(Family::BlackWhite, 21, 1);
    const auto b2 = qasynth::gen5::save(Family::Black2White2, 22, 4);  // two different valid copies
    return {pt, dsv(pt), bw, dsv(bw), b2};
}

} // namespace

const qafuzz::Target& qaTarget() {
    // Footer fields of the .dsv container (actual size, padded size, version, cookie).
    const size_t footer = 0x80000 + 13;
    static const qafuzz::Target target{
        "source-containers", runOne, seeds, qasynth::gen5::forge,
        {footer, footer + 1, footer + 2, footer + 4, footer + 6, footer + 20, footer + 24},
        {{footer, footer + 4, 4}, {0, 0x40000, 0x40000}},
    };
    return target;
}
