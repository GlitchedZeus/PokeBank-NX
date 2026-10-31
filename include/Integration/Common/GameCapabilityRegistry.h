#ifndef POKEBANK_COMMON_GAME_CAPABILITY_REGISTRY_H
#define POKEBANK_COMMON_GAME_CAPABILITY_REGISTRY_H

// One registry for every PokeBank NX title (Gen I-IX): which shared application workflows each
// exact game supports today, which native mechanics the game actually has, and WHY a workflow is
// unavailable where it is. The shared UI, the Save Instances flow and the cross-generation
// regression matrix all read this table; nothing else may advertise a capability.
//
// Rules (tests/test_game_capability_registry.cpp enforces them):
//   * Cumulative inheritance: a shared workflow that an earlier generation supports must be at
//     least ReadOnly or Blocked-with-a-named-reason for every later title where it applies.
//     NotImplemented is never acceptable for an inherited workflow.
//   * Mechanics are per title: Z-Move data belongs to SM/USUM only, Dynamax to SWSH only, etc.
//   * A Blocked status always names its prerequisite; a NotApplicable status names the native
//     reason the game lacks the capability.
//   * "Supported" means reachable through the shared application workflow and covered by host
//     tests. It is NEVER a hardware-acceptance claim: see HardwareState and UiIntegration.
//   * No title is OwnerAcceptedExactBuild unless hardwareEvidence names the exact NRO the owner
//     accepted. Owner results on earlier candidates are OwnerTestedEarlierCandidate, not acceptance.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace PokeVault::Integration::Capability {

enum class Workflow : uint8_t {
    Dashboard,        // Home card: artwork, identity, provider label, current Party overview
    SaveInstances,    // profile assignment, discovery, validation, newest/history ordering
    Party,
    Boxes,
    PokemonEditor,    // shared View/Edit workspace with provider-driven field states
    NativeMechanics,  // the title's own mechanics shown/edited inside the shared editor
    Trainer,
    Bag,
    Pokedex,
    Create,
    Clone,
    Move,
    Remove,
    Backup,           // historical backup browser / immutable original
    Review,
    Export,           // separately verified Export Copy
    Count
};
inline constexpr size_t WorkflowCount = static_cast<size_t>(Workflow::Count);

enum class Support : uint8_t {
    Supported,      // implemented through the shared workflow, host-tested
    ReadOnly,       // visible through the shared workflow; editing not verified
    Blocked,        // deliberately unavailable; `reason` names the prerequisite
    NotApplicable,  // the native game has no such mechanic/data; `reason` says so
    NotImplemented, // nothing exists yet (only legal where no earlier generation supports it)
};

struct WorkflowStatus {
    Support support = Support::NotImplemented;
    std::string_view reason{};
};

enum class HardwareState : uint8_t {
    NotDeviceTested,              // no owner result on any build
    OwnerTestedEarlierCandidate,  // owner rounds ran on earlier candidates; no exact-build acceptance
    OwnerAcceptedExactBuild,      // requires hardwareEvidence naming the exact NRO SHA-256
};

// How far the title's application path is actually wired, independent of backend support.
enum class UiIntegration : uint8_t {
    LegacyReaderOnly,          // installed-title reader through the pre-shared generic screens
    ClassicSharedUi,           // Gen I-V shared Trainer/Party/Boxes/editor surfaces (Round 10 source)
    ModernSurfaceHostTested,   // ModernSharedSurface.inc over a NativeSession, driven by the real
                               // TrainerViewScreen in a host-UI harness test (not on hardware)
};

enum class Platform : uint8_t { GameBoy, GameBoyColor, GameBoyAdvance, NintendoDS, Nintendo3DS, NintendoSwitch };

enum class StatModel : uint8_t {
    DVStatExp,       // Gen I-II
    IVEV,            // Gen III-IX ordinary
    IVAwakening,     // Let's Go: IVs + Awakening Values, no EVs
    IVEffortLevel,   // Legends: Arceus: IVs + Effort Levels, no EVs
};

struct StorageGeometry {
    uint8_t boxCount = 0;
    uint8_t slotsPerBox = 0;
    uint8_t partySlots = 6;
    std::string_view model{};  // e.g. "SparseNative", "PointerBacked", "Pastures"
};

