#include "Legacy/RBYSourceBrowser.h"

#include "Games/GameIdentity.h"

#include <algorithm>

namespace PokeVault::Legacy {
namespace {
std::string leafName(const std::string& path) {
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string stableIdentity(const RBYSource& source) {
    if (!source.sourceIdentity.empty()) return source.sourceIdentity;
    return source.canonicalPath.empty() ? source.normalizedPath : source.canonicalPath;
}
} // namespace

std::vector<FRLGSourceCard> buildRBYSourceCards(const RBYDiscoveryResult& discovery) {
    std::vector<FRLGSourceCard> cards;
    for (size_t index = 0; index < discovery.sources.size(); ++index) {
        const auto& source = discovery.sources[index];
        if (!source.ready()) continue;
        const auto* game = Games::findGame(source.gameId);
        if (!game || game->platform != Games::Platform::GameBoy ||
            game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0)
            continue;
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
            cardIndex = cards.size() - 1;
        } else {
            cardIndex = static_cast<size_t>(std::distance(cards.begin(), cardIt));
        }

        const std::string identity = source.canonicalPath.empty() ? source.path : source.canonicalPath;
        const auto& strictTrainer = source.save->trainer();
        const std::string fingerprint = source.contentFingerprint.empty()
            ? std::string("unavailable") : source.contentFingerprint;
        const std::string details = (strictTrainer.name.empty() ? std::string("Unknown trainer")
                                                                 : strictTrainer.name) +
            " | Party " + std::to_string(source.save->party().size()) + " | FP " +
            fingerprint.substr(0, std::min<size_t>(12, fingerprint.size()));
        FRLGSaveInstance instance;
        instance.sourceIndex = index;
        instance.kind = LegacySaveInstanceKind::BatterySave;
        instance.label = leafName(source.path);
        instance.providerLabel = "RetroArch";
        instance.sourceLabel = details;
        instance.location = source.path;
        instance.normalizedPath = source.normalizedPath;
        instance.sourceIdentity = stableIdentity(source);
        instance.sourceAliases = source.sourceAliases;
        instance.contentFingerprint = fingerprint;
        instance.trainerName = strictTrainer.name;
        instance.fileSize = source.fileSize;
        instance.modifiedTime = source.modifiedTime;
        instance.partyCount = source.save->party().size();
        instance.gameId = std::string(game->id);
        instance.generation = 1;
        instance.platformLabel = std::string(Games::platformName(game->platform));
        instance.providerId = PokeVault::Source::providerIdFor(instance.providerLabel);
        instance.sourcePath = source.path;
        instance.physicalIdentity = identity;
        instance.containerType = "Battery save";
        instance.validation = PokeVault::Source::ValidationStatus::Ready;
        instance.access = PokeVault::Source::AccessMode::ReadOnly;
        instance.diagnostic = source.detail;
        PokeVault::Source::appendDeduplicated(cards[cardIndex].instances, std::move(instance));
    }

    for (auto& card : cards)
        PokeVault::Source::sortNewestFirst(card.instances);
    return cards;
}

std::vector<FRLGSourceCard> buildRBYSourceCardsForProfile(
    const RBYDiscoveryResult& discovery, const LegacySourceBindings& bindings,
    std::string_view profileIdentity) {
    auto cards = buildRBYSourceCards(discovery);
    for (auto& card : cards) {
        for (auto& instance : card.instances)
            bindings.applyClaims(instance);
        card.instances.erase(std::remove_if(card.instances.begin(), card.instances.end(),
            [&](const auto& instance) {
                return !PokeVault::Source::visibleToProfile(instance, profileIdentity);
            }), card.instances.end());
        if (!card.instances.empty()) PokeVault::Source::sortNewestFirst(card.instances);
    }
    cards.erase(std::remove_if(cards.begin(), cards.end(), [](const auto& card) {
        return card.instances.empty();
    }), cards.end());
    return cards;
}

const RBYSource* resolveRBYSaveInstance(
    const RBYDiscoveryResult& discovery, const FRLGSourceCard& card,
    size_t instanceIndex) noexcept {
    if (instanceIndex >= card.instances.size()) return nullptr;
    const auto& instance = card.instances[instanceIndex];
    if (instance.kind != LegacySaveInstanceKind::BatterySave ||
        instance.sourceIndex >= discovery.sources.size()) return nullptr;
    const auto& source = discovery.sources[instance.sourceIndex];
    if (!source.ready() || source.gameId != card.gameId) return nullptr;
    if (instance.normalizedPath != source.normalizedPath ||
        instance.sourceIdentity != stableIdentity(source) ||
        instance.fileSize != source.fileSize ||
        instance.modifiedTime != source.modifiedTime ||
        instance.contentFingerprint != source.contentFingerprint) return nullptr;
    if (source.save->metadata().sourceGameId != card.gameId) return nullptr;
    const auto* game = Games::findGame(card.gameId);
    if (!game || game->platform != Games::Platform::GameBoy ||
        game->support != Games::SourceSupport::ReadOnly || game->switchTitleId != 0)
        return nullptr;
    return &source;
}

} // namespace PokeVault::Legacy
