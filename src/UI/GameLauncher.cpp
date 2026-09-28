#include "UI/GameLauncher.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <string>
#include <sys/stat.h>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace UI {
namespace {

bool regularFile(const std::string& path) {
    struct stat st{};
    return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string switchPath(std::string path) {
    if (!path.empty() && path.front() == '/' && path.find(":/") == std::string::npos)
        return "sdmc:" + path;
    return path;
}

bool hasExtension(std::string_view path, std::string_view ext) {
    if (path.size() < ext.size()) return false;
    auto tail = path.substr(path.size() - ext.size());
    for (size_t i = 0; i < ext.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(tail[i])) !=
            std::tolower(static_cast<unsigned char>(ext[i]))) return false;
    return true;
}

bool readTextFile(const std::string& path, std::string& out) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return false; }
    const long size = std::ftell(f);
    if (size < 0 || size > 4 * 1024 * 1024) { std::fclose(f); return false; }
    std::rewind(f);
    out.resize(static_cast<size_t>(size));
    const size_t got = out.empty() ? 0 : std::fread(out.data(), 1, out.size(), f);
    const bool ok = got == out.size() && std::fclose(f) == 0;
    if (!ok) out.clear();
    return ok;
}

bool extractJsonStringAfter(const std::string& text, size_t keyPos,
                            std::string_view key, std::string& out) {
    const size_t colon = text.find(':', keyPos + key.size());
    if (colon == std::string::npos) return false;
    const size_t quote = text.find('"', colon + 1);
    if (quote == std::string::npos) return false;
    out.clear();
    bool escape = false;
    for (size_t i = quote + 1; i < text.size(); ++i) {
        const char c = text[i];
        if (escape) {
            if (c == '"' || c == '\\' || c == '/') out.push_back(c);
            else if (c == 'n') out.push_back('\n');
            else if (c == 't') out.push_back('\t');
            else out.push_back(c);
            escape = false;
            continue;
        }
        if (c == '\\') { escape = true; continue; }
        if (c == '"') return true;
        out.push_back(c);
    }
    out.clear();
    return false;
}

GameLaunchDescriptor resolveRetroArch(std::string_view sourcePath) {
    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::RetroArch;
    result.providerId = "retroarch";
    result.launcherPath = "sdmc:/retroarch/retroarch_switch.nro";

    if (!regularFile(result.launcherPath)) {
        result.state = GameLaunchState::LauncherMissing;
        result.detail = "RetroArch is not installed at /retroarch/retroarch_switch.nro.";
        return result;
    }

    const std::string wanted = normalizedLaunchStem(sourcePath);
    if (wanted.empty()) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "This save has no stable content basename to match.";
        return result;
    }

    DIR* dir = ::opendir("sdmc:/retroarch/playlists");
    if (!dir) {
        result.state = GameLaunchState::NeedsContentLink;
        result.detail = "No RetroArch playlist could be read; link the game file once.";
        return result;
    }

    bool sawContent = false;
    size_t filesRead = 0;
    while (const dirent* entry = ::readdir(dir)) {
        if (++filesRead > 128) break;
        const std::string name(entry->d_name);
        if (name == "." || name == ".." || !hasExtension(name, ".lpl")) continue;
        const std::string playlist = "sdmc:/retroarch/playlists/" + name;
        std::string text;
        if (!readTextFile(playlist, text)) continue;

        size_t pos = 0;
        while ((pos = text.find("\"path\"", pos)) != std::string::npos) {
            std::string content;
            if (!extractJsonStringAfter(text, pos, "\"path\"", content)) { pos += 6; continue; }
            const size_t nextPath = text.find("\"path\"", pos + 6);
            if (normalizedLaunchStem(content) != wanted) { pos += 6; continue; }

            content = switchPath(content);
            if (!regularFile(content)) { pos += 6; continue; }
            sawContent = true;

            const size_t corePos = text.find("\"core_path\"", pos + 6);
            if (corePos == std::string::npos ||
                (nextPath != std::string::npos && corePos > nextPath)) {
                pos += 6;
                continue;
            }
            std::string core;
            if (!extractJsonStringAfter(text, corePos, "\"core_path\"", core)) {
                pos += 6;
                continue;
            }
            core = switchPath(core);
            if (core == "DETECT" || core.empty() || !regularFile(core)) {
                pos += 6;
                continue;
            }

            result.contentPath = std::move(content);
            result.corePath = std::move(core);
            result.state = GameLaunchState::Ready;
            result.detail = "RetroArch playlist matched the validated save to its game file and core.";
            ::closedir(dir);
            return result;
        }
    }
    ::closedir(dir);

    result.state = GameLaunchState::NeedsContentLink;
    result.detail = sawContent
        ? "The matching RetroArch playlist does not name a usable core; link it once."
        : "No RetroArch playlist entry matches this save; link the game file once.";
    return result;
}

