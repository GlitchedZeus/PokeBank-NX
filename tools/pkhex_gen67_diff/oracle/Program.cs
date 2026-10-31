// Gen VI / Gen VII differential oracle for PokeBank NX (PK6 and PK7 records).
//
//   dotnet PKHeXGen67Oracle.dll dump                     (C++ tables, see include/Legality/Gen67LegalityData.inc)
//   dotnet PKHeXGen67Oracle.dll dump8                    (C++ tables, see include/Legality/Gen8LegalityData.inc)
//   dotnet PKHeXGen67Oracle.dll dump7b                   (C++ tables, see include/Legality/Gen7bLegalityData.inc)
//   dotnet PKHeXGen67Oracle.dll dump8a                   (C++ tables, see include/Legality/Gen8aLegalityData.inc)
//   dotnet PKHeXGen67Oracle.dll dump9a                   (C++ tables, see include/Legality/Gen9aLegalityData.inc)
//   dotnet PKHeXGen67Oracle.dll generate SEED MUTATIONS  (PK6/PK7 corpus, one record per line)
//   dotnet PKHeXGen67Oracle.dll generate7b SEED MUTATIONS (PB7 corpus, format column "7b")
//   dotnet PKHeXGen67Oracle.dll generate8 SEED MUTATIONS (PK8 corpus, same line format)
//   dotnet PKHeXGen67Oracle.dll generate8b SEED MUTATIONS (PB8 corpus, format column "8b")
//   dotnet PKHeXGen67Oracle.dll generate9 SEED MUTATIONS (PK9 corpus, format column "9")
//   dotnet PKHeXGen67Oracle.dll generate8a SEED MUTATIONS (PA8 corpus, format column "8a")
//   dotnet PKHeXGen67Oracle.dll generate9a SEED MUTATIONS (PA9 corpus, format column "9a")
//   dotnet PKHeXGen67Oracle.dll probe FORMAT SLOT HEX    (every PKHeX check result for one record)
//
// "dump" writes every PKHeX value the C++ analyzer reads (personal abilities, gender
// ratios, form counts and growth rates from PersonalTable.AO / PersonalTable.USUM, base
// PP, released held items, battle-only forms, form items, Vivillon console regions and
// the per-game ID limits) so the generated header can be checked against the pinned
// PKHeX source in CI.
//
// "generate" builds, with a fixed test trainer, every Gen VI encounter PKHeX knows for
// X, Y, Omega Ruby and Alpha Sapphire (PK6) and every Gen VII encounter for Sun, Moon,
// Ultra Sun and Ultra Moon (PK7), Gen V encounters moved forward with Poke Transporter
// (PK5 -> PK6 -> PK7), Virtual Console Gen I/II encounters moved with Poke Transporter
// (PK1/PK2 -> PK7) and "grown" copies (levelled, EVs, party records), plus MUTATIONS
// single-field mutations of each. One line per record:
//   <id> <6|7> <game> <box|party> <decrypted hex> VALID|INVALID <encounter>:<species> <group> ; <identifier>:<result> ...
//
// "generate8" does the same for Sword/Shield (PK8): every Gen VIII encounter PKHeX knows for
// Sword and Shield, records moved in through HOME (Gen VI/VII 3DS, Virtual Console, Let's Go,
// Brilliant Diamond/Shining Pearl, Legends: Arceus and Scarlet/Violet origins), grown copies
// and PK8-specific mutations (Dynamax level, stat nature, battle version, TR flags, ...).
// The records are synthetic (made by PKHeX from its own templates), never a personal save.
using System.Reflection;
using System.Text;
using PKHeX.Core;

if (args.Length == 1 && args[0] == "dump")
{
    Dump.Run();
    return 0;
}
if (args.Length == 1 && args[0] == "dump7b")
{
    Dump7b.Run();
    return 0;
}
if (args.Length == 1 && args[0] == "dump9a")
{
    Dump9a.Run();
    return 0;
}
if (args.Length == 1 && args[0] == "dump8a")
{
    Dump8a.Run();
    return 0;
}
if (args.Length == 1 && args[0] == "dump8")
{
    Dump8.Run();
    return 0;
}
if (args.Length == 4 && args[0] == "probe")
{
    var data = Convert.FromHexString(args[3]);
    PKM pk = args[1] switch { "6" => new PK6(data), "7" => new PK7(data), "8b" => new PB8(data), "9" => new PK9(data), "7b" => new PB7(data), "8a" => new PA8(data), "9a" => new PA9(data), _ => new PK8(data) };
    var slot = args[2] == "party" ? StorageSlotType.Party : StorageSlotType.Box;
    var la = new LegalityAnalysis(pk, slot);
    Console.WriteLine($"{(la.Valid ? "VALID" : "INVALID")} {la.EncounterMatch.GetType().Name}:{la.EncounterMatch.Species} v={pk.Version} lvl={pk.CurrentLevel}");
    foreach (var r in la.Results)
        Console.WriteLine($"  {(r.Valid ? "ok " : "BAD")} {r.Judgement} {r.Identifier}:{r.Result}");
    for (int i = 0; i < 4; i++)
        Console.WriteLine($"  move{i + 1} {la.Info.Moves[i].Valid} {la.Info.Moves[i].Info.Method}");
    return 0;
}
if (args.Length != 3 || args[0] is not ("generate" or "generate7b" or "generate8" or "generate8a" or "generate9a" or "generate8b" or "generate9"))
{
    Console.Error.WriteLine("usage: dump | dump7b | dump8 | generate SEED MUTATIONS | generate7b SEED MUTATIONS | generate8 SEED MUTATIONS | generate8b SEED MUTATIONS | generate9 SEED MUTATIONS | generate8a SEED MUTATIONS | generate9a SEED MUTATIONS | probe 6|7|7b|8|8a|8b|9|9a box|party HEX");
    return 2;
}
var gen = new Generator(new Random(int.Parse(args[1])), int.Parse(args[2]));
if (args[0] == "generate9a")
{
    gen.Native9a();
    gen.Home9a();
    gen.Grown();
    return 0;
}
if (args[0] == "generate8a")
{
    gen.Native8a();
    gen.Home8a();
    gen.Grown();
    return 0;
}
if (args[0] == "generate7b")
{
    gen.Native7b();
    gen.Grown();
    return 0;
}
if (args[0] == "generate9")
{
    gen.Native9();
    gen.Home9();
    gen.Grown();
    return 0;
}
if (args[0] == "generate8b")
{
    gen.Native8b();
    gen.Home8b();
    gen.Grown();
    return 0;
}
if (args[0] == "generate8")
{
    gen.Native8();
    gen.Home8();
    gen.Grown();
    return 0;
}
gen.Native6();
gen.Native7();
gen.Transfers();
gen.VirtualConsole();
gen.Grown();
return 0;

static class Dump
{
    internal static int Const(string name) =>
        Convert.ToInt32(typeof(Legal).GetField(name, BindingFlags.Static | BindingFlags.NonPublic | BindingFlags.Public)!.GetValue(null)!);

