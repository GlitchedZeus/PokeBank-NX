// Gen III / Gen IV differential oracle for PokeBank NX.
//
//   dotnet PKHeXGen34Oracle.dll generate SEED MUTATIONS [3|4|34] [base|grown]
//   dotnet PKHeXGen34Oracle.dll probe HEX        (80-byte PK3 or 136-byte PK4, decrypted)
//
// Builds every Gen III encounter PKHeX knows for species 1-386 in Ruby, Sapphire,
// Emerald, FireRed and LeafGreen, every Colosseum/XD (GameCube) encounter as the PK3 it
// becomes once traded into each of those five GBA games (CK3/XK3.ConvertToPK3, with a
// trainer whose TID/SID the name screen can produce; the e-Reader shadows with a Japanese
// trainer), and every Gen IV encounter for species 1-493 in Diamond, Pearl, Platinum,
// HeartGold and SoulSilver.
// Each is converted with a fixed test trainer ("Tester"), then MUTATIONS single-field
// mutations of each are added. One line per record:
//   <id> <cartridge> <decrypted box hex> VALID|INVALID <encounter type>:<species> ; <identifier>:<result> ...
// (Invalid results, then invalid moves as Move<n>:<method>, then Fishy results as ~<identifier>:<result>.)
// <cartridge> is the game the base record was generated for (the save it would sit in).
//
// The "grown" corpus (Gen IV only) instead takes every PKHeX-valid base Gen IV record and
// plays it forward: hatching, levels with EXP, level-up / TM / tutor moves, EV training,
// evolution (level, friendship, stone and trade), held items, Pokerus, common Gen IV
// ribbons and a trade into another Gen IV game's save. Some records take another shape
// instead: an unhatched egg (from a bred encounter), a Cute Charm lead capture or a
// Poke Radar chain shiny (from a wild slot), and Pokemon Ranger Manaphy eggs (hatched and
// unhatched). It also transfers every PKHeX-valid Gen III base record through Pal Park
// (PK3.ConvertToPK4). Only grown records PKHeX judges VALID are kept (ids g* and p*);
// MUTATIONS mutations of each follow (ids h*-name, q*-name).
// PK3 hex is the 80-byte decrypted box record in canonical G/A/E/M order; PK4 hex is the
// 136-byte decrypted box record in canonical A/B/C/D order. Records are synthetic.
using PKHeX.Core;

// Re-evaluate exact retained bytes against another pinned oracle. No filtering,
// mutation, renumbering, or caught analysis errors can silently shrink the corpus.
if (args.Length == 2 && args[0] == "replay")
{
    var replayIds = new HashSet<string>();
    foreach (var line in File.ReadLines(args[1]))
    {
        var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        if (parts.Length < 4 || !replayIds.Add(parts[0]))
            throw new InvalidDataException("Malformed or duplicate replay record");
        var cart = Enum.Parse<GameVersion>(parts[1]);
        var bytes = Convert.FromHexString(parts[2]);
        PKM pk = bytes.Length switch { 80 => new PK3(bytes), 136 => new PK4(bytes), _ => throw new InvalidDataException("Invalid replay format") };
        Emit(parts[0], cart, pk, false);
    }
    return 0;
}

if (args.Length >= 2 && args[0] == "probe")
{
    // probe HEX, or probe ORACLE_OUTPUT_FILE ID... (records by id from a generate run)
    var hexes = File.Exists(args[1])
        ? File.ReadLines(args[1]).Select(l => l.Split(' ')).Where(p => args.Skip(2).Contains(p[0])).Select(p => p[2]).ToList()
        : [args[1]];
    foreach (var hex in hexes)
    {
        var bytes = Convert.FromHexString(hex);
        PKM probe = bytes.Length == 80 ? new PK3(bytes) : new PK4(bytes);
        var pla = new LegalityAnalysis(probe);
        Console.WriteLine($"{(pla.Valid ? "VALID" : "INVALID")} {pla.EncounterMatch.GetType().Name}:{pla.EncounterMatch.Species} \"{pla.EncounterMatch.LongName}\"");
        Console.WriteLine($"  species {probe.Species} form {probe.Form} version {probe.Version} egg {probe.IsEgg} met {probe.MetLocation}@{probe.MetLevel} eggloc {probe.EggLocation} ball {probe.Ball} ability {probe.Ability} pid {probe.PID:X8} fateful {probe.FatefulEncounter} lang {probe.Language} OT {probe.OriginalTrainerName} {probe.TID16}/{probe.SID16}");
        foreach (var r in pla.Results)
            Console.WriteLine($"  {(r.Valid ? "ok " : "BAD")} {r.Identifier}:{r.Result}");
    }
    return 0;
}
if (args.Length >= 1 && args[0] == "grow")
    return Grow3.Run(args); // grown Gen III corpus, see Grow3.cs
