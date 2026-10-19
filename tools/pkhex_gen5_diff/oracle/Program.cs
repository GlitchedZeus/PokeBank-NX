// Gen V differential oracle for PokeBank NX.
//
//   dotnet PKHeXGen5Oracle.dll generate SEED MUTATIONS
//   dotnet PKHeXGen5Oracle.dll probe HEX
//   dotnet PKHeXGen5Oracle.dll casemap          (.NET invariant casing, every UTF-16 unit)
//   dotnet PKHeXGen5Oracle.dll wordfilter SEED  (PKHeX word filter verdicts for test strings)
//
// generate builds, with a fixed test trainer, every Gen V encounter PKHeX knows for
// species 1-649 in Black, White, Black 2 and White 2 ("native"), unhatched eggs of every
// breedable species with inherited moves in PKHeX's own MoveBreed order and in a shuffled
// order ("egg"), every in-game trade in every Gen V language ("trade"), every PGF card in
// PKHeX's Mystery Gift database ("event"), Gen III/IV encounters converted PK3 -> PK4 ->
// PK5 the way Poke Transfer does ("transfer"), "grown" copies of the PKHeX-valid ones
// (levelled, moves relearned from PKHeX's own learnable set, EVs, sometimes evolved),
// move-evolutions forced at any level ("evomove") and fused Kyurem forms with the
// form-exclusive moves ("kyurem"). Each record then gets MUTATIONS single-field
// mutations. One line per record:
//   <id> <decrypted 136-byte PK5 hex> VALID|INVALID <encounter type>:<species> <origin> <group> <game> ; <identifier>:<result> ...
// The records are synthetic (made by PKHeX from its own templates), never a personal save.
using System.Reflection;
using System.Text;
using PKHeX.Core;

if (args.Length == 2 && args[0] == "probe")
{
    // Every check result for one decrypted PK5 (debugging a divergence).
    var probe = new PK5(Convert.FromHexString(args[1]));
    var pla = new LegalityAnalysis(probe);
    Console.WriteLine($"{(pla.Valid ? "VALID" : "INVALID")} {pla.EncounterMatch.GetType().Name}:{pla.EncounterMatch.Species} nick='{probe.Nickname}' ot='{probe.OriginalTrainerName}'");
    foreach (var r in pla.Results)
        Console.WriteLine($"  {(r.Valid ? "ok " : "BAD")} {r.Judgement} {r.Identifier}:{r.Result}");
    for (int i = 0; i < 4; i++)
        Console.WriteLine($"  move{i + 1} {pla.Info.Moves[i].Valid} {pla.Info.Moves[i].Info.Method}");
    Console.WriteLine($"  PIDIV {pla.Info.PIDIV.Type}, MethodFinder {MethodFinder.Analyze(probe).Type}");
    return 0;
}
if (args.Length == 1 && args[0] == "casemap")
{
    // <unit> <ToLowerInvariant> <ToUpperInvariant> <IsNumber> <IsWhiteSpace>, for every UTF-16 code unit.
    var sb = new StringBuilder();
    for (int c = 0; c <= 0xFFFF; c++)
    {
        var ch = (char)c;
        sb.Append($"{c:X4} {(int)char.ToLowerInvariant(ch):X4} {(int)char.ToUpperInvariant(ch):X4} {(char.IsNumber(ch) ? 1 : 0)} {(char.IsWhiteSpace(ch) ? 1 : 0)}\n");
    }
    Console.Out.Write(sb.ToString());
    return 0;
}
if (args.Length == 1 && args[0] == "patterns")
{
    // <list> <index> <UTF-16 hex of the pattern as PKHeX compiled it (lower-cased)>
    foreach (var (name, get) in new (string, Func<int, string>)[] { ("nx", WordFilterNX.GetPattern), ("3ds", WordFilter3DS.GetPattern), ("gen5", WordFilter5.GetPattern) })
    {
        for (int i = 0; ; i++)
        {
            string p;
            try { p = get(i); } catch (IndexOutOfRangeException) { break; }
            Console.WriteLine($"{name} {i} {Text.Hex(p)}");
        }
    }
    return 0;
}
if (args.Length == 2 && args[0] == "wordfilter")
{
    // <UTF-16 hex> <filtered: Gen5 origin> <Gen4 origin> <Gen3 origin>
    var wrng = new Random(int.Parse(args[1]));
    foreach (var s in Text.WordFilterProbes(wrng))
    {
        bool g5 = WordFilter.IsFiltered(s, EntityContext.Gen5, EntityContext.Gen5, out _, out _);
        bool g4 = WordFilter.IsFiltered(s, EntityContext.Gen5, EntityContext.Gen4, out _, out _);
        bool g3 = WordFilter.IsFiltered(s, EntityContext.Gen5, EntityContext.Gen3, out _, out _);
        Console.WriteLine($"{Text.Hex(s)} {(g5 ? 1 : 0)}{(g4 ? 1 : 0)}{(g3 ? 1 : 0)}");
    }
    return 0;
}
if (args.Length == 1 && args[0] == "fixednames")
{
    // Names Gen III/IV encounters fix, as they arrive in a PK5 (Pal Park, Poke Transfer):
    // <generation> <fixed nickname 0|1> <fixed OT 0|1> <fateful 0|1> <language> <nickname hex> <OT hex> <template>
    // for every template in PKHeX's Gen III/IV tables and Gen IV Mystery Gift database
    // whose nickname or OT is not the player's.
    Generator.FixedNames();
    return 0;
}
// Re-evaluate a fixed corpus or apply three simultaneous mutations to every base.
// No record in the input is silently removed; adversarial output is an additional set.
if (args.Length == 2 && args[0] == "replay")
{
    Generator.Replay(args[1]);
    return 0;
}
if (args.Length == 3 && args[0] == "adversarial")
{
    new Generator(new Random(int.Parse(args[2])), 0).Adversarial(args[1]);
    return 0;
}
if (args.Length != 3 || args[0] != "generate")
{
    Console.Error.WriteLine("usage: generate SEED MUTATIONS | probe HEX | casemap | patterns | wordfilter SEED | fixednames");
    return 2;
}

