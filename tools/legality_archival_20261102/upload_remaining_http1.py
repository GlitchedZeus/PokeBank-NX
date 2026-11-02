import pathlib,json,subprocess,concurrent.futures,urllib.parse,hashlib
p=pathlib.Path('outputs/archival-20261102-5fea7345').resolve();m=json.loads((p/'ARCHIVE_MANIFEST.v1.json').read_text());names=[a for a in m['assets'] if a['name']!='legality-storage-evidence-v1-part01.tar.gz'];repo='GlitchedZeus/PokeBank-NX';rid=431141822
info=json.loads(subprocess.check_output(['gh','api',f'repos/{repo}/releases/{rid}']));existing={a['name']:a for a in info['assets']};token=subprocess.check_output(['gh','auth','token'],text=True).strip()
def upload(a):
 name=a['name'];f=p/name;assert hashlib.file_digest(f.open('rb'),'sha256').hexdigest()==a['sha256']
 if name in existing:
  x=existing[name];assert x['state']=='uploaded' and x['size']==a['bytes'] and x['digest']=='sha256:'+a['sha256'];return (name,'already_uploaded')
 url=f'https://uploads.github.com/repos/{repo}/releases/{rid}/assets?name='+urllib.parse.quote(name,safe='')
 with (p/('HTTP1_PROGRESS_PRIVATE_'+name+'.log')).open('x') as err:
  z=subprocess.run(['curl','--config','-','--http1.1','--connect-timeout','20','--max-time','1800','--request','POST','--header','Content-Type: application/gzip','--data-binary','@'+str(f),'--output',str(p/('HTTP1_RESPONSE_'+name+'.json')),'--write-out','%{http_code} %{size_upload} %{speed_upload} %{time_total}',url],input='header = "Authorization: Bearer '+token+'"\n',text=True,stdout=subprocess.PIPE,stderr=err)
 assert z.returncode==0,(name,z.returncode,z.stdout)
 result=json.loads((p/('HTTP1_RESPONSE_'+name+'.json')).read_text());assert result.get('state')=='uploaded' and result.get('name')==name and result.get('size')==a['bytes'] and result.get('digest')=='sha256:'+a['sha256'],(name,result.get('message'))
 print(name,'UPLOADED',z.stdout,flush=True);return name,'uploaded'
with concurrent.futures.ThreadPoolExecutor(max_workers=5) as pool:list(pool.map(upload,names))
print('ALL_REMAINING_UPLOADS_COMPLETE',flush=True)
