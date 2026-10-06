from pathlib import Path

src_path = Path("src/UI/SaveSelectScreen.cpp")
test_path = Path("tests/test_game_hub_contract.py")
src = src_path.read_text(encoding="utf-8")
test = test_path.read_text(encoding="utf-8")

if "legacySaveGameIdForForwarder" in src:
    raise SystemExit("forwarder save-source repair is already present")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {count}")
    return text.replace(old, new, 1)


old = '''        std::string productSourceLabel(std::string_view raw) {
            if (raw == "LOCAL SAVE") return "System save";
            if (raw == "REMEMBERED") return "Linked save";
            if (raw == "CHOOSE SAVE") return "Choose save";
            if (raw == "MISSING") return "Missing source";
            if (raw == "INVALID") return "Invalid source";
            if (raw == "AMBIGUOUS") return "Needs attention";
            return std::string(raw);
        }
'''
new = old + '''
        // HOME forwarders are launch identities, not native Pokemon save containers. Keep the
        // installed Switch title ID for ZR launch, but bind preview/open to the real GBA battery
        // save assigned to the same profile.
        std::string_view legacySaveGameIdForForwarder(std::string_view gameId) noexcept {
            if (gameId == "firered_switch") return "firered_gba";
            if (gameId == "leafgreen_switch") return "leafgreen_gba";
            return {};
        }
'''
src = replace_once(src, old, new, "forwarder identity helper")

old = '''            const auto cards = legacyBindings
                ? PokeVault::Legacy::buildFRLGSourceCardsForProfile(
                    legacySources, *legacyBindings, profileIdentity(user.uid))
                : std::vector<PokeVault::Legacy::FRLGSourceCard>{};
            user.titles.reserve(user.titles.size() + cards.size());
            for (const auto& card : cards) {
'''
new = '''            const auto cards = legacyBindings
                ? PokeVault::Legacy::buildFRLGSourceCardsForProfile(
                    legacySources, *legacyBindings, profileIdentity(user.uid))
                : std::vector<PokeVault::Legacy::FRLGSourceCard>{};

            // An installed FireRed/LeafGreen HOME forwarder can have its own Switch savedata,
            // but that savedata belongs to the forwarder and is not the GBA cartridge save.
            // Attach the profile's validated GBA source to the installed card while retaining its
            // titleId so launch still targets the installed HOME application.
            for (auto& installed : user.titles) {
                if (installed.sourceKind != SelectedSourceKind::SwitchTitle) continue;
                const std::string_view saveGameId = legacySaveGameIdForForwarder(installed.gameId);
                if (saveGameId.empty()) continue;
                const auto card = std::find_if(cards.begin(), cards.end(), [&](const auto& candidate) {
                    return candidate.gameId == saveGameId;
                });
                if (card == cards.end()) continue;
                installed.legacyInstances = card->instances;
                installed.sourceLabel = card->sourceLabel;
                installed.locationLabel = std::to_string(card->instances.size()) +
                    (card->instances.size() == 1 ? " SAVE" : " SAVES");
                if (card->instances.size() == 1)
                    installed.trainerName = card->instances.front().trainerName;
            }

            user.titles.reserve(user.titles.size() + cards.size());
            for (const auto& card : cards) {
'''
src = replace_once(src, old, new, "attach legacy sources to forwarder cards")

old = '''        if (title.sourceKind == SelectedSourceKind::SwitchTitle && title.titleId != 0) {
            const Result mount = fsdevMountSaveData("pbpreview", title.titleId, user->uid);
'''
new = '''        const std::string_view forwarderSaveGameId = legacySaveGameIdForForwarder(title.gameId);
        if (title.sourceKind == SelectedSourceKind::SwitchTitle && !forwarderSaveGameId.empty()) {
            if (title.legacyInstances.size() != 1 || !legacyCatalog) {
                partyPreviewStatus = "Party preview unavailable.";
                return;
            }
            const auto& instance = title.legacyInstances.front();
            if (instance.sourceIndex >= legacyCatalog->sources.size()) {
                partyPreviewStatus = "Party preview unavailable.";
                return;
            }
            const auto& source = legacyCatalog->sources[instance.sourceIndex];
            if (!source.ready() || source.gameId != forwarderSaveGameId ||
                !source.isGen3() || !source.save) {
                partyPreviewStatus = "Party preview unavailable.";
                return;
            }
            previewTrainerName = instance.trainerName;
            previewTrainerGender = source.save->trainer().gender;
            previewTrainerGenderKnown = true;
            const auto dex = source.save->dexProgress();
            previewDexSeen = dex.seen;
            previewDexCaught = dex.caught;
            previewDexTotal = dex.total;
            const auto party = source.save->party();
            for (size_t i = 0; i < std::min(party.size(), partyPreview.size()); ++i)
                addParty(i, party[i].species, 0, 0, false);
            partyPreviewStatus = std::any_of(partyPreview.begin(), partyPreview.end(),
                    [](const auto& slot) { return slot.species != 0; })
                ? "Current save party" : "No active party Pokémon.";
            return;
        }

        if (title.sourceKind == SelectedSourceKind::SwitchTitle && title.titleId != 0) {
            const Result mount = fsdevMountSaveData("pbpreview", title.titleId, user->uid);
'''
src = replace_once(src, old, new, "forwarder preview route")

