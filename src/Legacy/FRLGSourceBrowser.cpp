#include "Legacy/FRLGSourceBrowser.h"

#include "Games/GameIdentity.h"

namespace PokeVault::Legacy {
    std::vector<FRLGSourceCard> buildFRLGSourceCards(const FRLGDiscoveryResult& discovery) {
        std::vector<FRLGSourceCard> cards;
        cards.reserve(discovery.sources.size());
        for (size_t index = 0; index < discovery.sources.size(); ++index) {
            const auto& source = discovery.sources[index];
            if (!source.ready()) continue;

            const auto* game = Games::findGame(source.gameId);
            if (!game || game->platform != Games::Platform::GameBoyAdvance ||
                game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0) {
                continue;
            }
            if (source.save->metadata().sourceGameId != game->id) continue;

            cards.push_back({
                index,
                std::string(game->id),
                std::string(game->title),
                std::string(Games::platformName(game->platform)),
                "RETROARCH",
                source.path,
            });
        }
        return cards;
    }

    const FRLGSource* resolveFRLGSourceCard(
        const FRLGDiscoveryResult& discovery, const FRLGSourceCard& card) noexcept {
        if (card.sourceIndex >= discovery.sources.size()) return nullptr;
        const auto& source = discovery.sources[card.sourceIndex];
        if (!source.ready() || source.gameId != card.gameId) return nullptr;
        if (source.save->metadata().sourceGameId != card.gameId) return nullptr;
        const auto* game = Games::findGame(card.gameId);
        if (!game || game->platform != Games::Platform::GameBoyAdvance ||
            game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0) {
            return nullptr;
        }
        return &source;
    }
}
