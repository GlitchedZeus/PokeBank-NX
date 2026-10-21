// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Legality/Legality.h"
#include <utility>
namespace Legality::Presentation {
struct Line { std::string text; Severity severity; };
inline const char* coverageLabel(CoverageLevel value) noexcept {
 switch(value){case CoverageLevel::Complete:return "Complete";case CoverageLevel::Partial:return "Partial";case CoverageLevel::None:return "None";}
 return "None";
}
inline std::string headline(const Report& report) {
 if(report.verdict()==Verdict::NoProblemsFound)return "No problems found by complete checks.";
 if(report.verdict()==Verdict::Invalid){size_t n=0;for(const auto&i:report.issues)n+=i.severity==Severity::Invalid;return "Invalid: "+std::to_string(n)+" issue(s).";}
 size_t n=0;for(const auto&i:report.issues)n+=i.severity==Severity::Warning;
 return n ? "Incomplete / Unknown: "+std::to_string(n)+" warning(s)."
          : "Incomplete / Unknown: required checks or evidence are unresolved.";
}
inline std::vector<std::string> coverageRows(const Report& report) {
 const auto& c=report.coverage;
 const std::pair<const char*,CoverageLevel> areas[]={
  {"structure",c.structure},{"source game",c.sourceGame},{"internal",c.internal},
  {"moves",c.moves},{"encounter",c.encounter},{"event/gift",c.eventGift},
  {"PID/RNG",c.pidRng},{"egg/breeding",c.eggBreeding},{"transfer",c.transfer}};
 std::vector<std::string> rows;rows.reserve(9);
 for(const auto&[name,level]:areas)rows.push_back(std::string(name)+": "+coverageLabel(level));
 return rows;
}
inline std::vector<Line> lines(const Report& report) {
 const auto verdict=report.verdict();
 std::vector<Line> result{{headline(report),verdict==Verdict::Invalid?Severity::Invalid:
                            verdict==Verdict::Incomplete?Severity::Warning:Severity::Info}};
 if(verdict==Verdict::Incomplete)for(auto& text:coverageRows(report))result.push_back({std::move(text),Severity::Warning});
 for(const auto&i:report.issues)if(i.severity!=Severity::Info)
  result.push_back({std::string(i.severity==Severity::Invalid?"Invalid: ":"Warning: ")+i.text,i.severity});
 return result;
}
} // namespace Legality::Presentation
