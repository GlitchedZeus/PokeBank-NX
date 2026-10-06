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

print("RetroArch return-host packaging contract: PASS")
