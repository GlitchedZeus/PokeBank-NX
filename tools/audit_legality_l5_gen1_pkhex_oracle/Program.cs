// L5-only executable reference. Runs ORIGINAL pinned PKHeX.Core, not a
// translated source model. No retail history or original save is asserted.
using System.Text.Json;
using PKHeX.Core;
using System.Linq;

if (args.Length != 1 || !File.Exists(args[0]))
{
    Console.Error.WriteLine("usage: L5Gen1PKHeXOracle <synthetic-vectors.json>");
    return 2;
}
using var document = JsonDocument.Parse(File.ReadAllText(args[0]));
var root = document.RootElement;
if (root.GetProperty("schema").GetString() != "L5_GEN1_RBY_NATIVE_VECTORS_V1")
    throw new InvalidDataException("Vector schema mismatch");
var cases = root.GetProperty("cases").EnumerateArray().ToArray();
if (cases.Length < 12 || cases.Length > 256)
    throw new InvalidDataException("Unexpected vector count");
var unique = new HashSet<string>(StringComparer.Ordinal);
foreach (var value in cases)
{
    var id = value.GetProperty("id").GetString() ?? throw new InvalidDataException("Missing id");
    if (!unique.Add(id)) throw new InvalidDataException("Duplicate case id");
    var species = value.GetProperty("species").GetUInt16();
    var level = value.GetProperty("level").GetByte();
    var move = value.GetProperty("move").GetUInt16();
    var dv = value.GetProperty("dv16").GetUInt16();
    var experience = value.GetProperty("experience").GetUInt32();
    var statExperience = value.GetProperty("stat_experience").GetUInt16();
    var statExpHP = value.GetProperty("stat_exp_hp").GetUInt16();
    var statExpAttack = value.GetProperty("stat_exp_attack").GetUInt16();
    var statExpDefense = value.GetProperty("stat_exp_defense").GetUInt16();
    var statExpSpeed = value.GetProperty("stat_exp_speed").GetUInt16();
    var statExpSpecial = value.GetProperty("stat_exp_special").GetUInt16();
    if (statExpHP != statExperience) throw new InvalidDataException("Stat Experience HP alias mismatch");
    if (species is < 1 or > 151 || level is < 1 or > 100 || move > 255)
        throw new InvalidDataException("Out of bounded PK1 vector domain");
    // PK1's native binary lacks the originating game name. The source label
    // is comparison metadata ONLY and cannot certify R vs B vs Yellow.
    var pk = new PK1
    {
        Species = species, Stat_Level = level, EXP = experience,
        TID16 = 1234, Move1 = move, Move1_PP = move == 0 ? 0 : 35,
        DV16 = dv, OriginalTrainerName = "L5",
        EV_HP = statExpHP, EV_ATK = statExpAttack, EV_DEF = statExpDefense,
        EV_SPE = statExpSpeed, EV_SPC = statExpSpecial,
    };
    pk.SetNotNicknamed(2);
    var originalSpecies = pk.Species;
    var originalMove = pk.Move1;
    var originalDV = pk.DV16;
    var analysis = new LegalityAnalysis(pk);
    bool abilityInvalid = analysis.Results.Any(x =>
        x.Identifier == CheckIdentifier.Ability && x.Judgement == Severity.Invalid);
    bool genderInvalid = analysis.Results.Any(x =>
        x.Identifier == CheckIdentifier.Gender && x.Judgement == Severity.Invalid);
    // Do not convert the global Valid flag into authenticated legal status.
    // PKHeX's encounter verifier may reject a synthetic origin.
    var result = new
    {
        schema = "L5_GEN1_LIVE_RUNTIME_RESULT_V1", engine = "PKHEX_CORE",
        pkhex_revision = "6501f0ab46e8f8ca048539dbaf8cae8cb104e722",
        id, source = value.GetProperty("source").GetString(), original_source = "RBY_VERSION_UNKNOWN",
        execution_mode = "actual_original_pkhex_core",
        species = originalSpecies, move = originalMove, dv16 = originalDV,
        dv_hp = pk.IV_HP, dv_attack = pk.IV_ATK, dv_defense = pk.IV_DEF,
        dv_speed = pk.IV_SPE, dv_special = pk.IV_SPA,
        stat_exp_hp = pk.EV_HP, stat_exp_attack = pk.EV_ATK,
        stat_exp_defense = pk.EV_DEF, stat_exp_speed = pk.EV_SPE,
        stat_exp_special = pk.EV_SPC,
        pk1_ability = pk.Ability, pk1_gender = pk.Gender,
        ability_invalid = abilityInvalid, gender_invalid = genderInvalid,
        parsed = analysis.Parsed, pkhex_global_valid = analysis.Valid,
        invalid_check_identifiers = analysis.Results
            .Where(x => x.Judgement == Severity.Invalid)
            .Select(x => x.Identifier.ToString()).Distinct().OrderBy(x => x).ToArray(),
        original_retail_history_authenticated = false,
    };
    Console.WriteLine(JsonSerializer.Serialize(result));
}
return 0;
