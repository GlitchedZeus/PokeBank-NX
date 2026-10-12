// L6 Stage 4 adversarial gate-reconciliation regressions.
// Native and sanitizer executions are not original-source legality evidence.
#include "../../tools/legality_l6_stage4_gate_ledger.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

namespace G = PokeBank::L6::GateLedger;
namespace {
constexpr auto head = "0123456789abcdef0123456789abcdef01234567";
constexpr auto tree = "fedcba9876543210fedcba9876543210fedcba98";
constexpr auto other = "1111111111111111111111111111111111111111";
static_assert(G::validHexSha(head));
static_assert(!G::validHexSha("bad"));
static_assert(!G::validHexSha("ABCDEF0123456789ABCDEF0123456789ABCDEF01"));

void checkInvariant(const G::Summary& s) {
    assert(!s.originalRetailAttested);
    assert(!s.globallyLegalSupported);
    assert(!s.globallyInvalidSupported);
    assert(!s.integrationAcceptanceAuthorized);
    assert(s.verified <= G::kExpectedReceipts);
    assert(s.missing <= G::kExpectedReceipts);
}
std::array<G::Receipt,G::kExpectedReceipts> complete() {
    std::array<G::Receipt,G::kExpectedReceipts> entries{};
    for (std::size_t i=0;i<entries.size();++i) {
        entries[i] = {
            static_cast<G::Lane>(i / G::kProfileCount),
            static_cast<G::Profile>(i % G::kProfileCount),
            head, tree, true, true, false, 0, 16, 16
        };
    }
    return entries;
}
void testAllProfiles(unsigned& checks) {
    const auto good=complete();
    auto s=G::evaluate(head,tree,good);
    assert(s.state==G::State::NativeOnlyVerified);
    assert(s.verified==12 && s.failed==0 && s.missing==0);
    checkInvariant(s);++checks;
    for(std::size_t i=0;i<good.size();++i) {
        auto candidate=good;
        candidate[i].completed=false;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.failed==1 && s.verified==11);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].timedOut=true;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.failed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].exitCode=134;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.failed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].executedPrograms=15;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.failed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].applicationTree=other;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.wrongIdentity==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].started=false;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::Blocked && s.failed==1);
        checkInvariant(s);++checks;
    }
}
void testMalformed(unsigned& checks) {
    auto good=complete();
    for(std::size_t i=0;i<good.size();++i) {
        std::vector<G::Receipt> repeated(good.begin(),good.end());
        repeated.push_back(good[i]);
        auto s=G::evaluate(head,tree,repeated);
        assert(s.state==G::State::MalformedReceipt && s.duplicate==1);
        checkInvariant(s);++checks;
        auto candidate=good;
        candidate[i].lane=static_cast<G::Lane>(255);
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::MalformedReceipt && s.malformed==1 &&
               s.missing==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].profile=static_cast<G::Profile>(255);
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::MalformedReceipt && s.malformed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].requiredPrograms=0;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::MalformedReceipt && s.malformed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].executedPrograms=17;
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::MalformedReceipt && s.malformed==1);
        checkInvariant(s);++checks;
        candidate=good;candidate[i].applicationHead="junk";
        s=G::evaluate(head,tree,candidate);
        assert(s.state==G::State::MalformedReceipt && s.malformed==1);
        checkInvariant(s);++checks;
    }
    std::vector<G::Receipt> overflowing(65,good.front());
    auto s=G::evaluate(head,tree,overflowing);
    assert(s.state==G::State::MalformedReceipt && s.malformed==1);
    checkInvariant(s);++checks;
    s=G::evaluate("wrong",tree,good);
    assert(s.state==G::State::MalformedReceipt);
    checkInvariant(s);++checks;
    s=G::evaluate(head,"wrong",good);
    assert(s.state==G::State::MalformedReceipt);
    checkInvariant(s);++checks;
}
void testExhaustivePartialProfiles(unsigned& checks) {
    // 3 states across every profile of L1-L4: passed, timed out, absent.
    // L5 and L6 remain present; all 3^8 combinations exercised.
    for(unsigned code=0;code<6561;++code) {
        auto good=complete();
        std::array<bool,8> absent{};
        unsigned remaining=code,missing=0,failed=0;
        for(unsigned i=0;i<8;++i) {
            const unsigned choice=remaining%3;remaining/=3;
            absent[i]=(choice==2);
            if(choice==1){good[i].timedOut=true;++failed;}
            if(choice==2)++missing;
        }
        std::vector<G::Receipt> entries;
        for(std::size_t i=0;i<good.size();++i)
            if(i>=8 || !absent[i]) entries.push_back(good[i]);
        const auto s=G::evaluate(head,tree,entries);
        assert(s.missing==missing && s.failed==failed);
        assert(s.verified==12-missing-failed);
        assert(s.state==(code==0?G::State::NativeOnlyVerified:G::State::Blocked));
        checkInvariant(s);++checks;
    }
}
}
int main() {
    unsigned checks=0;
    testAllProfiles(checks);
    testMalformed(checks);
    testExhaustivePartialProfiles(checks);
    assert(checks==6709);
    std::cout<<"L6 Stage 4 exact-head gate ledger: "<<checks
             <<" PASS (native-only, never authenticating)\n";
}
