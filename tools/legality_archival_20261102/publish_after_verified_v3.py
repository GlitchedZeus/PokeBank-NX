import pathlib,json,subprocess,time
p=pathlib.Path('outputs/archival-20261102-5fea7345').resolve();m=json.loads((p/'ARCHIVE_MANIFEST.v1.json').read_text());names=[x['name'] for x in m['assets']]+['legality-storage-evidence-v1-part09.tar.gz','legality-storage-evidence-v1-part10.tar.gz','legality-storage-evidence-v1-part09-http1.tar.gz','legality-major-2144a682-PART_B-RECONSTITUTED.tar.gz','legality-major-2144a682-CI_LOGS.tar.gz','FINAL_REPORT.md'];start=time.time()
while True:
 ready=all((p/('REMOTE_VERIFIED_'+n+'.json')).exists() for n in names)
 if ready:break
 if time.time()-start>3600:raise SystemExit('Verification incomplete; release remains draft and cleanup locked')
 time.sleep(20)
for n in names:
 d=json.loads((p/('REMOTE_VERIFIED_'+n+'.json')).read_text());assert d['download_and_sha256_verified'] and d['remote_digest_matches_local'],n
ref=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/git/ref/tags/evidence/gen1-5-2144a682-20261102']));assert ref['object']['sha']=='2144a6820490fa81c932dd2d917b5153283feefc'
info=json.loads(subprocess.check_output(['gh','api','repos/GlitchedZeus/PokeBank-NX/releases/431141822']));assert info['tag_name']=='evidence/gen1-5-2144a682-20261102' and info['prerelease']
notes='''Development / evidence archive only. This is separate from official application releases and is not release acceptance.

Frozen engine anchor: 2144a6820490fa81c932dd2d917b5153283feefc (PR #314). No merge, application integration or new application build was performed during archival. Historical candidate binaries remain labelled by their original paths/commits inside storage evidence.

All payload archives have been independently downloaded, SHA-256 checked and their contents recovered/rehashed. The storage archive preserves 13,225 original file paths plus recorded aliases; no difficult fixtures, reference errors, failures or raw disagreements were removed. See ARCHIVE_MANIFEST.v2.json.gz, VERIFICATION_REPORT.v2.json and RECOVERY_GUIDE.v6.md (attached as final archival controls).

Part B is explicitly RECONSTITUTED: the original compressed file disappeared after initial inventory, but every one of its 889 payload names/hashes and its original manifest SHA match. The original gzip bytes are not claimed recovered. All 2846+688 payload bindings from two older missing EABE archives are also preserved. A byte-identical part09-http1 duplicate from transfer diagnosis is intentionally retained and independently verified.

Known legality findings remain unchanged: 21 raw Gen IV false-Invalid disagreements, 344 fresh Gen V reference execution errors, Incomplete outcomes and unresolved original-game/integration research. No 100% completeness or acquired-specimen authenticity claim is made.

Global Codex credentials/configuration, conversations/session histories, private/unproven saves, game dumps and unreviewed local Git metadata are not public assets. No cleanup was executed. Follow the verified, file-only cleanup inventory and recovery guide; excluded private/local material remains UNSAFE TO DELETE.
'''
with (p/'RELEASE_NOTES_VERIFIED.v2.md').open('x') as f:f.write(notes)
subprocess.run(['gh','release','edit','evidence/gen1-5-2144a682-20261102','--repo','GlitchedZeus/PokeBank-NX','--draft=false','--prerelease','--latest=false','--notes-file',str(p/'RELEASE_NOTES_VERIFIED.v2.md')],check=True)
print('PUBLIC_EVIDENCE_PRERELEASE_PUBLISHED_AT_VERIFIED_ANCHOR',flush=True)
subprocess.run(['python3',str(p/'build_final_inventory_v3.py')],check=True)
with (p/'CLEANUP_DRY_RUN_VERIFICATION.log').open('x') as f:subprocess.run(['python3',str(p/'cleanup_guard_v4.py'),str(p/'CLEANUP_INVENTORY.v2.json')],stdout=f,stderr=subprocess.STDOUT,check=True)
print('FINAL_MANIFEST_AND_CLEANUP_DRY_RUN_PASS_NO_REMOVAL',flush=True)