// Native mechanics a title actually has. Only the shared editor rows for these mechanics are
// offered on that title; the editor itself is shared by every generation.
struct NativeMechanics {
    bool megaForms = false;        // Gen VI+: Mega Evolution forms (held item / key item driven)
    bool fairyType = false;        // Gen VI+
    bool superTraining = false;    // Gen VI
    bool pokemonAmie = false;      // Gen VI (Refresh in VII)
    bool memories = false;         // Gen VI-VII native memory blocks, Gen VIII+ partial
    bool geolocation = false;      // Gen VI-VII
    bool zMoves = false;           // SM/USUM
    bool alolanForms = false;      // SM/USUM (+ LGPE)
    bool hyperTraining = false;    // Gen VII+ (Bottle Caps)
    bool awakeningValues = false;  // LGPE
    bool partnerRestrictions = false; // LGPE starter partner
    bool goOrigin = false;         // LGPE / HOME GO transfers
    bool dynamax = false;          // SWSH
    bool gigantamax = false;       // SWSH
    bool technicalRecords = false; // SWSH TRs
    bool battleTeamLocks = false;  // SWSH
    bool effortLevels = false;     // PLA
    bool alphaPokemon = false;     // PLA, Z-A
    bool moveMastery = false;      // PLA
    bool pastures = false;         // PLA storage vocabulary
    bool teraType = false;         // SV
    bool mintedNature = false;     // Gen VIII+: Nature vs Stat Nature
    bool marks = false;            // Gen VIII+ marks (ribbons extended)
    bool ribbons = false;          // Gen III+ (not Gen V/IX in some tables; see per title)
    bool plusFlags = false;        // Legends: Z-A
    bool scale = false;            // Gen VIII+ height/weight/scale
};

struct GameCapabilities {
    std::string_view id;           // exact game id, must match Games::GameDescriptor::id
    std::string_view title;
    uint8_t generation = 0;
    Platform platform = Platform::GameBoy;
    std::string_view family;       // e.g. "XY", "ORAS", "SM", "USUM", "LGPE"
    std::string_view recordFormat; // e.g. "PK6"
    StatModel statModel = StatModel::IVEV;
    StorageGeometry storage{};
    NativeMechanics mechanics{};
    std::array<WorkflowStatus, WorkflowCount> workflows{};
    HardwareState hardware = HardwareState::NotDeviceTested;
    std::string_view exportRoot{}; // app-owned export folder stem (empty when export is blocked)
    std::string_view provider{};   // "modern-native" when the shared Modern adapter serves it
    UiIntegration ui = UiIntegration::LegacyReaderOnly;
    std::string_view uiEvidence{};       // host-UI test(s) that drive the real screen for this title
    std::string_view hardwareEvidence{}; // exact NRO SHA-256 + owner record; empty unless accepted

    [[nodiscard]] constexpr const WorkflowStatus& status(Workflow w) const noexcept {
        return workflows[static_cast<size_t>(w)];
    }
    [[nodiscard]] constexpr bool supports(Workflow w) const noexcept {
        return status(w).support == Support::Supported;
    }
    [[nodiscard]] constexpr bool visible(Workflow w) const noexcept {
        const auto s = status(w).support;
        return s == Support::Supported || s == Support::ReadOnly;
    }
};

[[nodiscard]] constexpr std::string_view workflowName(Workflow w) noexcept {
    switch (w) {
        case Workflow::Dashboard: return "Dashboard/Save Instances";
        case Workflow::SaveInstances: return "Save Instances";
        case Workflow::Party: return "Party";
        case Workflow::Boxes: return "Boxes";
        case Workflow::PokemonEditor: return "Pokemon editor";
        case Workflow::NativeMechanics: return "Native mechanics";
        case Workflow::Trainer: return "Trainer";
        case Workflow::Bag: return "Bag";
        case Workflow::Pokedex: return "Pokedex";
        case Workflow::Create: return "Create";
        case Workflow::Clone: return "Clone";
        case Workflow::Move: return "Move/Swap";
        case Workflow::Remove: return "Remove";
        case Workflow::Backup: return "Backup";
        case Workflow::Review: return "Review";
        case Workflow::Export: return "Export and safety";
        case Workflow::Count: break;
    }
    return "?";
}

