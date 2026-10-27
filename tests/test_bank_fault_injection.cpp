// Q01 (independent QA 2026-10-27; upstream PKSE 620c5f6): the production Trainer::Bank, linked
// with the real DurableFile and a fault-injecting readAllBytes, must tell a genuinely missing
// Bank from an existing one it cannot use. Only a missing Bank may become a first run. Every
// other case leaves the original bytes intact, commits no empty replacement and blocks saving
// with a readable reason.
//
// Runs in a private temporary working directory: the Bank's relative sdmc:/ root lands there.
#include "Trainer/Bank.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <unistd.h>
#include <vector>

// ---- test seams -------------------------------------------------------------------------------
namespace {
enum class ReadFault { None, OpenFails, ShortRead, AllocationFails };
ReadFault gFault = ReadFault::None;
int gReads = 0;
}  // namespace

namespace Utils {
// Same contract as src/Utils/ReadAllBytes.cpp (nullptr on any failure, new[] allocation), with
// injectable failures. Running as root, chmod cannot deny a read, so OpenFails stands in for a
// permission failure as well.
uint8_t* readAllBytes(const char* path, size_t* outSize) {
    ++gReads;
    if (outSize) *outSize = 0;
    if (!path) return nullptr;
    if (gFault == ReadFault::OpenFails) return nullptr;
    FILE* f = std::fopen(path, "rb");
    if (!f) return nullptr;
    std::fseek(f, 0, SEEK_END);
    const long length = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (length < 0 || gFault == ReadFault::AllocationFails) { std::fclose(f); return nullptr; }
    auto* bytes = new (std::nothrow) uint8_t[length ? static_cast<size_t>(length) : 1];
    size_t want = static_cast<size_t>(length);
    if (gFault == ReadFault::ShortRead && want > 0) want -= 1;
    const size_t got = std::fread(bytes, 1, want, f);
    std::fclose(f);
    if (got != static_cast<size_t>(length)) { delete[] bytes; return nullptr; }
    if (outSize) *outSize = got;
    return bytes;
}
void logInfoToFile(const char*) {}
void logInfoToFile(std::string) {}
bool gVerbose = std::getenv("BANK_FI_VERBOSE") != nullptr;
void logErrorToFile(const char* m) { if (gVerbose) std::fprintf(stderr, "ERR %s\n", m); }
void logErrorToFile(std::string m) { if (gVerbose) std::fprintf(stderr, "ERR %s\n", m.c_str()); }
void logInfoToFile(const char*, const char*) {}
void logErrorToFile(const char* m, const char* c) { if (gVerbose) std::fprintf(stderr, "ERR %s: %s\n", m, c); }
}  // namespace Utils

// Trainer.cpp is not linked; the LGPE entity only uses this for a display name.
namespace Trainer { const char* getSpeciesName(uint16_t) { return "?"; } }

