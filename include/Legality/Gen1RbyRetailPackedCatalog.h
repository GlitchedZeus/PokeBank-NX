#pragma once
// L3 original Gen I per-species level-up and evolution source table.
// Data extracted from pret/pokered@af519899719f0754965776faac0e836a3b906e6d
// data/pokemon/evos_moves.asm (blob 6a6704f98b8b075f2fd81c91d41a1dda0f2a047d)
// and pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
// data/pokemon/evos_moves.asm (blob 8bee626ae17d052a17562be9959a13b6102146f7).
// Symbol -> index: constants/pokedex_constants.asm; move -> ID:
// constants/move_constants.asm. Entries packed as (level<<8)|move.
// Evolution: (target<<10)|(method<<7)|condition, method 1 level,
// 2 stone, 3 trade; stones Moon=1,Fire=2,Thunder=3,Water=4,Leaf=5.
// The 6-bit textual encoding is lossless and carries no claims about
// native PK1 history, machine compatibility, or trainer decisions.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1RbyRetailPackedCatalog {
using sv=std::string_view;
inline constexpr sv kRB=
"AdJA0WBRNBtLCJKClPDBM;AdJA0WBZNB5LCZKC5PDZM;AdJA0WBZNB5LCtKDdPEFM;Ak0A8rBZjB6jCY1C5T;Ak0A8rBhjCGjCo1DhT;Ak0A8rBhjCSjC41DdT;AiRA83BYsBxuCOCCo4;AiRA83BgsB9uCeCC84;AiRA83BgsB9uCqCDQ4;;;AxdA9NBBOBFPBUwBoSCA8;;;AwfBB0BQpBljB4qCNh;AUcAxiBMSBwRCRhCx3;AUcAxiBUSB8RChhDF3;AUcAxiBUSB8RCxhDZ3;AdiA6eBd0CKi;AdiA6eBt0Cmi;AkrA8fBZ3B1BCRh;AkrA8fBl3CJBCth;AooBEsBiJB9nCYz;AooBEsBuJCRnC8z;AlWBBiBqBCFhCtX;;AocBGjBgoB-BCaa;AocBGjBsoCSBC-a;AgKA4oBUnB0sCSaCsY;AgKA4oBcnCAsCmaDIY;AgKA4oBci;AgeA4oBV0B0fCQgCsY;AgeA4oBd0CAfCkgDIY;AgeA4oBcl;A0vBIDBhrB92CdvDBx;;BBiBUuBxtCM1CpT;;AkBA4yBNvBgDB2cCIiCcm;;AowA8sBVtBwRCRy;AowA8sBVtCARCty;A9NBFOBNPBgzCFQC5M;A9NBFOBNPBwzCZQDRM;A9NBFOBNP;A1OBSNBuTCKjClK;A1OBSNB6TCejDBK;BhNBuNB5OCM8CZPCte;BhNBuNB5OCY8CtPDJe;A8tBNbBgcB-jChZ;A8tBNbBgcCOjC9Z;AwsBEGBhnCGaCyj;AwsBEGBhnCWaDOj;BwnB8yCRdCuaDQ4;BwnB8yCddDCaDs4;A8CBWaBt0CFFCcl;A8CBWaBt0CVFC4l;BI0BcrB4kCdhDI1;;BBfBM3BkDB8iCaFC04;BBfBM3BoDCEiCmFDE4;BBfBM3;;BBdBQyBs8B9pCZeCpz;BBdBQyBs8B9pCZeCpz;BRDBkrCB0CdFC5C;BRDBkrCR0CxFDRC;BRDBkrCR0CxFDRC;A0jA9NBJPBVOBozCFLCoV;A0jA9NBJPBdOB0zCZLDEV;A0jA9NBJP;AcwA0jBIoBY3BuECFwChnDA4;AcwA0jBIoBY3BuECNwCtnDI4;AtvBBYBV4BpqB9ZCSZ;AtvBBYBV4B1qCRZCuZ;AtvBBYBV4B1qCRZCuZ;B4nCAXCMtCdTCskDBh;B4nCAXCMtCdTC8kDdh;BIyBYdBstCE3CiFDBe;BIyBYdBstCE3CVuCyFDde;BUxBlUB0wCNWCmBC9n;BUxBlUB0wCZWC6BDZn;AcrA8fBcOB9hCej;BQtBgfB5BCRjCihCxh;BQtBgfB5BCdjC2hDNh;B4tCM-CicC0kDI6;B4tCM-CycDIkDg6;B6LCFrCV8CpqDBnDeX;B6LCFrCV8C1qDVnDyX;BIwBeAB4-CcrDI6;DKD;BtfCOK;B1fCaK;B1fCaK;A8UBNYBljCEVCtq;AwyBFdBgdB2LCBeCVg;AwyBFdBgdCGLCVeCtg;BQLBkMB4XCOYChq;BQLBkMCIXCqYDFq;BExBZ4B1xCSBCuZ;BExBZ4B1xCiBDKZ;BlzBxJCBOCVNCpMDBP;BwX;BkrB90CYlCubC5j;BkrCF0CklDCbDdj;CEbCYaCt0DCIDUZ;CEHCYICsJDAFDVE;AcXA8yBdvB8VCdn;CB8CVsCh4C1yDCZ;CB8CdsCt4DFyDWZ;B4XCMnCgfC0gDIrDck;B4XCMnCgfDAgDcrEAk;BgvB4tCZrCxvDBxDYm;B1HCBNCROCdPC0VDFK;BosB8nCQFCkrC6S;BNsBgrB43CVhC04;BNsBgrB43ClhDQ4;BMwBgeB4fCV_C0gDZh;BMwBgeB4fCd_DAgDZh;BE3BZqBtpCCBCVrCpxC84;;A9dBdxB8DCdgC-k;BErBR0BhoB2jCMOCph;BJ6BcDB8ICciC8lDo7;CJUCVnCoJDFxDZX;CQrCdtCsHDBsDR7Dc1;BlFB4MCR0CtqDGjDYO;BUXBwnCMrCxjDMk;A8h;BQsBlSCArCk4DQ_;BAvBQ2BkiB9tCY6C44;;BtiB8nCUsC0k;BtiB83CUnCgsCqXCxyDA2DY4;BtiB9UCUnChWCoYCxhDAqDZX;BtiB80CUnCgsCorCxTDBjDY1;Bc8BxpCNhCqh;CIeCcrC6DDU4;CIeCcrCyDDE4;CJHCejCwrDE4;CJHCejC4rDU4;CEwCYsC0kDY_;CMiClqDAmDg_;DM7DdhDw2;DNXDdhDxx;DMrDdhDyP;ApWBRhB4VChSDI_;ApWBRhCMVC1SDc_;ApWBRhCMVC1SDw_;D9wEJeEZpEs2FGF;AqQBQFB52Che;";
inline constexpr sv kYellow=
"AdJA0WBRNBtLCJKClPDBM;AdJA0WBZNB5LCZKC5PDZM;AdJA0WBZNB5LCtKDdPEFM;Ak0A8rBZjB6jCY1C5T;Ak0A8rBhjCGjCo1DhT;Ak0A8rBhjCSjC41DdT;AiRA83BYsBxuCOCCo4;AiRA83BgsB9uCeCC84;AiRA83BgsB9uCqCDQ4;;Adq;ApdA1NA5OA9PBIwBcSBwQCI8;;;AwfBB0BQpBljB4qCNh;AUcAxiBMSBwRCRhCx3;AUcAxiBUSB8RChhDF3;AUcAxiBUSB8RCxhDZ3;AdiA6eBd0CKi;AdiA6eBt0Cmi;AkrA8fBZ3B1BCRh;AkrA8fBl3CJBCth;AooBEsBiJB9nCYz;AooBEsBuJCRnC8z;AYnAhWAtiA9oBQVBpVCFhClXDJx;;AocBGjBgoB-BCaa;AocBGjBsoCSBC-a;AgKAwYBEoBcnB4sCaa;AgKAwYBMoBsnCQsC6a;AgKAwYBci;AgeAwYBEoBd0B4fCYg;AgeAwYBMoBt0CQfC4g;AgeAwYBcl;A0vBIDBhrB92CdvDBx;;BBiBUuBxtCM1CpT;;AkBA4yBNvBgDB2cCIiCcm;;AowA8sBVtBwRCRy;AowA8sBVtCARCty;A9NBFOBNPBgzCFQC5M;A9NBFOBNPBwzCZQDRM;A9NBFOBNP;A1OBSNBuTCKjClK;A1OBSNB6TCejDBK;AswBNdBZNBuNB5OCM8CZPCte;BZNBuNB5OCY8CtPDJe;A8tBNbBgcB-jChZ;A8tBNbBgcCOjC9Z;AwsBEGBhnCGaCyj;AwsBEGBhnCWaDOj;BwnB8yCRdCuaDQ4;BwnB8yCddDCaDs4;AlDA8CBWaBt0CFFCclC1n;AlDA8CBWaBt0BxjCVFC4lC1n;BI0BcrB4kCdhDI1;;BBfBM3BkDB8iCaFC04;BBfBM3BoDCEiCmFDE4;BBfBM3;;BBdBQyBs8B9pCZeCpz;BBdBQyBs8B9pCZeCpz;BRDBkrCB0CdFC5C;BRDBkrCR0CxFDRC;BRDBkrCR0CxFDRC;A0jA9NBJPBVOBozCFLCoV;A0jA9NBJPBdOB0zCZLDEV;A0jA9NBJP;AcwA0jBIoBY3BuECFwChnDA4;AcwA0jBIoBY3BuECNwCtnDI4;AtvBBYBV4BpqB9ZCSZ;AtvBBYBV4B1qCRZCuZ;AtvBBYBV4B1qCRZCuZ;B4nCAXCMtCdTCskDBh;B4nCAXCMtCdTC8kDdh;BIyBYdBstCE3CiFDBe;BIyBYdBstCE3CVuCyFDde;BUxBlUB0wCNWCmBC9n;BUxBlUB0wCZWC6BDZn;AcrA8fBcOB9hCej;BQtBgfB5BCRjCihCxh;BQtBgfB5BCdjC2hDNh;B4tCM-CicC0kDI6;B4tCM-CycDIkDg6;B6LCFrCV8CpqDBnDeX;B6LCFrCV8C1qDVnDyX;BIwBeAB4-CcrDI6;DKD;BtfCOK;B1fCaK;B1fCaK;A8UBNYBljCEVCtq;AwyBFdBgdB2LCBeCVg;AwyBFdBgdCGLCVeCtg;BQLBkMB4XCOYChq;BQLBkMCIXCqYDFq;BExBZ4B1xCSBCuZ;BExBZ4B1xCiBDKZ;BlzBxJCBOCVNCpMDBP;BwX;Ap9A0nBIdBkrB90CYlCubC5j;Ap9A0nBIdBkrCF0CklDCbDdj;CEbCYaCt0DCIDUZ;CEHCYICsJDAFDVE;AcXA8yBdvB8VCdn;CB8CVsCh4C1yDCZ;CB8CdsCt4DFyDWZ;B4XCMnCgfC0gDIrDck;B4XCMnCgfDAgDcrEAk;AwDBgvB4tCZrCxvDBxDYm;BgUBtHB0WCBNCROCdPC0VDBK;BosB8nCQFCkrC6S;BNsBgrB43CVhC04;BNsBgrB43ClhDQ4;BMwBgeB4fCV_C0gDZh;BMwBgeB4fCd_DAgDZh;BE3BZqBtpCCBCVrCpxC84;;A9dBdxB8DCdgC-k;BErBR0BhoB2jCMOCphDIR;BJ6BcDB8ICciC8lDo7;CJUCVnCoJDFxDZX;CQrCdtCsHDBsDR7Dc1;BUUBlFB4MCR0CtqDGjDYO;BUXBwnCMrCxjDMk;A8h;BQsBlSCArCk4DQ_;BAvBQ2BkiB9tCY6C44;;AgcBAtBdiB4sCR0Cok;AgcBA3BdiB4sCQ-CpyCo2C-XDQ4;AgcBBUBdiB4YCQqCpWC9hDRX;AgcBA0BdiB4sCRTCp7C8rDQ1;Bc8BxpCNhCqh;CIeCcrC6DDU4;CIeCcrCyDDE4;CJHCejCwrDE4;CJHCejC4rDU4;CEwCYsC0kDY_;CMiClqDAmDg_;DM7DdhDw2;DNXDdhDxx;DMrDdhDyP;ApWBRhB4VChSDI_;ApWBRhCMVC1SDc_;ApWBRhCMVC1SDw_;D9wEJeEZpEs2FGF;AqQBQFB52Che;";
inline constexpr sv kEvo=
"AiQ;Ayg;;BSQ;Bik;;CCQ;CSk;;CyH;DCK;;DiH;DyK;;ESS;Eik;;FCU;;FiU;;GCW;;GkD;;HCW;;HiQ;H0B;;ISQ;IkB;;JEB;;JkC;;KEB;;KiW;;LCV;LUF;;LyY;;MSf;;Mya;;NSc;;Nyh;;OSc;;O0C;;PSZ;PkE;;QCQ;QWA;;Qyc;RGA;;RiV;R0F;;SSe;;SyZ;TGA;;Tio;;UCl;;Uie;;;VSf;;Vyi;;WSm;;W0E;;XSZ;XmA;;;YSa;;Yyc;;ZSe;;Z0F;;aSc;;;;;bij;;cCq;;;;;dSg;;dyh;;eUE;;;;;;;;;giU;;;;iECh0DhkE;;;;;iyo;;jSo;;;;;;;lCe;lS3;;;;";
enum class Game : std::uint8_t {Unknown,Red,Blue,Yellow};
struct Offer{std::uint8_t level=0;std::uint16_t move=0;};
struct Evolution {std::uint16_t target=0;std::uint8_t kind=0,condition=0;};
struct Table{std::array<std::uint16_t,153> index{};std::array<std::uint32_t,920> records{};std::uint16_t used=0;bool valid=true;};
constexpr std::uint8_t symbol(char c) noexcept {
 if(c>='A'&&c<='Z')return static_cast<std::uint8_t>(c-'A');
 if(c>='a'&&c<='z')return static_cast<std::uint8_t>(c-'a'+26);
 if(c>='0'&&c<='9')return static_cast<std::uint8_t>(c-'0'+52);
 if(c=='-')return 62;
 if(c=='_')return 63;
 return 255;
}
consteval Table decode(sv s,bool sortByLevel=false) {
 Table t{};std::uint16_t sp=1; t.index[1]=0;
 for(std::size_t i=0;i<s.size();) {
  if(s[i]==';') {
   if(sp>151){t.valid=false;break;}
   // Retail Yellow has e.g. Primeape level 46 listed before level 45.
   // Level-up chronology is by attained level; keep same-level source order.
   if(sortByLevel)for(std::uint16_t n=t.index[sp]+1;n<t.used;++n){
     const auto v=t.records[n];auto j=n;
     while(j>t.index[sp] && (t.records[j-1]>>8)>(v>>8)){
       t.records[j]=t.records[j-1];--j;
     }
     t.records[j]=v;
   }
   t.index[++sp]=t.used;++i;continue;
  }
  if(i+3>s.size() || t.used>=t.records.size()){t.valid=false;break;}
  const auto a=symbol(s[i]),b=symbol(s[i+1]),c=symbol(s[i+2]);
  if(a>63||b>63||c>63){t.valid=false;break;}
  t.records[t.used++]=(static_cast<std::uint32_t>(a)<<12)|(static_cast<std::uint32_t>(b)<<6)|c;
  i+=3;
 }
 if(sp!=152)t.valid=false;
 return t;
}
inline constexpr Table kRBTable=decode(kRB,true),kYellowTable=decode(kYellow,true),kEvoTable=decode(kEvo);
constexpr bool validSpecies(std::uint16_t species) noexcept {return species>=1&&species<=151;}
constexpr bool known(Game g) noexcept {return g==Game::Red||g==Game::Blue||g==Game::Yellow;}
constexpr std::uint16_t offerCount(Game game,std::uint16_t species) noexcept {
 if(!validSpecies(species))return 0;
 if(game==Game::Yellow)return kYellowTable.index[species+1]-kYellowTable.index[species];
 if(game==Game::Red||game==Game::Blue)return kRBTable.index[species+1]-kRBTable.index[species];
 return 0;
}
constexpr Offer offerAt(Game game,std::uint16_t species,std::uint16_t index) noexcept {
 if(!known(game)||!validSpecies(species)||index>=offerCount(game,species))return {};
 const auto packed=game==Game::Yellow?
  kYellowTable.records[kYellowTable.index[species]+index]:
  kRBTable.records[kRBTable.index[species]+index];
 return {static_cast<std::uint8_t>(packed>>8),static_cast<std::uint16_t>(packed&255)};
}
constexpr std::uint16_t evolutionCount(std::uint16_t species) noexcept {
 return validSpecies(species)?kEvoTable.index[species+1]-kEvoTable.index[species]:0;
}
constexpr Evolution evolutionAt(std::uint16_t species,std::uint16_t index) noexcept {
 if(!validSpecies(species)||index>=evolutionCount(species))return {};
 const auto x=kEvoTable.records[kEvoTable.index[species]+index];
 return {static_cast<std::uint16_t>(x>>10),static_cast<std::uint8_t>((x>>7)&7),static_cast<std::uint8_t>(x&127)};
}
static_assert(kRBTable.valid&&kYellowTable.valid&&kEvoTable.valid);
static_assert(kRBTable.used==728&&kYellowTable.used==755&&kEvoTable.used==72);
static_assert(offerCount(Game::Blue,25)==5&&offerCount(Game::Yellow,25)==9);
static_assert(offerAt(Game::Red,25,0).level==9&&offerAt(Game::Yellow,25,0).level==6);
static_assert(evolutionAt(25,0).target==26&&evolutionAt(25,0).kind==2&&evolutionAt(25,0).condition==3);
} // namespace Legality::Gen1RbyRetailPackedCatalog
