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


game_launcher = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")
retro_start = game_launcher.index("if (descriptor.backend == GameLaunchBackend::RetroArch)")
homebrew_start = game_launcher.index("if (descriptor.backend == GameLaunchBackend::HomebrewNro)", retro_start)
retro_block = game_launcher[retro_start:homebrew_start]
require("envSetNextLoad(target.c_str(), argv.c_str())" in retro_block,
        "device-proven RetroArch startup must chain directly to the resolved target")
require("prepareRetroArchReturnHost" not in retro_block and
        "envSetNextLoad(returnHostPath.c_str()" not in retro_block,
        "experimental return host must stay out of the hardware-critical RetroArch startup path")

print("RetroArch return-host packaging + direct-startup contract: PASS")