std::string quoted(std::string_view value) {
    std::string out = "\"";
    for (char c : value) {
        if (c == '\"' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('\"');
    return out;
}

} // namespace

GameLaunchDescriptor resolveGameLaunch(uint64_t titleId,
                                       std::string_view providerId,
                                       std::string_view sourcePath) {
    if (titleId != 0) {
        GameLaunchDescriptor result;
        result.backend = GameLaunchBackend::SwitchTitle;
        result.titleId = titleId;
        result.providerId = "switch";

#ifdef __SWITCH__
        auto* ctl = static_cast<NsApplicationControlData*>(
            std::malloc(sizeof(NsApplicationControlData)));
        if (!ctl) {
            result.state = GameLaunchState::Unavailable;
            result.detail = "Could not check whether the game is currently launchable.";
            return result;
        }
        u64 outSize = 0;
        const Result rc = nsGetApplicationControlData(
            NsApplicationControlSource_Storage, titleId, ctl,
            sizeof(NsApplicationControlData), &outSize);
        std::free(ctl);
        if (R_FAILED(rc) || outSize == 0) {
            result.state = GameLaunchState::Unavailable;
            result.detail = "The save exists, but the game is not currently installed or inserted.";
            return result;
        }
#endif
        result.state = GameLaunchState::Ready;
        result.detail = "Launch installed Nintendo Switch title.";
        return result;
    }

    if (launchProviderIsRetroArch(providerId))
        return resolveRetroArch(sourcePath);

    GameLaunchDescriptor result;
    result.backend = GameLaunchBackend::HomebrewNro;
    result.providerId = std::string(providerId);
    result.state = GameLaunchState::NeedsContentLink;
    result.detail = "This emulator save is valid, but its ROM/content path is not linked yet.";
    return result;
}

bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error) {
    error.clear();
    if (!descriptor.ready()) {
        error = descriptor.detail.empty() ? "This game is not ready to launch." : descriptor.detail;
        return false;
    }

#ifdef __SWITCH__
    if (descriptor.backend == GameLaunchBackend::SwitchTitle) {
        const Result rc = appletRequestLaunchApplication(descriptor.titleId, nullptr);
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Switch launch request failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }

    if (descriptor.backend == GameLaunchBackend::RetroArch ||
        descriptor.backend == GameLaunchBackend::HomebrewNro) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        if (!regularFile(descriptor.launcherPath)) {
            error = "The configured emulator/launcher NRO is missing.";
            return false;
        }

        std::string argv = quoted(descriptor.launcherPath);
        if (!descriptor.corePath.empty())
            argv += " -L " + quoted(descriptor.corePath);
        if (!descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);

        const Result rc = envSetNextLoad(descriptor.launcherPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "Homebrew launch request failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
#else
    (void)descriptor;
#endif

    error = "Launch is unavailable on this build.";
    return false;
}

} // namespace UI
