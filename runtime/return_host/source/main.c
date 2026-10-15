// PokeBank NX RetroArch return host.
//
// The nested NRO mapping/trampoline design is adapted from switchbrew/nx-hbloader under
// its ISC license (see ../LICENSE.nx-hbloader.txt). This helper intentionally does one job:
// run the already-selected emulator NRO inside the current hbloader process, honor its
// next-load requests, and reload the exact calling PokeBank NRO when the chain ends (or, in
// launcher mode, when the emulator asks to go back to its own launcher). The decisions live in
// return_host_policy.h so host tests exercise the same code. Each launch rewrites a short
// stage log at sdmc:/switch/PokeBank-NX/runtime/return-host.log for device diagnosis.

#include <switch.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "return_host_policy.h"

#define LOG_PATH "sdmc:/switch/PokeBank-NX/runtime/return-host.log"

static RhState g_state;
static char g_nextNroPath[RH_LOADER_PATH_CAP];
static char g_nextArgv[RH_LOADER_ARGV_CAP];
static char g_argv[RH_LOADER_ARGV_CAP];
static bool g_logStarted = false;
static u8 g_savedTls[0x100];
static void* g_heapAddr;
static u64 g_heapSize;
static Handle g_procHandle = INVALID_HANDLE;
static NroHeader g_nroHeader;
static u64 g_nroMappedSize;
static AccountUid g_userIdStorage;
static u64 g_syscallHints[3];
static bool g_isApplication = false;
static enum {
    CodeMemoryUnavailable    = 0,
    CodeMemoryForeignProcess = BIT(0),
    CodeMemorySameProcess    = BIT(0) | BIT(1),
} g_codeMemoryCapability = CodeMemoryUnavailable;

u64 g_nroAddr = 0;
Result g_lastRet = 0;

extern u32 __nx_applet_type;

static const char g_noticeText[] = "PokeBank NX RetroArch return host";

void NX_NORETURN nroEntrypointTrampoline(const ConfigEntry* entries, u64 handle, u64 entrypoint);
void NX_NORETURN loadNro(void);

u32 __nx_fs_num_sessions = 1;
u32 __nx_fsdev_direntry_cache_size = 1;
bool __nx_fsdev_support_cwd = false;

void __libnx_initheap(void) {
    static char innerHeap[0x10000];
    extern char* fake_heap_start;
    extern char* fake_heap_end;
    fake_heap_start = innerHeap;
    fake_heap_end = innerHeap + sizeof(innerHeap);
}

void __appInit(void) {
    Result rc = smInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
    rc = fsInitialize();
    if (R_FAILED(rc)) diagAbortWithResult(rc);
}

void __appExit(void) {
    fsExit();
    smExit();
}

static void getIsApplication(void) {
    u64 flag = 0;
    Result rc = svcGetInfo(&flag, InfoType_IsApplication, CUR_PROCESS_HANDLE, 0);
    if (R_SUCCEEDED(rc)) {
        g_isApplication = flag != 0;
        return;
    }
    u64 currentPid = 0;
    if (R_FAILED(svcGetProcessId(&currentPid, CUR_PROCESS_HANDLE))) return;
    rc = pmshellInitialize();
    if (R_SUCCEEDED(rc)) {
        u64 applicationPid = 0;
        rc = pmshellGetApplicationProcessIdForShell(&applicationPid);
        pmshellExit();
        if (R_SUCCEEDED(rc) && currentPid == applicationPid)
            g_isApplication = true;
    }
}

static bool isKernel5xOrLater(void) {
    u64 dummy = 0;
    const Result rc =
        svcGetInfo(&dummy, InfoType_UserExceptionContextAddress, INVALID_HANDLE, 0);
    return R_VALUE(rc) != KERNELRESULT(InvalidEnumValue);
}

static bool isKernel4x(void) {
    u64 dummy = 0;
    const Result rc =
        svcGetInfo(&dummy, InfoType_InitialProcessIdRange, INVALID_HANDLE, 0);
    return R_VALUE(rc) != KERNELRESULT(InvalidEnumValue);
}

static void getCodeMemoryCapability(void) {
    if (detectMesosphere()) {
        g_codeMemoryCapability = CodeMemorySameProcess;
        return;
    }
    if (isKernel5xOrLater()) {
        Handle code = INVALID_HANDLE;
        Result rc = svcCreateCodeMemory(&code, g_heapAddr, 0x1000);
        if (R_SUCCEEDED(rc)) {
            rc = svcControlCodeMemory(code, (CodeMapOperation)-1, 0, 0x1000, 0);
            svcCloseHandle(code);
            g_codeMemoryCapability =
                R_VALUE(rc) == KERNELRESULT(InvalidEnumValue)
                    ? CodeMemorySameProcess
                    : CodeMemoryForeignProcess;
        }
        return;
    }
    if (isKernel4x()) g_codeMemoryCapability = CodeMemorySameProcess;
}

