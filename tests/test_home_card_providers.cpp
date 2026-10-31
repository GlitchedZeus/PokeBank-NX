// Home card provider identity, label fitting and the fingerprinted Home
// snapshot cache. Shared across Generations I-V: every generation's cards and
// Save Instance rows go through these exact helpers in SaveSelectScreen.cpp.
#include "Source/HomeCardSnapshot.h"
#include "Source/SaveProviderIdentity.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace S = PokeVault::Source;

namespace {

// Deterministic stand-in for PKSEFramebuffer::measureText: 9 px per ASCII
// byte, 14 px per non-ASCII code point (e.g. the bullet and ellipsis).
int fakeMeasure(const std::string& text) {
    int width = 0;
    for (size_t i = 0; i < text.size(); ++i) {
        const auto byte = static_cast<unsigned char>(text[i]);
        if (byte < 0x80) width += 9;
        else if ((byte & 0xC0) == 0xC0) width += 14;
    }
    return width;
}

bool validUtf8(const std::string& text) {
    for (size_t i = 0; i < text.size();) {
        const auto byte = static_cast<unsigned char>(text[i]);
        size_t length = byte < 0x80 ? 1 : (byte & 0xE0) == 0xC0 ? 2 : (byte & 0xF0) == 0xE0 ? 3 : 4;
        if (i + length > text.size()) return false;
        for (size_t k = 1; k < length; ++k)
            if ((static_cast<unsigned char>(text[i + k]) & 0xC0) != 0x80) return false;
        i += length;
    }
    return true;
}

S::SaveInstance row(uint8_t generation, std::string platform, std::string provider,
                    std::string sourceLabel) {
    S::SaveInstance instance;
    instance.generation = generation;
    instance.platformLabel = std::move(platform);
    instance.providerLabel = std::move(provider);
    instance.providerId = S::providerIdFor(instance.providerLabel);
    instance.sourceLabel = std::move(sourceLabel);
    return instance;
}

void providerIdentityTable() {
    // Exact labels written by each generation's discovery / manual link code.
    struct Case { std::string_view raw; std::string_view shown; };
    const Case cases[] = {
        {"DraStic", "DraStic"},              // Gen IV/V DraStic backup roots
        {"DraStic Savestate", "DraStic (savestate)"}, // Gen IV savestate roots (diagnostic rows)
        {"melonDS", "melonDS"},              // Gen IV/V melonDS roots
        {"RetroArch", "RetroArch"},          // Gen I-V RetroArch roots
        {"RetroArch/DeSmuME", "RetroArch (DeSmuME)"}, // Gen IV .dsv under RetroArch
        {"mGBA", "mGBA"},                    // Gen I-III configured mGBA root
        {"Tico", "Tico"},                    // Gen I-III Tico battery folders
        {"Dekopon", "Dekopon"},
        {"Azahar", "Azahar"},
        {"Checkpoint", "Checkpoint (3DS backup)"}, // Gen VI/VII Checkpoint-layout export folders
        {"Manual", "Manual file"},           // explicit manual file link
        {"RETROARCH", "RetroArch"},          // historical upper-cased classic card label
        {"", "Unknown provider"},            // pre-provider binding row
        {"Remembered", "Unknown provider"},  // Gen IV fallback when no provider recorded
        {"Source", "Unknown provider"},
        {"DraStic2", "Unknown provider"},    // never fuzzy/guessed
        {"drastic/backup", "Unknown provider"},
    };
    for (const auto& c : cases) {
        assert(S::providerDisplayName(c.raw) == c.shown);
    }
    // A provider is never derived from generation or platform defaults.
    assert(S::identifyProvider("Nintendo DS") == S::ProviderIdentity::Unknown);
    assert(S::identifyProvider("Game Boy Advance") == S::ProviderIdentity::Unknown);
}

void homeCardsShowRealProvidersForEveryGeneration() {
    struct Card {
        uint8_t generation;
        std::string_view platform;
        std::string_view state;
        std::string_view provider;
        std::string_view expected;
    };
    const Card cards[] = {
        {1, "Game Boy", "RETROARCH", "RetroArch", "Game Boy \xE2\x80\xA2 RetroArch"},
        {1, "Game Boy", "TICO", "Tico", "Game Boy \xE2\x80\xA2 Tico"},
        {2, "GBC", "RETROARCH", "RetroArch", "GBC \xE2\x80\xA2 RetroArch"},
        {3, "Game Boy Advance", "MGBA", "mGBA", "Game Boy Advance \xE2\x80\xA2 mGBA"},
        {3, "Game Boy Advance", "MULTI-SOURCE", "Multiple providers",
            "Game Boy Advance \xE2\x80\xA2 Multiple providers"},
        {4, "Nintendo DS", "REMEMBERED", "DraStic", "Nintendo DS \xE2\x80\xA2 DraStic"},
        {4, "Nintendo DS", "REMEMBERED", "melonDS", "Nintendo DS \xE2\x80\xA2 melonDS"},
        {4, "Nintendo DS", "REMEMBERED", "RetroArch/DeSmuME",
            "Nintendo DS \xE2\x80\xA2 RetroArch (DeSmuME)"},
        {4, "Nintendo DS", "REMEMBERED", "Manual", "Nintendo DS \xE2\x80\xA2 Manual file"},
        {4, "Nintendo DS", "REMEMBERED", "", "Nintendo DS \xE2\x80\xA2 Unknown provider"},
        {5, "Nintendo DS", "REMEMBERED", "DraStic", "Nintendo DS \xE2\x80\xA2 DraStic"},
        {5, "Nintendo DS", "REMEMBERED", "melonDS", "Nintendo DS \xE2\x80\xA2 melonDS"},
        {5, "Nintendo DS", "REMEMBERED", "RetroArch", "Nintendo DS \xE2\x80\xA2 RetroArch"},
        {5, "Nintendo DS", "REMEMBERED", "Manual", "Nintendo DS \xE2\x80\xA2 Manual file"},
        {5, "Nintendo DS", "REMEMBERED / NEW-GAME SAVE, NO TRAINER YET", "DraStic",
            "Nintendo DS \xE2\x80\xA2 DraStic \xE2\x80\xA2 New game, no trainer yet"},
        {5, "Nintendo DS", "REMEMBERED / CHOOSE COPY", "melonDS",
            "Nintendo DS \xE2\x80\xA2 melonDS \xE2\x80\xA2 Choose copy"},
        {5, "Nintendo DS", "NO TRAINER DATA / RELINK", "DraStic",
            "Nintendo DS \xE2\x80\xA2 DraStic \xE2\x80\xA2 No trainer data"},
        {5, "Nintendo DS", "RELINK SAVE", "RetroArch",
            "Nintendo DS \xE2\x80\xA2 RetroArch \xE2\x80\xA2 Relink save"},
        {5, "Nintendo DS", "REMEMBERED / STALE", "DraStic",
            "Nintendo DS \xE2\x80\xA2 DraStic \xE2\x80\xA2 Stale"},
    };
    for (const auto& card : cards) {
        const auto line = S::homeSourceLine(card.platform,
            S::homeCardSourceText(card.state, card.provider));
        if (line != card.expected) {
            std::cerr << "gen " << int(card.generation) << ": got '" << line
                      << "' expected '" << card.expected << "'\n";
            assert(false);
        }
        // The old generic label is gone for every generation.
        assert(line.find("Linked save") == std::string::npos);
    }
    // Missing / invalid / ambiguous / unassigned stay distinct and name no
    // provider, even when a stale binding row still carries one.
    assert(S::homeCardSourceText("MISSING", "DraStic") == "Missing source");
    assert(S::homeCardSourceText("INVALID", "melonDS") == "Invalid source");
    assert(S::homeCardSourceText("AMBIGUOUS", "RetroArch") == "Needs attention");
    assert(S::homeCardSourceText("CHOOSE SAVE", "") == "Choose save");
    assert(S::homeCardSourceText("LOCAL SAVE", "") == "System save");
}

void saveInstanceRowsShowRealProviders() {
    const std::vector<S::SaveInstance> rows = {
        row(1, "Game Boy", "RetroArch", "Gen I / RED"),
        row(2, "GBC", "Tico", "Gen II / GOLD"),
        row(3, "Game Boy Advance", "mGBA", "Battery save"),
        row(4, "Nintendo DS", "DraStic", "Raw cartridge save"),
        row(4, "Nintendo DS", "RetroArch/DeSmuME", "DSV cartridge backup"),
        row(5, "Nintendo DS", "melonDS", "Gen V / Black / NX"),
        row(5, "Nintendo DS", "Manual", "Gen V / White"),
        row(4, "Nintendo DS", "Remembered", "Manual cartridge save"),
    };
    const char* expected[] = {
        "RetroArch / Gen I / RED",
        "Tico / Gen II / GOLD",
        "mGBA / Battery save",
        "DraStic / Raw cartridge save",
        "RetroArch (DeSmuME) / DSV cartridge backup",
        "melonDS / Gen V / Black / NX",
        "Manual file / Gen V / White",
        "Unknown provider / Manual cartridge save",
    };
    for (size_t i = 0; i < rows.size(); ++i)
        assert(S::saveInstanceSourceLine(rows[i]) == expected[i]);

    // A classic card owning instances from two providers says so.
    std::vector<S::SaveInstance> mixed = {rows[0], row(1, "Game Boy", "Tico", "Gen I")};
    assert(S::cardProviderLabel(mixed) == "Multiple providers");
    assert(S::cardProviderDisplay(S::cardProviderLabel(mixed)) == "Multiple providers");
    std::vector<S::SaveInstance> same = {rows[0], row(1, "Game Boy", "RETROARCH", "Gen I")};
    assert(S::cardProviderDisplay(S::cardProviderLabel(same)) == "RetroArch");
    assert(S::cardProviderLabel({}).empty());
}

void labelsNeverExceedTheCard() {
    const std::string longest =
        "Game Boy Advance \xE2\x80\xA2 RetroArch (DeSmuME) \xE2\x80\xA2 New game, no trainer yet";
    for (int width = 0; width <= 600; width += 7) {
        const auto fitted = S::ellipsizeToWidth(longest, width, fakeMeasure);
        assert(fakeMeasure(fitted) <= width);
        assert(validUtf8(fitted));
        if (fakeMeasure(longest) <= width) assert(fitted == longest);
        else if (!fitted.empty()) assert(fitted.size() >= 3 &&
            fitted.compare(fitted.size() - 3, 3, "\xE2\x80\xA6") == 0);
    }
    // Ellipsis alone does not fit: draw nothing rather than overflow.
    assert(S::ellipsizeToWidth(longest, 10, fakeMeasure).empty());
    assert(S::ellipsizeToWidth(longest, -5, fakeMeasure).empty());
    // Classic Games tile (CLASSIC_TILE_W - 16) and Home hero (326 px) budgets
    // for every provider the table knows.
    for (std::string_view provider : {"DraStic", "melonDS", "RetroArch", "RetroArch/DeSmuME",
                                      "mGBA", "Tico", "Dekopon", "Azahar", "Checkpoint", "Manual", ""}) {
        const auto line = S::homeSourceLine("Nintendo DS",
            S::homeCardSourceText("REMEMBERED / CHOOSE COPY", provider));
        for (int budget : {150, 184, 326}) {
            const auto fitted = S::ellipsizeToWidth(line, budget, fakeMeasure);
            assert(fakeMeasure(fitted) <= budget && validUtf8(fitted));
        }
    }
    // Trailing spaces are not left before the ellipsis.
    const auto spaced = S::ellipsizeToWidth("AAAA BBBB", 9 * 5 + 14, fakeMeasure);
    assert(spaced == "AAAA\xE2\x80\xA6");
}

S::HomeCardSnapshot validSnapshot(std::string trainer, uint16_t species, uint8_t level,
                                  bool shiny, std::string fingerprint) {
    S::HomeCardSnapshot s;
    s.state = S::SnapshotState::Current;
    s.trainerName = std::move(trainer);
    s.trainerGender = 1;
    s.trainerGenderKnown = true;
    s.dexSeen = 3;
    s.dexCaught = 2;
    s.dexTotal = 649;
    s.partyCount = 1;
    s.party[0] = {species, level, 0, shiny};
    s.contentFingerprint = std::move(fingerprint);
    return s;
}

void snapshotCacheReparsesOnlyOnChange() {
    S::HomeSnapshotCache cache;
    std::optional<S::FileStamp> disk = S::FileStamp{0x80000, 1000};
    S::HomeCardSnapshot parsed = validSnapshot("NX", 25, 50, true, "aa");
    int stats = 0, parses = 0;
    auto stat = [&](const std::string& path) {
        assert(path == "sdmc:/switch/drastic/backup/Pokemon Black.dsv");
        ++stats;
        return disk;
    };
    auto parse = [&]() { ++parses; return parsed; };
    S::SnapshotRequest request{"p1|black_nds", "sdmc:/switch/drastic/backup/Pokemon Black.dsv",
                               "id\tDraStic\tBW\t\t\t"};
    S::SnapshotRefresh outcome{};

    // App start: the party is available without opening any editor.
    const auto* snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == 1);
    assert(snapshot->current() && snapshot->party[0].species == 25 &&
           snapshot->party[0].level == 50 && snapshot->party[0].shiny);
    assert(S::homePartyStatus(*snapshot) == "Current save party");

