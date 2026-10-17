#ifndef POKEBANK_GEN5_BAG_SCREEN_H
#define POKEBANK_GEN5_BAG_SCREEN_H

#include "Integration/Gen5/Gen5StagedPokemonWorkspace.h"
#include "UI/InventoryUIContract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Session-local control model for the Gen V Bag inside the ONE shared
// Items/Inventory surface (the same panel Gen III/IV use). It owns only the
// pocket/row cursor and the Add/Remove modal steps; every quantity, Add and
// Remove goes through Gen5StagedBag, which audits the exact 4-byte slot.
// Controller and touch use the same entry points:
//   A / tap selected row   -> quantity keypad (mutable pockets only)
//   X / footer X           -> Add picker      (mutable pockets only)
//   Y / footer Y           -> Remove confirm, second Y stages the removal
//   L/R / header arrows    -> previous/next native pocket
// Key Items and TMs/HMs are view-only; an unrecognized bag is unavailable.
class Gen5BagScreen {
public:
    using Workspace=Gen5StagedPokemonWorkspace;
    enum class Surface : uint8_t { Browse, AddPicker, ConfirmRemove };

    struct QuantityRequest {
        size_t pocket = 0;
        size_t row = 0;
        uint16_t itemId = 0;
        uint16_t current = 0;
        uint16_t maximum = 0;
    };
    struct FooterEntry { const char* glyph; const char* label; };

    [[nodiscard]] Surface surface() const noexcept { return surface_; }
    [[nodiscard]] size_t pocket() const noexcept { return pocket_; }
    [[nodiscard]] size_t row() const noexcept { return row_; }
    [[nodiscard]] const std::vector<uint16_t>& addChoices() const noexcept { return addChoices_; }
    [[nodiscard]] size_t addIndex() const noexcept { return addIndex_; }
    [[nodiscard]] uint16_t chosenAddItem() const noexcept {
        return addIndex_<addChoices_.size()?addChoices_[addIndex_]:0;
    }
    [[nodiscard]] const std::optional<Trainer::InventoryItem>& pendingRemove() const noexcept {
        return removeTarget_;
    }

    [[nodiscard]] static constexpr bool mutablePocket(size_t pocket) noexcept {
        return pocket<BagPocket5Count &&
               pocket!=static_cast<size_t>(BagPocket5::KeyItems) &&
               pocket!=static_cast<size_t>(BagPocket5::Machines);
    }
    [[nodiscard]] static bool available(const Workspace& ws) {
        return ws.bagPresentation().has_value();
    }
    [[nodiscard]] static size_t rowCount(const Workspace& ws,size_t pocket) {
        const auto bag=ws.bagPresentation();
        return bag && pocket<BagPocket5Count ? (*bag)[pocket].size() : 0;
    }
    [[nodiscard]] std::optional<Trainer::InventoryItem> selected(const Workspace& ws) const {
        const auto bag=ws.bagPresentation();
        if(!bag || pocket_>=BagPocket5Count || row_>=(*bag)[pocket_].size())return std::nullopt;
        return (*bag)[pocket_][row_];
    }

    // Shared contract: the same A/X/Y availability rules Gen III/IV show.
    [[nodiscard]] PokeBank::UIModel::InventoryActionAvailability
    availability(const Workspace& ws) const {
        return PokeBank::UIModel::inventoryActionAvailability(
            available(ws),mutablePocket(pocket_),selected(ws).has_value(),
            mutablePocket(pocket_));
    }
    [[nodiscard]] std::vector<FooterEntry> footer(const Workspace& ws) const {
        if(surface_==Surface::AddPicker)
            return {{"D-pad/Stick","Choose"},{"L/R","Page"},{"A","Set Quantity"},{"B","Cancel"}};
        if(surface_==Surface::ConfirmRemove)
            return {{"Y","Confirm Remove"},{"B","Cancel"}};
        std::vector<FooterEntry> out{{"L/R","Pocket"}};
        const auto can=availability(ws);
        if(can.editAmount)out.push_back({"A","Quantity"});
        if(can.addItem)out.push_back({"X","Add Item"});
        if(can.removeItem)out.push_back({"Y","Remove"});
        if(ws.hasChanges())out.push_back({"+","Review"});
        out.push_back({"B","Back"});
        return out;
    }
    [[nodiscard]] std::string status(const Workspace& ws) const {
        if(!available(ws))
            return "Gen V bag failed native validation; Items are unavailable";
        if(!mutablePocket(pocket_))
            return "Key Items and TMs/HMs come from story progress; view only";
        return "Bag edits are staged in app memory; the source SAV5 is unchanged";
    }

