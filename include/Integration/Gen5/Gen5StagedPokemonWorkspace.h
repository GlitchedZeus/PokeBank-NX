#ifndef POKEBANK_GEN5_STAGED_WORKSPACE_H
#define POKEBANK_GEN5_STAGED_WORKSPACE_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// The original SAV5 bytes and native source's backup copies are immutable.
// This app-owned workspace accumulates encrypted PK5 changes by explicit
// verified slot identity ONLY. It cannot serialize, inject or overwrite a SAV.
class Gen5StagedPokemonWorkspace {
public:
    enum class Region : uint8_t { Party, Box };
    struct Slot {
        Region region = Region::Party;
        size_t box = 0;  // zero for Party
        size_t slot = 0;
        bool operator<(const Slot& other) const noexcept {
            if(region!=other.region)return region<other.region;
            if(box!=other.box)return box<other.box;
            return slot<other.slot;
        }
    };
    struct PendingChange {
        Slot location;
        std::vector<uint8_t> before;
        std::vector<uint8_t> after;
    };

    explicit Gen5StagedPokemonWorkspace(const Gen5ReadOnlySave& validated)
        : source_(validated) {}

    [[nodiscard]] const Gen5ReadOnlySave& sourceSave() const noexcept { return source_; }

    [[nodiscard]] bool hasChanges() const noexcept {
        for(const auto& pair:pending_)if(pair.second.dirty())return true;
        return false;
    }
    [[nodiscard]] size_t changedRecordCount() const noexcept {
        size_t n=0;
        for(const auto& pair:pending_)if(pair.second.dirty())++n;
        return n;
    }

    [[nodiscard]] bool stageParty(size_t slot,StagedPokemon5Record::Field field,
                                   size_t stat,uint32_t value,std::string* error=nullptr) {
        return stageSlot({Region::Party,0,slot},field,stat,value,error);
    }
    [[nodiscard]] bool stageBox(size_t box,size_t slot,StagedPokemon5Record::Field field,
                                 size_t stat,uint32_t value,std::string* error=nullptr) {
        return stageSlot({Region::Box,box,slot},field,stat,value,error);
    }

    [[nodiscard]] std::optional<Pokemon5ReadOnly> viewParty(size_t slot) const {
        return view({Region::Party,0,slot});
    }
    [[nodiscard]] std::optional<Pokemon5ReadOnly> viewBox(size_t box,size_t slot) const {
        return view({Region::Box,box,slot});
    }
    [[nodiscard]] std::vector<PendingChange> pendingReview() const {
        std::vector<PendingChange> out;
        for(const auto& [slot,record]:pending_) {
            if(!record.dirty())continue;
            out.push_back({slot,
                {record.originalBytes().begin(),record.originalBytes().end()},
                {record.stagedBytes().begin(),record.stagedBytes().end()}});
        }
        return out;
    }
    void discardAll() noexcept { pending_.clear(); }

private:
    [[nodiscard]] std::optional<Pokemon5ReadOnly> sourceSlot(const Slot& s) const {
        if(s.region==Region::Party) {
            // Undeclared party slots are not editable.
            if(s.slot>=source_.partyCount() || s.box!=0)return std::nullopt;
            return source_.partyPokemon(s.slot);
        }
        return source_.boxPokemon(s.box,s.slot);
    }
    [[nodiscard]] std::optional<Pokemon5ReadOnly> view(const Slot& s) const {
        const auto it=pending_.find(s);
        if(it!=pending_.end())return it->second.current();
        return sourceSlot(s);
    }
    bool stageSlot(const Slot& location,StagedPokemon5Record::Field field,
                   size_t stat,uint32_t value,std::string* error) {
        const auto it=pending_.find(location);
        if(it!=pending_.end()) {
            const bool ok=it->second.stage(field,stat,value,error);
            if(ok && !it->second.dirty())pending_.erase(it);
            return ok;
        }
        const auto old=sourceSlot(location);
        if(!old || !old->valid() || old->empty()) {
            if(error)*error="Gen V slot is absent, empty, or invalid; no edit allowed";
            return false;
        }
        auto edit=StagedPokemon5Record::create(old->originalEncryptedBytes(),error);
        if(!edit)return false;
        if(!edit->stage(field,stat,value,error))return false;
        if(edit->dirty())pending_.emplace(location,std::move(*edit));
        if(error)error->clear();
        return true;
    }

    Gen5ReadOnlySave source_;
    std::map<Slot,StagedPokemon5Record> pending_;
};

} // namespace PokeVault::Integration::Gen5
#endif