    // Focus changes / polls with an unchanged file: stat only, never reparse.
    for (int frame = 0; frame < 120; ++frame) {
        snapshot = &cache.refresh(request, stat, parse, &outcome);
        assert(outcome == S::SnapshotRefresh::Reused);
    }
    assert(parses == 1 && cache.parseCount() == 1);
    assert(cache.find("p1|black_nds") == snapshot);

    // Emulator writes the save: mtime/size change -> strict re-validation.
    disk = S::FileStamp{0x80000, 1060};
    parsed = validSnapshot("NX", 133, 51, false, "bb");
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == 2);
    assert(snapshot->current() && snapshot->party[0].species == 133 &&
           snapshot->contentFingerprint == "bb");

    // A size-only change is also a change.
    disk = S::FileStamp{0x80010, 1060};
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == 3);

    // Re-validation fails (half-written save): last good data is kept but is
    // explicitly STALE, never labelled current.
    disk = S::FileStamp{0x80000, 1120};
    parsed = S::unavailableSnapshot("checksum mismatch");
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == 4);
    assert(snapshot->state == S::SnapshotState::Stale && !snapshot->current());
    assert(snapshot->party[0].species == 133);
    assert(snapshot->reason.find("checksum mismatch") != std::string::npos);
    const auto staleStatus = S::homePartyStatus(*snapshot);
    assert(staleStatus.rfind("STALE", 0) == 0 && staleStatus != "Current save party");
    // The failed state is cached too: no reparse loop while the file is idle.
    cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Reused && parses == 4);
    // Stale twice stays stale (reason replaced, not nested).
    disk = S::FileStamp{0x80000, 1130};
    parsed = S::unavailableSnapshot("still broken");
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(snapshot->state == S::SnapshotState::Stale &&
           snapshot->reason == "Save changed and could not be re-validated: still broken");

    // The file becomes valid again.
    disk = S::FileStamp{0x80000, 1200};
    parsed = validSnapshot("NX", 26, 52, false, "cc");
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(snapshot->current() && snapshot->reason.empty() && snapshot->party[0].species == 26);

    // Missing file: no stale party presented as the save.
    disk.reset();
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Missing && !snapshot->hasData());
    assert(snapshot->party[0].species == 0 && snapshot->trainerName.empty());
    assert(S::homePartyStatus(*snapshot) == "Linked save file is missing.");

    // A relink to a different provider/copy invalidates even with an
    // identical stamp.
    disk = S::FileStamp{0x80000, 1200};
    cache.refresh(request, stat, parse, &outcome);
    const int before = parses;
    request.bindingSignature = "id\tmelonDS\tBW\tgen5-copy-1-backup\t\t";
    cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == before + 1);

    // A failure on a NEW assignment has no previous data: Unavailable.
    request.bindingSignature = "other";
    parsed = S::unavailableSnapshot("");
    snapshot = &cache.refresh(request, stat, parse, &outcome);
    assert(snapshot->state == S::SnapshotState::Unavailable && !snapshot->reason.empty());

    // Unassigned card.
    S::SnapshotRequest none{"p1|white_nds", "", ""};
    snapshot = &cache.refresh(none, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Unassigned && !snapshot->hasData());

    // Profiles are independent cache entries.
    S::SnapshotRequest other = request;
    other.key = "p2|black_nds";
    parsed = validSnapshot("ZE", 1, 5, false, "dd");
    snapshot = &cache.refresh(other, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && snapshot->trainerName == "ZE");
    cache.invalidate("p2|black_nds");
    assert(cache.find("p2|black_nds") == nullptr);
}

