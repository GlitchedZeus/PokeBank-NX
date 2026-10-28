// Read-only additional observation of an unmodified source-built assembly.
// Does not change either original oracle's validity, bytes or denominator.
using PKHeX.Core;
using System.Security.Cryptography;
using System.Text.Json;

foreach (var line in File.ReadLines(args[0]))
{
    var fields = line.Split(' ');
    var bytes = Convert.FromHexString(fields[1]);
    var pk = new PK5(bytes);
    var analysis = new LegalityAnalysis(pk);
    var buffer = new ChainBreedStep[10];
    var trace = new ChainBreedTrace(buffer) { Flags = ChainBreedEraFlags.None };
    var chain = ChainBreedLegality.IsValid(pk.Species, pk.Form, pk.Version, pk.Moves, ref trace);
    Console.WriteLine(JsonSerializer.Serialize(new
    {
        id = fields[0], sha256 = Convert.ToHexStringLower(SHA256.HashData(bytes)),
        species = pk.Species, version = pk.Version, moves = pk.Moves,
        full_report_valid = analysis.Valid, chain_valid = chain,
        steps = trace.Steps.ToArray(),
    }));
}
