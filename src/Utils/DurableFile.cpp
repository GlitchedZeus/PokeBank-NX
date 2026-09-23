#include "Utils/DurableFile.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace PokeBank::Storage::DurableFile {
namespace {

std::string uniqueSibling(const std::string& target, const char* suffix) {
    for (uint64_t generation = 0; generation < 1000000; ++generation) {
        const std::string candidate =
            target + suffix + std::to_string(generation);
        struct stat st{};
        errno = 0;
        if (stat(candidate.c_str(), &st) != 0) {
            if (errno == ENOENT) return candidate;
            return {};
        }
    }
    return {};
}

bool readExact(const std::string& path, std::vector<uint8_t>& out, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) {
        error = "open for verification failed: " + std::string(std::strerror(errno));
        return false;
    }
    if (std::fseek(f, 0, SEEK_END) != 0) {
        error = "seek failed";
        std::fclose(f);
        return false;
    }
    const long size = std::ftell(f);
    if (size < 0 || std::fseek(f, 0, SEEK_SET) != 0) {
        error = "size/rewind failed";
        std::fclose(f);
        return false;
    }
    out.assign(static_cast<size_t>(size), 0);
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool closeOk = std::fclose(f) == 0;
    if (got != out.size()) {
        error = "verification read was short";
        return false;
    }
    if (!closeOk) {
        error = "verification close failed";
        return false;
    }
    return true;
}

bool writeTemp(const std::string& path, std::span<const uint8_t> bytes, std::string& error) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        error = "temp open failed: " + std::string(std::strerror(errno));
        return false;
    }

    const size_t written = bytes.empty() ? 0 : std::fwrite(bytes.data(), 1, bytes.size(), f);
    bool ok = written == bytes.size();
    if (!ok) error = "temp write was short";

    if (ok && std::fflush(f) != 0) {
        ok = false;
        error = "temp fflush failed";
    }
    if (ok && ::fsync(fileno(f)) != 0) {
        ok = false;
        error = "temp fsync failed";
    }
    if (std::fclose(f) != 0 && ok) {
        ok = false;
        error = "temp fclose failed";
    }
    return ok;
}

bool bytesEqual(std::span<const uint8_t> expected, const std::vector<uint8_t>& actual) {
    return expected.size() == actual.size() &&
           std::equal(expected.begin(), expected.end(), actual.begin());
}

} // namespace

Result replace(const std::string& target,
               std::span<const uint8_t> bytes,
               const Validator& validator) {
    Result result;

    const std::string temp = uniqueSibling(target, ".tmp.");
    if (temp.empty()) {
        result.error = "could not allocate a unique temp generation";
        return result;
    }

    if (!writeTemp(temp, bytes, result.error)) {
        std::remove(temp.c_str());
        return result;
    }

    std::vector<uint8_t> reread;
    if (!readExact(temp, reread, result.error) || !bytesEqual(bytes, reread)) {
        if (result.error.empty()) result.error = "temp reread differs from requested bytes";
        result.failedPath = uniqueSibling(target, ".failed.");
        if (!result.failedPath.empty())
            std::rename(temp.c_str(), result.failedPath.c_str());
        else
            std::remove(temp.c_str());
        return result;
    }

    std::string validationError;
    if (!validator(std::span<const uint8_t>(reread.data(), reread.size()), validationError)) {
        result.error = validationError.empty() ? "temp validation failed" : validationError;
        result.failedPath = uniqueSibling(target, ".failed.");
        if (!result.failedPath.empty())
            std::rename(temp.c_str(), result.failedPath.c_str());
        else
            std::remove(temp.c_str());
        return result;
    }

    struct stat current{};
    errno = 0;
    const bool hadTarget = stat(target.c_str(), &current) == 0;
    if (!hadTarget && errno != ENOENT) {
        result.error = "could not inspect current target";
        std::remove(temp.c_str());
        return result;
    }

    if (hadTarget) {
        result.previousPath = uniqueSibling(target, ".previous.");
        if (result.previousPath.empty()) {
            result.error = "could not allocate a unique previous generation";
            std::remove(temp.c_str());
            return result;
        }
        if (std::rename(target.c_str(), result.previousPath.c_str()) != 0) {
            result.error = "could not preserve current target before promotion";
            std::remove(temp.c_str());
            result.previousPath.clear();
            return result;
        }
    }

    if (std::rename(temp.c_str(), target.c_str()) != 0) {
        result.error = "temp promotion failed";
        if (hadTarget)
            std::rename(result.previousPath.c_str(), target.c_str());
        return result;
    }

    reread.clear();
    validationError.clear();
    const bool promotedReadOk = readExact(target, reread, result.error);
    const bool promotedBytesOk = promotedReadOk && bytesEqual(bytes, reread);
    const bool promotedValidationOk =
        promotedBytesOk &&
        validator(std::span<const uint8_t>(reread.data(), reread.size()), validationError);

    if (!promotedReadOk || !promotedBytesOk || !promotedValidationOk) {
        if (result.error.empty()) {
            if (!promotedBytesOk) result.error = "promoted target differs from requested bytes";
            else result.error = validationError.empty() ? "promoted validation failed" : validationError;
        }

        result.failedPath = uniqueSibling(target, ".failed.");
        bool failedPreserved = false;
        if (!result.failedPath.empty())
            failedPreserved = std::rename(target.c_str(), result.failedPath.c_str()) == 0;

        if (!failedPreserved) {
            // Never overwrite a questionable promoted target just to make rollback look successful.
            result.error += "; failed promoted target could not be preserved";
            return result;
        }

        if (hadTarget && std::rename(result.previousPath.c_str(), target.c_str()) != 0) {
            result.error += "; previous generation could not be restored";
            return result;
        }
        return result;
    }

    result.ok = true;
    return result;
}

} // namespace PokeBank::Storage::DurableFile
