#pragma once
#include "Source/ImmutableSourceSnapshot.h"
#include "Trainer/Trainer.h"
#include <memory>
#include <string>

namespace PokeVault::Legacy {
// Opens exactly the independently captured historical file. Never changes a
// profile binding, discovers a replacement live source, or writes the snapshot.
std::unique_ptr<Trainer::Trainer> openBackupTrainer(
    const Source::ImmutableSnapshot::File& file,const std::string& gameId,std::string& error);
}