// Best-effort stage log. The first line of a launch truncates the file, so it only ever holds
// the most recent handoff. sdmc is mounted only around each write: a running child must own
// the SD session state, exactly as nx-hbloader leaves it.
static void hostLog(const char* fmt, ...) {
    char line[RH_LOADER_PATH_CAP + 160];
    va_list args;
    va_start(args, fmt);
    const int size = vsnprintf(line, sizeof(line) - 1, fmt, args);
    va_end(args);
    if (size <= 0) return;
    size_t length = (size_t)size < sizeof(line) - 1 ? (size_t)size : sizeof(line) - 2;
    line[length++] = '\n';
    if (R_FAILED(fsdevMountSdmc())) return;
    const int flags = O_WRONLY | O_CREAT | (g_logStarted ? O_APPEND : O_TRUNC);
    const int fd = open(LOG_PATH, flags, 0644);
    if (fd >= 0) {
        if (write(fd, line, length) == (ssize_t)length) g_logStarted = true;
        close(fd);
    }
    fsdevUnmountAll();
}

static void NX_NORETURN returnToPokeBank(const char* reason, Result childResult) {
    char returnArgv[RH_LOADER_PATH_CAP + 4];
    const bool argvOk = rh_build_return_argv(&g_state, returnArgv, sizeof(returnArgv)) != 0;
    if (g_state.returnPath[0] && argvOk && envHasNextLoad()) {
        envSetNextLoad(g_state.returnPath, returnArgv);
        hostLog("return reason=%s result=0x%08X target=%s", reason,
                (unsigned)childResult, g_state.returnPath);
    } else {
        hostLog("return-unavailable reason=%s result=0x%08X", reason, (unsigned)childResult);
    }
    exit(0);
}

static bool readExact(int fd, void* out, size_t size) {
    u8* p = (u8*)out;
    size_t done = 0;
    while (done < size) {
        const ssize_t got = read(fd, p + done, size - done);
        if (got <= 0) return false;
        done += (size_t)got;
    }
    return true;
}

static void unmapCurrentNro(void) {
    if (!g_nroAddr || !g_nroMappedSize) return;
    const NroHeader* header = &g_nroHeader;
    u64 rwSize = header->segments[2].size + header->bss_size;
    rwSize = (rwSize + 0xFFF) & ~0xFFFULL;

    svcBreak(BreakReason_NotificationOnlyFlag | BreakReason_PreUnloadDll,
             g_nroAddr, g_nroMappedSize);
    svcUnmapProcessCodeMemory(g_procHandle,
        g_nroAddr + header->segments[0].file_off,
        (u64)g_heapAddr + header->segments[0].file_off,
        header->segments[0].size);
    svcUnmapProcessCodeMemory(g_procHandle,
        g_nroAddr + header->segments[1].file_off,
        (u64)g_heapAddr + header->segments[1].file_off,
        header->segments[1].size);
    svcUnmapProcessCodeMemory(g_procHandle,
        g_nroAddr + header->segments[2].file_off,
        (u64)g_heapAddr + header->segments[2].file_off,
        rwSize);
    svcBreak(BreakReason_NotificationOnlyFlag | BreakReason_PostUnloadDll,
             g_nroAddr, g_nroMappedSize);
    g_nroAddr = 0;
    g_nroMappedSize = 0;
}

