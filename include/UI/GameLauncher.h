#ifndef UI_GAME_LAUNCHER_H
#define UI_GAME_LAUNCHER_H

#include <cstdint>
#include <string>
#include <string_view>

#include "UI/GameLaunchModel.h"

namespace UI {

GameLaunchDescriptor resolveGameLaunch(uint64_t titleId,
                                       std::string_view gameId,
                                       std::string_view providerId,
                                       std::string_view sourcePath,
                                       std::string_view bindingKey = {});

bool saveGameLaunchBinding(std::string_view bindingKey,
                           std::string_view gameId,
                           std::string_view providerId,
                           std::string_view contentPath,
                           std::string& error);

bool forgetGameLaunchBinding(std::string_view bindingKey, std::string& error);

std::string suggestedGameLaunchBrowseRoot(std::string_view gameId,
                                          std::string_view providerId,
                                          std::string_view sourcePath);

bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error);

} // namespace UI

#endif
