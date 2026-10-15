// Sword/Shield SwishCrypto container, PK8 codec and strict `main` parser.
// Differential reference: PKSM-Core aa22d7a4 (vendor/PKSM-Core) crypto_swsh.cpp and
// its header-only PK6/7/8 block crypt, an implementation independent of PKHeX.

#include "Integration/Gen8/Gen8PK8.h"
#include "Integration/Gen8/Gen8Save.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"

#include "fixtures/gen8/Gen8SyntheticSave.h"
#include "fixtures/gen8/Gen8TestSupport.h"

#include "pkx/PK8.hpp"
#include "utils/crypto.hpp"

#include <algorithm>
#include <memory>
#include <random>
#include <string>

using namespace PokeVault::Integration::Gen8;
namespace F = Gen8Fixture;

namespace {

// PKSM-Core PK8.hpp: BLOCK_LENGTH = 80 (private there, so restated and asserted here).
constexpr size_t PksmBlockLength = 80;

std::shared_ptr<uint8_t[]> shared(const std::vector<uint8_t>& v) {
    std::shared_ptr<uint8_t[]> p(new uint8_t[v.size()]);
    std::copy(v.begin(), v.end(), p.get());
    return p;
}

// PKSM-Core's PK8 crypt, from the same header-only primitives PK8.cpp uses.
std::vector<uint8_t> pksmDecrypt8(std::vector<uint8_t> d) {
    const uint32_t ec = PK8::read32(d, 0);
    pksm::crypto::pkm::crypt<pksm::PK8::BOX_LENGTH - 8>(d.data() + 8, ec);
    if (d.size() == pksm::PK8::PARTY_LENGTH)
        pksm::crypto::pkm::crypt<pksm::PK8::PARTY_LENGTH - pksm::PK8::BOX_LENGTH>(d.data() + pksm::PK8::BOX_LENGTH, ec);
    pksm::crypto::pkm::blockShuffle<PksmBlockLength>(d.data() + 8, static_cast<uint8_t>((ec >> 13) & 31));
    return d;
}

void testConstantsAgainstPksm() {
    // PKSM applyXor over zeros reproduces the xorpad; PKSM sign reproduces the salted hash.
    std::vector<uint8_t> zeros(0x7F * 3 + 5 + Swish::HashSize, 0);
    auto p = shared(zeros);
    pksm::crypto::swsh::applyXor(p, zeros.size());
    std::vector<uint8_t> mine(zeros.size() - Swish::HashSize, 0);
    Swish::cryptStaticXorpad(mine);
    GEN8_CHECK(std::equal(mine.begin(), mine.end(), p.get()));
    for (size_t i = 0; i < Swish::StaticXorpad.size(); ++i) GEN8_CHECK(mine[i] == Swish::StaticXorpad[i]);

    std::vector<uint8_t> data(1000);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<uint8_t>(i * 31 + 7);
    auto q = shared(data);
    pksm::crypto::swsh::sign(q, data.size());
    const auto hash = Swish::computeHash(std::span<const uint8_t>(data).first(data.size() - Swish::HashSize));
    GEN8_CHECK(std::equal(hash.begin(), hash.end(), q.get() + data.size() - Swish::HashSize));
}

void testContainerRoundTripAndPksm() {
    const auto file = F::makeSave();
    GEN8_CHECK(file.size() == Layout::SwordShieldSizes[0]);
    GEN8_CHECK(Swish::hashValid(file));
    auto shared8 = shared(file);
    GEN8_CHECK(pksm::crypto::swsh::verify(shared8, file.size()));

    Swish::ParseStatus st{};
    const auto blocks = Swish::decrypt(file, &st);
    GEN8_CHECK(blocks && st == Swish::ParseStatus::Ok);
    if (!blocks) return;
    const auto again = Swish::encrypt(*blocks);
    GEN8_CHECK(again == file);  // byte-identical round trip

    // PKSM-Core's own block walker over the same file must see the same blocks.
    pksm::crypto::swsh::applyXor(shared8, file.size());
    const auto list = pksm::crypto::swsh::getBlockList(shared8, file.size());
    GEN8_CHECK(list.size() == blocks->size());
    for (size_t i = 0; i < std::min(list.size(), blocks->size()); ++i) {
        GEN8_CHECK(list[i]->key() == (*blocks)[i].key);
        if ((*blocks)[i].data.empty()) continue;
        const uint8_t* d = list[i]->decryptedData();
        GEN8_CHECK(std::equal((*blocks)[i].data.begin(), (*blocks)[i].data.end(), d));
    }
}

void testStrictContainerRejections() {
    auto file = F::makeSave();
    Swish::ParseStatus st{};

    auto bad = file;
    bad[100] ^= 1;
    GEN8_CHECK(!Swish::decrypt(bad, &st) && st == Swish::ParseStatus::HashMismatch);
    GEN8_CHECK(!Swish::decrypt(std::span<const uint8_t>(file).first(Swish::HashSize), &st) &&
               st == Swish::ParseStatus::TooSmall);

    const auto reseal = [](std::vector<uint8_t> raw) {  // raw = decrypted serialized blocks
        Swish::cryptStaticXorpad(raw);
        const auto h = Swish::computeHash(raw);
        raw.insert(raw.end(), h.begin(), h.end());
        return raw;
    };
    {   // unknown standalone type 6
        Swish::Block b{0x1111, static_cast<Swish::TypeCode>(6), Swish::TypeCode::None, {}};
        std::vector<uint8_t> raw;
        Swish::writeBlock(b, raw);
        GEN8_CHECK(!Swish::decrypt(reseal(raw), &st) && st == Swish::ParseStatus::UnknownType);
    }
    {   // array of objects is not a sized subtype
        Swish::Block b{0x2222, Swish::TypeCode::Array, Swish::TypeCode::Object, {}};
        std::vector<uint8_t> raw;
        Swish::writeBlock(b, raw);
        GEN8_CHECK(!Swish::decrypt(reseal(raw), &st) && st == Swish::ParseStatus::UnknownType);
    }
    {   // object length running past the payload
        Swish::Block b{0x3333, Swish::TypeCode::Object, Swish::TypeCode::None, {1, 2, 3, 4}};
        std::vector<uint8_t> raw;
        Swish::writeBlock(b, raw);
        raw.resize(raw.size() - 2);
        GEN8_CHECK(!Swish::decrypt(reseal(raw), &st) && st == Swish::ParseStatus::Truncated);
    }
    {   // trailing bytes that are not a whole block
        Swish::Block b{0x4444, Swish::TypeCode::Bool1, Swish::TypeCode::None, {}};
        std::vector<uint8_t> raw;
        Swish::writeBlock(b, raw);
        raw.push_back(0);
        raw.push_back(0);
        GEN8_CHECK(!Swish::decrypt(reseal(raw), &st) && st == Swish::ParseStatus::Truncated);
    }
    {   // duplicate keys
        Swish::Block b{0x5555, Swish::TypeCode::Byte, Swish::TypeCode::None, {9}};
        std::vector<uint8_t> raw;
        Swish::writeBlock(b, raw);
        Swish::writeBlock(b, raw);
        GEN8_CHECK(!Swish::decrypt(reseal(raw), &st) && st == Swish::ParseStatus::DuplicateKey);
    }
    {   // every value type and the Bool codes round-trip exactly
        std::vector<Swish::Block> all;
        uint32_t key = 0x100;
        for (uint8_t t : {1, 2, 3}) all.push_back({key++, static_cast<Swish::TypeCode>(t), Swish::TypeCode::None, {}});
        for (uint8_t t = 8; t <= 17; ++t) {
            const auto tc = static_cast<Swish::TypeCode>(t);
            std::vector<uint8_t> v(Swish::typeSize(tc));
            for (size_t i = 0; i < v.size(); ++i) v[i] = static_cast<uint8_t>(t * 13 + i);
            all.push_back({key++, tc, Swish::TypeCode::None, v});
            std::vector<uint8_t> arr(Swish::typeSize(tc) * 3, static_cast<uint8_t>(t));
            all.push_back({key++, Swish::TypeCode::Array, tc, arr});
        }
        all.push_back({key++, Swish::TypeCode::Array, Swish::TypeCode::Bool3, {0, 1, 2}});
        const auto enc = Swish::encrypt(all);
        const auto dec = Swish::decrypt(enc, &st);
        GEN8_CHECK(dec && dec->size() == all.size());
        if (dec) {
            for (size_t i = 0; i < all.size(); ++i) {
                GEN8_CHECK((*dec)[i].key == all[i].key && (*dec)[i].type == all[i].type &&
                           (*dec)[i].subtype == all[i].subtype && (*dec)[i].data == all[i].data);
            }
            GEN8_CHECK(Swish::encrypt(*dec) == enc);
        }
    }
}

void testPk8AgainstPksm() {
    static_assert(pksm::PK8::BOX_LENGTH == PK8::StoredSize);
    static_assert(pksm::PK8::PARTY_LENGTH == PK8::PartySize);
    static_assert(PksmBlockLength == PK8::BlockSize);
    std::mt19937 rng(8);
    for (int round = 0; round < 400; ++round) {
        std::vector<uint8_t> enc(round % 2 ? PK8::StoredSize : PK8::PartySize);
        for (auto& b : enc) b = static_cast<uint8_t>(rng());
        const auto mine = PK8::decrypt(enc);
        GEN8_CHECK(mine == pksmDecrypt8(enc));
        GEN8_CHECK(PK8::encrypt(mine) == enc);
    }
    for (uint32_t sv = 0; sv < 32; ++sv)
        for (size_t i = 0; i < 4; ++i)
            GEN8_CHECK(pksm::crypto::pkm::BlockPositions[sv * 4 + i] == PK8::BlockPosition[(sv % 24) * 4 + i]);

    const auto plain = F::plainMon(25, 9);
    const auto enc = PK8::encrypt(plain);
    PK8::DecodeStatus st{};
    const auto mon = PK8::Pokemon8::decode(enc, &st);
    GEN8_CHECK(mon && st == PK8::DecodeStatus::Ok);
    if (mon) {
        GEN8_CHECK(mon->species() == 25 && mon->tid() == 1009 && mon->ball() == 4);
        GEN8_CHECK(mon->nickname() == u"SYNTH" && mon->originalTrainerName() == u"TESTER");
        GEN8_CHECK(mon->serialize() == enc);
    }
    auto sanity = plain;
    PK8::write16(sanity, 0x04, 1);
    GEN8_CHECK(!PK8::Pokemon8::decode(PK8::encrypt(sanity), &st) && st == PK8::DecodeStatus::BadSanity);
    auto sum = plain;
    sum[0x40] ^= 1;
    GEN8_CHECK(!PK8::Pokemon8::decode(PK8::encrypt(sum), &st) && st == PK8::DecodeStatus::BadChecksum);
    GEN8_CHECK(!PK8::Pokemon8::decode(std::vector<uint8_t>(0x104), &st) && st == PK8::DecodeStatus::UnknownSize);
    const auto blank = PK8::Pokemon8::decode(F::blankMon());
    GEN8_CHECK(blank && blank->empty() && blank->allZero());
}

void testSaveParse() {
    const auto file = F::makeSave();
    const auto copy = file;
    std::string error;
    const auto save = Gen8Save::parse(file, "sword_switch", &error);
    GEN8_CHECK(save.has_value());
    GEN8_CHECK(file == copy);  // parse never mutates its input
    if (!save) { std::fprintf(stderr, "%s\n", error.c_str()); return; }
    GEN8_CHECK(save->exactGameId() == "sword_switch" && save->version() == 44);
    GEN8_CHECK(save->partyCount() == 2);
    GEN8_CHECK(save->party(0).occupied() && save->party(0).pokemon->species() == 810);
    GEN8_CHECK(save->party(1).occupied() && save->party(2).blank());
    GEN8_CHECK(save->box(0, 0).occupied() && save->box(0, 0).pokemon->species() == 25);
    GEN8_CHECK(save->box(31, 29).occupied() && save->box(31, 29).pokemon->species() == 888);
    GEN8_CHECK(save->box(5, 5).blank());
    GEN8_CHECK(!save->box(32, 0).pokemon && !save->party(6).pokemon);
    GEN8_CHECK(save->diagnostics().occupiedBoxRecords == 3);
    GEN8_CHECK(save->diagnostics().emptyBoxRecords == Layout::BoxCount * Layout::BoxSlots - 3);
    GEN8_CHECK(save->diagnostics().invalidBoxRecords == 0);
    const auto t = save->trainer();
    GEN8_CHECK(t.tid == 12345 && t.sid == 54321 && t.version == 44 && t.gender == 1 && t.language == 2);
    GEN8_CHECK(t.rawName == u"Hop");
    GEN8_CHECK(save->teams().has_value() && save->teams()->size() == 1);
    if (save->teams() && !save->teams()->empty()) {
        const auto& team = save->teams()->front();
        GEN8_CHECK(team.team == 0 && team.box == 0 && team.slot == 1 && !team.locked);
    }

    // Every accepted patch-history size parses; Shield maps to its own exact game.
    for (const size_t size : Layout::SwordShieldSizes) {
        F::Options o;
        o.fileSize = size;
        o.version = Layout::VersionShield;
        const auto s = Gen8Save::parse(F::makeSave(o));
        GEN8_CHECK(s && s->exactGameId() == "shield_switch");
    }
    GEN8_CHECK(!Gen8Save::parse(file, "shield_switch", &error));
}

void testSaveRejections() {
    std::string error;
    const auto rejects = [&](std::vector<Swish::Block> blocks, const char* why) {
        const auto file = Swish::encrypt(blocks);
        const bool rejected = !Gen8Save::parse(file, {}, &error);
        if (!rejected) std::fprintf(stderr, "expected rejection: %s\n", why);
        GEN8_CHECK(rejected);
    };
    // Size outside the accepted set (the filler is sized for SIZE_G8SWSH, then grown).
    {
        auto blocks = F::makeBlocks();
        F::findBlock(blocks, 0x7E57F111u)->data.push_back(0);
        rejects(blocks, "unknown size");
    }
    const auto resize = [](std::vector<Swish::Block>& blocks, long delta) {
        auto& pad = F::findBlock(blocks, 0x7E57F111u)->data;
        pad.resize(static_cast<size_t>(static_cast<long>(pad.size()) - delta));
    };
    {
        auto blocks = F::makeBlocks();
        F::findBlock(blocks, Layout::KeyMyStatus)->data[Layout::MyStatusGame] = 30;
        rejects(blocks, "bad version");
    }
    {
        auto blocks = F::makeBlocks();
        F::findBlock(blocks, Layout::KeyParty)->data[Layout::PartyCountOffset] = 7;
        rejects(blocks, "party count 7");
    }
    {
        auto blocks = F::makeBlocks();
        F::findBlock(blocks, Layout::KeyParty)->data[0x40] ^= 0xFF;  // party slot 1 corrupt
        rejects(blocks, "declared party invalid");
    }
    {
        auto blocks = F::makeBlocks();
        auto* party = F::findBlock(blocks, Layout::KeyParty);
        party->data[Layout::PartyCountOffset] = 3;  // slot 3 is blank
        rejects(blocks, "declared party blank");
    }
    {
        auto blocks = F::makeBlocks();
        auto* box = F::findBlock(blocks, Layout::KeyBox);
        box->data.resize(box->data.size() - 1);
        resize(blocks, -1);
        rejects(blocks, "box truncated");
    }
    {
        auto blocks = F::makeBlocks();
        blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                                    [](const Swish::Block& b) { return b.key == Layout::KeyMyStatus; }),
                     blocks.end());
        rejects(blocks, "missing MyStatus");
    }
    {
        auto blocks = F::makeBlocks();
        auto* party = F::findBlock(blocks, Layout::KeyParty);
        const size_t before = party->serializedSize();
        party->type = Swish::TypeCode::Array;
        party->subtype = Swish::TypeCode::UInt16;
        resize(blocks, static_cast<long>(party->serializedSize() - before));
        rejects(blocks, "party not a byte payload");
    }
    // Invalid undeclared party slots and invalid box records are quarantined, not fatal.
    {
        auto blocks = F::makeBlocks();
        F::findBlock(blocks, Layout::KeyParty)->data[4 * Layout::RecordSize + 0x40] ^= 0xFF;
        F::findBlock(blocks, Layout::KeyBox)->data[7 * Layout::RecordSize + 0x40] ^= 0xFF;
        const auto s = Gen8Save::parse(Swish::encrypt(blocks), {}, &error);
        GEN8_CHECK(s && s->diagnostics().invalidUndeclaredPartyRecords == 1 &&
                   s->diagnostics().invalidBoxRecords == 1 && s->box(0, 7).state == SlotState::Invalid);
    }
    // Team references outside storage, or no team blocks at all: teams() is empty.
    {
        auto blocks = F::makeBlocks();
        PK8::write16(F::findBlock(blocks, Layout::KeyTeamIndexes)->data, 2, static_cast<uint16_t>(40 << 8));
        const auto s = Gen8Save::parse(Swish::encrypt(blocks));
        GEN8_CHECK(s && !s->teams().has_value());
        F::Options o;
        o.includeTeams = false;
        const auto none = Gen8Save::parse(F::makeSave(o));
        GEN8_CHECK(none && !none->teams().has_value());
        o.includeTeams = true;
        o.lockTeam0 = true;
        const auto locked = Gen8Save::parse(F::makeSave(o));
        GEN8_CHECK(locked && locked->teams() && locked->teams()->front().locked);
    }
}

} // namespace

int main() {
    testConstantsAgainstPksm();
    testContainerRoundTripAndPksm();
    testStrictContainerRejections();
    testPk8AgainstPksm();
    testSaveParse();
    testSaveRejections();
    return Gen8Test::finish("test_gen8_swsh");
}
