#pragma once
// Original-field availability audit of ALL 247 pinned Gen IV Pokémon WC4
// projections. "Projected" means structural fields are in the generated
// database, NOT that the retail wondercard/distribution is authenticated.
// No PCD, PGT, Pokemon binary or source event table is embedded here.
#include "Legality/Gen4EventTemplate.h"
#include "Legality/Gen4EventArchiveAttestation.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4EventTemplateAudit {
namespace Archive=Gen4EventArchiveAttestation;
inline constexpr std::size_t kCount=Gen4EventTemplate::kGen4EventTemplateCount;
struct Row {
    std::size_t projectionIndex = 0;
    uint16_t cardId = 0;
    uint16_t species = 0;
    uint16_t rowsSharingCardId = 0;
    Archive::ArchiveGrade archive = Archive::ArchiveGrade::Unverified;
    std::array<Archive::FieldOrigin,3> originalFields{};
};
inline constexpr auto kRows=[] {
    std::array<Row,kCount> rows{};
    for (std::size_t i=0;i<kCount;++i) {
        const auto& t=Gen4EventTemplate::kGen4EventTemplates[i];
        uint16_t count=0;
        for(const auto& other:Gen4EventTemplate::kGen4EventTemplates)
            if(other.cardId==t.cardId)++count;
        rows[i]={i,t.cardId,t.species,count,
                 Archive::ArchiveGrade::Unverified,
                 {Archive::FieldOrigin::Unavailable,
                  Archive::FieldOrigin::Unavailable,
                  Archive::FieldOrigin::Unavailable}};
    }
    return rows;
}();
constexpr std::size_t reusedIdRows() noexcept {
    std::size_t count=0;
    for(const auto& row:kRows)if(row.rowsSharingCardId>1)++count;
    return count;
}
constexpr std::size_t distinctCardIds() noexcept {
    std::size_t count=0;
    for(std::size_t i=0;i<kCount;++i) {
        bool earlier=false;
        for(std::size_t j=0;j<i;++j)
            if(kRows[j].cardId==kRows[i].cardId)earlier=true;
        if(!earlier)++count;
    }
    return count;
}
// Explicit dataset revision sentinel. An upstream regeneration must revisit
// this audit before any "source verified" field may be asserted.
static_assert(kCount==247);
static_assert(distinctCardIds()==70);
static_assert(reusedIdRows()==219);
static_assert(kRows[0].cardId==67 && kRows[0].species==4);
static_assert(kRows[0].rowsSharingCardId==1);
static_assert(kRows[4].rowsSharingCardId==3);
static_assert(kRows[0].originalFields[0]==Archive::FieldOrigin::Unavailable);
} // namespace Legality::Gen4EventTemplateAudit
