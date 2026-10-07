#include "Integration/Gen1/Gen1StagedInventoryEditor.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

using namespace PokeVault::Integration::Gen1;
using PokeVault::Inventory::ClassicPocket;

namespace {
struct Layout {
    RegionLayout region;
    std::size_t checksum, mainLength, bag, pc, party, currentBox, currentBoxIndex, starter, friendship;
    std::size_t boxCapacity;
};
constexpr Layout INTL{RegionLayout::International,0x3523,0x0F8B,0x25C9,0x27E6,0x2F2C,0x30C0,0x284C,0x29C3,0x271C,20};
constexpr Layout JPN{RegionLayout::Japanese,0x3594,0x0FFC,0x25C4,0x27DC,0x2ED5,0x302D,0x2842,0x29B9,0x2712,30};

uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value=0xFF;
    for (uint8_t byte:bytes) value=static_cast<uint8_t>(value-byte);
    return value;
}
void writeBlock(std::vector<uint8_t>& raw, std::size_t offset, std::size_t capacity,
                const std::vector<InventoryEntry>& entries) {
    std::fill(raw.begin()+offset, raw.begin()+offset+1+capacity*2+1, 0);
    raw[offset]=static_cast<uint8_t>(entries.size());
    for (std::size_t i=0;i<entries.size();++i) {
        raw[offset+1+i*2]=entries[i].itemId;
        raw[offset+2+i*2]=entries[i].quantity;
    }
    raw[offset+1+entries.size()*2]=0xFF;
}
std::vector<uint8_t> fixture(SourceGame game, const Layout& layout,
                             std::vector<InventoryEntry> bag={{20,3},{45,1},{201,2}},
                             std::vector<InventoryEntry> pc={{1,7},{83,4}}) {
    std::vector<uint8_t> raw(kRawSaveSize,0);
    raw[layout.party]=0; raw[layout.party+1]=0xFF;
    raw[layout.currentBox]=0; raw[layout.currentBox+1]=0xFF;
    raw[layout.currentBoxIndex]=2; // stored boxes intentionally not initialized.
    raw[layout.starter]=game==SourceGame::Yellow?0x54:0x99;
    raw[layout.friendship]=game==SourceGame::Yellow?90:0;
    writeBlock(raw,layout.bag,20,bag);
    writeBlock(raw,layout.pc,50,pc);
    raw[layout.checksum]=diff8(std::span<const uint8_t>(raw).subspan(0x2598,layout.mainLength));
    return raw;
}
std::unique_ptr<StagedInventoryEditor> open(SourceGame game, const Layout& layout,
                                             const std::vector<InventoryEntry>& bag={{20,3},{45,1},{201,2}}) {
    auto raw=fixture(game,layout,bag);
    auto parsed=parse(raw,game);
    assert(parsed);
    std::string error;
    auto editor=StagedInventoryEditor::create(*parsed.save,error);
    assert(editor && error.empty());
    return editor;
}
}

int main() {
    {
        auto editor=open(SourceGame::Yellow,INTL);
        const auto original=std::vector<uint8_t>(editor->originalBytes().begin(),editor->originalBytes().end());
        std::string error;
        auto medicines=editor->entries(ClassicPocket::Medicines,error);
        assert(error.empty() && medicines.size()==1 && medicines[0].itemId==20);
        auto balls=editor->entries(ClassicPocket::Balls,error);
        assert(error.empty() && balls.empty()); // valid empty virtual category, not an error.
        assert(!editor->stageSetQuantity(ClassicPocket::Balls,13,1,error)); // no modern Dusk Ball interpretation.
        assert(editor->stageSetQuantity(ClassicPocket::Medicines,20,25,error));
        assert(editor->stageSetQuantity(ClassicPocket::Balls,4,10,error));
        assert(!editor->stageSetQuantity(ClassicPocket::KeyItems,45,2,error));
        assert(editor->hasPendingChanges() && editor->pendingChanges().size()==2);
        assert(std::equal(editor->originalBytes().begin(),editor->originalBytes().end(),original.begin()));

        auto output=editor->finalize(error);
        assert(!output.empty() && error.empty());
        assert(std::equal(original.begin(),original.end(),editor->originalBytes().begin()));
        auto strict=parse(output,SourceGame::Yellow);
        assert(strict);
        auto decoded=decodeInventory(output,RegionLayout::International);
        assert(decoded.available);
        auto potion=std::find_if(decoded.bag.begin(),decoded.bag.end(),[](const auto& e){return e.itemId==20;});
        auto ball=std::find_if(decoded.bag.begin(),decoded.bag.end(),[](const auto& e){return e.itemId==4;});
        assert(potion!=decoded.bag.end() && potion->quantity==25);
        assert(ball!=decoded.bag.end() && ball->quantity==10);
        assert(output[INTL.checksum]==diff8(std::span<const uint8_t>(output).subspan(0x2598,INTL.mainLength)));

        assert(editor->stageRemove(ClassicPocket::Medicines,20,error));
        output=editor->finalize(error);
        decoded=decodeInventory(output,RegionLayout::International);
        assert(decoded.available);
        assert(std::none_of(decoded.bag.begin(),decoded.bag.end(),[](const auto& e){return e.itemId==20;}));
        editor->discard();
        assert(!editor->hasPendingChanges());
        assert(std::equal(editor->stagedBytes().begin(),editor->stagedBytes().end(),editor->originalBytes().begin()));
    }
    {
        auto editor=open(SourceGame::Red,JPN);
        std::string error;
        assert(editor->stageSetQuantity(ClassicPocket::Balls,4,9,error));
        auto output=editor->finalize(error);
        assert(!output.empty());
        assert(parse(output,SourceGame::Red));
        assert(output[JPN.checksum]==diff8(std::span<const uint8_t>(output).subspan(0x2598,JPN.mainLength)));
    }
    {
        // Full Bag must reject a new stack without overwriting any existing entry.
        std::vector<InventoryEntry> full;
        for (uint8_t id: {1,2,3,5,6,10,11,12,13,14,15,16,17,18,19,20,29,30,31,32}) full.push_back({id,1});
        auto editor=open(SourceGame::Blue,INTL,full);
        const auto before=std::vector<uint8_t>(editor->stagedBytes().begin(),editor->stagedBytes().end());
        std::string error;
        assert(!editor->stageSetQuantity(ClassicPocket::Balls,4,1,error));
        assert(error.find("No free slots")!=std::string::npos);
        assert(std::equal(before.begin(),before.end(),editor->stagedBytes().begin()));
    }
    {
        // Middle removal compacts the real pair list and keeps the terminator valid.
        auto editor=open(SourceGame::Red,INTL,{{20,3},{11,2},{4,5}});
        std::string error;
        assert(editor->stageRemove(ClassicPocket::Medicines,11,error));
        auto output=editor->finalize(error);
        auto decoded=decodeInventory(output,RegionLayout::International);
        assert(decoded.available && decoded.bag.size()==2);
        assert(decoded.bag[0].itemId==20 && decoded.bag[1].itemId==4);
        assert(output[INTL.bag]==2);
        assert(output[INTL.bag+1+2*2]==0xFF);
    }

    std::cout << "RBY staged inventory editor: PASS\n";
    return 0;
}