[[nodiscard]] constexpr std::string_view supportName(Support s) noexcept {
    switch (s) {
        case Support::Supported: return "PASS";
        case Support::ReadOnly: return "READ-ONLY";
        case Support::Blocked: return "BLOCKED";
        case Support::NotApplicable: return "NOT APPLICABLE";
        case Support::NotImplemented: return "NOT IMPLEMENTED";
    }
    return "?";
}

namespace Detail {
constexpr WorkflowStatus ok() noexcept { return {Support::Supported, {}}; }
constexpr WorkflowStatus ro(std::string_view why) noexcept { return {Support::ReadOnly, why}; }
constexpr WorkflowStatus blocked(std::string_view why) noexcept { return {Support::Blocked, why}; }
constexpr WorkflowStatus na(std::string_view why) noexcept { return {Support::NotApplicable, why}; }
constexpr WorkflowStatus none() noexcept { return {Support::NotImplemented, {}}; }

// The Gen I-V baseline: every shared workflow exists and is host-tested on the Round 10 source
// (fix/gen4-bag-remove-compaction 3634d1e6). Pokedex editing is read-only everywhere.
constexpr std::array<WorkflowStatus, WorkflowCount> classic(bool gen1) noexcept {
    return {{
        ok(), ok(), ok(), ok(), ok(), ok(),
        ok(),  // Trainer
        gen1 ? ok() : ok(),
        ro("Pokedex progress is shown; editing is not independently verified"),
        ok(), ok(), ok(), ok(), ok(), ok(), ok(),
    }};
}

// Gen VI X/Y/ORAS through the shared Modern adapter (Integration/Gen6 + Integration/Modern).
constexpr std::array<WorkflowStatus, WorkflowCount> gen6() noexcept {
    return {{
        ok(), ok(), ok(), ok(), ok(),
        ro("Mega/form, Super Training and Pokemon-Amie bytes are shown read-only; no verified writer"),
        ro("MyStatus trainer fields are read-only until a byte-mask verifier exists for block 17"),
        ok(),
        ro("Zukan seen/caught counts are read-only"),
        blocked("Create needs verifiable EC/PID, personal, met, handler, memory and geolocation defaults (Gen6StagedSession::createCapability)"),
        ok(),
        ok(),
        ok(),
        ok(),
        ok(),
        ok(),
    }};
}

// Gen VII Sun/Moon/Ultra through the same adapter; export stays off until SAV7 re-signing is proven.
constexpr std::array<WorkflowStatus, WorkflowCount> gen7sm() noexcept {
    return {{
        ok(), ok(), ok(), ok(), ok(),
        ro("Z-Crystal, Alolan form and Hyper Training bytes are shown read-only; no verified writer"),
        ro("MyStatus trainer fields are read-only"),
        ok(),
        ro("Zukan seen/caught counts are read-only"),
        blocked("Create is unsupported for the same reasons as Gen VI, plus SAV7 export is blocked"),
        ok(),
        ok(),
        ok(),
        ok(),
        ok(),
        blocked("SAV7 MemeCrypto re-signing has no retail acceptance evidence (docs/gen7/README.md); staged changes stay in app memory"),
    }};
}

// Let's Go, SWSH, BDSP, PLA, SV, Z-A: the installed-title readers exist (Trainer7LGPE,
// Trainer8SWSH, ...) but they present through the pre-shared generic screens and have no
// verified staged session or export verifier. Every editing workflow is therefore Blocked with the
// named prerequisite; nothing is advertised that the shared editor cannot do safely.
constexpr std::array<WorkflowStatus, WorkflowCount> switchReader(std::string_view adapter) noexcept {
    return {{
        ok(),  // Dashboard: installed-title card via the account save mount
        ro("Installed save is console-managed; manual/Checkpoint copies need the strict parser of the adapter"),
        ro("Shown by the legacy modern reader; staged editing needs the shared Modern adapter"),
        ro("Shown by the legacy modern reader; staged editing needs the shared Modern adapter"),
        blocked(adapter),
        blocked(adapter),
        ro("Trainer facts shown read-only"),
        ro("Pockets shown read-only; no verified staged bag"),
        ro("Pokedex counts shown read-only"),
        blocked(adapter), blocked(adapter), blocked(adapter), blocked(adapter),
        ro("Immutable Switch historical backups are read-only previews"),
        blocked(adapter),
        blocked("Installed-game injection is hard-disabled; a verified Export Copy needs the adapter's strict re-parse and byte-diff verifier"),
    }};
}

// Sword/Shield through the shared Modern adapter (Integration/Gen8: Gen8StagedSession + Gen8NativeSession).
// The installed-title reader (Trainer8SWSH) still serves the console-mounted save read-only; an
// assigned `main` file opens in the shared surface with staged editing and a verified Export Copy.
constexpr std::array<WorkflowStatus, WorkflowCount> gen8swsh() noexcept {
    return {{
        ok(),  // Dashboard: installed-title card and the assigned-file card
        ok(),  // Save Instances: manual link with strict SCBlock validation + fingerprint revalidation
        ok(), ok(), ok(),
        ro("Dynamax Level, Gigantamax, mint (stat) nature and battle-team membership are shown read-only; no verified writer"),
        ro("MyStatus trainer facts are read-only"),
        ro("Installed-title reader shows pockets read-only; staged Bag needs a verified MyItem (0x1177C2C4) pouch decoder"),
        ro("Installed-title reader shows counts read-only; the shared adapter has no verified Zukan8 decoder"),
        blocked("Create needs verifiable EC/PID, personal, met, handler, memory and cached-stat defaults (Gen8StagedSession::createUnsupportedReason)"),
        ok(),  // Clone: box records into verified blank, team-free box slots
        ok(),  // Move: box-to-box, team-referenced records refused
        ok(),  // Remove: team-free box records
        ro("Immutable Switch historical backups are read-only previews"),
        ok(),
        ok(),  // Export Copy: canonical SwishCrypto re-encryption + block-level verifier, exports/gen8
    }};
}

constexpr std::string_view kLgpeAdapter =
    "LGPE needs a PB7 staged session over the pointer-backed 1000-slot storage with starter/party pointer repair";
constexpr std::string_view kBdspAdapter =
    "BDSP needs the paired-file authority protocol (which of the two save files is current) before any staged session";
constexpr std::string_view kPlaAdapter =
    "PLA needs a PA8 staged session over the SCBlock container with Effort Level, mastery and pasture geometry verification";
constexpr std::string_view kSvAdapter =
    "SV needs a PK9 staged session over the SCBlock container with revision/DLC block verification and Tera/Minted-Nature masks";
constexpr std::string_view kZaAdapter =
    "Z-A needs a separate PA9/SAV9ZA staged session (0x198 box stride, Plus flags, footer/presence updates); it is not SV";

constexpr NativeMechanics mech6() noexcept {
    NativeMechanics m;
    m.megaForms = m.fairyType = m.superTraining = m.pokemonAmie = m.memories = m.geolocation = m.ribbons = true;
    return m;
}
constexpr NativeMechanics mech7() noexcept {
    NativeMechanics m;
    m.megaForms = m.fairyType = m.pokemonAmie = m.memories = m.geolocation = m.ribbons = true;
    m.zMoves = m.alolanForms = m.hyperTraining = true;
    return m;
}
constexpr NativeMechanics mech7b() noexcept {
    NativeMechanics m;
    m.fairyType = m.alolanForms = m.hyperTraining = m.awakeningValues = m.partnerRestrictions = m.goOrigin = true;
    m.megaForms = true;  // Mega Evolution exists in Let's Go (Mega Stones as Key Items)
    return m;
}
constexpr NativeMechanics mech8swsh() noexcept {
    NativeMechanics m;
    m.fairyType = m.hyperTraining = m.dynamax = m.gigantamax = m.technicalRecords = m.battleTeamLocks = true;
    m.mintedNature = m.marks = m.ribbons = m.scale = m.memories = true;
    return m;
}
constexpr NativeMechanics mech8bdsp() noexcept {
    NativeMechanics m;
    m.fairyType = m.hyperTraining = m.mintedNature = m.ribbons = m.scale = true;
    return m;
}
constexpr NativeMechanics mech8pla() noexcept {
    NativeMechanics m;
    m.fairyType = m.hyperTraining = m.effortLevels = m.alphaPokemon = m.moveMastery = m.pastures = true;
    m.mintedNature = m.scale = m.ribbons = true;
    return m;
}
constexpr NativeMechanics mech9sv() noexcept {
    NativeMechanics m;
    m.fairyType = m.hyperTraining = m.teraType = m.mintedNature = m.marks = m.ribbons = m.scale = m.memories = true;
    return m;
}
constexpr NativeMechanics mech9za() noexcept {
    NativeMechanics m;
    m.fairyType = m.hyperTraining = m.alphaPokemon = m.plusFlags = m.mintedNature = m.scale = m.megaForms = true;
    return m;
}
constexpr NativeMechanics mech1() noexcept { return {}; }
constexpr NativeMechanics mech3() noexcept { NativeMechanics m; m.ribbons = true; return m; }
constexpr NativeMechanics mech5() noexcept { NativeMechanics m; m.ribbons = true; return m; }

constexpr GameCapabilities classicRow(std::string_view id, std::string_view title, uint8_t gen, Platform p,
                                      std::string_view family, std::string_view fmt, StatModel stat,
                                      StorageGeometry geo, NativeMechanics mech) noexcept {
    // Gen I-V: owner hardware rounds ran on earlier candidates (docs/hardware/ROUND*_RESULTS_*),
    // but issue #313 (all-20-release acceptance) is open and no exact build is accepted.
    return {id, title, gen, p, family, fmt, stat, geo, mech, classic(gen == 1),
            HardwareState::OwnerTestedEarlierCandidate, "exports", "classic",
            UiIntegration::ClassicSharedUi, "Round 10 host-UI harness suite (tests/host-ui)", {}};
}
constexpr GameCapabilities gen6Row(std::string_view id, std::string_view title, std::string_view family) noexcept {
    return {id, title, 6, Platform::Nintendo3DS, family, "PK6", StatModel::IVEV,
            {31, 30, 6, "SparseNative"}, mech6(), gen6(), HardwareState::NotDeviceTested, "exports/gen6", "modern-native",
            UiIntegration::ModernSurfaceHostTested, "test_ui_modern_surface_host", {}};
}
constexpr GameCapabilities gen7Row(std::string_view id, std::string_view title, std::string_view family) noexcept {
    return {id, title, 7, Platform::Nintendo3DS, family, "PK7", StatModel::IVEV,
            {32, 30, 6, "SparseNative"}, mech7(), gen7sm(), HardwareState::NotDeviceTested, {}, "modern-native",
            UiIntegration::ModernSurfaceHostTested, "test_ui_modern_surface_gen7_host", {}};
}
constexpr GameCapabilities swshRow(std::string_view id, std::string_view title) noexcept {
    return {id, title, 8, Platform::NintendoSwitch, "SWSH", "PK8", StatModel::IVEV, {32, 30, 6, "SCBlock"},
            mech8swsh(), gen8swsh(), HardwareState::NotDeviceTested, "exports/gen8", "modern-native",
            UiIntegration::ModernSurfaceHostTested, "test_ui_modern_surface_gen8_host", {}};
}
constexpr GameCapabilities switchRow(std::string_view id, std::string_view title, uint8_t gen, std::string_view family,
                                     std::string_view fmt, StatModel stat, StorageGeometry geo, NativeMechanics mech,
                                     std::string_view adapter) noexcept {
    return {id, title, gen, Platform::NintendoSwitch, family, fmt, stat, geo, mech, switchReader(adapter),
            HardwareState::NotDeviceTested, {}, "legacy-modern-reader", UiIntegration::LegacyReaderOnly, {}, {}};
}
} // namespace Detail

