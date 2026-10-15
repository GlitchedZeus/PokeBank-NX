// PokeBank NX application QA: Gen I-V cross-generation acceptance matrix (Thread 4, QA-owned).
//
// Every cell is driven through the REAL production entry points (discovery, Save Instance
// identity, legacy bindings, read-only parsers, staged editors / workspaces, verified working-copy
// export, Home snapshot paths) on SYNTHETIC saves only. A cell that needs the Switch (rendering,
// HID, HOME launch, emulator process) is recorded as HardwareOnly with the reason; a cell for a
// feature the production code deliberately does not offer for that version is Unsupported with the
// reason. Known defects are recorded as Known with their QA-F id: their strict, failing repro lives
// in tests/test_qa_known_failures.cpp, never here.
//
// Facts are short normalised observations (for example "home.party.form" = "kept"/"dropped") that
// the runner compares across versions to flag inconsistent cross-generation behaviour.
#pragma once

#include <array>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace qamatrix {

enum class Status { Pass, Fail, Known, HardwareOnly, Unsupported };

// Column order of docs/qa/GEN1_5_MATRIX.md.
inline constexpr std::array<std::string_view, 21> kCells = {
    "discovery",      // the version's production discovery / inspection accepts the synthetic save
    "linking",        // LegacySourceBindings assignment round-trips and resolves to the same file
    "provider",       // SaveProviderIdentity / provider label for the source
    "identity",       // exact game/version id (never guessed from the filename)
    "trainer-name",   // trainer name decoded
    "trainer-gender", // trainer gender (or "not stored" for the version)
    "party",          // party count and members
    "boxes",          // box storage
    "pokedex",        // seen/caught/total
    "bag",            // inventory read model
    "navigation",     // shared editor navigation model; focus moves never mutate
    "creation",       // Create defaults and validation
    "fields",         // every editable field stages and round-trips
    "pickers",        // species / move picker data
    "actions",        // Actions menu model
    "working-copy",   // staging never touches the source; the working copy carries the edit
    "review",         // review model lists staged changes
    "export",         // verified export: strict reparse and byte whitelist
    "immutability",   // source bytes and mtime unchanged after every step
    "home-refresh",   // Home snapshot refresh after the file changes
    "relaunch",       // a fresh process (new cache / bindings reload) sees the current file
};

struct Entry {
    std::string version;
    std::string cell;
    Status status;
    std::string note;
};

class Matrix {
public:
    void set(std::string version, std::string_view cell, Status status, std::string note = {}) {
        entries_.push_back({std::move(version), std::string(cell), status, std::move(note)});
    }
    void pass(std::string version, std::string_view cell, std::string note = {}) {
        set(std::move(version), cell, Status::Pass, std::move(note));
    }
    void fail(std::string version, std::string_view cell, std::string note) {
        set(std::move(version), cell, Status::Fail, std::move(note));
    }
    // Records a cell from a boolean outcome: Pass when ok, otherwise Fail with the reason.
    void check(std::string version, std::string_view cell, bool ok, std::string failNote,
               std::string passNote = {}) {
        if (ok) pass(std::move(version), cell, std::move(passNote));
        else fail(std::move(version), cell, std::move(failNote));
    }
    void hardwareOnly(std::string version, std::string_view cell, std::string reason) {
        set(std::move(version), cell, Status::HardwareOnly, std::move(reason));
    }
    void unsupported(std::string version, std::string_view cell, std::string reason) {
        set(std::move(version), cell, Status::Unsupported, std::move(reason));
    }
    void known(std::string version, std::string_view cell, std::string findingAndNote) {
        set(std::move(version), cell, Status::Known, std::move(findingAndNote));
    }
    void fact(const std::string& version, const std::string& key, std::string value) {
        facts_[key][version] = std::move(value);
    }
    const std::vector<Entry>& entries() const noexcept { return entries_; }
    const std::map<std::string, std::map<std::string, std::string>>& facts() const noexcept { return facts_; }

private:
    std::vector<Entry> entries_;
    std::map<std::string, std::map<std::string, std::string>> facts_;
};

// Version ids, in matrix row order (20 Gen I-V versions).
inline constexpr std::array<std::string_view, 20> kVersions = {
    "red", "blue", "yellow", "gold", "silver", "crystal",
    "ruby", "sapphire", "emerald", "firered", "leafgreen",
    "diamond", "pearl", "platinum", "heartgold", "soulsilver",
    "black", "white", "black2", "white2",
};

void runGen12(Matrix& m);
void runGen3(Matrix& m);
void runGen4(Matrix& m);
void runGen5(Matrix& m);

} // namespace qamatrix
