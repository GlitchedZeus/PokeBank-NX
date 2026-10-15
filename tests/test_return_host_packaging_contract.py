from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
makefile = (ROOT / "runtime/return_host/Makefile").read_text(encoding="utf-8")
root_makefile = (ROOT / "Makefile").read_text(encoding="utf-8")


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


require("export APP_TITLE   := PokeBank Return Host" in makefile,
        "return host must own explicit NACP title metadata")
require("export APP_AUTHOR  := PokeBank NX" in makefile and
        "export APP_VERSION := 0.1.0" in makefile,
        "return host must own explicit NACP author/version metadata")
require("export NROFLAGS := --nacp=$(CURDIR)/$(TARGET).nacp" in makefile,
        "return host must replace inherited parent NRO flags with its own NACP path")
require("$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp" in makefile,
        "return-host NRO must depend on its generated NACP before elf2nro runs")
require("NO_NACP" not in makefile,
        "return host must not suppress the NACP required by current libnx switch_rules")
require("RETURN_HOST_DIR := $(CURDIR)/runtime/return_host" in root_makefile and
        "$(BUILD): return-host" in root_makefile,
        "main PokeBank build must still package the nested return host")

host_source = (ROOT / "runtime/return_host/source/main.c").read_text(encoding="utf-8")
require("getIsApplication();" in host_source and
        "EnvAppletFlags_ApplicationOverride" in host_source,
        "nested host must preserve nx-hbloader application-mode semantics")
require("getCodeMemoryCapability();" in host_source and
        "svcControlCodeMemory" in host_source,
        "nested host must derive same-process code-memory capability before launching RetroArch")
require("BreakReason_PreLoadDll" in host_source and
        "const u64 mappedSize = (imageWithBss + 0xFFF) & ~0xFFFULL;" in host_source,
        "nested host must use nx-hbloader load notifications and full image+BSS mapping geometry")
require("envGetHeapOverrideAddr()" in host_source and
        "svcSetHeapSize" not in host_source,
        "nested host must preserve the outer hbloader child heap instead of trying to re-own the process heap")


policy = (ROOT / "runtime/return_host/source/return_host_policy.h").read_text(encoding="utf-8")
require('#include "return_host_policy.h"' in host_source and
        "rh_parse_args(argc, argv, &g_state)" in host_source and
        "rh_next_action(&g_state, g_nextNroPath)" in host_source,
        "nested host must take every launch/return decision from the shared, host-tested policy")
require("'\\\\'" not in host_source and "'\\\\'" not in policy,
        "libnx argvSetup has no escape character; the host must never backslash-escape argv")
require("smExit();" in host_source.split("int main(", 1)[1],
        "nested host must close SM before running children, as nx-hbloader main() does")
require('"sdmc:/switch/PokeBank-NX/runtime/return-host.log"' in host_source and
        "O_TRUNC" in host_source,
        "nested host must leave a bounded per-launch stage log for device diagnosis")

game_launcher = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")
request = game_launcher[game_launcher.index("bool requestGameLaunch("):]
request = request[:request.index("\n}\n")]
require("buildGameLaunchChainPlan(descriptor, options)" in request and
        "envSetNextLoad(plan.nroPath.c_str(), plan.argv.c_str())" in request,
        "every homebrew handoff must go through the bounded, host-tested chain plan")
gate = request.index("if (returnHostOptedIn())")
require(request.index("prepareReturnHost(hostPath, hostError)") > gate and
        request.count("prepareReturnHost(") == 1 and
        request.count("options.returnHostEnabled = true;") == 1,
        "the return host must stay behind the explicit owner opt-in; the default stays direct")
require("std::string quoted(" not in game_launcher,
        "the escaping quoted() helper produced argv libnx cannot parse; use gameLaunchAppendArg")

print("Return-host packaging + opt-in launch-plan contract: PASS")
