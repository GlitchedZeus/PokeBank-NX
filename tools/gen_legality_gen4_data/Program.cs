// Generates include/Legality/Gen4LegalityData.h and src/Legality/Gen4LegalityData.cpp from a
// pinned PKHeX.Core build.
//
// PKHeX.Core is the reference legality implementation the project measures itself against
// (docs/legality/PKHEX_FIRST_REUSE_POLICY.md). Its Gen IV encounter tables are built at runtime
// from binary resources plus C# initializers (statics, trades, Pokewalker courses, Ranch gifts,
// PCD Mystery Gifts), so this tool loads PKHeX.Core and dumps the exact runtime objects instead of
// re-parsing C# source. Raw binary resources (personal, learnsets, egg moves, evolutions) are
// emitted byte-for-byte; their SHA-256 values are recorded so a regenerated table can be traced.
//
// PKHeX is GPL-3.0; PokeBank NX is AGPL-3.0, which GPL-3.0 section 13 permits combining with.
//
// Usage: dotnet run -- PKHEX_ROOT OUT_HEADER OUT_SOURCE
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using PKHeX.Core;

internal static class Program
{
    private const string Pin = "6501f0ab46e8f8ca048539dbaf8cae8cb104e722";
    private const BindingFlags Any = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static | BindingFlags.Instance;
    private static readonly Assembly Core = typeof(PKM).Assembly;
    private static string PkhexRoot = "";
    private static readonly List<(string Path, string Sha)> Pins = [];

    private static int Main(string[] args)
    {
        if (args.Length != 3)
        {
            Console.Error.WriteLine("usage: Gen4DataDump PKHEX_ROOT OUT_HEADER OUT_SOURCE");
            return 2;
        }
        PkhexRoot = args[0];
        var h = new StringBuilder();
        var c = new StringBuilder();
        Emit(h, c);
        File.WriteAllText(args[1], h.ToString().Replace("\r\n", "\n"));
        File.WriteAllText(args[2], c.ToString().Replace("\r\n", "\n"));
        return 0;
    }

    // ------------------------------------------------------------------ reflection helpers
    private static Type T(string name) => Core.GetType("PKHeX.Core." + name) ?? throw new Exception($"type {name}");
    private static object SF(string type, string name)
    {
        var t = T(type);
        var f = t.GetField(name, Any);
        if (f is not null)
            return f.GetValue(null)!;
        var p = t.GetProperty(name, Any) ?? throw new Exception($"{type}.{name}");
        return p.GetValue(null)!;
    }
    private static object? Call(string type, string name, params object[] a)
    {
        var m = T(type).GetMethods(Any).First(x => x.Name == name && x.GetParameters().Length == a.Length
            && x.GetParameters().Select(p => p.ParameterType).Zip(a.Select(v => v.GetType())).All(z => z.First == z.Second || z.First.IsAssignableFrom(z.Second)));
        return m.Invoke(null, a);
    }
    private static object? IF(object o, string name)
    {
        for (var t = o.GetType(); t is not null; t = t.BaseType)
        {
            var f = t.GetField(name, Any);
            if (f is not null)
                return f.GetValue(o);
            var p = t.GetProperty(name, Any);
            if (p is not null)
                return p.GetValue(o);
        }
        throw new Exception($"{o.GetType().Name}.{name}");
    }

    private static byte[] Resource(string name, string relativePath)
    {
        var data = Util.GetBinaryResource(name);
        var path = Path.Combine(PkhexRoot, relativePath);
        var disk = File.ReadAllBytes(path);
        if (!disk.AsSpan().SequenceEqual(data))
            throw new Exception($"{relativePath} differs from the embedded resource {name}");
        Pins.Add((relativePath, Convert.ToHexStringLower(SHA256.HashData(disk))));
        return data;
    }

    private static string PinText(string relativePath)
    {
        var path = Path.Combine(PkhexRoot, relativePath);
        var disk = File.ReadAllBytes(path);
        Pins.Add((relativePath, Convert.ToHexStringLower(SHA256.HashData(disk))));
        return Encoding.UTF8.GetString(disk);
    }

    private static List<byte[]> BinLinker16(byte[] data, string ident)
    {
        if (data[0] != ident[0] || data[1] != ident[1])
            throw new Exception($"bad ident {ident}");
        int count = BitConverter.ToUInt16(data, 2);
        var result = new List<byte[]>(count);
        for (int i = 0; i < count; i++)
        {
            int start = BitConverter.ToUInt16(data, 4 + (i * 2));
            int end = BitConverter.ToUInt16(data, 6 + (i * 2));
            result.Add(data[start..end]);
        }
        return result;
    }

    // ------------------------------------------------------------------ emit helpers
    private static string Arr<TV>(IEnumerable<TV> values, int width = 16)
    {
        var list = values.Select(v => v!.ToString()!).ToList();
        var sb = new StringBuilder();
        for (int i = 0; i < list.Count; i += width)
            sb.Append("    ").Append(string.Join(", ", list.Skip(i).Take(width))).Append(",\n");
        return sb.ToString();
    }
    private static string Hex(IEnumerable<byte> data, int width = 24) => Arr(data.Select(b => $"0x{b:X2}"), width);

    private static string U16Lit(string s)
    {
        // \xHHHH escapes (valid for surrogates too); split the literal when a hex digit follows one.
        var sb = new StringBuilder("u\"");
        bool afterHex = false, split = false;
        foreach (var ch in s)
        {
            if (ch is >= ' ' and < (char)0x7F && ch is not '"' and not '\\' and not '?')
            {
                if (afterHex && Uri.IsHexDigit(ch))
                {
                    sb.Append("\" u\"");
                    split = true;
                }
                sb.Append(ch);
                afterHex = false;
            }
            else
            {
                sb.Append($"\\x{(int)ch:X4}");
                afterHex = true;
            }
        }
        sb.Append('"');
        return split ? $"({sb})" : sb.ToString(); // parenthesized: clang -Wstring-concatenation
    }

    private static void Flatten<TV>(IReadOnlyList<IReadOnlyList<TV>> lists, out List<TV> flat, out List<int> starts)
    {
        flat = [];
        starts = [0];
        foreach (var l in lists)
        {
            flat.AddRange(l);
            starts.Add(flat.Count);
        }
    }

    // ------------------------------------------------------------------ main emission
    private static void Emit(StringBuilder h, StringBuilder c)
    {
        const string header = "// GENERATED by tools/gen_legality_gen4_data -- do not edit by hand.\n"
            + "// Source: kwsch/PKHeX@" + Pin + " (PKHeX.Core legality resources and runtime tables, GPL-3.0).\n"
            + "// Used under GPL-3.0 section 13 alongside this AGPL-3.0 project.\n";
        h.Append(header).Append('\n');
        h.Append("#pragma once\n\n#include <cstddef>\n#include <cstdint>\n\nnamespace Legality::Gen4Data {\n\n");
        c.Append(header).Append('\n');
        c.Append("#include \"Legality/Gen4LegalityData.h\"\n\nnamespace Legality::Gen4Data {\n\n");

        EmitPersonal(h, c);
        EmitLearnsets(h, c);
        EmitEvolutions(h, c);
        EmitBreeding(h, c);
        EmitEncounters(h, c);
        EmitTrades(h, c);
        EmitGifts(h, c);
        EmitMoves(h, c);
        EmitText(h, c);
        EmitWordFilter(h, c);
        EmitGen3(h, c);

        h.Append("/// SHA-256 of every PKHeX input file, so a regenerated table can be traced to its source.\n");
        h.Append("struct SourcePin { const char* path; const char* sha256; };\n");
        h.Append($"inline constexpr std::size_t kSourcePinCount = {Pins.Count};\n");
        h.Append("extern const SourcePin kSourcePins[];\n");
        h.Append($"inline constexpr const char* kPkhexCommit = \"{Pin}\";\n\n");
        h.Append("} // namespace Legality::Gen4Data\n");
        c.Append("const SourcePin kSourcePins[] = {\n");
        foreach (var (p, s) in Pins)
            c.Append($"    {{\"{p}\", \"{s}\"}},\n");
        c.Append("};\n\n} // namespace Legality::Gen4Data\n");
    }