void writeDuringParseForcesRevalidation() {
    S::HomeSnapshotCache cache;
    int statCalls = 0, parses = 0;
    // The emulator writes between the pre-parse and post-parse stat.
    auto stat = [&](const std::string&) -> std::optional<S::FileStamp> {
        ++statCalls;
        return S::FileStamp{0x80000, statCalls == 1 ? 10 : 11};
    };
    auto parse = [&]() { ++parses; return validSnapshot("NX", 25, 5, false, "x"); };
    S::SnapshotRequest request{"k", "/save", "sig"};
    S::SnapshotRefresh outcome{};
    cache.refresh(request, stat, parse, &outcome);
    assert(parses == 1);
    cache.refresh(request, stat, parse, &outcome);
    assert(outcome == S::SnapshotRefresh::Parsed && parses == 2);
}

void newGameAndEmptyPartyStates() {
    auto s = validSnapshot("", 0, 0, false, "x");
    s.party = {};
    s.partyCount = 0;
    assert(S::homePartyStatus(s) == "No active party Pok\xC3\xA9mon.");
    s.newGameNoTrainer = true;
    assert(S::homePartyStatus(s) == "New-game save: no trainer or party yet.");
    // "cached party" never appears for any state.
    for (auto state : {S::SnapshotState::Current, S::SnapshotState::Stale,
                       S::SnapshotState::Unavailable}) {
        s.state = state;
        s.reason = "r";
        assert(S::homePartyStatus(s).find("cached") == std::string::npos);
    }
}

