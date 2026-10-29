#!/usr/bin/env python3
"""Prepare a narrow UI patch from a verified #307 snapshot, never modify that checkout."""
import argparse, difflib, hashlib, json, pathlib, subprocess
PIN = '9d4b8b71befea78c76802c15f1911a0ca24e775c'
p = argparse.ArgumentParser(); p.add_argument('app', type=pathlib.Path); p.add_argument('output', type=pathlib.Path); p.add_argument('--head', default=PIN); p.add_argument('--gen3-provider', action='store_true'); p.add_argument('--presentation-only', action='store_true', help='Only the shared report/modal patch; leave the current Gen V renderer to its owner'); a=p.parse_args(); PIN=a.head
assert subprocess.check_output(['git','-C',str(a.app),'rev-parse','HEAD'], text=True).strip()==PIN
changes={}
def read(path):
    s=(a.app/path).read_text(); changes[path]=[s,s]; return s
def replace(s, old, new):
    assert s.count(old)==1, old[:100]
    return s.replace(old,new)
path='src/UI/Modals/PokemonDetailsModal.cpp'; s=read(path)
s=replace(s,'#include "Legality/Legality.h"','#include "Legality/Legality.h"\n#include "Legality/ReportPresentation.h"')
s=s.replace('legalityRep.ok()', 'legalityRep.verdict() == Legality::Verdict::NoProblemsFound')
s=replace(s,'const std::string label = "Legality: " + std::to_string(legalityRep.problemCount())\n                                        + " issue(s)  -  R / tap to view";', '''const std::string label = legalityRep.verdict() == Legality::Verdict::Invalid
                    ? "Legality: Invalid  -  R / tap to view"
                    : "Legality: Incomplete / Unknown  -  R / tap to view";''')
start=s.index('            if (legalityRep.verdict()', s.index('struct ReportLine'))
end=s.index('            const int ow = 820',start)
s=s[:start]+'''            for (const auto& line : Legality::Presentation::lines(legalityRep)) {
                lines.push_back({line.text, line.severity == Legality::Severity::Invalid
                    ? Color(235, 100, 100) : line.severity == Legality::Severity::Warning
                    ? Colors::Orange : Color(120, 205, 140)});
            }
'''+s[end:]; changes[path][1]=s
path='src/UI/TrainerViewScreenBase.inc'; s=read(path)
s=replace(s,'if (pokemon && !Legality::analyze(*pokemon, pokemon->getGameGroup()).ok()) {', '''const std::string_view legalitySource = details.source == EditSource::Bank
                    ? std::string_view{} : std::string_view(sourceGameId);
                if (pokemon && Legality::analyze(*pokemon, pokemon->getGameGroup(), legalitySource)
                        .verdict() != Legality::Verdict::NoProblemsFound) {'''); changes[path][1]=s
