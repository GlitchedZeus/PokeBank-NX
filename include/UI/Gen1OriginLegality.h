#pragma once

// Gen1OriginLegality.h - E03: the Gen I report for the ONE shared Origin & Legality panel, used by
// every Gen I surface (box Actions, the Party popup and the legacy editor overlay). A Gen I record
// stores no origin game, met location, met level or met date and has no per-record checksum; the
// one guardrail is the exact-game move compatibility check (pinned pret pokered learnsets).

#include "Integration/Gen1/Gen1MoveCompatibility.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Names/MoveNames.h"
#include "UI/ClassicGameContext.h"
#include "UI/SharedOriginLegality.h"

#include <optional>
#include <string>
#include <string_view>

namespace PokeBank::UIModel::Gen1OriginLegality {

namespace Gen1 = PokeVault::Integration::Gen1;
namespace OL = PokeBank::UIModel::OriginLegality;

inline std::optional<Gen1::SourceGame> sourceGame(std::string_view id) noexcept {
    if (id == "red_gb") return Gen1::SourceGame::Red;
    if (id == "blue_gb") return Gen1::SourceGame::Blue;
    if (id == "yellow_gb") return Gen1::SourceGame::Yellow;
    return std::nullopt;
}

inline OL::Report emptyReport(std::string_view sourceGameId) {
    OL::Report report;
    report.exactGame = std::string(ClassicGameContext::exactGameName(sourceGameId));
    report.format = "Gen I record";
    report.structure = OL::Structure::NotChecked;
    report.originGame = OL::notStored();
    report.metLocation = OL::notStored();
    report.metLevel = OL::notStored();
    report.metDate = OL::notStored();
    return report;
}

inline OL::Report report(std::string_view sourceGameId, const Gen1::PokemonRecord& p) {
    auto out = emptyReport(sourceGameId);
    const auto game = sourceGame(sourceGameId);
    for (const auto move : p.moves) {
        if (move == 0) {
            out.moves.push_back({"Empty", "Empty", OL::Tone::Dim});
            continue;
        }
        const bool ok = game && Gen1::MoveCompatibility::canLearnMove(*game, p.species, move);
        out.moves.push_back({std::string(Names::getMoveName(move)), ok ? "OK" : "Unusual (preserved)",
                             ok ? OL::Tone::Good : OL::Tone::Caution});
    }
    return out;
}

} // namespace PokeBank::UIModel::Gen1OriginLegality