    public static void Run()
    {
        var o = Console.Out;
        o.WriteLine("// GENERATED by tools/pkhex_gen67_diff/oracle (dotnet PKHeXGen67Oracle.dll dump) -- do not edit.");
        o.WriteLine("// Source: PKHeX.Core at the commit pinned in .github/workflows/legality-gen67-pkhex-diff.yml.");
        o.WriteLine("// Included once, by src/Legality/Gen67LegalityData.cpp.");
        o.WriteLine($"constexpr uint16_t kMaxSpecies6 = {Const("MaxSpeciesID_6")}, kMaxMove6XY = {Const("MaxMoveID_6_XY")}, kMaxMove6AO = {Const("MaxMoveID_6_AO")};");
        o.WriteLine($"constexpr uint16_t kMaxItem6XY = {Const("MaxItemID_6_XY")}, kMaxItem6AO = {Const("MaxItemID_6_AO")}, kMaxBall6 = {Const("MaxBallID_6")};");
        o.WriteLine($"constexpr uint16_t kMaxSpecies7SM = {Const("MaxSpeciesID_7")}, kMaxMove7SM = {Const("MaxMoveID_7")}, kMaxItem7SM = {Const("MaxItemID_7")};");
        o.WriteLine($"constexpr uint16_t kMaxSpecies7 = {Const("MaxSpeciesID_7_USUM")}, kMaxMove7 = {Const("MaxMoveID_7_USUM")}, kMaxItem7 = {Const("MaxItemID_7_USUM")}, kMaxBall7 = {Const("MaxBallID_7")};");
        o.WriteLine($"constexpr uint8_t kVivillonMaxWildForm = {Vivillon3DS.MaxWildFormID};");
        Personal(o, "kPersonal6", PersonalTable.AO, Const("MaxSpeciesID_6"));
        Personal(o, "kPersonal7", PersonalTable.USUM, Const("MaxSpeciesID_7_USUM"));
        Bytes(o, "kBasePP6", Enumerable.Range(0, Const("MaxMoveID_6_AO") + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen6, (ushort)m)));
        Bytes(o, "kBasePP7", Enumerable.Range(0, Const("MaxMoveID_7_USUM") + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen7, (ushort)m)));
        Bits(o, "kHeldItems6", Enumerable.Range(0, Const("MaxItemID_6_AO") + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen6)));
        Bits(o, "kHeldItems7", Enumerable.Range(0, Const("MaxItemID_7_USUM") + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen7)));
        foreach (var (name, format, table, max) in new[] { ("kBattleForms6", (byte)6, (IPersonalTable)PersonalTable.AO, Const("MaxSpeciesID_6")), ("kBattleForms7", (byte)7, PersonalTable.USUM, Const("MaxSpeciesID_7_USUM")) })
        {
            var rows = new List<string>();
            for (ushort s = 1; s <= max; s++)
            {
                for (byte f = 0; f < 32; f++)
                {
                    if (FormInfo.IsBattleOnlyForm(s, f, format))
                        rows.Add($"{{{s},{f}}}");
                }
            }
            o.WriteLine($"constexpr std::array<SpeciesForm, {rows.Count}> {name}{{{{{string.Join(",", rows)}}}}};");
        }
        foreach (var (name, format, maxItem) in new[] { ("kArceusItems6", (byte)6, Const("MaxItemID_6_AO")), ("kArceusItems7", (byte)7, Const("MaxItemID_7_USUM")) })
            FormItems(o, name, maxItem, i => FormItem.GetFormArceus(i, format));
        FormItems(o, "kGenesectItems", Const("MaxItemID_7_USUM"), i => FormItem.GetFormGenesect(i));
        FormItems(o, "kSilvallyItems", Const("MaxItemID_7_USUM"), i => FormItem.GetFormSilvally(i));
        var viv = new List<string>();
        for (byte f = 0; f <= Vivillon3DS.MaxWildFormID; f++)
        {
            int mask = 0;
            for (byte r = 0; r < 8; r++)
                if (Vivillon3DS.IsPatternValid(f, r)) mask |= 1 << r;
            viv.Add($"0x{mask:X2}");
        }
        o.WriteLine($"constexpr std::array<uint8_t, {viv.Count}> kVivillonConsoleRegions{{{{{string.Join(",", viv)}}}}};");
        // Locale3DS.IsConsoleRegionCountryValid: one 256-bit country set per console region 0-6.
        var countries = new List<int>();
        for (byte r = 0; r < 7; r++)
        {
            var bits = new byte[32];
            for (int c = 0; c < 256; c++)
                if (Locale3DS.IsConsoleRegionCountryValid(r, (byte)c)) bits[c >> 3] |= (byte)(1 << (c & 7));
            countries.AddRange(bits.Select(b => (int)b));
        }
        Bytes(o, "kConsoleRegionCountries", countries);
        // Locale3DS.IsRegionLockedLanguageValidVC: language bit mask per console region 0-6.
        var vcLang = new List<string>();
        for (byte r = 0; r < 7; r++)
        {
            int mask = 0;
            for (int l = 0; l < 16; l++)
                if (Locale3DS.IsRegionLockedLanguageValidVC(r, l)) mask |= 1 << l;
            vcLang.Add($"0x{mask:X4}");
        }
        o.WriteLine($"constexpr std::array<uint16_t, {vcLang.Count}> kVcRegionLanguages{{{{{string.Join(",", vcLang)}}}}};");
        // Experience.IsValidNatureMetLevel2: nature bit mask per growth rate 0-5.
        var natures = new List<string>();
        for (byte g = 0; g < 6; g++)
        {
            uint mask = 0;
            for (int n = 0; n < 25; n++)
                if (Experience.IsValidNatureMetLevel2(g, (Nature)n)) mask |= 1u << n;
            natures.Add($"0x{mask:X8}u");
        }
        o.WriteLine($"constexpr std::array<uint32_t, {natures.Count}> kVcNatureMetLevel2{{{{{string.Join(",", natures)}}}}};");
    }

    private static void Personal(TextWriter o, string name, IPersonalTable table, int maxSpecies)
    {
        // One row per species and form: {species, form, ability1, ability2, hidden, gender, growth, formCount}.
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var baseInfo = table[s];
            var count = Math.Max((byte)1, baseInfo.FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                var a = (IPersonalAbility12H)pi;
                rows.Add($"{{{s},{f},{a.Ability1},{a.Ability2},{a.AbilityH},{pi.Gender},{pi.EXPGrowth},{count}}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow, {rows.Count}> {name}{{{{");
        for (int i = 0; i < rows.Count; i += 8)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(8)) + ",");
        o.WriteLine("}};");
    }

    internal static void Bytes(TextWriter o, string name, IEnumerable<int> values)
    {
        var v = values.ToList();
        o.WriteLine($"constexpr std::array<uint8_t, {v.Count}> {name}{{{{");
        for (int i = 0; i < v.Count; i += 32)
            o.WriteLine("    " + string.Join(",", v.Skip(i).Take(32)) + ",");
        o.WriteLine("}};");
    }

    internal static void Bits(TextWriter o, string name, IEnumerable<bool> values)
    {
        var v = values.ToList();
        var bytes = new byte[(v.Count + 7) / 8];
        for (int i = 0; i < v.Count; i++)
            if (v[i]) bytes[i >> 3] |= (byte)(1 << (i & 7));
        o.WriteLine($"constexpr uint16_t {name}Count = {v.Count};");
        Bytes(o, name, bytes.Select(b => (int)b));
    }

    internal static void FormItems(TextWriter o, string name, int maxItem, Func<int, byte> get)
    {
        var rows = new List<string>();
        for (int i = 1; i <= maxItem; i++)
        {
            var f = get(i);
            if (f != 0) rows.Add($"{{{i},{f}}}");
        }
        o.WriteLine($"constexpr std::array<ItemForm, {rows.Count}> {name}{{{{{string.Join(",", rows)}}}}};");
    }
}

static class Dump7b
{
    public static void Run()
    {
        var o = Console.Out;
        int maxSpecies = Dump.Const("MaxSpeciesID_7b"), maxMove = Dump.Const("MaxMoveID_7b"), maxItem = Dump.Const("MaxItemID_7b");
        o.WriteLine("// GENERATED by tools/pkhex_gen67_diff/oracle (dotnet PKHeXGen67Oracle.dll dump7b) -- do not edit.");
        o.WriteLine("// Source: PKHeX.Core at the commit pinned in .github/workflows/legality-gen67-pkhex-diff.yml.");
        o.WriteLine("// Included once, by src/Legality/Gen7bLegalityData.cpp.");
        o.WriteLine($"constexpr uint16_t kMaxSpecies7b = {maxSpecies}, kMaxMove7b = {maxMove}, kMaxItem7b = {maxItem}, kMaxBall7b = {Dump.Const("MaxBallID_7b")};");
        // PersonalTable.GG, one row per species and form: {species, form, ability1, ability2,
        // hidden, gender, growth, formCount, present, HP, Atk, Def, Spe, SpA, SpD, height, weight}.
        var table = PersonalTable.GG;
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                rows.Add($"{{{s},{f},{pi.Ability1},{pi.Ability2},{pi.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)},{pi.HP},{pi.ATK},{pi.DEF},{pi.SPE},{pi.SPA},{pi.SPD},{pi.Height},{pi.Weight}}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow7b, {rows.Count}> kPersonal7b{{{{");
        for (int i = 0; i < rows.Count; i += 6)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(6)) + ",");
        o.WriteLine("}};");
        Dump.Bytes(o, "kBasePP7b", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen7b, (ushort)m)));
        Dump.Bits(o, "kHeldItems7b", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen7b)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 7))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms7b{{{{{string.Join(",", battle)}}}}};");
    }
}

