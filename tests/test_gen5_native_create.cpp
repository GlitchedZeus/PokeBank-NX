#include "Integration/Gen5/Gen5NativeCreate.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

#include <cstdlib>
#include <iostream>
#include <limits>

namespace G = PokeVault::Integration::Gen5;
namespace C = G::Crypto;
static size_t checks = 0, creations = 0;
#define CHECK(...) do { ++checks; if (!(__VA_ARGS__)) { \
    std::cerr << "FAIL line " << __LINE__ << ": " << #__VA_ARGS__ << '\n'; std::exit(1); } } while (false)

// Test-only decoder: no production crypto, checksum, block-order or read helpers.
// Block positions transcribed from pinned PKHeX PokeCrypto.BlockPosition.
static std::vector<uint8_t> oracleDecode(const std::vector<uint8_t>& encrypted) {
    CHECK(encrypted.size() == 136);
    constexpr std::array<std::string_view, 24> orders{
        "0123","0132","0213","0312","0231","0321","1023","1032",
        "2013","3012","2031","3021","1203","1302","2103","3102",
        "2301","3201","1230","1320","2130","3120","2310","3210"};
    uint32_t pid = 0;
    for (size_t i = 0; i < 4; ++i) pid |= uint32_t(encrypted[i]) << (i * 8);
    uint32_t seed = encrypted[6] + 256u * encrypted[7];
    auto physical = encrypted, logical = encrypted;
    for (size_t i = 8; i < 136; i += 2) {
        seed = uint32_t(uint64_t(seed) * 0x41c64e6d + 0x6073);
        physical[i] ^= uint8_t(seed >> 16);
        physical[i + 1] ^= uint8_t(seed >> 24);
    }
    const auto order = orders[((pid >> 13) & 31) % 24];
    for (size_t i = 0; i < 128; ++i)
        logical[8 + i] = physical[8 + size_t(order[i / 32] - '0') * 32 + i % 32];
    uint32_t sum = 0;
    for (size_t i = 8; i < 136; i += 2) sum += logical[i] + 256u * logical[i + 1];
    CHECK((sum & 65535) == encrypted[6] + 256u * encrypted[7]);
    return logical;
}
static G::CreateRequest5 request(uint32_t species = 25, G::CreateGame5 game = G::CreateGame5::Black) {
    G::CreateRequest5 r;
    r.game = game; r.species = species; r.level = 50; r.nature = 3;
    r.pid = 0x123400ff; r.abilitySlot = 0; r.friendship = 70;
    r.trainer = G::CreateTrainer5{12345,54321,1,2,"TEST OT"};
    r.nickname = "TEST MON";
    r.encounter = {G::CreateHistory5::CallerSuppliedNative,1,1,4,2024,2,29};
    r.moves[0] = {1,0,std::nullopt};
    const auto p = G::personal5(species,0,static_cast<uint32_t>(game) >= 22);
    CHECK(p);
    r.gender = G::createGenderForPID5(p.info->genderRatio,*r.pid);
    return r;
}
static G::CreateResult5 succeeds(const G::CreateRequest5& r) {
    const auto snapshot = r;
    auto out = G::createNative5(r);
    if (!out) std::cerr << out.validation.message << '\n';
    CHECK(out); ++creations;
    CHECK(r == snapshot);
    CHECK(out.encrypted.size() == 136);
    CHECK(G::createNative5(r).encrypted == out.encrypted);
    const auto d = oracleDecode(out.encrypted);
    CHECK(C::decrypt(out.encrypted) == d);
    CHECK(C::encryptCandidate(d) == out.encrypted);
    G::Pokemon5ReadOnly p(out.encrypted);
    CHECK(p.valid() && p.sanityValid() && p.checksumValid() && !p.empty() && !p.partyRecord());
    CHECK(p.species() == r.species && p.form() == r.form && p.pid() == *r.pid);
    CHECK(p.nature() == r.nature && p.friendship() == r.friendship);
    CHECK(p.gender() == r.gender && p.hiddenAbility() == (r.abilitySlot == 2));
    const auto personal = G::personal5(r.species,r.form,static_cast<uint32_t>(r.game) >= 22);
    CHECK(p.ability() == G::abilityForSlot5(*personal.info,r.abilitySlot));
    CHECK(p.experience() == G::createExperience5(personal.info->growthRate,r.level));
    CHECK(p.tid() == r.trainer->tid && p.sid() == r.trainer->sid);
    CHECK(p.language() == r.trainer->language && p.originalTrainerGender() == r.trainer->gender);
    CHECK(p.originVersion() == static_cast<uint32_t>(r.game));
    CHECK(p.metLocation() == r.encounter.location && p.metLevel() == r.encounter.level && p.ball() == r.encounter.ball);
    CHECK(d[0x7b] == r.encounter.year - 2000 && d[0x7c] == r.encounter.month && d[0x7d] == r.encounter.day);
    CHECK(G::asciiName5(d,true) == r.nickname && G::asciiName5(d,false) == r.trainer->name);
    CHECK((C::read32(d,0x38) >> 30) == 2); // nickname true, egg false
    CHECK(!p.egg() && !p.fatefulEncounter() && !p.heldItem() && !p.pokerusState());
    for (size_t i = 0; i < 6; ++i) CHECK(p.ivs()[i] == r.ivs[i] && p.evs()[i] == r.evs[i]);
    for (size_t i = 0; i < 4; ++i) {
        CHECK(p.moves()[i] == r.moves[i].id && p.ppUps()[i] == r.moves[i].ppUps);
        CHECK(p.pp()[i] == r.moves[i].pp.value_or(G::maxPP5(r.moves[i].id,r.moves[i].ppUps)));
    }
    for (size_t pos : {4,5,0x0a,0x0b,0x16,0x43,0x44,0x45,0x46,0x47,0x5e,
                       0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x78,0x79,0x7a,0x7e,0x7f,0x85,0x86,0x87}) CHECK(d[pos] == 0);
    for (size_t i = 0x1e; i < 0x28; ++i) CHECK(d[i] == 0);
    for (size_t i = 0x3c; i < 0x40; ++i) CHECK(d[i] == 0);
    CHECK(d[0x42] == (r.abilitySlot == 2 ? 1 : 0));
    return out;
}
static void rejects(const G::CreateRequest5& r, G::CreateError5 expected) {
    const auto snapshot = r;
    const auto out = G::createNative5(r);
    CHECK(!out && out.encrypted.empty() && out.validation.error == expected && !out.validation.message.empty());
    CHECK(r == snapshot);
}
static void sweep() {
    std::array<bool,6> growthSeen{};
    for (const auto game : {G::CreateGame5::White,G::CreateGame5::Black,G::CreateGame5::White2,G::CreateGame5::Black2}) {
        for (uint32_t species = 1; species <= 649; ++species) {
            auto r = request(species,game);
            growthSeen[G::personal5(species,0,static_cast<uint32_t>(game) >= 22).info->growthRate] = true;
            for (uint32_t level : {1,50,100}) { r.level = level; succeeds(r); }
        }
    }
    for (bool seen : growthSeen) CHECK(seen);
    for (uint32_t nature = 0; nature < 25; ++nature) {
        auto r = request(); r.nature = nature; succeeds(r);
    }
    // Every 5-bit shuffle selector, including aliases 24..31 and PID ability bit.
    for (uint32_t selector = 0; selector < 32; ++selector) {
        auto r = request(); r.pid = 0x246000ffu | (selector << 13);
        r.abilitySlot = (*r.pid >> 16) & 1; succeeds(r);
    }
    for (uint32_t slot = 0; slot < 3; ++slot) {
        auto r = request(133); r.abilitySlot = slot;
        r.pid = 0x123400ffu | ((slot == 1 ? 1u : 0u) << 16); succeeds(r);
    }
    for (uint32_t species : {29,32,81,25}) {
        auto r = request(species); succeeds(r);
        if (species == 25) { r.pid = 0x12340000; r.gender = 1; succeeds(r); }
    }
    for (uint32_t language : {2,3,4,5,7}) {
        auto r = request(); r.trainer->language = language; succeeds(r);
    }
    auto r = request(); r.ivs.fill(31); r.evs = {255,255,0,0,0,0}; r.friendship = 255;
    r.trainer->tid = 0; r.trainer->sid = 65535; r.nickname = "0123456789"; succeeds(r);
    r.pid = 0; r.gender = 1; r.trainer->tid = 0; r.trainer->sid = 0;
    CHECK(G::Pokemon5ReadOnly(succeeds(r).encrypted).shiny()); // no implicit reroll
    for (uint32_t species : {201,386,479,550,585,586}) {
        auto f = request(species); f.form = 1; succeeds(f);
    }
    for (uint32_t species : {641,642,645,646,647}) {
        auto f = request(species); f.form = 1; rejects(f,G::CreateError5::SpeciesForm);
        f.game = G::CreateGame5::Black2; succeeds(f);
    }
    // Six exact reference thresholds at levels 1/50/100, not self-derived.
    constexpr std::array<uint32_t,6> at50{125000,125000,142500,117360,100000,156250};
    constexpr std::array<uint32_t,6> at100{1000000,600000,1640000,1059860,800000,1250000};
    for (uint8_t growth = 0; growth < 6; ++growth) {
        CHECK(G::createExperience5(growth,1) == 0);
        CHECK(G::createExperience5(growth,50) == at50[growth]);
        CHECK(G::createExperience5(growth,100) == at100[growth]);
    }
    // Pinned PKHeX Experience.Growth1 levels 69..98: catches misplaced floor.
    constexpr std::array<uint32_t,30> erratic{267406,276458,286328,296358,305767,316074,
        326531,336255,346965,357812,367807,378880,390077,400293,411686,423190,
        433572,445239,457001,467489,479378,491346,501878,513934,526049,536557,
        548720,560922,571333,583539};
    for (size_t i = 0; i < erratic.size(); ++i) {
        CHECK(G::createExperience5(1,uint32_t(i + 69)) == erratic[i]);
        auto e = request(290); e.level = uint32_t(i + 69);
        CHECK(G::Pokemon5ReadOnly(succeeds(e).encrypted).experience() == erratic[i]);
    }
    // All native move IDs and PP-Up settings; no learnset/encounter claim.
    for (uint32_t move = 1; move <= 559; ++move) for (uint32_t ups = 0; ups <= 3; ++ups) {
        auto m = request(); m.moves[0] = {move,ups,std::nullopt};
        if (G::basePP5(move) == 1 && ups) rejects(m,G::CreateError5::Moves);
        else succeeds(m);
    }
}
static void negative() {
    using E = G::CreateError5;
    const auto bad = [](auto modify, E error) { auto r = request(); modify(r); rejects(r,error); };
    bad([](auto& r){r.format=G::CreateFormat5::Party;},E::Format);
    bad([](auto& r){r.format=static_cast<G::CreateFormat5>(999);},E::Format);
    bad([](auto& r){r.game=static_cast<G::CreateGame5>(999);},E::Game);
    for (uint32_t n : {0u,650u,65536u,UINT32_MAX}) bad([=](auto& r){r.species=n;},E::SpeciesForm);
    for (uint32_t n : {1u,32u,UINT32_MAX}) bad([=](auto& r){r.form=n;},E::SpeciesForm);
    for (uint32_t n : {0u,101u,UINT32_MAX}) bad([=](auto& r){r.level=n;},E::Level);
    bad([](auto& r){r.nature=25;},E::Nature);
    for (size_t i = 0; i < 6; ++i) {
        bad([=](auto& r){r.ivs[i]=32;},E::IV);
        bad([=](auto& r){r.evs[i]=256;},E::EV);
    }
    bad([](auto& r){r.evs={255,255,1,0,0,0};},E::EV);
    bad([](auto& r){r.evs.fill(UINT32_MAX);},E::EV);
    bad([](auto& r){r.friendship=256;},E::Friendship);
    bad([](auto& r){r.trainer.reset();},E::Trainer);
    bad([](auto& r){r.trainer->tid=65536;},E::Trainer);
    bad([](auto& r){r.trainer->sid=65536;},E::Trainer);
    bad([](auto& r){r.trainer->gender=2;},E::Trainer);
    for (uint32_t n : {0,1,6,8,9,256}) bad([=](auto& r){r.trainer->language=n;},E::Language);
    for (const auto& text : {std::string(),std::string(11,'X'),std::string("A\0B",3),std::string("\xc3\xa9"),std::string("\n")})
        bad([&](auto& r){r.nickname=text;},E::Text);
    bad([](auto& r){r.trainer->name="12345678";},E::Text);
    bad([](auto& r){r.trainer->name="";},E::Text);
    bad([](auto& r){r.pid.reset();},E::PID);
    bad([](auto& r){r.gender=1;},E::Gender);
    bad([](auto& r){r.gender=256;},E::Gender);
    bad([](auto& r){r.abilitySlot=1;},E::Ability);
    bad([](auto& r){r.abilitySlot=3;},E::Ability);
    auto fixed = request(81); fixed.gender=0; rejects(fixed,E::Gender);
    auto gated = request(555); gated.form=1; rejects(gated,E::SpeciesForm);
    bad([](auto& r){r.moves[0]={};},E::Moves);
    bad([](auto& r){r.moves[0].id=560;},E::Moves);
    bad([](auto& r){r.moves[0].id=UINT32_MAX;},E::Moves);
    bad([](auto& r){r.moves[0].ppUps=4;},E::Moves);
    bad([](auto& r){r.moves[0].pp=36;},E::Moves);
    bad([](auto& r){r.moves[1].pp=1;},E::Moves);
    bad([](auto& r){r.moves[1].ppUps=1;},E::Moves);
    bad([](auto& r){r.moves[1].id=1;},E::Moves);
    bad([](auto& r){r.moves[2].id=85;},E::Moves);
    for (auto kind : {G::CreateHistory5::Unspecified,G::CreateHistory5::Egg,G::CreateHistory5::Event,G::CreateHistory5::Transfer})
        bad([=](auto& r){r.encounter.kind=kind;},E::History);
    bad([](auto& r){r.encounter.year=2023;},E::History); // non-leap Feb 29
    bad([](auto& r){r.encounter.month=13;},E::History);
    bad([](auto& r){r.encounter.day=0;},E::History);
    bad([](auto& r){r.encounter.year=2100;},E::History);
    for (uint32_t n : {0,3,138,154,30001,40001,65536}) bad([=](auto& r){r.encounter.location=n;},E::History);
    bad([](auto& r){r.encounter.level=51;},E::History);
    bad([](auto& r){r.encounter.ball=0;},E::History);
    bad([](auto& r){r.encounter.ball=26;},E::History);
    auto valid = request(); valid.moves={G::CreateMove5{1,3,56},G::CreateMove5{85,1,0},G::CreateMove5{559,0,5},G::CreateMove5{33,2,std::nullopt}};
    succeeds(valid);
    auto damaged = succeeds(request()).encrypted;
    damaged[8] ^= 1;
    CHECK(!G::Pokemon5ReadOnly(damaged).valid());
    for (size_t size : {0,135,137,219,221,236}) {
        auto wrong = damaged; wrong.resize(size);
        CHECK(!G::Pokemon5ReadOnly(wrong).sizeValid());
    }
}
static void recordIntegration() {
    auto out = succeeds(request());
    const auto original = out.encrypted;
    std::string error;
    auto edit = G::StagedPokemon5Record::create(out.encrypted,&error,false);
    CHECK(edit && error.empty() && !edit->dirty());
    CHECK(edit->stage(G::StagedPokemon5Record::Field::Nature,0,24,&error));
    CHECK(edit->stageName(true,"NEW NAME",&error));
    CHECK(edit->current().valid() && edit->current().nature() == 24);
    CHECK(out.encrypted == original);
    const auto edited = std::vector<uint8_t>(edit->stagedBytes().begin(),edit->stagedBytes().end());
    CHECK(!edit->stage(G::StagedPokemon5Record::Field::IV,0,32,&error));
    CHECK(std::equal(edited.begin(),edited.end(),edit->stagedBytes().begin()));
    edit->rollback();
    CHECK(!edit->dirty() && std::equal(original.begin(),original.end(),edit->stagedBytes().begin()));
    // Previously returned results do not alias future results or caller strings.
    auto next = request(); next.nickname="DIFFERENT";
    succeeds(next); CHECK(out.encrypted == original);
}
int main() {
    sweep(); negative(); recordIntegration();
    std::cout << "Gen V native Create: " << creations << " creation cases, " << checks << " checks passed\n";
}
