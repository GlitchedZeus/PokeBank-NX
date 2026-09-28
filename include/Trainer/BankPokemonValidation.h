#ifndef TRAINER_BANK_POKEMON_VALIDATION_H
#define TRAINER_BANK_POKEMON_VALIDATION_H

namespace Trainer::BankPokemonValidation {

template <typename PokemonLike>
inline bool accepts(const PokemonLike* pokemon) noexcept {
    return pokemon != nullptr &&
           pokemon->speciesID() != 0 &&
           pokemon->checksum() == pokemon->calculateChecksum();
}

} // namespace Trainer::BankPokemonValidation

#endif
