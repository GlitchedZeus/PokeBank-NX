// L2 Phase A differential oracle host (not a Nintendo Switch binary).
// Runs UNMODIFIED original PKHeX.Core C# API via ProjectReference.
// The origin pin and GPL-3.0-or-later source notices are maintained by
// the upstream checkout. No PK5 / PK6 decoder is written here.
using PKHeX.Core;
for (var gen = 5; gen <= 6; ++gen)
{
    var maxSpecies = gen == 5 ? 649 : 721;
    for (ushort species = 1; species <= maxSpecies; ++species)
    for (byte form = 0; form <= 6; ++form)
    {
        var permission = gen == 5
            ? new EncounterEgg5(species, form, GameVersion.B).Ability
            : new EncounterEgg6(species, form, GameVersion.X).Ability;
        var hatch = Breeding.CanHatchAsEgg(species);
        var hidden = permission == AbilityPermission.Any12H;
        var split = Breeding.IsSplitBreedNotBabySpecies(species, (byte)gen);
        var gender = Breeding.IsGenderSpeciesDetermination(species);
        Console.WriteLine($"{gen},{species},{form},{(hatch ? 1 : 0)},{(hidden ? 1 : 0)},{(split ? 1 : 0)},{(gender ? 1 : 0)}");
    }
}
