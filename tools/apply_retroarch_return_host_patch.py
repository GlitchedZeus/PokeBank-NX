#!/usr/bin/env python3
from pathlib import Path
import textwrap

ROOT = Path(__file__).resolve().parents[1]


def replace_once(rel: str, old: str, new: str) -> None:
    path = ROOT / rel
    text = path.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{rel}: expected exactly one guarded match, found {count}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


def write_new(rel: str, content: str) -> None:
    path = ROOT / rel
    if path.exists():
        raise SystemExit(f"{rel}: refusing to overwrite an existing file")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(textwrap.dedent(content).lstrip("\n"), encoding="utf-8")


replace_once(
    "include/UI/GameLauncher.h",
    "std::string suggestedGameLaunchBrowseRoot(std::string_view gameId,\n"
    "                                          std::string_view providerId,\n"
    "                                          std::string_view sourcePath);\n\n"
    "bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error);",
    "std::string suggestedGameLaunchBrowseRoot(std::string_view gameId,\n"
    "                                          std::string_view providerId,\n"
    "                                          std::string_view sourcePath);\n\n"
    "// Capture the exact PokeBank NRO that launched this process. RetroArch uses it as the\n"
    "// explicit return target through the bundled return host; other launch backends ignore it.\n"
    "void setGameLaunchReturnPath(std::string_view path);\n\n"
    "bool requestGameLaunch(const GameLaunchDescriptor& descriptor, std::string& error);",
)

replace_once(
    "src/main.cpp",
    '#include "UI/UI.h"\n#include "Utils/Logger.h"',
    '#include "UI/UI.h"\n#include "UI/GameLauncher.h"\n#include "Utils/Logger.h"',
)
replace_once("src/main.cpp", "int main()\n{", "int main(int argc, char** argv)\n{")
replace_once(
    "src/main.cpp",
    "    Utils::loadSettings();\n\n"
    "    // Exact-marker opt-in only.",
    "    Utils::loadSettings();\n\n"
    "    // Keep the exact currently running NRO path so emulator handoffs can return to this\n"
    "    // application instead of dropping to HOME/hbmenu after a normal RetroArch quit.\n"
    "    if (argc > 0 && argv && argv[0]) UI::setGameLaunchReturnPath(argv[0]);\n\n"
    "    // Exact-marker opt-in only.",
)

replace_once(
    "src/UI/GameLauncher.cpp",
    '#include "Utils/PokeBankPaths.h"\n#include "Games/GameIdentity.h"',
    '#include "Utils/PokeBankPaths.h"\n#include "Utils/FileUtilities.h"\n#include "Games/GameIdentity.h"',
)
replace_once(
    "src/UI/GameLauncher.cpp",
    'using BindingMap = std::map<std::string, StoredLaunchBinding>;\n'
    'constexpr std::string_view kBindingHeader = "POKEBANK_GAME_LAUNCH_BINDINGS_V1\\n";',
    'using BindingMap = std::map<std::string, StoredLaunchBinding>;\n'
    'constexpr std::string_view kBindingHeader = "POKEBANK_GAME_LAUNCH_BINDINGS_V1\\n";\n'
    'constexpr const char* kReturnHostRomfsPath = "romfs:/runtime/PokeBankReturnHost.nro";\n'
    'constexpr const char* kReturnHostFileName = "PokeBankReturnHost.nro";\n'
    'std::string g_gameLaunchReturnPath;',
)
replace_once(
    "src/UI/GameLauncher.cpp",
    "bool directory(const std::string& path) {\n"
    "    struct stat st{};\n"
    "    return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);\n"
    "}\n\n"
    "std::string switchPath(std::string path) {",
    "bool directory(const std::string& path) {\n"
    "    struct stat st{};\n"
    "    return !path.empty() && ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);\n"
    "}\n\n"
    "bool prepareRetroArchReturnHost(std::string& hostPath, std::string& error) {\n"
    "    if (g_gameLaunchReturnPath.empty() || !regularFile(g_gameLaunchReturnPath)) {\n"
    "        error = \"PokeBank NX cannot prove its own NRO path for the RetroArch return handoff.\";\n"
    "        return false;\n"
    "    }\n"
    "\n"
    "    const std::string runtimeDir = PokeBank::Paths::root() + \"/runtime\";\n"
    "    std::string pathError;\n"
    "    if (!PokeBank::Paths::ensureDirectoryTree(runtimeDir, &pathError)) {\n"
    "        error = pathError.empty() ? \"Could not prepare the PokeBank runtime directory.\" : pathError;\n"
    "        return false;\n"
    "    }\n"
    "\n"
    "    hostPath = runtimeDir + \"/\" + kReturnHostFileName;\n"
    "    if (!Utils::copyFile(kReturnHostRomfsPath, hostPath.c_str()) || !regularFile(hostPath)) {\n"
    "        error = \"Could not install the bundled RetroArch return host.\";\n"
    "        return false;\n"
    "    }\n"
    "    return true;\n"
    "}\n\n"
    "std::string switchPath(std::string path) {",
)
replace_once(
    "src/UI/GameLauncher.cpp",
    "} // namespace\n\n"
    "GameLaunchDescriptor resolveGameLaunch(uint64_t titleId,",
    "} // namespace\n\n"
    "void setGameLaunchReturnPath(std::string_view path) {\n"
    "#ifdef __SWITCH__\n"
    "    std::string candidate(path);\n"
    "    if (!candidate.empty() && candidate.front() == '/' && candidate.find(\":/\") == std::string::npos)\n"
    "        candidate = \"sdmc:\" + candidate;\n"
    "    g_gameLaunchReturnPath = regularFile(candidate) ? std::move(candidate) : std::string{};\n"
    "#else\n"
    "    (void)path;\n"
    "#endif\n"
    "}\n\n"
    "GameLaunchDescriptor resolveGameLaunch(uint64_t titleId,",
)

