// Reads gen_corpus.py (PK1) / gen_corpus2.py (PK2) lines on stdin and prints PKHeX's verdict for each:
//   <id> VALID|INVALID <encounter> ; <identifier>:<result code> ...
// GB cartridge-era settings match PokeBank NX: Time Capsule tradebacks, Stadium and
// Stadium 2 allowed, 3DS Virtual Console rules off.
using PKHeX.Core;

ParseSettings.AllowEraCartGB = true;

string? line;
while ((line = Console.ReadLine()) is not null)
{
    var parts = line.Split(' ');
    if (parts.Length < 6)
        continue;
    var body = Convert.FromHexString(parts[3]);
    var ot = Convert.FromHexString(parts[4]);
    var nick = Convert.FromHexString(parts[5]);
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
