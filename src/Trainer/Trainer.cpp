/**
 * Trainer.cpp - Trainer/Save File Data Management Implementation
 *
 * This file implements the Trainer class for managing Pokemon save file data.
 * Pokemon encryption/decryption logic has been moved to PKM/Gen8Encryption.cpp
 * and Pokemon class implementations have been moved to PKM/PK8.cpp, PKM/PK7.cpp, etc.
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <span>
#include <algorithm>

#include "Utils/Logger.h"
#include "Trainer/Trainer.h"
#include "Encryption/Gen8Encryption.h"