old_retro = '''    if (descriptor.backend == GameLaunchBackend::RetroArch) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        const std::string& target = descriptor.corePath.empty()
            ? descriptor.launcherPath : descriptor.corePath;
        if (!regularFile(target)) {
            error = "The RetroArch core NRO for this game is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !retroArchContentExists(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }
        std::string argv = quoted(target);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        const Result rc = envSetNextLoad(target.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "RetroArch game launch failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
'''
new_retro = '''    if (descriptor.backend == GameLaunchBackend::RetroArch) {
        if (!envHasNextLoad()) {
            error = "The current homebrew loader does not support chaining to another NRO.";
            return false;
        }
        const std::string& target = descriptor.corePath.empty()
            ? descriptor.launcherPath : descriptor.corePath;
        if (!regularFile(target)) {
            error = "The RetroArch core NRO for this game is missing.";
            return false;
        }
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !retroArchContentExists(descriptor.contentPath)) {
            error = "The linked game ROM is missing.";
            return false;
        }

        // Stock RetroArch does not remember an arbitrary caller NRO for normal Quit. Chain through
        // the tiny PokeBank-owned return host instead: it runs the exact same core + ROM, honors any
        // child envSetNextLoad request, and reloads this exact PokeBank NRO only when RetroArch
        // returns normally with no next child scheduled.
        std::string returnHostPath;
        if (!prepareRetroArchReturnHost(returnHostPath, error)) return false;

        std::string argv = quoted(returnHostPath) + " " + quoted(g_gameLaunchReturnPath) +
                           " " + quoted(target);
        if (descriptor.state != GameLaunchState::LauncherOnly &&
            !descriptor.contentPath.empty())
            argv += " " + quoted(descriptor.contentPath);
        if (argv.size() >= 2000) {
            error = "The RetroArch return handoff arguments are too long.";
            return false;
        }

        const Result rc = envSetNextLoad(returnHostPath.c_str(), argv.c_str());
        if (R_FAILED(rc)) {
            char buf[96];
            std::snprintf(buf, sizeof(buf), "RetroArch return-host launch failed (0x%08X).", static_cast<unsigned>(rc));
            error = buf;
            return false;
        }
        return true;
    }
'''
replace_once("src/UI/GameLauncher.cpp", old_retro, new_retro)