if (args.Length < 3 || args[0] != "generate")
{
    Console.Error.WriteLine("usage: generate SEED MUTATIONS [3|4|34]");
    return 2;
}
var rng = new Random(int.Parse(args[1]));
var mutations = int.Parse(args[2]);
var which = args.Length > 3 ? args[3] : "34";
var grown = args.Length > 4 && args[4] == "grown";
if (args.Length > 4 && args[4] is not ("base" or "grown"))
{
    Console.Error.WriteLine("corpus must be base or grown");
    return 2;
}
var count = 0;
var seen = new HashSet<string>();
GameVersion[] gen4Carts = [GameVersion.D, GameVersion.P, GameVersion.Pt, GameVersion.HG, GameVersion.SS];

// In the grown corpus, Gen III encounters feed Pal Park transfers into Gen IV saves.
if (which.Contains('3') || (grown && which.Contains('4')))
    Run(3, [GameVersion.R, GameVersion.S, GameVersion.E, GameVersion.FR, GameVersion.LG], 386);
if (which.Contains('4'))
    Run(4, gen4Carts, 493);
if (grown && which.Contains('4'))
{
    // Pokemon Ranger's Manaphy egg (PGT), received in each Gen IV game.
    foreach (var cart in gen4Carts)
    {
        var tr = new SimpleTrainerInfo(cart) { OT = "Tester", TID16 = (ushort)rng.Next(1, 65535), SID16 = (ushort)rng.Next(1, 65535), Language = (int)LanguageID.English };
        var gift = new PGT { CardType = 7, ItemSubID = 1 };
        for (int i = 0; i < 4; i++)
        {
            PK4 egg;
            try { egg = (PK4)gift.ConvertToPKM(tr); }
            catch (Exception) { break; }
            if (i % 2 == 1 && egg.IsEgg)
            {
                egg.IsEgg = false; // hatched in a town of the receiving game
                var towns = Enumerable.Range(1, 234).Select(l => (ushort)l).Where(l => EggHatchLocation4.IsValidMet4(l, egg.Version)).ToArray();
                egg.MetLocation = towns[rng.Next(towns.Length)];
                egg.MetDate = egg.EggMetDate;
                egg.OriginalTrainerFriendship = 120;
                egg.ClearNickname();
            }
            EmitKept("g", "h", cart, egg);
        }
    }
}
return 0;

