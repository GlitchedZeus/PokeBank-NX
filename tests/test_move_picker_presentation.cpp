#include "UI/MovePickerPresentation.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
static std::string read(const char* p){std::ifstream in(p);assert(in);return {std::istreambuf_iterator<char>(in),{}};}
int main(){using Enums::GameVersion;namespace M=PokeBank::UIModel::MovePickerPresentation;
const auto* b1=Names::getMoveBattleStats(117,GameVersion::RBY);const auto* b4=Names::getMoveBattleStats(117,GameVersion::HGSS);assert(b1&&b4&&b1->power==0&&b1->accuracy==100&&b1->pp==10&&b4->power==1&&b4->accuracy==0&&b4->pp==10);
const auto* g3=Names::getMoveBattleStats(202,GameVersion::FRLG);const auto* g4=Names::getMoveBattleStats(202,GameVersion::DP);assert(g3&&g4&&g3->pp==5&&g4->pp==10);\nconst auto* hypDP=Names::getMoveBattleStats(95,GameVersion::DP);const auto* hypD=Names::getMoveBattleStats(95,GameVersion::D);const auto* hypP=Names::getMoveBattleStats(95,GameVersion::P);const auto* hypPt=Names::getMoveBattleStats(95,GameVersion::PT);const auto* hypHG=Names::getMoveBattleStats(95,GameVersion::HGSS);assert(hypDP&&hypD&&hypP&&hypPt&&hypHG&&hypDP->accuracy==70&&hypD->accuracy==70&&hypP->accuracy==70&&hypPt->accuracy==60&&hypHG->accuracy==60);
assert(M::numberedName(33)=="033 - Tackle");assert(M::numberedName(0)=="000 - Empty move slot");assert(M::statsLabel(33,GameVersion::RBY)=="Acc 95   Pwr 35   PP 35");assert(M::statsLabel(117,GameVersion::RBY)=="Acc 100   Pwr —   PP 10");assert(M::statsLabel(117,GameVersion::PT)=="Acc —   Pwr 1   PP 10");
assert(Names::getMoveBattleStats(166,GameVersion::RBY)==nullptr);assert(Names::getMoveBattleStats(252,GameVersion::GSC)==nullptr);assert(Names::getMoveBattleStats(355,GameVersion::FRLG)==nullptr);assert(Names::getMoveBattleStats(468,GameVersion::PT)==nullptr);
const auto compact=M::compactPickerLayout();assert(compact.width==760&&compact.height==520&&compact.visibleRows==9&&compact.rowStart==86&&compact.rowStep==42&&compact.highlightHeight==38);
for(const char* p:{"src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc","src/UI/Gen2PokemonPickerOverlay.inc","src/UI/Gen3SharedPokemonSurface.inc","src/UI/Gen4SharedPokemonSurface.inc"}){const auto s=read(p);assert(s.find("#include \"UI/MovePickerPresentation.h\"")!=std::string::npos);assert(s.find("MoveUI::rowLabel")!=std::string::npos);}std::cout<<"Gen I-IV move picker presentation: PASS\\n";}
