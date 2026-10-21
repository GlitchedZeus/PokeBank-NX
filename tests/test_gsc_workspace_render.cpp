#include <iterator>
#include <fstream>
#include "UI/Gen2WorkspacePresentation.h"
#include "UI/NatureStatPresentation.h"
#include "UI/SharedEditorChrome.h"
#include "UI/SharedSpeciesPicker.h"
#include "UI/Gen2HeldItemPicker.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
namespace {
struct Text { int x, y, w, h; std::string value; UI::Color color; };
std::vector<Text> texts;
std::vector<std::pair<uint16_t, bool>> sprites;
std::vector<uint8_t> spriteForms;
// The native framebuffer clips extra rows for smooth touch scrolling. The
// renderer test double must record only visible pixels, not every drawText call.
bool clipActive = false;
int clipX = 0, clipY = 0, clipW = 0, clipH = 0;
bool intersects(const Text& a, const Text& b) {
    return a.x < b.x+b.w && a.x+a.w > b.x && a.y < b.y+b.h && a.y+a.h > b.y;
}
}
namespace UI {
const TouchGestureSnapshot& latestTouchGesture() noexcept {static const TouchGestureSnapshot value{};return value;}
PKSEFramebuffer::PKSEFramebuffer() : width(1280),height(720) {}
PKSEFramebuffer::~PKSEFramebuffer() = default;
void PKSEFramebuffer::measureText(const std::string& s, int& w, int& h, TextStyle) { w = s.size()*9; h = 18; }
void PKSEFramebuffer::drawText(int x,int y,const std::string& s,Color c,TextStyle style) {
    int w,h; measureText(s,w,h,style);
    int left = x, top = y, right = x + w, bottom = y + h;
    if (clipActive) {
        left = std::max(left, clipX);
        top = std::max(top, clipY);
        right = std::min(right, clipX + clipW);
        bottom = std::min(bottom, clipY + clipH);
    }
    if (left < right && top < bottom)
        texts.push_back({left, top, right - left, bottom - top, s, c});
}
void PKSEFramebuffer::drawText(int x,int y,const char* s,Color c,TextStyle style) { drawText(x,y,std::string(s),c,style); }
void PKSEFramebuffer::drawFilledRect(int,int,int,int,Color) {}
void PKSEFramebuffer::drawSoftShadow(int,int,int,int,int) {}
void PKSEFramebuffer::drawHDivider(int,int,int) {}
void PKSEFramebuffer::popTransform() {}
void PKSEFramebuffer::drawSymbol(int x,int y,const std::string& s,Color c,TextStyle style) {drawText(x,y,s,c,style);}
int PKSEFramebuffer::lineHeight(TextStyle) const {return 18;}
void PKSEFramebuffer::drawFilledCircle(int,int,int,Color) {}
void PKSEFramebuffer::drawFilledRoundedRect(int,int,int,int,int,Color) {}
void PKSEFramebuffer::drawRoundedRect(int,int,int,int,int,Color,int) {}
void PKSEFramebuffer::drawCircle(int,int,int,Color,int) {}
void PKSEFramebuffer::setClipRect(int x,int y,int w,int h) {
    clipActive = true; clipX = x; clipY = y; clipW = w; clipH = h;
}
void PKSEFramebuffer::clearClip() { clipActive = false; }
void PKSEFramebuffer::drawSelectionHighlight(int,int,int,int) {}
void PKSEFramebuffer::drawSpriteStaticContained(int,int,int,int,int,int,const unsigned char*,int) {}
void PKSEFramebuffer::drawImageScaled(int,int,int,int,int,int,const unsigned char*,int) {}
Sprite* SpriteManager::getSprite(uint16_t species, bool shiny) { sprites.emplace_back(species,shiny); return nullptr; }
Sprite* SpriteManager::getSprite(uint16_t species, uint8_t form, bool shiny) {
    sprites.emplace_back(species,shiny); spriteForms.push_back(form); return nullptr;
}
Sprite* SpriteManager::getTypeSprite(uint8_t) { return nullptr; }
// Shared Gen II held-item picker optionally renders matching ROMFS artwork.
// This isolated render host test deliberately has no image/assets backend.
Sprite* SpriteManager::getItemSprite(const std::string&) { return nullptr; }
// The shared species chooser (SharedSpeciesPicker::drawContent) previews the box icon; no
// assets backend here either, so the chooser draws its text-only fallback.
Sprite* SpriteManager::getIconSprite(uint16_t, bool) { return nullptr; }
}
int main() {
    UI::PKSEFramebuffer fb;
    namespace P=PokeBank::UIModel::SharedEditorPresentation;
    for(int rowCount=1;rowCount<=20;++rowCount) {
        std::vector<std::pair<std::string,std::string>> rows;
        for(int i=0;i<rowCount;++i)rows.push_back({"Native field "+std::to_string(i),
            i==0?"A deliberately long stored value that must stay inside its own column without overlapping the next column":
            "Native value "+std::to_string(i)});
        std::vector<int> touchButtons{1,2,3};texts.clear();
        UI::SharedEditorChrome::drawOriginLegalityPanel(fb,touchButtons,rows,P::LegalityState::NotChecked);
        assert(touchButtons.empty());
        assert(UI::g_navHits.size()==1); // Only the visible B Back footer remains.
        assert(std::any_of(texts.begin(),texts.end(),[](const Text& t){return t.value=="Not checked";}));
        for(size_t i=0;i<texts.size();++i) {
            const auto& t=texts[i];
            assert(t.x>=0 && t.y>=0 && t.x+t.w<=1280 && t.y+t.h<=720);
            for(size_t j=i+1;j<texts.size();++j)assert(!intersects(t,texts[j]));
        }
    }
    texts.clear();
    UI::SharedPokemonShell::drawPortrait(fb,0,0,201,true,23);
    assert(sprites.back() == std::make_pair(uint16_t{201},true) && spriteForms.back() == 23);
    UI::SharedPokemonShell::drawPortrait(fb,0,0,25,false);
    assert(spriteForms.back() == 0);
    sprites.clear(); spriteForms.clear(); texts.clear();
    const auto sameColor = [](UI::Color a, UI::Color b) {
        return a.r==b.r && a.g==b.g && a.b==b.b && a.a==b.a;
    };
    for (unsigned nature=0;nature<25;++nature) {
        texts.clear();
        UI::NatureStats::drawEffect(fb,0,0,nature);
        if (PokeBank::UIModel::NatureStatEffect::neutral(nature)) {
            assert(texts.size()==1 && texts[0].value.find("Neutral")!=std::string::npos);
        } else {
            assert(texts.size()==2 && texts[0].value.starts_with("+10%") && texts[1].value.starts_with("-10%"));
            assert(sameColor(texts[0].color,UI::Colors::Success) && sameColor(texts[1].color,UI::Colors::Error));
            assert(!intersects(texts[0],texts[1]));
        }
        assert(sameColor(UI::NatureStats::statColor(nature,0,UI::Colors::TextDim),UI::Colors::TextDim));
    }
    texts.clear();
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
                        // Phase 9: text in the GEN II DATA pane (x 12..210) stays inside it and
                        // never reaches the BATTLE STATS pane that starts at x 220.
                        if (t.x < 12 + 198) {
                            if (t.x + t.w > 12 + 198 - 10) {
                                std::cerr << "Data text spills: " << t.value << " right " << t.x + t.w << '\n';
                                assert(false);
                            }
                        }
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
    // The final hardware panel (506 x 304 from the 1280x720 shell): every Crystal landmark, time,
    // level and OT gender keeps all data text inside the data pane and above the panel bottom.
    for (bool party : {false, true}) {
        p.partyRecord = party;
        for (uint16_t location = 0; location < 128; ++location) {
            for (uint16_t time = 1; time <= 3; ++time) {
                p.caughtData = static_cast<uint16_t>((time << 14) | (63u << 8) | 0x80 | location);
                texts.clear();
                UI::Gen2WorkspacePresentation::drawDataAndGraph(fb, 0, 0, 506, 304, p, G::SourceGame::Crystal);
                for (const auto& t : texts) {
                    if (t.x >= 12 + 198) continue; // graph pane
                    assert(t.x >= 12 && t.x + t.w <= 12 + 198 - 10);
                    assert(t.y >= 0 && t.y + t.h <= 304);
                }
            }
        }
    }
    // A value too long for the pane wraps (and a word longer than the pane breaks); rows that
    // cannot fit end with "..." rather than overflowing.
    {
        const auto measure = [](const std::string& text) { return static_cast<int>(text.size()) * 9; };
        std::vector<W::DataRow> rows{{"Met", "A landmark name that is far too long for this narrow pane"},
                                     {"Note", "Supercalifragilisticexpialidocious"}};
        for (int i = 0; i < 20; ++i) rows.push_back({"Row", std::to_string(i)});
        // Gen II rows go through the ONE shared data-pane layout.
        {
            std::ifstream in("include/UI/Gen2WorkspacePresentation.h", std::ios::binary);
            const std::string header((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            assert(header.find("Pane::layoutRows(rows, Pane::kTextWidth") != std::string::npos);
            assert(header.find("layoutDataRows") == std::string::npos); // no second Gen II copy
        }
        namespace Pane = PokeBank::UIModel::SharedDataPane;
        const auto lines = Pane::layoutRows(rows, Pane::kTextWidth, 200, measure);
        assert(!lines.empty() && static_cast<int>(lines.size()) == 200 / Pane::kLineHeight);
        for (const auto& line : lines) {
            assert(measure(line.text) <= Pane::kTextWidth);
            assert(line.y + Pane::kLineHeight <= 200);
        }
        assert(lines.front().label && lines.front().text == "Met");
        assert(lines.back().text.size() >= 3 && lines.back().text.substr(lines.back().text.size() - 3) == "...");
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
    // The item sprite is the verified GSC-era MOVE TYPE disc, not the
    // item number: TM27 Return -> Normal; TM28 Dig -> Ground; HM01 Cut -> Normal.
    // Each generation maps machine IDs separately, so number-only art is wrong.
    assert(Held::spriteName(218) == "tm-normal");
    assert(Held::spriteName(219) == "tm-ground");
    assert(Held::spriteName(243) == "hm-normal");
    assert(Held::spriteName(193) == "tm-unknown"); // Original GSC ??? Curse TM03
    assert(Held::spriteName(0) == "None");
    // GSC bag/held modal uses dedicated 32px original-game motifs; the
    // corresponding 160x144 stationery is presented only in the mail preview.
    assert(Held::spriteName(158) == "gsc-mail-icon-flower-mail");
    assert(Held::spriteName(181) == "gsc-mail-icon-surf-mail");
    assert(Held::spriteName(182) == "gsc-mail-icon-liteblue-mail");
    assert(Held::spriteName(183) == "gsc-mail-icon-portraitmail");
    assert(Held::spriteName(184) == "gsc-mail-icon-lovely-mail");
    assert(Held::spriteName(185) == "gsc-mail-icon-eon-mail");
    assert(Held::spriteName(186) == "gsc-mail-icon-morph-mail");
    assert(Held::spriteName(187) == "gsc-mail-icon-bluesky-mail");
    assert(Held::spriteName(188) == "gsc-mail-icon-music-mail");
    assert(Held::spriteName(189) == "gsc-mail-icon-mirage-mail");
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
        // Render the same accepted Gen III modal geometry used by Gen II-IV.
        UI::Gen2HeldItemPickerPresentation::drawList(fb, 0, 0, items, i);
        assert(texts.size() <= static_cast<std::size_t>(Held::pageSize));
        for (const auto& text : texts)
            assert(text.x >= 0 && text.y >= 0 &&
                   text.x + text.w <= PokeBank::UIModel::SharedHeldItemPicker::modalWidth &&
                   text.y + text.h <= PokeBank::UIModel::SharedHeldItemPicker::modalHeight);
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
        assert(std::any_of(texts.begin(),texts.end(),[](const Text& t){return t.value == "Shiny: Yes";}));
    }
    std::cout << "Gen II production Data/Graph and shared species renderer: native gating, exact generation title, bounded nonoverlapping text, sprite identity PASS\n";
}