void Run(int gen, GameVersion[] versions, ushort maxSpecies)
{
    for (ushort species = 1; species <= maxSpecies; species++)
    {
        IPersonalInfo pi = gen == 3 ? PersonalTable.E[species] : PersonalTable.HGSS[species];
        for (byte form = 0; form < Math.Max((byte)1, pi.FormCount); form++)
        {
            // Gen III also enumerates the GameCube (Colosseum/XD) templates, which only
            // EncounterGenerator3GC yields; their records are held in each GBA game's save.
            GameVersion[] origins = gen == 3 ? [.. versions, GameVersion.CXD] : versions;
            foreach (var version in origins)
            {
                var tid = (ushort)rng.Next(1, 65535);
                var sid = (ushort)rng.Next(1, 65535);
                if (version is GameVersion.R or GameVersion.S)
                {
                    // Ruby/Sapphire draw SID then TID from consecutive LCRNG calls; PKHeX
                    // rejects (TrainerIDNoSeed) any pair no seed can produce.
                    var s1 = (uint)rng.NextInt64(0, 1L << 32) * 0x41C64E6Du + 0x6073u;
                    var s2 = s1 * 0x41C64E6Du + 0x6073u;
                    sid = (ushort)(s1 >> 16);
                    tid = (ushort)(s2 >> 16);
                }
                var tr = new SimpleTrainerInfo(version)
                {
                    OT = "Tester",
                    TID16 = tid,
                    SID16 = sid,
                    Language = (int)LanguageID.English,
                };
                PKM blank = gen == 3 ? new PK3 { Species = species, Form = form, Version = version }
                                     : new PK4 { Species = species, Form = form, Version = version };
                List<IEncounterable> encounters;
                try { encounters = EncounterMovesetGenerator.GenerateEncounters(blank, tr, ReadOnlyMemory<ushort>.Empty, version).ToList(); }
                catch (Exception) { continue; }
                foreach (var enc in encounters)
                {
                    if (enc.Context != (gen == 3 ? EntityContext.Gen3 : EntityContext.Gen4))
                        continue;
                    var key = $"{version}:{enc.GetType().Name}:{enc.Species}:{enc.Form}:{enc.Version}:{enc.Location}:{enc.EggLocation}:{enc.LevelMin}:{enc.LongName}";
                    if (!seen.Add(key))
                        continue;
                    // A GameCube-origin record sits in each GBA game's save (one record per
                    // cart, each converted anew); any other sits in its own origin game's save.
                    var gameCube = enc.Version is GameVersion.CXD or GameVersion.COLO or GameVersion.XD;
                    foreach (var holder in gameCube ? versions : [version])
                    {
                    // GameCube trainers draw TID/SID 1000 frames after the name screen's seed;
                    // the e-Reader shadows exist only in Japanese Colosseum.
                    var owner = gameCube ? CxdTrainer(rng, enc is EncounterShadow3Colo { IsEReader: true }) : tr;
                    PKM made;
                    try { made = enc.ConvertToPKM(owner); }
                    catch (Exception) { continue; }
                    // A GameCube record enters a GBA save through the link cable as a PK3.
                    if (made is CK3 ck3) made = ck3.ConvertToPK3();
                    else if (made is XK3 xk3) made = xk3.ConvertToPK3();
                    if (made is not (PK3 or PK4))
                        continue;
                    var cart = made.Version; // the record sits in a save of its own origin game
                    if (!versions.Contains(cart))
                        cart = holder;
                    if (grown)
                    {
                        EmitGrown(gen, cart, made);
                        continue;
                    }
                    Emit($"b{count++}", cart, made);
                    if (made is PK4 { Species: (ushort)Species.Unown } unown)
                    {
                        // A Gen IV Unown slot converts to one letter; the letter is stored
                        // independently of the PID, so cover every form PKHeX can judge.
                        for (byte f = 0; f < 28; f++)
                        {
                            if (f == unown.Form)
                                continue;
                            var letter = unown.Clone();
                            letter.Form = f;
                            Emit($"b{count++}", cart, letter);
                        }
                    }
                    for (int m = 0; m < mutations; m++)
                    {
                        var copy = made.Clone();
                        var name = gen == 3 ? Mutate3((PK3)copy, rng) : Mutate4((PK4)copy, rng);
                        Emit($"m{count++}-{name}", cart, copy);
                    }
                    }
                }
            }
        }
    }
}

// A Colosseum/XD trainer: the name screen's final seed, advanced 1000 frames, yields
// TID then SID (MethodCXD.TryGetSeedTrainerID); PKHeX rejects pairs no such seed produces.
static SimpleTrainerInfo CxdTrainer(Random rng, bool japanese)
{
    while (true)
    {
        var seed = (uint)rng.NextInt64(0, 1L << 32);
        if (!MethodCXD.IsValidNameScreenEndSeed(XDRNG.Prev1000(seed), out _))
            continue;
        var tid = (ushort)XDRNG.Next16(ref seed);
        var sid = (ushort)XDRNG.Next16(ref seed);
        return new SimpleTrainerInfo(GameVersion.CXD)
        {
            OT = japanese ? "\u30c6\u30b9\u30c8" : "Tester",
            TID16 = tid,
            SID16 = sid,
            Language = (int)(japanese ? LanguageID.Japanese : LanguageID.English),
        };
    }
}