    private static void EmitPersonal(StringBuilder h, StringBuilder c)
    {
        var dp = Resource("personal_dp", "PKHeX.Core/Resources/byte/personal/personal_dp");
        var pt = Resource("personal_pt", "PKHeX.Core/Resources/byte/personal/personal_pt");
        var hgss = Resource("personal_hgss", "PKHeX.Core/Resources/byte/personal/personal_hgss");
        Resource("tutors_g4.pkl", "PKHeX.Core/Resources/byte/personal/tutors_g4.pkl");
        h.Append("/// Raw PersonalInfo4 records (0x2C bytes each); PKHeX PersonalTable.DP / Pt / HGSS.\n");
        h.Append("inline constexpr std::size_t kPersonalSize = 0x2C;\n");
        h.Append($"inline constexpr std::size_t kPersonalDPCount = {dp.Length / 0x2C};\n");
        h.Append($"inline constexpr std::size_t kPersonalPtCount = {pt.Length / 0x2C};\n");
        h.Append($"inline constexpr std::size_t kPersonalHGSSCount = {hgss.Length / 0x2C};\n");
        h.Append("extern const uint8_t kPersonalDP[]; extern const uint8_t kPersonalPt[]; extern const uint8_t kPersonalHGSS[];\n");
        h.Append("/// PersonalInfo4.TypeTutors bits (LSB first, PersonalInfo4.TutorMoves order), loaded only into\n");
        h.Append("/// PersonalTable.HGSS by PKHeX (tutors_g4.pkl); 7 bytes per HGSS personal record.\n");
        h.Append("extern const uint8_t kTutorBitsHGSS[];\n\n");
        c.Append("const uint8_t kPersonalDP[] = {\n").Append(Hex(dp)).Append("};\n");
        c.Append("const uint8_t kPersonalPt[] = {\n").Append(Hex(pt)).Append("};\n");
        c.Append("const uint8_t kPersonalHGSS[] = {\n").Append(Hex(hgss)).Append("};\n");
        var bits = new List<byte>();
        var table = PersonalTable.HGSS;
        for (int i = 0; i < table.Count; i++)
        {
            var t = table[i].TypeTutors;
            var b = new byte[7];
            for (int k = 0; k < t.Length && k < 56; k++)
            {
                if (t[k])
                    b[k >> 3] |= (byte)(1 << (k & 7));
            }
            bits.AddRange(b);
        }
        c.Append("const uint8_t kTutorBitsHGSS[] = {\n").Append(Hex(bits, 21)).Append("};\n\n");
    }

    private static void EmitLearnsets(StringBuilder h, StringBuilder c)
    {
        h.Append("struct LevelMove { uint16_t move; uint8_t level; };\n");
        void Learn(string name, string res, string path, string ident)
        {
            var entries = BinLinker16(Resource(res, path), ident);
            var lists = new List<IReadOnlyList<string>>();
            foreach (var e in entries)
            {
                int count = e.Length / 3;
                var l = new List<string>();
                for (int i = 0; i < count; i++)
                    l.Add($"{{{BitConverter.ToUInt16(e, i * 2)}, {e[(count * 2) + i]}}}");
                lists.Add(l);
            }
            Flatten(lists, out var flat, out var starts);
            h.Append($"inline constexpr std::size_t kLevelUp{name}Count = {entries.Count};\n");
            h.Append($"extern const LevelMove kLevelUp{name}[]; extern const uint16_t kLevelUp{name}Start[];\n");
            c.Append($"const LevelMove kLevelUp{name}[] = {{\n").Append(Arr(flat, 8)).Append("};\n");
            c.Append($"const uint16_t kLevelUp{name}Start[] = {{\n").Append(Arr(starts)).Append("};\n");
        }
        void Egg(string name, string res, string path, string ident)
        {
            var entries = BinLinker16(Resource(res, path), ident);
            var lists = entries.Select(e => (IReadOnlyList<int>)Enumerable.Range(0, e.Length / 2).Select(i => (int)BitConverter.ToUInt16(e, i * 2)).ToList()).ToList();
            Flatten(lists, out var flat, out var starts);
            h.Append($"inline constexpr std::size_t kEggMoves{name}Count = {entries.Count};\n");
            h.Append($"extern const uint16_t kEggMoves{name}[]; extern const uint16_t kEggMoves{name}Start[];\n");
            c.Append($"const uint16_t kEggMoves{name}[] = {{\n").Append(Arr(flat)).Append("};\n");
            c.Append($"const uint16_t kEggMoves{name}Start[] = {{\n").Append(Arr(starts)).Append("};\n");
        }
        h.Append("/// Level-up learnsets indexed by each game's PersonalTable4.GetFormIndex (lvlmove_dp/pt/hgss.pkl).\n");
        Learn("DP", "lvlmove_dp.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_dp.pkl", "dp");
        Learn("Pt", "lvlmove_pt.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_pt.pkl", "pt");
        Learn("HGSS", "lvlmove_hgss.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_hgss.pkl", "hs");
        h.Append("/// Egg moves indexed by species (eggmove_dppt.pkl shared by D/P/Pt, eggmove_hgss.pkl).\n");
        Egg("DPPt", "eggmove_dppt.pkl", "PKHeX.Core/Resources/byte/eggmove/eggmove_dppt.pkl", "dp");
        Egg("HGSS", "eggmove_hgss.pkl", "PKHeX.Core/Resources/byte/eggmove/eggmove_hgss.pkl", "hs");
        h.Append('\n');
        c.Append('\n');
    }

    private static void EmitEvolutions(StringBuilder h, StringBuilder c)
    {
        var entries = BinLinker16(Resource("evos_g4.pkl", "PKHeX.Core/Resources/byte/evolve/evos_g4.pkl"), "g4");
        var lists = new List<IReadOnlyList<string>>();
        foreach (var e in entries)
        {
            var l = new List<string>();
            for (int off = 0; off + 8 <= e.Length; off += 8)
            {
                var type = (EvolutionType)e[off];
                var arg = BitConverter.ToUInt16(e, off + 2);
                var species = BitConverter.ToUInt16(e, off + 4);
                var form = e[off + 6];
                var level = e[off + 7];
                var lvlup = type.IsLevelUpRequired ? 1 : 0;
                l.Add($"{{{(int)type}, {arg}, {species}, {form}, {level}, {lvlup}}}");
            }
            lists.Add(l);
        }
        Flatten(lists, out var flat, out var starts);
        h.Append("/// PKHeX EvolutionSet methods per species (evos_g4.pkl); levelUp is EvolutionType.IsLevelUpRequired.\n");
        h.Append("struct Evolution { uint8_t type; uint16_t argument; uint16_t species; uint8_t form; uint8_t level; uint8_t levelUp; };\n");
        h.Append($"inline constexpr std::size_t kEvolutionCount = {entries.Count};\n");
        h.Append("extern const Evolution kEvolutions[]; extern const uint16_t kEvolutionStart[];\n\n");
        c.Append("const Evolution kEvolutions[] = {\n").Append(Arr(flat, 6)).Append("};\n");
        c.Append("const uint16_t kEvolutionStart[] = {\n").Append(Arr(starts)).Append("};\n\n");
    }

    private static void EmitBreeding(StringBuilder h, StringBuilder c)
    {
        // Per species 0..493: baby species (EvolutionTree.Evolves4.GetBaseSpeciesForm, form 0) and flags.
        var tree = EvolutionTree.Evolves4;
        var rows = new List<string>();
        for (ushort s = 0; s <= 493; s++)
        {
            var (bs, bf) = s == 0 ? ((ushort)0, (byte)0) : tree.GetBaseSpeciesForm(s, 0);
            int flags = 0;
            if (s != 0 && Breeding.CanHatchAsEgg(s)) flags |= 1;
            if (s != 0 && Breeding.IsSplitBreedNotBabySpecies4(s)) flags |= 2;
            if (s != 0 && (bool)Call("Breeding", "GetCanInheritMoves", s)!) flags |= 4;
            if (s != 0 && Breeding.IsGenderSpeciesDetermination(s)) flags |= 8;
            if (s != 0 && Breeding.IsSplitBreedNotBabySpecies3(s)) flags |= 16;
            rows.Add($"{{{bs}, {bf}, {flags}}}");
        }
        h.Append("/// Per species: EvolutionTree.Evolves4.GetBaseSpeciesForm(species, 0) and Breeding flags\n");
        h.Append("/// (1 CanHatchAsEgg, 2 IsSplitBreedNotBabySpecies4, 4 GetCanInheritMoves,\n");
        h.Append("/// 8 IsGenderSpeciesDetermination, 16 IsSplitBreedNotBabySpecies3).\n");
        h.Append("struct Breed { uint16_t baby; uint8_t babyForm; uint8_t flags; };\n");
        h.Append("extern const Breed kBreed[494];\n\n");
        c.Append("const Breed kBreed[494] = {\n").Append(Arr(rows, 8)).Append("};\n\n");
        // Base species/form for every (species, form) a Gen IV entity can have: forms beyond 0 matter only
        // for species whose base form differs (e.g. Burmy -> Wormadam keeps form).
        var formRows = new List<string>();
        var hg = PersonalTable.HGSS;
        for (ushort s = 1; s <= 493; s++)
        {
            int fc = Math.Max((int)hg[s].FormCount, s switch { 201 => 28, 351 => 4, 412 or 413 => 3, 422 or 423 => 2, 493 => 18, _ => 1 });
            for (byte f = 1; f < fc; f++)
            {
                var (bs, bf) = tree.GetBaseSpeciesForm(s, f);
                formRows.Add($"{{{s}, {f}, {bs}, {bf}}}");
            }
        }
        h.Append("struct BaseForm { uint16_t species; uint8_t form; uint16_t baby; uint8_t babyForm; };\n");
        h.Append($"inline constexpr std::size_t kBaseFormCount = {formRows.Count};\n");
        h.Append("extern const BaseForm kBaseForms[];\n\n");
        c.Append("const BaseForm kBaseForms[] = {\n").Append(Arr(formRows, 8)).Append("};\n\n");
    }

