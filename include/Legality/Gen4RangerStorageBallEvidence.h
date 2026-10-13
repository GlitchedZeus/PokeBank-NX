#pragma once
// L4 Ranger native read-only boundary: a PK4 has two different Ball fields.
// The active STORAGE layout, not the larger raw byte, selects the field.
// Selecting a native field never authenticates a Ranger source or receipt.
#include <cstdint>
namespace Legality::Gen4RangerStorageBallEvidence {
enum class Storage : uint8_t { Unknown, DPPt, HGSS };
struct Result {
    bool storageKnown=false;
    uint8_t ball=0;
};
constexpr Result select(Storage storage, uint8_t dppt, uint8_t hgss) noexcept {
    switch (storage) {
        case Storage::DPPt: return {true, dppt};
        case Storage::HGSS: return {true, hgss};
        case Storage::Unknown: return {};
    }
    return {}; // Out-of-domain source tags cannot select an arbitrary field.
}
static_assert(select(Storage::DPPt,4,16).ball==4);
static_assert(select(Storage::HGSS,16,4).ball==4);
static_assert(!select(Storage::Unknown,4,16).storageKnown);
static_assert(!select(static_cast<Storage>(255),4,16).storageKnown);
}