static Result loadImageAndMap(const char* path) {
    Result rc = fsdevMountSdmc();
    if (R_FAILED(rc)) return rc;

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fsdevUnmountAll();
        return MAKERESULT(Module_HomebrewLoader, 41);
    }

    u8* nrobuf = (u8*)g_heapAddr;
    NroStart* start = (NroStart*)nrobuf;
    NroHeader* header = (NroHeader*)(nrobuf + sizeof(NroStart));
    u8* rest = nrobuf + sizeof(NroStart) + sizeof(NroHeader);

    bool ok = readExact(fd, start, sizeof(*start)) && readExact(fd, header, sizeof(*header));
    if (!ok || header->magic != NROHEADER_MAGIC ||
        header->size < sizeof(NroStart) + sizeof(NroHeader)) {
        close(fd);
        fsdevUnmountAll();
        return MAKERESULT(Module_HomebrewLoader, 42);
    }

    const size_t restSize = header->size - sizeof(NroStart) - sizeof(NroHeader);
    const u64 rwSize =
        (header->segments[2].size + header->bss_size + 0xFFF) & ~0xFFFULL;
    const u64 imageWithBss = (u64)header->size + header->bss_size;
    if (imageWithBss < header->size) {
        close(fd);
        fsdevUnmountAll();
        return MAKERESULT(Module_HomebrewLoader, 43);
    }
    const u64 mappedSize = (imageWithBss + 0xFFF) & ~0xFFFULL;
    if (mappedSize > g_heapSize || !readExact(fd, rest, restSize)) {
        close(fd);
        fsdevUnmountAll();
        return MAKERESULT(Module_HomebrewLoader, 43);
    }
    close(fd);
    fsdevUnmountAll();

    for (int i = 0; i < 3; ++i) {
        if (header->segments[i].file_off >= header->size ||
            header->segments[i].size > header->size ||
            header->segments[i].file_off + header->segments[i].size > header->size)
            return MAKERESULT(Module_HomebrewLoader, 44);
    }

    memset(nrobuf + header->size, 0, header->bss_size);
    memcpy(&g_nroHeader, header, sizeof(g_nroHeader));
    header = &g_nroHeader;

    virtmemLock();
    void* mapAddr = virtmemFindCodeMemory(mappedSize, 0);
    if (!mapAddr) {
        virtmemUnlock();
        return MAKERESULT(Module_HomebrewLoader, 45);
    }
    rc = svcMapProcessCodeMemory(g_procHandle, (u64)mapAddr, (u64)nrobuf, mappedSize);
    virtmemUnlock();
    if (R_FAILED(rc)) return rc;

    g_nroAddr = (u64)mapAddr;
    g_nroMappedSize = mappedSize;

    rc = svcSetProcessMemoryPermission(g_procHandle,
        g_nroAddr + header->segments[0].file_off, header->segments[0].size, Perm_R | Perm_X);
    if (R_FAILED(rc)) { unmapCurrentNro(); return rc; }
    rc = svcSetProcessMemoryPermission(g_procHandle,
        g_nroAddr + header->segments[1].file_off, header->segments[1].size, Perm_R);
    if (R_FAILED(rc)) { unmapCurrentNro(); return rc; }
    rc = svcSetProcessMemoryPermission(g_procHandle,
        g_nroAddr + header->segments[2].file_off, rwSize, Perm_Rw);
    if (R_FAILED(rc)) { unmapCurrentNro(); return rc; }

    return 0;
}

