// Reads gen_corpus.py (PK1) / gen_corpus2.py (PK2) lines on stdin and prints PKHeX's verdict for each:
//   <id> VALID|INVALID <encounter> ; <identifier>:<result code> ...
// GB cartridge-era settings match PokeBank NX: Time Capsule tradebacks, Stadium and
// Stadium 2 allowed, 3DS Virtual Console rules off.
using PKHeX.Core;

ParseSettings.AllowEraCartGB = true;
var includeFishy = args.Length == 1 && args[0] == "--fishy";
if (args.Length != 0 && !includeFishy) return 2;

// A line the corpus protocol does not allow is an error, never a silent skip: compare.py
// must see a verdict for every corpus record or fail the run.
string? line;
var lineNumber = 0;
while ((line = Console.ReadLine()) is not null)
{
    ++lineNumber;
    var parts = line.Split(' ');
    if (parts.Length is not (6 or 7))
    {
        Console.Error.WriteLine($"oracle: corpus line {lineNumber} has {parts.Length} fields");
        return 2;
    }
    var body = Convert.FromHexString(parts[3]);
    var ot = Convert.FromHexString(parts[4]);
    var nick = Convert.FromHexString(parts[5]);
    if (!((body.Length == 44 && parts.Length == 6) || (body.Length == 48 && parts.Length == 7 && parts[6] is "0" or "1"))
        || ot.Length != nick.Length || ot.Length is not (6 or 11))
    {
        Console.Error.WriteLine($"oracle: corpus line {lineNumber} has a malformed record");
        return 2;
    }
    PKM pk = body.Length == 48
        ? new PK2(body, ot, nick) { IsEgg = parts[6] == "1" }
        : new PK1(body, ot, nick);
    var la = new LegalityAnalysis(pk);
    var enc = la.EncounterMatch;
    // Move verdicts live in Info.Moves, not in Results.
    var invalid = la.Results.Where(r => !r.Valid || (includeFishy && r.Judgement == Severity.Fishy))
        .Select(r => $"{(r.Judgement == Severity.Fishy ? "~" : "")}{r.Identifier}:{r.Result}")
        .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
            .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
    Console.WriteLine($"{parts[0]} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species}:{enc.Version}:{enc.LevelMin} ; {string.Join(' ', invalid)}");
}
return 0;
