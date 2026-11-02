#include "Legality/Gen5FullCardCatalog.h"
#include "Legality/Gen5LegalityData.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <vector>

namespace G=Legality::Gen5Gift;
namespace F=G::FullCard;
namespace D=Legality::Gen5Data;

int main(int argc,char** argv) {
    static_assert(!G::ArchiveMatch{}.authenticatesReceipt());
    assert(G::kArchivedFullCards.size()==63);
    std::set<uint16_t> indices;
    for(const auto& card:G::kArchivedFullCards) {
        assert(card.catalogIndex<D::kPgfCount && card.fullSha256.size()==64);
        indices.insert(card.catalogIndex);
        const std::span<const uint8_t> pgf(D::kPgf+card.catalogIndex*D::kPgfSize,D::kPgfSize);
        assert(F::comparePayloads(card.payload,pgf)==F::PayloadRelation::ReceiptDateOnly);
        const auto evidence=G::archivedFullCardEvidence(card.catalogIndex,pgf,20,card.language);
        assert(evidence.templates>0 && evidence.pairedReceivingClaim && !evidence.authenticatesReceipt());
        assert(!G::archivedFullCardEvidence(card.catalogIndex,pgf,20,6).pairedReceivingClaim);
        assert(!G::archivedFullCardEvidence(card.catalogIndex,pgf,19,card.language).pairedReceivingClaim);
        assert(G::archivedFullCardEvidence(9999,pgf,20,card.language).templates==0);
        auto wrong=card.payload;
        // Every non-date byte remains part of the identity, including item,
        // moves, OT, title, language, PID/nature/ability restrictions and ID.
        for(size_t i=0;i<wrong.size();++i) {
            if(i>=F::kDateOffset && i<F::kDateEnd) continue;
            wrong=card.payload;wrong[i]^=1;
            assert(G::archivedFullCardEvidence(card.catalogIndex,wrong,20,card.language).templates==0);
        }
        assert(G::archivedFullCardEvidence(card.catalogIndex,pgf.first(203),20,card.language).templates==0);
    }
    size_t released=0,unmatched=0;
    // Optional corpus replay uses ORIGINAL hash-verified full-card bytes,
    // supplied by full_card_evidence.py, not cards assembled by this parser.
    for(int arg=1;arg<argc;++arg) {
        std::ifstream input(argv[arg],std::ios::binary);assert(input);
        const std::vector<uint8_t> raw((std::istreambuf_iterator<char>(input)),{});
        const auto before=raw;
        const auto view=F::parse(raw);
        assert(view.checksumValid());
        assert(view.catalogVersionClaims()!=0);
        size_t matches=0;
        for(size_t index=0;index<D::kPgfCount;++index) {
            const std::span<const uint8_t> pgf(D::kPgf+index*D::kPgfSize,D::kPgfSize);
            const auto relation=view.payloadRelation(pgf);
            if(relation==F::PayloadRelation::ReceiptDateOnly || relation==F::PayloadRelation::Exact) {
                ++matches;
                const auto evidence=G::archivedFullCardEvidence(index,pgf,20,view.language);
                assert(evidence.templates && !evidence.authenticatesReceipt());
            }
        }
        assert(matches<=1);
        if(matches)++released;else ++unmatched;
        assert(raw==before);
    }
    if(argc>1)assert(argc==82 && released==63 && unmatched==18);
    std::cout<<"Full-card catalog:63 released rows, "<<indices.size()
             <<" distinct PGFs; exact/date-only identity, per-card receiving claims, non-date mutation refusal and no receipt authentication PASS\n";
    if(argc>1)std::cout<<"Original archive:81 CRC-valid full cards;63 released matches/18 unmatched debug cards; immutable inputs PASS\n";
}
