#!/usr/bin/env python3
"""Generate the offline QuickQA HTML checklist and its Markdown twin from one item list (JSON)."""
import html, json, sys, pathlib

def main(items_path, out_html, out_md):
    spec = json.load(open(items_path, encoding="utf-8"))
    ident = spec["identity"]; items = spec["items"]
    # ---------- Markdown ----------
    md = [f"# {spec['title']}", "",
          "**Status: NOT DEVICE TESTED.** Focused candidate retest. This is **not** the complete 20-game release "
          "acceptance required by [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313).", "",
          "| Identity | Value |", "| --- | --- |"]
    for k, v in ident.items(): md.append(f"| {k} | `{v}` |")
    md += ["", "Use the interactive HTML version (same items) to record results offline and export JSON. "
           "Mark each check PASS, FAIL or NOT TESTED; NOT TESTED is never a failure.", ""]
    sec = None
    for it in items:
        if it["section"] != sec:
            sec = it["section"]; md += [f"## {sec}", ""]
        md += [f"### {it['id']} · {it['name']}", f"- **Game:** {it['game']}", f"- **Get there:** {it['where']}",
               f"- **Do:** {it['do']}", f"- **Expected:** {it['expect']}",
               "- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED", "- **Comment:**", ""]
    pathlib.Path(out_md).write_text("\n".join(md).rstrip() + "\n", encoding="utf-8")
    # ---------- HTML ----------
    data = json.dumps(spec, ensure_ascii=False)
    page = """<!doctype html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1"><title>PokeBank NX QuickQA</title>
<style>
:root{--bg:#f6f7f9;--fg:#15171a;--card:#fff;--line:#d9dde3;--mut:#5b6470;--pass:#1f8a3b;--fail:#c62828;--nt:#6b7280;--acc:#2563eb}
@media (prefers-color-scheme:dark){:root{--bg:#0f1115;--fg:#e8eaed;--card:#171a20;--line:#2b303a;--mut:#9aa3ad;--acc:#60a5fa}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,sans-serif}
header{position:sticky;top:0;background:var(--card);border-bottom:1px solid var(--line);padding:10px 16px;z-index:2}
h1{font-size:17px;margin:0 0 4px}.id{font:12px ui-monospace,monospace;color:var(--mut);word-break:break-all}
.bar{display:flex;gap:8px;flex-wrap:wrap;margin-top:8px;align-items:center}
button,label.btn{border:1px solid var(--line);background:var(--card);color:var(--fg);border-radius:8px;padding:6px 10px;font-size:14px;cursor:pointer}
main{max-width:860px;margin:0 auto;padding:12px 16px 80px}h2{font-size:15px;margin:22px 0 8px;color:var(--mut);text-transform:uppercase;letter-spacing:.04em}
.c{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px;margin:10px 0}
.c h3{margin:0 0 6px;font-size:15px}.k{color:var(--mut);font-weight:600}
.r{display:flex;gap:6px;margin:8px 0 6px}.r button.on.PASS{background:var(--pass);color:#fff;border-color:var(--pass)}
.r button.on.FAIL{background:var(--fail);color:#fff;border-color:var(--fail)}.r button.on.NT{background:var(--nt);color:#fff;border-color:var(--nt)}
textarea{width:100%;min-height:44px;border:1px solid var(--line);border-radius:8px;background:transparent;color:var(--fg);padding:6px;font:inherit}
.sum{font-weight:600}.note{color:var(--mut);font-size:13px}
</style></head><body><header><h1 id="t"></h1><div class="id" id="ident"></div>
<div class="bar"><span class="sum" id="sum"></span><button id="exp">Export JSON</button>
<label class="btn">Import JSON<input type="file" id="imp" accept="application/json" hidden></label>
<button id="clr">Clear all</button></div>
<div class="note">NOT DEVICE TESTED until you test this exact NRO. Focused retest, not the full #313 release acceptance. Results save in this browser automatically.</div>
</header><main id="m"></main>
<script>
const SPEC=__DATA__;const KEY="pokebanknx-quickqa-"+SPEC.identity["Source commit"];
let st={};try{st=JSON.parse(localStorage.getItem(KEY)||"{}")}catch(e){st={}}
function save(){try{localStorage.setItem(KEY,JSON.stringify(st))}catch(e){}summ()}
function summ(){let p=0,f=0,n=0;for(const it of SPEC.items){const r=(st[it.id]||{}).result;if(r==="PASS")p++;else if(r==="FAIL")f++;else if(r==="NOT TESTED")n++}
document.getElementById("sum").textContent=`PASS ${p} · FAIL ${f} · NOT TESTED ${n} · open ${SPEC.items.length-p-f-n} / ${SPEC.items.length}`}
function esc(s){return s.replace(/[&<>]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;"}[c]))}
document.getElementById("t").textContent=SPEC.title;
document.getElementById("ident").innerHTML=Object.entries(SPEC.identity).map(([k,v])=>esc(k)+": "+esc(v)).join("<br>");
const m=document.getElementById("m");let sec=null;
for(const it of SPEC.items){if(it.section!==sec){sec=it.section;const h=document.createElement("h2");h.textContent=sec;m.appendChild(h)}
const c=document.createElement("div");c.className="c";
c.innerHTML=`<h3>${esc(it.id)} · ${esc(it.name)}</h3><div><span class="k">Game:</span> ${esc(it.game)}</div>
<div><span class="k">Get there:</span> ${esc(it.where)}</div><div><span class="k">Do:</span> ${esc(it.do)}</div>
<div><span class="k">Expected:</span> ${esc(it.expect)}</div>
<div class="r">${["PASS","FAIL","NOT TESTED"].map(x=>`<button data-v="${x}" class="${x==="NOT TESTED"?"NT":x}">${x}</button>`).join("")}</div>
<textarea placeholder="Comment / bug details"></textarea>`;
const s=st[it.id]||{};const bs=c.querySelectorAll(".r button");
function paint(){bs.forEach(b=>b.classList.toggle("on",(st[it.id]||{}).result===b.dataset.v))}
bs.forEach(b=>b.onclick=()=>{st[it.id]=st[it.id]||{};st[it.id].result=st[it.id].result===b.dataset.v?undefined:b.dataset.v;paint();save()});
const ta=c.querySelector("textarea");ta.value=s.comment||"";ta.oninput=()=>{st[it.id]=st[it.id]||{};st[it.id].comment=ta.value;save()};
paint();m.appendChild(c)}
document.getElementById("exp").onclick=()=>{const out={format:"pokebanknx-quickqa-v1",title:SPEC.title,identity:SPEC.identity,
exported_at:new Date().toISOString(),items:Object.fromEntries(SPEC.items.map(it=>[it.id,{name:it.name,result:(st[it.id]||{}).result||null,comment:(st[it.id]||{}).comment||""}]))};
const a=document.createElement("a");a.href=URL.createObjectURL(new Blob([JSON.stringify(out,null,2)],{type:"application/json"}));
a.download="PokeBank-NX-QuickQA-"+SPEC.identity["Source commit"].slice(0,8)+".json";a.click()};
document.getElementById("imp").onchange=e=>{const f=e.target.files[0];if(!f)return;f.text().then(t=>{const d=JSON.parse(t);
if(d.identity&&d.identity["NRO SHA-256"]!==SPEC.identity["NRO SHA-256"]&&!confirm("This file is for a different NRO. Import anyway?"))return;
for(const [k,v] of Object.entries(d.items||{}))st[k]={result:v.result||undefined,comment:v.comment||""};save();location.reload()})};
document.getElementById("clr").onclick=()=>{if(confirm("Clear every result and comment?")){st={};save();location.reload()}};
summ();
</script></body></html>"""
    pathlib.Path(out_html).write_text(page.replace("__DATA__", data), encoding="utf-8")

if __name__ == "__main__":
    main(*sys.argv[1:4])
