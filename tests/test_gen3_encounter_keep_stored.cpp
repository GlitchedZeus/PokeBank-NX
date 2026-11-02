// R2-BUG-008: Ruby Pikachu Lv. 5 (Met Lv. 5) was offered only "Safari Zone (Hoenn) - Wild Lv. 25-27".
// The template data is right (Pikachu is wild only in the Hoenn Safari Zone, Lv. 25-27, in R/S/E),
// so a Lv. 5 / Met Lv. 5 Pikachu did not come from that encounter (traded, hatched-and-evolved,
// event ...). The picker presented that template as the only choice and focused it as if it were
// the record's origin. It now offers the stored location first ("keep stored", which changes
// nothing) and labels a template whose range excludes the stored met level.
#include "UI/Gen3EncounterPickerModel.h"

#include <cassert>
#include <iostream>
#include <string>

int main() {
    namespace Picker = PokeBank::UIModel::Gen3EncounterPicker;
    namespace Encounter = PokeVault::Integration::EncounterGuardrails;
    for (const char* game : {"ruby_gba", "sapphire_gba", "emerald_gba"}) {
        const auto choices = Encounter::forGameSpeciesWithGen3Provenance(game, 25);
        assert(choices.size() == 1);
        const auto& safari = choices.front().encounter;
        assert(safari.location == 57 && safari.minLevel == 25 && safari.maxLevel == 27);

        // Stored origin outside the templates (e.g. a trade location), Met Lv. 5.
        const uint16_t storedTrade = 255;
        const auto rows = Picker::rowValuesWithStored(choices, storedTrade);
        assert(rows.size() == 2 && rows[0] == Picker::kKeepStoredRow && rows[1] == 0);
        assert(Picker::selectedRowWithStored(choices, rows, storedTrade, 5) == 0); // focus keeps stored
        assert(Picker::choiceAt(choices, rows[0]) == nullptr);                     // applying changes nothing
        assert(!Picker::matchesStoredMetLevel(safari, 5));                        // labelled, not a match

        // Stored origin IS the Safari template at Met Lv. 26: no keep row, the template is focused.
        const auto safariRows = Picker::rowValuesWithStored(choices, 57);
        assert(safariRows.size() == 1 && safariRows[0] == 0);
        assert(Picker::selectedRowWithStored(choices, safariRows, 57, 26) == 0);
        assert(Picker::matchesStoredMetLevel(safari, 26));

        // No stored location (a Create draft): templates only; a met level of 0 is not judged.
        assert(Picker::rowValuesWithStored(choices, 0).size() == 1);
        assert(Picker::matchesStoredMetLevel(safari, 0));
    }
    std::cout << "Gen III Met Location keeps a stored non-template origin (R2-BUG-008) PASS\n";
    return 0;
}