void NX_NORETURN loadNro(void) {
    memcpy((u8*)armGetTls() + 0x100, g_savedTls, sizeof(g_savedTls));
    unmapCurrentNro();

    if (g_state.hops > 0) {
        // A child just returned. RetroArch's normal Quit and DraStic's launcher exit leave the
        // next-load path empty; a core restart / launcher->emulator handoff fills it in.
        const RhAction action = rh_next_action(&g_state, g_nextNroPath);
        hostLog("child-exit hop=%u result=0x%08X next=%s", g_state.hops,
                (unsigned)g_lastRet, g_nextNroPath[0] ? g_nextNroPath : "(none)");
        if (action == RH_ACTION_RETURN) returnToPokeBank("chain-ended", g_lastRet);
        if (action == RH_ACTION_RETURN_INTERCEPTED)
            returnToPokeBank("launcher-requested", g_lastRet);
        if (!rh_copy_path(g_state.childPath, sizeof(g_state.childPath), g_nextNroPath))
            returnToPokeBank("next-path-invalid", MAKERESULT(Module_HomebrewLoader, 46));
        // libnx envSetNextLoad() leaves argv empty when the child passed NULL; hbloader then
        // uses the path itself as argv[0].
        const char* nextArgv = g_nextArgv[0] ? g_nextArgv : g_nextNroPath;
        if (!rh_copy_path(g_state.childArgv, sizeof(g_state.childArgv), nextArgv))
            returnToPokeBank("next-argv-invalid", MAKERESULT(Module_HomebrewLoader, 47));
    }
    if (!rh_copy_path(g_argv, sizeof(g_argv), g_state.childArgv))
        returnToPokeBank("argv-invalid", MAKERESULT(Module_HomebrewLoader, 47));

    svcBreak(BreakReason_NotificationOnlyFlag | BreakReason_PreLoadDll,
             (uintptr_t)g_argv, sizeof(g_argv));
    const Result rc = loadImageAndMap(g_state.childPath);
    hostLog("load hop=%u rc=0x%08X path=%s", g_state.hops + 1, (unsigned)rc, g_state.childPath);
    if (R_FAILED(rc)) returnToPokeBank("load-failed", rc);

    g_nextNroPath[0] = '\0';
    g_nextArgv[0] = '\0';
    ++g_state.hops;

    const u64 childHeapStart = (u64)g_heapAddr + g_nroMappedSize;
    const u64 childHeapSize = g_heapSize - g_nroMappedSize;

    #define M EntryFlag_IsMandatory
    static ConfigEntry entries[] = {
        { EntryType_MainThreadHandle,     0, {0, 0} },
        { EntryType_ProcessHandle,        0, {0, 0} },
        { EntryType_AppletType,           0, {0, 0} },
        { EntryType_OverrideHeap,         M, {0, 0} },
        { EntryType_Argv,                 0, {0, 0} },
        { EntryType_NextLoadPath,         0, {0, 0} },
        { EntryType_LastLoadResult,       0, {0, 0} },
        { EntryType_SyscallAvailableHint, 0, {0, 0} },
        { EntryType_SyscallAvailableHint2,0, {0, 0} },
        { EntryType_RandomSeed,           0, {0, 0} },
        { EntryType_UserIdStorage,        0, {0, 0} },
        { EntryType_HosVersion,           0, {0, 0} },
        { EntryType_EndOfList,            0, {0, 0} },
    };
    #undef M

    entries[0].Value[0] = envGetMainThreadHandle();
    entries[1].Value[0] = g_procHandle;
    entries[2].Value[0] = __nx_applet_type;
    entries[2].Value[1] = 0;
    if (g_isApplication) {
        entries[2].Value[0] = AppletType_SystemApplication;
        entries[2].Value[1] = EnvAppletFlags_ApplicationOverride;
    }
    entries[3].Value[0] = childHeapStart;
    entries[3].Value[1] = childHeapSize;
    entries[4].Value[1] = (u64)(uintptr_t)g_argv;
    entries[5].Value[0] = (u64)(uintptr_t)g_nextNroPath;
    entries[5].Value[1] = (u64)(uintptr_t)g_nextArgv;
    entries[6].Value[0] = g_lastRet;
    entries[7].Value[0] = g_syscallHints[0];
    entries[7].Value[1] = g_syscallHints[1];
    entries[8].Value[0] = g_syscallHints[2];
    if (!(g_codeMemoryCapability & BIT(0)))
        entries[7].Value[1] &= ~(1ULL << (0x4B - 64));
    if (!(g_codeMemoryCapability & BIT(1)))
        entries[7].Value[1] &= ~(1ULL << (0x4C - 64));
    entries[9].Value[0] = randomGet64();
    entries[9].Value[1] = randomGet64();
    entries[10].Value[0] = (u64)(uintptr_t)&g_userIdStorage;
    entries[11].Value[0] = hosversionGet();
    entries[11].Value[1] = hosversionIsAtmosphere() ? 0x41544d4f53504852ULL : 0;
    entries[12].Value[0] = (u64)(uintptr_t)g_noticeText;
    entries[12].Value[1] = sizeof(g_noticeText);

    svcBreak(BreakReason_NotificationOnlyFlag | BreakReason_PostLoadDll,
             g_nroAddr, g_nroMappedSize);
    nroEntrypointTrampoline(entries, (u64)-1, g_nroAddr);
}

int main(int argc, char** argv) {
    memcpy(g_savedTls, (u8*)armGetTls() + 0x100, sizeof(g_savedTls));

    const RhParseResult parsed = rh_parse_args(argc, argv, &g_state);
    if (parsed != RH_PARSE_OK) {
        // Without a validated return target there is nothing safe to schedule; the outer
        // loader's default (hbmenu, or HOME for a forwarder) takes over.
        hostLog("refused argc=%d parse=%d", argc, (int)parsed);
        return 2;
    }
    hostLog("start mode=%s return=%s child=%s",
            g_state.mode == RH_MODE_LAUNCHER ? RH_MODE_RETURN_ON_LAUNCHER : RH_MODE_RETURN_ON_EXIT,
            g_state.returnPath, g_state.childPath);

    if (!envHasNextLoad() || !envHasHeapOverride()) {
        hostLog("refused loader-has-next-load=%d heap-override=%d",
                (int)envHasNextLoad(), (int)envHasHeapOverride());
        return 3;
    }
    g_heapAddr = envGetHeapOverrideAddr();
    g_heapSize = envGetHeapOverrideSize();
    g_procHandle = envGetOwnProcessHandle();
    if (!g_heapAddr || g_heapSize < 0x200000 || g_procHandle == INVALID_HANDLE)
        returnToPokeBank("environment-invalid", MAKERESULT(Module_HomebrewLoader, 48));

    getIsApplication();
    getCodeMemoryCapability();
    // nx-hbloader closes SM before running any NRO (main(): "Close SM as we don't need it
    // anymore"); the host does the same so every child starts with the loader's resource
    // picture. fs stays open for the next image load, as in nx-hbloader.
    smExit();

    for (unsigned svc = 0; svc < 0xC0; ++svc) {
        if (envIsSyscallHinted(svc))
            g_syscallHints[svc / 64] |= 1ULL << (svc % 64);
    }
    AccountUid* inheritedUser = envGetUserIdStorage();
    if (inheritedUser) g_userIdStorage = *inheritedUser;

    loadNro();
}