void EmitGrown(int gen, GameVersion cart, PKM made)
{
    made.RefreshChecksum();
    if (!Analyze(made, cart).Valid)
        return; // only legitimate records are grown
    PK4 result;
    string baseId, mutId;
    if (gen == 3)
    {
        if (made.IsEgg || !which.Contains('4'))
            return;
        result = ((PK3)made).ConvertToPK4(); // Pal Park
        cart = gen4Carts[rng.Next(gen4Carts.Length)];
        (baseId, mutId) = ("p", "q");
    }
    else
    {
        var pk4 = (PK4)made.Clone();
        PK4? shaped = rng.Next(4) == 0 ? Reshape4(pk4, rng) : null;
        result = shaped ?? Grow4(pk4, rng, cart, gen4Carts, out cart);
        (baseId, mutId) = ("g", "h");
    }
    EmitKept(baseId, mutId, cart, result);
}

// Emits a grown record PKHeX accepts, then its mutations.
void EmitKept(string baseId, string mutId, GameVersion cart, PK4 result)
{
    result.RefreshChecksum();
    if (!Analyze(result, cart).Valid)
        return; // keep only grown records PKHeX accepts
    Emit($"{baseId}{count++}", cart, result);
    for (int m = 0; m < mutations; m++)
    {
        var copy = (PK4)result.Clone();
        var name = Mutate4(copy, rng);
        Emit($"{mutId}{count++}-{name}", cart, copy);
    }
}

// Other legitimate shapes of a base record: an unhatched egg, a Cute Charm lead capture
// or a Poke Radar chain shiny. Null when the record has no such shape.
static PK4? Reshape4(PK4 pk, Random rng)
{
    var la = new LegalityAnalysis(pk);
    if (la.EncounterMatch is EncounterEgg4)
    {
        // Still an egg: received from the Day-Care man (or by trade), not hatched yet.
        pk.IsEgg = true;
        pk.MetLocation = rng.Next(4) == 0 ? Locations.LinkTrade4 : (ushort)0;
        if (pk.MetLocation == 0)
            pk.MetDate = null;
        pk.MetLevel = 0;
        pk.Nickname = SpeciesName.GetEggName(pk.Language, 4);
        pk.IsNicknamed = false;
        pk.OriginalTrainerFriendship = (byte)rng.Next(0, pk.PersonalInfo.HatchCycles + 1);
        return pk;
    }
    if (la.EncounterMatch is not EncounterSlot4 slot)
        return null;
    var gr = pk.PersonalInfo.Gender;
    bool hgss = pk.Version is GameVersion.HG or GameVersion.SS;
    for (int attempt = 0; attempt < 400; attempt++)
    {
        var copy = (PK4)pk.Clone();
        var seed = (uint)rng.NextInt64(0, 1L << 32);
        uint pid;
        var radar = !hgss && rng.Next(2) == 0;
        if (radar)
        {
            pid = ClassicEraRNG.GetChainShinyPID(ref seed, pk.ID32);
        }
        else
        {
            if (gr is 0 or 254 or 255)
                return null;
            var rand = LCRNG.Prev(seed) >> 16; // the nature call right before the IVs
            var nature = hgss ? rand % 25 : rand / 0xA3E;
            pid = rng.Next(2) == 0 ? CuteCharm4.GetPIDFemale(nature) : CuteCharm4.GetPIDMale(gr, nature);
        }
        var iv1 = LCRNG.Next16(ref seed);
        var iv2 = LCRNG.Next16(ref seed);
        copy.PID = pid;
        copy.Gender = EntityGender.GetFromPIDAndRatio(pid, gr);
        copy.RefreshAbility((int)(pid & 1));
        copy.IV_HP = (int)(iv1 & 31); copy.IV_ATK = (int)((iv1 >> 5) & 31); copy.IV_DEF = (int)((iv1 >> 10) & 31);
        copy.IV_SPE = (int)(iv2 & 31); copy.IV_SPA = (int)((iv2 >> 5) & 31); copy.IV_SPD = (int)((iv2 >> 10) & 31);
        copy.RefreshChecksum();
        var want = radar ? PIDType.ChainShiny : PIDType.CuteCharm;
        if (MethodFinder.Analyze(copy).Type == want && new LegalityAnalysis(copy).Valid)
            return copy;
    }
    return null;
}