replace_once(
    "Makefile",
    "ifneq ($(ROMFS),)\n"
    "\texport NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)\n"
    "endif\n\n"
    "# Default target when you just run 'make'. Only builds.\n"
    "default: game-card-art $(BUILD)",
    "ifneq ($(ROMFS),)\n"
    "\texport NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)\n"
    "endif\n\n"
    "# The RetroArch return host is a tiny PokeBank-owned nested NRO loader. Build it first and\n"
    "# embed it in the main RomFS so the runtime can install an exact matching helper atomically.\n"
    "RETURN_HOST_DIR := $(CURDIR)/runtime/return_host\n"
    "RETURN_HOST_NRO := $(RETURN_HOST_DIR)/PokeBankReturnHost.nro\n"
    "RETURN_HOST_ROMFS := $(CURDIR)/romfs/runtime/PokeBankReturnHost.nro\n\n"
    ".PHONY: return-host\n"
    "return-host:\n"
    "\t@$(MAKE) --no-print-directory -C $(RETURN_HOST_DIR)\n"
    "\t@mkdir -p \"$(dir $(RETURN_HOST_ROMFS))\"\n"
    "\t@cp -f \"$(RETURN_HOST_NRO)\" \"$(RETURN_HOST_ROMFS)\"\n\n"
    "# Default target when you just run 'make'. Only builds.\n"
    "default: game-card-art $(BUILD)",
)
replace_once(
    "Makefile",
    "$(BUILD):\n"
    "\t@[ -d $@ ] || mkdir -p $@\n"
    "\t@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile all",
    "$(BUILD): return-host\n"
    "\t@[ -d $@ ] || mkdir -p $@\n"
    "\t@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile all",
)
replace_once(
    "Makefile",
    "clean:\n"
    "\t@printf \"clean ...\\n\"\n",
    "clean:\n"
    "\t@printf \"clean ...\\n\"\n"
    "\t@$(MAKE) --no-print-directory -C runtime/return_host clean || true\n"
    "\t@rm -f romfs/runtime/PokeBankReturnHost.nro\n",
)

replace_once(
    ".github/workflows/product-ui-native.yml",
    "      - 'src/UI/GameLauncher.cpp'\n"
    "      - 'include/UI/Gen2HeldItemPicker.h'",
    "      - 'src/UI/GameLauncher.cpp'\n"
    "      - 'src/main.cpp'\n"
    "      - 'Makefile'\n"
    "      - 'runtime/return_host/**'\n"
    "      - 'include/UI/Gen2HeldItemPicker.h'",
)
replace_once(
    ".github/workflows/product-ui-native.yml",
    '          nro = Path("PokeBankNX.nro").read_bytes()\n\n'
    '          def verify_png(path: Path):',
    '          nro = Path("PokeBankNX.nro").read_bytes()\n'
    '          return_host = Path("romfs/runtime/PokeBankReturnHost.nro").read_bytes()\n'
    '          if not return_host or return_host not in nro:\n'
    '              raise SystemExit("bundled RetroArch return host is missing from final NRO RomFS")\n\n'
    '          def verify_png(path: Path):',
)

replace_once(
    "tests/test_game_hub_contract.py",
    'launcher_source = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")\n'
    'header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")',
    'launcher_source = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")\n'
    'launcher_header = (ROOT / "include/UI/GameLauncher.h").read_text(encoding="utf-8")\n'
    'main_source = (ROOT / "src/main.cpp").read_text(encoding="utf-8")\n'
    'root_makefile = (ROOT / "Makefile").read_text(encoding="utf-8")\n'
    'return_host_source = (ROOT / "runtime/return_host/source/main.c").read_text(encoding="utf-8")\n'
    'return_host_license = (ROOT / "runtime/return_host/LICENSE.nx-hbloader.txt").read_text(encoding="utf-8")\n'
    'header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")',
)
replace_once(
    "tests/test_game_hub_contract.py",
    'require("result.corePath = defaultRetroArchCore" in launcher,\n'
    '        "stored launch metadata must not choose an arbitrary RetroArch core")\n',
    'require("result.corePath = defaultRetroArchCore" in launcher,\n'
    '        "stored launch metadata must not choose an arbitrary RetroArch core")\n'
    '\n'
    '# RetroArch normal Quit must return through the PokeBank-owned host instead of falling to HOME.\n'
    'require("setGameLaunchReturnPath(std::string_view path)" in launcher_header and\n'
    '        "UI::setGameLaunchReturnPath(argv[0]);" in main_source and\n'
    '        "int main(int argc, char** argv)" in main_source,\n'
    '        "PokeBank must capture the exact currently running NRO as the emulator return target")\n'
    'retro_start = launcher_source.index("if (descriptor.backend == GameLaunchBackend::RetroArch)")\n'
    'retro_end = launcher_source.index("if (descriptor.backend == GameLaunchBackend::HomebrewNro)", retro_start)\n'
    'retro_launch = launcher_source[retro_start:retro_end]\n'
    'require("prepareRetroArchReturnHost" in retro_launch and\n'
    '        "envSetNextLoad(returnHostPath.c_str(), argv.c_str())" in retro_launch and\n'
    '        "envSetNextLoad(target.c_str(), argv.c_str())" not in retro_launch,\n'
    '        "RetroArch must chain through the bundled return host rather than exiting directly to the outer loader")\n'
    'require("RETURN_HOST_DIR := $(CURDIR)/runtime/return_host" in root_makefile and\n'
    '        "$(BUILD): return-host" in root_makefile and\n'
    '        "romfs/runtime/PokeBankReturnHost.nro" in root_makefile,\n'
    '        "the native build must package the exact return host inside PokeBank RomFS")\n'
    'require("EntryType_NextLoadPath" in return_host_source and\n'
    '        "nroEntrypointTrampoline" in return_host_source and\n'
    '        "envSetNextLoad(g_returnPath, returnArgv)" in return_host_source and\n'
    '        "g_nextNroPath[0] == \'\\0\'" in return_host_source,\n'
    '        "the return host must honor RetroArch child chaining and reload PokeBank only after normal final return")\n'
    'require("Copyright 2017-2018 nx-hbloader Authors" in return_host_license and\n'
    '        "Permission to use, copy, modify" in return_host_license,\n'
    '        "nx-hbloader-derived return-host code must retain its permissive upstream notice")\n',
)

