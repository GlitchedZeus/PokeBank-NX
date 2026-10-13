// Native RBY legality/source table must be a subset of the EXISTING editor
// union picker; a picker bit does not prove a specific machine receipt.
#include "Legality/Gen1OriginalMachineTables.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#ifdef NDEBUG
#error Assertions are mandatory for native original-source parity tests
#endif
namespace EditorData {
#include "../src/Integration/Gen1/Gen1MoveCompatibilityData.inc"
}
namespace Source=Legality::Gen1OriginalMachineTables;
static_assert(Source::kRB.size()==152 && Source::kYellow.size()==152);
static_assert(Source::kMoves.size()==56);
int main(){
  std::uint32_t allowedCount=0,editorExtra=0,queries=0;
  for(unsigned g=1;g<=3;++g)for(unsigned species=1;species<=151;++species){
    const auto source=g==3?Source::kYellow[species]:Source::kRB[species];
    const std::uint64_t* words=EditorData::kCompatibleMovesRB[species];
    if(g==3)for(const auto& row:EditorData::kCompatibleMovesYOverrides)
      if(row.species==species){words=row.words;break;}
    assert(words!=nullptr);
    for(unsigned machine=1;machine<=55;++machine){
      const auto move=Source::kMoves[machine];
      assert(move>=1&&move<=165);
      const bool allowed=(source&(std::uint64_t{1}<<(machine-1)))!=0;
      const bool offered=(words[move>>6]&(std::uint64_t{1}<<(move&63)))!=0;
      const bool hm=machine>50;
      const auto n=static_cast<std::uint8_t>(hm?machine-50:machine);
      assert(Source::moveFor(static_cast<std::uint8_t>(g),
              static_cast<std::uint16_t>(species),hm,n)==(allowed?move:0));
      if(allowed){assert(offered);++allowedCount;}
      else if(offered)++editorExtra; // Level-up/other move, not TM receipt
      ++queries;
    }
  }
  assert(queries==3u*151u*55u);
  assert(allowedCount==2u*3037u+3043u);
  assert(editorExtra==8);
  assert(Source::moveFor(1,6,true,2)==0);
  assert(Source::moveFor(3,6,true,2)==19);
  std::cout<<"Gen I existing editor retains all 9,117 original RBY TM/HM allowances, "
             "8 union-only move entries; 24,915 indexed checks PASS\n";
}
