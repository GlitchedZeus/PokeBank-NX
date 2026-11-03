#!/usr/bin/env python3
"""Master Vault v1 wiring contract (docs/roadmap/VAULT_V1_IMPLEMENTATION_PLAN.md).

- The Vault is OFF by default: VaultFeature.h defaults the flag to 0 and the device Makefile only adds
  -DPOKEBANK_ENABLE_VAULT=1 under `make POKEBANK_VAULT=1`.
- ONE shared entry: every Gen I-V box sheet dispatches Copy to Vault / Withdraw from Vault to the shared
  UI/PokemonFileMenus (copyToVault / beginWithdraw), the Gen I-III Party sheet dispatches Copy to Vault,
  and the one Vault browser (UI/VaultDialog) is modal in the composite screen like the file dialog.
- Copy, not Move: copyToVault never stages, removes or writes a save; withdraw stages only through the
  same per-generation import callbacks Import from file uses.
- The storage core uses no exceptions / RTTI (device build: -fno-rtti -fno-exceptions).
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
failures = []


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def require(ok, what):
    if not ok:
        failures.append(what)


feature = read("include/Vault/VaultFeature.h")
require("#define POKEBANK_ENABLE_VAULT 0" in feature, "VaultFeature.h must default the Vault flag to 0")
makefile = read("Makefile")
require(re.search(r"ifeq \(\$\(POKEBANK_VAULT\),1\)\s*\nDEFINES\s*\+=\s*-DPOKEBANK_ENABLE_VAULT=1\s*\nendif", makefile),
        "Makefile: -DPOKEBANK_ENABLE_VAULT=1 only under POKEBANK_VAULT=1")
require(makefile.count("POKEBANK_ENABLE_VAULT") == 1, "Makefile must not enable the Vault anywhere else")
require("src/Vault" in makefile, "Makefile SOURCES must build src/Vault")

# Every generation's sheet reaches the shared entry points.
sites = {
    1: ("src/UI/ClassicReleaseActionFix.inc", "copyToVault(screen, 1,", "beginWithdraw(screen, 1,"),
    2: ("src/UI/ClassicReleaseActionFix.inc", "copyToVault(screen, 2,", "beginWithdraw(screen, 2,"),
    3: ("src/UI/Gen3SharedPokemonSurface.inc", "copyToVault(screen, 3,", "beginWithdraw(screen, 3,"),
    4: ("src/UI/Gen4SharedPokemonSurface.inc", "copyToVault(screen, 4,", "beginWithdraw(screen, 4,"),
    5: ("src/UI/Gen5SharedReadOnlySurface.inc", "copyToVault(screen,5,", "beginWithdraw(screen,5,"),
}
for gen, (path, copy, withdraw) in sites.items():
    text = read(path)
    require(copy in text, f"Gen {gen}: {path} must dispatch Copy to Vault to PokemonFileMenus::copyToVault")
    require(withdraw in text, f"Gen {gen}: {path} must dispatch Withdraw from Vault to PokemonFileMenus::beginWithdraw")
require("PokemonFileMenus::copyToVault(screen,gen,true,0,slot)" in read("src/UI/ClassicPartyActions.inc"),
        "Gen I-III Party sheet must dispatch Copy to Vault")
gen4 = read("src/UI/Gen4SharedPokemonSurface.inc")
require("kCopyToVaultLabel" in gen4 and "kWithdrawFromVaultLabel" in gen4 and "PokeBank::Vault::kEnabled" in gen4,
        "Gen IV's own list must use the shared Vault labels behind the build flag")

composite = read("src/UI/TrainerViewScreenCompositeOverlay.cpp")
require("VaultDialog::handleInput(*this" in composite and "VaultDialog::drawDialog(screen, fb)" in composite,
        "the Vault browser must be modal in the composite screen")

menus = read("src/UI/PokemonFileMenus.inc")
m = re.search(r"bool copyToVault\(.*?\n}\n", menus, re.S)
require(m is not None, "PokemonFileMenus.inc must define copyToVault")
if m:
    body = m.group(0)
    for banned in (r"\bstage\w*\(", r"Remove", r"[Rr]elease", r"finaliz", r"DurableFile", r"writeback", r"\bImport\("):
        require(not re.search(banned, body), f"copyToVault must not touch the save ({banned!r} found)")
    require("exportBytes(" in body and "deposit(" in body, "copyToVault deposits exactly the Export to file record")
w = re.search(r"bool beginWithdraw\(.*?\n}\n", menus, re.S)
require(w is not None, "PokemonFileMenus.inc must define beginWithdraw")
if w:
    body = w.group(0)
    for gen, cb in ((1, "gen1Import(screen, box)"), (2, "gen2Import(screen, box)"), (3, "gen3Import(screen, box, slot)"),
                    (4, "gen4Import(screen, box, slot)"), (5, "gen5Import(screen, box, slot)")):
        require(cb in body, f"Withdraw for Gen {gen} must stage through the Import from file callback")

for rel in ("src/Vault/VaultStore.cpp", "include/Vault/VaultStore.h", "include/Vault/VaultSearch.h", "src/UI/VaultDialog.cpp"):
    text = read(rel)
    for banned in (r"\bthrow\b", r"\btry\s*\{", r"dynamic_cast", r"\btypeid\b"):
        require(not re.search(banned, text), f"{rel}: {banned} is not allowed in the device build")

if failures:
    for f in failures:
        print("FAIL:", f)
    sys.exit(1)
print("test_vault_wiring_contract: PASS")