    private static int Ability(AbilityPermission a) => (int)(sbyte)a;

    private static void EmitEncounters(StringBuilder h, StringBuilder c)
    {
        Resource("encounter_d.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_d.pkl");
        Resource("encounter_p.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_p.pkl");
        Resource("encounter_pt.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_pt.pkl");
        Resource("encounter_hg.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_hg.pkl");
        Resource("encounter_ss.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_ss.pkl");
        Resource("encounter_walker4.pkl", "PKHeX.Core/Resources/legality/wild/Gen4/encounter_walker4.pkl");
        PinText("PKHeX.Core/Legality/Encounters/Data/Gen4/Encounters4DPPt.cs");
        PinText("PKHeX.Core/Legality/Encounters/Data/Gen4/Encounters4HGSS.cs");

        var areaRows = new List<string>();
        var slotRows = new List<string>();
        foreach (var (type, field) in new[] { ("Encounters4DPPt", "SlotsD"), ("Encounters4DPPt", "SlotsP"), ("Encounters4DPPt", "SlotsPt"), ("Encounters4HGSS", "SlotsHG"), ("Encounters4HGSS", "SlotsSS") })
        {
            var areas = (EncounterArea4[])SF(type, field);
            foreach (var a in areas)
            {
                areaRows.Add($"{{{(int)a.Version}, {a.Location}, {(int)a.Type}, {a.Rate}, {(uint)a.GroundTile}u, {slotRows.Count}, {a.Slots.Length}}}");
                foreach (var s in a.Slots)
                    slotRows.Add($"{{{s.Species}, {s.Form}, {s.LevelMin}, {s.LevelMax}, {s.SlotNumber}, {s.MagnetPullIndex}, {s.MagnetPullCount}, {s.StaticIndex}, {s.StaticCount}}}");
            }
        }
        h.Append("/// GameVersion values (PK4 origin game byte).\n");
        h.Append("inline constexpr uint8_t kHG = 7, kSS = 8, kD = 10, kP = 11, kPt = 12;\n");
        h.Append($"inline constexpr uint8_t kVerDP = {(int)GameVersion.DP}, kVerDPPt = {(int)GameVersion.DPPt}, kVerHGSS = {(int)GameVersion.HGSS}, kVerGen4 = {(int)GameVersion.Gen4};\n");
        h.Append("/// EncounterArea4 in PKHeX order: D, P, Pt, HG, SS (Encounters4DPPt.Slots*, Encounters4HGSS.Slots*).\n");
        h.Append("/// type is SlotType4; groundTile is GroundTileAllowed flags.\n");
        h.Append("struct Area4 { uint8_t version; uint8_t location; uint8_t type; uint8_t rate; uint32_t groundTile; uint16_t slotStart; uint16_t slotCount; };\n");
        h.Append("struct Slot4 { uint16_t species; uint8_t form; uint8_t levelMin; uint8_t levelMax; uint8_t slotNumber;\n");
        h.Append("               uint8_t magnetPullIndex; uint8_t magnetPullCount; uint8_t staticIndex; uint8_t staticCount; };\n");
        h.Append($"inline constexpr std::size_t kAreaCount = {areaRows.Count};\n");
        h.Append($"inline constexpr std::size_t kSlotCount = {slotRows.Count};\n");
        h.Append("extern const Area4 kAreas[]; extern const Slot4 kSlots[];\n\n");
        c.Append("const Area4 kAreas[] = {\n").Append(Arr(areaRows, 4)).Append("};\n");
        c.Append("const Slot4 kSlots[] = {\n").Append(Arr(slotRows, 4)).Append("};\n\n");

        // Statics: each PKHeX list in enumeration order, tagged with its list id.
        var lists = new[] {
            ("Encounters4DPPt", "StaticD"), ("Encounters4DPPt", "StaticP"), ("Encounters4DPPt", "StaticDP"),
            ("Encounters4DPPt", "StaticPt"), ("Encounters4DPPt", "StaticDPPt"),
            ("Encounters4HGSS", "StaticHG"), ("Encounters4HGSS", "StaticSS"), ("Encounters4HGSS", "Encounter_HGSS"),
        };
        var staticRows = new List<string>();
        var listStart = new List<int>();
        foreach (var (type, field) in lists)
        {
            listStart.Add(staticRows.Count);
            foreach (var s in (EncounterStatic4[])SF(type, field))
            {
                var m = s.Moves;
                staticRows.Add($"{{{(int)s.Version}, {s.Species}, {s.Form}, {s.Level}, {s.Location}, {s.EggLocation}, {Ability(s.Ability)}, {(int)s.Shiny}, {s.Gender}, {(int)s.FixedBall}, {(s.FatefulEncounter ? 1 : 0)}, {(s.IsRoaming ? 1 : 0)}, {(uint)s.GroundTile}u, {s.HeldItem}, {(int)s.Nature}, {{{m.Move1}, {m.Move2}, {m.Move3}, {m.Move4}}}}}");
            }
        }
        listStart.Add(staticRows.Count);
        h.Append("/// EncounterStatic4 lists in PKHeX order: StaticD, StaticP, StaticDP, StaticPt, StaticDPPt,\n");
        h.Append("/// StaticHG, StaticSS, Encounter_HGSS. kStaticListStart[i]..[i+1] bounds list i.\n");
        h.Append("/// ability is AbilityPermission, shiny is Shiny (0 random, 1 never, 2 always), gender 0xFF random,\n");
        h.Append("/// ball is Ball (0 none), nature 25 random.\n");
        h.Append("struct Static4 { uint8_t version; uint16_t species; uint8_t form; uint8_t level; uint8_t location; uint16_t eggLocation;\n");
        h.Append("                 int8_t ability; uint8_t shiny; uint8_t gender; uint8_t ball; uint8_t fateful; uint8_t roaming;\n");
        h.Append("                 uint32_t groundTile; uint16_t heldItem; uint8_t nature; uint16_t moves[4]; };\n");
        h.Append("enum StaticList : uint8_t { StaticD, StaticP, StaticDP, StaticPt, StaticDPPt, StaticHG, StaticSS, StaticHGSS, StaticListCount };\n");
        h.Append($"inline constexpr std::size_t kStaticCount = {staticRows.Count};\n");
        h.Append("extern const Static4 kStatics[]; extern const uint16_t kStaticListStart[];\n\n");
        c.Append("const Static4 kStatics[] = {\n").Append(Arr(staticRows, 2)).Append("};\n");
        c.Append("const uint16_t kStaticListStart[] = {\n").Append(Arr(listStart)).Append("};\n\n");

        var walker = (EncounterStatic4Pokewalker[])SF("Encounters4HGSS", "Encounter_PokeWalker");
        var walkerRows = walker.Select(w => $"{{{(int)w.Course}, {w.Species}, {w.Level}, {w.Gender}, {{{w.Moves.Move1}, {w.Moves.Move2}, {w.Moves.Move3}, {w.Moves.Move4}}}}}").ToList();
        h.Append("/// EncounterStatic4Pokewalker in PKHeX order (encounter_walker4.pkl).\n");
        h.Append("struct Walker4 { uint8_t course; uint16_t species; uint8_t level; uint8_t gender; uint16_t moves[4]; };\n");
        h.Append($"inline constexpr std::size_t kWalkerCount = {walkerRows.Count};\n");
        h.Append("extern const Walker4 kWalkers[];\n\n");
        c.Append("const Walker4 kWalkers[] = {\n").Append(Arr(walkerRows, 4)).Append("};\n\n");
    }