var rng = new Random(int.Parse(args[1]));
var mutations = int.Parse(args[2]);
var gen = new Generator(rng, mutations);
gen.Native();
gen.Trades();
gen.Cards();
gen.Transfers();
gen.Grown();
gen.SpecialForms();
return 0;

/// <summary>Test strings for names: species names, bad-word samples, digits, scripts.</summary>
static class Text
{
    public static readonly int[] Languages5 = [1, 2, 3, 4, 5, 7, 8];
    private static readonly string[] Fillers = ["Rex", "Bubbles", "ab", "Zz", "Max", "Ace", "Q", "Li", "Kip", "Neo"];

    public static string Hex(string s)
    {
        var sb = new StringBuilder();
        foreach (var c in s)
            sb.Append(((int)c).ToString("X4"));
        return sb.ToString();
    }

    private static List<string>? _patterns;

    /// <summary>A string each word-filter pattern matches (and a few it may not), from PKHeX's own lists.</summary>
    public static List<string> BadSamples()
    {
        if (_patterns != null)
            return _patterns;
        var result = new List<string>();
        foreach (var get in new Func<int, string>[] { WordFilter3DS.GetPattern, WordFilterNX.GetPattern, i => WordFilter5.GetPattern(i) })
        {
            for (int i = 0; ; i++)
            {
                string p;
                try { p = get(i); } catch { break; }
                result.Add(Sample(p));
            }
        }
        return _patterns = result;
    }

    // Literal text a simple pattern requires: drop .* and anchors, first branch of a group,
    // first member of a class, unescape.
    private static string Sample(string p)
    {
        var sb = new StringBuilder();
        for (int i = 0; i < p.Length; i++)
        {
            char c = p[i];
            if (c == '.' && i + 1 < p.Length && p[i + 1] == '*') { i++; continue; }
            if (c is '^' or '$' or '?' or '*' or '+') continue;
            if (c == '\\' && i + 1 < p.Length) { sb.Append(p[++i]); continue; }
            if (c == '[')
            {
                int end = p.IndexOf(']', i);
                var inner = p[(i + 1)..end];
                sb.Append(inner.StartsWith('^') ? 'x' : inner[0]);
                i = end;
                continue;
            }
            if (c == '(')
            {
                int end = p.IndexOf(')', i);
                var inner = p[(i + 1)..end];
                sb.Append(inner.Split('|')[0]);
                i = end;
                continue;
            }
            if (c == ')' || c == '|') continue;
            sb.Append(c == '.' ? 'a' : c);
        }
        return sb.ToString();
    }

    public static string RandomName(Random rng, int max, int language)
    {
        string s;
        switch (rng.Next(12))
        {
            case 0: case 1: // a species name in any Gen V language (possibly another species)
                s = SpeciesName.GetSpeciesNameGeneration((ushort)rng.Next(1, 700), Languages5[rng.Next(Languages5.Length)], 5);
                break;
            case 2: case 3: case 4: // something a word filter should catch
            {
                var bad = BadSamples();
                s = bad[rng.Next(bad.Count)];
                if (rng.Next(3) == 0) s = s.ToUpperInvariant();
                if (rng.Next(4) == 0 && s.Length < max) s = Fillers[rng.Next(Fillers.Length)][..1] + s;
                break;
            }
            case 5: // digits
                s = rng.Next(3) == 0 ? "12345" : rng.Next(2) == 0 ? "A1234" : "Ab" + rng.Next(100, 9999);
                break;
            case 6: // kana / hangul / CJK
                s = rng.Next(4) switch
                {
                    0 => "テスト",
                    1 => "たまご",
                    2 => "테스트",
                    _ => "名前",
                };
                break;
            case 7: // full-width Latin and spaces
                s = rng.Next(2) == 0 ? "ＡＢＣ" : "A B";
                break;
            case 8: // long
                s = "ABCDEFGHIJKL";
                break;
            default:
                s = Fillers[rng.Next(Fillers.Length)] + (rng.Next(3) == 0 ? rng.Next(10).ToString() : "");
                break;
        }
        if (s.Length > max)
            s = s[..max];
        return s;
    }

