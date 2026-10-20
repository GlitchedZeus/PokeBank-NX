#ifndef POKEBANK_UI_ABILITY_EFFECTS_H
#define POKEBANK_UI_ABILITY_EFFECTS_H

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

// NEW-13 (Round 3): the Ability picker shows each ability's NAME and a short effect instead of
// "Slot 1 / Slot 2". The effect text is a one-line summary of the in-battle (or field) effect in
// the Generation III-V games this app edits, indexed by the native ability id (the same ids as
// Trainer::getAbilityName / vendor/PKSM-Core/strings/eng/abilities.txt; tests/test_ability_effects
// checks the names line up). Where an ability changed in Generation V the earlier text is kept
// for Generation III-IV (Sturdy, Lightning Rod, Storm Drain, Stench, Rough Skin, Plus / Minus).
// Only abilities whose effect works against their holder are flagged (drawback -> red); nothing
// is painted green, so colour only appears where it means something.
namespace UI::AbilityEffects {

struct Entry {
    std::string_view name;     // must equal Trainer::getAbilityName(id)
    std::string_view effect;   // Generation V text (and III-IV unless overridden below)
    bool drawback = false;
};

inline constexpr int kLastGen3 = 76;    // Air Lock
inline constexpr int kLastGen4 = 123;   // Bad Dreams
inline constexpr int kLastGen5 = 164;   // Teravolt

inline constexpr std::array<Entry, kLastGen5 + 1> kTable{{
    {"", ""},
    {"Stench", "Attacks may flinch; fewer wild encounters"},
    {"Drizzle", "Summons rain when it enters battle"},
    {"Speed Boost", "Raises Speed at the end of every turn"},
    {"Battle Armor", "Protected from critical hits"},
    {"Sturdy", "Survives one hit from full HP; OHKO moves fail"},
    {"Damp", "Prevents Self-Destruct and Explosion"},
    {"Limber", "Cannot be paralyzed"},
    {"Sand Veil", "Evasion rises in a sandstorm"},
    {"Static", "Contact may paralyze the attacker"},
    {"Volt Absorb", "Electric moves restore its HP instead"},
    {"Water Absorb", "Water moves restore its HP instead"},
    {"Oblivious", "Cannot be infatuated"},
    {"Cloud Nine", "Cancels the effects of weather"},
    {"Compound Eyes", "Raises the accuracy of its moves"},
    {"Insomnia", "Cannot fall asleep"},
    {"Color Change", "Becomes the type of the move that hit it"},
    {"Immunity", "Cannot be poisoned"},
    {"Flash Fire", "Fire moves power up its Fire moves instead"},
    {"Shield Dust", "Blocks the added effects of moves"},
    {"Own Tempo", "Cannot be confused"},
    {"Suction Cups", "Cannot be forced to switch out"},
    {"Intimidate", "Lowers foes' Attack when it enters battle"},
    {"Shadow Tag", "Foes cannot flee or switch out"},
    {"Rough Skin", "Contact attackers lose 1/8 of their HP"},
    {"Wonder Guard", "Only super-effective moves can hit it"},
    {"Levitate", "Immune to Ground-type moves"},
    {"Effect Spore", "Contact may poison, paralyze or sleep attacker"},
    {"Synchronize", "Passes burn, poison or paralysis to the foe"},
    {"Clear Body", "Foes cannot lower its stats"},
    {"Natural Cure", "Status heals when it switches out"},
    {"Lightning Rod", "Draws Electric moves; immune, raises Sp. Atk"},
    {"Serene Grace", "Doubles the chance of moves' added effects"},
    {"Swift Swim", "Speed doubles in rain"},
    {"Chlorophyll", "Speed doubles in harsh sunlight"},
    {"Illuminate", "Raises wild encounter rate when leading"},
    {"Trace", "Copies a foe's Ability when it enters battle"},
    {"Huge Power", "Doubles its Attack"},
    {"Poison Point", "Contact may poison the attacker"},
    {"Inner Focus", "Cannot flinch"},
    {"Magma Armor", "Cannot be frozen"},
    {"Water Veil", "Cannot be burned"},
    {"Magnet Pull", "Steel-type foes cannot flee or switch out"},
    {"Soundproof", "Immune to sound-based moves"},
    {"Rain Dish", "Restores HP in rain"},
    {"Sand Stream", "Summons a sandstorm when it enters battle"},
    {"Pressure", "Moves aimed at it use 1 extra PP"},
    {"Thick Fat", "Halves damage from Fire and Ice moves"},
    {"Early Bird", "Wakes from sleep twice as fast"},
    {"Flame Body", "Contact may burn the attacker"},
    {"Run Away", "Can always flee from wild battles"},
    {"Keen Eye", "Its accuracy cannot be lowered"},
    {"Hyper Cutter", "Its Attack cannot be lowered"},
    {"Pickup", "May pick up an item after battle"},
    {"Truant", "Can only act every other turn", true},
    {"Hustle", "Attack x1.5, physical moves less accurate"},
    {"Cute Charm", "Contact may infatuate the attacker"},
    {"Plus", "Sp. Atk x1.5 with a Plus or Minus ally"},
    {"Minus", "Sp. Atk x1.5 with a Plus or Minus ally"},
    {"Forecast", "Castform changes type with the weather"},
    {"Sticky Hold", "Its held item cannot be taken"},
    {"Shed Skin", "May cure its status at the end of a turn"},
    {"Guts", "Attack x1.5 while it has a status condition"},
    {"Marvel Scale", "Defense x1.5 while it has a status condition"},
    {"Liquid Ooze", "HP-draining moves hurt the user instead"},
    {"Overgrow", "Grass moves x1.5 at 1/3 HP or less"},
    {"Blaze", "Fire moves x1.5 at 1/3 HP or less"},
    {"Torrent", "Water moves x1.5 at 1/3 HP or less"},
    {"Swarm", "Bug moves x1.5 at 1/3 HP or less"},
    {"Rock Head", "Takes no recoil damage"},
    {"Drought", "Summons harsh sunlight when it enters battle"},
    {"Arena Trap", "Grounded foes cannot flee or switch out"},
    {"Vital Spirit", "Cannot fall asleep"},
    {"White Smoke", "Foes cannot lower its stats"},
    {"Pure Power", "Doubles its Attack"},
    {"Shell Armor", "Protected from critical hits"},
    {"Air Lock", "Cancels the effects of weather"},
    {"Tangled Feet", "Evasion rises while it is confused"},
    {"Motor Drive", "Electric moves raise its Speed instead"},
    {"Rivalry", "Stronger vs. same gender, weaker vs. opposite"},
    {"Steadfast", "Speed rises each time it flinches"},
    {"Snow Cloak", "Evasion rises in hail"},
    {"Gluttony", "Eats pinch Berries at 1/2 HP instead of 1/4"},
    {"Anger Point", "Maxes Attack after taking a critical hit"},
    {"Unburden", "Speed doubles once its held item is used"},
    {"Heatproof", "Halves damage from Fire moves and burns"},
    {"Simple", "Its stat changes are doubled"},
    {"Dry Skin", "Water heals it; Fire and sunlight hurt more"},
    {"Download", "Raises Attack or Sp. Atk when it enters battle"},
    {"Iron Fist", "Punching moves x1.2"},
    {"Poison Heal", "Restores HP each turn while poisoned"},
    {"Adaptability", "Same-type moves x2 instead of x1.5"},
    {"Skill Link", "Multi-strike moves always hit 5 times"},
    {"Hydration", "Cures its status condition in rain"},
    {"Solar Power", "Sp. Atk x1.5 in sunlight, but loses HP"},
    {"Quick Feet", "Speed x1.5 while it has a status condition"},
    {"Normalize", "All of its moves become Normal type"},
    {"Sniper", "Boosts the power of its critical hits"},
    {"Magic Guard", "Only takes damage from attacks"},
    {"No Guard", "Moves used by or against it always hit"},
    {"Stall", "Always moves last in its priority bracket", true},
    {"Technician", "Moves of 60 power or less x1.5"},
    {"Leaf Guard", "Cannot get a status condition in sunlight"},
    {"Klutz", "Cannot use its held item", true},
    {"Mold Breaker", "Its moves ignore foes' Abilities"},
    {"Super Luck", "Raises its critical-hit ratio"},
    {"Aftermath", "Damages the attacker that KOs it by contact"},
    {"Anticipation", "Senses a foe's super-effective or OHKO move"},
    {"Forewarn", "Reveals a foe's strongest move on entry"},
    {"Unaware", "Ignores foes' stat changes"},
    {"Tinted Lens", "Not-very-effective moves deal double damage"},
    {"Filter", "Super-effective damage taken x0.75"},
    {"Slow Start", "Attack and Speed halved for its first 5 turns", true},
    {"Scrappy", "Normal and Fighting moves hit Ghost types"},
    {"Storm Drain", "Draws Water moves; immune, raises Sp. Atk"},
    {"Ice Body", "Restores HP in hail"},
    {"Solid Rock", "Super-effective damage taken x0.75"},
    {"Snow Warning", "Summons hail when it enters battle"},
    {"Honey Gather", "May find Honey after battle"},
    {"Frisk", "Reveals a foe's held item on entry"},
    {"Reckless", "Recoil moves x1.2"},
    {"Multitype", "Arceus's type follows its held Plate"},
    {"Flower Gift", "Boosts its side's Attack and Sp. Def in sun"},
    {"Bad Dreams", "Sleeping foes lose HP every turn"},
    {"Pickpocket", "Steals the held item of a contact attacker"},
    {"Sheer Force", "Moves lose added effects for x1.3 power"},
    {"Contrary", "Its stat changes are reversed"},
    {"Unnerve", "Foes cannot eat Berries"},
    {"Defiant", "Attack sharply rises when a foe lowers a stat"},
    {"Defeatist", "Attack and Sp. Atk halved at 1/2 HP or less", true},
    {"Cursed Body", "May disable a move that hits it"},
    {"Healer", "May cure an adjacent ally's status"},
    {"Friend Guard", "Reduces damage dealt to its allies"},
    {"Weak Armor", "Physical hits lower Defense, raise Speed"},
    {"Heavy Metal", "Doubles its weight"},
    {"Light Metal", "Halves its weight"},
    {"Multiscale", "Halves damage taken while at full HP"},
    {"Toxic Boost", "Physical moves x1.5 while poisoned"},
    {"Flare Boost", "Special moves x1.5 while burned"},
    {"Harvest", "May restore a Berry it has used"},
    {"Telepathy", "Avoids damaging moves from its allies"},
    {"Moody", "Each turn one stat sharply rises, another falls"},
    {"Overcoat", "Takes no damage from sandstorm or hail"},
    {"Poison Touch", "Its contact moves may poison the target"},
    {"Regenerator", "Restores 1/3 of its HP when it switches out"},
    {"Big Pecks", "Its Defense cannot be lowered"},
    {"Sand Rush", "Speed doubles in a sandstorm"},
    {"Wonder Skin", "Status moves are more likely to miss it"},
    {"Analytic", "Moves x1.3 when it moves last"},
    {"Illusion", "Appears as the last Pokemon in its party"},
    {"Imposter", "Transforms into the foe when it enters battle"},
    {"Infiltrator", "Its moves bypass the foe's screens"},
    {"Mummy", "Contact turns the attacker's Ability to Mummy"},
    {"Moxie", "Attack rises each time it knocks out a foe"},
    {"Justified", "Attack rises when hit by a Dark move"},
    {"Rattled", "Speed rises when hit by Bug, Ghost or Dark"},
    {"Magic Bounce", "Reflects status moves back at the user"},
    {"Sap Sipper", "Grass moves raise its Attack instead"},
    {"Prankster", "Its status moves gain +1 priority"},
    {"Sand Force", "Rock, Ground, Steel moves x1.3 in sandstorm"},
    {"Iron Barbs", "Contact attackers lose 1/8 of their HP"},
    {"Zen Mode", "Darmanitan changes form at 1/2 HP or less"},
    {"Victory Star", "Raises the accuracy of it and its allies"},
    {"Turboblaze", "Its moves ignore foes' Abilities"},
    {"Teravolt", "Its moves ignore foes' Abilities"},
}};

// The first generation (3..5) the ability exists in; 0 for an unknown id.
constexpr int introducedIn(uint16_t ability) noexcept {
    return ability == 0 || ability > kLastGen5 ? 0 : ability <= kLastGen3 ? 3 : ability <= kLastGen4 ? 4 : 5;
}

// Short effect for `ability` in Generation `generation` (3..5); empty when the ability does not
// exist in that generation.
constexpr std::string_view effect(uint16_t ability, int generation) noexcept {
    const int since = introducedIn(ability);
    if (since == 0 || generation < since || generation > 5) return {};
    if (generation <= 4) {
        // Generation V changed these; Generation III-IV keep the earlier effect.
        switch (ability) {
            case 1: return "No effect in Generation III-IV battles";              // Stench
            case 5: return "One-hit KO moves fail against it";                    // Sturdy
            case 24: return "Contact attackers lose 1/16 of their HP";             // Rough Skin
            case 31: return "Draws single-target Electric moves to itself";        // Lightning Rod
            case 57: return "Sp. Atk x1.5 with a Minus ally";                     // Plus
            case 58: return "Sp. Atk x1.5 with a Plus ally";                      // Minus
            case 114: return "Draws single-target Water moves to itself";          // Storm Drain
            default: break;
        }
    }
    return kTable[ability].effect;
}

constexpr bool drawback(uint16_t ability) noexcept {
    return ability > 0 && ability <= kLastGen5 && kTable[ability].drawback;
}

// The picker row: "<name>  |  <effect>" (the shared value list draws the effect on the right,
// as Met Location draws method / level). `name` may carry a slot note such as " (Hidden)".
inline constexpr std::string_view kSplit = "  |  ";
inline std::string abilityEffectRow(std::string_view name, uint16_t ability, int generation) {
    std::string out(name);
    const auto text = effect(ability, generation);
    if (!text.empty()) out.append(kSplit).append(text);
    return out;
}
// The name part of a row (review lines and status text never carry the effect).
inline std::string nameOf(const std::string& row) {
    const auto at = row.find(kSplit);
    return at == std::string::npos ? row : row.substr(0, at);
}
// True when a row's effect text is a drawback (drawn red).
constexpr bool isDrawbackEffect(std::string_view text) noexcept {
    if (text.empty()) return false;
    for (const auto& e : kTable)
        if (e.drawback && e.effect == text) return true;
    return false;
}

} // namespace UI::AbilityEffects

#endif