    private static void EmitTrades(StringBuilder h, StringBuilder c)
    {
        var strings = new List<string>();
        int AddStrings(object? mem)
        {
            int start = strings.Count;
            var arr = mem is ReadOnlyMemory<string> r ? r.ToArray() : [];
            for (int i = 0; i < 9; i++)
                strings.Add(i < arr.Length ? arr[i] ?? "" : "");
            return start;
        }
        var rows = new List<string>();
        foreach (var (type, field, list) in new[] { ("Encounters4DPPt", "TradeGift_DPPtIngame", 0), ("Encounters4HGSS", "TradeGift_HGSS", 1) })
        {
            foreach (var t in (EncounterTrade4PID[])SF(type, field))
            {
                var ivs = t.IVs;
                var m = t.Moves;
                int ot = AddStrings(IF(t, "TrainerNames"));
                int nick = AddStrings(IF(t, "Nicknames"));
                rows.Add($"{{{list}, {(int)t.Version}, {t.Species}, {t.Level}, {Ability(t.Ability)}, {t.OTGender}, {t.TID16}, {t.SID16}, {t.Gender}, "
                    + $"{{{ivs.HP}, {ivs.ATK}, {ivs.DEF}, {ivs.SPE}, {ivs.SPA}, {ivs.SPD}}}, {{{m.Move1}, {m.Move2}, {m.Move3}, {m.Move4}}}, "
                    + $"{t.MetLocation}, {t.Contest}, {t.PID}u, {ot}, {nick}}}");
            }
        }
        h.Append("/// EncounterTrade4PID: list 0 TradeGift_DPPtIngame, 1 TradeGift_HGSS. ivs are fixed values.\n");
        h.Append("/// otNames/nicknames index kTradeStrings: 9 entries each, indexed by language id 0..8.\n");
        h.Append("struct Trade4 { uint8_t list; uint8_t version; uint16_t species; uint8_t level; int8_t ability; uint8_t otGender;\n");
        h.Append("                uint16_t tid; uint16_t sid; uint8_t gender; int8_t ivs[6]; uint16_t moves[4]; uint8_t metLocation;\n");
        h.Append("                uint8_t contest; uint32_t pid; uint16_t otNames; uint16_t nicknames; };\n");
        h.Append($"inline constexpr std::size_t kTradeCount = {rows.Count};\n");
        h.Append("extern const Trade4 kTrades[];\n");
        c.Append("const Trade4 kTrades[] = {\n").Append(Arr(rows, 1)).Append("};\n");
        h.Append($"inline constexpr std::size_t kTradeStringCount = {strings.Count};\n");
        h.Append("extern const char16_t* const kTradeStrings[];\n");
        c.Append("const char16_t* const kTradeStrings[] = {\n").Append(Arr(strings.Select(U16Lit), 9)).Append("};\n");

        var ranch = (EncounterTrade4RanchGift[])SF("Encounters4DPPt", "RanchGifts");
        var ranchRows = ranch.Select(r => $"{{{r.PID}u, {r.Species}, {r.MetLevel}, {r.Level}, {(r.FatefulEncounter ? 1 : 0)}, {r.Location}, {r.EggLocation}, {(int)r.FixedBall}, "
            + $"{{{r.Moves.Move1}, {r.Moves.Move2}, {r.Moves.Move3}, {r.Moves.Move4}}}, {r.SID16}, {r.OTGender}, {r.Gender}, {Ability(r.Ability)}, {r.Form}}}").ToList();
        h.Append("/// EncounterTrade4RanchGift (My Pokemon Ranch, Diamond), TID 1000.\n");
        h.Append("struct Ranch4 { uint32_t pid; uint16_t species; uint8_t metLevel; uint8_t level; uint8_t fateful; uint16_t location;\n");
        h.Append("                uint16_t eggLocation; uint8_t ball; uint16_t moves[4]; uint16_t sid; uint8_t otGender; uint8_t gender;\n");
        h.Append("                int8_t ability; uint8_t form; };\n");
        h.Append($"inline constexpr std::size_t kRanchCount = {ranchRows.Count};\n");
        h.Append("extern const Ranch4 kRanch[];\n\n");
        c.Append("const Ranch4 kRanch[] = {\n").Append(Arr(ranchRows, 1)).Append("};\n\n");
    }

    private static void EmitGifts(StringBuilder h, StringBuilder c)
    {
        Resource("wc4.pkl", "PKHeX.Core/Resources/legality/mgdb/wc4.pkl");
        var db = EncounterEvent.MGDB_G4;
        var bytes = new List<byte>();
        var ids = new List<int>();
        foreach (var pcd in db)
        {
            var pgt = pcd.Gift;
            var head = pgt.Data[..8].ToArray();
            var pk = pgt.PK.Data.ToArray(); // decrypted PK4 party data
            if (pk.Length != 236)
                throw new Exception("PGT PK4 size");
            bytes.AddRange(head);
            bytes.AddRange(pk);
            ids.Add(pcd.CardID);
        }
        h.Append("/// PCD Mystery Gifts (wc4.pkl) in PKHeX order: 8-byte PGT header then the decrypted 236-byte PK4.\n");
        h.Append("inline constexpr std::size_t kPcdRecordSize = 8 + 236;\n");
        h.Append($"inline constexpr std::size_t kPcdCount = {db.Length};\n");
        h.Append("extern const uint8_t kPcd[]; extern const uint16_t kPcdCardId[];\n\n");
        c.Append("const uint8_t kPcd[] = {\n").Append(Hex(bytes)).Append("};\n");
        c.Append("const uint16_t kPcdCardId[] = {\n").Append(Arr(ids)).Append("};\n\n");
    }

    private static void EmitMoves(StringBuilder h, StringBuilder c)
    {
        var pp = Enumerable.Range(0, 468).Select(m => (int)MoveInfo.GetPP(EntityContext.Gen4, (ushort)m)).ToList();
        h.Append("/// MoveInfo.GetPP(Gen4, move) for moves 0..467.\n");
        h.Append("inline constexpr std::size_t kMoveCount = 468;\n");
        h.Append("extern const uint8_t kMovePP[];\n");
        c.Append("const uint8_t kMovePP[] = {\n").Append(Arr(pp, 20)).Append("};\n");
        var items = new List<int>();
        for (int i = 0; i < 1000; i++)
        {
            if (i != 0 && ItemRestrictions.IsHeldItemAllowed(i, EntityContext.Gen4))
                items.Add(i);
        }
        h.Append("/// ItemRestrictions.IsHeldItemAllowed(item, Gen4) for item != 0, ascending.\n");
        h.Append($"inline constexpr std::size_t kHeldItemCount = {items.Count};\n");
        h.Append("extern const uint16_t kHeldItems[];\n");
        c.Append("const uint16_t kHeldItems[] = {\n").Append(Arr(items)).Append("};\n");
        h.Append("/// PersonalInfo4 static move lists.\n");
        h.Append("extern const uint16_t kMachineTM[92]; extern const uint16_t kMachineHMDPPt[8]; extern const uint16_t kMachineHMHGSS[8];\n");
        h.Append("extern const uint16_t kTutorMoves[52];\n");
        c.Append("const uint16_t kMachineTM[92] = {\n").Append(Arr(PersonalInfo4.MachineMovesTechnical.ToArray().Select(x => (int)x))).Append("};\n");
        c.Append("const uint16_t kMachineHMDPPt[8] = {\n").Append(Arr(PersonalInfo4.MachineMovesHiddenDPPt.ToArray().Select(x => (int)x))).Append("};\n");
        c.Append("const uint16_t kMachineHMHGSS[8] = {\n").Append(Arr(PersonalInfo4.MachineMovesHiddenHGSS.ToArray().Select(x => (int)x))).Append("};\n");
        c.Append("const uint16_t kTutorMoves[52] = {\n").Append(Arr(PersonalInfo4.TutorMoves.ToArray().Select(x => (int)x))).Append("};\n");
        // Experience table (6 growth rates x 100 levels) for level/EXP agreement.
        var exp = new List<uint>();
        for (byte g = 0; g < 6; g++)
        {
            for (byte lvl = 1; lvl <= 100; lvl++)
                exp.Add(Experience.GetEXP(lvl, g));
        }
        h.Append("/// Experience.GetEXP(level, growth): kExperience[growth * 100 + level - 1].\n");
        h.Append("extern const uint32_t kExperience[600];\n\n");
        c.Append("const uint32_t kExperience[600] = {\n").Append(Arr(exp.Select(x => $"{x}u"), 10)).Append("};\n\n");
    }

