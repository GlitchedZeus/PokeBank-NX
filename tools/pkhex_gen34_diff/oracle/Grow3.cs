// Grown Gen III corpus for the PKHeX differential.
//
//   dotnet PKHeXGen34Oracle.dll grow SEED GROWTHS BASE_FILE
//
// BASE_FILE is the output of `generate SEED 0 3`. For every base PK3 PKHeX accepts, GROWTHS
// grown copies are made by replaying ordinary play on it: EXP gain (battle EXP or Rare Candy),
// level-up moves from the origin or receiving game's learnset, EV training, friendship,
// level/stone/trade/friendship evolution (renaming an unnicknamed Pokemon as the game does),
// held items and trades to a save of another Gen III cartridge (whose trainer is not the OT).
// Each step is kept only while PKHeX still judges the Pokemon VALID, so every emitted record
// is PKHeX-valid. Lines use the generate format, with the receiving cartridge as <cartridge>.
using PKHeX.Core;

static class Grow3
{
    private static readonly GameVersion[] Carts = [GameVersion.R, GameVersion.S, GameVersion.E, GameVersion.FR, GameVersion.LG];

    public static int Run(string[] args)
    {
        if (args.Length < 4)
        {
            Console.Error.WriteLine("usage: grow SEED GROWTHS BASE_FILE");
            return 2;
        }
        var rng = new Random(int.Parse(args[1]));
        var growths = int.Parse(args[2]);
        var count = 0;
        foreach (var line in File.ReadLines(args[3]))
        {
            var p = line.Split(' ');
            if (p.Length < 4 || !p[0].StartsWith('b') || p[3] != "VALID" || p[2].Length != 160)
                continue;
            var bytes = Convert.FromHexString(p[2]);
            var baseCart = Enum.Parse<GameVersion>(p[1]);
            for (int g = 0; g < growths; g++)
            {
                PK3 pk = new(bytes.ToArray());
                if (pk.IsEgg)
                    break;
                var cart = baseCart;
                var steps = new List<string>();
                var ops = rng.Next(1, 6);
                for (int i = 0; i < ops; i++)
                {
                    var trial = pk.Clone();
                    var trialCart = cart;
                    string? name = rng.Next(7) switch
                    {
                        0 or 1 => GainExp(trial, rng),
                        2 => LearnMove(trial, rng, trialCart),
                        3 => TrainEVs(trial, rng),
                        4 => Evolve(trial, rng),
                        5 => HoldItem(trial, rng),
                        _ => Trade(ref trialCart, rng),
                    };
                    if (name is null)
                        continue;
                    trial.RefreshChecksum();
                    if (!new LegalityAnalysis(trial).Valid)
                        continue;
                    pk = trial;
                    cart = trialCart;
                    steps.Add(name);
                }
                if (steps.Count == 0)
                    continue;
                Emit($"g{count++}-{string.Join('+', steps)}", cart, pk);
            }
        }
        return 0;
    }

    private static string? GainExp(PK3 pk, Random rng)
    {
        var growth = pk.PersonalInfo.EXPGrowth;
        var level = Experience.GetLevel(pk.EXP, growth);
        if (level >= Experience.MaxLevel)
            return null;
        var target = (byte)Math.Min(Experience.MaxLevel, level + rng.Next(1, 41));
        var floor = Experience.GetEXP(target, growth);
        if (rng.Next(4) == 0 || target == Experience.MaxLevel)
        {
            pk.EXP = floor; // Rare Candy lands exactly on the level threshold.
            return "candy";
        }
        var next = Experience.GetEXP((byte)(target + 1), growth);
        pk.EXP = floor + (uint)rng.NextInt64(0, next - floor);
        return "exp";
    }