if not a.presentation_only:
    path='src/UI/Gen5SharedReadOnlySurface.inc'; s=read(path)
    s=replace(s,'#include "Inventory/ItemQuantityPolicy.h"','#include "Inventory/ItemQuantityPolicy.h"\n#include "Legality/Gen5WorkspaceAdapter.h"')
    s=replace(s,'case Surface::Legality:\n            if(keys&HidNpadButton_B)(void)state.back();', '''case Surface::Legality:
                if((keys&HidNpadButton_Up) || tap==78101)
                    screen.details.legalityScroll=std::max(0,screen.details.legalityScroll-1);
                if((keys&HidNpadButton_Down) || tap==78102)++screen.details.legalityScroll;
                if((keys&HidNpadButton_B) || tap==78103)(void)state.back();''')
    s=replace(s,'} else if(state.surface()==Surface::CloneDestination)\n                    stageCloneOrMoveByKeypad(screen);','''} else if(state.surface()==Surface::CloneDestination)
                        stageCloneOrMoveByKeypad(screen);
                    if(state.surface()==Surface::Legality)screen.details.legalityScroll=0;''')
    start=s.index('// Origin & Legality (Round 3 Part 9)'); end=s.index('void drawLegality5',start)
    s=s[:start]+'''// The current staged PK5 is checked through the common engine verdict model.
    // Save identity/trainer/slot context comes from the validated workspace.
    '''+s[end:]
    s=replace(s,'fb.drawText(x+24,y+52,"PK5 structure checked • no full legality checker",\n                Colors::TextDim,TextStyle::Caption);','''const auto result=Legality::Gen5App::analyzeWorkspace(workspace,state.target());
        if(result)fb.drawText(x+24,y+52,result->verdictLine,
            result->verdict==Legality::Verdict::Invalid?Colors::Warning:
            result->verdict==Legality::Verdict::Incomplete?Colors::Orange:Colors::Success,
            TextStyle::Caption);''')
    s=replace(s,'if(!pk || !pk->valid() || pk->empty()) {','if(!pk || !pk->valid() || pk->empty() || !result) {')
    s=replace(s,'const std::pair<std::string,std::string> rows[]={','std::vector<std::pair<std::string,std::string>> rows={')
    s=replace(s,'{"Encounter check",P::legalityLabel(P::LegalityState::Unknown)},\n        {"Legality",P::legalityLabel(P::LegalityState::Incomplete)},','''{"Encounter check",Legality::Presentation::coverageLabel(result->report.coverage.encounter)},
            {"Legality",result->verdict==Legality::Verdict::Invalid?"Invalid":
                result->verdict==Legality::Verdict::Incomplete?"Incomplete / Unknown":"No problems found"},''')
    s=replace(s,'#include "Legality/Gen5WorkspaceAdapter.h"','#include "Legality/Gen5WorkspaceAdapter.h"\n#include "Legality/ReportPresentation.h"')
    start=s.index('    int rowY=y+96;',s.index('void drawLegality5')); end=s.index('\n}\nvoid drawDecision', start)
    s=s[:start]+'''    for(const auto& line:result->coverageLines)rows.emplace_back("",line);
        for(const auto& line:result->issueLines)rows.emplace_back("",line);
        constexpr int visible=12,rowHeight=28;
        const int maxScroll=std::max(0,static_cast<int>(rows.size())-visible);
        auto& scroll=screen.details.legalityScroll;
        scroll=std::clamp(scroll,0,maxScroll);
        fb.setClipRect(x+24,y+92,w-48,visible*rowHeight);
        for(int i=scroll;i<std::min(static_cast<int>(rows.size()),scroll+visible);++i) {
            const auto& [label,value]=rows[static_cast<size_t>(i)];
            const int rowY=y+96+(i-scroll)*rowHeight;
            fb.drawText(x+34,rowY,label,Colors::TextDim,TextStyle::Caption);
            fb.drawText(x+(label.empty()?34:290),rowY,value,Colors::Text,TextStyle::Caption);
        }
        fb.clearClip();
        drawGlyphButton(fb,x+24,y+h-56,140,40,"Up","Previous");
        drawGlyphButton(fb,x+180,y+h-56,140,40,"Down","Next");
        drawGlyphButton(fb,x+w-170,y+h-56,146,40,"B","Close");
        screen.touchButtons.push_back({78101,x+24,y+h-56,140,40});
        screen.touchButtons.push_back({78102,x+180,y+h-56,140,40});
        screen.touchButtons.push_back({78103,x+w-170,y+h-56,146,40});
        drawNavBar(fb,{{"Up/Down","Scroll"},{"B","Back"}});'''+s[end:]
    changes[path][1]=s
