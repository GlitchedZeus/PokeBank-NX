# Research validation

Use Python 3, a C99 compiler and a C++17 compiler. Place the external repositories in one `SOURCE_ROOT` directory under the names in [repositories.json](evidence/repositories.json), checked out at those exact revisions. The PokeBank NX Git object database must also contain the accepted and pending revisions cited by the source registry. No ROM, save or event binary is required.

From the repository root:

```sh
python docs/legality/research/scripts/validate_research.py SOURCE_ROOT
python docs/legality/research/scripts/reproduce_inheritance.py SOURCE_ROOT
python docs/legality/research/scripts/reproduce_nature_window.py SOURCE_ROOT
python docs/legality/research/scripts/check_emerald_pending.py SOURCE_ROOT
git diff --check
```

`inventory_sources.py SOURCE_ROOT` refreshes baseline inventory, resource manifests and selected facts; it preserves supplemental live-reconciliation registry records. It records checkout revisions, so use the pinned checkouts rather than silently replacing the evidence with future versions. The reproduction scripts reject drift in their retail inputs. The source validator hashes Git objects at each registry revision, including later NX snapshots.

## Executed evidence

| Check | Result | Evidence and limits |
|---|---|---|
| Original Ruby/FireRed/Emerald inheritance C vs independent Python | 24/24 pass | [inheritance-vectors.json](evidence/inheritance-vectors.json); eight seeds per game, explicit stage stub inputs, no full egg legitimacy claim |
| HGSS original nature constructor vs Python and bounded NX helper | Pass | [nature-window-vector.json](evidence/nature-window-vector.json); 129-rejection synthetic generation stage; no complete native encounter |
| Pending #247 Emerald table vs pinned PKHeX | 386 species / 3,947 ordered pairs match | [emerald-levelup-crosscheck.json](evidence/emerald-levelup-crosscheck.json); possible shared provenance, not independent retail execution |
| Five existing focused helper executables | Pass | [focused-validation.json](evidence/focused-validation.json); original accepted baseline |
| Existing ball-domain analyzer and BCC multi-retry native PK4 executables | Both pass | [native-validation.json](evidence/native-validation.json); synthetic native-format fixtures at original baseline |
| Registry bytes, CSV structure, relative document links and tracked changed-file scope | Pass after final document completion | Run `validate_research.py`; output recorded in [document-validation.json](evidence/document-validation.json) |

Focused tests were compiled with `g++ -std=c++17 -O2 -Iinclude` from the matching `tests/<test-name>.cpp` files. The two analyzer targets were built and run using:

```sh
make -f Makefile.host HOST_BUILD=/tmp/nx-research-host \
  /tmp/nx-research-host/test_gen34_ball_domain_analyzer \
  /tmp/nx-research-host/test_gen4_bcc_multiretry_native_pk4 -j2
/tmp/nx-research-host/test_gen34_ball_domain_analyzer
/tmp/nx-research-host/test_gen4_bcc_multiretry_native_pk4
```

Those tests ran against the research branch's unchanged production baseline `fecfbca9`, not the entire later accepted production tree. Build output included a missing optional PCG vendor-directory lookup warning; the selected targets compiled and executed successfully.

The 70-row catalog comprises 24 verified inheritance-stage outputs, 24 fixed-seed/parent contradictory-output controls, one verified long nature-stage witness plus one fixed-output control, 13 unexecuted scenario proposals, five inspected upstream Gen II test expectations, and two rows pointing to existing native test suites. A contradictory fixed-output control disproves that exact construction, not every possible Pokémon history.

No full host suite, .NET test suite, Switch build or sanitizer run was performed for this documentation-only change. Proposed scenario rows are not passing tests. Retail functions are extracted to temporary files for the local harness; their source bodies are not committed.

## Tranche 2 validation

Run:

```sh
python docs/legality/research/scripts/audit_parent_slot_binding.py SOURCE_ROOT
python docs/legality/research/scripts/reproduce_parent_eligibility.py SOURCE_ROOT
python docs/legality/research/scripts/validate_research.py SOURCE_ROOT
```

At pinned accepted engine `fe3e3d05`, the first script confirms 48 direct retail-stage agreements and reproduces 24 incorrect candidate-local joint rejections. This is an expected defect reproduction, not a production fix test. The second passes 17,496 original-function/independent-model eligibility comparisons across three retail sources plus 24 Emerald role cases. All copied retail/NX inputs exist only in temporary directories. Python with safe tar extraction filters, a C compiler and C++17 are required. The historical counts above describe the first tranche; the expanded registry has 76 sources and the catalog 143 rows.

## Tranche 3 validation

`python docs/legality/research/scripts/reproduce_gen4_egg_ivs.py SOURCE_ROOT` passes 25,382 original-C/Python stage comparisons and the same number of known-branch inverse checks. It saves 138 detailed cases and full matrix digests. All are synthetic stages with explicit initial child/parent IV stubs, not native egg records. The newest source registry has 79 entries and the catalog 145 rows. See [Tranche 3](TRANCHE_3_GEN4_EGG_RNG.md) for integration and remaining research.