// Plays a legitimate Gen IV record forward the way a player would.
static PK4 Grow4(PK4 pk, Random rng, GameVersion cart, GameVersion[] carts, out GameVersion newCart)
{
    newCart = cart;
    if (pk.IsEgg)
    {
        // Hatch in a town of the origin game.
        var towns = Enumerable.Range(1, 234).Select(l => (ushort)l)
            .Where(l => EggHatchLocation4.IsValidMet4(l, pk.Version)).ToArray();
        pk.IsEgg = false;
        pk.MetLocation = towns[rng.Next(towns.Length)];
        pk.MetLevel = 0;
        pk.OriginalTrainerFriendship = 120;
        pk.ClearNickname();
    }
    var level = (int)pk.CurrentLevel;
    var target = Math.Min(100, level + rng.Next(0, 41));
    if (target > level)
        SetLevel(pk, target, rng);

    // Evolution (one or two stages).
    for (int stage = 0; stage < 2 && rng.Next(4) != 0; stage++)
    {
        var evos = EvolutionTree.Evolves4.Forward.GetForward(pk.Species, pk.Form).Span;
        if (evos.Length == 0)
            break;
        var e = evos[rng.Next(evos.Length)];
        if (e.Method is EvolutionType.LevelUpShedinja)
            break;
        if (e.Method.IsLevelUpRequired)
        {
            var needed = Math.Max(e.Level, (byte)(pk.MetLevel + 1));
            if (needed > 100)
                break;
            if (pk.CurrentLevel < needed)
                SetLevel(pk, needed, rng);
        }
        if (e.Method is EvolutionType.LevelUpFriendship or EvolutionType.LevelUpFriendshipMorning or EvolutionType.LevelUpFriendshipNight)
            pk.OriginalTrainerFriendship = 255;
        if (e.Method is EvolutionType.Trade or EvolutionType.TradeHeldItem)
            newCart = TradeTarget(pk, newCart, carts, rng);
        var nicknamed = pk.IsNicknamed;
        pk.Species = e.Species;
        pk.Form = e.GetDestinationForm(pk.Form);
        pk.RefreshAbility((int)(pk.PID & 1));
        if (!nicknamed)
            pk.ClearNickname();
    }

    // Moves: keep, latest level-up moves, or a mix with TMs/HMs and tutors.
    var choice = rng.Next(3);
    if (choice != 0)
    {
        Span<ushort> moves = stackalloc ushort[4];
        var types = choice == 1 ? MoveSourceType.LevelUp
            : MoveSourceType.LevelUp | MoveSourceType.Machine | MoveSourceType.TypeTutor | MoveSourceType.SpecialTutor;
        new LegalityAnalysis(pk).GetSuggestedCurrentMoves(moves, types);
        if (moves[0] != 0)
        {
            pk.SetMoves(moves);
            pk.HealPP();
        }
    }

    // EV training (battles need EXP gained since the encounter).
    if (rng.Next(4) != 0)
    {
        var remaining = 510;
        foreach (var stat in Enumerable.Range(0, 6).OrderBy(_ => rng.Next()))
        {
            if (rng.Next(3) == 0)
                continue;
            var ev = rng.Next(0, Math.Min(255, remaining) + 1);
            pk.SetEV(stat, ev);
            remaining -= ev;
        }
        if (remaining == 0 && rng.Next(2) == 0)
            pk.RibbonEffort = true;
    }

    // Held item (Giratina and Arceus forms follow their item, so they keep theirs).
    if (rng.Next(3) == 0 && pk.Species is not ((ushort)Species.Giratina or (ushort)Species.Arceus))
    {
        int item;
        do item = rng.Next(1, 537); while (!ItemRestrictions.IsHeldItemAllowed(item, EntityContext.Gen4));
        pk.HeldItem = item;
    }

    // Pokerus: infected (days within the strain's range) or cured.
    if (rng.Next(6) == 0)
    {
        var strain = rng.Next(1, 16);
        pk.PokerusState = (byte)((strain << 4) | rng.Next(0, (strain & 3) + 2));
    }

    // Ribbons any Gen IV Pokemon can earn.
    if (rng.Next(8) == 0)
        pk.RibbonChampionSinnoh = true;
    if (rng.Next(8) == 0)
        pk.RibbonFootprint = true;
    if (rng.Next(8) == 0)
        pk.RibbonAlert = true;

    // Traded into another Gen IV game's save.
    if (rng.Next(2) == 0)
        newCart = TradeTarget(pk, newCart, carts, rng);
    return pk;
}

