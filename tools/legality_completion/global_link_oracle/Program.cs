// Source export and explicitly requested synthetic transfer witnesses.
// No source writes or receipt claims.
using System.Collections;
using System.Reflection;
using System.Text.Json;
using PKHeX.Core;
if(args.Length==2 && args[0]=="convert4") {
    var p=new PK4(Convert.FromHexString(args[1])).ConvertToPK5();
    p.MetDate=new DateOnly(2026,10,18);p.RefreshChecksum();
    Console.WriteLine(Convert.ToHexString(p.Data[..136]));return;
}
if(args.Length==1 && args[0]=="ev-delta") {
    Console.WriteLine(EffortExpLegality.GetRequiredEffortEXP(new[]{101,104,0,0,0,0},1,false,4,4));return;
}
if(args.Length==2 && args[0]=="read-fixture") {
    var raw=File.ReadAllBytes(args[1]);var ext=Path.GetExtension(args[1]).ToLowerInvariant();
    var context=ext switch {".pk1"=>EntityContext.Gen1,".pk2"=>EntityContext.Gen2,".pk4"=>EntityContext.Gen4,".pk5"=>EntityContext.Gen5,_=>EntityContext.Gen3};
    var entity=EntityFormat.GetFromBytes(raw,context) ?? throw new Exception("Unrecognized fixture format");
    var originalType=entity.GetType().Name;
    PKM p=entity switch {CK3 c=>c.ConvertToPK3(),XK3 x=>x.ConvertToPK3(),_=>entity};
    if(p.Format is <1 or >5)throw new Exception("Outside Gen I-V");
    var size=p.Format==1?44:p.Format==2?48:p.Format==3?80:136;
    Console.WriteLine(JsonSerializer.Serialize(new {format=p.Format,sourceType=originalType,version=p.Version.ToString(),species=p.Species,
        hex=Convert.ToHexString(p.Data[..size]),ot=p.Format<=2?Convert.ToHexString(p.OriginalTrainerTrash):"",
        nickname=p.Format<=2?Convert.ToHexString(p.NicknameTrash):"",language=p.Japanese?1:p is PK2 {Korean:true}?8:0,egg=p.IsEgg}));return;
}
var rows = new List<object>();
foreach (var name in new[]{"Encounters5BW","Encounters5B2W2","Encounters5DR"}) {
    var type=typeof(PKM).Assembly.GetType("PKHeX.Core."+name) ?? throw new Exception(name);
    foreach(var field in type.GetFields(BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.Static).OrderBy(f=>f.Name)) {
        if(!field.FieldType.IsArray || field.GetValue(null) is not IEnumerable entries)continue;
        int i=0;
        foreach(var entry in entries) {
            int index=i++;
            if(entry is not EncounterStatic5Entree e)continue;
            byte versions=0;
            if(e.Version.Contains(GameVersion.W)) versions|=1;
            if(e.Version.Contains(GameVersion.B)) versions|=2;
            if(e.Version.Contains(GameVersion.W2))versions|=4;
            if(e.Version.Contains(GameVersion.B2))versions|=8;
            rows.Add(new {source=name+"."+field.Name,index,species=e.Species,form=e.Form,level=e.Level,
                gender=e.Gender==255?-1:(int)e.Gender,ability=(int)e.Ability,versions,move=e.Moves.Move1,
                languages=(byte)e.Promotion.GetLanguages(),promotion=e.Promotion.ToString()});
        }
    }
}
if(rows.Count==0)throw new Exception("No Gen V Entree rows found");
Console.WriteLine(JsonSerializer.Serialize(rows));
