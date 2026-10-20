#pragma once

// SharedOriginLegality.h - E03 (Round 3 hardware): ONE Origin & Legality report for every Gen I-V
// game. Before, each generation drew its own popup (Gen I "not checked", Gen II party View, Gen III
// a fixed list of claims, Gen IV structure + moves, Gen V met data) and the wording disagreed.
//
// Truthfulness rules (owner: "do not fabricate clean verdicts"; the full legality engine, PR #314,
// is NOT integrated):
//   * Legality is "Not checked" in every generation: no legality evaluation runs anywhere in
//     this build. Editor guardrails (structure, move compatibility) are shown as their own rows
//     and are never promoted to a verdict; Verified / Problems found are unreachable.
//   * The encounter check is always "Unknown": no encounter-legality engine runs here.
//   * A field the native format does not store reads "Not stored in <format>", and a stored field
//     this editor does not decode reads "Not read by this editor" -- never a placeholder value such
//     as 0 / None that could be mistaken for data.
//   * Structure is "Not checked" unless the adapter really validated the record.
// The rows are a fixed order so every generation shows the same layout; adapters only fill data.

#include "UI/SharedEditorPresentation.h"

#include <string>
#include <utility>
#include <vector>

namespace PokeBank::UIModel::OriginLegality {

namespace Presentation = PokeBank::UIModel::SharedEditorPresentation;

enum class Tone : unsigned char { Plain, Dim, Good, Caution, Locked };
enum class Structure : unsigned char { NotChecked, Valid, Invalid };

struct Row {
    std::string label;
    std::string value;
    Tone tone = Tone::Plain;
};

// One move slot's editor guardrail result ("OK", "Transfer", "Preserved", "Invalid"...), only when
// the generation really classifies moves for this record.
struct MoveCheck {
    std::string move;
    std::string status;
    Tone tone = Tone::Plain;
};

// One origin field: a decoded value, a field the format never stores, or one not decoded here.
struct Field {
    enum class Kind : unsigned char { NotRead, NotStored, Value };
    Kind kind = Kind::NotRead;
    std::string text;
};
inline Field value(std::string text) { return {Field::Kind::Value, std::move(text)}; }
inline Field notStored() { return {Field::Kind::NotStored, {}}; }
inline Field notRead() { return {Field::Kind::NotRead, {}}; }

struct Report {
    std::string exactGame;      // the selected source save's exact title
    std::string format;         // "Gen I record", "Gen II record", "PK3", "PK4", "PK5"
    Structure structure = Structure::NotChecked;
    Field originGame;
    Field metLocation;
    Field metLevel;
    Field metDate;
    std::vector<MoveCheck> moves;
};

// The one legality state for every Gen I-V surface (panel and View / Edit data panes alike).
inline constexpr Presentation::LegalityState kLegality = Presentation::LegalityState::NotChecked;
static_assert(kLegality != Presentation::LegalityState::Verified &&
              kLegality != Presentation::LegalityState::ProblemsFound,
              "Origin & Legality never reports a verdict it did not compute");

inline constexpr const char* kSubtitle = "Stored origin and editor guardrails only \xE2\x80\xA2 no legality verdict";
inline constexpr const char* kEncounterCheck = "Unknown (not evaluated)";
inline constexpr const char* kSourceMutation = "Hard disabled (staged copy only)";

inline constexpr const char* kNotRead = "Not read by this editor";
inline std::string notStoredText(const Report& r) { return "Not stored in " + r.format; }

// The fixed row order every generation draws.
inline std::vector<Row> rows(const Report& r) {
    std::vector<Row> out;
    out.push_back({"Exact source", r.exactGame, Tone::Plain});
    switch (r.structure) {
        case Structure::Valid: out.push_back({"Record structure", r.format + " valid", Tone::Good}); break;
        case Structure::Invalid: out.push_back({"Record structure", r.format + " invalid", Tone::Caution}); break;
        case Structure::NotChecked: out.push_back({"Record structure", "Not checked", Tone::Dim}); break;
    }
    out.push_back({"Source mutation", kSourceMutation, Tone::Locked});
    const auto field = [&](const char* label, const Field& f) {
        switch (f.kind) {
            case Field::Kind::Value: out.push_back({label, f.text, Tone::Plain}); break;
            case Field::Kind::NotStored: out.push_back({label, notStoredText(r), Tone::Dim}); break;
            case Field::Kind::NotRead: out.push_back({label, kNotRead, Tone::Dim}); break;
        }
    };
    field("Origin game", r.originGame);
    field("Met location", r.metLocation);
    field("Met level", r.metLevel);
    field("Met date", r.metDate);
    out.push_back({"Encounter check", kEncounterCheck, Tone::Caution});
    out.push_back({"Legality", std::string(Presentation::legalityLabel(kLegality)) + " (legality checker not integrated)",
                   Tone::Caution});
    return out;
}

inline constexpr int kFixedRows = 9;  // every generation, before optional move rows

} // namespace PokeBank::UIModel::OriginLegality
