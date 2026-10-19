// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
// Save-local e-Reader berry context. No global PKHeX-style last-loaded-save state.
// Layout/name policy: PKHeX@6501f0ab SAV3.EBerryName, SaveBlock3Large{RS,E,FRLG},
// EReaderBerrySettings.GetStatus. Native checksum: pret/pokeruby src/berry.c
// GetEnigmaBerryChecksum (zero pointer fields); pokeemerald src/berry.c (raw sum).
// Original source pins and limits are in docs/legality/completion-20261019/SOURCES.md.
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Utils/Gen3Text.h"
#include <optional>
#include <string>
#include <string_view>
namespace Legality {
class Gen3BerryContext {
public:
 enum class Match : uint8_t { Unknown, NoData, Recognized, Unrecognized, WrongRegion };
 static std::optional<Gen3BerryContext> fromSave(std::span<const uint8_t> bytes,
                                               std::string_view exactGame, uint8_t language) {
  namespace G=PokeVault::Integration::Gen3;
  namespace D=G::Detail;
  if(bytes.size()!=D::kSaveSize) return std::nullopt;
  G::SourceGame game;
  if(exactGame=="ruby_gba")game=G::SourceGame::RubyGBA;
  else if(exactGame=="sapphire_gba")game=G::SourceGame::SapphireGBA;
  else if(exactGame=="emerald_gba")game=G::SourceGame::EmeraldGBA;
  else if(exactGame=="firered_gba")game=G::SourceGame::FireRedGBA;
  else if(exactGame=="leafgreen_gba")game=G::SourceGame::LeafGreenGBA;
  else return std::nullopt;
  const D::SlotValidation slots[2]{D::validateSlot(bytes,0),D::validateSlot(bytes,1)};
  if(!slots[0].valid&&!slots[1].valid)return std::nullopt;
  const auto& selected=slots[D::selectActiveSlot(slots)];
  if(!D::familyMatches(game,D::detectFamily(bytes,selected)))return std::nullopt;
  const bool rs=game==G::SourceGame::RubyGBA||game==G::SourceGame::SapphireGBA;
  const size_t offset=rs?0x3160:game==G::SourceGame::EmeraldGBA?0x31F8:0x30EC;
  const size_t length=rs?0x530:0x34;
  // Logical large block is save sections1..4, rotated physically in either slot.
  const auto at=[&](size_t p)->uint8_t {
    return bytes[selected.logicalSectorOffsets[1+p/D::kSectorDataSize]+p%D::kSectorDataSize];
  };
  Gen3BerryContext result;result.game_=exactGame;
  // Unknown charset must not be guessed from a record's origin language.
  if(language<1||language>7||language==6)return result;
  if(at(offset)==0||at(offset)==0xFF){result.match_=Match::NoData;return result;}
  uint32_t calculated=0,stored=0;
  for(size_t i=0;i<length-4;++i)if(!rs||i<0xC||i>=0x14)calculated+=at(offset+i);
  for(size_t i=0;i<4;++i)stored|=uint32_t{at(offset+length-4+i)}<<(8*i);
  // Bad source substructure is missing evidence, not a violation by the Pokemon.
  if(calculated!=stored||at(offset+0x14)==0||at(offset+0xA)==0)return result;
  std::u16string name;
  for(size_t i=0;i<7;++i){const uint8_t b=at(offset+i);if(b==0xFF)break;name+=Utils::gen3ToChar(b,language);}
  const auto usa=name==u"PUMKIN"||name==u"DRASH"||name==u"EGGANT"||name==u"STRIB"||name==u"CHILAN"||name==u"NUTPEA";
  const auto jpn=name==u"カチャ"||name==u"ブ－カ"||name==u"ビスナ"||name==u"エドマ"||name==u"ホズ"||name==u"ラッカ"||
                 name==u"ギネマ"||name==u"クオ"||name==u"ヤゴ"||name==u"トウガ"||name==u"ニニク"||name==u"トポ";
  result.match_=usa?(language==1?Match::WrongRegion:Match::Recognized)
                    :jpn?(language==1?Match::Recognized:Match::WrongRegion):Match::Unrecognized;
  return result;
 }
 Match match(std::string_view exactGame) const noexcept {return exactGame==game_?match_:Match::Unknown;}
private:
 Match match_=Match::Unknown;
 std::string game_;
};
} // namespace Legality
