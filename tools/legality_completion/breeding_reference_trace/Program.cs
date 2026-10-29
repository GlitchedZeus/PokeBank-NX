// Read-only additional observation of an unmodified source-built assembly.
// Does not change either original oracle's validity, bytes or denominator.
using PKHeX.Core;
using System.Security.Cryptography;
using System.Text.Json;

if (args[0] == "bw2-index")
{
    var rows = new List<object>();
    for (ushort species = 1; species <= 649; species++)
    {
        var learn = LearnSource5B2W2.Instance.GetLearnset(species, 0);
        var levels = learn.GetAllLevels().ToArray();
        var moves = learn.GetAllMoves().ToArray();
        var initial = moves.Where((_, i) => levels[i] <= 1).Distinct().ToArray();
        var egg = LearnSource5BW.Instance.GetEggMoves(species, 0).ToArray();
        var pi = PersonalTable.B2W2[species];
        var machines = new List<ushort>();
        var machineList = PersonalInfo5BW.MachineMoves;
        // Deliberately include every compatible TM/HM entry. This is a
        // necessary-condition superset, not a claim that every item was issued.
        for (var i = 0; i < machineList.Length; i++)
            if (pi.GetIsLearnTM(i)) machines.Add(machineList[i]);
        rows.Add(new { species, levels = moves, initial, egg, machines });
    }
    var pidgey = LearnSource5B2W2.Instance.GetLearnset(16, 0);
    Console.WriteLine(JsonSerializer.Serialize(new
    {
        rows,
        pidgey_names = pidgey.GetAllMoves().ToArray().Select(m => ((Move)m).ToString()).ToArray(),
        pidgey_levels = pidgey.GetAllLevels().ToArray().Select(level => (int)level).ToArray(),
        machine_table = PersonalInfo5BW.MachineMoves.ToArray(),
        move_names = Enumerable.Range(0, 560).Select(m => ((Move)m).ToString()).ToArray(),
    }));
    return;
}

foreach (var line in File.ReadLines(args[0]))
{
    var fields = line.Split(' ');
    var bytes = Convert.FromHexString(fields[1]);
    var pk = new PK5(bytes);
    var analysis = new LegalityAnalysis(pk);
    var buffer = new ChainBreedStep[10];
    var trace = new ChainBreedTrace(buffer) { Flags = ChainBreedEraFlags.None };
    var chain = ChainBreedLegality.IsValid(pk.Species, pk.Form, pk.Version, pk.Moves, ref trace);
    // Observe the full report's actual subset, preserving its original Valid.
    // EggVerifier.GatherInheritedMoves omits machine results. This additional
    // trace is not a replacement oracle or a comparison exception.
    var personal = GameData.GetPersonal(pk.Version)[pk.Species, pk.Form];
    var includeLevel = personal.OnlyFemale || Breeding.IsGenderSpeciesDetermination(pk.Species);
    var methods = new List<string>();
    var inherited = new List<ushort>();
    var fullMoves = pk.Moves;
    for (var i = 0; i < 4; i++)
    {
        var method = analysis.Info.Moves[i].Info.Method;
        methods.Add(method.ToString());
        if (fullMoves[i] != 0 && (method == LearnMethod.EggMove || (includeLevel && method == LearnMethod.InheritLevelUp)))
            inherited.Add(fullMoves[i]);
    }
    var gathered = inherited.ToArray();
    bool? gatheredChain = gathered.Length == 0 || (gathered.Length == 1 && !includeLevel)
        ? null : ChainBreedLegality.IsValid(pk.Species, pk.Form, pk.Version, ChainBreedEraFlags.None, gathered);
    Console.WriteLine(JsonSerializer.Serialize(new
    {
        id = fields[0], sha256 = Convert.ToHexStringLower(SHA256.HashData(bytes)),
        species = pk.Species, version = pk.Version, moves = pk.Moves,
        move_names = fullMoves.Select(m => ((Move)m).ToString()).ToArray(),
        full_report_valid = analysis.Valid, chain_valid = chain,
        move_methods = methods, gathered_moves = gathered, gathered_chain_valid = gatheredChain,
        steps = trace.Steps.ToArray(),
    }));
}
