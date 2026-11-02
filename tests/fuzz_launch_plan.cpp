// QA fuzz target: Thread L's launch/return plan (include/UI/GameLaunchModel.h and the libnx-free
// return host policy runtime/return_host/source/return_host_policy.h).
//
// Input bytes become a launch descriptor + return options, a return-host argv, and a return-selection
// record. Invariants checked on every input:
//  * buildGameLaunchChainPlan: the return host is used ONLY when explicitly enabled (default off);
//    an accepted plan's NRO path fits nx-hbloader's 512-byte buffer and its argv the 2048-byte one,
//    and the argv tokenises under libnx argvSetup() rules to exactly the intended arguments
//    (direct: target [content]; hosted: host, PokeBank, mode, target [content]); a hosted argv is
//    accepted by the host's own rh_parse_args and yields the same return path and child argv.
//  * rh_parse_args / rh_next_action / rh_build_return_argv on arbitrary argv never overrun their
//    fixed buffers (ASan) and, when accepted, round-trip the child arguments.
//  * parseGameLaunchReturnSelection never crashes; an accepted record re-serialises and re-parses
//    to the same selection.
// References for the loader buffer sizes and the argv tokeniser are pinned in return_host_policy.h
// (nx-hbloader 82b95122, libnx 20cbcfd9).
#include "fuzz_support.h"

#include "UI/GameLaunchModel.h"

#include <cctype>
#include <cstring>
#include <optional>

