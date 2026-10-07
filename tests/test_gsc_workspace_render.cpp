#include "UI/Gen2WorkspacePresentation.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/Gen2HeldItemPicker.h"
#include <cassert>
#include <iostream>
#include <vector>
namespace {
struct Text { int x, y, w, h; std::string value; UI::Color color; };
std::vector<Text> texts;
std::vector<std::pair<uint16_t, bool>> sprites;
bool intersects(const Text& a, const Text& b) {
    return a.x < b.x+b.w && a.x+a.w > b.x && a.y < b.y+b.h && a.y+a.h > b.y;
}
}
namespace UI {
PKSEFramebuffer::PKSEFramebuffer() = default;
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle) { w = s.size()*9; h = 18; }
void PKSEFramebuffer::drawText(int x,int y,const std::string& s,Color c,TextStyle style) {
    int w,h; measureText(s,w,h,style); texts.push_back({x,y,w,h,s,c});
}
void PKSEFramebuffer::drawText(int x,int y,const char* s,Color c,TextStyle style) { drawText(x,y,std::string(s),c,style); }
void PKSEFramebuffer::drawFilledRect(int,int,int,int,Color) {}
void PKSEFramebuffer::drawFilledCircle(int,int,int,Color) {}
void PKSEFramebuffer::drawFilledRoundedRect(int,int,int,int,int,Color) {}
void PKSEFramebuffer::drawRoundedRect(int,int,int,int,int,Color,int) {}
void PKSEFramebuffer::drawSelectionHighlight(int,int,int,int) {}
void PKSEFramebuffer::drawSpriteStaticContained(int,int,int,int,int,int,const unsigned char*,int) {}
void PKSEFramebuffer::drawImageScaled(int,int,int,int,int,int,const unsigned char*,int) {}
Sprite* SpriteManager::getSprite(uint16_t species, bool shiny) { sprites.emplace_back(species,shiny); return nullptr; }
Sprite* SpriteManager::getTypeSprite(uint8_t) { return nullptr; }
}
int main() {
    UI::PKSEFramebuffer fb;
    namespace G = PokeVault::Integration::Gen2;
    namespace W = PokeBank::UIModel::Gen2Workspace;
    G::PokemonRecord p{};
    p.species = 155; p.level = 5; p.dvs = {15,15,15,15,15};
    for (auto game : {G::SourceGame::Gold, G::SourceGame::Silver, G::SourceGame::Crystal}) {
        for (bool party : {false,true}) {
            p.partyRecord = party; p.currentHP = 123; p.maxHP = 999;
            p.attack = 998; p.defense = 997; p.speed = 996; p.specialAttack = 995; p.specialDefense = 994;
            for (uint8_t status : {0,7,8,16,32,64}) {
                p.status = status;
                for (uint16_t location = 0; location < 96; ++location) {
                    p.caughtData = static_cast<uint16_t>(0xFFFF & (0xFF80 | location));
                    texts.clear();
                    UI::Gen2WorkspacePresentation::drawDataAndGraph(fb, 0, 0, 438, 260, p, game);
                    const auto gen2TitleCount = std::count_if(texts.begin(), texts.end(), [](const Text& t) {
                        return t.value == "GEN II DATA";
                    });
                    assert(gen2TitleCount == 1);
                    assert(std::none_of(texts.begin(), texts.end(), [](const Text& t) {
                        return t.value == "GEN I DATA";
                    }));
                    for (std::size_t i = 0; i < texts.size(); ++i) {
                        const auto& t = texts[i];
                        if (!(t.x >= 0 && t.y >= 0 && t.x+t.w <= 438 && t.y+t.h <= 260)) {
                            std::cerr << "Outside: " << t.value << " at " << t.x << "," << t.y << " size " << t.w << "," << t.h << '\n';
                            assert(false);
                        }
                        for (std::size_t j=i+1;j<texts.size();++j) assert(!intersects(t,texts[j]));
                    }
                }
            }
            const auto rows = W::dataRows(p, game);
            auto has = [&](const char* label) { return std::any_of(rows.begin(),rows.end(),[&](const auto& r){return r.label==label;}); };
            assert(has("HP") == party && has("Status") == party);
            assert(has("Caught history"));
            assert(has("Met") && has("Met level") && has("Time") && has("OT gender"));
            if (party) assert((W::battleStats(p) == std::array<uint16_t,6>{999,998,997,996,995,994}));
            else assert(W::battleStats(p)[0] != 999);
        }
    }
    // The cramped accepted legacy layer prioritizes real caught history over
    // auxiliary party rows, while the final 1280x720 hardware panel has room for both.
    p.partyRecord = true;
    p.currentHP = 123;
    p.maxHP = 999;
    p.status = 8;
    p.caughtData = static_cast<uint16_t>(0xFF80 | 16);
    texts.clear();
    UI::Gen2WorkspacePresentation::drawDataAndGraph(fb, 0, 0, 438, 260, p, G::SourceGame::Silver);
    const auto renderedContains = [](std::string_view needle) {
        return std::any_of(texts.begin(), texts.end(), [needle](const Text& t) {
            return t.value.find(needle) != std::string::npos;
        });
    };
    assert(renderedContains("Caught history"));
    assert(renderedContains("Met"));
    assert(renderedContains("OT gender"));
    assert(!renderedContains("HP:"));
    assert(!renderedContains("Status:"));

    texts.clear();
    UI::Gen2WorkspacePresentation::drawDataAndGraph(fb, 0, 0, 506, 312, p, G::SourceGame::Silver);
    assert(renderedContains("HP:"));
    assert(renderedContains("Status:"));
    assert(renderedContains("Caught history"));

    // Native Gold/Silver records with zero caught-data bytes must say that the
    // history was not recorded, without manufacturing met fields.
    p.partyRecord = false;
    p.caughtData = 0;
    for (auto game : {G::SourceGame::Gold, G::SourceGame::Silver}) {
        const auto rows = W::dataRows(p, game);
        const auto caught = std::find_if(rows.begin(), rows.end(), [](const auto& row) {
            return row.label == "Caught history";
        });
        assert(caught != rows.end() && caught->value == "Not recorded by Gold/Silver");
        for (const char* label : {"Met", "Met level", "Time", "OT gender"})
            assert(std::none_of(rows.begin(), rows.end(), [label](const auto& row) { return row.label == label; }));
    }

    // A Crystal-origin Pokémon retains its caught-data bytes when traded into
    // Gold/Silver. Decode the record itself rather than gating on the current save.
    namespace Native = PokeBank::UIModel::Gen2Native;
    Native::CrystalCaughtData retained{};
    retained.present = true;
    retained.timeOfDay = 3;
    retained.levelCode = 12;
    retained.originalTrainerFemale = true;
    retained.location = 16; // Goldenrod City
    p.caughtData = Native::encodeCrystalCaughtData(retained);
    for (auto game : {G::SourceGame::Gold, G::SourceGame::Silver, G::SourceGame::Crystal}) {
        const auto rows = W::dataRows(p, game);
        const auto value = [&](const char* label) -> std::string {
            const auto it = std::find_if(rows.begin(), rows.end(), [label](const auto& row) {
                return row.label == label;
            });
            assert(it != rows.end());
            return it->value;
        };
        assert(value("Met") == "Goldenrod City");
        assert(value("Met level") == "Lv 12");
        assert(value("Time") == "Night");
        assert(value("OT gender") == "Female");
        assert(value("Caught history") == (game == G::SourceGame::Crystal
            ? "Crystal caught data" : "Retained Crystal data"));
    }

    // Generation II has five displayed DV/Stat Exp rows: HP is derived and
    // there is only one stored Special pair. That one pair feeds both split stats.
    p.partyRecord = false;
    p.species = 155;
    p.level = 50;
    p.dvs = {0, 10, 10, 10, 2};
    p.statExperience = {0, 0, 0, 0, 0};
    assert(p.dvs.size() == 5 && p.statExperience.size() == 5);
    const auto lowSpecialDV = W::battleStats(p);
    p.dvs[4] = 14;
    const auto highSpecialDV = W::battleStats(p);
    assert(lowSpecialDV[4] != highSpecialDV[4]);
    assert(lowSpecialDV[5] != highSpecialDV[5]);
    p.statExperience[4] = 65535;
    const auto highSpecialExp = W::battleStats(p);
    assert(highSpecialDV[4] != highSpecialExp[4]);
    assert(highSpecialDV[5] != highSpecialExp[5]);

    namespace Held = PokeBank::UIModel::Gen2HeldItemPicker;
    const auto items = PokeBank::UIModel::Gen2PokemonEditor::heldItemChoices();
    assert(Held::itemName(218) == "TM27 — Return");
    assert(Held::itemName(219) == "TM28 — Dig");
    assert(Held::itemName(243) == "HM01 — Cut");
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        assert(Held::initialIndex(items, items[i]) == i);
        for (int direction : {-1, 1}) {
            const int horizontal = Held::move(i, items.size(), direction, 0);
            assert(horizontal / Held::columns == i / Held::columns);
            const int vertical = Held::move(i, items.size(), 0, direction);
            assert(vertical % Held::columns == i % Held::columns);
            const int paged = Held::move(i, items.size(), 0, 0, direction);
            assert(paged >= 0 && paged < static_cast<int>(items.size()));
        }
        texts.clear();
        UI::Gen2HeldItemPickerPresentation::drawList(fb, 0, 0, 992, items, i);
        assert(texts.size() <= static_cast<std::size_t>(Held::pageSize));
        for (const auto& text : texts)
            assert(text.x >= 0 && text.y >= 0 && text.x + text.w <= 992 && text.y + text.h <= 384);
        for (std::size_t j = 0; j < texts.size(); ++j)
            for (std::size_t k = j + 1; k < texts.size(); ++k) assert(!intersects(texts[j], texts[k]));
    }
    assert(Held::move(22, items.size(), 0, 1) == 26);
    assert(Held::move(5, items.size(), 0, 0, 1) == 45);
    assert(Held::move(45, items.size(), 0, 0, -1) == 5);

    // The actual shared renderer must request both sprite appearances for the
    // hovered species, including the Gen II-only dex range, without a session write.
    // Type drawing is deliberately mocked here; generation-correct type data has its
    // own executable contract in test_classic_picker_type_contract.
    for (uint16_t dex : {1,151,152,251}) {
        texts.clear(); sprites.clear();
        UI::SharedSpeciesPicker::drawContent(fb,100,76,dex,251,true,
            [](uint16_t id){ return std::to_string(id); }, [](uint16_t id){ return std::to_string(id); });
        assert((sprites == std::vector<std::pair<uint16_t,bool>>{{dex,false},{dex,true}}));
        assert(std::any_of(texts.begin(),texts.end(),[](const Text& t){return t.value == "Intended: Shiny";}));
    }
    std::cout << "Gen II production Data/Graph and shared species renderer: native gating, exact generation title, bounded nonoverlapping text, sprite identity PASS\n";
}