static class Dump8
{
    public static void Run()
    {
        var o = Console.Out;
        int maxSpecies = Dump.Const("MaxSpeciesID_8"), maxMove = Dump.Const("MaxMoveID_8_R2"), maxItem = Dump.Const("MaxItemID_8_R2");
        o.WriteLine("// GENERATED by tools/pkhex_gen67_diff/oracle (dotnet PKHeXGen67Oracle.dll dump8) -- do not edit.");
        o.WriteLine("// Source: PKHeX.Core at the commit pinned in .github/workflows/legality-gen67-pkhex-diff.yml.");
        o.WriteLine("// Included once, by src/Legality/Gen8LegalityData.cpp.");
        o.WriteLine($"constexpr uint16_t kMaxSpecies8 = {maxSpecies}, kMaxMove8 = {maxMove}, kMaxItem8 = {maxItem}, kMaxBall8 = {Dump.Const("MaxBallID_8")};");
        // PersonalTable.SWSH, one row per species and form: {species, form, ability1, ability2,
        // hidden, gender, growth, formCount, present, HP, Atk, Def, Spe, SpA, SpD}, and the
        // row's TR permissions (PersonalInfo8SWSH.GetIsLearnTR, 100 bits in 13 bytes).
        var table = PersonalTable.SWSH;
        var rows = new List<string>();
        var tr = new List<int>();
        int trCount = ((IPermitRecord)table[1]).RecordCountUsed;
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                rows.Add($"{{{s},{f},{pi.Ability1},{pi.Ability2},{pi.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)},{pi.HP},{pi.ATK},{pi.DEF},{pi.SPE},{pi.SPA},{pi.SPD}}}");
                var bits = new byte[(trCount + 7) / 8];
                for (int i = 0; i < trCount; i++)
                    if (pi.GetIsLearnTR(i)) bits[i >> 3] |= (byte)(1 << (i & 7));
                tr.AddRange(bits.Select(b => (int)b));
            }
        }
        o.WriteLine($"constexpr uint8_t kTechRecordCount8 = {trCount};");
        o.WriteLine($"constexpr std::array<PersonalRow8, {rows.Count}> kPersonal8{{{{");
        for (int i = 0; i < rows.Count; i += 6)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(6)) + ",");
        o.WriteLine("}};");
        Dump.Bytes(o, "kTechRecords8", tr);
        Dump.Bytes(o, "kBasePP8", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen8, (ushort)m)));
        Dump.Bits(o, "kHeldItems8", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen8)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            // Forms at or above the form count are already outside the species' forms.
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 8))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms8{{{{{string.Join(",", battle)}}}}};");
        Dump.FormItems(o, "kArceusItems8", maxItem, i => FormItem.GetFormArceus(i, 8));
        Dump.FormItems(o, "kGenesectItems8", maxItem, i => FormItem.GetFormGenesect(i));
        Dump.FormItems(o, "kSilvallyItems8", maxItem, i => FormItem.GetFormSilvally(i));
        // Nature.IsMint: the natures a stat-nature byte may hold when it differs from the nature.
        uint mints = 0;
        for (int n = 0; n < 25; n++)
            if (((Nature)n).IsMint) mints |= 1u << n;
        o.WriteLine($"constexpr uint32_t kMintNatures = 0x{mints:X8}u;");
        Bdsp(o);
        ScarletViolet(o);
    }

    /// <summary>
    /// Scarlet/Violet (PK9) tables, same row layout as Sword/Shield, plus the internal-to-national
    /// species map (SpeciesConverter.GetNational9) for the internal ids that differ.
    /// </summary>
    private static void ScarletViolet(TextWriter o)
    {
        int maxSpecies = Dump.Const("MaxSpeciesID_9"), maxMove = Dump.Const("MaxMoveID_9"), maxItem = Dump.Const("MaxItemID_9");
        o.WriteLine($"constexpr uint16_t kMaxSpecies9 = {maxSpecies}, kMaxMove9 = {maxMove}, kMaxItem9 = {maxItem}, kMaxBall9 = {Dump.Const("MaxBallID_9")};");
        var table = PersonalTable.SV;
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                var a = (IPersonalAbility12H)pi;
                rows.Add($"{{{s},{f},{a.Ability1},{a.Ability2},{a.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)},{pi.HP},{pi.ATK},{pi.DEF},{pi.SPE},{pi.SPA},{pi.SPD}}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow8, {rows.Count}> kPersonal9{{{{");
        for (int i = 0; i < rows.Count; i += 6)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(6)) + ",");
        o.WriteLine("}};");
        Dump.Bytes(o, "kBasePP9", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen9, (ushort)m)));
        Dump.Bits(o, "kHeldItems9", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen9)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 9))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms9{{{{{string.Join(",", battle)}}}}};");
        Dump.FormItems(o, "kArceusItems9", maxItem, i => FormItem.GetFormArceus(i, 9));
        Dump.FormItems(o, "kOgerponItems9", maxItem, i => FormItem.GetFormOgerpon(i));
        // TM permissions of a species' whole line: every form of the species and of every
        // pre-evolution (EvolutionTree.Evolves9). MiscVerifierPK9.VerifyTechRecordFlags accepts a
        // flag that the current species/form or a pre-evolution in its chain can learn, so a flag
        // outside this union is never accepted.
        int tmCount = ((IPermitRecord)table[1]).RecordCountUsed;
        var tms = new List<int>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var line = new HashSet<ushort>();
            var todo = new Stack<ushort>();
            if (s != 0) todo.Push(s);
            while (todo.Count != 0)
            {
                var sp = todo.Pop();
                if (!line.Add(sp)) continue;
                for (byte f = 0; f < Math.Max((byte)1, table[sp].FormCount); f++)
                    foreach (var (pre, _) in EvolutionTree.Evolves9.Reverse.GetPreEvolutions(sp, f))
                        todo.Push(pre);
            }
            var bits = new byte[(tmCount + 7) / 8];
            foreach (var sp in line)
            {
                for (byte f = 0; f < Math.Max((byte)1, table[sp].FormCount); f++)
                {
                    var pi = table.GetFormEntry(sp, f);
                    for (int i = 0; i < tmCount; i++)
                        if (pi.GetIsLearnTM(i)) bits[i >> 3] |= (byte)(1 << (i & 7));
                }
            }
            tms.AddRange(bits.Select(b => (int)b));
        }
        o.WriteLine($"constexpr uint16_t kTmCount9 = {tmCount};");
        Dump.Bytes(o, "kTmLine9", tms);
        // SpeciesConverter.GetNational9 for every internal id up to the first one past the
        // highest species that maps to itself again; ids above kSpecies9First + size map to themselves.
        const int first = 917;
        int last = first;
        for (int raw = first; raw < 2048; raw++)
            if (SpeciesConverter.GetNational9((ushort)raw) != raw) last = raw;
        o.WriteLine($"constexpr uint16_t kSpecies9First = {first};");
        o.WriteLine($"constexpr std::array<uint16_t, {last - first + 1}> kSpecies9National{{{{{string.Join(",", Enumerable.Range(first, last - first + 1).Select(r => SpeciesConverter.GetNational9((ushort)r)))}}}}};");
    }

    /// <summary>Brilliant Diamond / Shining Pearl (PB8) tables, same row layout as Sword/Shield.</summary>
    private static void Bdsp(TextWriter o)
    {
        int maxSpecies = Dump.Const("MaxSpeciesID_8b"), maxMove = Dump.Const("MaxMoveID_8b"), maxItem = Dump.Const("MaxItemID_8b");
        o.WriteLine($"constexpr uint16_t kMaxSpecies8b = {maxSpecies}, kMaxMove8b = {maxMove}, kMaxItem8b = {maxItem}, kMaxBall8b = {Dump.Const("MaxBallID_8b")};");
        var table = PersonalTable.BDSP;
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                var a = (IPersonalAbility12H)pi;
                rows.Add($"{{{s},{f},{a.Ability1},{a.Ability2},{a.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)},{pi.HP},{pi.ATK},{pi.DEF},{pi.SPE},{pi.SPA},{pi.SPD}}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow8, {rows.Count}> kPersonal8b{{{{");
        for (int i = 0; i < rows.Count; i += 6)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(6)) + ",");
        o.WriteLine("}};");
        Dump.Bytes(o, "kBasePP8b", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen8b, (ushort)m)));
        Dump.Bits(o, "kHeldItems8b", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen8b)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 8))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms8b{{{{{string.Join(",", battle)}}}}};");
        Dump.FormItems(o, "kArceusItems8b", maxItem, i => FormItem.GetFormArceus(i, 8));
    }
}

