#pragma once
// Standalone native Pokemon files for Gen I-V (owner Oct 28, Feature A).
//
// Ports the format rules of the owner's PKSE PR #128 (kiasta/PKSE 27a81c9) and its follow-ups
// fa06a1b (PKHeX plaintext convention), aa4b72e (Gen I box records with no party-level cache),
// 18c2555 (plaintext first, content checked against the extension) and 620c5f6 (.ekN import), onto
// PokeBank NX's own crypto. PKSE is not a runtime dependency. AGPL-3.0 like the rest of the app.
//
// Formats (PKHeX 8e3eb3d4 PokeCrypto.cs:15-23, EntityFormat.cs:68-77):
//   .pk1  single-entry list, 69 bytes (international) / 59 (Japanese): count 1, species, 0xFF,
//         44-byte party body, OT, nickname.
//   .pk2  same shape, 73 / 63 bytes, 48-byte party body.
//   .pk3  80 (stored) / 100 (party);  .ek3 the same sizes encrypted.
//   .pk4  136 / 236;                  .ek4 encrypted.
//   .pk5  136 / 220;                  .ek5 encrypted.
//
// Encryption is decided by CONTENT, never by the extension alone: a record must pass its native
// checksum in exactly one reading (plain or encrypted). When both readings pass (about 1 in 65536),
// PKHeX's own predicate breaks the tie (IsEncrypted3: plain checksum mismatch; IsEncrypted45: u32 at
// 0x64 != 0) and the extension must agree, otherwise the file is refused as ambiguous. A Gen IV/V
// stored record whose content says the other generation (PKHeX GetFormat45) is refused. Nothing here
// invents data, converts between generations or touches a save: it returns the native plaintext
// record and what it is.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeBank::PokemonFile {

enum class Format : uint8_t { PK1, PK2, PK3, PK4, PK5 };

struct Decoded {
    Format format = Format::PK1;
    int generation = 1;
    bool party = false;        // Gen III-V: party-sized record. Gen I/II lists always carry a party body.
    bool japanese = false;     // Gen I/II only: Japanese-length list
    bool wasEncrypted = false; // the file held the encrypted (.ekN) form
    std::vector<uint8_t> plain;  // PKHeX plaintext layout: decrypted, substructures in canonical order
};

// Supported extensions, lower case: .pk1 .pk2 .pk3 .pk4 .pk5 .ek3 .ek4 .ek5.
[[nodiscard]] bool supportedFileName(std::string_view fileName) noexcept;

// Parse one file's bytes. On refusal returns nullopt and a short reason in `error`.
[[nodiscard]] std::optional<Decoded> decode(std::span<const uint8_t> bytes, std::string_view fileName,
                                            std::string& error);

// The plain extension PokeBank NX writes for a record of this generation (".pk1" ... ".pk5").
[[nodiscard]] std::string_view plainExtension(int generation) noexcept;

}  // namespace PokeBank::PokemonFile
