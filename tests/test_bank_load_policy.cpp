// Bank load start-up policy (upstream PKSE 620c5f6 "bank file safety", ported as a decision
// over real files). Exercises BankLoadPolicy::decide against an actual directory with the real
// DurableFile recovery, then checks Bank::load is wired to it.
#include "Trainer/BankLoadPolicy.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
namespace P = Trainer::BankLoadPolicy;

std::string readText(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    return {std::istreambuf_iterator<char>(in), {}};
}

void writeText(const std::string& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out);
    out << bytes;
}

bool exists(const std::string& path) {
    struct stat st{};
    return ::stat(path.c_str(), &st) == 0;
}

// Stand-in for Bank::validateStorageImage: a "bank" here is any image starting with BANKMAGIC.
bool validBank(std::span<const uint8_t> bytes, std::string& error) {
    static const std::string magic = "BANKMAGIC";
    if (bytes.size() < magic.size() ||
        std::string(bytes.begin(), bytes.begin() + static_cast<long>(magic.size())) != magic) {
        error = "not a bank";
        return false;
    }
    return true;
}

std::string freshDir(const char* name) {
    char templ[] = "/tmp/pokebank-bank-load-XXXXXX";
    const char* made = ::mkdtemp(templ);
    assert(made);
    const std::string dir = std::string(made) + "/" + name;
    assert(::mkdir(dir.c_str(), 0700) == 0);
    return dir;
}
}  // namespace

int main() {
    // 1. Genuinely absent, nothing preserved: the only first run.
    {
        const auto dir = freshDir("first-run");
        const auto d = P::decide(dir + "/bank.dat", validBank);
        assert(d.start == P::Start::FirstRun);
        assert(!d.restored);
        assert(!exists(dir + "/bank.dat"));
    }

    // 2. Interrupted save: target moved to .previous.N, new temp never promoted.
    //    The validating generation is restored and read; no migration.
    {
        const auto dir = freshDir("interrupted");
        writeText(dir + "/bank.dat.previous.0", "BANKMAGIC-older");
        writeText(dir + "/bank.dat.previous.4", "BANKMAGIC-newest");
        writeText(dir + "/bank.dat.tmp.0", "BANKMAGIC-half-written-temp");
        const auto d = P::decide(dir + "/bank.dat", validBank);
        assert(d.start == P::Start::ReadExisting);
        assert(d.restored);
        assert(readText(dir + "/bank.dat") == "BANKMAGIC-newest");
        // Older evidence and the temp are never consumed or deleted.
        assert(readText(dir + "/bank.dat.previous.0") == "BANKMAGIC-older");
        assert(exists(dir + "/bank.dat.tmp.0"));
    }

    // 3. Missing target whose preserved generations do not validate: blocked, never a first run,
    //    and the evidence is left exactly as found.
    {
        const auto dir = freshDir("bad-previous");
        writeText(dir + "/bank.dat.previous.2", "garbage");
        const auto d = P::decide(dir + "/bank.dat", validBank);
        assert(d.start == P::Start::Blocked);
        assert(!d.detail.empty());
        assert(!exists(dir + "/bank.dat"));
        assert(readText(dir + "/bank.dat.previous.2") == "garbage");
    }

    // 4. bank.dat exists but is not a regular file: blocked, untouched.
    {
        const auto dir = freshDir("not-regular");
        assert(::mkdir((dir + "/bank.dat").c_str(), 0700) == 0);
        const auto d = P::decide(dir + "/bank.dat", validBank);
        assert(d.start == P::Start::Blocked);
        struct stat st{};
        assert(::stat((dir + "/bank.dat").c_str(), &st) == 0 && S_ISDIR(st.st_mode));
    }

    // 5. A regular bank.dat is read even if a stale .previous exists; nothing is restored over it.
    {
        const auto dir = freshDir("present");
        writeText(dir + "/bank.dat", "BANKMAGIC-current");
        writeText(dir + "/bank.dat.previous.0", "BANKMAGIC-old");
        const auto d = P::decide(dir + "/bank.dat", validBank);
        assert(d.start == P::Start::ReadExisting);
        assert(!d.restored);
        assert(readText(dir + "/bank.dat") == "BANKMAGIC-current");
        assert(readText(dir + "/bank.dat.previous.0") == "BANKMAGIC-old");
    }

    // 6. Production wiring: Bank::load consults the policy before reading, migrates only on
    //    FirstRun, and an existing file that fails to read is quarantined, not migrated over.
    {
        const auto bank = readText("src/Trainer/Bank.cpp");
        assert(!bank.empty());
        const auto load = bank.find("void Bank::load()");
        assert(load != std::string::npos);
        const auto body = bank.substr(load, bank.find("void Bank::migrateLegacyBanks()") - load);
        const auto decide = body.find("BankLoadPolicy::decide(path, &Bank::validateStorageImage)");
        const auto read = body.find("readAllBytes(path.c_str(), &fileSize)");
        const auto migrate = body.find("migrateLegacyBanks();");
        const auto firstRun = body.find("if (start.start == BankLoadPolicy::Start::FirstRun)");
        assert(decide != std::string::npos && read != std::string::npos);
        assert(decide < read);
        assert(firstRun != std::string::npos && firstRun < migrate && migrate < read);
        assert(body.find("migrateLegacyBanks();", migrate + 1) == std::string::npos);
        const auto nullRead = body.find("if (!file) {", read);
        assert(nullRead != std::string::npos);
        const auto nullBlock = body.substr(nullRead, body.find("return;", nullRead) - nullRead);
        assert(nullBlock.find("abandonFile(") != std::string::npos);
        assert(nullBlock.find("migrateLegacyBanks") == std::string::npos);
        const auto blocked = body.find("if (start.start == BankLoadPolicy::Start::Blocked)");
        assert(blocked != std::string::npos && blocked < read);
        assert(body.find("writeBlocked = true;", blocked) < firstRun);
    }

    std::cout << "Bank load policy (PKSE 620c5f6 parity: unreadable/interrupted bank never a first run): PASS\n";
}
