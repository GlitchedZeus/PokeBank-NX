// Gen V differential oracle for PokeBank NX.
//
//   dotnet PKHeXGen5Oracle.dll generate SEED MUTATIONS
//
// Builds every Gen V encounter PKHeX knows for species 1-649 in Black, White,
// Black 2 and White 2, converts each to a PK5 with a fixed test trainer, then
// adds MUTATIONS single-field mutations of each. Prints one line per record:
//   <id> <decrypted 136-byte PK5 hex> VALID|INVALID <encounter type> ; <identifier>:<result> ...
// The records are synthetic (made by PKHeX from its own templates), never a
// personal save.
using PKHeX.Core;

if (args.Length == 2 && args[0] == "probe")
{
    // Every check result for one decrypted PK5 (debugging a divergence).
    var probe = new PK5(Convert.FromHexString(args[1]));
    var pla = new LegalityAnalysis(probe);
    Console.WriteLine($"{(pla.Valid ? "VALID" : "INVALID")} {pla.EncounterMatch.GetType().Name}:{pla.EncounterMatch.Species}");
    foreach (var r in pla.Results)
        Console.WriteLine($"  {(r.Valid ? "ok " : "BAD")} {r.Identifier}:{r.Result}");
    return 0;
}
if (args.Length != 3 || args[0] != "generate")
{
    Console.Error.WriteLine("usage: generate SEED MUTATIONS");
    return 2;
}
var rng = new Random(int.Parse(args[1]));
var mutations = int.Parse(args[2]);
GameVersion[] versions = [GameVersion.B, GameVersion.W, GameVersion.B2, GameVersion.W2];

var count = 0;
var seen = new HashSet<string>();
for (ushort species = 1; species <= 649; species++)
{
    var pi = PersonalTable.B2W2[species];
    for (byte form = 0; form < Math.Max((byte)1, pi.FormCount); form++)
    {
        foreach (var version in versions)
        {
            var tr = new SimpleTrainerInfo(version)
            {
                OT = "Tester",
                TID16 = (ushort)rng.Next(1, 65535),
                SID16 = (ushort)rng.Next(1, 65535),
                Language = (int)LanguageID.English,
            };
            var blank = new PK5 { Species = species, Form = form, Version = version };
            IEnumerable<IEncounterable> encounters;
            try { encounters = EncounterMovesetGenerator.GenerateEncounters(blank, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
            catch (Exception) { continue; }
            foreach (var enc in encounters)
            {
                if (enc.Context != EntityContext.Gen5)
                    continue;
                var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.EggLocation}:{enc.LevelMin}:{enc.LongName}";
                if (!seen.Add(key))
                    continue;
                PKM made;
                try { made = enc.ConvertToPKM(tr); }
                catch (Exception) { continue; }
                if (made is not PK5 pk)
                    continue;
                Emit($"b{count++}", pk);
                for (int m = 0; m < mutations; m++)
                {
                    var copy = pk.Clone();
                    var name = Mutate(copy, rng);
                    copy.RefreshChecksum();
                    Emit($"m{count++}-{name}", copy);
                }
            }
        }
    }
}
return 0;

static void Emit(string id, PK5 pk)
{
    pk.RefreshChecksum(); // a freshly converted PK5 has not been written yet
    var la = new LegalityAnalysis(pk);
    var enc = la.EncounterMatch;
    var invalid = la.Results.Where(r => !r.Valid).Select(r => $"{r.Identifier}:{r.Result}")
        .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
            .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
    var hex = Convert.ToHexString(pk.Data[..136]);
    Console.WriteLine($"{id} {hex} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species} ; {string.Join(' ', invalid)}");
}

static string Mutate(PK5 pk, Random rng)
{
    switch (rng.Next(28))
    {
        case 0: pk.Ball = (byte)rng.Next(1, 26); return "ball";
        case 1: pk.MetLevel = (byte)Math.Clamp(pk.MetLevel + rng.Next(-3, 4), 0, 100); return "metlevel";
        case 2: pk.HiddenAbility = !pk.HiddenAbility; return "hidden";
        case 3: pk.PID ^= 0x10000; return "abilitybit";
        case 4: pk.Gender = (byte)(pk.Gender ^ 1); return "gender";
        case 5: pk.Nature = (Nature)rng.Next(25); return "nature";
        case 6: pk.PID = (uint)rng.NextInt64(0, 1L << 32); return "pid";
        case 7: pk.MetLocation = (ushort)new[] { 4, 6, 8, 19, 32, 64, 75, 117, 30001, 30002, 30003, 30015, 40001, 60002 }[rng.Next(14)]; return "metlocation";
        case 8: pk.SetMove(rng.Next(4) == 0 ? 3 : 0, (ushort)rng.Next(1, 560)); return "move";
        case 9: pk.SetEV(rng.Next(6), rng.Next(256)); return "ev";
        case 10: pk.SetIV(rng.Next(6), rng.Next(32)); return "iv";
        case 11: pk.FatefulEncounter = !pk.FatefulEncounter; return "fateful";
        case 12: pk.EggLocation = 60002; return "egglocation";
        case 13: pk.Version = (GameVersion)rng.Next(20, 24); return "version";
        case 14: pk.Form = (byte)rng.Next(4); return "form";
        case 15: pk.Language = rng.Next(0, 10); return "language";
        case 16: pk.HeldItem = rng.Next(0, 650); return "item";
        case 17: pk.EXP = (uint)Math.Max(0, (int)pk.EXP + rng.Next(-2000, 2000)); return "exp";
        case 18: pk.Ability = rng.Next(1, 165); return "ability";
        case 19: pk.MetDate = new DateOnly(2000 + rng.Next(0, 99), rng.Next(1, 13), rng.Next(1, 29)); return "metdate";
        case 20: pk.Move1_PPUps = rng.Next(0, 4); pk.Move1_PP = rng.Next(0, 70); return "pp";
        case 21: pk.Data[0x24 + rng.Next(4)] |= (byte)(1 << rng.Next(8)); return "ribbon";
        case 22: pk.ContestCool = (byte)rng.Next(1, 256); return "contest";
        case 23: pk.Species = (ushort)Math.Clamp(pk.Species + (rng.Next(2) == 0 ? -1 : 1), 1, 649); return "species";
        case 24: pk.NSparkle = !pk.NSparkle; return "nsparkle";
        case 25: pk.PokerusState = (byte)rng.Next(256); return "pokerus";
        case 26: pk.Data[0x16] = (byte)rng.Next(256); return "markings";
        default: pk.TID16 = (ushort)rng.Next(65536); return "tid";
    }
}