    public static IEnumerable<string> WordFilterProbes(Random rng)
    {
        foreach (var s in BadSamples())
        {
            yield return s;
            if (s.Length >= 2)
            {
                yield return s.ToUpperInvariant();
                yield return "x" + s + "y";
                yield return s[..^1];
            }
        }
        for (int i = 0; i < 3000; i++)
            yield return RandomName(rng, 12, Languages5[rng.Next(Languages5.Length)]);
        for (ushort sp = 1; sp <= 1025; sp++)
        {
            foreach (var lang in new[] { 1, 2, 3, 4, 5, 7, 8, 9, 10 })
            {
                var n = SpeciesName.GetSpeciesName(sp, lang);
                yield return n;
                if (rng.Next(4) == 0) yield return n.ToUpperInvariant();
            }
        }
        // Normalizer edge cases: half-width kana with voicing marks, small kana, hiragana, full-width.
        yield return "ｼﾈ";
        yield return "ｼﾈﾊﾞ";
        yield return "しね";
        yield return "ころす";
        yield return "ｺﾛｽ";
        yield return "ＦＵＣＫ";
        yield return "ｆｕｃｋ";
        yield return "f u c k";
        yield return "ｳﾞ";
        yield return "ヵヶ";
    }
}

sealed class Generator(Random rng, int mutations)
{
    private int count;
    public static void Replay(string file)
    {
        foreach (var line in File.ReadLines(file))
        {
            var words = line.Split(' ');
            if (words.Length < 7) throw new FormatException("Malformed corpus line");
            Print(words[0], new PK5(Convert.FromHexString(words[1])), words[4], words[5]);
        }
    }
    public void Adversarial(string file)
    {
        foreach (var line in File.ReadLines(file))
        {
            var words = line.Split(' ');
            if (words.Length < 7) throw new FormatException("Malformed corpus line");
            if (!words[0].StartsWith('b')) continue; // declared additional base-only set
            var pk = new PK5(Convert.FromHexString(words[1]));
            var names = new[] { Mutate(pk), Mutate(pk), Mutate(pk) };
            pk.RefreshChecksum();
            Print($"a{words[0]}-{string.Join('+', names)}", pk, words[4], "adversarial");
        }
    }

    private readonly List<(PK5 pk, string origin)> grownSeeds = [];
    private static readonly GameVersion[] Versions5 = [GameVersion.B, GameVersion.W, GameVersion.B2, GameVersion.W2];
    private static readonly string[] OtByLanguage = ["", "テスト", "Tester", "Testeur", "Prova", "Tester", "", "Probador", "테스트"];

    // GEN5_ORACLE_TRACE=1 writes each step to stderr (to find a step that does not finish).
    private static readonly bool Tracing = Environment.GetEnvironmentVariable("GEN5_ORACLE_TRACE") == "1";
    private static void Trace(string step)
    {
        if (Tracing)
            Console.Error.WriteLine(step);
    }

    private SimpleTrainerInfo Trainer(GameVersion version, int language)
    {
        return new SimpleTrainerInfo(version)
        {
            OT = OtByLanguage[language],
            TID16 = (ushort)rng.Next(1, 65535),
            SID16 = (ushort)rng.Next(1, 65535),
            Language = language,
            Gender = (byte)rng.Next(2),
        };
    }