    void reset() noexcept {
        surface_=Surface::Browse;
        pocket_=0;
        row_=0;
        clearModal();
    }
    void changePocket(int delta) noexcept {
        if(surface_!=Surface::Browse)return;
        constexpr int count=static_cast<int>(BagPocket5Count);
        pocket_=static_cast<size_t>(((static_cast<int>(pocket_)+delta)%count+count)%count);
        row_=0;
    }
    void moveRow(int delta,const Workspace& ws) {
        if(surface_!=Surface::Browse)return;
        const size_t rows=rowCount(ws,pocket_);
        if(rows==0) {row_=0;return;}
        row_=static_cast<size_t>(std::clamp(static_cast<long>(row_)+delta,
                                            0L,static_cast<long>(rows)-1));
    }
    // Gen I-IV Items convention (TrainerViewScreenBase Items list): Up/Down
    // wrap, Left/Right page with wrap and land on the page's first item, a
    // vertical swipe that starts on a row scrolls by whole rows, and a
    // completed tap on a row selects it and acts like A.
    void stepRow(int direction,const Workspace& ws) {
        if(surface_!=Surface::Browse)return;
        const long rows=static_cast<long>(rowCount(ws,pocket_));
        if(rows==0) {row_=0;return;}
        row_=static_cast<size_t>(((static_cast<long>(row_)+(direction<0?-1:1))%rows+rows)%rows);
    }
    void stepPage(int direction,size_t perPage,const Workspace& ws) {
        if(surface_!=Surface::Browse || perPage==0)return;
        const size_t rows=rowCount(ws,pocket_);
        const size_t pages=(rows+perPage-1)/perPage;
        if(pages<=1)return;
        const size_t page=row_/perPage;
        const size_t next=direction<0?(page+pages-1)%pages:(page+1)%pages;
        row_=std::min(next*perPage,rows-1);
    }
    [[nodiscard]] bool tapRow(size_t row,const Workspace& ws) {
        if(surface_!=Surface::Browse || row>=rowCount(ws,pocket_))return false;
        row_=row;
        return true;
    }
    // Keep the cursor valid after any refresh (remove of the last row, etc).
    void clamp(const Workspace& ws) {
        const size_t rows=rowCount(ws,pocket_);
        if(row_>=rows)row_=rows?rows-1:0;
    }

    [[nodiscard]] std::optional<QuantityRequest> beginQuantity(const Workspace& ws,
                                                               std::string& error) const {
        if(surface_!=Surface::Browse || !availability(ws).editAmount) {
            error=available(ws)?"Gen V quantity is fixed in this pocket"
                               :"Gen V bag is unavailable";
            return std::nullopt;
        }
        const auto item=selected(ws);
        if(!item) {error="Choose an existing Gen V item; X adds a new stack";return std::nullopt;}
        error.clear();
        return QuantityRequest{pocket_,row_,item->itemId,item->count,
                               bagPocketLayout5(ws.sourceSave().family(),pocket_).maxCount};
    }
    template<class Refresh>
    [[nodiscard]] bool applyQuantity(Workspace& ws,const QuantityRequest& request,
                                     int value,Refresh&& refresh,std::string& error) {
        const auto item=selected(ws);
        if(surface_!=Surface::Browse || request.pocket!=pocket_ || request.row!=row_ ||
           !item || item->itemId!=request.itemId || item->count!=request.current) {
            error="Gen V item selection changed; quantity edit cancelled";
            return false;
        }
        if(value<1 || value>static_cast<int>(request.maximum)) {
            error="Gen V item quantity outside native limits";
            return false;
        }
        if(value==static_cast<int>(request.current)) {error.clear();return true;}
        return mutate(ws,std::forward<Refresh>(refresh),error,[&](Gen5StagedBag& bag,std::string* why) {
            return bag.setQuantity(pocket_,row_,static_cast<uint16_t>(value),why);
        });
    }