write_new(
    "runtime/return_host/LICENSE.nx-hbloader.txt",
    r'''
    Portions of PokeBankReturnHost are adapted from nx-hbloader.

    Copyright 2017-2018 nx-hbloader Authors

    Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
    ''',
)

write_new(
    "runtime/return_host/Makefile",
    r'''
    .SUFFIXES:

    ifeq ($(strip $(DEVKITPRO)),)
    $(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
    endif

    TOPDIR ?= $(CURDIR)
    include $(DEVKITPRO)/libnx/switch_rules

    TARGET      := PokeBankReturnHost
    BUILD       := build
    SOURCES     := source
    INCLUDES    :=
    NO_ICON     := 1
    NO_NACP     := 1

    ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
    CFLAGS      := -g -Wall -O2 -ffunction-sections $(ARCH)
    CFLAGS      += $(INCLUDE) -D__SWITCH__
    ASFLAGS     := -g $(ARCH)
    LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
    LIBS        := -lnx
    LIBDIRS     := $(LIBNX)

    ifneq ($(BUILD),$(notdir $(CURDIR)))

    export OUTPUT := $(CURDIR)/$(TARGET)
    export TOPDIR := $(CURDIR)
    export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
    export DEPSDIR := $(CURDIR)/$(BUILD)

    CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
    SFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
    export LD   := $(CC)
    export OFILES := $(CFILES:.c=.o) $(SFILES:.s=.o)
    export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                      $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                      -I$(CURDIR)/$(BUILD)
    export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

    .PHONY: all clean $(BUILD)
    all: $(BUILD)

    $(BUILD):
    	@[ -d $@ ] || mkdir -p $@
    	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

    clean:
    	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).elf $(TARGET).lst $(TARGET).map

    else

    .PHONY: all
    DEPENDS := $(OFILES:.o=.d)
    all: $(OUTPUT).nro
    $(OUTPUT).nro: $(OUTPUT).elf
    $(OUTPUT).elf: $(OFILES)
    -include $(DEPENDS)

    endif
    ''',
)

write_new(
    "runtime/return_host/source/trampoline.s",
    r'''
    // Minimal nested-NRO trampoline adapted from switchbrew/nx-hbloader (ISC).
    .section .text.nroEntrypointTrampoline, "ax", %progbits
    .align 2
    .global nroEntrypointTrampoline
    .type   nroEntrypointTrampoline, %function
    .cfi_startproc
    nroEntrypointTrampoline:
        adrp x8, __stack_top
        ldr  x8, [x8, #:lo12:__stack_top]
        mov  sp, x8

        blr  x2

        adrp x1, g_lastRet
        str  w0, [x1, #:lo12:g_lastRet]

        adrp x8, __stack_top
        ldr  x8, [x8, #:lo12:__stack_top]
        mov  sp, x8

        b    loadNro
    .cfi_endproc

    .section .text.__libnx_exception_entry, "ax", %progbits
    .align 2
    .global __libnx_exception_entry
    .type   __libnx_exception_entry, %function
    .cfi_startproc
    __libnx_exception_entry:
        adrp x7, g_nroAddr
        ldr  x7, [x7, #:lo12:g_nroAddr]
        cbz  x7, .Lfail
        br   x7
    .Lfail:
        mov w0, #0xf801
        svc 0x28
    .cfi_endproc
    ''',
)

write_new(
    "runtime/return_host/source/main.c",
    r'''
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
    ''',
)

print("RetroArch return-host patch staged successfully")
