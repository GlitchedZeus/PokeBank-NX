/*
 * PokeBank NX return host: pure launch/return policy.
 *
 * Everything here is free of libnx so the exact same decisions are compiled into the
 * PokeBankReturnHost NRO and into the host tests (tests/test_game_launch_model.cpp).
 *
 * References (pinned):
 *  - switchbrew/nx-hbloader 82b95122c5ae8dc059bf23893ba7623c72c86773, source/main.c:
 *    the loader owns 512-byte next-load path and 2048-byte next-load argv buffers, and when a
 *    child exits with an empty next-load path it loads DEFAULT_NRO "sdmc:/hbmenu.nro".
 *  - switchbrew/libnx 20cbcfd90567f7bf84df7725f27e345e2164fa6b:
 *    runtime/env.c envSetNextLoad() strcpy()s into those loader buffers without a bound;
 *    runtime/argv.c argvSetup() splits on whitespace, treats '"' as an argument delimiter
 *    only, and has NO escape character, so a backslash is literal and an embedded '"' can
 *    never be represented.
 *  - libretro/RetroArch b2538f1249e81d687dbcaece491674713a38ae68,
 *    frontend/drivers/platform_switch.c: a normal Quit leaves switch_fork_mode at
 *    FRONTEND_FORK_NONE, so frontend_switch_exitspawn() schedules nothing.
 *  - NaGaa95/DrasticDS_nx 3698668ba5ff01ec7fd721e178e5c59bf57c020b: the DrasticDS.nro
 *    launcher records its own argv[0] as Wrapper/LauncherPath (launcher/source/forwarder.cpp
 *    resolveLauncherPath, normalising "/switch/..." to "sdmc:/switch/...") and chains to
 *    .emu/DrasticDS_nx.nro; the in-game "Quit to launcher" then calls
 *    envSetNextLoad(launcher_path, launcher_path) (source/main.c).
 */
#ifndef POKEBANK_RETURN_HOST_POLICY_H
#define POKEBANK_RETURN_HOST_POLICY_H

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* nx-hbloader g_nextNroPath / g_nextArgv sizes. */
#define RH_LOADER_PATH_CAP 512
#define RH_LOADER_ARGV_CAP 2048

#define RH_MODE_RETURN_ON_EXIT "return-on-exit"
#define RH_MODE_RETURN_ON_LAUNCHER "return-on-launcher"

typedef enum {
    /* Run the child; honour every next-load it requests; return to PokeBank only when the
     * final child exits with nothing scheduled (RetroArch Quit). */
    RH_MODE_EXIT = 0,
    /* Additionally treat a later request to reload the very first child (the emulator's
     * own launcher NRO) as "quit to launcher" and return to PokeBank instead (DraStic). */
    RH_MODE_LAUNCHER = 1
} RhMode;

typedef enum {
    RH_PARSE_OK = 0,
    RH_PARSE_ARGC = 1,
    RH_PARSE_RETURN_PATH = 2,
    RH_PARSE_MODE = 3,
    RH_PARSE_CHILD_PATH = 4,
    RH_PARSE_CHILD_ARGV = 5
} RhParseResult;

typedef enum {
    RH_ACTION_RETURN = 0,            /* nothing scheduled: reload PokeBank */
    RH_ACTION_CHAIN = 1,             /* load the requested child inside the host */
    RH_ACTION_RETURN_INTERCEPTED = 2 /* child asked for its launcher: reload PokeBank */
} RhAction;

typedef struct {
    char returnPath[RH_LOADER_PATH_CAP];
    char firstChildPath[RH_LOADER_PATH_CAP];
    char childPath[RH_LOADER_PATH_CAP];
    char childArgv[RH_LOADER_ARGV_CAP];
    RhMode mode;
    unsigned hops; /* children started so far */
} RhState;

/* An argument survives libnx argvSetup() unchanged only if it is non-empty and contains no
 * '"' and no control characters (whitespace is fine inside quotes). */
static inline int rh_arg_representable(const char* value) {
    if (!value || !*value) return 0;
    for (const char* p = value; *p; ++p) {
        const unsigned char c = (unsigned char)*p;
        if (c == '"' || c < 0x20 || c == 0x7F) return 0;
    }
    return 1;
}