old = '''        if (selected.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (!legacyCatalog || selected.legacyInstances.empty()) {
'''
new = '''        const std::string_view forwarderSaveGameId = legacySaveGameIdForForwarder(selectedGameId);
        if (selected.sourceKind == SelectedSourceKind::SwitchTitle && !forwarderSaveGameId.empty()) {
            if (!legacyCatalog || selected.legacyInstances.empty()) {
                hubNotice = "Link the matching GBA save before opening this forwarder.";
                return;
            }
            int sourceIndex = 0;
            if (selected.legacyInstances.size() > 1) {
                sourceIndex = preferredLegacySourceIndex(
                    selected.legacyInstances, legacyBindings, profile, forwarderSaveGameId);
                if (sourceIndex < 0) {
                    hubNotice = "Choose one preferred GBA save from the matching FireRed/LeafGreen game card first.";
                    return;
                }
            }
            const auto shown = selected.legacyInstances[static_cast<size_t>(sourceIndex)];
            if (shown.sourceIndex >= legacyCatalog->sources.size()) {
                hubNotice = "The linked GBA save is no longer available. Refresh its source first.";
                return;
            }
            const auto& cachedSource = legacyCatalog->sources[shown.sourceIndex];
            const bool catalogMatches = cachedSource.ready() &&
                cachedSource.gameId == forwarderSaveGameId &&
                cachedSource.normalizedPath == shown.normalizedPath &&
                cachedSource.contentFingerprint == shown.contentFingerprint;
            if (!catalogMatches || !legacySnapshotStillCurrent(shown)) {
                hubNotice = "The linked GBA save changed since discovery. Refresh its source first.";
                return;
            }
            selectedUserUid = user->uid;
            selectedTitleId = 0;
            selectedTitleName = selected.name;
            this->selectedGameId = std::string(forwarderSaveGameId);
            selectedSourceKind = SelectedSourceKind::RetroArchFRLG;
            selectedLegacySourceIndex = shown.sourceIndex;
            titleSelected = true;
            return;
        }

        if (selected.sourceKind == SelectedSourceKind::RetroArchFRLG) {
            if (!legacyCatalog || selected.legacyInstances.empty()) {
'''
src = replace_once(src, old, new, "forwarder open route")

old = '''        if (title.sourceKind == SelectedSourceKind::SwitchTitle) {
            selectedUserUid = user->uid;
            selectedTitleId = title.titleId;
'''
new = '''        if (title.sourceKind == SelectedSourceKind::SwitchTitle &&
            !legacySaveGameIdForForwarder(title.gameId).empty()) {
            // Items for a HOME forwarder belong to the same validated GBA source as Open.
            selectCurrentTitle();
            return;
        }

        if (title.sourceKind == SelectedSourceKind::SwitchTitle) {
            selectedUserUid = user->uid;
            selectedTitleId = title.titleId;
'''
src = replace_once(src, old, new, "forwarder items route")

marker = '''require("SWSH_CURRENT_BOX, Enums::SCTypeCode::Byte, 1, error" in sc_validation,
'''
if marker not in test:
    raise SystemExit("test insertion marker missing")
contract = '''require('if (gameId == "firered_switch") return "firered_gba";' in source and
        'if (gameId == "leafgreen_switch") return "leafgreen_gba";' in source,
        "HOME forwarders must map save identity to the real GBA release")
require("installed.legacyInstances = card->instances;" in source and
        "installed.sourceLabel = card->sourceLabel;" in source,
        "FireRed/LeafGreen forwarder cards must attach the validated profile GBA source")
require("source.gameId != forwarderSaveGameId" in source and
        "this->selectedGameId = std::string(forwarderSaveGameId);" in source and
        "selectedSourceKind = SelectedSourceKind::RetroArchFRLG;" in source,
        "forwarder preview/open must consume the GBA save while retaining Switch launch identity")
require(source.index("!forwarderSaveGameId.empty()") < source.index('fsdevMountSaveData("pbpreview"'),
        "forwarder preview must bypass the forwarder's native Switch savedata mount")
'''
test = test.replace(marker, contract + marker, 1)

src_path.write_text(src, encoding="utf-8")
test_path.write_text(test, encoding="utf-8")