    private static void EmitText(StringBuilder h, StringBuilder c)
    {
        // Gen IV character table: ConvertValue2CharG4 for every code unit that maps to something.
        var map = new List<string>();
        for (int v = 0; v < 0xFFFF; v++)
        {
            var ch = StringConverter4Util.ConvertValue2CharG4((ushort)v);
            if (ch == StringConverter4.Terminator)
                continue;
            map.Add($"{{0x{v:X4}, 0x{(int)StringConverter4Util.NormalizeGenderSymbol((char)ch):X4}}}");
        }
        h.Append("/// StringConverter4.LoadString per code unit: {encoded value, decoded UTF-16 char} for every value\n");
        h.Append("/// ConvertValue2CharG4 maps to a char other than its NUL (0xFFFF); others decode to U+FFFF.\n");
        h.Append("struct CharMap { uint16_t value; uint16_t ch; };\n");
        h.Append($"inline constexpr std::size_t kCharMapCount = {map.Count};\n");
        h.Append("extern const CharMap kCharMap[];\n");
        c.Append("const CharMap kCharMap[] = {\n").Append(Arr(map, 8)).Append("};\n");

        // Species names as Gen IV games write them, per language id 1..8.
        var names = new List<string>();
        for (int lang = 0; lang <= 8; lang++)
        {
            for (ushort s = 0; s <= 493; s++)
            {
                var name = lang is 0 or 6 ? "" : SpeciesName.GetSpeciesNameGeneration(s, lang, 4);
                names.Add(U16Lit(name));
            }
        }
        h.Append("/// SpeciesName.GetSpeciesNameGeneration(species, language, 4): kSpeciesNames4[language * 494 + species]\n");
        h.Append("/// (species 0 is the egg name, GetEggName(language, 4)); languages 0 and 6 are empty.\n");
        h.Append("extern const char16_t* const kSpeciesNames4[9 * 494];\n");
        c.Append("const char16_t* const kSpeciesNames4[9 * 494] = {\n").Append(Arr(names, 8)).Append("};\n");

        // Lowercase folding for every char a Gen IV string can decode to (char.ToLowerInvariant).
        var chars = new SortedSet<int>();
        for (int v = 0; v < 0xFFFF; v++)
        {
            var ch = StringConverter4Util.ConvertValue2CharG4((ushort)v);
            if (ch != StringConverter4.Terminator)
                chars.Add(StringConverter4Util.NormalizeGenderSymbol((char)ch));
        }
        var lower = chars.Where(ch => char.ToLowerInvariant((char)ch) != ch).Select(ch => $"{{0x{ch:X4}, 0x{(int)char.ToLowerInvariant((char)ch):X4}}}").ToList();
        h.Append("/// char.ToLowerInvariant for every decodable Gen IV char that changes: {char, lower}.\n");
        h.Append($"inline constexpr std::size_t kLowerCount = {lower.Count};\n");
        h.Append("extern const CharMap kLower[];\n");
        c.Append("const CharMap kLower[] = {\n").Append(Arr(lower, 8)).Append("};\n");

        // Lowercase species names for WordFilter species-name exemptions (SpeciesName.SpeciesDictLower):
        // first language hit wins, per context language order. Keep only names a Gen IV string can spell.
        var alphabet = new HashSet<char>(chars.Select(x => char.ToLowerInvariant((char)x)));
        var speciesLang = (string[][])SF("SpeciesName", "SpeciesLang");
        string Dict(EntityContext ctx, string name)
        {
            var seen = new Dictionary<string, int>(StringComparer.Ordinal);
            foreach (var lang in Language.GetAvailableGameLanguages(ctx))
            {
                var list = speciesLang[lang];
                var dict = new Dictionary<string, int>(StringComparer.Ordinal);
                for (int s = 1; s < list.Length; s++)
                    dict[list[s].ToLowerInvariant()] = s; // later duplicates overwrite, like PKHeX's GetDictionary
                foreach (var (k, v) in dict)
                    seen.TryAdd(k, v);
            }
            var rows = seen.Where(kv => kv.Key.All(alphabet.Contains)).OrderBy(kv => kv.Key, StringComparer.Ordinal)
                .Select(kv => $"{{{U16Lit(kv.Key)}, {kv.Value}}}").ToList();
            h.Append($"inline constexpr std::size_t k{name}Count = {rows.Count};\n");
            h.Append($"extern const SpeciesKey k{name}[];\n");
            c.Append($"const SpeciesKey k{name}[] = {{\n").Append(Arr(rows, 4)).Append("};\n");
            return name;
        }
        h.Append("/// Lowercased species names (SpeciesName.SpeciesDictLower) reachable from Gen IV text, sorted by\n");
        h.Append("/// UTF-16 code units; species is the first language hit in Language.GetAvailableGameLanguages order.\n");
        h.Append("struct SpeciesKey { const char16_t* name; uint16_t species; };\n");
        Dict(EntityContext.Gen4, "SpeciesLowerGen4");
        Dict(EntityContext.Gen7, "SpeciesLowerGen7");
        Dict(EntityContext.Gen3, "SpeciesLowerGen3");

        // Case-sensitive species names (SpeciesName.SpeciesDict, used by TryGetSpeciesAnyLanguage) a
        // Gen IV string can spell, first language hit in Gen IV language order.
        var exact = new HashSet<char>(chars.Select(x => (char)x));
        var seenExact = new Dictionary<string, int>(StringComparer.Ordinal);
        foreach (var lang in Language.GetAvailableGameLanguages(EntityContext.Gen4))
        {
            var list = speciesLang[lang];
            var dict = new Dictionary<string, int>(StringComparer.Ordinal);
            for (int s = 1; s < list.Length; s++)
                dict[list[s]] = s;
            foreach (var (k, v) in dict)
                seenExact.TryAdd(k, v);
        }
        var exactRows = seenExact.Where(kv => kv.Key.All(exact.Contains)).OrderBy(kv => kv.Key, StringComparer.Ordinal)
            .Select(kv => $"{{{U16Lit(kv.Key)}, {kv.Value}}}").ToList();
        h.Append("/// Species names as SpeciesName.SpeciesDict holds them (case-sensitive) that a Gen IV string can spell,\n");
        h.Append("/// sorted by UTF-16 code units; species is the first hit in the Gen IV language order.\n");
        h.Append($"inline constexpr std::size_t kSpeciesGen4Count = {exactRows.Count};\n");
        h.Append("extern const SpeciesKey kSpeciesGen4[];\n");
        c.Append("const SpeciesKey kSpeciesGen4[] = {\n").Append(Arr(exactRows, 4)).Append("};\n");

        // char.IsNumber and char.IsWhiteSpace over every char a Gen IV string can decode to.
        var numbers = chars.Where(ch => char.IsNumber((char)ch)).Select(ch => $"0x{ch:X4}").ToList();
        var spaces = chars.Where(ch => char.IsWhiteSpace((char)ch)).Select(ch => $"0x{ch:X4}").ToList();
        h.Append("/// Decodable Gen IV chars for which char.IsNumber / char.IsWhiteSpace is true, ascending.\n");
        h.Append($"inline constexpr std::size_t kNumberCharCount = {numbers.Count};\n");
        h.Append("extern const uint16_t kNumberChars[];\n");
        h.Append($"inline constexpr std::size_t kWhiteSpaceCharCount = {spaces.Count};\n");
        h.Append("extern const uint16_t kWhiteSpaceChars[];\n");
        c.Append("const uint16_t kNumberChars[] = {\n").Append(Arr(numbers, 12)).Append("};\n");
        c.Append("const uint16_t kWhiteSpaceChars[] = {\n").Append(Arr(spaces, 12)).Append("};\n");
        h.Append('\n');
        c.Append('\n');
    }

    private static void EmitWordFilter(StringBuilder h, StringBuilder c)
    {
        var nx = PinText("PKHeX.Core/Resources/text/badwords/badwords_switch.txt");
        var ds = PinText("PKHeX.Core/Resources/text/badwords/badwords_3ds.txt");
        static List<string> Lines(string text)
        {
            // WordFilter.LoadPatterns: lowercase invariant, one regex per line.
            var lower = text.ToLowerInvariant();
            return lower.Split('\n').Select(l => l.TrimEnd('\r')).ToList();
        }
        var a = Lines(nx);
        var b = Lines(ds);
        h.Append("/// WordFilterNX / WordFilter3DS patterns (badwords_switch.txt, badwords_3ds.txt), lowercased as\n");
        h.Append("/// WordFilter.LoadPatterns does; .NET regular expressions in the subset Gen4Legality's matcher accepts.\n");
        h.Append($"inline constexpr std::size_t kWordFilterNXCount = {a.Count};\n");
        h.Append($"inline constexpr std::size_t kWordFilter3DSCount = {b.Count};\n");
        h.Append("extern const char16_t* const kWordFilterNX[]; extern const char16_t* const kWordFilter3DS[];\n\n");
        c.Append("const char16_t* const kWordFilterNX[] = {\n").Append(Arr(a.Select(U16Lit), 4)).Append("};\n");
        c.Append("const char16_t* const kWordFilter3DS[] = {\n").Append(Arr(b.Select(U16Lit), 4)).Append("};\n\n");
    }