void assignmentRequestSignature() {
    PokeVault::Legacy::AssignedFile assigned;
    assigned.status = PokeVault::Legacy::AssignedFileStatus::Ready;
    assigned.sourceIdentity = "inode:2:42";
    assigned.binding = {"p1", "black_nds", "/sd/black.sav", "DraStic", "BW"};
    const auto a = S::assignedSnapshotRequest("p1", "black_nds", assigned);
    assert(a.key == "p1|black_nds" && a.sourcePath == "/sd/black.sav");
    auto relinked = assigned;
    relinked.binding.sourceType = "melonDS";
    assert(S::assignedSnapshotRequest("p1", "black_nds", relinked).bindingSignature !=
           a.bindingSignature);
    relinked = assigned;
    relinked.binding.sourceVariant = "gen5-copy-1-backup";
    assert(S::assignedSnapshotRequest("p1", "black_nds", relinked).bindingSignature !=
           a.bindingSignature);
    auto missing = assigned;
    missing.status = PokeVault::Legacy::AssignedFileStatus::Missing;
    assert(S::assignedSnapshotRequest("p1", "black_nds", missing).sourcePath.empty());
    assert(!S::statRegularFile("/definitely/not/a/pokebank/save.sav"));
    assert(!S::statRegularFile("/"));
    assert(!S::statRegularFile(""));
}

} // namespace

int main() {
    providerIdentityTable();
    homeCardsShowRealProvidersForEveryGeneration();
    saveInstanceRowsShowRealProviders();
    labelsNeverExceedTheCard();
    snapshotCacheReparsesOnlyOnChange();
    writeDuringParseForcesRevalidation();
    newGameAndEmptyPartyStates();
    assignmentRequestSignature();
    std::cout << "Home cards: real providers Gen I-V, measured labels, fingerprinted snapshot "
                 "cache (no reparse when unchanged, stale/unavailable explicit) PASS\n";
    return 0;
}