if a.gen3_provider:
    path='include/Legacy/FRLGReadOnlyTrainer.h'; v=read(path)
    v=replace(v,'#include "Trainer/Trainer.h"','#include "Trainer/Trainer.h"\n#include "Legality/Gen3BerryContext.h"\n#include "Legality/Legality.h"')
    v=replace(v,'    bool refreshStagedPokemonPresentation(std::string& error);','    bool refreshStagedPokemonPresentation(std::string& error);\n    Legality::Report analyzeLegality(const Pokemon::Pokemon& pokemon) const;')
    v=replace(v,'    std::string sourceGameId_;','    std::string sourceGameId_;\n    std::optional<Legality::Gen3BerryContext> legalityBerry_;')
    changes[path][1]=v
    path='src/Legacy/FRLGReadOnlyTrainer.cpp'; v=read(path)
    v=replace(v,'        if (!trainer->populate(save, error)) return nullptr;','        if (!trainer->populate(save, error)) return nullptr;\n        trainer->legalityBerry_ = Legality::Gen3BerryContext::fromSave(\n            save.sourceBytes(), metadata.sourceGameId, save.trainer().language);')
    marker='    bool FRLGReadOnlyTrainer::refreshStagedPokemonPresentation'
    position=v.index(marker)
    v=v[:position]+'''    Legality::Report FRLGReadOnlyTrainer::analyzeLegality(const Pokemon::Pokemon& pokemon) const {
        bool owned=false;
        for(const auto& mon:party)owned|=mon.get()==&pokemon;
        for(const auto& box:boxes)for(const auto& mon:box)owned|=mon.get()==&pokemon;
        if(!owned)return Legality::analyze(pokemon,pokemon.getGameGroup());
        return Legality::analyze(pokemon,Legality::Context{pokemon.getGameGroup(),
            sourceGameId_,legalityBerry_?&*legalityBerry_:nullptr});
    }

'''+v[position:]
    changes[path][1]=v
    condition='(legalitySource=="ruby_gba" || legalitySource=="sapphire_gba" || legalitySource=="emerald_gba" || legalitySource=="firered_gba" || legalitySource=="leafgreen_gba")'
    path='src/UI/Modals/PokemonDetailsModal.cpp'; v=changes[path][1]
    v=replace(v,'#include "Legality/Legality.h"','#include "Legality/Legality.h"\n#include "Legacy/FRLGReadOnlyTrainer.h"')
    v=replace(v,'const Legality::Report legalityRep =','Legality::Report legalityRep =')
    anchor='            Legality::analyze(*p, p->getGameGroup(), legalitySource);'
    v=replace(v,anchor,anchor+'\n        if '+condition+'\n            legalityRep=static_cast<const PokeVault::Legacy::FRLGReadOnlyTrainer&>(screen.trainer).analyzeLegality(*p);')
    changes[path][1]=v
    path='src/UI/TrainerViewScreenBase.inc'; v=changes[path][1]
    v='#include "Legacy/FRLGReadOnlyTrainer.h"\n'+v
    before='if (pokemon && Legality::analyze(*pokemon, pokemon->getGameGroup(), legalitySource)\n                        .verdict() != Legality::Verdict::NoProblemsFound) {'
    after='if (pokemon && [&] {\n                    const auto report='+condition+'\n                        ? static_cast<const PokeVault::Legacy::FRLGReadOnlyTrainer&>(trainer).analyzeLegality(*pokemon)\n                        : Legality::analyze(*pokemon,pokemon->getGameGroup(),legalitySource);\n                    return report.verdict()!=Legality::Verdict::NoProblemsFound;\n                }()) {'
    v=replace(v,before,after); changes[path][1]=v
out=''.join(''.join(difflib.unified_diff(old.splitlines(True),new.splitlines(True),fromfile='a/'+path,tofile='b/'+path)) for path,(old,new) in changes.items())
# Git accepts an empty context line without the optional leading space.
out=''.join('\n' if line==' \n' else line for line in out.splitlines(True))
a.output.write_text(out)
a.output.with_suffix('.json').write_text(json.dumps({'app_head':PIN,'files':{path:{'before_sha256':hashlib.sha256(old.encode()).hexdigest(),'after_sha256':hashlib.sha256(new.encode()).hexdigest()} for path,(old,new) in changes.items()},'requires':'Selectively import the reviewed legality engine and Gen5WorkspaceAdapter/ReportPresentation first. Never merge the whole campaign blindly.','native_build':'Not run in this environment; hardware rendering pending.'},indent=2)+'\n')
print('Prepared',len(changes),'files; app checkout unchanged.')
