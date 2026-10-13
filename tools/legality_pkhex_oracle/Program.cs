// Host-only ORIGINAL PKHeX.Core oracle; NEVER deployed on Nintendo Switch.
// Source must be SHA-256 authenticated against owner's PKHeX-master.zip map.
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
        if (parts.Length is not (7 or 9) || parts[0].Length == 0 || !seen.Add(parts[0]))
            throw new InvalidDataException("Invalid or duplicate native vector");
        // Legacy 7-column Product-PK5 format is unchanged. Extended native
        // Gen VI/VII uses id, family, encoding, hex, status, species, nature,
        // evTotal, pid; encodings are explicit and *not* inferred from a save.
        bool extended = parts.Length == 9;
        string family = extended ? parts[1] : "PK5";
        string encoding = extended ? parts[2] : "encrypted";
        string status = extended ? parts[4] : parts[2];
        byte[] input = Convert.FromHexString(parts[extended ? 3 : 1]);
        bool validFormat = family switch {
            "PK5" => !extended && input.Length is (136 or 220),
            "PK6" or "PK7" => extended && input.Length is (232 or 260),
            _ => false,
        };
        if (!validFormat || (encoding != "encrypted" && encoding != "plain") ||
            (!extended && encoding != "encrypted"))
            throw new InvalidDataException("Unsupported native family/encoding/length");
        if (extended && encoding == "plain" && status != "Parsed")
            throw new InvalidDataException("Only parsed decrypted-vector evidence is supported");
        var originalHash = Convert.ToHexString(SHA256.HashData(input)).ToLowerInvariant();
        // PKHeX.Core's original constructors and PokeCrypto are invoked here.
        // No reimplementation or copied legality algorithms in this host.
        PKM pk = family switch {
            "PK5" => new PK5(input.ToArray()),
            "PK6" => new PK6(input.ToArray()),
            "PK7" => new PK7(input.ToArray()),
            _ => throw new InvalidDataException("Unreachable family"),
        };
        int sanity = pk switch {
            PK5 p => p.Sanity,
            PK6 p => p.Sanity,
            PK7 p => p.Sanity,
            _ => throw new InvalidDataException("Unreachable entity"),
        };
        int[] evs = [pk.EV_HP, pk.EV_ATK, pk.EV_DEF,
                     pk.EV_SPE, pk.EV_SPA, pk.EV_SPD];
        int totalEV = evs.Sum();
        object? legality = null;
        if (status == "Parsed") {
            var report = new LegalityAnalysis(pk);
            legality = new {parsed=report.Parsed,valid=report.Valid,
                checks=report.Results.Select(c => new {
                    category=c.Identifier.ToString(),severity=c.Judgement.ToString(),
                    code=c.Result.ToString(),value=c.Value}).ToArray()};
        }
        Console.WriteLine(JsonSerializer.Serialize(new {
            id=parts[0],family,encoding,rawSha256=originalHash,nativeStatus=status,
            sanity,checksumValid=pk.ChecksumValid,
            species=(int)pk.Species,nature=(int)pk.Nature,
            evTotal=totalEV,maxEV=evs.Max(),pid=pk.PID,originalPKHeX=legality,
            synthetic=true,sourceHistoryAuthenticated=false,
        }));
    }
    return 0;
} catch(Exception ex) {
    Console.Error.WriteLine("PKHeX oracle fail closed: " + ex);
    return 1;
}
