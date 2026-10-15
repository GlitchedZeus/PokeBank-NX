// PokeBank NX application QA: reproductions of OPEN defects (Thread 4, QA-owned).
//
// NOT part of host-test / host-sanitize / `make qa`: each case FAILS (non-zero exit) while its defect
// is open, so it can be ported verbatim into the owning thread's suite as the regression test for
// the fix. When the fix lands, the case moves to test_qa_regressions.cpp. Run:
//   make -f Makefile.host qa-known-failures
// Details: docs/qa/FINDINGS.md.
#include "test_qa_support.h"

#include "Encryption/Encryption5.h"
#include "Integration/Gen5/Gen5PersonalData.h"
#include "Integration/Gen5/Gen5PidCoordination.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "fuzz_synthetic_saves.h"

#include <iostream>

using namespace qacontract;
namespace G5 = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;

namespace {

// QA-F4 (Low, Thread 2R: include/Integration/Gen5/Gen5StagedPokemonRecord.h, Field::Personality).
// The PID replay guard treats "gender link" and "ability link" as ONE condition: a record is
// protected only when BOTH already hold. A Gen V origin record whose stored gender matches its PID
// but whose ability byte does not match its PID ability slot therefore gets no gender protection,
// and a replayed PID may silently desynchronise stored gender from the PID (PKHeX
// EntityGender.GetFromPID). Found by fuzz_gen5_edits (libFuzzer, 2026-10-15).
bool personalityReplayKeepsGenderLinkIndependently() {
    currentCase() = "QA-F4 PID replay must keep an existing gender link on its own";
    constexpr uint16_t eevee = 133;               // gender ratio 31 (variable)
    constexpr uint32_t pid = 0x11220090u;          // low byte 0x90 >= 31 -> male (0)
    qacontract::Bytes plain(C::StoredSize, 0);
    qasynth::w32(plain, 0x00, pid);
    qasynth::w16(plain, 0x08, eevee);
    qasynth::w16(plain, 0x0C, 12345);
    qasynth::w16(plain, 0x0E, 54321);
    qasynth::w32(plain, 0x10, 1000);
    plain[0x15] = 9;                               // ability byte NOT Eevee's slot ability
    plain[0x40] = 0;                               // stored gender male, matching the PID
    plain[0x5F] = 21;                              // Black: a Gen V origin
    plain[0x84] = 0x80 | 5;
    const auto encrypted = C::encryptCandidate(plain);
    const auto personal = G5::personal5(eevee, 0, false);
    QA_EXPECT(personal.info && G5::variableGenderRatio5(personal.info->genderRatio), "fixture species");
    QA_EXPECT(G5::genderFromPid5(pid, personal.info->genderRatio) == 0, "fixture gender is PID-linked");

    std::string error;
    auto record = G5::StagedPokemon5Record::create(encrypted, &error, false);
    QA_EXPECT(record.has_value(), "fixture record opens: " + error);
    if (!record) return false;
    const uint32_t breaking = 0x0009FE00u;         // low byte 0x00 < 31 -> female
    const bool accepted = record->stage(G5::StagedPokemon5Record::Field::Personality, 0, breaking, &error);
    bool linkKept = true;
    if (accepted) {
        const auto staged = C::decrypt(record->stagedBytes());
        const uint8_t storedGender = static_cast<uint8_t>((staged[0x40] >> 1) & 3);
        linkKept = G5::genderFromPid5(C::read32(staged, 0), personal.info->genderRatio) == storedGender;
    }
    QA_EXPECT(!accepted || linkKept,
              "a replayed PID was accepted although it breaks the record's existing PID/gender link");
    return !accepted || linkKept;
}

} // namespace

int main() {
    std::cout << "Known open defects (each line FAILS until fixed):\n";
    const bool f4 = personalityReplayKeepsGenderLinkIndependently();
    std::cout << "  QA-F4 Gen V PID replay keeps an existing gender link: " << (f4 ? "fixed" : "OPEN") << "\n";
    if (failures() != 0) {
        std::cerr << failures() << " known defect(s) still reproduce\n";
        return 1;
    }
    std::cout << "No known C++ defect reproduces any more: move these cases into test_qa_regressions.cpp.\n";
    return 0;
}
