#pragma once
#include <cstdint>
#include <string>
#include <vector>
bool verifyBackupSessionTrace(const std::vector<uint8_t>& bytes,const std::string& game,std::string& error);
