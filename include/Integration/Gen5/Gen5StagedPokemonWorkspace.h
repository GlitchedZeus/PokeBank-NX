#ifndef POKEBANK_GEN5_STAGED_WORKSPACE_H
#define POKEBANK_GEN5_STAGED_WORKSPACE_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedPokemonRecord.h"
#include "Integration/Gen5/Gen5NativeCreate.h"
#include "Integration/Gen5/Gen5Bag.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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
    // Native slot identity must be canonical. An unrecognized enum value
    // must never alias a valid box, and party references cannot use a box id.
    [[nodiscard]] static constexpr bool canonicalSlot(const Slot& s) noexcept {
        return s.region==Region::Party ? (s.box==0 && s.slot<6) :
               s.region==Region::Box && s.box<24 && s.slot<30;
    }
    enum class ChangeKind : uint8_t { FieldEdit, CloneBox, RemoveBox, CreateBox };
    struct PendingChange {
        Slot location;
        std::vector<uint8_t> before;
        std::vector<uint8_t> after;
        bool black2White2 = false;
        ChangeKind kind = ChangeKind::FieldEdit;
        Slot cloneSource{};
        std::vector<uint8_t> cloneSourceOriginal;
        std::optional<CreateRequest5> createRequest;
        std::vector<uint8_t> replacementOriginal;
    };

    explicit Gen5StagedPokemonWorkspace(const Gen5ReadOnlySave& validated)
        : source_(validated) {}

    [[nodiscard]] const Gen5ReadOnlySave& sourceSave() const noexcept { return source_; }
    [[nodiscard]] bool black2White2() const noexcept {
        return source_.exactGameId()=="black2_nds" || source_.exactGameId()=="white2_nds";
    }

    [[nodiscard]] bool hasChanges() const noexcept {
        if(!boxReplacements_.empty() || bagDirty())return true;
        for(const auto& pair:pending_)if(pair.second.dirty())return true;
        return false;
    }
    [[nodiscard]] size_t changedRecordCount() const noexcept {
        size_t n=0;
        for(const auto& pair:pending_)if(pair.second.dirty())++n;
        return n+boxReplacements_.size();
    }

    [[nodiscard]] bool stageParty(size_t slot,StagedPokemon5Record::Field field,
                                   size_t stat,uint32_t value,std::string* error=nullptr) {
        return stageSlot({Region::Party,0,slot},field,stat,value,error);
    }
    [[nodiscard]] bool stageBox(size_t box,size_t slot,StagedPokemon5Record::Field field,
                                 size_t stat,uint32_t value,std::string* error=nullptr) {
        return stageSlot({Region::Box,box,slot},field,stat,value,error);
    }

    [[nodiscard]] bool stagePartyName(size_t slot,bool nickname,
                                     std::string_view name,std::string* error=nullptr) {
        return stageNameSlot({Region::Party,0,slot},nickname,name,error);
    }
    [[nodiscard]] bool stageBoxName(size_t box,size_t slot,bool nickname,
                                   std::string_view name,std::string* error=nullptr) {
        return stageNameSlot({Region::Box,box,slot},nickname,name,error);
    }


    // Clone one ORIGINAL validated occupied PK5 into a physically empty box
    // destination. No party-count change, no source-save write, no cross-game
    // conversion, no arbitrary PK5 fabrication. A party source is reduced to
    // the native 136-byte boxed form by decrypt/truncate/re-encrypt.
    [[nodiscard]] bool cloneToEmptyBox(Slot from,size_t box,size_t slot,
                                       std::string* error=nullptr) {
        const Slot to{Region::Box,box,slot};
        const auto fail=[&](const char* why) {
            if(error)*error=why;
            return false;
        };
        if(!canonicalSlot(from) || !canonicalSlot(to) ||
           pending_.count(from) || boxReplacements_.count(from) ||
           pending_.count(to) || boxReplacements_.count(to))
            return fail("Gen V clone requires an unmodified source and destination");
        const auto original=sourceSlot(from),target=sourceSlot(to);
        if(!original || !original->valid() || original->empty() ||
           !target || !target->valid() || !target->empty() ||
           target->originalEncryptedBytes().size()!=Crypto::StoredSize)
            return fail("Gen V clone requires occupied original and truly empty box slot");
        const auto empty=target->originalEncryptedBytes();
        if(!std::all_of(empty.begin(),empty.end(),[](uint8_t v){return v==0;}))
            return fail("Gen V clone refuses nonzero empty-slot payload");
        auto decoded=Crypto::decrypt(original->originalEncryptedBytes());
        if(decoded.size()!=Crypto::StoredSize && decoded.size()!=Crypto::PartySize)
            return fail("Gen V clone encountered a noncanonical PK5 geometry");
        decoded.resize(Crypto::StoredSize);
        const auto encrypted=Crypto::encryptCandidate(decoded);
        const Pokemon5ReadOnly checked(encrypted);
        if(!checked.valid() || checked.empty() ||
           checked.species()!=original->species() ||
           checked.pid()!=original->pid())
            return fail("Gen V native boxed clone failed strict PK5 verification");
        BoxReplacement replacement;
        replacement.kind=ChangeKind::CloneBox;
        replacement.after=encrypted;
        replacement.replacementOriginal=encrypted;
        replacement.cloneSource=from;
        replacement.cloneSourceOriginal.assign(
            original->originalEncryptedBytes().begin(),
            original->originalEncryptedBytes().end());
        boxReplacements_.emplace(to,std::move(replacement));
        if(error)error->clear();
        return true;
    }
    // Backend of the EXISTING shared Create action: app-memory only and
    // strictly to a physically empty boxed PK5 slot. All native attributes
    // are supplied explicitly by the caller and independently validated.
    [[nodiscard]] bool createInEmptyBox(const CreateRequest5& request,
                                        size_t box,size_t slot,
                                        std::string* error=nullptr) {
        const Slot target{Region::Box,box,slot};
        const auto fail=[&](std::string_view why) {
            if(error)*error=std::string(why);
            return false;
        };
        if(!emptyCloneDestination(box,slot))
            return fail("Gen V Create requires a verified unchanged empty box slot");
        const auto game=source_.exactGameId();
        const uint32_t native=game=="white_nds"?20:
            game=="black_nds"?21:
            game=="white2_nds"?22:
            game=="black2_nds"?23:0;
        if(native==0 || static_cast<uint32_t>(request.game)!=native ||
           request.format!=CreateFormat5::Boxed)
            return fail("Gen V Create game and boxed format must match selected SAV5");
        const auto result=createNative5(request);
        if(!result)return fail(result.validation.message);
        const Pokemon5ReadOnly checked(result.encrypted);
        if(!checked.valid() || checked.empty() || checked.partyRecord() ||
           checked.originVersion()!=native || checked.species()!=request.species)
            return fail("Gen V Create failed exact-title boxed PK5 verification");
        BoxReplacement replacement;
        replacement.kind=ChangeKind::CreateBox;
        replacement.after=result.encrypted;
        replacement.replacementOriginal=result.encrypted;
        replacement.createRequest=request;
        if(!boxReplacements_.emplace(target,std::move(replacement)).second)
            return fail("Gen V Create destination changed during staging");
        if(error)error->clear();
        return true;
    }

    // A boxed removal is an app-owned all-zero PK5 replacement only.
    // Party removal and party compaction are explicitly unsupported.
    [[nodiscard]] bool removeBox(size_t box,size_t slot,
                                 std::string* error=nullptr) {
        const Slot location{Region::Box,box,slot};
        const auto fail=[&](const char* why) {
            if(error)*error=why;
            return false;
        };
        if(!canonicalSlot(location) || pending_.count(location) ||
           boxReplacements_.count(location))
            return fail("Gen V boxed removal requires an unmodified target");
        const auto old=sourceSlot(location);
        if(!old || !old->valid() || old->empty() ||
           old->originalEncryptedBytes().size()!=Crypto::StoredSize)
            return fail("Gen V removal requires one occupied valid original box slot");
        BoxReplacement replacement;
        replacement.kind=ChangeKind::RemoveBox;
        replacement.after.assign(Crypto::StoredSize,0);
        boxReplacements_.emplace(location,std::move(replacement));
        if(error)error->clear();
        return true;
    }
    [[nodiscard]] bool undoBoxReplacement(size_t box,size_t slot) noexcept {
        return boxReplacements_.erase({Region::Box,box,slot})==1;
    }
    [[nodiscard]] bool isBoxReplacement(size_t box,size_t slot) const noexcept {
        return boxReplacements_.count({Region::Box,box,slot})!=0;
    }
    [[nodiscard]] bool originalOccupied(Slot location) const {
        if(!canonicalSlot(location) || pending_.count(location) ||
           boxReplacements_.count(location))return false;
        const auto pk=sourceSlot(location);
        return pk && pk->valid() && !pk->empty();
    }
    [[nodiscard]] bool emptyCloneDestination(size_t box,size_t slot) const {
        const Slot location{Region::Box,box,slot};
        if(!canonicalSlot(location) || pending_.count(location) ||
           boxReplacements_.count(location))return false;
        const auto pk=sourceSlot(location);
        if(!pk || !pk->valid() || !pk->empty())return false;
        const auto bytes=pk->originalEncryptedBytes();
        return bytes.size()==Crypto::StoredSize &&
            std::all_of(bytes.begin(),bytes.end(),[](uint8_t byte){return byte==0;});
    }
    [[nodiscard]] size_t firstEmptyCloneDestination() const {
        for(size_t box=0;box<24;++box)
            for(size_t slot=0;slot<30;++slot)
                if(emptyCloneDestination(box,slot))return box*30+slot;
        return 720;
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
                {record.stagedBytes().begin(),record.stagedBytes().end()},
                black2White2(),ChangeKind::FieldEdit,{}, {},std::nullopt,{}});
        }
        for(const auto& [slot,replacement]:boxReplacements_) {
            const auto original=sourceSlot(slot);
            if(!original)continue;
            out.push_back({slot,
                {original->originalEncryptedBytes().begin(),
                 original->originalEncryptedBytes().end()},
                replacement.after,black2White2(),replacement.kind,
                replacement.cloneSource,replacement.cloneSourceOriginal,
                replacement.createRequest,replacement.replacementOriginal});
        }
        return out;
    }
    void discardAll() noexcept { pending_.clear();boxReplacements_.clear();bag_.reset(); }

    // App-owned Items-block copy for native Bag edits. Created on first use from
    // the selected validated copy; nullptr when the bag is not fully recognized.
    [[nodiscard]] Gen5StagedBag* stagedBag(std::string* error=nullptr) {
        if(!bag_)bag_=Gen5StagedBag::create(source_,error);
        return bag_?&*bag_:nullptr;
    }
    [[nodiscard]] const Gen5StagedBag* stagedBagIfOpened() const noexcept {
        return bag_?&*bag_:nullptr;
    }
    [[nodiscard]] bool bagDirty() const noexcept { return bag_ && bag_->dirty(); }
    void discardBag() noexcept { bag_.reset(); }