namespace {

using namespace UI;

// Port of libnx runtime/argv.c argvSetup() tokenisation, identical to the one in
// tests/test_game_launch_model.cpp (libnx 20cbcfd9): whitespace separates, a leading '"' opens an
// argument that ends at the next '"', there is no escape character.
std::vector<std::string> libnxSplitArgv(const std::string& args) {
    std::vector<std::string> out;
    const char* argstart = nullptr;
    size_t arglen = 0;
    bool quote = false;
    for (size_t i = 0; i < args.size(); ++i) {
        const char c = args[i];
        if (!argstart && std::isspace(static_cast<unsigned char>(c))) continue;
        if (!argstart) {
            if (c == '"') { argstart = &args[i + 1]; quote = true; }
            else { argstart = &args[i]; ++arglen; }
            continue;
        }
        const bool end = quote ? c == '"' : std::isspace(static_cast<unsigned char>(c)) != 0;
        if (!end) ++arglen;
        if (end && arglen) {
            out.emplace_back(argstart, arglen);
            argstart = nullptr;
            quote = false;
            arglen = 0;
        }
    }
    if (arglen) out.emplace_back(argstart, arglen);
    return out;
}

RhParseResult hostParse(const std::vector<std::string>& tokens, RhState& state) {
    std::vector<std::string> copy = tokens;
    std::vector<char*> argv;
    for (auto& token : copy) argv.push_back(token.data());
    argv.push_back(nullptr);
    return rh_parse_args(static_cast<int>(copy.size()), argv.data(), &state);
}

bool terminatedWithin(const char* buffer, size_t cap) { return std::memchr(buffer, '\0', cap) != nullptr; }

struct Reader {
    std::span<const uint8_t> data;
    size_t pos = 0;
    uint8_t byte() { return pos < data.size() ? data[pos++] : 0; }
    std::string text(size_t maxLength) {
        const size_t want = (static_cast<size_t>(byte()) | static_cast<size_t>(byte()) << 8) % (maxLength + 1);
        const size_t n = std::min(want, data.size() - std::min(pos, data.size()));
        std::string out(reinterpret_cast<const char*>(data.data()) + std::min(pos, data.size()), n);
        pos += n;
        return out;
    }
    std::string rest() {
        const size_t start = std::min(pos, data.size());
        pos = data.size();
        return std::string(reinterpret_cast<const char*>(data.data()) + start, data.size() - start);
    }
};

const char* const kProviders[] = {"retroarch", "DraStic", "melonDS", "mGBA", "Tico", "unknown-emu"};

void checkPlan(const GameLaunchDescriptor& d, const GameLaunchReturnOptions& o) {
    const auto plan = buildGameLaunchChainPlan(d, o);
    QA_CHECK(plan.ok == plan.error.empty());
    if (!o.returnHostEnabled) QA_CHECK(!plan.viaReturnHost && plan.returnHostSkipped.empty());
    if (!plan.ok) {
        QA_CHECK(plan.nroPath.empty() && plan.argv.empty() && plan.quitRoute == GameLaunchQuitRoute::None);
        return;
    }
    qafuzz::noteAccepted();
    QA_CHECK(d.backend == GameLaunchBackend::RetroArch || d.backend == GameLaunchBackend::HomebrewNro);
    QA_CHECK(gameLaunchPathFitsLoader(plan.nroPath));
    QA_CHECK(!plan.argv.empty() && plan.argv.size() < RH_LOADER_ARGV_CAP);

    const std::string& target = d.backend == GameLaunchBackend::RetroArch && !d.corePath.empty()
                                    ? d.corePath : d.launcherPath;
    std::vector<std::string> payload{target};
    if (d.state != GameLaunchState::LauncherOnly && !d.contentPath.empty()) payload.push_back(d.contentPath);
    const bool drastic = gameLaunchDescriptorKind(d) == GameLaunchProviderKind::DraStic;
    const auto tokens = libnxSplitArgv(plan.argv);

    if (!plan.viaReturnHost) {
        QA_CHECK(plan.nroPath == target);
        QA_CHECK(tokens == payload);
        QA_CHECK(plan.quitRoute == (drastic ? GameLaunchQuitRoute::EmulatorLauncher
                                            : GameLaunchQuitRoute::LoaderDefault));
        return;
    }
    QA_CHECK(o.returnHostEnabled && plan.returnHostSkipped.empty());
    QA_CHECK(plan.nroPath == o.returnHostNroPath);
    QA_CHECK(plan.quitRoute == GameLaunchQuitRoute::ReturnsToPokeBank);
    std::vector<std::string> expected{std::string(o.returnHostNroPath), std::string(o.pokeBankNroPath),
                                      drastic ? RH_MODE_RETURN_ON_LAUNCHER : RH_MODE_RETURN_ON_EXIT};
    expected.insert(expected.end(), payload.begin(), payload.end());
    QA_CHECK(tokens == expected);
    // The host receives exactly this argv from the loader and must accept it unchanged.
    RhState state;
    QA_CHECK(hostParse(tokens, state) == RH_PARSE_OK);
    QA_CHECK(std::string(state.returnPath) == o.pokeBankNroPath);
    QA_CHECK(std::string(state.childPath) == target && std::string(state.firstChildPath) == target);
    QA_CHECK(state.mode == (drastic ? RH_MODE_LAUNCHER : RH_MODE_EXIT));
    QA_CHECK(libnxSplitArgv(state.childArgv) == payload);
    char back[RH_LOADER_ARGV_CAP];
    QA_CHECK(rh_build_return_argv(&state, back, sizeof(back)));
    QA_CHECK(libnxSplitArgv(back) == std::vector<std::string>{std::string(o.pokeBankNroPath)});
}

void checkHostArgs(const std::vector<std::string>& tokens) {
    RhState state;
    const auto result = hostParse(tokens, state);
    QA_CHECK(terminatedWithin(state.returnPath, sizeof(state.returnPath)));
    QA_CHECK(terminatedWithin(state.childPath, sizeof(state.childPath)));
    QA_CHECK(terminatedWithin(state.firstChildPath, sizeof(state.firstChildPath)));
    QA_CHECK(terminatedWithin(state.childArgv, sizeof(state.childArgv)));
    if (result == RH_PARSE_OK) {
        QA_CHECK(tokens.size() == 4 || tokens.size() == 5);
        QA_CHECK(std::string(state.returnPath) == tokens[1] && std::string(state.childPath) == tokens[3]);
        const std::vector<std::string> child(tokens.begin() + 3, tokens.end());
        QA_CHECK(libnxSplitArgv(state.childArgv) == child);
        // A child that schedules nothing returns to PokeBank; the first child itself is only
        // intercepted in launcher mode after at least two hops.
        QA_CHECK(rh_next_action(&state, "") == RH_ACTION_RETURN);
        QA_CHECK(rh_next_action(&state, state.firstChildPath) == RH_ACTION_CHAIN);
        state.hops = 2;
        QA_CHECK(rh_next_action(&state, state.firstChildPath) ==
                 (state.mode == RH_MODE_LAUNCHER ? RH_ACTION_RETURN_INTERCEPTED : RH_ACTION_CHAIN));
    }
    (void)rh_next_action(&state, tokens.empty() ? nullptr : tokens.back().c_str());
}

void checkReturnSelection(std::string_view text, int64_t now) {
    GameLaunchReturnSelection parsed;
    if (!parseGameLaunchReturnSelection(text, now, parsed)) {
        QA_CHECK(parsed.gameId.empty() && parsed.savedAt == 0);
        return;
    }
    qafuzz::noteAccepted();
    std::string serialized;
    QA_CHECK(serializeGameLaunchReturnSelection(parsed, serialized));
    GameLaunchReturnSelection again;
    QA_CHECK(parseGameLaunchReturnSelection(serialized, now, again));
    QA_CHECK(again.profileIdentity == parsed.profileIdentity && again.gameId == parsed.gameId &&
             again.sourceIdentity == parsed.sourceIdentity && again.sourceKind == parsed.sourceKind &&
             again.titleId == parsed.titleId && again.userIndex == parsed.userIndex &&
             again.titleIndex == parsed.titleIndex && again.classicGamesActive == parsed.classicGamesActive &&
             again.savedAt == parsed.savedAt);
}

void runOne(std::span<const uint8_t> data) {
    Reader r{data};
    GameLaunchDescriptor d;
    d.backend = static_cast<GameLaunchBackend>(r.byte() % 4);
    d.state = static_cast<GameLaunchState>(r.byte() % 6);
    d.providerId = kProviders[r.byte() % 6];
    const uint8_t flags = r.byte();
    d.launcherPath = r.text(2600);
    d.corePath = r.text(2600);
    d.contentPath = r.text(2600);
    const std::string pokeBank = r.text(700);
    const std::string host = r.text(700);
    GameLaunchReturnOptions options;
    QA_CHECK(!options.returnHostEnabled); // opt-in: never on by default
    options.pokeBankNroPath = pokeBank;
    options.returnHostNroPath = host;
    options.returnHostEnabled = (flags & 1) != 0;
    checkPlan(d, options);
    checkPlan(d, GameLaunchReturnOptions{});

    const std::string tail = r.rest();
    // The same tail drives the host's argv parser (as the loader would tokenise it) and the
    // return-selection reader at two clock values.
    // The loader hands the host a NUL-terminated C string: nothing after the first NUL exists.
    checkHostArgs(libnxSplitArgv(tail.substr(0, tail.find('\0'))));
    checkReturnSelection(tail, 1'800'000'000);
    checkReturnSelection(tail, static_cast<int64_t>(flags) << 24);
}

qafuzz::Bytes encode(uint8_t backend, uint8_t state, uint8_t provider, uint8_t flags,
                     std::initializer_list<std::string_view> texts, std::string_view tail = {}) {
    qafuzz::Bytes out{backend, state, provider, flags};
    for (const auto text : texts) {
        out.push_back(static_cast<uint8_t>(text.size()));
        out.push_back(static_cast<uint8_t>(text.size() >> 8));
        out.insert(out.end(), text.begin(), text.end());
    }
    out.insert(out.end(), tail.begin(), tail.end());
    return out;
}

std::vector<qafuzz::Bytes> seeds() {
    const std::string record =
        "POKEBANK_LAUNCH_RETURN_V1\nprofile=user-1\ngame=black_nds\nsource=drastic:abc\n"
        "kind=3\ntitle=0100000000001234\nuser=0\nindex=7\nclassic=1\nsaved=1799999000\n";
    const std::string hostArgv =
        "\"sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro\" \"sdmc:/switch/PokeBank-NX.nro\" "
        "\"return-on-launcher\" \"sdmc:/switch/DrasticDS.nro\" \"sdmc:/roms/nds/Pokemon Black.nds\"";
    const std::string longPath = "sdmc:/" + std::string(500, 'a') + ".nro";
    return {
        // RetroArch core + content, direct; return selection in the tail.
        encode(2, 0, 0, 0, {"sdmc:/retroarch/switch/retroarch_switch.nro", "sdmc:/retroarch/cores/mgba_libretro_libnx.nro",
                            "sdmc:/roms/gba/Pokemon Emerald.gba", "sdmc:/switch/PokeBank-NX.nro",
                            "sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro"}, record),
        // DraStic homebrew NRO + content through the enabled return host; host argv in the tail.
        encode(3, 0, 1, 1, {"sdmc:/switch/DrasticDS.nro", "", "sdmc:/roms/nds/Pokemon Black.nds",
                            "sdmc:/switch/PokeBank-NX.nro", "sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro"},
               hostArgv),
        // Launcher-only melonDS, host enabled but PokeBank path unknown (direct fallback).
        encode(3, 1, 2, 1, {"sdmc:/switch/melonDS.nro", "", "sdmc:/roms/nds/Pokemon Platinum.nds", "",
                            "sdmc:/switch/PokeBank-NX/runtime/PokeBankReturnHost.nro"}),
        // Paths near the 512-byte loader limit.
        encode(3, 0, 5, 1, {longPath, "", longPath, longPath, longPath}),
    };
}

} // namespace

const qafuzz::Target& qaTarget() {
    // No checksums, count bytes or copy regions: forge/countOffsets/swaps stay empty.
    static const qafuzz::Target target{"launch-plan", runOne, seeds, {}, {}, {}};
    return target;
}