static class Dump8a
{
    public static void Run()
    {
        var o = Console.Out;
        int maxSpecies = Dump.Const("MaxSpeciesID_8a"), maxMove = Dump.Const("MaxMoveID_8a"), maxItem = Dump.Const("MaxItemID_8a");
        o.WriteLine("// GENERATED by tools/pkhex_gen67_diff/oracle (dotnet PKHeXGen67Oracle.dll dump8a) -- do not edit.");
        o.WriteLine("// Source: PKHeX.Core at the commit pinned in .github/workflows/legality-gen67-pkhex-diff.yml.");
        o.WriteLine("// Included once, by src/Legality/Gen8aLegalityData.cpp.");
        o.WriteLine($"constexpr uint16_t kMaxSpecies8a = {maxSpecies}, kMaxMove8a = {maxMove}, kMaxItem8a = {maxItem}, kMaxBall8a = {Dump.Const("MaxBallID_8a")};");
        // PersonalTable.LA, one row per species and form: {species, form, ability1, ability2,
        // hidden, gender, growth, formCount, present, height, weight, move shop permissions}.
        var table = PersonalTable.LA;
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                ulong shop = 0;
                for (int i = 0; i < pi.RecordCountUsed; i++)
                    if (pi.IsRecordPermitted(i)) shop |= 1ul << i;
                rows.Add($"{{{s},{f},{pi.Ability1},{pi.Ability2},{pi.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)},{pi.Height},{pi.Weight},0x{shop:X}ull}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow8a, {rows.Count}> kPersonal8a{{{{");
        for (int i = 0; i < rows.Count; i += 6)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(6)) + ",");
        o.WriteLine("}};");
        // PersonalInfo8LA.RecordPermitIndexes: the move shop's moves, by record index.
        var shopMoves = table[1].RecordPermitIndexes.ToArray();
        o.WriteLine($"constexpr std::array<uint16_t, {shopMoves.Length}> kMoveShop8a{{{{{string.Join(",", shopMoves)}}}}};");
        Dump.Bytes(o, "kBasePP8a", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen8a, (ushort)m)));
        Dump.Bits(o, "kHeldItems8a", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen8a)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 8))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms8a{{{{{string.Join(",", battle)}}}}};");
        uint mints = 0;
        for (int n = 0; n < 25; n++)
            if (((Nature)n).IsMint) mints |= 1u << n;
        o.WriteLine($"constexpr uint32_t kMintNatures8a = 0x{mints:X8}u;");
    }
}

static class Dump9a
{
    public static void Run()
    {
        var o = Console.Out;
        int maxSpecies = Dump.Const("MaxSpeciesID_9a"), maxMove = Dump.Const("MaxMoveID_9a"), maxItem = Dump.Const("MaxItemID_9a");
        o.WriteLine("// GENERATED by tools/pkhex_gen67_diff/oracle (dotnet PKHeXGen67Oracle.dll dump9a) -- do not edit.");
        o.WriteLine("// Source: PKHeX.Core at the commit pinned in .github/workflows/legality-gen67-pkhex-diff.yml.");
        o.WriteLine("// Included once, by src/Legality/Gen9aLegalityData.cpp.");
        o.WriteLine($"constexpr uint16_t kMaxSpecies9a = {maxSpecies}, kMaxMove9a = {maxMove}, kMaxItem9a = {maxItem}, kMaxBall9a = {Dump.Const("MaxBallID_9a")};");
        // PersonalTable.ZA, one row per species and form: {species, form, ability1, ability2,
        // hidden, gender, growth, formCount, present}.
        var table = PersonalTable.ZA;
        var rows = new List<string>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var count = Math.Max((byte)1, table[s].FormCount);
            for (byte f = 0; f < count; f++)
            {
                var pi = table.GetFormEntry(s, f);
                rows.Add($"{{{s},{f},{pi.Ability1},{pi.Ability2},{pi.AbilityH},{pi.Gender},{pi.EXPGrowth},{count},{(table.IsPresentInGame(s, f) ? 1 : 0)}}}");
            }
        }
        o.WriteLine($"constexpr std::array<PersonalRow9a, {rows.Count}> kPersonal9a{{{{");
        for (int i = 0; i < rows.Count; i += 8)
            o.WriteLine("    " + string.Join(",", rows.Skip(i).Take(8)) + ",");
        o.WriteLine("}};");
        Dump.Bytes(o, "kBasePP9a", Enumerable.Range(0, maxMove + 1).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen9a, (ushort)m)));
        Dump.Bits(o, "kHeldItems9a", Enumerable.Range(0, maxItem + 1).Select(i => ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen9a)));
        var battle = new List<string>();
        for (ushort s = 1; s <= maxSpecies; s++)
        {
            for (byte f = 0; f < table[s].FormCount; f++)
            {
                if (FormInfo.IsBattleOnlyForm(s, f, 9))
                    battle.Add($"{{{s},{f}}}");
            }
        }
        o.WriteLine($"constexpr std::array<SpeciesForm, {battle.Count}> kBattleForms9a{{{{{string.Join(",", battle)}}}}};");
        // LegendsZAVerifier.GetInvalidPlusMove superset: for each species, the Plus move indexes
        // that any form of the species or of any pre-evolution learns by TM or by level up (at
        // any level), or holds in its Plus learnset.
        var plusMoves = PersonalInfo9ZA.PlusMoves;
        int plusCount = table[1].PlusCountUsed;
        var plus = new List<int>();
        for (ushort s = 0; s <= maxSpecies; s++)
        {
            var line = new HashSet<ushort>();
            var todo = new Stack<ushort>();
            if (s != 0) todo.Push(s);
            while (todo.Count != 0)
            {
                var sp = todo.Pop();
                if (!line.Add(sp)) continue;
                for (byte f = 0; f < Math.Max((byte)1, table[sp].FormCount); f++)
                    foreach (var (pre, _) in EvolutionTree.Evolves9a.Reverse.GetPreEvolutions(sp, f))
                        todo.Push(pre);
            }
            var bits = new byte[(plusCount + 7) / 8];
            foreach (var sp in line)
            {
                for (byte f = 0; f < Math.Max((byte)1, table[sp].FormCount); f++)
                {
                    var pi = table.GetFormEntry(sp, f);
                    var (learn, plusSet) = LearnSource9ZA.GetLearnsetAndPlus(sp, f);
                    for (int i = 0; i < plusCount; i++)
                    {
                        var move = plusMoves[i];
                        var tm = pi.RecordPermitIndexes.IndexOf(move);
                        if ((tm != -1 && pi.GetIsLearnTM(tm)) || learn.GetAllMoves().Contains(move) || plusSet.GetAllMoves().Contains(move))
                            bits[i >> 3] |= (byte)(1 << (i & 7));
                    }
                }
            }
            plus.AddRange(bits.Select(b => (int)b));
        }
        o.WriteLine($"constexpr uint16_t kPlusCount9a = {plusCount};");
        Dump.Bytes(o, "kPlusLine9a", plus);
        uint mints = 0;
        for (int n = 0; n < 25; n++)
            if (((Nature)n).IsMint) mints |= 1u << n;
        o.WriteLine($"constexpr uint32_t kMintNatures9a = 0x{mints:X8}u;");
    }
}

