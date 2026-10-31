#!/usr/bin/env python3
"""Expose swallowed reference exceptions in an isolated diagnostic-only source copy.

Never use this assembly as the official legality oracle. Only the suppression
switch changes; source-pinned official binaries and original checkout remain intact.
"""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
PIN='bfe89c74a3f5265805e364b79bc04ad38ebbc5ea'
PROGRAM='''using PKHeX.Core;
using System.Text.Json;
string? line;while((line=Console.ReadLine())!=null){var s=line.Split(' ');try{var p=new PK5(Convert.FromHexString(s[1]));var a=new LegalityAnalysis(p);Console.WriteLine(JsonSerializer.Serialize(new{id=s[0],error="",valid=a.Valid}));}catch(Exception e){Console.WriteLine(JsonSerializer.Serialize(new{id=s[0],error=e.GetType().FullName,message=e.Message,stack=e.StackTrace}));}}
'''
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--reference',type=Path,required=True);p.add_argument('--work',type=Path,required=True);p.add_argument('--dotnet',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert subprocess.check_output(['git','-C',str(a.source),'rev-parse','HEAD'],text=True).strip()==PIN
 assert not subprocess.check_output(['git','-C',str(a.source),'diff','--','PKHeX.Core'],text=True)
 assert not a.work.exists(),'use a new diagnostic work directory to preserve previous evidence'
 a.work.mkdir(parents=True);shutil.copytree(a.source/'PKHeX.Core',a.work/'PKHeX.Core',ignore=shutil.ignore_patterns('bin','obj'));shutil.copy2(a.source/'Directory.Build.props',a.work/'Directory.Build.props')
 source=a.source/'PKHeX.Core/Legality/LegalityAnalysis.cs';changed=a.work/'PKHeX.Core/Legality/LegalityAnalysis.cs';text=changed.read_text();assert text.startswith('#define SUPPRESS');changed.write_text(text.replace('#define SUPPRESS','#undef SUPPRESS',1))
 probe=a.work/'Probe';probe.mkdir();(probe/'Program.cs').write_text(PROGRAM);(probe/'Probe.csproj').write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><ImplicitUsings>enable</ImplicitUsings></PropertyGroup><ItemGroup><ProjectReference Include="../PKHeX.Core/PKHeX.Core.csproj" /></ItemGroup></Project>')
 rows=[l for l in a.reference.read_text().splitlines() if 'Misc:Error' in l];assert rows
 data=''.join(' '.join(l.split()[:2])+'\n' for l in rows);(probe/'input.txt').write_text(data)
 with (a.work/'build.log').open('w') as log:subprocess.run([str(a.dotnet.resolve()),'build',str((probe/'Probe.csproj').resolve()),'-c','Release','--ignore-failed-sources'],stdout=log,stderr=subprocess.STDOUT,check=True)
 assembly=probe/'bin/Release/net10.0/Probe.dll';result=subprocess.run([str(a.dotnet.resolve()),str(assembly.resolve())],input=data,text=True,capture_output=True,check=True);observed=[json.loads(l) for l in result.stdout.splitlines()];assert [r['id'] for r in observed]==[l.split()[0] for l in rows]
 a.output.write_text(result.stdout)
 manifest=dict(policy='Diagnostic-only; execution failure is not a legality verdict',reference=PIN,rows=len(rows),source_transform='one SUPPRESS directive disabled',files={str(x):digest(x) for x in [a.reference,source,changed,probe/'Program.cs',probe/'Probe.csproj',probe/'input.txt',assembly,assembly.parent/'PKHeX.Core.dll',a.output]})
 a.output.with_suffix('.manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 print(json.dumps(dict(cases=len(observed),errors=sum(bool(x.get('error')) for x in observed))))
if __name__=='__main__':main()