    private static string? LearnMove(PK3 pk, Random rng, GameVersion cart)
    {
        var game = rng.Next(2) == 0 ? pk.Version : cart;
        Learnset learn = game switch
        {
            GameVersion.E or GameVersion.CXD => LearnSource3E.Instance.GetLearnset(pk.Species, 0), // Colosseum/XD learn as Emerald
            GameVersion.FR => LearnSource3FR.Instance.GetLearnset(pk.Species, 0),
            GameVersion.LG => LearnSource3LG.Instance.GetLearnset(pk.Species, 0),
            _ => LearnSource3RS.Instance.GetLearnset(pk.Species, 0),
        };
        var level = Experience.GetLevel(pk.EXP, pk.PersonalInfo.EXPGrowth);
        var pool = learn.GetMoveRange(level).ToArray();
        ushort[] moves = [pk.Move1, pk.Move2, pk.Move3, pk.Move4];
        var fresh = pool.Where(m => !moves.Contains(m)).ToArray();
        if (fresh.Length == 0)
            return null;
        var move = fresh[rng.Next(fresh.Length)];
        var empty = Array.IndexOf(moves, (ushort)0);
        var slot = empty >= 0 ? empty : rng.Next(4);
        moves[slot] = move;
        var ppUps = new[] { pk.Move1_PPUps, pk.Move2_PPUps, pk.Move3_PPUps, pk.Move4_PPUps };
        ppUps[slot] = 0;
        pk.Move1 = moves[0]; pk.Move2 = moves[1]; pk.Move3 = moves[2]; pk.Move4 = moves[3];
        pk.Move1_PPUps = ppUps[0]; pk.Move2_PPUps = ppUps[1]; pk.Move3_PPUps = ppUps[2]; pk.Move4_PPUps = ppUps[3];
        pk.HealPP();
        return "move";
    }

    private static string? TrainEVs(PK3 pk, Random rng)
    {
        Span<int> evs = stackalloc int[6];
        pk.GetEVs(evs);
        var total = 0;
        foreach (var e in evs)
            total += e;
        var gain = rng.Next(1, 120);
        for (int i = 0; i < gain && total < 510; i++)
        {
            var s = rng.Next(6);
            if (evs[s] >= 255)
                continue;
            evs[s]++;
            total++;
        }
        pk.SetEVs(evs);
        return "ev";
    }

    private static string? Evolve(PK3 pk, Random rng)
    {
        var forward = EvolutionTree.Evolves3.Forward.GetForward(pk.Species, 0).ToArray();
        if (forward.Length == 0)
            return null;
        var evo = forward[rng.Next(forward.Length)];
        if (evo.Species > 386)
            return null;
        var growth = pk.PersonalInfo.EXPGrowth;
        var level = Experience.GetLevel(pk.EXP, growth);
        if (evo.Level != 0 || evo.LevelUp != 0)
        {
            var target = (byte)Math.Min(Experience.MaxLevel, Math.Max(level + 1, evo.Level));
            if (target > level)
                pk.EXP = Experience.GetEXP(target, growth) + (uint)rng.Next(0, 2);
        }
        pk.OriginalTrainerFriendship = 255;
        var defaultName = !pk.IsNicknamed;
        pk.Species = evo.Species;
        if (defaultName)
            pk.Nickname = SpeciesName.GetSpeciesNameGeneration(evo.Species, pk.Language, 3);
        return "evolve";
    }

    private static string? HoldItem(PK3 pk, Random rng)
    {
        pk.HeldItem = (ushort)rng.Next(1, 377);
        return "item";
    }

    private static string? Trade(ref GameVersion cart, Random rng)
    {
        var next = Carts[rng.Next(Carts.Length)];
        if (next == cart)
            return null;
        cart = next;
        return "trade";
    }

    private static void Emit(string id, GameVersion cart, PK3 pk)
    {
        pk.RefreshChecksum();
        var la = new LegalityAnalysis(pk);
        var invalid = la.Results.Where(r => !r.Valid).Select(r => $"{r.Identifier}:{r.Result}")
            .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
                .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
        var hex = Convert.ToHexString(pk.Data[..80]);
        Console.WriteLine($"{id} {cart} {hex} {(la.Valid ? "VALID" : "INVALID")} {la.EncounterMatch.GetType().Name}:{la.EncounterMatch.Species} ; {string.Join(' ', invalid)}");
    }
}
