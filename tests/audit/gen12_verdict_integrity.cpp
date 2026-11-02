// Deterministic public-checker boundary probes and verdict-integrity checks.
// These do not certify encounter history or authenticate original specimens.
#include "Legality/GBLegality.h"
#include <algorithm>
#include <iostream>
namespace G = Legality::GB;

bool coherent(const G::Result& r) {
    if (r.status == G::Status::Legal)
        return r.problems.empty() && r.origin.kind != G::EncounterKind::None;
    if (r.problems.empty()) return false;
    const bool definitive = std::any_of(r.problems.begin(), r.problems.end(),
        [](const G::Problem& p) { return p.definitive; });
    return (r.status == G::Status::Illegal) == definitive;
}

int main() {
    unsigned count = 0;
    for (unsigned value=0; value<256; ++value) for (unsigned field=0; field<4; ++field) {
        G::Entity1 a; a.species=1; a.level=50; a.moves={33,45,15,36};
        a.type1=22; a.type2=3; a.catchRate=45;
        a.otName="GOLD"; a.nickname="MON"; a.language=G::Language::International;
        G::Entity2 b; b.species=1; b.level=50; b.moves=a.moves;
        b.exp=117360; b.otName=a.otName; b.nickname=a.nickname; b.language=a.language;
        if(field==0) a.species=b.species=static_cast<uint8_t>(value);
        if(field==1) a.level=b.level=static_cast<uint8_t>(value);
        if(field==2) a.moves[0]=b.moves[0]=static_cast<uint8_t>(value);
        if(field==3) { a.catchRate=static_cast<uint8_t>(value); b.heldItem=static_cast<uint8_t>(value); }
        const auto aMoves=a.moves,bMoves=b.moves;
        const auto x=G::checkGen1(a),y=G::checkGen2(b);
        if(!coherent(x)||!coherent(y)||a.moves!=aMoves||b.moves!=bMoves)
            return std::cerr<<"verdict/mutation integrity failure "<<field<<":"<<value<<'\n',1;
        const bool invalid1=(field==0&&(value==0||value>151)) ||
                            (field==1&&(value==0||value>100)) ||
                            (field==2&&(value==0||value>165));
        const bool invalid2=(field==0&&(value==0||value>251)) ||
                            (field==1&&(value==0||value>100)) ||
                            (field==2&&(value==0||value>251));
        if((invalid1&&x.status!=G::Status::Illegal)||(invalid2&&y.status!=G::Status::Illegal))
            return std::cerr<<"malformed record was not rejected "<<field<<":"<<value<<'\n',1;
        count+=2;
    }
    std::cout<<"GB boundary/verdict/immutability probes: "<<count<<" checks PASS\n";
}
