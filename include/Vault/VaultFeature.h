#pragma once
// Master Vault v1 build flag. OFF by default: no candidate build shows "Copy to Vault" or
// "Withdraw from Vault" until the owner has validated the Vault on hardware. Turn it on for a test
// build only:   make POKEBANK_VAULT=1      (adds -DPOKEBANK_ENABLE_VAULT=1)
// Host tests that exercise the wired rows compile with -DPOKEBANK_ENABLE_VAULT=1 explicitly.
// Dependency-free on purpose: the shared action contracts include it.

#ifndef POKEBANK_ENABLE_VAULT
#define POKEBANK_ENABLE_VAULT 0
#endif

namespace PokeBank::Vault {

inline constexpr bool kEnabled = POKEBANK_ENABLE_VAULT != 0;

}  // namespace PokeBank::Vault
