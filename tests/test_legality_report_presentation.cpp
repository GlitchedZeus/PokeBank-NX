#include "Legality/ReportPresentation.h"
#include <cassert>
#include <iostream>
int main(){
 size_t cases=0;
 // All coverage combinations: warning-free does not imply complete. This tests visible
 // strings and severity for every state, rather than merely calling the formatter.
 for(unsigned code=0;code<19683;++code){
  Legality::Report r;unsigned n=code;bool all=true;
  for(auto*p:{&r.coverage.structure,&r.coverage.sourceGame,&r.coverage.internal,&r.coverage.moves,&r.coverage.encounter,&r.coverage.eventGift,&r.coverage.pidRng,&r.coverage.eggBreeding,&r.coverage.transfer}){
   *p=static_cast<Legality::CoverageLevel>(n%3);all&=n%3==2;n/=3;
  }
  for(int severity=0;severity<3;++severity){auto report=r;if(severity)report.issues.push_back({severity==1?Legality::Severity::Warning:Legality::Severity::Invalid,"case",Legality::CheckIdentifier::Misc});
   const auto lines=Legality::Presentation::lines(report);assert(!lines.empty());
   const bool clean=severity==0&&all;
   if(clean){assert(lines[0].text=="No problems found by complete checks.");assert(lines.size()==1);}
   else if(severity==2){assert(lines[0].text=="Invalid: 1 issue(s).");assert(lines.size()==2);assert(lines[1].text=="Invalid: case");}
   else{assert(lines[0].text.starts_with("Incomplete / Unknown:"));assert(lines.size()==10u+(severity==1));for(const auto&l:lines)assert(l.text.find("No problems found")==std::string::npos);}
   ++cases;
  }
 }
 std::cout<<"Legality rendered verdict/coverage: "<<cases<<" state/severity combinations PASS\n";
}