private:
    struct BoxReplacement {
        ChangeKind kind = ChangeKind::RemoveBox;
        std::vector<uint8_t> after;
        Slot cloneSource{};
        std::vector<uint8_t> cloneSourceOriginal;
        std::optional<CreateRequest5> createRequest;
        std::vector<uint8_t> replacementOriginal;
    };
    [[nodiscard]] std::optional<Pokemon5ReadOnly> sourceSlot(const Slot& s) const {
        if(!canonicalSlot(s))return std::nullopt;
        if(s.region==Region::Party) {
            // Undeclared party slots are not editable.
            if(s.slot>=source_.partyCount() || s.box!=0)return std::nullopt;
            return source_.partyPokemon(s.slot);
        }
        return source_.boxPokemon(s.box,s.slot);
    }
    [[nodiscard]] std::optional<Pokemon5ReadOnly> view(const Slot& s) const {
        if(!canonicalSlot(s))return std::nullopt;
        const auto replacement=boxReplacements_.find(s);
        if(replacement!=boxReplacements_.end())
            return Pokemon5ReadOnly(replacement->second.after);
        const auto it=pending_.find(s);
        if(it!=pending_.end())return it->second.current();
        return sourceSlot(s);
    }
    bool stageSlot(const Slot& location,StagedPokemon5Record::Field field,
                   size_t stat,uint32_t value,std::string* error) {
        if(!canonicalSlot(location)) {
            if(error)*error="Gen V staged slot identity is invalid";
            return false;
        }
        const auto replacement=boxReplacements_.find(location);
        if(replacement!=boxReplacements_.end()) {
            if(replacement->second.kind==ChangeKind::RemoveBox) {
                if(error)*error="Gen V removed slot cannot be edited";
                return false;
            }
            auto edit=StagedPokemon5Record::create(
                replacement->second.after,error,black2White2());
            if(!edit || !edit->stage(field,stat,value,error))return false;
            replacement->second.after.assign(
                edit->stagedBytes().begin(),edit->stagedBytes().end());
            if(error)error->clear();
            return true;
        }
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
        auto edit=StagedPokemon5Record::create(old->originalEncryptedBytes(),error,black2White2());
        if(!edit)return false;
        if(!edit->stage(field,stat,value,error))return false;
        if(edit->dirty())pending_.emplace(location,std::move(*edit));
        if(error)error->clear();
        return true;
    }

    bool stageNameSlot(const Slot& location,bool nickname,
                       std::string_view name,std::string* error) {
        if(!canonicalSlot(location)) {
            if(error)*error="Gen V native name slot identity is invalid";
            return false;
        }
        const auto replacement=boxReplacements_.find(location);
        if(replacement!=boxReplacements_.end()) {
            if(replacement->second.kind==ChangeKind::RemoveBox) {
                if(error)*error="Gen V removed slot cannot be renamed";
                return false;
            }
            auto edit=StagedPokemon5Record::create(
                replacement->second.after,error,black2White2());
            if(!edit || !edit->stageName(nickname,name,error))return false;
            replacement->second.after.assign(
                edit->stagedBytes().begin(),edit->stagedBytes().end());
            if(error)error->clear();
            return true;
        }
        const auto it=pending_.find(location);
        if(it!=pending_.end()) {
            const bool ok=it->second.stageName(nickname,name,error);
            if(ok && !it->second.dirty())pending_.erase(it);
            return ok;
        }
        const auto old=sourceSlot(location);
        if(!old || !old->valid() || old->empty()) {
            if(error)*error="Gen V name target absent, empty or invalid";
            return false;
        }
        auto edit=StagedPokemon5Record::create(
            old->originalEncryptedBytes(),error,black2White2());
        if(!edit || !edit->stageName(nickname,name,error))return false;
        if(edit->dirty())pending_.emplace(location,std::move(*edit));
        if(error)error->clear();
        return true;
    }

    Gen5ReadOnlySave source_;
    std::map<Slot,StagedPokemon5Record> pending_;
    std::map<Slot,BoxReplacement> boxReplacements_;
    std::optional<Gen5StagedBag> bag_;
};

} // namespace PokeVault::Integration::Gen5
#endif