// A trade partner's game must have the species and form (D/P lack the Platinum forms);
// the HG/SS event Spiky-eared Pichu cannot be traded at all.
static GameVersion TradeTarget(PK4 pk, GameVersion current, GameVersion[] carts, Random rng)
{
    if (pk is { Species: (ushort)Species.Pichu, Form: 1 })
        return current;
    var target = carts[rng.Next(carts.Length)];
    PersonalTable4 table = target switch
    {
        GameVersion.D or GameVersion.P => PersonalTable.DP,
        GameVersion.Pt => PersonalTable.Pt,
        _ => PersonalTable.HGSS,
    };
    return table.IsPresentInGame(pk.Species, pk.Form) ? target : current;
}

static void SetLevel(PK4 pk, int level, Random rng)
{
    var growth = pk.PersonalInfo.EXPGrowth;
    var exp = Experience.GetEXP((byte)level, growth);
    if (level < 100)
        exp += (uint)rng.Next(0, (int)(Experience.GetEXP((byte)(level + 1), growth) - exp));
    pk.EXP = exp;
}

static void Emit(string id, GameVersion cart, PKM pk, bool refreshChecksum = true)
{
    if(refreshChecksum)pk.RefreshChecksum();
    var la = Analyze(pk, cart);
    var enc = la.EncounterMatch;
    var invalid = la.Results.Where(r => !r.Valid).Select(r => $"{r.Identifier}:{r.Result}")
        .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
            .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"))
        .Concat(la.Results.Where(r => r.Judgement == Severity.Fishy).Select(r => $"~{r.Identifier}:{r.Result}"));
    var len = pk is PK3 ? 80 : 136;
    var hex = Convert.ToHexString(pk.Data[..len]);
    Console.WriteLine($"{id} {cart} {hex} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species} ; {string.Join(' ', invalid)}");
}

// A PK4 is judged as PKHeX judges a slot of that game's save: with the save's personal
// table (D/P lack the Platinum forms such as Rotom's appliances and Giratina-Origin).
static LegalityAnalysis Analyze(PKM pk, GameVersion cart) => pk is PK4
    ? new LegalityAnalysis(pk, cart switch
    {
        GameVersion.D or GameVersion.P => PersonalTable.DP,
        GameVersion.Pt => PersonalTable.Pt,
        _ => PersonalTable.HGSS,
    })
    : new LegalityAnalysis(pk);

static string Mutate3(PK3 pk, Random rng)
{
    switch (rng.Next(22))
    {
        case 0: pk.Ball = (byte)rng.Next(1, 13); return "ball";
        case 1: pk.MetLevel = (byte)Math.Clamp(pk.MetLevel + rng.Next(-3, 4), 0, 100); return "metlevel";
        case 2: pk.AbilityBit = !pk.AbilityBit; return "abilitybit";
        case 3: pk.PID ^= 1; return "pidlowbit";
        case 4: pk.PID = (uint)rng.NextInt64(0, 1L << 32); return "pid";
        case 5: pk.MetLocation = (ushort)new[] { 0, 16, 17, 32, 87, 88, 101, 126, 136, 202, 253, 254, 255 }[rng.Next(13)]; return "metlocation";
        case 6: pk.SetMove(rng.Next(4) == 0 ? 3 : 0, (ushort)rng.Next(1, 355)); return "move";
        case 7: pk.SetEV(rng.Next(6), rng.Next(256)); return "ev";
        case 8: pk.SetIV(rng.Next(6), rng.Next(32)); return "iv";
        case 9: pk.FatefulEncounter = !pk.FatefulEncounter; return "fateful";
        case 10: pk.Version = (GameVersion)new byte[] { 1, 2, 3, 4, 5, 15 }[rng.Next(6)]; return "version";
        case 11: pk.Language = rng.Next(0, 8); return "language";
        case 12: pk.HeldItem = rng.Next(0, 377); return "item";
        case 13: pk.EXP = (uint)Math.Max(0, (int)pk.EXP + rng.Next(-2000, 2000)); return "exp";
        case 14: pk.Move1_PPUps = rng.Next(0, 4); pk.Move1_PP = rng.Next(0, 70); return "pp";
        case 15: pk.Data[0x4C + rng.Next(3)] |= (byte)(1 << rng.Next(8)); return "ribbon";
        case 16: pk.ContestCool = (byte)rng.Next(1, 256); return "contest";
        case 17: pk.Species = (ushort)Math.Clamp(pk.Species + (rng.Next(2) == 0 ? -1 : 1), 1, 386); return "species";
        case 18: pk.Data[0x44] = (byte)rng.Next(256); return "pokerus";
        case 19: pk.OriginalTrainerGender = (byte)(pk.OriginalTrainerGender ^ 1); return "otgender";
        case 20: pk.IsEgg = !pk.IsEgg; return "egg";
        default: pk.TID16 = (ushort)rng.Next(65536); return "tid";
    }
}