sealed class Generator(Random rng, int mutations)
{
    private int count;
    private readonly List<(PKM pk, string group)> grownSeeds = [];
    private static readonly GameVersion[] Versions6 = [GameVersion.X, GameVersion.Y, GameVersion.OR, GameVersion.AS];
    private static readonly GameVersion[] Versions7 = [GameVersion.SN, GameVersion.MN, GameVersion.US, GameVersion.UM];
    private static readonly GameVersion[] Versions8 = [GameVersion.SW, GameVersion.SH];
    private static readonly GameVersion[] Versions8b = [GameVersion.BD, GameVersion.SP];
    private static readonly GameVersion[] Versions9 = [GameVersion.SL, GameVersion.VL];

    private SimpleTrainerInfo Trainer(GameVersion version) => new(version)
    {
        OT = "Tester",
        TID16 = (ushort)rng.Next(1, 65535),
        SID16 = (ushort)rng.Next(1, 65535),
        Language = (int)LanguageID.English,
        Gender = (byte)rng.Next(2),
    };

    public void Native6() => Native(Versions6, 721, PersonalTable.AO, EntityContext.Gen6, v => new PK6 { Version = v }, "native6");
    public void Native7() => Native(Versions7, 807, PersonalTable.USUM, EntityContext.Gen7, v => new PK7 { Version = v }, "native7");

    /// <summary>Sword/Shield encounters, at most <see cref="PerForm8"/> per species, form and game.</summary>
    public void Native8() => Native(Versions8, 898, PersonalTable.SWSH, EntityContext.Gen8, v => new PK8 { Version = v }, "native8", PerForm8);

    private const int PerForm8 = 8;

    /// <summary>Brilliant Diamond/Shining Pearl encounters, at most <see cref="PerForm8"/> per species, form and game.</summary>
    public void Native8b() => Native(Versions8b, 493, PersonalTable.BDSP, EntityContext.Gen8b, v => new PB8 { Version = v }, "native8b", PerForm8);

    /// <summary>Let's Go Pikachu/Eevee encounters (including GO Park transfers), at most <see cref="PerForm8"/> per species, form and game.</summary>
    public void Native7b() => Native([GameVersion.GP, GameVersion.GE], 809, PersonalTable.GG, EntityContext.Gen7b, v => new PB7 { Version = v }, "native7b", PerForm8);

    /// <summary>Scarlet/Violet encounters, at most <see cref="PerForm8"/> per species, form and game.</summary>
    public void Native9() => Native(Versions9, 1025, PersonalTable.SV, EntityContext.Gen9, v => new PK9 { Version = v }, "native9", PerForm8);

    /// <summary>Legends: Arceus encounters, at most <see cref="PerForm8"/> per species and form.</summary>
    public void Native8a() => Native([GameVersion.PLA], 905, PersonalTable.LA, EntityContext.Gen8a, v => new PA8 { Version = v }, "native8a", PerForm8);

    /// <summary>Records moved into Legends: Arceus through HOME.</summary>
    public void Home8a() => Home(typeof(PA8), [GameVersion.PLA, GameVersion.PLA], PersonalTable.LA, "lahome");

    /// <summary>Legends: Z-A encounters, at most <see cref="PerForm8"/> per species and form.</summary>
    public void Native9a() => Native([GameVersion.ZA], 1000, PersonalTable.ZA, EntityContext.Gen9a, v => new PA9 { Version = v }, "native9a", PerForm8);

    /// <summary>Records moved into Legends: Z-A through HOME.</summary>
    public void Home9a() => Home(typeof(PA9), [GameVersion.ZA, GameVersion.ZA], PersonalTable.ZA, "zahome");

    /// <summary>Records moved into Scarlet/Violet through HOME.</summary>
    public void Home9() => Home(typeof(PK9), Versions9, PersonalTable.SV, "svhome");

    /// <summary>Records moved into Sword/Shield through HOME.</summary>
    public void Home8() => Home(typeof(PK8), Versions8, PersonalTable.SWSH, "home");

    /// <summary>Records moved into Brilliant Diamond/Shining Pearl through HOME.</summary>
    public void Home8b() => Home(typeof(PB8), Versions8b, PersonalTable.BDSP, "bdhome");

