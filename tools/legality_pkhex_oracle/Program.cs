// Host-only unmodified PKHeX.Core oracle; NEVER deployed on Nintendo Switch.
using PKHeX.Core;
using System.Security.Cryptography;
using System.Text.Json;
if (args.Length != 1) {
    Console.Error.WriteLine("usage: oracle vectors.tsv"); return 2;
}
try {
    var seen = new HashSet<string>(StringComparer.Ordinal);
    foreach (var line in File.ReadLines(args[0])) {
        var parts = line.Split('\t');
        if (parts.Length != 7 || !seen.Add(parts[0]) || parts[0].Length == 0)
            throw new InvalidDataException("Invalid/duplicate native vector");
        byte[] input=Convert.FromHexString(parts[1]);
        if (input.Length is not (136 or 220))
            throw new InvalidDataException("Not a bounded native PK5");
        var originalHash=Convert.ToHexString(SHA256.HashData(input)).ToLowerInvariant();
        var pk=new PK5(input.ToArray()); // unmodified source bytes
        int totalEV=pk.EV_HP+pk.EV_ATK+pk.EV_DEF+pk.EV_SPE+pk.EV_SPA+pk.EV_SPD;
        object? legality=null;
        if (parts[2]=="Parsed") {
            var report=new LegalityAnalysis(pk);
            legality=new {parsed=report.Parsed,valid=report.Valid,
                checks=report.Results.Select(c=>new {
                    category=c.Identifier.ToString(),severity=c.Judgement.ToString(),
                    code=c.Result.ToString(),value=c.Value}).ToArray()};
        }
        Console.WriteLine(JsonSerializer.Serialize(new {
            id=parts[0],rawSha256=originalHash,nativeStatus=parts[2],
            sanity=(int)pk.Sanity,checksumValid=pk.ChecksumValid,
            species=(int)pk.Species,nature=(int)pk.Nature,
            evTotal=totalEV,pid=pk.PID,originalPKHeX=legality,
            synthetic=true,sourceHistoryAuthenticated=false}));
    }
    return 0;
} catch(Exception ex) {
    Console.Error.WriteLine("PKHeX oracle fail closed: "+ex);
    return 1;
}
