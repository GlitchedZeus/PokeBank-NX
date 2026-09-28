#include "Legacy/GSCSourceBrowser.h"

#include "Games/GameIdentity.h"

#include <algorithm>

namespace PokeVault::Legacy {
namespace {
std::string leafName(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string stableIdentity(const GSCSource& source) {
    if (!source.sourceIdentity.empty()) return source.sourceIdentity;
    return source.canonicalPath.empty() ? source.normalizedPath : source.canonicalPath;
}

bool validGSCGame(const Games::GameDescriptor* game) noexcept {
    return game && game->platform == Games::Platform::GameBoyColor &&
        game->dataGeneration == 2 && game->switchTitleId == 0;
}
} // namespace

std::vector<FRLGSourceCard> buildGSCSourceCards(const GSCDiscoveryResult& discovery) {
    std::vector<FRLGSourceCard> cards;
    for (std::size_t index = 0; index < discovery.sources.size(); ++index) {
        const auto& source = discovery.sources[index];
        if (!source.ready()) continue;
        const auto* game = Games::findGame(source.gameId);
        if (!validGSCGame(game) || source.save->metadata().sourceGameId != game->id) continue;

        auto cardIt = std::find_if(cards.begin(), cards.end(), [&](const auto& card) {
            return card.gameId == game->id;
        });
        std::size_t cardIndex = 0;
        if (cardIt == cards.end()) {
            cards.push_back({
                std::string(game->id), std::string(game->title), "GBC", "RETROARCH",
                std::string(game->id), {},
            });
            cardIndex = cards.size() - 1;
        } else {
            cardIndex = static_cast<std::size_t>(std::distance(cards.begin(), cardIt));
        }

        const std::string identity = source.canonicalPath.empty() ? source.path : source.canonicalPath;
        const auto& strictTrainer = source.save->trainer();
        const std::string fingerprint = source.contentFingerprint.empty()
            ? std::string("unavailable") : source.contentFingerprint;
        const std::string details = (strictTrainer.name.empty() ? std::string("Unknown trainer")
                                                                 : strictTrainer.name) +
            " | Party " + std::to_string(source.save->party().size()) + " | FP " +
            fingerprint.substr(0, std::min<std::size_t>(12, fingerprint.size()));
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
        instance.generation = 2;
        instance.platformLabel = "GBC";
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

std::vector<FRLGSourceCard> buildGSCSourceCardsForProfile(
    const GSCDiscoveryResult& discovery, const LegacySourceBindings& bindings,
    std::string_view profileIdentity) {
    auto cards = buildGSCSourceCards(discovery);
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

const GSCSource* resolveGSCSaveInstance(
    const GSCDiscoveryResult& discovery, const FRLGSourceCard& card,
    std::size_t instanceIndex) noexcept {
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
    if (!validGSCGame(Games::findGame(card.gameId))) return nullptr;
    return &source;
}

} // namespace PokeVault::Legacy
