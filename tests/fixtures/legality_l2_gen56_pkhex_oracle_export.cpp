#include "Legality/Gen56PKHeXEggTemplate.h"
#include <cassert>
#include <iostream>
namespace P=Legality::Gen56PKHeXEggTemplate;
int main() {
  // Same format emitted by tests/pkhex_oracle_gen56/Program.cs.
  // This adapter contains no independent test oracle or manufactured PK5/PK6.
  for(unsigned gen=5;gen<=6;++gen) {
    const auto game=gen==5?P::Game::Black:P::Game::X;
    const unsigned maxSpecies=gen==5?649:721;
    for(unsigned species=1;species<=maxSpecies;++species)
      for(unsigned form=0;form<=6;++form){
        auto s=static_cast<std::uint16_t>(species);
        auto f=static_cast<std::uint8_t>(form);
        const auto hatch=P::canHatchAsEgg(s);
        const auto hidden=gen==5?P::isHiddenPossible5(s):P::isHiddenPossible6(s,f);
        const auto split=P::splitBreedNotBaby(s);
        const auto determined=P::speciesDeterminedByGender(s);
        assert(P::validSpecies(game,s));
        std::cout<<gen<<','<<species<<','<<form<<','<<hatch<<','<<hidden<<','<<split<<','<<determined<<'\n';
      }
  }
}
