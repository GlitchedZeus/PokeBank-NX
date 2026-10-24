// P0 #320 host gates: the Product Home quit guard and the opt-in input trace.
// - Leaving PokeBank NX needs an explicit Y inside the visible decision; A, B, R, L, ZL, ZR, X,
//   the D-pad and every other single button never quit.
// - The input trace is off without its marker, records only button names / tags (never free
//   text or paths), skips idle frames, flushes every line and stays bounded.
#include "UI/HomeQuitConfirm.h"
#include "UI/InputTrace.h"

#include <cassert>
#include <cstdint>
#include <csignal>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

namespace Q = PokeBank::UIModel::HomeQuit;
namespace T = PokeBank::Diagnostics::InputTrace;

namespace {

std::string slurp(const std::string& path) {
    std::ifstream in(path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
size_t countLines(const std::string& text, const std::string& prefix) {
    size_t n = 0, pos = 0;
    while ((pos = text.find(prefix, pos)) != std::string::npos) {
        if (pos == 0 || text[pos - 1] == '\n') ++n;
        pos += prefix.size();
    }
    return n;
}

void quitDecisionTable() {
    // Every single libnx button bit 0..27: only Y (bit 3) quits, only A/B (bits 0/1) stay.
    for (int bit = 0; bit < 28; ++bit) {
        const std::uint64_t pressed = 1ull << bit;
        const Q::Decision d = Q::decide(pressed);
        if (bit == 3) assert(d == Q::Decision::Quit);
        else if (bit == 0 || bit == 1) assert(d == Q::Decision::Stay);
        else assert(d == Q::Decision::None); // R, L, ZL, ZR, X, +, -, D-pad, sticks, SL/SR
    }
    assert(Q::decide(0) == Q::Decision::None);
    // A never quits, and an ambiguous mash resolves to the safe choice.
    assert(Q::decide(Q::kButtonA) == Q::Decision::Stay);
    assert(Q::decide(Q::kButtonA | Q::kButtonY) == Q::Decision::Stay);
    assert(Q::decide(Q::kButtonB | Q::kButtonY) == Q::Decision::Stay);
    assert(Q::decide((1ull << 7) | Q::kButtonY) == Q::Decision::Quit); // R+Y: Y still explicit
    static_assert(Q::decide(Q::kButtonY) == Q::Decision::Quit);
    static_assert(Q::kButtonA == 1ull << 0 && Q::kButtonB == 1ull << 1 && Q::kButtonY == 1ull << 3);
}

void buttonNamesFollowLibnx() {
    assert(T::buttonNames(0) == "-");
    assert(T::buttonNames(1ull << 0) == "A");
    assert(T::buttonNames((1ull << 1) | (1ull << 7)) == "B|R");
    assert(T::buttonNames(1ull << 9) == "ZR");
    assert(T::buttonNames(1ull << 13) == "UP");
    assert(T::buttonNames(1ull << 27) == "RIGHT_SR");
    assert(T::buttonNames(1ull << 40) == "BIT40");
    assert(T::sanitizeTag("ov=2,title=3") == "ov=2,title=3");
    assert(T::sanitizeTag("sdmc:/switch/PokeBank NX/save.sav") == "sdmc:_switch_PokeBank_NX_save.sav");
    assert(T::sanitizeTag(std::string(400, 'a')).size() == 160);
}

void recorderIsOptInBoundedAndFlushed(const std::string& dir) {
    const std::string marker = dir + "/input_trace.enable";
    const std::string log = dir + "/input-trace.log";
    // No marker: nothing is ever written.
    {
        T::Recorder r;
        r.configure(marker, log);
        assert(!r.enabled());
        r.frame("home", 1, 1, 1, "x");
        r.event("home", "app-exit");
        std::FILE* f = std::fopen(log.c_str(), "r");
        assert(!f);
    }
    std::FILE* m = std::fopen(marker.c_str(), "w");
    assert(m && std::fclose(m) == 0);
    {
        T::Recorder r;
        r.configure(marker, log, 5);
        assert(r.enabled());
        r.tick();
        r.frame("home", 0, 1ull << 7, 0, "idle"); // held only, no edge: not logged
        r.frame("home", 1ull << 7, 1ull << 7, 1ull << 7, "ov=0,title=1");
        r.event("home", "open-title", "kind=3,intent=0");
        // Flushed per line: readable while the recorder is still open (crash safety).
        const std::string text = slurp(log);
        assert(text.find("F=1 S=home RAW=R HELD=R LOGICAL=R ST=ov=0,title=1 T=") != std::string::npos);
        assert(text.find("E=1 S=home EVENT=open-title kind=3,intent=0") != std::string::npos);
        assert(text.find("idle") == std::string::npos);
        // Bounded: after 5 lines the file restarts instead of growing.
        for (int i = 0; i < 12; ++i) r.event("home", "spin");
        const std::string bounded = slurp(log);
        assert(countLines(bounded, "E=") + countLines(bounded, "F=") <= 5);
        assert(bounded.find("# trace restarted after reaching its line limit") != std::string::npos);
    }
    std::remove(log.c_str());
    std::remove(marker.c_str());
}

void terminateIsRecordedBeforeAbort(const std::string& dir) {
    // -fno-exceptions crash guard: an uncaught library error leaves a final flushed trace line.
    const std::string marker = dir + "/input_trace.enable";
    const std::string log = dir + "/terminate.log";
    std::FILE* m = std::fopen(marker.c_str(), "w");
    assert(m && std::fclose(m) == 0);
    const pid_t child = ::fork();
    assert(child >= 0);
    if (child == 0) {
        T::recorder().configure(marker, log);
        T::installTerminateRecorder();
        T::recorder().tick();
        T::recorder().event("home", "open-title", "kind=1");
        std::terminate();
    }
    int status = 0;
    assert(::waitpid(child, &status, 0) == child);
    assert(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT); // still aborts, as before
    const std::string text = slurp(log);
    const size_t open = text.find("E=1 S=home EVENT=open-title kind=1");
    const size_t term = text.find("E=1 S=process EVENT=terminate origin=uncaught-library-error");
    assert(open != std::string::npos && term != std::string::npos && open < term);
    std::remove(log.c_str());
    std::remove(marker.c_str());
}

} // namespace

int main() {
    char pattern[] = "/tmp/pokebank_input_guard_XXXXXX";
    assert(::mkdtemp(pattern));
    const std::string dir = pattern;
    quitDecisionTable();
    buttonNamesFollowLibnx();
    recorderIsOptInBoundedAndFlushed(dir);
    terminateIsRecordedBeforeAbort(dir);
    assert(::rmdir(dir.c_str()) == 0);
    std::cout << "Home quit guard (only an explicit Y quits; A/B stay), opt-in bounded input trace and terminate recorder PASS\n";
}