    /// <summary>Every Gen V encounter (English trainer), plus every in-game trade in every language.</summary>
    public void Native()
    {
        var seen = new HashSet<string>();
        for (ushort species = 1; species <= 649; species++)
        {
            var pi = PersonalTable.B2W2[species];
            for (byte form = 0; form < Math.Max((byte)1, pi.FormCount); form++)
            {
                foreach (var version in Versions5)
                {
                    var tr = Trainer(version, (int)LanguageID.English);
                    var blank = new PK5 { Species = species, Form = form, Version = version };
                    List<IEncounterable> encounters;
                    try { encounters = EncounterMovesetGenerator.GenerateEncounters(blank, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                    catch (Exception ex) { Audit.Exception("Program.cs:326", ex); continue; }
                    foreach (var enc in encounters)
                    {
                        if (enc.Context != EntityContext.Gen5)
                            continue;
                        var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.EggLocation}:{enc.LevelMin}:{enc.LongName}";
                        if (!seen.Add(key))
                            continue;
                        var origin = OriginOf(enc);
                        if (TryConvert(enc, tr) is { } pk)
                        {
                            EmitWithMutations(pk, origin, "base");
                            if (enc is EncounterEgg5 egg)
                            {
                                foreach (var unhatched in Unhatched(egg, pk))
                                    EmitWithMutations(unhatched, origin, "egg");
                            }
                        }
                        // In-game trades: every other language too.
                        if (enc is EncounterTrade5BW or EncounterTrade5B2W2)
                        {
                            foreach (var lang in Text.Languages5)
                            {
                                if (lang == (int)LanguageID.English)
                                    continue;
                                if (TryConvert(enc, Trainer(version, lang)) is { } pl)
                                    EmitWithMutations(pl, origin, "lang");
                            }
                        }
                    }
                }
            }
        }
    }

    /// <summary>Every in-game trade template, read from PKHeX's tables, in every language and game.</summary>
    public void Trades()
    {
        var all = new List<IEncounterable>();
        foreach (var type in new[] { typeof(Encounters5BW), typeof(Encounters5B2W2) })
        {
            foreach (var f in type.GetFields(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic))
            {
                if (f.GetValue(null) is EncounterTrade5BW[] a) all.AddRange(a);
                if (f.GetValue(null) is EncounterTrade5B2W2[] b) all.AddRange(b);
            }
        }
        foreach (var enc in all)
        {
            foreach (var version in Versions5)
            {
                if (!enc.Version.Contains(version))
                    continue;
                foreach (var lang in Text.Languages5)
                {
                    if (TryConvert(enc, Trainer(version, lang)) is { } pk)
                        EmitWithMutations(pk, "trade", "table");
                }
            }
        }
    }

    /// <summary>Every PGF card in PKHeX's Gen V Mystery Gift database.</summary>
    public void Cards()
    {
        foreach (var card in EncounterEvent.MGDB_G5)
        {
            if (!card.IsEntity)
                continue;
            var games = Versions5.Where(card.CanBeReceivedByVersion).ToArray();
            if (games.Length == 0)
                continue;
            for (int k = 0; k < 2; k++)
            {
                var version = games[rng.Next(games.Length)];
                int lang = card.Language != 0 ? card.Language : Text.Languages5[rng.Next(Text.Languages5.Length)];
                if (TryConvert(card, Trainer(version, lang)) is { } pk)
                    EmitWithMutations(pk, "event", "card");
            }
        }
    }

    private static string HexOrDash(string s) => s.Length == 0 ? "-" : Text.Hex(s);

    /// <summary>See the "fixednames" mode.</summary>
    public static void FixedNames()
    {
        GameVersion[] games = [GameVersion.R, GameVersion.S, GameVersion.E, GameVersion.FR, GameVersion.LG, GameVersion.CXD,
            GameVersion.D, GameVersion.P, GameVersion.Pt, GameVersion.HG, GameVersion.SS];
        var all = new List<IEncounterable>();
        foreach (var type in typeof(PKM).Assembly.GetTypes())
        {
            if (type.ContainsGenericParameters)
                continue;
            foreach (var f in type.GetFields(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic))
            {
                if (!f.FieldType.IsArray || f.FieldType.GetElementType() is not { } et || !typeof(IEncounterable).IsAssignableFrom(et))
                    continue;
                object? value;
                try { value = f.GetValue(null); } catch (Exception ex) { Audit.Exception("Program.cs:425", ex); continue; }
                if (value is Array arr)
                    all.AddRange(arr.OfType<IEncounterable>().Where(e => e.Generation is 3 or 4));
            }
        }
        var seen = new HashSet<string>();
        foreach (var enc in all.Distinct())
        {
            bool fixedNick = enc is IFixedNickname { IsFixedNickname: true };
            bool fixedOT = enc is IFixedTrainer { IsFixedTrainer: true } or MysteryGift { IsEgg: false } or ITrainerID16ReadOnly;
            if (!fixedNick && !fixedOT)
                continue;
            int[] langs = enc.Generation == 3 ? [1, 2, 3, 4, 5, 7] : Text.Languages5;
            foreach (var version in games.Where(v => enc.Version == v || enc.Version.Contains(v)).Take(1))
            {
                foreach (var lang in langs)
                {
                    var tr = new SimpleTrainerInfo(version) { OT = OtByLanguage[lang], TID16 = 12345, SID16 = 54321, Language = lang };
                    PKM made;
                    try { made = enc.ConvertToPKM(tr); }
                    catch (Exception ex) { Audit.Exception("Program.cs:445", ex); continue; }
                    if (ToPK5(made) is not { } pk5)
                        continue;
                    var line = $"{enc.Generation} {(fixedNick ? 1 : 0)} {(fixedOT ? 1 : 0)} {(pk5.FatefulEncounter ? 1 : 0)} {pk5.Language} " +
                               $"{HexOrDash(pk5.Nickname)} {HexOrDash(pk5.OriginalTrainerName)} {enc.GetType().Name}:{enc.Species}";
                    if (seen.Add(line))
                        Console.WriteLine(line);
                }
            }
        }
    }

    /// <summary>Gen III/IV encounters converted PK3 -> PK4 -> PK5 (Pal Park, then Poke Transfer).</summary>
    public void Transfers()
    {
        GameVersion[] gen3 = [GameVersion.R, GameVersion.S, GameVersion.E, GameVersion.FR, GameVersion.LG, GameVersion.CXD];
        GameVersion[] gen4 = [GameVersion.D, GameVersion.P, GameVersion.Pt, GameVersion.HG, GameVersion.SS];
        var seen = new HashSet<string>();
        for (ushort species = 1; species <= 493; species++)
        {
            foreach (var version in gen3.Concat(gen4))
            {
                bool isGen3 = gen3.Contains(version);
                if (isGen3 && species > 386)
                    continue;
                byte forms = isGen3 ? (byte)1 : Math.Max((byte)1, PersonalTable.Pt[species].FormCount);
                if (species is (int)Species.Unown or (int)Species.Deoxys)
                    forms = isGen3 ? (byte)1 : forms;
                for (byte form = 0; form < forms; form++)
                {
                    int lang = isGen3 ? new[] { 1, 2, 3, 4, 5, 7 }[rng.Next(6)] : Text.Languages5[rng.Next(Text.Languages5.Length)];
                    var tr = Trainer(version, lang);
                    PKM blank = isGen3 ? new PK3 { Species = species, Form = form, Version = version } : new PK4 { Species = species, Form = form, Version = version };
                    List<IEncounterable> encounters;
                    try { encounters = EncounterMovesetGenerator.GenerateEncounters(blank, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                    catch (Exception ex) { Audit.Exception("Program.cs:480", ex); continue; }
                    // Bound the corpus: a few templates per species, form and game.
                    var picked = encounters.Where(e => e.Context is EntityContext.Gen3 or EntityContext.Gen4)
                        .OrderBy(_ => rng.Next()).Take(3);
                    foreach (var enc in picked)
                    {
                        var key = $"{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.EggLocation}:{enc.LevelMin}:{enc.LongName}";
                        if (!seen.Add(key))
                            continue;
                        // My Pokemon Ranch was never released in Korean; PKHeX's template
                        // has no Korean OT name, so give that gift a language it existed in.
                        var trEnc = enc is EncounterTrade4RanchGift && lang == (int)LanguageID.Korean
                            ? Trainer(version, (int)LanguageID.English) : tr;
                        // PKHeX at the pinned commit: for a trainer ID no Colosseum save can
                        // have, EncounterStarter3Colo falls back to MethodCXD.SetStarterSecondFromIVs,
                        // whose loop never ends once a seed fails the name-screen check
                        // ("continue" without stepping). Give the starters a possible trainer ID.
                        if (enc is EncounterStarter3Colo && ColoStarterTrainer(enc.Species, version, lang) is { } colo)
                            trEnc = colo;
                        else if (enc is EncounterStarter3Colo)
                            continue;
                        Trace($"convert {key} lang {trEnc.Language} tid {trEnc.TID16} sid {trEnc.SID16}");
                        PKM made;
                        try { made = enc.ConvertToPKM(trEnc); }
                        catch (Exception ex) { Audit.Exception("Program.cs:504", ex); continue; }
                        var pk5 = ToPK5(made);
                        if (pk5 is null || pk5.IsEgg)
                            continue;
                        EmitWithMutations(pk5, "transfer", "base");
                    }
                }
            }
        }
    }

    /// <summary>A trainer whose ID the Colosseum starter RNG can produce with this starter.</summary>
    private SimpleTrainerInfo? ColoStarterTrainer(ushort species, GameVersion version, int lang)
    {
        for (int i = 0; i < 1000; i++)
        {
            var tr = Trainer(version, lang);
            var probe = new CK3 { Species = species };
            if (MethodCXD.SetStarterFromTrainerID(probe, EncounterCriteria.Unrestricted, tr.TID16, tr.SID16))
                return tr;
        }
        return null;
    }

    private static PK5? ToPK5(PKM made)
    {
        try
        {
            if (made is CK3 ck3) made = ck3.ConvertToPK3();
            if (made is XK3 xk3) made = xk3.ConvertToPK3();
            if (made is PK3 pk3)
            {
                pk3.RefreshChecksum();
                made = pk3.ConvertToPK4();
            }
            if (made is BK4 bk4) made = bk4.ConvertToPK4();
            if (made is RK4 rk4) made = rk4.ConvertToPK4();
            if (made is PK4 pk4)
            {
                pk4.RefreshChecksum();
                return pk4.ConvertToPK5();
            }
        }
        catch (Exception ex)
        {
            Audit.Exception("Program.cs:547", ex);
        }
        return null;
    }

    /// <summary>Grown copies of PKHeX-valid records: levelled, new moves, EVs, sometimes evolved.</summary>
    public void Grown()
    {
        foreach (var (seed, origin) in grownSeeds.ToArray())
        {
            if (rng.Next(100) >= 40)
                continue;
            var pk = seed.Clone();
            var growth = pk.PersonalInfo.EXPGrowth;
            int level = pk.CurrentLevel;
            if (level < 100)
            {
                byte next = (byte)rng.Next(level + 1, 101);
                uint exp = Experience.GetEXP(next, growth);
                if (next < 100 && rng.Next(2) == 0)
                    exp += (uint)rng.Next((int)(Experience.GetEXP((byte)(next + 1), growth) - exp));
                pk.EXP = exp;
            }
            if (rng.Next(3) == 0)
                TryEvolve(pk);
            // EVs, at most 510 in total.
            int budget = 510;
            for (int i = 0; i < 6; i++)
            {
                int v = Math.Min(budget, rng.Next(0, 256));
                if (rng.Next(3) == 0) v = 0;
                pk.SetEV(i, v);
                budget -= v;
            }
            // Moves PKHeX itself says this Pokemon can know now.
            var la = new LegalityAnalysis(pk);
            if (la.EncounterMatch is not EncounterInvalid)
            {
                var possible = LearnPossible.Get(pk, la.EncounterMatch, la.Info.EvoChainsAllGens);
                var learnable = Enumerable.Range(1, Math.Min(possible.Length - 1, 559)).Where(m => possible[m]).ToList();
                if (learnable.Count > 0 && rng.Next(4) != 0)
                {
                    int n = Math.Min(learnable.Count, rng.Next(1, 5));
                    var moves = learnable.OrderBy(_ => rng.Next()).Take(n).Select(m => (ushort)m).ToList();
                    while (moves.Count < 4) moves.Add(0);
                    pk.SetMoves(moves.ToArray());
                    int Ups(int i) => moves[i] == 0 || moves[i] == (ushort)Move.Sketch ? 0 : rng.Next(4);
                    pk.Move1_PPUps = Ups(0); pk.Move2_PPUps = Ups(1); pk.Move3_PPUps = Ups(2); pk.Move4_PPUps = Ups(3);
                    pk.HealPP();
                }
            }
            Emit(pk, origin, "grown", withMutations: true);
        }
    }

    /// <summary>
    /// Unhatched eggs of this template: the level-1 moves alone, then with moves the father
    /// can pass on (egg moves, inherited level-up moves, TMs, Volt Tackle for Pichu) in the
    /// order PKHeX's MoveBreed expects, then the same moves in a shuffled order.
    /// </summary>
    private IEnumerable<PK5> Unhatched(EncounterEgg5 egg, PK5 hatched)
    {
        var species = egg.Species;
        bool bw = egg.Version is GameVersion.B or GameVersion.W;
        ILearnSource ls = bw ? LearnSource5BW.Instance : LearnSource5B2W2.Instance;
        var baseMoves = ls.GetLearnset(species, 0).GetBaseEggMoves(EncounterEgg5.Level).ToArray();
        var eggMoves = LearnSource5BW.Instance.GetEggMoves(species, 0).ToArray();
        var inherit = ls.GetInheritMoves(species, 0).ToArray();
        IPersonalInfoTM pi = bw ? PersonalTable.BW[species] : PersonalTable.B2W2[species];
        var tmList = PersonalInfo5BW.MachineMoves[..^1].ToArray();
        var tms = tmList.Where((_, i) => pi.GetIsLearnTM(i)).ToArray();
        for (int k = 0; k < 3; k++)
        {
            var pk = hatched.Clone();
            pk.IsEgg = true;
            pk.IsNicknamed = true;
            pk.Nickname = SpeciesName.GetSpeciesNameGeneration(0, pk.Language, 5);
            pk.MetLocation = 0;
            pk.MetDate = null;
            pk.OriginalTrainerFriendship = pk.PersonalInfo.HatchCycles;
            var wanted = new List<ushort>(baseMoves);
            if (k > 0)
            {
                var extra = new List<ushort>();
                if (eggMoves.Length > 0) extra.Add(eggMoves[rng.Next(eggMoves.Length)]);
                if (inherit.Length > 0 && rng.Next(2) == 0) extra.Add(inherit[rng.Next(inherit.Length)]);
                if (tms.Length > 0 && rng.Next(2) == 0) extra.Add(tms[rng.Next(tms.Length)]);
                if (species == (int)Species.Pichu && rng.Next(2) == 0) extra.Add((ushort)Move.VoltTackle);
                foreach (var m in extra)
                {
                    if (!wanted.Contains(m))
                        wanted.Add(m);
                }
                while (wanted.Count > 4)
                    wanted.RemoveAt(0);
            }
            var moves = new ushort[4];
            for (int i = 0; i < wanted.Count; i++)
                moves[i] = wanted[i];
            if (k == 2)
            {
                moves = moves.OrderBy(_ => rng.Next()).ToArray(); // probably not an order breeding produces
                if (moves[0] == 0 && wanted.Count > 0)
                    (moves[0], moves[wanted.Count - 1]) = (moves[wanted.Count - 1], moves[0]);
            }
            else
            {
                var expected = new ushort[4];
                MoveBreed.GetExpectedMoves(moves, egg, expected);
                moves = expected;
            }
            pk.SetMoves(moves);
            pk.Move1_PPUps = pk.Move2_PPUps = pk.Move3_PPUps = pk.Move4_PPUps = 0;
            pk.HealPP();
            yield return pk;
        }
    }

    /// <summary>Species that evolve by levelling up while knowing a move (EvolutionRestrictions), with their evolution.</summary>
    private static ushort MoveEvolution(ushort species) => species switch
    {
        (int)Species.MimeJr => (int)Species.MrMime,
        (int)Species.Bonsly => (int)Species.Sudowoodo,
        (int)Species.Aipom => (int)Species.Ambipom,
        (int)Species.Lickitung => (int)Species.Lickilicky,
        (int)Species.Tangela => (int)Species.Tangrowth,
        (int)Species.Yanma => (int)Species.Yanmega,
        (int)Species.Piloswine => (int)Species.Mamoswine,
        _ => 0,
    };

    /// <summary>Forces the move evolution at the current level (valid or not, PKHeX decides) and the fused Kyurem forms.</summary>
    public void SpecialForms()
    {
        foreach (var (seed, origin) in grownSeeds.ToArray())
        {
            var to = MoveEvolution(seed.Species);
            if (to != 0)
            {
                var pk = seed.Clone();
                if (pk.CurrentLevel < 100 && rng.Next(2) == 0)
                    pk.EXP = Experience.GetEXP((byte)rng.Next(pk.CurrentLevel + 1, 101), pk.PersonalInfo.EXPGrowth);
                pk.Species = to;
                pk.Form = 0;
                pk.RefreshAbility(pk.AbilityNumber >> 1);
                pk.Gender = pk.GetSaneGender();
                if (!pk.IsNicknamed)
                    pk.Nickname = SpeciesName.GetSpeciesNameGeneration(to, pk.Language, 5);
                Emit(pk, origin, "evomove", withMutations: true);
            }
            if (seed.Species == (int)Species.Kyurem)
            {
                ushort[] exclusive = [(ushort)Move.ScaryFace, (ushort)Move.FusionFlare, (ushort)Move.FusionBolt];
                for (byte form = 0; form <= 2; form++)
                {
                    foreach (var move in exclusive)
                    {
                        var pk = seed.Clone();
                        pk.Form = form;
                        pk.RefreshAbility(pk.AbilityNumber >> 1); // the fused forms have their own ability
                        pk.SetMove(0, move);
                        pk.HealPP();
                        Emit(pk, origin, "kyurem", withMutations: false);
                    }
                }
            }
        }
    }

    private void TryEvolve(PK5 pk)
    {
        // Try the next species numbers; keep the one PKHeX accepts as an evolution.
        for (int d = 1; d <= 2; d++)
        {
            var copy = pk.Clone();
            ushort to = (ushort)(pk.Species + d);
            if (to > 649)
                return;
            copy.Species = to;
            if (copy.Form >= copy.PersonalInfo.FormCount)
                copy.Form = 0;
            copy.RefreshAbility(copy.AbilityNumber >> 1);
            copy.Gender = copy.GetSaneGender();
            if (!copy.IsNicknamed)
                copy.Nickname = SpeciesName.GetSpeciesNameGeneration(to, copy.Language, 5);
            copy.RefreshChecksum();
            var la = new LegalityAnalysis(copy);
            if (la.Valid && la.EncounterMatch.Species != to)
            {
                copy.Data.CopyTo(pk.Data);
                return;
            }
        }
    }

    private static string OriginOf(IEncounterable enc) => enc switch
    {
        EncounterTrade5BW or EncounterTrade5B2W2 => "trade",
        PGF => "event",
        _ when enc.Generation < 5 => "transfer",
        _ => "native",
    };

    private static PK5? TryConvert(IEncounterable enc, ITrainerInfo tr)
    {
        try { return enc.ConvertToPKM(tr) as PK5; }
        catch (Exception ex) { Audit.Exception("Program.cs:754", ex); return null; }
    }

    private void EmitWithMutations(PK5 pk, string origin, string group) => Emit(pk, origin, group, withMutations: true);

    private void Emit(PK5 pk, string origin, string group, bool withMutations)
    {
        pk.RefreshChecksum(); // a freshly converted PK5 has not been written yet
        var valid = Print($"b{count++}", pk, origin, group);
        if (valid && group != "grown" && !pk.IsEgg)
            grownSeeds.Add((pk.Clone(), origin));
        if (!withMutations)
            return;
        for (int m = 0; m < mutations; m++)
        {
            var copy = pk.Clone();
            var name = Mutate(copy);
            copy.RefreshChecksum();
            Print($"m{count++}-{name}", copy, origin, group);
        }
    }

    private static bool Print(string id, PK5 pk, string origin, string group)
    {
        Trace($"analyse {id}");
        var la = new LegalityAnalysis(pk);
        var enc = la.EncounterMatch;
        var invalid = la.Results.Where(r => !r.Valid || r.Judgement == Severity.Fishy).Select(r => $"{(r.Judgement == Severity.Fishy ? "~" : "")}{r.Identifier}:{r.Result}")
            .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
                .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
        var hex = Convert.ToHexString(pk.Data[..136]);
        Console.WriteLine($"{id} {hex} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species} {origin} {group} {pk.Version} ; {string.Join(' ', invalid)}");
        return la.Valid;
    }

    private string Mutate(PK5 pk)
    {
        switch (rng.Next(34))
        {
            case 0: pk.Ball = (byte)rng.Next(1, 26); return "ball";
            case 1: pk.MetLevel = (byte)Math.Clamp(pk.MetLevel + rng.Next(-3, 4), 0, 100); return "metlevel";
            case 2: pk.HiddenAbility = !pk.HiddenAbility; return "hidden";
            case 3: pk.PID ^= 0x10000; return "abilitybit";
            case 4: pk.Gender = (byte)(pk.Gender ^ 1); return "gender";
            case 5: pk.Nature = (Nature)rng.Next(25); return "nature";
            case 6: pk.PID = (uint)rng.NextInt64(0, 1L << 32); return "pid";
            case 7: pk.MetLocation = (ushort)new[] { 4, 6, 8, 19, 32, 64, 75, 117, 30001, 30002, 30003, 30010, 30012, 30015, 40001, 60002 }[rng.Next(16)]; return "metlocation";
            case 8: pk.SetMove(rng.Next(4) == 0 ? 3 : 0, (ushort)rng.Next(1, 560)); return "move";
            case 9: pk.SetEV(rng.Next(6), rng.Next(256)); return "ev";
            case 10: pk.SetIV(rng.Next(6), rng.Next(32)); return "iv";
            case 11: pk.FatefulEncounter = !pk.FatefulEncounter; return "fateful";
            case 12: pk.EggLocation = (ushort)new[] { 60002, 30003, 30002, 2000, 2002, 0 }[rng.Next(6)]; return "egglocation";
            case 13: pk.Version = (GameVersion)new[] { 20, 21, 22, 23, 20, 21, 22, 23, 1, 3, 4, 10, 12, 15 }[rng.Next(14)]; return "version";
            case 14: pk.Form = (byte)rng.Next(4); return "form";
            case 15: pk.Language = rng.Next(0, 10); return "language";
            case 16: pk.HeldItem = rng.Next(0, 650); return "item";
            case 17: pk.EXP = (uint)Math.Max(0, (int)pk.EXP + rng.Next(-2000, 2000)); return "exp";
            case 18: pk.Ability = rng.Next(1, 165); return "ability";
            case 19: pk.MetDate = new DateOnly(2000 + rng.Next(0, 99), rng.Next(1, 13), rng.Next(1, 29)); return "metdate";
            case 20: pk.Move1_PPUps = rng.Next(0, 4); pk.Move1_PP = rng.Next(0, 70); return "pp";
            case 21: pk.Data[(rng.Next(3) switch { 0 => 0x24, 1 => 0x3C, _ => 0x60 }) + rng.Next(4)] |= (byte)(1 << rng.Next(8)); return "ribbon";
            case 22: pk.ContestCool = (byte)rng.Next(1, 256); return "contest";
            case 23: pk.Species = (ushort)Math.Clamp(pk.Species + (rng.Next(2) == 0 ? -1 : 1), 1, 649); return "species";
            case 24: pk.NSparkle = !pk.NSparkle; return "nsparkle";
            case 25: pk.PokerusState = (byte)rng.Next(256); return "pokerus";
            case 26: pk.Data[0x16] = (byte)rng.Next(256); return "markings";
            case 27: pk.TID16 = (ushort)rng.Next(65536); return "tid";
            case 28: case 29: // a player-chosen nickname (or a species name / bad word / odd text)
                pk.Nickname = Text.RandomName(rng, 11, pk.Language);
                pk.IsNicknamed = rng.Next(5) != 0;
                return "nickname";
            case 30: case 31: // another OT name (8 units fill the field without a terminator)
                pk.OriginalTrainerName = Text.RandomName(rng, 8, pk.Language);
                return "otname";
            case 32: pk.IsNicknamed = !pk.IsNicknamed; return "nickflag";
            default: pk.OriginalTrainerGender = (byte)(pk.OriginalTrainerGender ^ 1); return "otgender";
        }
    }
}

// stderr is retained by run.sh; exceptions are evidence gaps, never passing verdicts.
static class Audit
{
    public static void Exception(string site, Exception ex) => Console.Error.WriteLine(
        System.Text.Json.JsonSerializer.Serialize(new { kind="generation-exception", site,
            type=ex.GetType().FullName, message=ex.Message }));
}