namespace {
namespace fs = std::filesystem;
using Outcome = Trainer::Bank::LoadOutcome;

std::string bankPath() { return PokeBank::Paths::legacyBankRoot() + "/bank.dat"; }

std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void spit(const std::string& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::unique_ptr<Pokemon::Pokemon3FRLG> knownPokemon(uint16_t species) {
    std::array<uint8_t, 80> blank{};
    auto pk = std::make_unique<Pokemon::Pokemon3FRLG>(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    pk->setPID(0x12345678u);
    pk->setTID16(12345);
    pk->setSID16(54321);
    pk->setSpecies(species);
    pk->setLanguage(2);
    pk->setOTName(u"RED");
    pk->setNickname(u"KNOWN");
    pk->setLevel(20);
    return pk;
}

// A fresh empty working directory for each scenario.
void freshRoot() {
    static int n = 0;
    const fs::path dir = fs::temp_directory_path() /
        ("pokebank-bank-fault-" + std::to_string(::getpid()) + "-" + std::to_string(n++));
    fs::remove_all(dir);
    fs::create_directories(dir);
    assert(::chdir(dir.c_str()) == 0);
    fs::create_directory("sdmc:");  // the SD mount the console provides
    gFault = ReadFault::None;
}

// Writes a real Bank holding two known Pokemon through the production save path.
std::vector<uint8_t> writeKnownBank() {
    Trainer::Bank bank;
    assert(bank.lastLoadOutcome() == Outcome::FirstRun && !bank.isWriteBlocked());
    bank.boxes[0][0] = knownPokemon(25);
    bank.boxes[3][7] = knownPokemon(150);
    bank.boxNames[0] = "KEEP";
    assert(bank.save());
    const auto bytes = slurp(bankPath());
    assert(!bytes.empty());
    return bytes;
}

std::vector<std::string> bankFolder() {
    std::vector<std::string> names;
    for (const auto& e : fs::directory_iterator(PokeBank::Paths::legacyBankRoot()))
        names.push_back(e.path().filename().string());
    std::sort(names.begin(), names.end());
    return names;
}

// The blocked-state contract shared by every "exists but unusable" case.
void assertBlockedNoReplacement(Trainer::Bank& bank, Outcome expected,
                                const std::string& path, const std::vector<uint8_t>& original) {
    assert(bank.lastLoadOutcome() == expected);
    assert(bank.isWriteBlocked());
    assert(!bank.writeBlockReason().empty());
    const auto before = bankFolder();
    // The user tries to save anyway (e.g. after moving a Pokemon in): refused, nothing written.
    bank.boxes[1][1] = knownPokemon(133);
    assert(!bank.save());
    assert(bankFolder() == before);
    if (!original.empty()) assert(slurp(path) == original);
}
}  // namespace

int main() {
    const fs::path start = fs::current_path();

    // 0. Genuinely missing: the only first run, writable, and it really persists.
    {
        freshRoot();
        Trainer::Bank bank;
        assert(bank.lastLoadOutcome() == Outcome::FirstRun && !bank.isWriteBlocked());
        bank.boxes[0][0] = knownPokemon(1);
        assert(bank.save());
        Trainer::Bank reopened;
        assert(reopened.lastLoadOutcome() == Outcome::Loaded && !reopened.isWriteBlocked());
        assert(reopened.boxes[0][0] && reopened.boxes[0][0]->speciesID() == 1);
    }

    // 1-3. Existing Bank, read fails (open/permission, short read, allocation): left exactly in
    //      place (no rename, no quarantine), no migration, no replacement; a later healthy read
    //      opens it normally.
    for (const ReadFault fault : {ReadFault::OpenFails, ReadFault::ShortRead, ReadFault::AllocationFails}) {
        freshRoot();
        const auto original = writeKnownBank();
        const auto folder = bankFolder();
        gFault = fault;
        Trainer::Bank bank;
        assert(bank.boxes[0][0] == nullptr);  // nothing was invented
        assertBlockedNoReplacement(bank, Outcome::ReadFailed, bankPath(), original);
        assert(bankFolder() == folder);       // not even a quarantine rename
        gFault = ReadFault::None;
        Trainer::Bank healthy;
        assert(healthy.lastLoadOutcome() == Outcome::Loaded && !healthy.isWriteBlocked());
        assert(healthy.boxes[0][0] && healthy.boxes[0][0]->speciesID() == 25);
        assert(healthy.boxes[3][7] && healthy.boxes[3][7]->speciesID() == 150);
        assert(healthy.boxNames[0] == "KEEP");
    }

    // 4. Malformed contents (bad magic): preserved under a unique quarantine name, never deleted,
    //    saving blocked; the next launch is still not a first run while the set-aside copy exists.
    {
        freshRoot();
        writeKnownBank();
        std::vector<uint8_t> garbage(4096, 0x5A);
        spit(bankPath(), garbage);
        Trainer::Bank bank;
        assert(bank.lastLoadOutcome() == Outcome::Quarantined && bank.isWriteBlocked());
        assert(slurp(bankPath() + ".unreadable.0") == garbage);
        assert(bank.writeBlockReason().find("bank.dat.unreadable.0") != std::string::npos);
        assertBlockedNoReplacement(bank, Outcome::Quarantined, bankPath() + ".unreadable.0", garbage);
        assert(!fs::exists(bankPath()));
        Trainer::Bank relaunch;
        assert(relaunch.lastLoadOutcome() == Outcome::Blocked && relaunch.isWriteBlocked());
        assert(!relaunch.save() && !fs::exists(bankPath()));
        assert(slurp(bankPath() + ".unreadable.0") == garbage);
        // A second corrupt file later never overwrites the first set-aside copy.
        spit(bankPath(), std::vector<uint8_t>(64, 0x11));
        Trainer::Bank again;
        assert(again.lastLoadOutcome() == Outcome::Quarantined);
        assert(slurp(bankPath() + ".unreadable.0") == garbage);
        assert(slurp(bankPath() + ".unreadable.1") == std::vector<uint8_t>(64, 0x11));
    }

    // 5. Unsupported version (a newer writer): quarantined uniquely, saving blocked.
    {
        freshRoot();
        auto newer = writeKnownBank();
        newer[8] = 0x7F;
        spit(bankPath(), newer);
        Trainer::Bank bank;
        assertBlockedNoReplacement(bank, Outcome::Quarantined, bankPath() + ".unreadable.0", newer);
    }

    // 6. Truncated record table: inspectable, but saving would make the lost tail permanent.
    {
        freshRoot();
        auto original = writeKnownBank();
        original.resize(original.size() / 2);
        spit(bankPath(), original);
        Trainer::Bank bank;
        assert(bank.boxes[0][0] && bank.boxes[0][0]->speciesID() == 25);  // still viewable
        assertBlockedNoReplacement(bank, Outcome::Truncated, bankPath(), original);
    }

    // 7. Damaged record (checksum broken): the rest is viewable, the damage is not saved over.
    {
        freshRoot();
        auto original = writeKnownBank();
        // Record payload of box 3 slot 7 starts after the 16-byte header + tag.
        Trainer::Bank probe;
        const size_t recSize = (original.size() - 16) / (Trainer::Bank::BANK_BOX_COUNT * 30);
        (void)probe;
        const size_t rec = 16 + (3 * 30 + 7) * recSize;
        original[rec + 4 + 0x20] ^= 0xFF;
        spit(bankPath(), original);
        Trainer::Bank bank;
        assert(bank.lastLoadRejects() >= 1);
        assert(bank.boxes[0][0] && bank.boxes[0][0]->speciesID() == 25);
        assertBlockedNoReplacement(bank, Outcome::DamagedRecords, bankPath(), original);
    }

    // 8. Interrupted write: the exact state DurableFile::replace leaves between "target ->
    //    .previous.N" and "temp -> target". The previous generation is restored, nothing migrates.
    {
        freshRoot();
        const auto original = writeKnownBank();
        const std::string folder = PokeBank::Paths::legacyBankRoot();
        for (const auto& name : bankFolder())
            if (name != "bank.dat") fs::remove(folder + "/" + name);  // only our generation
        fs::rename(bankPath(), bankPath() + ".previous.0");
        spit(bankPath() + ".tmp.0", std::vector<uint8_t>(100, 0));  // half-written temp
        Trainer::Bank bank;
        assert(bank.lastLoadOutcome() == Outcome::Restored && !bank.isWriteBlocked());
        assert(slurp(bankPath()) == original);
        assert(bank.boxes[0][0] && bank.boxes[0][0]->speciesID() == 25);
        assert(fs::exists(bankPath() + ".tmp.0"));  // never promoted, never deleted
    }

    // 9. Interrupted write whose preserved generation is itself damaged: blocked, evidence kept.
    {
        freshRoot();
        writeKnownBank();
        const std::string folder = PokeBank::Paths::legacyBankRoot();
        for (const auto& name : bankFolder())
            if (name != "bank.dat") fs::remove(folder + "/" + name);
        fs::rename(bankPath(), bankPath() + ".previous.0");
        std::vector<uint8_t> bad(32, 0x00);
        spit(bankPath() + ".previous.0", bad);
        Trainer::Bank bank;
        assertBlockedNoReplacement(bank, Outcome::Blocked, bankPath() + ".previous.0", bad);
        assert(!fs::exists(bankPath()));
    }

    // 10. bank.dat is a directory (not inspectable as a file): blocked, untouched.
    {
        freshRoot();
        assert(PokeBank::Paths::ensureLegacyBankRoot(nullptr));
        fs::create_directories(bankPath());
        Trainer::Bank bank;
        assert(bank.lastLoadOutcome() == Outcome::Blocked && bank.isWriteBlocked());
        assert(!bank.save() && fs::is_directory(bankPath()));
    }

    assert(::chdir(start.c_str()) == 0);
    std::cout << "Bank fault injection (missing vs existing-unusable; production Bank): PASS\n";
}
