// Reads gen_corpus.py (PK1) / gen_corpus2.py (PK2) lines on stdin and prints PKHeX's verdict for each:
//   <id> VALID|INVALID <encounter> ; <identifier>:<result code> ...
// GB cartridge-era settings match PokeBank NX: Time Capsule tradebacks, Stadium and
// Stadium 2 allowed, 3DS Virtual Console rules off.
using PKHeX.Core;

ParseSettings.AllowEraCartGB = true;

string? line;
var ids = new HashSet<string>();
try
{
while ((line = Console.ReadLine()) is not null)
{
    var parts = line.Split(' ', StringSplitOptions.RemoveEmptyEntries);
    if (parts.Length is not (6 or 7) || !ids.Add(parts[0]))
        throw new FormatException("malformed or duplicate corpus record");
    var body = Convert.FromHexString(parts[3]);
    var ot = Convert.FromHexString(parts[4]);
    var nick = Convert.FromHexString(parts[5]);
    if (body.Length is not (44 or 48) ||
        parts[2] is not ("0" or "1" or "8") || (parts[2] == "8" && body.Length != 48))
        throw new FormatException("wrong body length or language flag");
    var nameLength = parts[2] == "1" ? 6 : 11;
    if (ot.Length != nameLength || nick.Length != nameLength ||
        (body.Length == 48 ? parts.Length != 7 || parts[6] is not ("0" or "1") : parts.Length != 6))
        throw new FormatException("wrong name length or egg flag");
    PKM pk = body.Length == 48
        ? new PK2(body, ot, nick) { IsEgg = parts.Length > 6 && parts[6] == "1" }
        : new PK1(body, ot, nick);
    var la = new LegalityAnalysis(pk);
    var enc = la.EncounterMatch;
    // Move verdicts live in Info.Moves, not in Results.
    var invalid = la.Results.Where(r => !r.Valid).Select(r => $"{r.Identifier}:{r.Result}")
        .Concat(la.Info.Moves.Select((m, i) => (m, i)).Where(x => !x.m.Valid)
            .Select(x => $"Move{x.i + 1}:{x.m.Info.Method}"));
    Console.WriteLine($"{parts[0]} {(la.Valid ? "VALID" : "INVALID")} {enc.GetType().Name}:{enc.Species}:{enc.Version}:{enc.LevelMin} ; {string.Join(' ', invalid)}");
}
}
catch (FormatException error)
{
    Console.Error.WriteLine($"Corpus integrity error: {error.Message}");
    Environment.ExitCode = 2;
}