static string Mutate4(PK4 pk, Random rng)
{
    switch (rng.Next(28))
    {
        case 0: pk.Ball = (byte)rng.Next(1, 25); return "ball";
        case 1: pk.MetLevel = (byte)Math.Clamp(pk.MetLevel + rng.Next(-3, 4), 0, 100); return "metlevel";
        case 2: pk.Ability = rng.Next(1, 124); return "ability";
        case 3: pk.PID ^= 1; return "pidlowbit";
        case 4: pk.Gender = (byte)(pk.Gender ^ 1); return "gender";
        case 5: pk.PID = (uint)rng.NextInt64(0, 1L << 32); return "pid";
        case 6: pk.MetLocation = (ushort)new[] { 0, 1, 16, 50, 55, 86, 126, 149, 182, 233, 2000, 2002, 3001, 3002 }[rng.Next(14)]; return "metlocation";
        case 7: pk.SetMove(rng.Next(4) == 0 ? 3 : 0, (ushort)rng.Next(1, 468)); return "move";
        case 8: pk.SetEV(rng.Next(6), rng.Next(256)); return "ev";
        case 9: pk.SetIV(rng.Next(6), rng.Next(32)); return "iv";
        case 10: pk.FatefulEncounter = !pk.FatefulEncounter; return "fateful";
        case 11: pk.EggLocation = (ushort)new[] { 0, 2000, 2002, 3001 }[rng.Next(4)]; return "egglocation";
        case 12: pk.Version = (GameVersion)new[] { 1, 2, 3, 4, 5, 7, 8, 10, 11, 12 }[rng.Next(10)]; return "version";
        case 13: pk.Form = (byte)rng.Next(4); return "form";
        case 14: pk.Language = rng.Next(0, 9); return "language";
        case 15: pk.HeldItem = rng.Next(0, 537); return "item";
        case 16: pk.EXP = (uint)Math.Max(0, (int)pk.EXP + rng.Next(-2000, 2000)); return "exp";
        case 17: pk.MetDate = new DateOnly(2000 + rng.Next(0, 99), rng.Next(1, 13), rng.Next(1, 29)); return "metdate";
        case 18: pk.Move1_PPUps = rng.Next(0, 4); pk.Move1_PP = rng.Next(0, 70); return "pp";
        case 19: pk.Data[0x24 + rng.Next(4)] |= (byte)(1 << rng.Next(8)); return "ribbon";
        case 20: pk.ContestCool = (byte)rng.Next(1, 256); return "contest";
        case 21: pk.Species = (ushort)Math.Clamp(pk.Species + (rng.Next(2) == 0 ? -1 : 1), 1, 493); return "species";
        case 22: pk.BallHGSS = (byte)rng.Next(0, 25); return "ballhgss";
        case 23: pk.PokerusState = (byte)rng.Next(256); return "pokerus";
        case 24: pk.ShinyLeaf = rng.Next(0, 64); return "shinyleaf";
        case 25: pk.MetLocationExtended = (ushort)rng.Next(0, 3); return "metlocext";
        case 26: pk.IsEgg = !pk.IsEgg; return "egg";
        default: pk.TID16 = (ushort)rng.Next(65536); return "tid";
    }
}