/* Appends ` "value"` (no leading space when out is empty). Never escapes: libnx has no escape
 * character, so an unrepresentable value is refused instead of being silently corrupted. */
static inline int rh_append_quoted(char* out, size_t cap, const char* value) {
    if (!out || cap == 0 || !rh_arg_representable(value)) return 0;
    const size_t used = strlen(out);
    const size_t len = strlen(value);
    const size_t sep = used ? 1u : 0u;
    if (used + sep + len + 2 >= cap) return 0;
    char* p = out + used;
    if (sep) *p++ = ' ';
    *p++ = '"';
    memcpy(p, value, len);
    p += len;
    *p++ = '"';
    *p = '\0';
    return 1;
}

static inline int rh_copy_path(char* out, size_t cap, const char* value) {
    if (!out || !value || !*value) return 0;
    const size_t len = strlen(value);
    if (len >= cap) return 0;
    memcpy(out, value, len + 1);
    return 1;
}

static inline int rh_lower_ascii(int c) {
    return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

/* Same SD file? Accepts "sdmc:/x", "/x" and case differences (FAT is case-insensitive). */
static inline int rh_same_sd_path(const char* a, const char* b) {
    if (!a || !b || !*a || !*b) return 0;
    if (strncmp(a, "sdmc:", 5) == 0) a += 5;
    if (strncmp(b, "sdmc:", 5) == 0) b += 5;
    for (; *a && *b; ++a, ++b)
        if (rh_lower_ascii((unsigned char)*a) != rh_lower_ascii((unsigned char)*b)) return 0;
    return *a == '\0' && *b == '\0';
}

/* argv protocol: <host> <returnNro> <mode> <childNro> [content]. The child's own argv[0] is
 * its NRO path because libnx mounts an NRO's RomFS from argv[0] (romfs_dev.c romfsMountSelf). */
static inline RhParseResult rh_parse_args(int argc, char* const* argv, RhState* state) {
    if (!state) return RH_PARSE_ARGC;
    memset(state, 0, sizeof(*state));
    if (argc < 4 || argc > 5 || !argv) return RH_PARSE_ARGC;
    if (!rh_arg_representable(argv[1]) ||
        !rh_copy_path(state->returnPath, sizeof(state->returnPath), argv[1]))
        return RH_PARSE_RETURN_PATH;
    if (!argv[2]) return RH_PARSE_MODE;
    if (strcmp(argv[2], RH_MODE_RETURN_ON_EXIT) == 0) state->mode = RH_MODE_EXIT;
    else if (strcmp(argv[2], RH_MODE_RETURN_ON_LAUNCHER) == 0) state->mode = RH_MODE_LAUNCHER;
    else return RH_PARSE_MODE;
    if (!rh_arg_representable(argv[3]) ||
        !rh_copy_path(state->childPath, sizeof(state->childPath), argv[3]) ||
        !rh_copy_path(state->firstChildPath, sizeof(state->firstChildPath), argv[3]))
        return RH_PARSE_CHILD_PATH;
    if (!rh_append_quoted(state->childArgv, sizeof(state->childArgv), argv[3]))
        return RH_PARSE_CHILD_ARGV;
    if (argc == 5 && !rh_append_quoted(state->childArgv, sizeof(state->childArgv), argv[4]))
        return RH_PARSE_CHILD_ARGV;
    return RH_PARSE_OK;
}

/* Decide what follows a child's exit. requestedPath/requestedArgv are what the child left in
 * the host's next-load buffers (empty path = nothing scheduled). */
static inline RhAction rh_next_action(const RhState* state, const char* requestedPath) {
    if (!state || !requestedPath || !*requestedPath) return RH_ACTION_RETURN;
    if (state->mode == RH_MODE_LAUNCHER && state->hops >= 2 &&
        rh_same_sd_path(requestedPath, state->firstChildPath))
        return RH_ACTION_RETURN_INTERCEPTED;
    return RH_ACTION_CHAIN;
}

/* Argument string the outer loader receives to bring PokeBank back. */
static inline int rh_build_return_argv(const RhState* state, char* out, size_t cap) {
    if (!state || !out || cap == 0) return 0;
    out[0] = '\0';
    return rh_append_quoted(out, cap, state->returnPath);
}

#ifdef __cplusplus
}
#endif

#endif
