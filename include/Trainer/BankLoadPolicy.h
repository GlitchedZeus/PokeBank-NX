#ifndef TRAINER_BANK_LOAD_POLICY_H
#define TRAINER_BANK_LOAD_POLICY_H

// Decides how Bank::load may start, before any byte of bank.dat is trusted.
//
// Upstream PKSE 620c5f6 ("bank file safety") found that a bank.dat which exists but cannot be
// read was treated as a first run. PokeBank NX had the same gap: readAllBytes() returning null
// for any reason ran the one-time legacy migration and opened an empty Bank, and a power cut in
// DurableFile::replace between "target -> .previous.N" and "temp -> target" left no bank.dat at
// all, which also read as a first run. Only a target that is genuinely absent, with no preserved
// generation, may be a first run.

#include <cerrno>
#include <string>
#include <sys/stat.h>

#include "Utils/DurableFile.h"

namespace Trainer::BankLoadPolicy {

enum class Start {
    FirstRun,       // no bank.dat and no preserved generation: legacy migration is allowed
    ReadExisting,   // a regular bank.dat is in place (possibly just restored); read and parse it
    Blocked,        // cannot tell what is there: open empty, never migrate, never write
};

struct Decision {
    Start start = Start::Blocked;
    bool restored = false;       // a validated .previous.N generation was put back as bank.dat
    std::string detail;
};

inline Decision decide(const std::string& path,
                       const PokeBank::Storage::DurableFile::Validator& validator) {
    Decision out;
    struct stat st{};
    errno = 0;
    if (::stat(path.c_str(), &st) == 0) {
        if (S_ISREG(st.st_mode)) {
            out.start = Start::ReadExisting;
        } else {
            out.detail = "Bank path exists but is not a regular file; original left untouched.";
        }
        return out;
    }
    if (errno != ENOENT) {
        out.detail = "Bank file could not be inspected; original left untouched.";
        return out;
    }

    const auto recovery = PokeBank::Storage::DurableFile::recoverMissingTarget(path, validator);
    if (!recovery.ok) {
        // Preserved generations exist but none validates (or the folder cannot be listed). They
        // are recovery evidence: an empty first-run Bank saved over them would hide them.
        out.detail = "Bank file is missing and its preserved copies could not be verified; "
                     "nothing was migrated or overwritten.";
        return out;
    }
    if (recovery.restored) {
        out.start = Start::ReadExisting;
        out.restored = true;
        out.detail = "Bank restored from " + recovery.restoredFrom;
        return out;
    }
    out.start = Start::FirstRun;
    return out;
}

}  // namespace Trainer::BankLoadPolicy

#endif
