#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>
#include <sys/stat.h>

// Opt-in hardware input diagnostics (P0 #320). Off unless the owner creates the marker file;
// then every frame with a new button edge, and every exit / open / dispatch decision, is
// appended as one line to a bounded log the owner can copy off the SD card:
//
//   F=<frame> S=<screen> RAW=<pressed> HELD=<held> LOGICAL=<post-navigation> ST=<state>
//   E=<frame> S=<screen> EVENT=<origin> <detail>
//
// Each line is flushed immediately so a crash or abort keeps everything before it. Only button
// names, screen/overlay/focus tags and decision origins are recorded: never save contents,
// trainer or Pokemon names, account ids or file paths. Header-only and libnx-free (button bit
// names follow switch/services/hid.h HidNpadButton) so it is host-testable.
namespace PokeBank::Diagnostics::InputTrace {

// libnx HidNpadButton bit order (switch/services/hid.h).
inline constexpr const char* kButtonNames[] = {
    "A", "B", "X", "Y", "LS", "RS", "L", "R", "ZL", "ZR", "PLUS", "MINUS",
    "LEFT", "UP", "RIGHT", "DOWN", "LS_LEFT", "LS_UP", "LS_RIGHT", "LS_DOWN",
    "RS_LEFT", "RS_UP", "RS_RIGHT", "RS_DOWN", "LEFT_SL", "LEFT_SR", "RIGHT_SL", "RIGHT_SR",
};
inline constexpr int kNamedButtons = static_cast<int>(sizeof(kButtonNames) / sizeof(kButtonNames[0]));

[[nodiscard]] inline std::string buttonNames(std::uint64_t mask) {
    if (mask == 0) return "-";
    std::string out;
    for (int bit = 0; bit < 64; ++bit) {
        if (!(mask & (1ull << bit))) continue;
        if (!out.empty()) out += '|';
        out += bit < kNamedButtons ? kButtonNames[bit] : ("BIT" + std::to_string(bit));
    }
    return out;
}

// Keeps a tag to [A-Za-z0-9_.:=,-] so no free text (and no path or name) can enter the log.
[[nodiscard]] inline std::string sanitizeTag(const std::string& tag) {
    std::string out;
    out.reserve(tag.size());
    for (const char c : tag) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '_' || c == '.' || c == ':' || c == '=' || c == ',' || c == '-';
        out += ok ? c : '_';
    }
    return out.substr(0, 160);
}

class Recorder {
public:
    // Enables tracing only when `markerPath` exists as a regular file. Safe to call again.
    void configure(const std::string& markerPath, const std::string& logPath,
                   std::size_t maxLines = 4000) {
        close();
        struct stat st{};
        enabled_ = ::stat(markerPath.c_str(), &st) == 0 && S_ISREG(st.st_mode);
        logPath_ = logPath;
        maxLines_ = maxLines == 0 ? 1 : maxLines;
        lines_ = 0;
        if (enabled_) open("w");
    }
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }
    [[nodiscard]] std::size_t linesWritten() const noexcept { return lines_; }

    void tick() noexcept { ++frame_; }

    // One line per frame that has any new edge or logical event; idle frames are not logged.
    void frame(const char* screen, std::uint64_t pressed, std::uint64_t held,
               std::uint64_t logical, const std::string& state) {
        if (!enabled_ || (pressed == 0 && logical == 0)) return;
        write("F=" + std::to_string(frame_) + " S=" + sanitizeTag(screen) +
              " RAW=" + buttonNames(pressed) + " HELD=" + buttonNames(held) +
              " LOGICAL=" + buttonNames(logical) + " ST=" + sanitizeTag(state));
    }
    void event(const char* screen, const char* origin, const std::string& detail = {}) {
        if (!enabled_) return;
        write("E=" + std::to_string(frame_) + " S=" + sanitizeTag(screen) +
              " EVENT=" + sanitizeTag(origin) + (detail.empty() ? "" : " " + sanitizeTag(detail)));
    }

    ~Recorder() { close(); }

private:
    void open(const char* mode) {
        file_ = std::fopen(logPath_.c_str(), mode);
        if (!file_) { enabled_ = false; return; }
        std::fputs("# PokeBank NX input trace (opt-in, no save data, no names, no paths)\n", file_);
        std::fflush(file_);
    }
    void close() {
        if (file_) std::fclose(file_);
        file_ = nullptr;
    }
    void write(const std::string& line) {
        if (!file_) return;
        if (lines_ >= maxLines_) { // bounded: restart the file rather than grow without limit
            close();
            open("w");
            if (!file_) return;
            std::fputs("# trace restarted after reaching its line limit\n", file_);
            lines_ = 0;
        }
        std::fputs(line.c_str(), file_);
        std::fputc('\n', file_);
        std::fflush(file_); // a crash or abort keeps every line before it
        ++lines_;
    }

    bool enabled_ = false;
    std::string logPath_;
    std::size_t maxLines_ = 4000;
    std::size_t lines_ = 0;
    std::uint64_t frame_ = 0;
    std::FILE* file_ = nullptr;
};

inline Recorder& recorder() {
    static Recorder instance;
    return instance;
}

// Crash guard for the -fno-exceptions build. A library throw (out_of_range, bad_alloc, ...) has
// no handler there, so it reaches std::terminate and the process silently returns to the Switch
// menu, which looks like "exited without warning". try/catch cannot be compiled in this build,
// so the guard records the terminate as the final, already-flushed trace line, giving a device
// run the screen and frame where it happened, and only then aborts as before.
inline void recordTerminateThenAbort() noexcept {
    Recorder& r = recorder();
    if (r.enabled()) r.event("process", "terminate", "origin=uncaught-library-error");
    std::abort();
}
inline void installTerminateRecorder() noexcept { std::set_terminate(&recordTerminateThenAbort); }

} // namespace PokeBank::Diagnostics::InputTrace