inline constexpr std::array<GameCapabilities, 38> Registry = {{
    // ---- Gen I-V baseline (Round 10 source) ----
    Detail::classicRow("red_gb", "Red", 1, Platform::GameBoy, "RBY", "PK1", StatModel::DVStatExp, {12, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("blue_gb", "Blue", 1, Platform::GameBoy, "RBY", "PK1", StatModel::DVStatExp, {12, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("yellow_gb", "Yellow", 1, Platform::GameBoy, "RBY", "PK1", StatModel::DVStatExp, {12, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("gold_gbc", "Gold", 2, Platform::GameBoyColor, "GS", "PK2", StatModel::DVStatExp, {14, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("silver_gbc", "Silver", 2, Platform::GameBoyColor, "GS", "PK2", StatModel::DVStatExp, {14, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("crystal_gbc", "Crystal", 2, Platform::GameBoyColor, "Crystal", "PK2", StatModel::DVStatExp, {14, 20, 6, "PackedNative"}, Detail::mech1()),
    Detail::classicRow("ruby_gba", "Ruby", 3, Platform::GameBoyAdvance, "RS", "PK3", StatModel::IVEV, {14, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("sapphire_gba", "Sapphire", 3, Platform::GameBoyAdvance, "RS", "PK3", StatModel::IVEV, {14, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("emerald_gba", "Emerald", 3, Platform::GameBoyAdvance, "E", "PK3", StatModel::IVEV, {14, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("firered_gba", "FireRed", 3, Platform::GameBoyAdvance, "FRLG", "PK3", StatModel::IVEV, {14, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("leafgreen_gba", "LeafGreen", 3, Platform::GameBoyAdvance, "FRLG", "PK3", StatModel::IVEV, {14, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("diamond_nds", "Diamond", 4, Platform::NintendoDS, "DP", "PK4", StatModel::IVEV, {18, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("pearl_nds", "Pearl", 4, Platform::NintendoDS, "DP", "PK4", StatModel::IVEV, {18, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("platinum_nds", "Platinum", 4, Platform::NintendoDS, "Pt", "PK4", StatModel::IVEV, {18, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("heartgold_nds", "HeartGold", 4, Platform::NintendoDS, "HGSS", "PK4", StatModel::IVEV, {18, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("soulsilver_nds", "SoulSilver", 4, Platform::NintendoDS, "HGSS", "PK4", StatModel::IVEV, {18, 30, 6, "SparseNative"}, Detail::mech3()),
    Detail::classicRow("black_nds", "Black", 5, Platform::NintendoDS, "BW", "PK5", StatModel::IVEV, {24, 30, 6, "SparseNative"}, Detail::mech5()),
    Detail::classicRow("white_nds", "White", 5, Platform::NintendoDS, "BW", "PK5", StatModel::IVEV, {24, 30, 6, "SparseNative"}, Detail::mech5()),
    Detail::classicRow("black2_nds", "Black 2", 5, Platform::NintendoDS, "B2W2", "PK5", StatModel::IVEV, {24, 30, 6, "SparseNative"}, Detail::mech5()),
    Detail::classicRow("white2_nds", "White 2", 5, Platform::NintendoDS, "B2W2", "PK5", StatModel::IVEV, {24, 30, 6, "SparseNative"}, Detail::mech5()),
    // ---- Gen VI ----
    Detail::gen6Row("x_3ds", "X", "XY"),
    Detail::gen6Row("y_3ds", "Y", "XY"),
    Detail::gen6Row("omegaruby_3ds", "Omega Ruby", "ORAS"),
    Detail::gen6Row("alphasapphire_3ds", "Alpha Sapphire", "ORAS"),
    // ---- Gen VII ----
    Detail::gen7Row("sun_3ds", "Sun", "SM"),
    Detail::gen7Row("moon_3ds", "Moon", "SM"),
    Detail::gen7Row("ultrasun_3ds", "Ultra Sun", "USUM"),
    Detail::gen7Row("ultramoon_3ds", "Ultra Moon", "USUM"),
    Detail::switchRow("letsgo_pikachu_switch", "Let's Go Pikachu", 7, "LGPE", "PB7", StatModel::IVAwakening, {40, 25, 6, "PointerBacked"}, Detail::mech7b(), Detail::kLgpeAdapter),
    Detail::switchRow("letsgo_eevee_switch", "Let's Go Eevee", 7, "LGPE", "PB7", StatModel::IVAwakening, {40, 25, 6, "PointerBacked"}, Detail::mech7b(), Detail::kLgpeAdapter),
    // ---- Gen VIII ----
    Detail::swshRow("sword_switch", "Sword"),
    Detail::swshRow("shield_switch", "Shield"),
    Detail::switchRow("brilliant_diamond_switch", "Brilliant Diamond", 8, "BDSP", "PB8", StatModel::IVEV, {40, 30, 6, "FlatPairedFile"}, Detail::mech8bdsp(), Detail::kBdspAdapter),
    Detail::switchRow("shining_pearl_switch", "Shining Pearl", 8, "BDSP", "PB8", StatModel::IVEV, {40, 30, 6, "FlatPairedFile"}, Detail::mech8bdsp(), Detail::kBdspAdapter),
    Detail::switchRow("legends_arceus_switch", "Legends Arceus", 8, "PLA", "PA8", StatModel::IVEffortLevel, {32, 30, 6, "Pastures"}, Detail::mech8pla(), Detail::kPlaAdapter),
    // ---- Gen IX ----
    Detail::switchRow("scarlet_switch", "Scarlet", 9, "SV", "PK9", StatModel::IVEV, {32, 30, 6, "SCBlock"}, Detail::mech9sv(), Detail::kSvAdapter),
    Detail::switchRow("violet_switch", "Violet", 9, "SV", "PK9", StatModel::IVEV, {32, 30, 6, "SCBlock"}, Detail::mech9sv(), Detail::kSvAdapter),
    Detail::switchRow("legends_za_switch", "Legends Z-A", 9, "ZA", "PA9", StatModel::IVEV, {32, 30, 6, "SCBlock"}, Detail::mech9za(), Detail::kZaAdapter),
}};

[[nodiscard]] constexpr std::span<const GameCapabilities> all() noexcept { return Registry; }

[[nodiscard]] constexpr const GameCapabilities* find(std::string_view id) noexcept {
    for (const auto& row : Registry)
        if (row.id == id) return &row;
    return nullptr;
}

[[nodiscard]] constexpr bool isModernNativeId(std::string_view id) noexcept {
    const auto* row = find(id);
    return row && row->provider == "modern-native";
}

// Cumulative inheritance: does any title of an EARLIER generation make this workflow visible?
[[nodiscard]] constexpr bool inheritedWorkflow(Workflow w, uint8_t generation) noexcept {
    for (const auto& row : Registry)
        if (row.generation < generation && row.visible(w)) return true;
    return false;
}

struct InheritanceViolation {
    std::string_view id;
    Workflow workflow;
};

// Every inherited workflow must be Supported, ReadOnly, Blocked (with a reason) or NotApplicable
// (with a reason). The first violation found is returned; nullopt means the registry is coherent.
[[nodiscard]] constexpr std::optional<InheritanceViolation> firstInheritanceViolation() noexcept {
    for (const auto& row : Registry) {
        for (size_t i = 0; i < WorkflowCount; ++i) {
            const auto w = static_cast<Workflow>(i);
            const auto& s = row.status(w);
            if (!inheritedWorkflow(w, row.generation)) continue;
            if (s.support == Support::NotImplemented) return InheritanceViolation{row.id, w};
            if ((s.support == Support::Blocked || s.support == Support::NotApplicable) && s.reason.empty())
                return InheritanceViolation{row.id, w};
        }
    }
    return std::nullopt;
}

// No title may claim exact-build owner acceptance without naming the evidence, and no title on the
// shared Modern surface may claim it while its UI has only host-harness evidence.
[[nodiscard]] constexpr bool hardwareClaimsAreEvidenced() noexcept {
    for (const auto& row : Registry)
        if (row.hardware == HardwareState::OwnerAcceptedExactBuild && row.hardwareEvidence.empty()) return false;
    return true;
}
static_assert(hardwareClaimsAreEvidenced(), "an OwnerAcceptedExactBuild row must name its exact-NRO evidence");

static_assert(!firstInheritanceViolation().has_value(),
              "a later generation dropped an inherited shared workflow without naming why");

} // namespace PokeVault::Integration::Capability

#endif
