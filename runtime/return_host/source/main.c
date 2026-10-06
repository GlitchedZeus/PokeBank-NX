// PokeBank NX RetroArch return host.
//
// The nested NRO mapping/trampoline design is adapted from switchbrew/nx-hbloader under
// its ISC license (see ../LICENSE.nx-hbloader.txt). This helper intentionally does one job:
// run the already-selected RetroArch core/ROM inside the current hbloader process, honor any
// child next-load request, and reload the exact calling PokeBank NRO after a normal final quit.

#include <switch.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RETURN_PATH_CAP 512
#define CHILD_PATH_CAP 512
#define CHILD_ARGV_CAP 2048

static char g_returnPath[RETURN_PATH_CAP];
static char g_nextNroPath[CHILD_PATH_CAP];
static char g_nextArgv[CHILD_ARGV_CAP];
static char g_argv[CHILD_ARGV_CAP];
static u8 g_savedTls[0x100];
static void* g_heapAddr;
static u64 g_heapSize;
static Handle g_procHandle = INVALID_HANDLE;
static NroHeader g_nroHeader;
static u64 g_nroMappedSize;
static AccountUid g_userIdStorage;
static u64 g_syscallHints[3];

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

static bool copyString(char* out, size_t cap, const char* value) {
    if (!out || cap == 0 || !value) return false;
    const size_t size = strlen(value);
    if (size >= cap) return false;
    memcpy(out, value, size + 1);
    return true;
}

static bool appendQuoted(char* out, size_t cap, const char* value) {
    if (!out || !value) return false;
    size_t used = strlen(out);
    if (used + 3 >= cap) return false;
    out[used++] = '"';
    for (const char* p = value; *p; ++p) {
        if (*p == '"' || *p == '\\') {
            if (used + 2 >= cap) return false;
            out[used++] = '\\';
        } else if (used + 1 >= cap) {
            return false;
        }
        out[used++] = *p;
    }
    if (used + 2 > cap) return false;
    out[used++] = '"';
    out[used] = '\0';
    return true;
}

static bool buildInitialChildArgv(const char* core, const char* content) {
    g_nextArgv[0] = '\0';
    if (!appendQuoted(g_nextArgv, sizeof(g_nextArgv), core)) return false;
    if (content && *content) {
        const size_t used = strlen(g_nextArgv);
        if (used + 2 >= sizeof(g_nextArgv)) return false;
        g_nextArgv[used] = ' ';
        g_nextArgv[used + 1] = '\0';
        if (!appendQuoted(g_nextArgv, sizeof(g_nextArgv), content)) return false;
    }
    return true;
}

static void makeReturnArgv(char out[RETURN_PATH_CAP * 2]) {
    out[0] = '\0';
    if (!appendQuoted(out, RETURN_PATH_CAP * 2, g_returnPath)) out[0] = '\0';
}

static void NX_NORETURN returnToPokeBank(Result childResult) {
    (void)childResult;
    char returnArgv[RETURN_PATH_CAP * 2];
    makeReturnArgv(returnArgv);
    if (g_returnPath[0] && returnArgv[0] && envHasNextLoad())
        envSetNextLoad(g_returnPath, returnArgv);
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
    const u64 rwSize = (header->segments[2].size + header->bss_size + 0xFFF) & ~0xFFFULL;
    const u64 mappedSize = header->segments[2].file_off + rwSize;
    if ((u64)header->size + header->bss_size > g_heapSize || mappedSize > g_heapSize ||
        !readExact(fd, rest, restSize)) {
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

    // A normal RetroArch Quit leaves the child next-load path empty. That is the exact point
    // where we return to PokeBank. If RetroArch requested a core/frontend restart, the child
    // populated these buffers and we honor that request inside the host instead.
    if (g_nextNroPath[0] == '\0') returnToPokeBank(g_lastRet);
    if (g_nextArgv[0] == '\0' && !copyString(g_nextArgv, sizeof(g_nextArgv), g_nextNroPath))
        returnToPokeBank(MAKERESULT(Module_HomebrewLoader, 46));
    if (!copyString(g_argv, sizeof(g_argv), g_nextArgv))
        returnToPokeBank(MAKERESULT(Module_HomebrewLoader, 47));

    const Result rc = loadImageAndMap(g_nextNroPath);
    if (R_FAILED(rc)) returnToPokeBank(rc);

    g_nextNroPath[0] = '\0';
    g_nextArgv[0] = '\0';

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
    entries[3].Value[0] = childHeapStart;
    entries[3].Value[1] = childHeapSize;
    entries[4].Value[1] = (u64)(uintptr_t)g_argv;
    entries[5].Value[0] = (u64)(uintptr_t)g_nextNroPath;
    entries[5].Value[1] = (u64)(uintptr_t)g_nextArgv;
    entries[6].Value[0] = g_lastRet;
    entries[7].Value[0] = g_syscallHints[0];
    entries[7].Value[1] = g_syscallHints[1];
    entries[8].Value[0] = g_syscallHints[2];
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

    if (argc < 3 || !argv || !argv[1] || !argv[2] ||
        !copyString(g_returnPath, sizeof(g_returnPath), argv[1]) ||
        !copyString(g_nextNroPath, sizeof(g_nextNroPath), argv[2]) ||
        !buildInitialChildArgv(argv[2], argc >= 4 ? argv[3] : NULL))
        return 2;

    if (!envHasNextLoad() || !envHasHeapOverride()) return 3;
    g_heapAddr = envGetHeapOverrideAddr();
    g_heapSize = envGetHeapOverrideSize();
    g_procHandle = envGetOwnProcessHandle();
    if (!g_heapAddr || g_heapSize < 0x200000 || g_procHandle == INVALID_HANDLE)
        returnToPokeBank(MAKERESULT(Module_HomebrewLoader, 48));

    for (unsigned svc = 0; svc < 0xC0; ++svc) {
        if (envIsSyscallHinted(svc))
            g_syscallHints[svc / 64] |= 1ULL << (svc % 64);
    }
    AccountUid* inheritedUser = envGetUserIdStorage();
    if (inheritedUser) g_userIdStorage = *inheritedUser;

    loadNro();
}