    // ------------------------------------------------------------------ Generation III (Pal Park origin)
    private static int GiftMethod(PIDType t) => t switch
    {
        PIDType.BACD_U => 1,
        PIDType.BACD_R => 2,
        PIDType.BACD_R_A => 3,
        PIDType.BACD_U_AX => 4,
        PIDType.BACD_TA => 5,
        PIDType.BACD_TS => 6,
        PIDType.BACD_RBCD => 7,
        PIDType.BACD_M => 8,
        PIDType.Channel => 9,
        PIDType.Method_2 => 10,
        _ => throw new Exception($"unmapped WC3 method {t}"),
    };

    private static int VersionMask(GameVersion v)
    {
        int mask = 0;
        for (int i = 1; i <= 15; i++)
        {
            if (v.Contains((GameVersion)i))
                mask |= 1 << i;
        }
        return mask;
    }

    private static string Moves4(Moveset m) => $"{{{m.Move1}, {m.Move2}, {m.Move3}, {m.Move4}}}";

    private static void EmitGen3(StringBuilder h, StringBuilder c)
    {
        h.Append("// ---------------------------------------------------------------- Generation III (Pal Park origin)\n");
        c.Append("// ---------------------------------------------------------------- Generation III (Pal Park origin)\n");

        // Personal: PersonalTable.E gender ratio and abilities (species 0..386); RS form counts must all be 1
        // (EvolutionTree.Evolves3 registers form 0 only).
        Resource("personal_e", "PKHeX.Core/Resources/byte/personal/personal_e");
        Resource("personal_rs", "PKHeX.Core/Resources/byte/personal/personal_rs");
        Resource("hmtm_g3.pkl", "PKHeX.Core/Resources/byte/personal/hmtm_g3.pkl");
        Resource("tutors_g3.pkl", "PKHeX.Core/Resources/byte/personal/tutors_g3.pkl");
        var e = PersonalTable.E;
        var rs = PersonalTable.RS;
        var prows = new List<string>();
        for (ushort s = 0; s <= 386; s++)
        {
            if (rs[s].FormCount != 1)
                throw new Exception($"RS form count {s}");
            var pi = e[s];
            prows.Add($"{{{pi.Gender}, {pi.Ability1}, {pi.Ability2}}}");
        }
        h.Append("/// PersonalTable.E per species 0..386: gender ratio and abilities.\n");
        h.Append("struct Personal3 { uint8_t gender; uint8_t ability1; uint8_t ability2; };\n");
        h.Append("extern const Personal3 kPersonalE3[387];\n");
        c.Append("const Personal3 kPersonalE3[387] = {\n").Append(Arr(prows, 8)).Append("};\n");

        // Level-up learnsets (indexed by species) and the shared Gen III egg moves.
        void Learn(string name, string res, string path, string ident)
        {
            var entries = BinLinker16(Resource(res, path), ident);
            var lists = new List<IReadOnlyList<string>>();
            foreach (var en in entries)
            {
                int count = en.Length / 3;
                var l = new List<string>();
                for (int i = 0; i < count; i++)
                    l.Add($"{{{BitConverter.ToUInt16(en, i * 2)}, {en[(count * 2) + i]}}}");
                lists.Add(l);
            }
            Flatten(lists, out var flat, out var starts);
            h.Append($"inline constexpr std::size_t kLevelUp{name}Count = {entries.Count};\n");
            h.Append($"extern const LevelMove kLevelUp{name}[]; extern const uint16_t kLevelUp{name}Start[];\n");
            c.Append($"const LevelMove kLevelUp{name}[] = {{\n").Append(Arr(flat, 8)).Append("};\n");
            c.Append($"const uint16_t kLevelUp{name}Start[] = {{\n").Append(Arr(starts)).Append("};\n");
        }
        h.Append("/// Gen III level-up learnsets indexed by species (lvlmove_e/rs/fr/lg.pkl).\n");
        Learn("E3", "lvlmove_e.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_e.pkl", "em");
        Learn("RS3", "lvlmove_rs.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_rs.pkl", "rs");
        Learn("FR3", "lvlmove_fr.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_fr.pkl", "fr");
        Learn("LG3", "lvlmove_lg.pkl", "PKHeX.Core/Resources/byte/levelup/lvlmove_lg.pkl", "lg");
        {
            var entries = BinLinker16(Resource("eggmove_rs.pkl", "PKHeX.Core/Resources/byte/eggmove/eggmove_rs.pkl"), "rs");
            var lists = entries.Select(en => (IReadOnlyList<int>)Enumerable.Range(0, en.Length / 2).Select(i => (int)BitConverter.ToUInt16(en, i * 2)).ToList()).ToList();
            Flatten(lists, out var flat, out var starts);
            h.Append("/// Gen III egg moves indexed by species (eggmove_rs.pkl, shared by every Gen III game).\n");
            h.Append($"inline constexpr std::size_t kEggMovesRS3Count = {entries.Count};\n");
            h.Append("extern const uint16_t kEggMovesRS3[]; extern const uint16_t kEggMovesRS3Start[];\n");
            c.Append("const uint16_t kEggMovesRS3[] = {\n").Append(Arr(flat)).Append("};\n");
            c.Append("const uint16_t kEggMovesRS3Start[] = {\n").Append(Arr(starts)).Append("};\n");
        }

        // Non-level-up Gen III move sources for a format-4 entity, per species: LearnSource3E.GetCanLearn
        // with Machine (TMs only outside format 3) and SpecialTutor, LearnSource3RS and LearnSource3FR
        // with SpecialTutor. All of them depend on the species alone.
        {
            var pk4 = new PK4();
            var eSrc = LearnSource3E.Instance;
            var rsSrc = LearnSource3RS.Instance;
            var frSrc = LearnSource3FR.Instance;
            var lists = new List<IReadOnlyList<int>>();
            for (ushort s = 0; s <= 386; s++)
            {
                var l = new List<int>();
                if (s != 0)
                {
                    var evo = new EvoCriteria { Species = s, LevelMax = 100, LevelMin = 1 };
                    for (ushort m = 1; m <= 354; m++)
                    {
                        bool ok = eSrc.GetCanLearn(pk4, e[s], evo, m, MoveSourceType.Machine | MoveSourceType.SpecialTutor) != default
                               || rsSrc.GetCanLearn(pk4, rs[s], evo, m, MoveSourceType.SpecialTutor) != default
                               || frSrc.GetCanLearn(pk4, PersonalTable.FR[s], evo, m, MoveSourceType.SpecialTutor) != default;
                        if (ok)
                            l.Add(m);
                    }
                }
                lists.Add(l);
            }
            Flatten(lists, out var flat, out var starts);
            h.Append("/// Per species 0..386, ascending: moves a format-4 entity could learn in Gen III other than by level-up\n");
            h.Append("/// (Emerald TMs and tutors, R/S/XD and FR/LG special tutors).\n");
            h.Append("extern const uint16_t kOtherMoves3[]; extern const uint16_t kOtherMoves3Start[388];\n");
            c.Append("const uint16_t kOtherMoves3[] = {\n").Append(Arr(flat)).Append("};\n");
            c.Append("const uint16_t kOtherMoves3Start[388] = {\n").Append(Arr(starts)).Append("};\n");
        }

        // Evolutions (evos_g3.pkl), same layout as evos_g4.
        {
            var entries = BinLinker16(Resource("evos_g3.pkl", "PKHeX.Core/Resources/byte/evolve/evos_g3.pkl"), "g3");
            var lists = new List<IReadOnlyList<string>>();
            foreach (var en in entries)
            {
                var l = new List<string>();
                for (int off = 0; off + 8 <= en.Length; off += 8)
                {
                    var type = (EvolutionType)en[off];
                    var arg = BitConverter.ToUInt16(en, off + 2);
                    var species = BitConverter.ToUInt16(en, off + 4);
                    var form = en[off + 6];
                    var level = en[off + 7];
                    var lvlup = type.IsLevelUpRequired ? 1 : 0;
                    l.Add($"{{{(int)type}, {arg}, {species}, {form}, {level}, {lvlup}}}");
                }
                lists.Add(l);
            }
            Flatten(lists, out var flat, out var starts);
            h.Append("/// EvolutionTree.Evolves3 methods per species (evos_g3.pkl).\n");
            h.Append($"inline constexpr std::size_t kEvolution3Count = {entries.Count};\n");
            h.Append("extern const Evolution kEvolutions3[]; extern const uint16_t kEvolution3Start[];\n");
            c.Append("const Evolution kEvolutions3[] = {\n").Append(Arr(flat, 6)).Append("};\n");
            c.Append("const uint16_t kEvolution3Start[] = {\n").Append(Arr(starts)).Append("};\n");
        }

        // Wild areas: SlotsSwarmRSE, SlotsR, SlotsS, SlotsE, SlotsFR, SlotsLG.
        Resource("encounter_rse_swarm.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_rse_swarm.pkl");
        Resource("encounter_r.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_r.pkl");
        Resource("encounter_s.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_s.pkl");
        Resource("encounter_e.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_e.pkl");
        Resource("encounter_fr.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_fr.pkl");
        Resource("encounter_lg.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_lg.pkl");
        PinText("PKHeX.Core/Legality/Encounters/Data/Gen3/Encounters3RSE.cs");
        PinText("PKHeX.Core/Legality/Encounters/Data/Gen3/Encounters3FRLG.cs");
        var areaRows = new List<string>();
        var slotRows = new List<string>();
        var areaStarts = new List<int>();
        foreach (var (type, field) in new[] { ("Encounters3RSE", "SlotsSwarmRSE"), ("Encounters3RSE", "SlotsR"), ("Encounters3RSE", "SlotsS"),
                     ("Encounters3RSE", "SlotsE"), ("Encounters3FRLG", "SlotsFR"), ("Encounters3FRLG", "SlotsLG") })
        {
            areaStarts.Add(areaRows.Count);
            foreach (var a in (EncounterArea3[])SF(type, field))
            {
                areaRows.Add($"{{{(int)a.Version}, {a.Location}, {(int)a.Type}, {a.Rate}, {slotRows.Count}, {a.Slots.Length}}}");
                foreach (var s in a.Slots)
                {
                    var moves = s is IMoveset ms ? Moves4(ms.Moves) : "{0, 0, 0, 0}";
                    slotRows.Add($"{{{s.Species}, {s.Form}, {s.LevelMin}, {s.LevelMax}, {s.SlotNumber}, {s.MagnetPullIndex}, {s.MagnetPullCount}, {s.StaticIndex}, {s.StaticCount}, {moves}}}");
                }
            }
        }
        areaStarts.Add(areaRows.Count);
        h.Append("/// EncounterArea3 lists in PKHeX order: SlotsSwarmRSE, SlotsR, SlotsS, SlotsE, SlotsFR, SlotsLG;\n");
        h.Append("/// kArea3ListStart[i]..[i+1] bounds list i. type is SlotType3; moves are the swarm movesets.\n");
        h.Append("struct Area3 { uint8_t version; uint8_t location; uint8_t type; uint8_t rate; uint16_t slotStart; uint16_t slotCount; };\n");
        h.Append("struct Slot3 { uint16_t species; uint8_t form; uint8_t levelMin; uint8_t levelMax; uint8_t slotNumber;\n");
        h.Append("               uint8_t magnetPullIndex; uint8_t magnetPullCount; uint8_t staticIndex; uint8_t staticCount; uint16_t moves[4]; };\n");
        h.Append("enum Area3List : uint8_t { SlotsSwarmRSE, SlotsR, SlotsS, SlotsE, SlotsFR, SlotsLG, Area3ListCount };\n");
        h.Append($"inline constexpr std::size_t kArea3Count = {areaRows.Count};\n");
        h.Append($"inline constexpr std::size_t kSlot3Count = {slotRows.Count};\n");
        h.Append("extern const Area3 kAreas3[]; extern const Slot3 kSlots3[]; extern const uint16_t kArea3ListStart[];\n");
        c.Append("const Area3 kAreas3[] = {\n").Append(Arr(areaRows, 4)).Append("};\n");
        c.Append("const Slot3 kSlots3[] = {\n").Append(Arr(slotRows, 3)).Append("};\n");
        c.Append("const uint16_t kArea3ListStart[] = {\n").Append(Arr(areaStarts)).Append("};\n");

        // Statics: StaticR, StaticS, StaticE, StaticRSE, StaticFR, StaticLG, StaticFRLG.
        var staticRows = new List<string>();
        var staticStarts = new List<int>();
        foreach (var (type, field) in new[] { ("Encounters3RSE", "StaticR"), ("Encounters3RSE", "StaticS"), ("Encounters3RSE", "StaticE"),
                     ("Encounters3RSE", "StaticRSE"), ("Encounters3FRLG", "StaticFR"), ("Encounters3FRLG", "StaticLG"), ("Encounters3FRLG", "StaticFRLG") })
        {
            staticStarts.Add(staticRows.Count);
            foreach (var s in (EncounterStatic3[])SF(type, field))
            {
                staticRows.Add($"{{{(int)s.Version}, {s.Species}, {s.Form}, {s.Level}, {s.Location}, {(int)s.FixedBall}, {(s.FatefulEncounter ? 1 : 0)}, "
                    + $"{(s.IsRoaming ? 1 : 0)}, {(s.IsEgg ? 1 : 0)}, {Moves4(s.Moves)}}}");
            }
        }
        staticStarts.Add(staticRows.Count);
        h.Append("/// EncounterStatic3 lists in PKHeX order: StaticR, StaticS, StaticE, StaticRSE, StaticFR, StaticLG, StaticFRLG.\n");
        h.Append("struct Static3 { uint8_t version; uint16_t species; uint8_t form; uint8_t level; uint8_t location; uint8_t ball;\n");
        h.Append("                 uint8_t fateful; uint8_t roaming; uint8_t isEgg; uint16_t moves[4]; };\n");
        h.Append("enum Static3List : uint8_t { StaticR3, StaticS3, StaticE3, StaticRSE3, StaticFR3, StaticLG3, StaticFRLG3, Static3ListCount };\n");
        h.Append($"inline constexpr std::size_t kStatic3Count = {staticRows.Count};\n");
        h.Append("extern const Static3 kStatics3[]; extern const uint16_t kStatic3ListStart[];\n");
        c.Append("const Static3 kStatics3[] = {\n").Append(Arr(staticRows, 2)).Append("};\n");
        c.Append("const uint16_t kStatic3ListStart[] = {\n").Append(Arr(staticStarts)).Append("};\n");

        // In-game trades with names as a Pal Park transfer writes them (StringConverter345.TransferGlyphs34).
        foreach (var lang in new[] { "de", "en", "es", "fr", "it", "ja" })
        {
            PinText($"PKHeX.Core/Resources/legality/gen3/text_traderse_{lang}.txt");
            PinText($"PKHeX.Core/Resources/legality/gen3/text_tradefrlg_{lang}.txt");
        }
        var tradeStrings = new List<string>();
        int AddNames(object? mem)
        {
            int start = tradeStrings.Count;
            var arr = mem is ReadOnlyMemory<string> r ? r.ToArray() : [];
            if (arr.Length != 8)
                throw new Exception("trade3 name count");
            for (int lang = 0; lang < 8; lang++)
            {
                var name = arr[lang] ?? "";
                var tmp = new char[name.Length];
                StringConverter345.TransferGlyphs34(name, lang, tmp);
                tradeStrings.Add(new string(tmp));
            }
            return start;
        }
        var tradeRows = new List<string>();
        var tradeLists = new[] { ("Encounters3RSE", "TradeGift_RS"), ("Encounters3RSE", "TradeGift_E"), ("Encounters3FRLG", "TradeGift_FR"),
                                 ("Encounters3FRLG", "TradeGift_LG"), ("Encounters3FRLG", "TradeGift_FRLG") };
        for (int li = 0; li < tradeLists.Length; li++)
        {
            foreach (var t in (EncounterTrade3[])SF(tradeLists[li].Item1, tradeLists[li].Item2))
            {
                var ivs = t.IVs;
                int ot = AddNames(IF(t, "TrainerNames"));
                int nick = AddNames(IF(t, "Nicknames"));
                tradeRows.Add($"{{{li}, {(int)t.Version}, {t.Species}, {t.Level}, {Ability(t.Ability)}, {t.Gender}, {t.OTGender}, {t.TID16}, {t.SID16}, "
                    + $"{{{ivs.HP}, {ivs.ATK}, {ivs.DEF}, {ivs.SPE}, {ivs.SPA}, {ivs.SPD}}}, "
                    + $"{{{t.ContestCool}, {t.ContestBeauty}, {t.ContestCute}, {t.ContestSmart}, {t.ContestTough}, {t.ContestSheen}}}, {t.PID}u, {ot}, {nick}}}");
            }
        }
        h.Append("/// EncounterTrade3: list 0 TradeGift_RS, 1 TradeGift_E, 2 TradeGift_FR, 3 TradeGift_LG, 4 TradeGift_FRLG.\n");
        h.Append("/// ivs HP Atk Def Spe SpA SpD (fixed); contest Cool Beauty Cute Smart Tough Sheen. otNames/nicknames index\n");
        h.Append("/// kTrade3Strings: 8 entries each (language 0..7) after StringConverter345.TransferGlyphs34.\n");
        h.Append("struct Trade3 { uint8_t list; uint8_t version; uint16_t species; uint8_t level; int8_t ability; uint8_t gender; uint8_t otGender;\n");
        h.Append("                uint16_t tid; uint16_t sid; int8_t ivs[6]; uint8_t contest[6]; uint32_t pid; uint16_t otNames; uint16_t nicknames; };\n");
        h.Append($"inline constexpr std::size_t kTrade3Count = {tradeRows.Count};\n");
        h.Append("extern const Trade3 kTrades3[];\n");
        h.Append($"inline constexpr std::size_t kTrade3StringCount = {tradeStrings.Count};\n");
        h.Append("extern const char16_t* const kTrade3Strings[];\n");
        c.Append("const Trade3 kTrades3[] = {\n").Append(Arr(tradeRows, 1)).Append("};\n");
        c.Append("const char16_t* const kTrade3Strings[] = {\n").Append(Arr(tradeStrings.Select(U16Lit), 8)).Append("};\n");

        // Event gifts (EncountersWC3.Encounter_WC3).
        PinText("PKHeX.Core/Legality/Encounters/Data/Gen3/EncountersWC3.cs");
        var giftStrings = new List<string>();
        var giftRows = new List<string>();
        foreach (var g in (EncounterGift3[])SF("EncountersWC3", "Encounter_WC3"))
        {
            int name = giftStrings.Count;
            giftStrings.Add(g.OriginalTrainerName ?? "");
            giftRows.Add($"{{{g.Species}, {g.Level}, {VersionMask(g.Version)}, {(g.IsEgg ? 1 : 0)}, {g.MetLevel}, {GiftMethod(g.Method)}, {Moves4(g.Moves)}, "
                + $"{g.TID16}, {g.SID16}, {g.Language}, {(int)g.Shiny}, {(g.FatefulEncounter ? 1 : 0)}, {(g.RibbonNational ? 1 : 0)}, "
                + $"{(int)g.OriginalTrainerGender}, {(g.IsFixedTrainer ? 1 : 0)}, {name}}}");
        }
        h.Append("/// EncounterGift3 (EncountersWC3.Encounter_WC3) in PKHeX order. versionMask has bit v set when Version.Contains(v);\n");
        h.Append("/// method: 1 BACD_U, 2 BACD_R, 3 BACD_R_A, 4 BACD_U_AX, 5 BACD_TA, 6 BACD_TS, 7 BACD_RBCD, 8 BACD_M,\n");
        h.Append("/// 9 Channel, 10 Method_2; tid/sid 0xFFFF unspecified; shiny is Shiny; otGender is GiftGender3; otName\n");
        h.Append("/// indexes kGift3Strings.\n");
        h.Append("struct Gift3 { uint16_t species; uint8_t level; uint16_t versionMask; uint8_t isEgg; uint8_t metLevel; uint8_t method;\n");
        h.Append("               uint16_t moves[4]; uint16_t tid; uint16_t sid; uint8_t language; uint8_t shiny; uint8_t fateful;\n");
        h.Append("               uint8_t ribbonNational; uint8_t otGender; uint8_t fixedTrainer; uint16_t otName; };\n");
        h.Append($"inline constexpr std::size_t kGift3Count = {giftRows.Count};\n");
        h.Append("extern const Gift3 kGifts3[]; extern const char16_t* const kGift3Strings[];\n");
        c.Append("const Gift3 kGifts3[] = {\n").Append(Arr(giftRows, 1)).Append("};\n");
        c.Append("const char16_t* const kGift3Strings[] = {\n").Append(Arr(giftStrings.Select(U16Lit), 8)).Append("};\n");

        // PCNY / PCJP distribution machines.
        Resource("encounter_pcny.pkl", "PKHeX.Core/Resources/legality/wild/Gen3/encounter_pcny.pkl");
        PinText("PKHeX.Core/Legality/Encounters/Templates/Gen3/Gifts/Distribution3NY.cs");
        PinText("PKHeX.Core/Legality/Encounters/Templates/Gen3/Gifts/Distribution3JPN.cs");
        var nyRows = ((EncounterGift3NY[])SF("EncountersWC3", "PCNY"))
            .Select(g => $"{{{g.Species}, {(int)g.Distribution}, {g.Level}, {Moves4(g.Moves)}}}").ToList();
        var jpRows = ((EncounterGift3JPN[])SF("EncountersWC3", "PCJP"))
            .Select(g => $"{{{g.Species}, {(int)g.Distribution}}}").ToList();
        h.Append("/// EncounterGift3NY (PCNY) and EncounterGift3JPN (PCJP) in PKHeX order; distribution is the\n");
        h.Append("/// Distribution3NY / Distribution3JPN value.\n");
        h.Append("struct GiftNY3 { uint16_t species; uint8_t distribution; uint8_t level; uint16_t moves[4]; };\n");
        h.Append("struct GiftJP3 { uint16_t species; uint8_t distribution; };\n");
        h.Append($"inline constexpr std::size_t kGiftNY3Count = {nyRows.Count};\n");
        h.Append($"inline constexpr std::size_t kGiftJP3Count = {jpRows.Count};\n");
        h.Append("extern const GiftNY3 kGiftsNY3[]; extern const GiftJP3 kGiftsJP3[];\n");
        c.Append("const GiftNY3 kGiftsNY3[] = {\n").Append(Arr(nyRows, 4)).Append("};\n");
        c.Append("const GiftJP3 kGiftsJP3[] = {\n").Append(Arr(jpRows, 8)).Append("};\n");

        // Colosseum bonus-disk / Mt. Battle gifts (Encounters3RSE.ColoGiftsR, ColoGiftsS).
        var coloStrings = new List<string>();
        var coloRows = new List<string>();
        foreach (var field in new[] { "ColoGiftsR", "ColoGiftsS" })
        {
            foreach (var g in (EncounterGift3Colo[])SF("Encounters3RSE", field))
            {
                int names = coloStrings.Count;
                var arr = IF(g, "TrainerNames") is ReadOnlyMemory<string> r ? r.ToArray() : [];
                if (arr.Length != 8)
                    throw new Exception("colo name count");
                coloStrings.AddRange(arr.Select(x => x ?? ""));
                coloRows.Add($"{{{g.Species}, {g.Level}, {(int)g.Version}, {g.Location}, {g.TID16}, {g.OriginalTrainerGender}, {Moves4(g.Moves)}, {names}}}");
            }
        }
        h.Append("/// EncounterGift3Colo in PKHeX order (ColoGiftsR then ColoGiftsS); names index kGiftColo3Strings (8 per gift).\n");
        h.Append("struct GiftColo3 { uint16_t species; uint8_t level; uint8_t version; uint8_t location; uint16_t tid; uint8_t otGender;\n");
        h.Append("                   uint16_t moves[4]; uint16_t names; };\n");
        h.Append($"inline constexpr std::size_t kGiftColo3Count = {coloRows.Count};\n");
        h.Append($"inline constexpr std::size_t kGiftColoR3Count = {((EncounterGift3Colo[])SF("Encounters3RSE", "ColoGiftsR")).Length};\n");
        h.Append("extern const GiftColo3 kGiftsColo3[]; extern const char16_t* const kGiftColo3Strings[];\n");
        c.Append("const GiftColo3 kGiftsColo3[] = {\n").Append(Arr(coloRows, 1)).Append("};\n");
        c.Append("const char16_t* const kGiftColo3Strings[] = {\n").Append(Arr(coloStrings.Select(U16Lit), 8)).Append("};\n");

        // Egg names as a Gen III encounter's egg is checked (SpeciesName.GetEggName(language, 3)).
        var eggNames = Enumerable.Range(0, 9).Select(l => U16Lit(SpeciesName.GetEggName(l, 3))).ToList();
        h.Append("/// SpeciesName.GetEggName(language, 3) for language 0..8.\n");
        h.Append("extern const char16_t* const kEggName3[9];\n");
        c.Append("const char16_t* const kEggName3[9] = {\n").Append(Arr(eggNames, 9)).Append("};\n");

        // Hand-ported RNG tables (MystryMew seeds) are traced to their source file.
        PinText("PKHeX.Core/Legality/RNG/ClassicEra/Gen3/MystryMew.cs");
        h.Append('\n');
        c.Append('\n');
    }
}
