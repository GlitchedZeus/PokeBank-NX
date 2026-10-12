// L6 Stage 5 production-linked immutable native PK4 / cross-lane audit.
// The hypothetical candidate claims below are never inferred from PK4 bytes,
// attached to an actual trainer, or used to modify Legality::analyze().
#include "Encryption/Encryption4.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "tools/legality_l6_stage5_joint_history_gate.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>
namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}
namespace J=Legality::Audit::JointHistory;
namespace {
void put16(std::vector<std::byte>& v,size_t at,uint16_t x) {
    v.at(at)=static_cast<std::byte>(x);v.at(at+1)=static_cast<std::byte>(x>>8);
}
void put32(std::vector<std::byte>& v,size_t at,uint32_t x) {
    put16(v,at,static_cast<uint16_t>(x));put16(v,at+2,static_cast<uint16_t>(x>>16));
}
std::vector<std::byte> original(uint32_t pid,uint16_t move=33) {
    std::vector<std::byte> plain(Encryption::SIZE_STORED4,std::byte{0});
    put32(plain,0,pid);put16(plain,8,1); // Bulbasaur
    put16(plain,0x0c,12345);put16(plain,0x0e,54321);
    put32(plain,0x10,135);
    plain[0x14]=std::byte{70};plain[0x15]=std::byte{65};
    plain[0x17]=std::byte{2};
    put16(plain,0x28,move);plain[0x30]=std::byte{35};
    plain[0x5f]=std::byte{12}; // Stored Platinum original identity.
    plain[0x83]=std::byte{4};plain[0x84]=std::byte{5};
    put16(plain,6,Encryption::checksum4(plain));
    return Encryption::encryptArray4(plain);
}
bool contains(const Legality::Report& r,Legality::Severity sev,std::string_view text) {
    for(const auto& i:r.issues)
        if(i.severity==sev && i.text.find(text)!=std::string::npos)return true;
    return false;
}
J::Input hypothetical(uint32_t id) {
    J::Input in{};in.pokemonIdentity=id;in.count=2;
    in.everyOriginalRouteEnumerated=in.everyAlternativeMethodEnumerated=false;
    for(uint8_t i=0;i<2;++i) {
        auto& r=in.routes[i];r.pokemonIdentity=id;r.routeIdentity=100u+i;
        r.independentlyAuthenticated=r.completeSamePokemonHistory=true;
        r.requiredLanes=J::Encounter|J::Breeding|J::Moves|J::Event;
        r.claims={J::Claim::Compatible,J::Claim::Missing,
                  i?J::Claim::Contradicted:J::Claim::Compatible,J::Claim::Missing};
    }
    return in;
}
}
int main() {
    unsigned checks=0;
    for(uint32_t index=0;index<64;++index) {
        const auto pid=0x12345678u+index*2u;
        const auto raw=original(pid);
        const auto preserved=raw;
        Pokemon::Pokemon4ReadOnly pk(raw,Enums::GameVersion::PT);
        assert(pk.valid() && !pk.empty() && pk.species()==1);
        Pokemon::Pokemon4ReadOnlyView view(pk);
        const auto detached=Legality::analyze(view,Enums::GameVersion::PT);
        const auto mislabeled=Legality::analyze(view,Enums::GameVersion::PT,"firered_gba");
        assert(!detached.hasInvalid() && detached.verdict()==Legality::Verdict::Incomplete);
        assert(!mislabeled.hasInvalid() &&
               contains(mislabeled,Legality::Severity::Warning,"source-game label conflicts"));
        // This externally proposed, incomplete history can never upgrade the
        // production report. The native record supplies no authenticated L2/L4.
        const auto proposal=hypothetical(9000u+index);
        assert(J::evaluate(proposal).status==J::Status::Unresolved);
        assert(raw==preserved);
        checks+=6;
        // A genuinely out-of-format move is independently detectable in the
        // encrypted native record, regardless of the missing source label.
        const auto corrupt=original(pid,468);
        Pokemon::Pokemon4ReadOnly bad(corrupt,Enums::GameVersion::PT);
        assert(bad.valid() && !bad.empty());
        Pokemon::Pokemon4ReadOnlyView badView(bad);
        const auto structural=Legality::analyze(badView,Enums::GameVersion::PT);
        assert(structural.hasInvalid() &&
               contains(structural,Legality::Severity::Invalid,"cannot exist in a Generation 4 save"));
        checks+=2;
    }
    assert(checks==512);
    std::cout<<"L6 Stage5 production-linked encrypted PK4 + cross-lane: "
             <<checks<<" PASS (synthetic, read-only)\n";
}
