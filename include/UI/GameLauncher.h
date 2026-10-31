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

// Capture the exact PokeBank NRO that launched this process. RetroArch uses it as the
// explicit return target through the bundled return host; other launch backends ignore it.
void setGameLaunchReturnPath(std::string_view path);

bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error);

// Where the emulator's own quit leads for this descriptor (for pre-launch UI copy). The answer
// reflects whether the opt-in return host is actually ready, and is remembered so the launch
// can never take a different route without saying so.
GameLaunchQuitRoute previewGameLaunchQuitRoute(const GameLaunchDescriptor& descriptor);

// Everything the launch dialog must say before the user confirms: the descriptor's own launch
// notice (for example an unproven RetroArch core) followed by the quit-route notice.
std::string gameLaunchDialogNotice(const GameLaunchDescriptor& descriptor);

// The exact handoff requestGameLaunch() performs (target NRO + argv). Fails, with a message, if
// the route differs from the last one previewed for this descriptor.
bool planGameLaunchHandoff(const GameLaunchDescriptor& descriptor, GameLaunchChainPlan& plan,
                           std::string& error);

// True once requestGameLaunch() has scheduled a handoff in this process.
bool gameLaunchHandoffRequested();

// Product Home focus at handoff, restored once on the next PokeBank start.
bool saveGameLaunchReturnSelection(const GameLaunchReturnSelection& selection, std::string& error);
bool consumeGameLaunchReturnSelection(int64_t now, GameLaunchReturnSelection& selection);

} // namespace UI

#endif