    [[nodiscard]] bool openAdd(Workspace& ws,std::string& error) {
        if(surface_!=Surface::Browse || !availability(ws).addItem) {
            error=available(ws)?"Gen V Key Items and TMs/HMs cannot be added"
                               :"Gen V bag is unavailable";
            return false;
        }
        const auto bag=ws.bagPresentation();
        addChoices_.clear();
        for(const uint16_t id:gen5BagChoices(ws.sourceSave().family(),pocket_)) {
            const bool present=std::any_of((*bag)[pocket_].begin(),(*bag)[pocket_].end(),
                [id](const auto& stack){return stack.itemId==id;});
            if(!present)addChoices_.push_back(id);
        }
        if(addChoices_.empty()) {
            error="No new native Gen V items are available for this pocket";
            return false;
        }
        if(rowCount(ws,pocket_)>=bagPocketLayout5(ws.sourceSave().family(),pocket_).slots) {
            addChoices_.clear();
            error="Gen V pocket has no empty native slot";
            return false;
        }
        addIndex_=0;
        surface_=Surface::AddPicker;
        error.clear();
        return true;
    }
    void moveAddCursor(int delta) noexcept {
        if(surface_!=Surface::AddPicker || addChoices_.empty())return;
        addIndex_=static_cast<size_t>(std::clamp(static_cast<long>(addIndex_)+delta,
                                                 0L,static_cast<long>(addChoices_.size())-1));
    }
    // Touch selects a picker row; a second tap on it is the same as A.
    [[nodiscard]] bool tapAddChoice(size_t index) noexcept {
        if(surface_!=Surface::AddPicker || index>=addChoices_.size())return false;
        const bool focused=index==addIndex_;
        addIndex_=index;
        return focused;
    }
    [[nodiscard]] uint16_t addMaximum(const Workspace& ws) const noexcept {
        return surface_==Surface::AddPicker
            ? bagPocketLayout5(ws.sourceSave().family(),pocket_).maxCount : 0;
    }
    template<class Refresh>
    [[nodiscard]] bool confirmAdd(Workspace& ws,uint16_t itemId,int quantity,
                                  Refresh&& refresh,std::string& error) {
        if(surface_!=Surface::AddPicker || itemId==0 || itemId!=chosenAddItem()) {
            error="Gen V Add selection changed; cancelled";
            return false;
        }
        if(quantity<1 || quantity>static_cast<int>(addMaximum(ws))) {
            error="Gen V item quantity outside native limits";
            return false;
        }
        const bool ok=mutate(ws,std::forward<Refresh>(refresh),error,
            [&](Gen5StagedBag& bag,std::string* why) {
                return bag.add(pocket_,itemId,static_cast<uint16_t>(quantity),why);
            });
        if(!ok)return false;
        clearModal();
        surface_=Surface::Browse;
        const auto bag=ws.bagPresentation();
        for(size_t i=0;bag && i<(*bag)[pocket_].size();++i)
            if((*bag)[pocket_][i].itemId==itemId)row_=i;
        return true;
    }

    [[nodiscard]] bool requestRemove(const Workspace& ws,std::string& error) {
        if(surface_!=Surface::Browse || !availability(ws).removeItem) {
            error=available(ws)?"Gen V Key Items and TMs/HMs cannot be removed"
                               :"Gen V bag is unavailable";
            return false;
        }
        removeTarget_=selected(ws);
        removeRow_=row_;
        surface_=Surface::ConfirmRemove;
        error.clear();
        return true;
    }
    template<class Refresh>
    [[nodiscard]] bool confirmRemove(Workspace& ws,Refresh&& refresh,std::string& error) {
        const auto item=selected(ws);
        if(surface_!=Surface::ConfirmRemove || !removeTarget_ || row_!=removeRow_ || !item ||
           item->itemId!=removeTarget_->itemId || item->count!=removeTarget_->count) {
            cancel();
            error="Gen V item changed; remove cancelled";
            return false;
        }
        surface_=Surface::Browse;
        const bool ok=mutate(ws,std::forward<Refresh>(refresh),error,
            [&](Gen5StagedBag& bag,std::string* why) { return bag.remove(pocket_,row_,why); });
        clearModal();
        clamp(ws);
        return ok;
    }
    void cancel() noexcept {
        clearModal();
        surface_=Surface::Browse;
    }

private:
    void clearModal() noexcept {
        addChoices_.clear();
        addIndex_=0;
        removeTarget_.reset();
        removeRow_=0;
    }
    // Snapshot the whole workspace; a refused bag edit or a refused shared
    // presentation refresh restores every staged byte exactly.
    template<class Refresh,class Op>
    [[nodiscard]] static bool mutate(Workspace& ws,Refresh&& refresh,std::string& error,Op&& op) {
        Workspace snapshot=ws;
        auto* bag=ws.stagedBag(&error);
        if(!bag)return false;
        if(!op(*bag,&error)) {
            ws=std::move(snapshot);
            return false;
        }
        if(!refresh(error)) {
            ws=std::move(snapshot);
            std::string restored;
            (void)refresh(restored);
            if(error.empty())error="Gen V bag presentation refresh failed; edit rolled back";
            return false;
        }
        error.clear();
        return true;
    }

    Surface surface_=Surface::Browse;
    size_t pocket_=0;
    size_t row_=0;
    std::vector<uint16_t> addChoices_;
    size_t addIndex_=0;
    std::optional<Trainer::InventoryItem> removeTarget_;
    size_t removeRow_=0;
};

} // namespace PokeVault::Integration::Gen5
#endif
