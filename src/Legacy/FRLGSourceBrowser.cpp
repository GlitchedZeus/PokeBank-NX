#include "Legacy/FRLGSourceBrowser.h"

#include "Games/GameIdentity.h"

#include <algorithm>
#include <set>

namespace PokeVault::Legacy {
    std::vector<FRLGSourceCard> buildFRLGSourceCards(const FRLGDiscoveryResult& discovery) {
        std::vector<FRLGSourceCard> cards;
        std::vector<std::set<std::string>> instanceKeys;
        for (size_t index = 0; index < discovery.sources.size(); ++index) {
            const auto& source = discovery.sources[index];
            if (!source.ready()) continue;

            const auto* game = Games::findGame(source.gameId);
            if (!game || game->platform != Games::Platform::GameBoyAdvance ||
                game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0) {
                continue;
            }
            if (source.save->metadata().sourceGameId != game->id) continue;

            auto cardIt = std::find_if(cards.begin(), cards.end(), [&](const auto& card) {
                return card.gameId == game->id;
            });
            size_t cardIndex = 0;
            if (cardIt == cards.end()) {
                cards.push_back({
                    std::string(game->id),
                    std::string(game->title),
                    std::string(Games::platformName(game->platform)),
                    "RETROARCH",
                    std::string(game->id),
                    {},
                });
                instanceKeys.emplace_back();
                cardIndex = cards.size() - 1;
            } else {
                cardIndex = static_cast<size_t>(std::distance(cards.begin(), cardIt));
            }

            // Discovery normally canonicalizes this already. Keeping the same guard at the model
            // boundary makes manually assembled/imported catalogs safe too. Never use content hash,
            // trainer name, or game id as the child identity: separate files must remain separate.
            const std::string& identity = source.canonicalPath.empty() ? source.path
                                                                       : source.canonicalPath;
            if (!instanceKeys[cardIndex].insert(identity).second) continue;
            auto& instances = cards[cardIndex].instances;
            const size_t number = instances.size() + 1;
            const std::string saveLabel = number == 1 ? "Main Save"
                : "Battery Save " + std::to_string(number);
            const std::string& trainerName = source.save->trainer().name;
            instances.push_back({
                index,
                LegacySaveInstanceKind::BatterySave,
                trainerName.empty() ? saveLabel : trainerName + " — " + saveLabel,
                "RETROARCH BATTERY SAVE",
                source.path,
                source.normalizedPath,
                identity,
                source.fileSize,
                source.modifiedTime,
            });
        }
        return cards;
    }

    const FRLGSource* resolveFRLGSaveInstance(
        const FRLGDiscoveryResult& discovery, const FRLGSourceCard& card,
        size_t instanceIndex) noexcept {
        if (instanceIndex >= card.instances.size()) return nullptr;
        const auto& instance = card.instances[instanceIndex];
        if (instance.kind != LegacySaveInstanceKind::BatterySave ||
            instance.sourceIndex >= discovery.sources.size()) return nullptr;
        const auto& source = discovery.sources[instance.sourceIndex];
        if (!source.ready() || source.gameId != card.gameId) return nullptr;
        if (instance.normalizedPath != source.normalizedPath ||
            instance.sourceIdentity != (source.canonicalPath.empty() ? source.path
                                                                      : source.canonicalPath) ||
            instance.fileSize != source.fileSize ||
            instance.modifiedTime != source.modifiedTime) return nullptr;
        if (source.save->metadata().sourceGameId != card.gameId) return nullptr;
        const auto* game = Games::findGame(card.gameId);
        if (!game || game->platform != Games::Platform::GameBoyAdvance ||
            game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0) {
            return nullptr;
        }
        return &source;
    }
}
