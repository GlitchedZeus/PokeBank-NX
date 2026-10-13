#pragma once
// SPDX-License-Identifier: GPL-3.0-or-later
// Detached Ultra Sun/Moon PK7 storage/party layout using the *shared* native
// PokeCrypto.Decrypt67 shim, not a new or duplicated format decoder.
// Source: owner archive pinned PKHeX.Core/PKM/PK7.cs and PokeCrypto.cs.
// PK7 is not PB7 (Let's Go); a caller must authenticate the record family
// separately. No source history, origin or save-write authority is implied.
#include "Legality/Gen6NativePK6Evidence.h"
#include <cstdint>
#include <span>
namespace Legality::Gen7NativePK7 {
inline constexpr std::uint16_t SpeciesCeiling=807; // USUM, not PB7 LGPE
using Status=Gen6NativePK6::Status;
using Layout=Gen6NativePK6::Layout;
using Evidence=Gen6NativePK6::Evidence;
inline Evidence inspect(std::span<const std::uint8_t> source,
                        bool encrypted) noexcept {
    return Gen6NativePK6::inspectWithSpeciesCeiling(source,encrypted,SpeciesCeiling);
}
static_assert(Gen6NativePK6::StoredBytes==232 && Gen6NativePK6::PartyBytes==260);
} // namespace Legality::Gen7NativePK7