    /// <summary>
    /// Records moved into a Switch game through HOME: 3DS Gen VI/VII natives, Gen III-V and Virtual
    /// Console records, Let's Go, Sword/Shield or Brilliant Diamond/Shining Pearl, Legends: Arceus
    /// and Scarlet/Violet.
    /// </summary>
    private void Home(Type target, GameVersion[] games, IPersonalTable present, string prefix)
    {
        GameVersion[] v3 = [GameVersion.R, GameVersion.S, GameVersion.E, GameVersion.FR, GameVersion.LG];
        GameVersion[] v4 = [GameVersion.D, GameVersion.P, GameVersion.Pt, GameVersion.HG, GameVersion.SS];
        GameVersion[] v5 = [GameVersion.B, GameVersion.W, GameVersion.B2, GameVersion.W2];
        var sources = new (string group, int maxSpecies, GameVersion[] versions, Func<PKM> blank, int keep)[]
        {
            ("home3", 386, v3, () => new PK3(), 6),
            ("home4", 493, v4, () => new PK4(), 6),
            ("home5", 649, v5, () => new PK5(), 5),
            ("home6", 721, Versions6, () => new PK6(), 4),
            ("home7", 807, Versions7, () => new PK7(), 3),
            ("homevc", 251, [GameVersion.RD, GameVersion.GN, GameVersion.BU, GameVersion.YW, GameVersion.GD, GameVersion.SI, GameVersion.C], () => new PK2(), 3),
            ("homelgpe", 809, [GameVersion.GP, GameVersion.GE], () => new PB7(), 1),
            target == typeof(PB8) ? ("homeswsh", 898, Versions8, () => new PK8(), 4) : ("homebdsp", 493, Versions8b, () => new PB8(), 3),
            target == typeof(PA8) ? ("homeswsh", 898, Versions8, () => new PK8(), 4) : ("homepla", 905, [GameVersion.PLA], () => new PA8(), 2),
            target == typeof(PK9) ? ("homeswsh", 898, Versions8, () => new PK8(), 4) : ("homesv", 1025, [GameVersion.SL, GameVersion.VL], () => new PK9(), 3),
        };
        if (target == typeof(PA9))
            sources = [.. sources, ("homeswsh", 898, Versions8, () => new PK8(), 4)];
        foreach (var (name, maxSpecies, versions, blank, keep) in sources)
        {
            var group = prefix + name[4..];
            var seen = new HashSet<string>();
            for (ushort species = 1; species <= maxSpecies; species++)
            {
                if (rng.Next(keep) != 0) continue; // a sample of the species keeps the corpus size bounded
                if (!present.IsPresentInGame(species, 0) && rng.Next(4) != 0) continue; // a few TransferBad records
                foreach (var version in versions)
                {
                    var tr = Trainer(version);
                    var b = blank();
                    b.Species = species;
                    if (b is PK1 or PK2 && species > 151 && version is GameVersion.RD or GameVersion.GN or GameVersion.BU or GameVersion.YW)
                        continue;
                    List<IEncounterable> encounters;
                    try { encounters = EncounterMovesetGenerator.GenerateEncounters(b, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                    catch (Exception) { continue; }
                    foreach (var enc in encounters.Take(3))
                    {
                        var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.LevelMin}";
                        if (!seen.Add(key))
                            continue;
                        if (TryConvert(enc, tr) is not { } src)
                            continue;
                        PKM? moved;
                        try { moved = EntityConverter.ConvertToType(src, target, out _); }
                        catch (Exception) { continue; }
                        if (moved is not IHomeTrack home || moved.GetType() != target)
                            continue;
                        // HOME assigns a tracker; the save that receives it becomes the handler.
                        var game = games[rng.Next(2)];
                        home.Tracker = (ulong)rng.NextInt64(1, long.MaxValue);
                        if (moved is PK8 p8) p8.UpdateHandler(Trainer(game));
                        else if (moved is PB8 b8) b8.UpdateHandler(Trainer(game));
                        else if (moved is PK9 k9) k9.UpdateHandler(Trainer(game));
                        else if (moved is PA8 a8) a8.UpdateHandler(Trainer(game));
                        else if (moved is PA9 a9) a9.UpdateHandler(Trainer(game));
                        Emit(moved, game, group, true);
                    }
                }
            }
        }
    }

    private void Native(GameVersion[] versions, int maxSpecies, IPersonalTable table, EntityContext context, Func<GameVersion, PKM> blank, string group, int perForm = int.MaxValue)
    {
        var seen = new HashSet<string>();
        for (ushort species = 1; species <= maxSpecies; species++)
        {
            var count = Math.Max((byte)1, table[species].FormCount);
            for (byte form = 0; form < count; form++)
            {
                foreach (var version in versions)
                {
                    var tr = Trainer(version);
                    var b = blank(version);
                    b.Species = species;
                    b.Form = form;
                    List<IEncounterable> encounters;
                    try { encounters = EncounterMovesetGenerator.GenerateEncounters(b, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                    catch (Exception) { continue; }
                    foreach (var enc in encounters.Where(e => e.Context == context).Take(perForm))
                    {
                        var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.EggLocation}:{enc.LevelMin}:{enc.LongName}";
                        if (!seen.Add(key))
                            continue;
                        if (TryConvert(enc, tr) is { } pk && pk.Context == context)
                            Emit(pk, version, group, true);
                    }
                }
            }
        }
    }

    /// <summary>Gen V encounters through Poke Transporter into PK6, and PK6 natives through Bank into PK7.</summary>
    public void Transfers()
    {
        var seen = new HashSet<string>();
        GameVersion[] versions5 = [GameVersion.B, GameVersion.W, GameVersion.B2, GameVersion.W2];
        for (ushort species = 1; species <= 649; species++)
        {
            if (rng.Next(3) != 0) continue; // a third of the species keeps the corpus size bounded
            foreach (var version in versions5)
            {
                var tr = Trainer(version);
                var b = new PK5 { Species = species, Version = version };
                List<IEncounterable> encounters;
                try { encounters = EncounterMovesetGenerator.GenerateEncounters(b, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                catch (Exception) { continue; }
                foreach (var enc in encounters.Where(e => e.Context == EntityContext.Gen5).Take(4))
                {
                    var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.LevelMin}";
                    if (!seen.Add(key))
                        continue;
                    if (TryConvert(enc, tr) is not PK5 pk5)
                        continue;
                    if (EntityConverter.ConvertToType(pk5, typeof(PK6), out _) is PK6 pk6)
                    {
                        Emit(pk6, Versions6[rng.Next(4)], "transfer6", true);
                        if (EntityConverter.ConvertToType(pk6, typeof(PK7), out _) is PK7 pk7)
                            Emit(pk7, Versions7[rng.Next(4)], "transfer7", true);
                    }
                }
            }
        }
    }

    /// <summary>Gen I/II Virtual Console encounters through Poke Transporter into PK7.</summary>
    public void VirtualConsole()
    {
        var seen = new HashSet<string>();
        foreach (var (maxSpecies, versions, make) in new (int, GameVersion[], Func<PKM>)[]
                 {
                     (151, [GameVersion.RD, GameVersion.GN, GameVersion.BU, GameVersion.YW], () => new PK1()),
                     (251, [GameVersion.GD, GameVersion.SI, GameVersion.C], () => new PK2()),
                 })
        {
            for (ushort species = 1; species <= maxSpecies; species++)
            {
                if (rng.Next(2) != 0) continue;
                foreach (var version in versions)
                {
                    var tr = Trainer(version);
                    var b = make();
                    b.Species = species;
                    List<IEncounterable> encounters;
                    try { encounters = EncounterMovesetGenerator.GenerateEncounters(b, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                    catch (Exception) { continue; }
                    foreach (var enc in encounters.Take(3))
                    {
                        var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Version}:{enc.Location}:{enc.LevelMin}";
                        if (!seen.Add(key))
                            continue;
                        if (TryConvert(enc, tr) is not { } gb)
                            continue;
                        if (EntityConverter.ConvertToType(gb, typeof(PK7), out _) is PK7 pk7)
                            Emit(pk7, Versions7[rng.Next(4)], "vc7", true);
                    }
                }
            }
        }
    }

    /// <summary>Levelled, EV-trained and party copies of the PKHeX-valid records.</summary>
    public void Grown()
    {
        var seeds = grownSeeds.ToList();
        foreach (var (seed, group) in seeds)
        {
            if (rng.Next(3) != 0) continue;
            var pk = seed.Clone();
            var growth = pk.PersonalInfo.EXPGrowth;
            var level = (byte)Math.Min(100, pk.CurrentLevel + rng.Next(0, 60));
            pk.EXP = Experience.GetEXP(level, growth) + (level < 100 ? (uint)rng.Next(0, 20) : 0);
            Span<int> evs = stackalloc int[6];
            int left = 510;
            for (int i = 0; i < 6; i++) { var v = Math.Min(left, rng.Next(0, 253)); evs[i] = v; left -= v; }
            pk.SetEVs(evs);
            if (pk is IHyperTrain ht && level == 100 && rng.Next(2) == 0)
            {
                // IHyperTrain.IsHyperTrained(i) pairs with PKM.GetIV(i): HP Atk Def Spe SpA SpD.
                if (pk.IV_HP != 31) ht.HT_HP = true;
                if (pk.IV_ATK != 31) ht.HT_ATK = true;
                if (pk.IV_DEF != 31) ht.HT_DEF = true;
                if (pk.IV_SPE != 31) ht.HT_SPE = true;
                if (pk.IV_SPA != 31) ht.HT_SPA = true;
                if (pk.IV_SPD != 31) ht.HT_SPD = true;
            }
            if (pk is PK8 p8 && rng.Next(2) == 0)
                p8.DynamaxLevel = (byte)rng.Next(0, 11);
            if (pk is PA8 a8 && rng.Next(2) == 0)
            {
                // Legends: Arceus raises Effort Levels (Ganbaru values) up to the IV's cap.
                a8.GV_HP = (byte)rng.Next(0, a8.GetMaxGanbaru(0) + 1);
                a8.GV_ATK = (byte)rng.Next(0, a8.GetMaxGanbaru(1) + 1);
                a8.GV_SPE = (byte)rng.Next(0, a8.GetMaxGanbaru(3) + 1);
            }
            if (pk is PA9 z9)
            {
                // Legends: Z-A marks each Plus move the record reaches by level.
                z9.SetPlusFlags((IPermitPlus)pk.PersonalInfo, new LegalityAnalysis(pk), false, false);
            }
            if (pk is PB7 b7)
            {
                // Let's Go has no EVs; it raises Awakening Values (never lowered) and recalculates CP.
                pk.SetEVs([0, 0, 0, 0, 0, 0]);
                b7.AwakeningSetRandom(0, AwakeningUtil.AwakeningMax);
                b7.ResetCalculatedValues();
            }
            var party = rng.Next(3) == 0;
            if (party) pk.ResetPartyStats();
            Emit(pk, Game(pk), "grown", false, party);
        }
    }

    private GameVersion Game(PKM pk) => pk.Format switch
    {
        _ when pk is PA8 => GameVersion.PLA,
        _ when pk is PA9 => GameVersion.ZA,
        _ when pk is PB7 => pk.Version is GameVersion.GP or GameVersion.GE ? pk.Version : (rng.Next(2) == 0 ? GameVersion.GP : GameVersion.GE),
        6 => Versions6.Contains(pk.Version) ? pk.Version : Versions6[rng.Next(4)],
        7 => Versions7.Contains(pk.Version) ? pk.Version : Versions7[rng.Next(4)],
        _ when pk is PB8 => Versions8b.Contains(pk.Version) ? pk.Version : Versions8b[rng.Next(2)],
        _ when pk is PK9 => Versions9.Contains(pk.Version) ? pk.Version : Versions9[rng.Next(2)],
        _ => Versions8.Contains(pk.Version) ? pk.Version : Versions8[rng.Next(2)],
    };

    private static PKM? TryConvert(IEncounterable enc, ITrainerInfo tr)
    {
        try { return enc.ConvertToPKM(tr); }
        catch (Exception) { return null; }
    }

    private void Emit(PKM pk, GameVersion game, string group, bool withMutations, bool party = false)
    {
        // A Let's Go record at rest in a box has the initial spirit and mood (the converter leaves them 0).
        if (pk is PB7 p7 && group == "native7b") p7.ResetSpiritMood();
        pk.RefreshChecksum();
        var valid = Print($"b{count++}", pk, game, group, party);
        if (valid && group != "grown" && !pk.IsEgg)
            grownSeeds.Add((pk.Clone(), group));
        if (!withMutations)
            return;
        for (int m = 0; m < mutations; m++)
        {
            var copy = pk.Clone();
            var name = Mutate(copy);
            copy.RefreshChecksum();
            Print($"m{count++}-{name}", copy, game, group, party);
        }
    }

    private static bool Print(string id, PKM pk, GameVersion game, string group, bool party)
    {
        var la = new LegalityAnalysis(pk, party ? StorageSlotType.Party : StorageSlotType.Box);
        var enc = la.EncounterMatch;
        var invalid = la.Results.Where(r => !r.Valid || r.Judgement == Severity.Fishy).Select(r => $"{(r.Judgement == Severity.Fishy ? "~" : "")}{r.Identifier}:{r.Result}")
            .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid).Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
        var bytes = pk.Data[..(party ? pk.SIZE_PARTY : pk.SIZE_STORED)].ToArray();
        Console.WriteLine($"{id} {(pk is PB8 ? "8b" : pk is PB7 ? "7b" : pk is PA8 ? "8a" : pk is PA9 ? "9a" : pk.Format.ToString())} {(int)game} {(party ? "party" : "box")} {Convert.ToHexString(bytes)} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species} {group} ; {string.Join(' ', invalid)}");
        return la.Valid;
    }

    private string Mutate(PKM pk)
    {
        if (pk is G8PKM g8 && rng.Next(3) == 0)
            return Mutate8(g8);
        if (pk is PK9 k9 && rng.Next(3) == 0)
            return Mutate9(k9);
        if (pk is PB7 b7 && rng.Next(3) == 0)
            return Mutate7b(b7);
        if (pk is PA8 a8 && rng.Next(3) == 0)
            return Mutate8a(a8);
        if (pk is PA9 a9 && rng.Next(3) == 0)
            return Mutate9a(a9);
        var maxSpecies = pk.Format switch { 6 => 721, 7 => pk is PB7 ? 809 : 807, 9 => pk is PA9 ? 1000 : 1025, _ => pk is PB8 ? 493 : pk is PA8 ? 905 : 898 };
        var maxMove = pk.Format switch { 6 => 621, 7 => pk is PB7 ? 742 : 728, 9 => 919, _ => pk is PA8 ? 850 : 826 };
        switch (rng.Next(32))
        {
            case 0: pk.Ball = (byte)rng.Next(0, 28); return "ball";
            case 1: pk.MetLevel = (byte)Math.Clamp(pk.MetLevel + rng.Next(-3, 4), 0, 100); return "metlevel";
            case 2: pk.AbilityNumber = new[] { 0, 1, 2, 3, 4, 5, 6, 8 }[rng.Next(8)]; return "abilitynumber";
            case 3: pk.Ability = rng.Next(0, pk.Format switch { 6 => 192, 7 => 234, _ => 268 }); return "ability";
            case 4: pk.Gender = (byte)rng.Next(3); return "gender";
            case 5: pk.Nature = (Nature)rng.Next(26); return "nature";
            case 6: pk.PID = (uint)rng.NextInt64(0, 1L << 32); return "pid";
            case 7: pk.PID = pk.EncryptionConstant; return "pid=ec";
            case 8: pk.EncryptionConstant = (uint)rng.NextInt64(0, 1L << 32); return "ec";
            case 9: pk.SetMove(rng.Next(4), (ushort)rng.Next(0, maxMove + 4)); return "move";
            case 10: pk.SetMove(rng.Next(1, 4), pk.Move1); return "dupmove";
            case 11: pk.SetEV(rng.Next(6), rng.Next(256)); return "ev";
            case 12: pk.SetIV(rng.Next(6), rng.Next(32)); return "iv";
            case 13: pk.Version = (GameVersion)new[] { 0, 1, 3, 6, 7, 10, 15, 20, 23, 24, 26, 27, 28, 30, 32, 33, 34, 35, 37, 39, 41, 42, 47, 44, 45, 46, 48, 50, 52, 53, 56 }[rng.Next(pk.Format >= 8 ? 31 : 23)]; return "version";
            case 14: pk.Form = (byte)rng.Next(0, 33); return "form";
            case 15: pk.Language = rng.Next(0, 12); return "language";
            case 16: pk.HeldItem = rng.Next(0, pk.Format switch { 6 => 780, 7 => 965, 9 => 2560, _ => 1612 }); return "item";
            case 17: pk.EXP = (uint)Math.Max(0, (long)pk.EXP + rng.Next(-5000, 5000)); return "exp";
            case 18: pk.EXP = Experience.GetEXP(100, pk.PersonalInfo.EXPGrowth) + (uint)rng.Next(1, 100); return "exp100+";
            case 19: pk.Move1_PPUps = rng.Next(0, 4); pk.Move1_PP = rng.Next(0, 70); return "pp";
            case 20: pk.Move2_PP = pk.GetMovePP(pk.Move2, 3) + rng.Next(1, 5); return "pphigh";
            case 21: pk.Species = (ushort)Math.Clamp(pk.Species + (rng.Next(2) == 0 ? -1 : 1), 1, maxSpecies + 2); return "species";
            case 22: pk.Species = (ushort)rng.Next(1, maxSpecies + 5); return "species-any";
            case 23: if (pk is IHyperTrain ht) { ht.HyperTrainFlags = (byte)rng.Next(1, 64); return "hypertrain"; } return "none";
            case 24: pk.FatefulEncounter = !pk.FatefulEncounter; return "fateful";
            case 25: pk.SetMove(0, 0); return "emptyfirst";
            case 26: pk.SetMove(rng.Next(0, 3), 0); return "emptygap";
            case 27: pk.HeldItem = new[] { 112, 298, 303, 116, 117, 904, 920, 778, 790 }[rng.Next(9)]; return "formitem";
            case 28: pk.IsEgg = !pk.IsEgg; return "egg";
            case 29: if (pk is IRegionOrigin r) { r.ConsoleRegion = (byte)rng.Next(0, 8); return "consoleregion"; } return "none";
            case 30: pk.CurrentLevel = (byte)Math.Max(1, pk.MetLevel - rng.Next(1, 5)); return "levelbelowmet";
            default: pk.SetEVs([252, 252, 6, 0, 0, 1]); return "ev511";
        }
    }

    /// <summary>Single-field mutations of the PA9-only fields.</summary>
    private string Mutate9a(PA9 pk)
    {
        switch (rng.Next(12))
        {
            case 0: pk.IsAlpha = !pk.IsAlpha; return "alpha9a";
            case 1: pk.LevelBoost = (byte)rng.Next(0, 4); return "levelboost";
            case 2: pk.SetMovePlusFlag(rng.Next(0, 360), true); return "plusflag";
            case 3: pk.Move1_PP = rng.Next(0, 40); return "pp9a";
            case 4: pk.Move2_PPUps = rng.Next(0, 4); return "ppups9a";
            case 5: pk.ObedienceLevel = (byte)Math.Clamp(pk.ObedienceLevel + rng.Next(-3, 4), 0, 101); return "obedience9a";
            case 6: pk.BattleVersion = (GameVersion)new[] { 0, 0, 44, 45, 50, 51 }[rng.Next(6)]; return "battleversion9a";
            case 7: pk.StatAlignment = (Nature)rng.Next(0, 26); return "statnature9a";
            case 8: pk.IsAlpha = true; pk.Tracker = (ulong)rng.NextInt64(0, 3); pk.HeightScalar = (byte)rng.Next(250, 256); return "alphascalar";
            case 9: pk.Language = rng.Next(9, 13); return "language9a";
            case 10: pk.Ball = (byte)rng.Next(24, 40); return "ball9a";
            default: pk.HyperTrainFlags = (byte)rng.Next(1, 64); return "hypertrain9a";
        }
    }

    /// <summary>Single-field mutations of the PA8-only fields.</summary>
    private string Mutate8a(PA8 pk)
    {
        switch (rng.Next(16))
        {
            case 0: pk.IsAlpha = !pk.IsAlpha; return "alpha";
            case 1: pk.IsNoble = true; return "noble";
            case 2: pk.AlphaMove = (ushort)pk.Permit.RecordPermitIndexes[rng.Next(pk.Permit.RecordCountUsed)]; return "alphamove";
            case 3: pk.SetPurchasedRecordFlag(rng.Next(pk.Permit.RecordCountUsed), true); return "purchased";
            case 4: pk.SetMasteredRecordFlag(rng.Next(pk.Permit.RecordCountUsed), true); return "mastered";
            case 5: pk.GV_DEF = (byte)rng.Next(0, 11); return "gv";
            case 6: pk.Scale = (byte)rng.Next(256); return "scale8a";
            case 7: pk.HeightScalar = (byte)rng.Next(256); return "heightscalar8a";
            case 8: pk.HeightAbsolute += rng.Next(2) == 0 ? 0.5f : -0.001f; return "height8a";
            case 9: pk.WeightAbsolute *= 1.01f; return "weight8a";
            case 10: pk.Sociability = (uint)rng.Next(0, 300); return "sociability8a";
            case 11: pk.DynamaxLevel = (byte)rng.Next(0, 11); return "dynamax8a";
            case 12: pk.SetMoveRecordFlag(rng.Next(0, 100), true); return "trflag8a";
            case 13: pk.Fullness = (byte)rng.Next(0, 256); return "fullness8a";
            case 14: pk.BattleVersion = (GameVersion)new[] { 0, 0, 44, 45, 43, 50 }[rng.Next(6)]; return "battleversion8a";
            default: pk.StatAlignment = (Nature)rng.Next(0, 26); return "statnature8a";
        }
    }

    /// <summary>Single-field mutations of the PB7-only fields.</summary>
    private string Mutate7b(PB7 pk)
    {
        switch (rng.Next(12))
        {
            case 0: pk.AV_ATK = (byte)rng.Next(150, 256); return "av";
            case 1: pk.Stat_CP = Math.Max(0, pk.Stat_CP + rng.Next(-3, 4)); return "cp";
            case 2: pk.Spirit = (byte)rng.Next(90, 111); return "spirit";
            case 3: pk.Mood = (byte)rng.Next(90, 111); return "mood";
            case 4: pk.ReceivedHour = (byte)rng.Next(20, 30); return "receivedtime";
            case 5: pk.ReceivedDay = (byte)rng.Next(0, 33); return "receivedday";
            case 6: pk.HeightAbsolute += rng.Next(2) == 0 ? 0.5f : -0.001f; return "height";
            case 7: pk.WeightAbsolute *= 1.01f; return "weight";
            case 8: pk.HeightScalar = (byte)rng.Next(256); return "heightscalar";
            case 9: pk.EV_SPA = rng.Next(1, 10); return "ev7b";
            case 10: pk.HyperTrainFlags = (byte)rng.Next(1, 64); pk.CurrentLevel = (byte)rng.Next(Math.Max(1, (int)pk.MetLevel), 101); return "hypertrain7b";
            default: pk.MetLocation = (ushort)new[] { 50, 3, 40001, 30001 }[rng.Next(4)]; pk.Version = (GameVersion)new[] { 34, 42, 43, 30 }[rng.Next(4)]; return "origin7b";
        }
    }

    /// <summary>Single-field mutations of the PK9-only fields.</summary>
    private string Mutate9(PK9 pk)
    {
        switch (rng.Next(12))
        {
            case 0: pk.TeraTypeOriginal = (MoveType)new[] { 0, 5, 17, 18, 19, 20, 99, 100 }[rng.Next(8)]; return "teraoriginal";
            case 1: pk.TeraTypeOverride = (MoveType)new[] { 0, 9, 10, 11, 17, 18, 19, 99 }[rng.Next(8)]; return "teraoverride";
            case 2: pk.ObedienceLevel = (byte)Math.Clamp(pk.ObedienceLevel + rng.Next(-3, 4), 0, 101); return "obedience";
            case 3: pk.StatAlignment = (Nature)rng.Next(0, 26); return "statnature";
            case 4: pk.BattleVersion = (GameVersion)new[] { 0, 0, 44, 45, 50, 51 }[rng.Next(6)]; return "battleversion";
            case 5: pk.SetMoveRecordFlag(rng.Next(0, pk.Permit.RecordCountUsed), true); return "tmflag";
            case 6: pk.HeightScalar = 0; pk.WeightScalar = 0; return "scale0";
            case 7: pk.Scale = (byte)rng.Next(256); return "scale";
            case 8: pk.Ball = (byte)rng.Next(24, 40); return "ball9";
            case 9: pk.HyperTrainFlags = (byte)rng.Next(1, 64); pk.CurrentLevel = (byte)rng.Next(Math.Max(1, (int)pk.MetLevel), 101); return "hypertrain9";
            case 10: pk.IsEgg = true; pk.TeraTypeOverride = (MoveType)rng.Next(0, 18); return "eggtera";
            default: pk.Version = 0; pk.IsEgg = rng.Next(2) == 0; return "version0";
        }
    }

    /// <summary>Single-field mutations of the PK8/PB8-only fields.</summary>
    private string Mutate8(G8PKM pk)
    {
        switch (rng.Next(pk is PB8 ? 14 : 12))
        {
            case 12: ((PB8)pk).IsDprIllegal = true; return "dprillegal";
            case 13: pk.Tracker = 0; pk.Species = 327; return "spinda";
            case 0: pk.DynamaxLevel = (byte)rng.Next(0, 13); return "dynamax";
            case 1: pk.StatAlignment = (Nature)rng.Next(0, 26); return "statnature";
            case 2: pk.BattleVersion = (GameVersion)new[] { 0, 0, 44, 45, 43, 50 }[rng.Next(6)]; return "battleversion";
            case 3: pk.SetMoveRecordFlag(rng.Next(0, 100), true); return "trflag";
            case 4: pk.Sociability = (uint)rng.Next(0, 300); return "sociability";
            case 5: pk.CanGigantamax = !pk.CanGigantamax; return "gigantamax";
            case 6: pk.MetLocation = (ushort)new[] { 59994, 59995, 59996, 59997, 59998, 59999, 60000, 30001, 30007, 30012 }[rng.Next(10)]; return "sidemet";
            case 7: pk.HeightScalar = 0; pk.WeightScalar = 0; return "scale0";
            case 8: pk.Form = (byte)rng.Next(0, 4); return "form8";
            case 9: pk.Ball = (byte)rng.Next(24, 38); return "ball8";
            case 10: pk.HyperTrainFlags = (byte)rng.Next(1, 64); pk.CurrentLevel = (byte)rng.Next(Math.Max(1, (int)pk.MetLevel), 101); return "hypertrain8";
            default: pk.IsEgg = true; pk.DynamaxLevel = (byte)rng.Next(1, 4); return "eggdynamax";
        }
    }
}
