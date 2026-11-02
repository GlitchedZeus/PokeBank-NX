#!/usr/bin/env python3
"""Build independent probe against an unchanged, revision-checked vendor checkout."""
import argparse, pathlib, subprocess, tempfile
PIN='bd3b16ee2c12bd9ee353cf3098cb998e7880638c'
p=argparse.ArgumentParser();p.add_argument('vendor',type=pathlib.Path);p.add_argument('--cc',default='gcc');p.add_argument('--msan',action='store_true',help='Clang only: expected uninitialized-read failure, not a passing gate');args=p.parse_args()
root=args.vendor.resolve()
sha=subprocess.check_output(['git','-C',str(root),'rev-parse','HEAD'],text=True).strip()
if sha!=PIN: raise SystemExit('Refusing unpinned vendor revision: '+sha)
with tempfile.TemporaryDirectory(prefix='gen69-signature-') as d:
    binary=pathlib.Path(d)/'probe'
    sources=[root/(x+'.c') for x in ('aes','memecrypto','mini-gmp','rsa','sha1')]
    subprocess.run([args.cc,'-std=gnu11','-O1','-g','-w',*(['-fsanitize=memory','-fPIE','-pie'] if args.msan else []),'-I'+str(root),str(pathlib.Path(__file__).with_name('signature_probe.c')),*map(str,sources),'-o',str(binary)],check=True)
    raise SystemExit(subprocess.run([str(binary)]).returncode)
