#pragma once

#include <string>

// Round 14 R08 / owner regression list ("Gen V save controls ... editor footer interactions and
// save-export messages"): the ONE wording every generation (Gen I-V) shows for the result of a
// verified staged export (Review X Export Copy, the Unsaved Changes X Export Copy, the Gen V export
// confirm, the Gen I-III Bag Options export). Generation code supplies only the location or the
// specific error; it never words the result itself. The original save is never written by an export,
// and every message says so.
namespace PokeBank::UIModel::ExportMessages {

inline constexpr const char* kNothingToExport = "No staged changes to export";

[[nodiscard]] inline std::string exported(const std::string& location) {
    return "Verified copy exported: " + location + " (original save unchanged)";
}

// `detail` is the specific refusal (a changed source, a failed reparse, a folder error); empty when
// the exporter gave none.
[[nodiscard]] inline std::string failed(const std::string& detail) {
    return detail.empty() ? std::string("Export failed (original save unchanged)")
                          : "Export failed: " + detail + " (original save unchanged)";
}

} // namespace PokeBank::UIModel::ExportMessages
