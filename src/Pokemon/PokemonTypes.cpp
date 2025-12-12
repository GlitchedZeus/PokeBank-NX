/**
 * PokemonTypes.cpp - Pokemon Type Data Lookup Implementation
 *
 * Contains type data for all Pokemon species and forms.
 * Data sourced from official Pokemon data.
 */

#include "Pokemon/PokemonTypes.h"

namespace Pokemon {

// Pokemon Base Types Table (National Dex #1 - #1025)
// Generated from https://pokemondb.net/pokedex/all
// Format: {TYPE1, TYPE2}, // ID Name

static const TypePair BASE_TYPES[] = {
    { TYPE_NONE, TYPE_NONE },  // 0 (placeholder)
    { TYPE_GRASS, TYPE_POISON },  // 1 Bulbasaur
    { TYPE_GRASS, TYPE_POISON },  // 2 Ivysaur
    { TYPE_GRASS, TYPE_POISON },  // 3 Venusaur
    { TYPE_FIRE, TYPE_NONE },  // 4 Charmander
    { TYPE_FIRE, TYPE_NONE },  // 5 Charmeleon
    { TYPE_FIRE, TYPE_FLYING },  // 6 Charizard
    { TYPE_WATER, TYPE_NONE },  // 7 Squirtle
    { TYPE_WATER, TYPE_NONE },  // 8 Wartortle
    { TYPE_WATER, TYPE_NONE },  // 9 Blastoise
    { TYPE_BUG, TYPE_NONE },  // 10 Caterpie
    { TYPE_BUG, TYPE_NONE },  // 11 Metapod
    { TYPE_BUG, TYPE_FLYING },  // 12 Butterfree
    { TYPE_BUG, TYPE_POISON },  // 13 Weedle
    { TYPE_BUG, TYPE_POISON },  // 14 Kakuna
    { TYPE_BUG, TYPE_POISON },  // 15 Beedrill
    { TYPE_NORMAL, TYPE_FLYING },  // 16 Pidgey
    { TYPE_NORMAL, TYPE_FLYING },  // 17 Pidgeotto
    { TYPE_NORMAL, TYPE_FLYING },  // 18 Pidgeot
    { TYPE_NORMAL, TYPE_NONE },  // 19 Rattata
    { TYPE_NORMAL, TYPE_NONE },  // 20 Raticate
    { TYPE_NORMAL, TYPE_FLYING },  // 21 Spearow
    { TYPE_NORMAL, TYPE_FLYING },  // 22 Fearow
    { TYPE_POISON, TYPE_NONE },  // 23 Ekans
    { TYPE_POISON, TYPE_NONE },  // 24 Arbok
    { TYPE_ELECTRIC, TYPE_NONE },  // 25 Pikachu
    { TYPE_ELECTRIC, TYPE_NONE },  // 26 Raichu
    { TYPE_GROUND, TYPE_NONE },  // 27 Sandshrew
    { TYPE_GROUND, TYPE_NONE },  // 28 Sandslash
    { TYPE_POISON, TYPE_NONE },  // 29 Nidoran♀
    { TYPE_POISON, TYPE_NONE },  // 30 Nidorina
    { TYPE_POISON, TYPE_GROUND },  // 31 Nidoqueen
    { TYPE_POISON, TYPE_NONE },  // 32 Nidoran♂
    { TYPE_POISON, TYPE_NONE },  // 33 Nidorino
    { TYPE_POISON, TYPE_GROUND },  // 34 Nidoking
    { TYPE_FAIRY, TYPE_NONE },  // 35 Clefairy
    { TYPE_FAIRY, TYPE_NONE },  // 36 Clefable
    { TYPE_FIRE, TYPE_NONE },  // 37 Vulpix
    { TYPE_FIRE, TYPE_NONE },  // 38 Ninetales
    { TYPE_NORMAL, TYPE_FAIRY },  // 39 Jigglypuff
    { TYPE_NORMAL, TYPE_FAIRY },  // 40 Wigglytuff
    { TYPE_POISON, TYPE_FLYING },  // 41 Zubat
    { TYPE_POISON, TYPE_FLYING },  // 42 Golbat
    { TYPE_GRASS, TYPE_POISON },  // 43 Oddish
    { TYPE_GRASS, TYPE_POISON },  // 44 Gloom
    { TYPE_GRASS, TYPE_POISON },  // 45 Vileplume
    { TYPE_BUG, TYPE_GRASS },  // 46 Paras
    { TYPE_BUG, TYPE_GRASS },  // 47 Parasect
    { TYPE_BUG, TYPE_POISON },  // 48 Venonat
    { TYPE_BUG, TYPE_POISON },  // 49 Venomoth
    { TYPE_GROUND, TYPE_NONE },  // 50 Diglett
    { TYPE_GROUND, TYPE_NONE },  // 51 Dugtrio
    { TYPE_NORMAL, TYPE_NONE },  // 52 Meowth
    { TYPE_NORMAL, TYPE_NONE },  // 53 Persian
    { TYPE_WATER, TYPE_NONE },  // 54 Psyduck
    { TYPE_WATER, TYPE_NONE },  // 55 Golduck
    { TYPE_FIGHTING, TYPE_NONE },  // 56 Mankey
    { TYPE_FIGHTING, TYPE_NONE },  // 57 Primeape
    { TYPE_FIRE, TYPE_NONE },  // 58 Growlithe
    { TYPE_FIRE, TYPE_NONE },  // 59 Arcanine
    { TYPE_WATER, TYPE_NONE },  // 60 Poliwag
    { TYPE_WATER, TYPE_NONE },  // 61 Poliwhirl
    { TYPE_WATER, TYPE_FIGHTING },  // 62 Poliwrath
    { TYPE_PSYCHIC, TYPE_NONE },  // 63 Abra
    { TYPE_PSYCHIC, TYPE_NONE },  // 64 Kadabra
    { TYPE_PSYCHIC, TYPE_NONE },  // 65 Alakazam
    { TYPE_FIGHTING, TYPE_NONE },  // 66 Machop
    { TYPE_FIGHTING, TYPE_NONE },  // 67 Machoke
    { TYPE_FIGHTING, TYPE_NONE },  // 68 Machamp
    { TYPE_GRASS, TYPE_POISON },  // 69 Bellsprout
    { TYPE_GRASS, TYPE_POISON },  // 70 Weepinbell
    { TYPE_GRASS, TYPE_POISON },  // 71 Victreebel
    { TYPE_WATER, TYPE_POISON },  // 72 Tentacool
    { TYPE_WATER, TYPE_POISON },  // 73 Tentacruel
    { TYPE_ROCK, TYPE_GROUND },  // 74 Geodude
    { TYPE_ROCK, TYPE_GROUND },  // 75 Graveler
    { TYPE_ROCK, TYPE_GROUND },  // 76 Golem
    { TYPE_FIRE, TYPE_NONE },  // 77 Ponyta
    { TYPE_FIRE, TYPE_NONE },  // 78 Rapidash
    { TYPE_WATER, TYPE_PSYCHIC },  // 79 Slowpoke
    { TYPE_WATER, TYPE_PSYCHIC },  // 80 Slowbro
    { TYPE_ELECTRIC, TYPE_STEEL },  // 81 Magnemite
    { TYPE_ELECTRIC, TYPE_STEEL },  // 82 Magneton
    { TYPE_NORMAL, TYPE_FLYING },  // 83 Farfetch'd
    { TYPE_NORMAL, TYPE_FLYING },  // 84 Doduo
    { TYPE_NORMAL, TYPE_FLYING },  // 85 Dodrio
    { TYPE_WATER, TYPE_NONE },  // 86 Seel
    { TYPE_WATER, TYPE_ICE },  // 87 Dewgong
    { TYPE_POISON, TYPE_NONE },  // 88 Grimer
    { TYPE_POISON, TYPE_NONE },  // 89 Muk
    { TYPE_WATER, TYPE_NONE },  // 90 Shellder
    { TYPE_WATER, TYPE_ICE },  // 91 Cloyster
    { TYPE_GHOST, TYPE_POISON },  // 92 Gastly
    { TYPE_GHOST, TYPE_POISON },  // 93 Haunter
    { TYPE_GHOST, TYPE_POISON },  // 94 Gengar
    { TYPE_ROCK, TYPE_GROUND },  // 95 Onix
    { TYPE_PSYCHIC, TYPE_NONE },  // 96 Drowzee
    { TYPE_PSYCHIC, TYPE_NONE },  // 97 Hypno
    { TYPE_WATER, TYPE_NONE },  // 98 Krabby
    { TYPE_WATER, TYPE_NONE },  // 99 Kingler
    { TYPE_ELECTRIC, TYPE_NONE },  // 100 Voltorb
    { TYPE_ELECTRIC, TYPE_NONE },  // 101 Electrode
    { TYPE_GRASS, TYPE_PSYCHIC },  // 102 Exeggcute
    { TYPE_GRASS, TYPE_PSYCHIC },  // 103 Exeggutor
    { TYPE_GROUND, TYPE_NONE },  // 104 Cubone
    { TYPE_GROUND, TYPE_NONE },  // 105 Marowak
    { TYPE_FIGHTING, TYPE_NONE },  // 106 Hitmonlee
    { TYPE_FIGHTING, TYPE_NONE },  // 107 Hitmonchan
    { TYPE_NORMAL, TYPE_NONE },  // 108 Lickitung
    { TYPE_POISON, TYPE_NONE },  // 109 Koffing
    { TYPE_POISON, TYPE_NONE },  // 110 Weezing
    { TYPE_GROUND, TYPE_ROCK },  // 111 Rhyhorn
    { TYPE_GROUND, TYPE_ROCK },  // 112 Rhydon
    { TYPE_NORMAL, TYPE_NONE },  // 113 Chansey
    { TYPE_GRASS, TYPE_NONE },  // 114 Tangela
    { TYPE_NORMAL, TYPE_NONE },  // 115 Kangaskhan
    { TYPE_WATER, TYPE_NONE },  // 116 Horsea
    { TYPE_WATER, TYPE_NONE },  // 117 Seadra
    { TYPE_WATER, TYPE_NONE },  // 118 Goldeen
    { TYPE_WATER, TYPE_NONE },  // 119 Seaking
    { TYPE_WATER, TYPE_NONE },  // 120 Staryu
    { TYPE_WATER, TYPE_PSYCHIC },  // 121 Starmie
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 122 Mr. Mime
    { TYPE_BUG, TYPE_FLYING },  // 123 Scyther
    { TYPE_ICE, TYPE_PSYCHIC },  // 124 Jynx
    { TYPE_ELECTRIC, TYPE_NONE },  // 125 Electabuzz
    { TYPE_FIRE, TYPE_NONE },  // 126 Magmar
    { TYPE_BUG, TYPE_NONE },  // 127 Pinsir
    { TYPE_NORMAL, TYPE_NONE },  // 128 Tauros
    { TYPE_WATER, TYPE_NONE },  // 129 Magikarp
    { TYPE_WATER, TYPE_FLYING },  // 130 Gyarados
    { TYPE_WATER, TYPE_ICE },  // 131 Lapras
    { TYPE_NORMAL, TYPE_NONE },  // 132 Ditto
    { TYPE_NORMAL, TYPE_NONE },  // 133 Eevee
    { TYPE_WATER, TYPE_NONE },  // 134 Vaporeon
    { TYPE_ELECTRIC, TYPE_NONE },  // 135 Jolteon
    { TYPE_FIRE, TYPE_NONE },  // 136 Flareon
    { TYPE_NORMAL, TYPE_NONE },  // 137 Porygon
    { TYPE_ROCK, TYPE_WATER },  // 138 Omanyte
    { TYPE_ROCK, TYPE_WATER },  // 139 Omastar
    { TYPE_ROCK, TYPE_WATER },  // 140 Kabuto
    { TYPE_ROCK, TYPE_WATER },  // 141 Kabutops
    { TYPE_ROCK, TYPE_FLYING },  // 142 Aerodactyl
    { TYPE_NORMAL, TYPE_NONE },  // 143 Snorlax
    { TYPE_ICE, TYPE_FLYING },  // 144 Articuno
    { TYPE_ELECTRIC, TYPE_FLYING },  // 145 Zapdos
    { TYPE_FIRE, TYPE_FLYING },  // 146 Moltres
    { TYPE_DRAGON, TYPE_NONE },  // 147 Dratini
    { TYPE_DRAGON, TYPE_NONE },  // 148 Dragonair
    { TYPE_DRAGON, TYPE_FLYING },  // 149 Dragonite
    { TYPE_PSYCHIC, TYPE_NONE },  // 150 Mewtwo
    { TYPE_PSYCHIC, TYPE_NONE },  // 151 Mew
    { TYPE_GRASS, TYPE_NONE },  // 152 Chikorita
    { TYPE_GRASS, TYPE_NONE },  // 153 Bayleef
    { TYPE_GRASS, TYPE_NONE },  // 154 Meganium
    { TYPE_FIRE, TYPE_NONE },  // 155 Cyndaquil
    { TYPE_FIRE, TYPE_NONE },  // 156 Quilava
    { TYPE_FIRE, TYPE_NONE },  // 157 Typhlosion
    { TYPE_WATER, TYPE_NONE },  // 158 Totodile
    { TYPE_WATER, TYPE_NONE },  // 159 Croconaw
    { TYPE_WATER, TYPE_NONE },  // 160 Feraligatr
    { TYPE_NORMAL, TYPE_NONE },  // 161 Sentret
    { TYPE_NORMAL, TYPE_NONE },  // 162 Furret
    { TYPE_NORMAL, TYPE_FLYING },  // 163 Hoothoot
    { TYPE_NORMAL, TYPE_FLYING },  // 164 Noctowl
    { TYPE_BUG, TYPE_FLYING },  // 165 Ledyba
    { TYPE_BUG, TYPE_FLYING },  // 166 Ledian
    { TYPE_BUG, TYPE_POISON },  // 167 Spinarak
    { TYPE_BUG, TYPE_POISON },  // 168 Ariados
    { TYPE_POISON, TYPE_FLYING },  // 169 Crobat
    { TYPE_WATER, TYPE_ELECTRIC },  // 170 Chinchou
    { TYPE_WATER, TYPE_ELECTRIC },  // 171 Lanturn
    { TYPE_ELECTRIC, TYPE_NONE },  // 172 Pichu
    { TYPE_FAIRY, TYPE_NONE },  // 173 Cleffa
    { TYPE_NORMAL, TYPE_FAIRY },  // 174 Igglybuff
    { TYPE_FAIRY, TYPE_NONE },  // 175 Togepi
    { TYPE_FAIRY, TYPE_FLYING },  // 176 Togetic
    { TYPE_PSYCHIC, TYPE_FLYING },  // 177 Natu
    { TYPE_PSYCHIC, TYPE_FLYING },  // 178 Xatu
    { TYPE_ELECTRIC, TYPE_NONE },  // 179 Mareep
    { TYPE_ELECTRIC, TYPE_NONE },  // 180 Flaaffy
    { TYPE_ELECTRIC, TYPE_NONE },  // 181 Ampharos
    { TYPE_GRASS, TYPE_NONE },  // 182 Bellossom
    { TYPE_WATER, TYPE_FAIRY },  // 183 Marill
    { TYPE_WATER, TYPE_FAIRY },  // 184 Azumarill
    { TYPE_ROCK, TYPE_NONE },  // 185 Sudowoodo
    { TYPE_WATER, TYPE_NONE },  // 186 Politoed
    { TYPE_GRASS, TYPE_FLYING },  // 187 Hoppip
    { TYPE_GRASS, TYPE_FLYING },  // 188 Skiploom
    { TYPE_GRASS, TYPE_FLYING },  // 189 Jumpluff
    { TYPE_NORMAL, TYPE_NONE },  // 190 Aipom
    { TYPE_GRASS, TYPE_NONE },  // 191 Sunkern
    { TYPE_GRASS, TYPE_NONE },  // 192 Sunflora
    { TYPE_BUG, TYPE_FLYING },  // 193 Yanma
    { TYPE_WATER, TYPE_GROUND },  // 194 Wooper
    { TYPE_WATER, TYPE_GROUND },  // 195 Quagsire
    { TYPE_PSYCHIC, TYPE_NONE },  // 196 Espeon
    { TYPE_DARK, TYPE_NONE },  // 197 Umbreon
    { TYPE_DARK, TYPE_FLYING },  // 198 Murkrow
    { TYPE_WATER, TYPE_PSYCHIC },  // 199 Slowking
    { TYPE_GHOST, TYPE_NONE },  // 200 Misdreavus
    { TYPE_PSYCHIC, TYPE_NONE },  // 201 Unown
    { TYPE_PSYCHIC, TYPE_NONE },  // 202 Wobbuffet
    { TYPE_NORMAL, TYPE_PSYCHIC },  // 203 Girafarig
    { TYPE_BUG, TYPE_NONE },  // 204 Pineco
    { TYPE_BUG, TYPE_STEEL },  // 205 Forretress
    { TYPE_NORMAL, TYPE_NONE },  // 206 Dunsparce
    { TYPE_GROUND, TYPE_FLYING },  // 207 Gligar
    { TYPE_STEEL, TYPE_GROUND },  // 208 Steelix
    { TYPE_FAIRY, TYPE_NONE },  // 209 Snubbull
    { TYPE_FAIRY, TYPE_NONE },  // 210 Granbull
    { TYPE_WATER, TYPE_POISON },  // 211 Qwilfish
    { TYPE_BUG, TYPE_STEEL },  // 212 Scizor
    { TYPE_BUG, TYPE_ROCK },  // 213 Shuckle
    { TYPE_BUG, TYPE_FIGHTING },  // 214 Heracross
    { TYPE_DARK, TYPE_ICE },  // 215 Sneasel
    { TYPE_NORMAL, TYPE_NONE },  // 216 Teddiursa
    { TYPE_NORMAL, TYPE_NONE },  // 217 Ursaring
    { TYPE_FIRE, TYPE_NONE },  // 218 Slugma
    { TYPE_FIRE, TYPE_ROCK },  // 219 Magcargo
    { TYPE_ICE, TYPE_GROUND },  // 220 Swinub
    { TYPE_ICE, TYPE_GROUND },  // 221 Piloswine
    { TYPE_WATER, TYPE_ROCK },  // 222 Corsola
    { TYPE_WATER, TYPE_NONE },  // 223 Remoraid
    { TYPE_WATER, TYPE_NONE },  // 224 Octillery
    { TYPE_ICE, TYPE_FLYING },  // 225 Delibird
    { TYPE_WATER, TYPE_FLYING },  // 226 Mantine
    { TYPE_STEEL, TYPE_FLYING },  // 227 Skarmory
    { TYPE_DARK, TYPE_FIRE },  // 228 Houndour
    { TYPE_DARK, TYPE_FIRE },  // 229 Houndoom
    { TYPE_WATER, TYPE_DRAGON },  // 230 Kingdra
    { TYPE_GROUND, TYPE_NONE },  // 231 Phanpy
    { TYPE_GROUND, TYPE_NONE },  // 232 Donphan
    { TYPE_NORMAL, TYPE_NONE },  // 233 Porygon2
    { TYPE_NORMAL, TYPE_NONE },  // 234 Stantler
    { TYPE_NORMAL, TYPE_NONE },  // 235 Smeargle
    { TYPE_FIGHTING, TYPE_NONE },  // 236 Tyrogue
    { TYPE_FIGHTING, TYPE_NONE },  // 237 Hitmontop
    { TYPE_ICE, TYPE_PSYCHIC },  // 238 Smoochum
    { TYPE_ELECTRIC, TYPE_NONE },  // 239 Elekid
    { TYPE_FIRE, TYPE_NONE },  // 240 Magby
    { TYPE_NORMAL, TYPE_NONE },  // 241 Miltank
    { TYPE_NORMAL, TYPE_NONE },  // 242 Blissey
    { TYPE_ELECTRIC, TYPE_NONE },  // 243 Raikou
    { TYPE_FIRE, TYPE_NONE },  // 244 Entei
    { TYPE_WATER, TYPE_NONE },  // 245 Suicune
    { TYPE_ROCK, TYPE_GROUND },  // 246 Larvitar
    { TYPE_ROCK, TYPE_GROUND },  // 247 Pupitar
    { TYPE_ROCK, TYPE_DARK },  // 248 Tyranitar
    { TYPE_PSYCHIC, TYPE_FLYING },  // 249 Lugia
    { TYPE_FIRE, TYPE_FLYING },  // 250 Ho-oh
    { TYPE_PSYCHIC, TYPE_GRASS },  // 251 Celebi
    { TYPE_GRASS, TYPE_NONE },  // 252 Treecko
    { TYPE_GRASS, TYPE_NONE },  // 253 Grovyle
    { TYPE_GRASS, TYPE_NONE },  // 254 Sceptile
    { TYPE_FIRE, TYPE_NONE },  // 255 Torchic
    { TYPE_FIRE, TYPE_FIGHTING },  // 256 Combusken
    { TYPE_FIRE, TYPE_FIGHTING },  // 257 Blaziken
    { TYPE_WATER, TYPE_NONE },  // 258 Mudkip
    { TYPE_WATER, TYPE_GROUND },  // 259 Marshtomp
    { TYPE_WATER, TYPE_GROUND },  // 260 Swampert
    { TYPE_DARK, TYPE_NONE },  // 261 Poochyena
    { TYPE_DARK, TYPE_NONE },  // 262 Mightyena
    { TYPE_NORMAL, TYPE_NONE },  // 263 Zigzagoon
    { TYPE_NORMAL, TYPE_NONE },  // 264 Linoone
    { TYPE_BUG, TYPE_NONE },  // 265 Wurmple
    { TYPE_BUG, TYPE_NONE },  // 266 Silcoon
    { TYPE_BUG, TYPE_FLYING },  // 267 Beautifly
    { TYPE_BUG, TYPE_NONE },  // 268 Cascoon
    { TYPE_BUG, TYPE_POISON },  // 269 Dustox
    { TYPE_WATER, TYPE_GRASS },  // 270 Lotad
    { TYPE_WATER, TYPE_GRASS },  // 271 Lombre
    { TYPE_WATER, TYPE_GRASS },  // 272 Ludicolo
    { TYPE_GRASS, TYPE_NONE },  // 273 Seedot
    { TYPE_GRASS, TYPE_DARK },  // 274 Nuzleaf
    { TYPE_GRASS, TYPE_DARK },  // 275 Shiftry
    { TYPE_NORMAL, TYPE_FLYING },  // 276 Taillow
    { TYPE_NORMAL, TYPE_FLYING },  // 277 Swellow
    { TYPE_WATER, TYPE_FLYING },  // 278 Wingull
    { TYPE_WATER, TYPE_FLYING },  // 279 Pelipper
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 280 Ralts
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 281 Kirlia
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 282 Gardevoir
    { TYPE_BUG, TYPE_WATER },  // 283 Surskit
    { TYPE_BUG, TYPE_FLYING },  // 284 Masquerain
    { TYPE_GRASS, TYPE_NONE },  // 285 Shroomish
    { TYPE_GRASS, TYPE_FIGHTING },  // 286 Breloom
    { TYPE_NORMAL, TYPE_NONE },  // 287 Slakoth
    { TYPE_NORMAL, TYPE_NONE },  // 288 Vigoroth
    { TYPE_NORMAL, TYPE_NONE },  // 289 Slaking
    { TYPE_BUG, TYPE_GROUND },  // 290 Nincada
    { TYPE_BUG, TYPE_FLYING },  // 291 Ninjask
    { TYPE_BUG, TYPE_GHOST },  // 292 Shedinja
    { TYPE_NORMAL, TYPE_NONE },  // 293 Whismur
    { TYPE_NORMAL, TYPE_NONE },  // 294 Loudred
    { TYPE_NORMAL, TYPE_NONE },  // 295 Exploud
    { TYPE_FIGHTING, TYPE_NONE },  // 296 Makuhita
    { TYPE_FIGHTING, TYPE_NONE },  // 297 Hariyama
    { TYPE_NORMAL, TYPE_FAIRY },  // 298 Azurill
    { TYPE_ROCK, TYPE_NONE },  // 299 Nosepass
    { TYPE_NORMAL, TYPE_NONE },  // 300 Skitty
    { TYPE_NORMAL, TYPE_NONE },  // 301 Delcatty
    { TYPE_DARK, TYPE_GHOST },  // 302 Sableye
    { TYPE_STEEL, TYPE_FAIRY },  // 303 Mawile
    { TYPE_STEEL, TYPE_ROCK },  // 304 Aron
    { TYPE_STEEL, TYPE_ROCK },  // 305 Lairon
    { TYPE_STEEL, TYPE_ROCK },  // 306 Aggron
    { TYPE_FIGHTING, TYPE_PSYCHIC },  // 307 Meditite
    { TYPE_FIGHTING, TYPE_PSYCHIC },  // 308 Medicham
    { TYPE_ELECTRIC, TYPE_NONE },  // 309 Electrike
    { TYPE_ELECTRIC, TYPE_NONE },  // 310 Manectric
    { TYPE_ELECTRIC, TYPE_NONE },  // 311 Plusle
    { TYPE_ELECTRIC, TYPE_NONE },  // 312 Minun
    { TYPE_BUG, TYPE_NONE },  // 313 Volbeat
    { TYPE_BUG, TYPE_NONE },  // 314 Illumise
    { TYPE_GRASS, TYPE_POISON },  // 315 Roselia
    { TYPE_POISON, TYPE_NONE },  // 316 Gulpin
    { TYPE_POISON, TYPE_NONE },  // 317 Swalot
    { TYPE_WATER, TYPE_DARK },  // 318 Carvanha
    { TYPE_WATER, TYPE_DARK },  // 319 Sharpedo
    { TYPE_WATER, TYPE_NONE },  // 320 Wailmer
    { TYPE_WATER, TYPE_NONE },  // 321 Wailord
    { TYPE_FIRE, TYPE_GROUND },  // 322 Numel
    { TYPE_FIRE, TYPE_GROUND },  // 323 Camerupt
    { TYPE_FIRE, TYPE_NONE },  // 324 Torkoal
    { TYPE_PSYCHIC, TYPE_NONE },  // 325 Spoink
    { TYPE_PSYCHIC, TYPE_NONE },  // 326 Grumpig
    { TYPE_NORMAL, TYPE_NONE },  // 327 Spinda
    { TYPE_GROUND, TYPE_NONE },  // 328 Trapinch
    { TYPE_GROUND, TYPE_DRAGON },  // 329 Vibrava
    { TYPE_GROUND, TYPE_DRAGON },  // 330 Flygon
    { TYPE_GRASS, TYPE_NONE },  // 331 Cacnea
    { TYPE_GRASS, TYPE_DARK },  // 332 Cacturne
    { TYPE_NORMAL, TYPE_FLYING },  // 333 Swablu
    { TYPE_DRAGON, TYPE_FLYING },  // 334 Altaria
    { TYPE_NORMAL, TYPE_NONE },  // 335 Zangoose
    { TYPE_POISON, TYPE_NONE },  // 336 Seviper
    { TYPE_ROCK, TYPE_PSYCHIC },  // 337 Lunatone
    { TYPE_ROCK, TYPE_PSYCHIC },  // 338 Solrock
    { TYPE_WATER, TYPE_GROUND },  // 339 Barboach
    { TYPE_WATER, TYPE_GROUND },  // 340 Whiscash
    { TYPE_WATER, TYPE_NONE },  // 341 Corphish
    { TYPE_WATER, TYPE_DARK },  // 342 Crawdaunt
    { TYPE_GROUND, TYPE_PSYCHIC },  // 343 Baltoy
    { TYPE_GROUND, TYPE_PSYCHIC },  // 344 Claydol
    { TYPE_ROCK, TYPE_GRASS },  // 345 Lileep
    { TYPE_ROCK, TYPE_GRASS },  // 346 Cradily
    { TYPE_ROCK, TYPE_BUG },  // 347 Anorith
    { TYPE_ROCK, TYPE_BUG },  // 348 Armaldo
    { TYPE_WATER, TYPE_NONE },  // 349 Feebas
    { TYPE_WATER, TYPE_NONE },  // 350 Milotic
    { TYPE_NORMAL, TYPE_NONE },  // 351 Castform
    { TYPE_NORMAL, TYPE_NONE },  // 352 Kecleon
    { TYPE_GHOST, TYPE_NONE },  // 353 Shuppet
    { TYPE_GHOST, TYPE_NONE },  // 354 Banette
    { TYPE_GHOST, TYPE_NONE },  // 355 Duskull
    { TYPE_GHOST, TYPE_NONE },  // 356 Dusclops
    { TYPE_GRASS, TYPE_FLYING },  // 357 Tropius
    { TYPE_PSYCHIC, TYPE_NONE },  // 358 Chimecho
    { TYPE_DARK, TYPE_NONE },  // 359 Absol
    { TYPE_PSYCHIC, TYPE_NONE },  // 360 Wynaut
    { TYPE_ICE, TYPE_NONE },  // 361 Snorunt
    { TYPE_ICE, TYPE_NONE },  // 362 Glalie
    { TYPE_ICE, TYPE_WATER },  // 363 Spheal
    { TYPE_ICE, TYPE_WATER },  // 364 Sealeo
    { TYPE_ICE, TYPE_WATER },  // 365 Walrein
    { TYPE_WATER, TYPE_NONE },  // 366 Clamperl
    { TYPE_WATER, TYPE_NONE },  // 367 Huntail
    { TYPE_WATER, TYPE_NONE },  // 368 Gorebyss
    { TYPE_WATER, TYPE_ROCK },  // 369 Relicanth
    { TYPE_WATER, TYPE_NONE },  // 370 Luvdisc
    { TYPE_DRAGON, TYPE_NONE },  // 371 Bagon
    { TYPE_DRAGON, TYPE_NONE },  // 372 Shelgon
    { TYPE_DRAGON, TYPE_FLYING },  // 373 Salamence
    { TYPE_STEEL, TYPE_PSYCHIC },  // 374 Beldum
    { TYPE_STEEL, TYPE_PSYCHIC },  // 375 Metang
    { TYPE_STEEL, TYPE_PSYCHIC },  // 376 Metagross
    { TYPE_ROCK, TYPE_NONE },  // 377 Regirock
    { TYPE_ICE, TYPE_NONE },  // 378 Regice
    { TYPE_STEEL, TYPE_NONE },  // 379 Registeel
    { TYPE_DRAGON, TYPE_PSYCHIC },  // 380 Latias
    { TYPE_DRAGON, TYPE_PSYCHIC },  // 381 Latios
    { TYPE_WATER, TYPE_NONE },  // 382 Kyogre
    { TYPE_GROUND, TYPE_NONE },  // 383 Groudon
    { TYPE_DRAGON, TYPE_FLYING },  // 384 Rayquaza
    { TYPE_STEEL, TYPE_PSYCHIC },  // 385 Jirachi
    { TYPE_PSYCHIC, TYPE_NONE },  // 386 Deoxys Normal Forme
    { TYPE_GRASS, TYPE_NONE },  // 387 Turtwig
    { TYPE_GRASS, TYPE_NONE },  // 388 Grotle
    { TYPE_GRASS, TYPE_GROUND },  // 389 Torterra
    { TYPE_FIRE, TYPE_NONE },  // 390 Chimchar
    { TYPE_FIRE, TYPE_FIGHTING },  // 391 Monferno
    { TYPE_FIRE, TYPE_FIGHTING },  // 392 Infernape
    { TYPE_WATER, TYPE_NONE },  // 393 Piplup
    { TYPE_WATER, TYPE_NONE },  // 394 Prinplup
    { TYPE_WATER, TYPE_STEEL },  // 395 Empoleon
    { TYPE_NORMAL, TYPE_FLYING },  // 396 Starly
    { TYPE_NORMAL, TYPE_FLYING },  // 397 Staravia
    { TYPE_NORMAL, TYPE_FLYING },  // 398 Staraptor
    { TYPE_NORMAL, TYPE_NONE },  // 399 Bidoof
    { TYPE_NORMAL, TYPE_WATER },  // 400 Bibarel
    { TYPE_BUG, TYPE_NONE },  // 401 Kricketot
    { TYPE_BUG, TYPE_NONE },  // 402 Kricketune
    { TYPE_ELECTRIC, TYPE_NONE },  // 403 Shinx
    { TYPE_ELECTRIC, TYPE_NONE },  // 404 Luxio
    { TYPE_ELECTRIC, TYPE_NONE },  // 405 Luxray
    { TYPE_GRASS, TYPE_POISON },  // 406 Budew
    { TYPE_GRASS, TYPE_POISON },  // 407 Roserade
    { TYPE_ROCK, TYPE_NONE },  // 408 Cranidos
    { TYPE_ROCK, TYPE_NONE },  // 409 Rampardos
    { TYPE_ROCK, TYPE_STEEL },  // 410 Shieldon
    { TYPE_ROCK, TYPE_STEEL },  // 411 Bastiodon
    { TYPE_BUG, TYPE_NONE },  // 412 Burmy Plant Cloak
    { TYPE_BUG, TYPE_GRASS },  // 413 Wormadam Plant Cloak
    { TYPE_BUG, TYPE_FLYING },  // 414 Mothim
    { TYPE_BUG, TYPE_FLYING },  // 415 Combee
    { TYPE_BUG, TYPE_FLYING },  // 416 Vespiquen
    { TYPE_ELECTRIC, TYPE_NONE },  // 417 Pachirisu
    { TYPE_WATER, TYPE_NONE },  // 418 Buizel
    { TYPE_WATER, TYPE_NONE },  // 419 Floatzel
    { TYPE_GRASS, TYPE_NONE },  // 420 Cherubi
    { TYPE_GRASS, TYPE_NONE },  // 421 Cherrim
    { TYPE_WATER, TYPE_NONE },  // 422 Shellos
    { TYPE_WATER, TYPE_GROUND },  // 423 Gastrodon
    { TYPE_NORMAL, TYPE_NONE },  // 424 Ambipom
    { TYPE_GHOST, TYPE_FLYING },  // 425 Drifloon
    { TYPE_GHOST, TYPE_FLYING },  // 426 Drifblim
    { TYPE_NORMAL, TYPE_NONE },  // 427 Buneary
    { TYPE_NORMAL, TYPE_NONE },  // 428 Lopunny
    { TYPE_GHOST, TYPE_NONE },  // 429 Mismagius
    { TYPE_DARK, TYPE_FLYING },  // 430 Honchkrow
    { TYPE_NORMAL, TYPE_NONE },  // 431 Glameow
    { TYPE_NORMAL, TYPE_NONE },  // 432 Purugly
    { TYPE_PSYCHIC, TYPE_NONE },  // 433 Chingling
    { TYPE_POISON, TYPE_DARK },  // 434 Stunky
    { TYPE_POISON, TYPE_DARK },  // 435 Skuntank
    { TYPE_STEEL, TYPE_PSYCHIC },  // 436 Bronzor
    { TYPE_STEEL, TYPE_PSYCHIC },  // 437 Bronzong
    { TYPE_ROCK, TYPE_NONE },  // 438 Bonsly
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 439 Mime Jr.
    { TYPE_NORMAL, TYPE_NONE },  // 440 Happiny
    { TYPE_NORMAL, TYPE_FLYING },  // 441 Chatot
    { TYPE_GHOST, TYPE_DARK },  // 442 Spiritomb
    { TYPE_DRAGON, TYPE_GROUND },  // 443 Gible
    { TYPE_DRAGON, TYPE_GROUND },  // 444 Gabite
    { TYPE_DRAGON, TYPE_GROUND },  // 445 Garchomp
    { TYPE_NORMAL, TYPE_NONE },  // 446 Munchlax
    { TYPE_FIGHTING, TYPE_NONE },  // 447 Riolu
    { TYPE_FIGHTING, TYPE_STEEL },  // 448 Lucario
    { TYPE_GROUND, TYPE_NONE },  // 449 Hippopotas
    { TYPE_GROUND, TYPE_NONE },  // 450 Hippowdon
    { TYPE_POISON, TYPE_BUG },  // 451 Skorupi
    { TYPE_POISON, TYPE_DARK },  // 452 Drapion
    { TYPE_POISON, TYPE_FIGHTING },  // 453 Croagunk
    { TYPE_POISON, TYPE_FIGHTING },  // 454 Toxicroak
    { TYPE_GRASS, TYPE_NONE },  // 455 Carnivine
    { TYPE_WATER, TYPE_NONE },  // 456 Finneon
    { TYPE_WATER, TYPE_NONE },  // 457 Lumineon
    { TYPE_WATER, TYPE_FLYING },  // 458 Mantyke
    { TYPE_GRASS, TYPE_ICE },  // 459 Snover
    { TYPE_GRASS, TYPE_ICE },  // 460 Abomasnow
    { TYPE_DARK, TYPE_ICE },  // 461 Weavile
    { TYPE_ELECTRIC, TYPE_STEEL },  // 462 Magnezone
    { TYPE_NORMAL, TYPE_NONE },  // 463 Lickilicky
    { TYPE_GROUND, TYPE_ROCK },  // 464 Rhyperior
    { TYPE_GRASS, TYPE_NONE },  // 465 Tangrowth
    { TYPE_ELECTRIC, TYPE_NONE },  // 466 Electivire
    { TYPE_FIRE, TYPE_NONE },  // 467 Magmortar
    { TYPE_FAIRY, TYPE_FLYING },  // 468 Togekiss
    { TYPE_BUG, TYPE_FLYING },  // 469 Yanmega
    { TYPE_GRASS, TYPE_NONE },  // 470 Leafeon
    { TYPE_ICE, TYPE_NONE },  // 471 Glaceon
    { TYPE_GROUND, TYPE_FLYING },  // 472 Gliscor
    { TYPE_ICE, TYPE_GROUND },  // 473 Mamoswine
    { TYPE_NORMAL, TYPE_NONE },  // 474 Porygon-Z
    { TYPE_PSYCHIC, TYPE_FIGHTING },  // 475 Gallade
    { TYPE_ROCK, TYPE_STEEL },  // 476 Probopass
    { TYPE_GHOST, TYPE_NONE },  // 477 Dusknoir
    { TYPE_ICE, TYPE_GHOST },  // 478 Froslass
    { TYPE_ELECTRIC, TYPE_GHOST },  // 479 Rotom
    { TYPE_PSYCHIC, TYPE_NONE },  // 480 Uxie
    { TYPE_PSYCHIC, TYPE_NONE },  // 481 Mesprit
    { TYPE_PSYCHIC, TYPE_NONE },  // 482 Azelf
    { TYPE_STEEL, TYPE_DRAGON },  // 483 Dialga
    { TYPE_WATER, TYPE_DRAGON },  // 484 Palkia
    { TYPE_FIRE, TYPE_STEEL },  // 485 Heatran
    { TYPE_NORMAL, TYPE_NONE },  // 486 Regigigas
    { TYPE_GHOST, TYPE_DRAGON },  // 487 Giratina Altered Forme
    { TYPE_PSYCHIC, TYPE_NONE },  // 488 Cresselia
    { TYPE_WATER, TYPE_NONE },  // 489 Phione
    { TYPE_WATER, TYPE_NONE },  // 490 Manaphy
    { TYPE_DARK, TYPE_NONE },  // 491 Darkrai
    { TYPE_GRASS, TYPE_NONE },  // 492 Shaymin Land Forme
    { TYPE_NORMAL, TYPE_NONE },  // 493 Arceus
    { TYPE_PSYCHIC, TYPE_FIRE },  // 494 Victini
    { TYPE_GRASS, TYPE_NONE },  // 495 Snivy
    { TYPE_GRASS, TYPE_NONE },  // 496 Servine
    { TYPE_GRASS, TYPE_NONE },  // 497 Serperior
    { TYPE_FIRE, TYPE_NONE },  // 498 Tepig
    { TYPE_FIRE, TYPE_FIGHTING },  // 499 Pignite
    { TYPE_FIRE, TYPE_FIGHTING },  // 500 Emboar
    { TYPE_WATER, TYPE_NONE },  // 501 Oshawott
    { TYPE_WATER, TYPE_NONE },  // 502 Dewott
    { TYPE_WATER, TYPE_NONE },  // 503 Samurott
    { TYPE_NORMAL, TYPE_NONE },  // 504 Patrat
    { TYPE_NORMAL, TYPE_NONE },  // 505 Watchog
    { TYPE_NORMAL, TYPE_NONE },  // 506 Lillipup
    { TYPE_NORMAL, TYPE_NONE },  // 507 Herdier
    { TYPE_NORMAL, TYPE_NONE },  // 508 Stoutland
    { TYPE_DARK, TYPE_NONE },  // 509 Purrloin
    { TYPE_DARK, TYPE_NONE },  // 510 Liepard
    { TYPE_GRASS, TYPE_NONE },  // 511 Pansage
    { TYPE_GRASS, TYPE_NONE },  // 512 Simisage
    { TYPE_FIRE, TYPE_NONE },  // 513 Pansear
    { TYPE_FIRE, TYPE_NONE },  // 514 Simisear
    { TYPE_WATER, TYPE_NONE },  // 515 Panpour
    { TYPE_WATER, TYPE_NONE },  // 516 Simipour
    { TYPE_PSYCHIC, TYPE_NONE },  // 517 Munna
    { TYPE_PSYCHIC, TYPE_NONE },  // 518 Musharna
    { TYPE_NORMAL, TYPE_FLYING },  // 519 Pidove
    { TYPE_NORMAL, TYPE_FLYING },  // 520 Tranquill
    { TYPE_NORMAL, TYPE_FLYING },  // 521 Unfezant
    { TYPE_ELECTRIC, TYPE_NONE },  // 522 Blitzle
    { TYPE_ELECTRIC, TYPE_NONE },  // 523 Zebstrika
    { TYPE_ROCK, TYPE_NONE },  // 524 Roggenrola
    { TYPE_ROCK, TYPE_NONE },  // 525 Boldore
    { TYPE_ROCK, TYPE_NONE },  // 526 Gigalith
    { TYPE_PSYCHIC, TYPE_FLYING },  // 527 Woobat
    { TYPE_PSYCHIC, TYPE_FLYING },  // 528 Swoobat
    { TYPE_GROUND, TYPE_NONE },  // 529 Drilbur
    { TYPE_GROUND, TYPE_STEEL },  // 530 Excadrill
    { TYPE_NORMAL, TYPE_NONE },  // 531 Audino
    { TYPE_FIGHTING, TYPE_NONE },  // 532 Timburr
    { TYPE_FIGHTING, TYPE_NONE },  // 533 Gurdurr
    { TYPE_FIGHTING, TYPE_NONE },  // 534 Conkeldurr
    { TYPE_WATER, TYPE_NONE },  // 535 Tympole
    { TYPE_WATER, TYPE_GROUND },  // 536 Palpitoad
    { TYPE_WATER, TYPE_GROUND },  // 537 Seismitoad
    { TYPE_FIGHTING, TYPE_NONE },  // 538 Throh
    { TYPE_FIGHTING, TYPE_NONE },  // 539 Sawk
    { TYPE_BUG, TYPE_GRASS },  // 540 Sewaddle
    { TYPE_BUG, TYPE_GRASS },  // 541 Swadloon
    { TYPE_BUG, TYPE_GRASS },  // 542 Leavanny
    { TYPE_BUG, TYPE_POISON },  // 543 Venipede
    { TYPE_BUG, TYPE_POISON },  // 544 Whirlipede
    { TYPE_BUG, TYPE_POISON },  // 545 Scolipede
    { TYPE_GRASS, TYPE_FAIRY },  // 546 Cottonee
    { TYPE_GRASS, TYPE_FAIRY },  // 547 Whimsicott
    { TYPE_GRASS, TYPE_NONE },  // 548 Petilil
    { TYPE_GRASS, TYPE_NONE },  // 549 Lilligant
    { TYPE_WATER, TYPE_NONE },  // 550 Basculin Red-Striped Form
    { TYPE_GROUND, TYPE_DARK },  // 551 Sandile
    { TYPE_GROUND, TYPE_DARK },  // 552 Krokorok
    { TYPE_GROUND, TYPE_DARK },  // 553 Krookodile
    { TYPE_FIRE, TYPE_NONE },  // 554 Darumaka
    { TYPE_FIRE, TYPE_NONE },  // 555 Darmanitan Standard Mode
    { TYPE_GRASS, TYPE_NONE },  // 556 Maractus
    { TYPE_BUG, TYPE_ROCK },  // 557 Dwebble
    { TYPE_BUG, TYPE_ROCK },  // 558 Crustle
    { TYPE_DARK, TYPE_FIGHTING },  // 559 Scraggy
    { TYPE_DARK, TYPE_FIGHTING },  // 560 Scrafty
    { TYPE_PSYCHIC, TYPE_FLYING },  // 561 Sigilyph
    { TYPE_GHOST, TYPE_NONE },  // 562 Yamask
    { TYPE_GHOST, TYPE_NONE },  // 563 Cofagrigus
    { TYPE_WATER, TYPE_ROCK },  // 564 Tirtouga
    { TYPE_WATER, TYPE_ROCK },  // 565 Carracosta
    { TYPE_ROCK, TYPE_FLYING },  // 566 Archen
    { TYPE_ROCK, TYPE_FLYING },  // 567 Archeops
    { TYPE_POISON, TYPE_NONE },  // 568 Trubbish
    { TYPE_POISON, TYPE_NONE },  // 569 Garbodor
    { TYPE_DARK, TYPE_NONE },  // 570 Zorua
    { TYPE_DARK, TYPE_NONE },  // 571 Zoroark
    { TYPE_NORMAL, TYPE_NONE },  // 572 Minccino
    { TYPE_NORMAL, TYPE_NONE },  // 573 Cinccino
    { TYPE_PSYCHIC, TYPE_NONE },  // 574 Gothita
    { TYPE_PSYCHIC, TYPE_NONE },  // 575 Gothorita
    { TYPE_PSYCHIC, TYPE_NONE },  // 576 Gothitelle
    { TYPE_PSYCHIC, TYPE_NONE },  // 577 Solosis
    { TYPE_PSYCHIC, TYPE_NONE },  // 578 Duosion
    { TYPE_PSYCHIC, TYPE_NONE },  // 579 Reuniclus
    { TYPE_WATER, TYPE_FLYING },  // 580 Ducklett
    { TYPE_WATER, TYPE_FLYING },  // 581 Swanna
    { TYPE_ICE, TYPE_NONE },  // 582 Vanillite
    { TYPE_ICE, TYPE_NONE },  // 583 Vanillish
    { TYPE_ICE, TYPE_NONE },  // 584 Vanilluxe
    { TYPE_NORMAL, TYPE_GRASS },  // 585 Deerling
    { TYPE_NORMAL, TYPE_GRASS },  // 586 Sawsbuck
    { TYPE_ELECTRIC, TYPE_FLYING },  // 587 Emolga
    { TYPE_BUG, TYPE_NONE },  // 588 Karrablast
    { TYPE_BUG, TYPE_STEEL },  // 589 Escavalier
    { TYPE_GRASS, TYPE_POISON },  // 590 Foongus
    { TYPE_GRASS, TYPE_POISON },  // 591 Amoonguss
    { TYPE_WATER, TYPE_GHOST },  // 592 Frillish
    { TYPE_WATER, TYPE_GHOST },  // 593 Jellicent
    { TYPE_WATER, TYPE_NONE },  // 594 Alomomola
    { TYPE_BUG, TYPE_ELECTRIC },  // 595 Joltik
    { TYPE_BUG, TYPE_ELECTRIC },  // 596 Galvantula
    { TYPE_GRASS, TYPE_STEEL },  // 597 Ferroseed
    { TYPE_GRASS, TYPE_STEEL },  // 598 Ferrothorn
    { TYPE_STEEL, TYPE_NONE },  // 599 Klink
    { TYPE_STEEL, TYPE_NONE },  // 600 Klang
    { TYPE_STEEL, TYPE_NONE },  // 601 Klinklang
    { TYPE_ELECTRIC, TYPE_NONE },  // 602 Tynamo
    { TYPE_ELECTRIC, TYPE_NONE },  // 603 Eelektrik
    { TYPE_ELECTRIC, TYPE_NONE },  // 604 Eelektross
    { TYPE_PSYCHIC, TYPE_NONE },  // 605 Elgyem
    { TYPE_PSYCHIC, TYPE_NONE },  // 606 Beheeyem
    { TYPE_GHOST, TYPE_FIRE },  // 607 Litwick
    { TYPE_GHOST, TYPE_FIRE },  // 608 Lampent
    { TYPE_GHOST, TYPE_FIRE },  // 609 Chandelure
    { TYPE_DRAGON, TYPE_NONE },  // 610 Axew
    { TYPE_DRAGON, TYPE_NONE },  // 611 Fraxure
    { TYPE_DRAGON, TYPE_NONE },  // 612 Haxorus
    { TYPE_ICE, TYPE_NONE },  // 613 Cubchoo
    { TYPE_ICE, TYPE_NONE },  // 614 Beartic
    { TYPE_ICE, TYPE_NONE },  // 615 Cryogonal
    { TYPE_BUG, TYPE_NONE },  // 616 Shelmet
    { TYPE_BUG, TYPE_NONE },  // 617 Accelgor
    { TYPE_GROUND, TYPE_ELECTRIC },  // 618 Stunfisk
    { TYPE_FIGHTING, TYPE_NONE },  // 619 Mienfoo
    { TYPE_FIGHTING, TYPE_NONE },  // 620 Mienshao
    { TYPE_DRAGON, TYPE_NONE },  // 621 Druddigon
    { TYPE_GROUND, TYPE_GHOST },  // 622 Golett
    { TYPE_GROUND, TYPE_GHOST },  // 623 Golurk
    { TYPE_DARK, TYPE_STEEL },  // 624 Pawniard
    { TYPE_DARK, TYPE_STEEL },  // 625 Bisharp
    { TYPE_NORMAL, TYPE_NONE },  // 626 Bouffalant
    { TYPE_NORMAL, TYPE_FLYING },  // 627 Rufflet
    { TYPE_NORMAL, TYPE_FLYING },  // 628 Braviary
    { TYPE_DARK, TYPE_FLYING },  // 629 Vullaby
    { TYPE_DARK, TYPE_FLYING },  // 630 Mandibuzz
    { TYPE_FIRE, TYPE_NONE },  // 631 Heatmor
    { TYPE_BUG, TYPE_STEEL },  // 632 Durant
    { TYPE_DARK, TYPE_DRAGON },  // 633 Deino
    { TYPE_DARK, TYPE_DRAGON },  // 634 Zweilous
    { TYPE_DARK, TYPE_DRAGON },  // 635 Hydreigon
    { TYPE_BUG, TYPE_FIRE },  // 636 Larvesta
    { TYPE_BUG, TYPE_FIRE },  // 637 Volcarona
    { TYPE_STEEL, TYPE_FIGHTING },  // 638 Cobalion
    { TYPE_ROCK, TYPE_FIGHTING },  // 639 Terrakion
    { TYPE_GRASS, TYPE_FIGHTING },  // 640 Virizion
    { TYPE_FLYING, TYPE_NONE },  // 641 Tornadus Incarnate Forme
    { TYPE_ELECTRIC, TYPE_FLYING },  // 642 Thundurus Incarnate Forme
    { TYPE_DRAGON, TYPE_FIRE },  // 643 Reshiram
    { TYPE_DRAGON, TYPE_ELECTRIC },  // 644 Zekrom
    { TYPE_GROUND, TYPE_FLYING },  // 645 Landorus Incarnate Forme
    { TYPE_DRAGON, TYPE_ICE },  // 646 Kyurem
    { TYPE_WATER, TYPE_FIGHTING },  // 647 Keldeo Ordinary Form
    { TYPE_NORMAL, TYPE_PSYCHIC },  // 648 Meloetta Aria Forme
    { TYPE_BUG, TYPE_STEEL },  // 649 Genesect
    { TYPE_GRASS, TYPE_NONE },  // 650 Chespin
    { TYPE_GRASS, TYPE_NONE },  // 651 Quilladin
    { TYPE_GRASS, TYPE_FIGHTING },  // 652 Chesnaught
    { TYPE_FIRE, TYPE_NONE },  // 653 Fennekin
    { TYPE_FIRE, TYPE_NONE },  // 654 Braixen
    { TYPE_FIRE, TYPE_PSYCHIC },  // 655 Delphox
    { TYPE_WATER, TYPE_NONE },  // 656 Froakie
    { TYPE_WATER, TYPE_NONE },  // 657 Frogadier
    { TYPE_WATER, TYPE_DARK },  // 658 Greninja
    { TYPE_NORMAL, TYPE_NONE },  // 659 Bunnelby
    { TYPE_NORMAL, TYPE_GROUND },  // 660 Diggersby
    { TYPE_NORMAL, TYPE_FLYING },  // 661 Fletchling
    { TYPE_FIRE, TYPE_FLYING },  // 662 Fletchinder
    { TYPE_FIRE, TYPE_FLYING },  // 663 Talonflame
    { TYPE_BUG, TYPE_NONE },  // 664 Scatterbug
    { TYPE_BUG, TYPE_NONE },  // 665 Spewpa
    { TYPE_BUG, TYPE_FLYING },  // 666 Vivillon
    { TYPE_FIRE, TYPE_NORMAL },  // 667 Litleo
    { TYPE_FIRE, TYPE_NORMAL },  // 668 Pyroar
    { TYPE_FAIRY, TYPE_NONE },  // 669 Flabebe
    { TYPE_FAIRY, TYPE_NONE },  // 670 Floette
    { TYPE_FAIRY, TYPE_NONE },  // 671 Florges
    { TYPE_GRASS, TYPE_NONE },  // 672 Skiddo
    { TYPE_GRASS, TYPE_NONE },  // 673 Gogoat
    { TYPE_FIGHTING, TYPE_NONE },  // 674 Pancham
    { TYPE_FIGHTING, TYPE_DARK },  // 675 Pangoro
    { TYPE_NORMAL, TYPE_NONE },  // 676 Furfrou
    { TYPE_PSYCHIC, TYPE_NONE },  // 677 Espurr
    { TYPE_PSYCHIC, TYPE_NONE },  // 678 Meowstic Male
    { TYPE_STEEL, TYPE_GHOST },  // 679 Honedge
    { TYPE_STEEL, TYPE_GHOST },  // 680 Doublade
    { TYPE_STEEL, TYPE_GHOST },  // 681 Aegislash Shield Forme
    { TYPE_FAIRY, TYPE_NONE },  // 682 Spritzee
    { TYPE_FAIRY, TYPE_NONE },  // 683 Aromatisse
    { TYPE_FAIRY, TYPE_NONE },  // 684 Swirlix
    { TYPE_FAIRY, TYPE_NONE },  // 685 Slurpuff
    { TYPE_DARK, TYPE_PSYCHIC },  // 686 Inkay
    { TYPE_DARK, TYPE_PSYCHIC },  // 687 Malamar
    { TYPE_ROCK, TYPE_WATER },  // 688 Binacle
    { TYPE_ROCK, TYPE_WATER },  // 689 Barbaracle
    { TYPE_POISON, TYPE_WATER },  // 690 Skrelp
    { TYPE_POISON, TYPE_DRAGON },  // 691 Dragalge
    { TYPE_WATER, TYPE_NONE },  // 692 Clauncher
    { TYPE_WATER, TYPE_NONE },  // 693 Clawitzer
    { TYPE_ELECTRIC, TYPE_NORMAL },  // 694 Helioptile
    { TYPE_ELECTRIC, TYPE_NORMAL },  // 695 Heliolisk
    { TYPE_ROCK, TYPE_DRAGON },  // 696 Tyrunt
    { TYPE_ROCK, TYPE_DRAGON },  // 697 Tyrantrum
    { TYPE_ROCK, TYPE_ICE },  // 698 Amaura
    { TYPE_ROCK, TYPE_ICE },  // 699 Aurorus
    { TYPE_FAIRY, TYPE_NONE },  // 700 Sylveon
    { TYPE_FIGHTING, TYPE_FLYING },  // 701 Hawlucha
    { TYPE_ELECTRIC, TYPE_FAIRY },  // 702 Dedenne
    { TYPE_ROCK, TYPE_FAIRY },  // 703 Carbink
    { TYPE_DRAGON, TYPE_NONE },  // 704 Goomy
    { TYPE_DRAGON, TYPE_NONE },  // 705 Sliggoo
    { TYPE_DRAGON, TYPE_NONE },  // 706 Goodra
    { TYPE_STEEL, TYPE_FAIRY },  // 707 Klefki
    { TYPE_GHOST, TYPE_GRASS },  // 708 Phantump
    { TYPE_GHOST, TYPE_GRASS },  // 709 Trevenant
    { TYPE_GHOST, TYPE_GRASS },  // 710 Pumpkaboo Average Size
    { TYPE_GHOST, TYPE_GRASS },  // 711 Gourgeist Average Size
    { TYPE_ICE, TYPE_NONE },  // 712 Bergmite
    { TYPE_ICE, TYPE_NONE },  // 713 Avalugg
    { TYPE_FLYING, TYPE_DRAGON },  // 714 Noibat
    { TYPE_FLYING, TYPE_DRAGON },  // 715 Noivern
    { TYPE_FAIRY, TYPE_NONE },  // 716 Xerneas
    { TYPE_DARK, TYPE_FLYING },  // 717 Yveltal
    { TYPE_DRAGON, TYPE_GROUND },  // 718 Zygarde 50% Forme
    { TYPE_ROCK, TYPE_FAIRY },  // 719 Diancie
    { TYPE_PSYCHIC, TYPE_GHOST },  // 720 Hoopa Hoopa Confined
    { TYPE_FIRE, TYPE_WATER },  // 721 Volcanion
    { TYPE_GRASS, TYPE_FLYING },  // 722 Rowlet
    { TYPE_GRASS, TYPE_FLYING },  // 723 Dartrix
    { TYPE_GRASS, TYPE_GHOST },  // 724 Decidueye
    { TYPE_FIRE, TYPE_NONE },  // 725 Litten
    { TYPE_FIRE, TYPE_NONE },  // 726 Torracat
    { TYPE_FIRE, TYPE_DARK },  // 727 Incineroar
    { TYPE_WATER, TYPE_NONE },  // 728 Popplio
    { TYPE_WATER, TYPE_NONE },  // 729 Brionne
    { TYPE_WATER, TYPE_FAIRY },  // 730 Primarina
    { TYPE_NORMAL, TYPE_FLYING },  // 731 Pikipek
    { TYPE_NORMAL, TYPE_FLYING },  // 732 Trumbeak
    { TYPE_NORMAL, TYPE_FLYING },  // 733 Toucannon
    { TYPE_NORMAL, TYPE_NONE },  // 734 Yungoos
    { TYPE_NORMAL, TYPE_NONE },  // 735 Gumshoos
    { TYPE_BUG, TYPE_NONE },  // 736 Grubbin
    { TYPE_BUG, TYPE_ELECTRIC },  // 737 Charjabug
    { TYPE_BUG, TYPE_ELECTRIC },  // 738 Vikavolt
    { TYPE_FIGHTING, TYPE_NONE },  // 739 Crabrawler
    { TYPE_FIGHTING, TYPE_ICE },  // 740 Crabominable
    { TYPE_FIRE, TYPE_FLYING },  // 741 Oricorio Baile Style
    { TYPE_BUG, TYPE_FAIRY },  // 742 Cutiefly
    { TYPE_BUG, TYPE_FAIRY },  // 743 Ribombee
    { TYPE_ROCK, TYPE_NONE },  // 744 Rockruff
    { TYPE_ROCK, TYPE_NONE },  // 745 Lycanroc Midday Form
    { TYPE_WATER, TYPE_NONE },  // 746 Wishiwashi Solo Form
    { TYPE_POISON, TYPE_WATER },  // 747 Mareanie
    { TYPE_POISON, TYPE_WATER },  // 748 Toxapex
    { TYPE_GROUND, TYPE_NONE },  // 749 Mudbray
    { TYPE_GROUND, TYPE_NONE },  // 750 Mudsdale
    { TYPE_WATER, TYPE_BUG },  // 751 Dewpider
    { TYPE_WATER, TYPE_BUG },  // 752 Araquanid
    { TYPE_GRASS, TYPE_NONE },  // 753 Fomantis
    { TYPE_GRASS, TYPE_NONE },  // 754 Lurantis
    { TYPE_GRASS, TYPE_FAIRY },  // 755 Morelull
    { TYPE_GRASS, TYPE_FAIRY },  // 756 Shiinotic
    { TYPE_POISON, TYPE_FIRE },  // 757 Salandit
    { TYPE_POISON, TYPE_FIRE },  // 758 Salazzle
    { TYPE_NORMAL, TYPE_FIGHTING },  // 759 Stufful
    { TYPE_NORMAL, TYPE_FIGHTING },  // 760 Bewear
    { TYPE_GRASS, TYPE_NONE },  // 761 Bounsweet
    { TYPE_GRASS, TYPE_NONE },  // 762 Steenee
    { TYPE_GRASS, TYPE_NONE },  // 763 Tsareena
    { TYPE_FAIRY, TYPE_NONE },  // 764 Comfey
    { TYPE_NORMAL, TYPE_PSYCHIC },  // 765 Oranguru
    { TYPE_FIGHTING, TYPE_NONE },  // 766 Passimian
    { TYPE_BUG, TYPE_WATER },  // 767 Wimpod
    { TYPE_BUG, TYPE_WATER },  // 768 Golisopod
    { TYPE_GHOST, TYPE_GROUND },  // 769 Sandygast
    { TYPE_GHOST, TYPE_GROUND },  // 770 Palossand
    { TYPE_WATER, TYPE_NONE },  // 771 Pyukumuku
    { TYPE_NORMAL, TYPE_NONE },  // 772 Type - Null
    { TYPE_NORMAL, TYPE_NONE },  // 773 Silvally
    { TYPE_ROCK, TYPE_FLYING },  // 774 Minior Meteor Form
    { TYPE_NORMAL, TYPE_NONE },  // 775 Komala
    { TYPE_FIRE, TYPE_DRAGON },  // 776 Turtonator
    { TYPE_ELECTRIC, TYPE_STEEL },  // 777 Togedemaru
    { TYPE_GHOST, TYPE_FAIRY },  // 778 Mimikyu
    { TYPE_WATER, TYPE_PSYCHIC },  // 779 Bruxish
    { TYPE_NORMAL, TYPE_DRAGON },  // 780 Drampa
    { TYPE_GHOST, TYPE_GRASS },  // 781 Dhelmise
    { TYPE_DRAGON, TYPE_NONE },  // 782 Jangmo-o
    { TYPE_DRAGON, TYPE_FIGHTING },  // 783 Hakamo-o
    { TYPE_DRAGON, TYPE_FIGHTING },  // 784 Kommo-o
    { TYPE_ELECTRIC, TYPE_FAIRY },  // 785 Tapu Koko
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 786 Tapu Lele
    { TYPE_GRASS, TYPE_FAIRY },  // 787 Tapu Bulu
    { TYPE_WATER, TYPE_FAIRY },  // 788 Tapu Fini
    { TYPE_PSYCHIC, TYPE_NONE },  // 789 Cosmog
    { TYPE_PSYCHIC, TYPE_NONE },  // 790 Cosmoem
    { TYPE_PSYCHIC, TYPE_STEEL },  // 791 Solgaleo
    { TYPE_PSYCHIC, TYPE_GHOST },  // 792 Lunala
    { TYPE_ROCK, TYPE_POISON },  // 793 Nihilego
    { TYPE_BUG, TYPE_FIGHTING },  // 794 Buzzwole
    { TYPE_BUG, TYPE_FIGHTING },  // 795 Pheromosa
    { TYPE_ELECTRIC, TYPE_NONE },  // 796 Xurkitree
    { TYPE_STEEL, TYPE_FLYING },  // 797 Celesteela
    { TYPE_GRASS, TYPE_STEEL },  // 798 Kartana
    { TYPE_DARK, TYPE_DRAGON },  // 799 Guzzlord
    { TYPE_PSYCHIC, TYPE_NONE },  // 800 Necrozma
    { TYPE_STEEL, TYPE_FAIRY },  // 801 Magearna
    { TYPE_FIGHTING, TYPE_GHOST },  // 802 Marshadow
    { TYPE_POISON, TYPE_NONE },  // 803 Poipole
    { TYPE_POISON, TYPE_DRAGON },  // 804 Naganadel
    { TYPE_ROCK, TYPE_STEEL },  // 805 Stakataka
    { TYPE_FIRE, TYPE_GHOST },  // 806 Blacephalon
    { TYPE_ELECTRIC, TYPE_NONE },  // 807 Zeraora
    { TYPE_STEEL, TYPE_NONE },  // 808 Meltan
    { TYPE_STEEL, TYPE_NONE },  // 809 Melmetal
    { TYPE_GRASS, TYPE_NONE },  // 810 Grookey
    { TYPE_GRASS, TYPE_NONE },  // 811 Thwackey
    { TYPE_GRASS, TYPE_NONE },  // 812 Rillaboom
    { TYPE_FIRE, TYPE_NONE },  // 813 Scorbunny
    { TYPE_FIRE, TYPE_NONE },  // 814 Raboot
    { TYPE_FIRE, TYPE_NONE },  // 815 Cinderace
    { TYPE_WATER, TYPE_NONE },  // 816 Sobble
    { TYPE_WATER, TYPE_NONE },  // 817 Drizzile
    { TYPE_WATER, TYPE_NONE },  // 818 Inteleon
    { TYPE_NORMAL, TYPE_NONE },  // 819 Skwovet
    { TYPE_NORMAL, TYPE_NONE },  // 820 Greedent
    { TYPE_FLYING, TYPE_NONE },  // 821 Rookidee
    { TYPE_FLYING, TYPE_NONE },  // 822 Corvisquire
    { TYPE_FLYING, TYPE_STEEL },  // 823 Corviknight
    { TYPE_BUG, TYPE_NONE },  // 824 Blipbug
    { TYPE_BUG, TYPE_PSYCHIC },  // 825 Dottler
    { TYPE_BUG, TYPE_PSYCHIC },  // 826 Orbeetle
    { TYPE_DARK, TYPE_NONE },  // 827 Nickit
    { TYPE_DARK, TYPE_NONE },  // 828 Thievul
    { TYPE_GRASS, TYPE_NONE },  // 829 Gossifleur
    { TYPE_GRASS, TYPE_NONE },  // 830 Eldegoss
    { TYPE_NORMAL, TYPE_NONE },  // 831 Wooloo
    { TYPE_NORMAL, TYPE_NONE },  // 832 Dubwool
    { TYPE_WATER, TYPE_NONE },  // 833 Chewtle
    { TYPE_WATER, TYPE_ROCK },  // 834 Drednaw
    { TYPE_ELECTRIC, TYPE_NONE },  // 835 Yamper
    { TYPE_ELECTRIC, TYPE_NONE },  // 836 Boltund
    { TYPE_ROCK, TYPE_NONE },  // 837 Rolycoly
    { TYPE_ROCK, TYPE_FIRE },  // 838 Carkol
    { TYPE_ROCK, TYPE_FIRE },  // 839 Coalossal
    { TYPE_GRASS, TYPE_DRAGON },  // 840 Applin
    { TYPE_GRASS, TYPE_DRAGON },  // 841 Flapple
    { TYPE_GRASS, TYPE_DRAGON },  // 842 Appletun
    { TYPE_GROUND, TYPE_NONE },  // 843 Silicobra
    { TYPE_GROUND, TYPE_NONE },  // 844 Sandaconda
    { TYPE_FLYING, TYPE_WATER },  // 845 Cramorant
    { TYPE_WATER, TYPE_NONE },  // 846 Arrokuda
    { TYPE_WATER, TYPE_NONE },  // 847 Barraskewda
    { TYPE_ELECTRIC, TYPE_POISON },  // 848 Toxel
    { TYPE_ELECTRIC, TYPE_POISON },  // 849 Toxtricity Amped Form
    { TYPE_FIRE, TYPE_BUG },  // 850 Sizzlipede
    { TYPE_FIRE, TYPE_BUG },  // 851 Centiskorch
    { TYPE_FIGHTING, TYPE_NONE },  // 852 Clobbopus
    { TYPE_FIGHTING, TYPE_NONE },  // 853 Grapploct
    { TYPE_GHOST, TYPE_NONE },  // 854 Sinistea
    { TYPE_GHOST, TYPE_NONE },  // 855 Polteageist
    { TYPE_PSYCHIC, TYPE_NONE },  // 856 Hatenna
    { TYPE_PSYCHIC, TYPE_NONE },  // 857 Hattrem
    { TYPE_PSYCHIC, TYPE_FAIRY },  // 858 Hatterene
    { TYPE_DARK, TYPE_FAIRY },  // 859 Impidimp
    { TYPE_DARK, TYPE_FAIRY },  // 860 Morgrem
    { TYPE_DARK, TYPE_FAIRY },  // 861 Grimmsnarl
    { TYPE_DARK, TYPE_NORMAL },  // 862 Obstagoon
    { TYPE_STEEL, TYPE_NONE },  // 863 Perrserker
    { TYPE_GHOST, TYPE_NONE },  // 864 Cursola
    { TYPE_FIGHTING, TYPE_NONE },  // 865 Sirfetch'd
    { TYPE_ICE, TYPE_PSYCHIC },  // 866 Mr. Rime
    { TYPE_GROUND, TYPE_GHOST },  // 867 Runerigus
    { TYPE_FAIRY, TYPE_NONE },  // 868 Milcery
    { TYPE_FAIRY, TYPE_NONE },  // 869 Alcremie
    { TYPE_FIGHTING, TYPE_NONE },  // 870 Falinks
    { TYPE_ELECTRIC, TYPE_NONE },  // 871 Pincurchin
    { TYPE_ICE, TYPE_BUG },  // 872 Snom
    { TYPE_ICE, TYPE_BUG },  // 873 Frosmoth
    { TYPE_ROCK, TYPE_NONE },  // 874 Stonjourner
    { TYPE_ICE, TYPE_NONE },  // 875 Eiscue Ice Face
    { TYPE_PSYCHIC, TYPE_NORMAL },  // 876 Indeedee Male
    { TYPE_ELECTRIC, TYPE_DARK },  // 877 Morpeko Full Belly Mode
    { TYPE_STEEL, TYPE_NONE },  // 878 Cufant
    { TYPE_STEEL, TYPE_NONE },  // 879 Copperajah
    { TYPE_ELECTRIC, TYPE_DRAGON },  // 880 Dracozolt
    { TYPE_ELECTRIC, TYPE_ICE },  // 881 Arctozolt
    { TYPE_WATER, TYPE_DRAGON },  // 882 Dracovish
    { TYPE_WATER, TYPE_ICE },  // 883 Arctovish
    { TYPE_STEEL, TYPE_DRAGON },  // 884 Duraludon
    { TYPE_DRAGON, TYPE_GHOST },  // 885 Dreepy
    { TYPE_DRAGON, TYPE_GHOST },  // 886 Drakloak
    { TYPE_DRAGON, TYPE_GHOST },  // 887 Dragapult
    { TYPE_FAIRY, TYPE_NONE },  // 888 Zacian Hero of Many Battles
    { TYPE_FIGHTING, TYPE_NONE },  // 889 Zamazenta Hero of Many Battles
    { TYPE_POISON, TYPE_DRAGON },  // 890 Eternatus
    { TYPE_FIGHTING, TYPE_NONE },  // 891 Kubfu
    { TYPE_FIGHTING, TYPE_DARK },  // 892 Urshifu Single Strike Style
    { TYPE_DARK, TYPE_GRASS },  // 893 Zarude
    { TYPE_ELECTRIC, TYPE_NONE },  // 894 Regieleki
    { TYPE_DRAGON, TYPE_NONE },  // 895 Regidrago
    { TYPE_ICE, TYPE_NONE },  // 896 Glastrier
    { TYPE_GHOST, TYPE_NONE },  // 897 Spectrier
    { TYPE_PSYCHIC, TYPE_GRASS },  // 898 Calyrex
    { TYPE_NORMAL, TYPE_PSYCHIC },  // 899 Wyrdeer
    { TYPE_BUG, TYPE_ROCK },  // 900 Kleavor
    { TYPE_GROUND, TYPE_NORMAL },  // 901 Ursaluna
    { TYPE_WATER, TYPE_GHOST },  // 902 Basculegion Male
    { TYPE_FIGHTING, TYPE_POISON },  // 903 Sneasler
    { TYPE_DARK, TYPE_POISON },  // 904 Overqwil
    { TYPE_FAIRY, TYPE_FLYING },  // 905 Enamorus Incarnate Forme
    { TYPE_GRASS, TYPE_NONE },  // 906 Sprigatito
    { TYPE_GRASS, TYPE_NONE },  // 907 Floragato
    { TYPE_GRASS, TYPE_DARK },  // 908 Meowscarada
    { TYPE_FIRE, TYPE_NONE },  // 909 Fuecoco
    { TYPE_FIRE, TYPE_NONE },  // 910 Crocalor
    { TYPE_FIRE, TYPE_GHOST },  // 911 Skeledirge
    { TYPE_WATER, TYPE_NONE },  // 912 Quaxly
    { TYPE_WATER, TYPE_NONE },  // 913 Quaxwell
    { TYPE_WATER, TYPE_FIGHTING },  // 914 Quaquaval
    { TYPE_NORMAL, TYPE_NONE },  // 915 Lechonk
    { TYPE_NORMAL, TYPE_NONE },  // 916 Oinkologne Male
    { TYPE_BUG, TYPE_NONE },  // 917 Tarountula
    { TYPE_BUG, TYPE_NONE },  // 918 Spidops
    { TYPE_BUG, TYPE_NONE },  // 919 Nymble
    { TYPE_BUG, TYPE_DARK },  // 920 Lokix
    { TYPE_ELECTRIC, TYPE_NONE },  // 921 Pawmi
    { TYPE_ELECTRIC, TYPE_FIGHTING },  // 922 Pawmo
    { TYPE_ELECTRIC, TYPE_FIGHTING },  // 923 Pawmot
    { TYPE_NORMAL, TYPE_NONE },  // 924 Tandemaus
    { TYPE_NORMAL, TYPE_NONE },  // 925 Maushold Family of Four
    { TYPE_FAIRY, TYPE_NONE },  // 926 Fidough
    { TYPE_FAIRY, TYPE_NONE },  // 927 Dachsbun
    { TYPE_GRASS, TYPE_NORMAL },  // 928 Smoliv
    { TYPE_GRASS, TYPE_NORMAL },  // 929 Dolliv
    { TYPE_GRASS, TYPE_NORMAL },  // 930 Arboliva
    { TYPE_NORMAL, TYPE_FLYING },  // 931 Squawkabilly Green Plumage
    { TYPE_ROCK, TYPE_NONE },  // 932 Nacli
    { TYPE_ROCK, TYPE_NONE },  // 933 Naclstack
    { TYPE_ROCK, TYPE_NONE },  // 934 Garganacl
    { TYPE_FIRE, TYPE_NONE },  // 935 Charcadet
    { TYPE_FIRE, TYPE_PSYCHIC },  // 936 Armarouge
    { TYPE_FIRE, TYPE_GHOST },  // 937 Ceruledge
    { TYPE_ELECTRIC, TYPE_NONE },  // 938 Tadbulb
    { TYPE_ELECTRIC, TYPE_NONE },  // 939 Bellibolt
    { TYPE_ELECTRIC, TYPE_FLYING },  // 940 Wattrel
    { TYPE_ELECTRIC, TYPE_FLYING },  // 941 Kilowattrel
    { TYPE_DARK, TYPE_NONE },  // 942 Maschiff
    { TYPE_DARK, TYPE_NONE },  // 943 Mabosstiff
    { TYPE_POISON, TYPE_NORMAL },  // 944 Shroodle
    { TYPE_POISON, TYPE_NORMAL },  // 945 Grafaiai
    { TYPE_GRASS, TYPE_GHOST },  // 946 Bramblin
    { TYPE_GRASS, TYPE_GHOST },  // 947 Brambleghast
    { TYPE_GROUND, TYPE_GRASS },  // 948 Toedscool
    { TYPE_GROUND, TYPE_GRASS },  // 949 Toedscruel
    { TYPE_ROCK, TYPE_NONE },  // 950 Klawf
    { TYPE_GRASS, TYPE_NONE },  // 951 Capsakid
    { TYPE_GRASS, TYPE_FIRE },  // 952 Scovillain
    { TYPE_BUG, TYPE_NONE },  // 953 Rellor
    { TYPE_BUG, TYPE_PSYCHIC },  // 954 Rabsca
    { TYPE_PSYCHIC, TYPE_NONE },  // 955 Flittle
    { TYPE_PSYCHIC, TYPE_NONE },  // 956 Espathra
    { TYPE_FAIRY, TYPE_STEEL },  // 957 Tinkatink
    { TYPE_FAIRY, TYPE_STEEL },  // 958 Tinkatuff
    { TYPE_FAIRY, TYPE_STEEL },  // 959 Tinkaton
    { TYPE_WATER, TYPE_NONE },  // 960 Wiglett
    { TYPE_WATER, TYPE_NONE },  // 961 Wugtrio
    { TYPE_FLYING, TYPE_DARK },  // 962 Bombirdier
    { TYPE_WATER, TYPE_NONE },  // 963 Finizen
    { TYPE_WATER, TYPE_NONE },  // 964 Palafin Zero Form
    { TYPE_STEEL, TYPE_POISON },  // 965 Varoom
    { TYPE_STEEL, TYPE_POISON },  // 966 Revavroom
    { TYPE_DRAGON, TYPE_NORMAL },  // 967 Cyclizar
    { TYPE_STEEL, TYPE_NONE },  // 968 Orthworm
    { TYPE_ROCK, TYPE_POISON },  // 969 Glimmet
    { TYPE_ROCK, TYPE_POISON },  // 970 Glimmora
    { TYPE_GHOST, TYPE_NONE },  // 971 Greavard
    { TYPE_GHOST, TYPE_NONE },  // 972 Houndstone
    { TYPE_FLYING, TYPE_FIGHTING },  // 973 Flamigo
    { TYPE_ICE, TYPE_NONE },  // 974 Cetoddle
    { TYPE_ICE, TYPE_NONE },  // 975 Cetitan
    { TYPE_WATER, TYPE_PSYCHIC },  // 976 Veluza
    { TYPE_WATER, TYPE_NONE },  // 977 Dondozo
    { TYPE_DRAGON, TYPE_WATER },  // 978 Tatsugiri Curly Form
    { TYPE_FIGHTING, TYPE_GHOST },  // 979 Annihilape
    { TYPE_POISON, TYPE_GROUND },  // 980 Clodsire
    { TYPE_NORMAL, TYPE_PSYCHIC },  // 981 Farigiraf
    { TYPE_NORMAL, TYPE_NONE },  // 982 Dudunsparce Two-Segment Form
    { TYPE_DARK, TYPE_STEEL },  // 983 Kingambit
    { TYPE_GROUND, TYPE_FIGHTING },  // 984 Great Tusk
    { TYPE_FAIRY, TYPE_PSYCHIC },  // 985 Scream Tail
    { TYPE_GRASS, TYPE_DARK },  // 986 Brute Bonnet
    { TYPE_GHOST, TYPE_FAIRY },  // 987 Flutter Mane
    { TYPE_BUG, TYPE_FIGHTING },  // 988 Slither Wing
    { TYPE_ELECTRIC, TYPE_GROUND },  // 989 Sandy Shocks
    { TYPE_GROUND, TYPE_STEEL },  // 990 Iron Treads
    { TYPE_ICE, TYPE_WATER },  // 991 Iron Bundle
    { TYPE_FIGHTING, TYPE_ELECTRIC },  // 992 Iron Hands
    { TYPE_DARK, TYPE_FLYING },  // 993 Iron Jugulis
    { TYPE_FIRE, TYPE_POISON },  // 994 Iron Moth
    { TYPE_ROCK, TYPE_ELECTRIC },  // 995 Iron Thorns
    { TYPE_DRAGON, TYPE_ICE },  // 996 Frigibax
    { TYPE_DRAGON, TYPE_ICE },  // 997 Arctibax
    { TYPE_DRAGON, TYPE_ICE },  // 998 Baxcalibur
    { TYPE_GHOST, TYPE_NONE },  // 999 Gimmighoul Chest Form
    { TYPE_STEEL, TYPE_GHOST },  // 1000 Gholdengo
    { TYPE_DARK, TYPE_GRASS },  // 1001 Wo-Chien
    { TYPE_DARK, TYPE_ICE },  // 1002 Chien-Pao
    { TYPE_DARK, TYPE_GROUND },  // 1003 Ting-Lu
    { TYPE_DARK, TYPE_FIRE },  // 1004 Chi-Yu
    { TYPE_DRAGON, TYPE_DARK },  // 1005 Roaring Moon
    { TYPE_FAIRY, TYPE_FIGHTING },  // 1006 Iron Valiant
    { TYPE_FIGHTING, TYPE_DRAGON },  // 1007 Koraidon
    { TYPE_ELECTRIC, TYPE_DRAGON },  // 1008 Miraidon
    { TYPE_WATER, TYPE_DRAGON },  // 1009 Walking Wake
    { TYPE_GRASS, TYPE_PSYCHIC },  // 1010 Iron Leaves
    { TYPE_GRASS, TYPE_DRAGON },  // 1011 Dipplin
    { TYPE_GRASS, TYPE_GHOST },  // 1012 Poltchageist
    { TYPE_GRASS, TYPE_GHOST },  // 1013 Sinistcha
    { TYPE_POISON, TYPE_FIGHTING },  // 1014 Okidogi
    { TYPE_POISON, TYPE_PSYCHIC },  // 1015 Munkidori
    { TYPE_POISON, TYPE_FAIRY },  // 1016 Fezandipiti
    { TYPE_GRASS, TYPE_NONE },  // 1017 Ogerpon Teal Mask
    { TYPE_STEEL, TYPE_DRAGON },  // 1018 Archaludon
    { TYPE_GRASS, TYPE_DRAGON },  // 1019 Hydrapple
    { TYPE_FIRE, TYPE_DRAGON },  // 1020 Gouging Fire
    { TYPE_ELECTRIC, TYPE_DRAGON },  // 1021 Raging Bolt
    { TYPE_ROCK, TYPE_PSYCHIC },  // 1022 Iron Boulder
    { TYPE_STEEL, TYPE_PSYCHIC },  // 1023 Iron Crown
    { TYPE_NORMAL, TYPE_NONE },  // 1024 Terapagos Normal Form
    { TYPE_POISON, TYPE_GHOST },  // 1025 Pecharunt
};

// Get base type data for any species (returns base form types)
static TypePair getBaseTypes(uint16_t speciesId) {
    if (speciesId < sizeof(BASE_TYPES) / sizeof(BASE_TYPES[0])) {
        return BASE_TYPES[speciesId];
    }
    return {TYPE_NORMAL, TYPE_NONE};
}

TypePair getPokemonTypes(uint16_t speciesId, uint8_t formId) {
    // Handle form-specific type changes
    if (formId > 0) {
        switch (speciesId) {
            // Alolan Forms
            case 19:  // Rattata-Alola
            case 20:  // Raticate-Alola
                if (formId == 1) return {TYPE_DARK, TYPE_NORMAL};
                break;
            case 26:  // Raichu-Alola
                if (formId == 1) return {TYPE_ELECTRIC, TYPE_PSYCHIC};
                break;
            case 27:  // Sandshrew-Alola
                if (formId == 1) return {TYPE_ICE, TYPE_STEEL};
                break;
            case 28:  // Sandslash-Alola
                if (formId == 1) return {TYPE_ICE, TYPE_STEEL};
                break;
            case 37:  // Vulpix-Alola
                if (formId == 1) return {TYPE_ICE, TYPE_NONE};
                break;
            case 38:  // Ninetales-Alola
                if (formId == 1) return {TYPE_ICE, TYPE_FAIRY};
                break;
            case 50:  // Diglett-Alola
            case 51:  // Dugtrio-Alola
                if (formId == 1) return {TYPE_GROUND, TYPE_STEEL};
                break;
            case 52:  // Meowth
                if (formId == 1) return {TYPE_DARK, TYPE_NONE};  // Alolan
                if (formId == 2) return {TYPE_STEEL, TYPE_NONE}; // Galarian
                break;
            case 53:  // Persian
                if (formId == 1) return {TYPE_DARK, TYPE_NONE};  // Alolan
                break;
            case 74:  // Geodude-Alola
            case 75:  // Graveler-Alola
            case 76:  // Golem-Alola
                if (formId == 1) return {TYPE_ROCK, TYPE_ELECTRIC};
                break;
            case 88:  // Grimer-Alola
            case 89:  // Muk-Alola
                if (formId == 1) return {TYPE_POISON, TYPE_DARK};
                break;
            case 103: // Exeggutor-Alola
                if (formId == 1) return {TYPE_GRASS, TYPE_DRAGON};
                break;
            case 105: // Marowak-Alola
                if (formId == 1) return {TYPE_FIRE, TYPE_GHOST};
                break;

            // Galarian Forms
            case 77:  // Ponyta-Galar
            case 78:  // Rapidash-Galar
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_FAIRY};
                break;
            case 79:  // Slowpoke-Galar
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_NONE};
                break;
            case 80:  // Slowbro-Galar
                if (formId == 1) return {TYPE_POISON, TYPE_PSYCHIC};
                break;
            case 83:  // Farfetch'd-Galar
                if (formId == 1) return {TYPE_FIGHTING, TYPE_NONE};
                break;
            case 110: // Weezing-Galar
                if (formId == 1) return {TYPE_POISON, TYPE_FAIRY};
                break;
            case 122: // Mr. Mime-Galar
                if (formId == 1) return {TYPE_ICE, TYPE_PSYCHIC};
                break;
            case 144: // Articuno-Galar
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_FLYING};
                break;
            case 145: // Zapdos-Galar
                if (formId == 1) return {TYPE_FIGHTING, TYPE_FLYING};
                break;
            case 146: // Moltres-Galar
                if (formId == 1) return {TYPE_DARK, TYPE_FLYING};
                break;
            case 199: // Slowking-Galar
                if (formId == 1) return {TYPE_POISON, TYPE_PSYCHIC};
                break;
            case 222: // Corsola-Galar
                if (formId == 1) return {TYPE_GHOST, TYPE_NONE};
                break;
            case 263: // Zigzagoon-Galar
            case 264: // Linoone-Galar
                if (formId == 1) return {TYPE_DARK, TYPE_NORMAL};
                break;
            case 554: // Darumaka-Galar
                if (formId == 1) return {TYPE_ICE, TYPE_NONE};
                break;
            case 555: // Darmanitan-Galar
                if (formId == 1) return {TYPE_ICE, TYPE_NONE};
                break;
            case 562: // Yamask-Galar
                if (formId == 1) return {TYPE_GROUND, TYPE_GHOST};
                break;
            case 618: // Stunfisk-Galar
                if (formId == 1) return {TYPE_GROUND, TYPE_STEEL};
                break;

            // Hisuian Forms
            case 58:  // Growlithe-Hisui
                if (formId == 1) return {TYPE_FIRE, TYPE_ROCK};
                break;
            case 59:  // Arcanine-Hisui
                if (formId == 1) return {TYPE_FIRE, TYPE_ROCK};
                break;
            case 100: // Voltorb-Hisui
            case 101: // Electrode-Hisui
                if (formId == 1) return {TYPE_ELECTRIC, TYPE_GRASS};
                break;
            case 157: // Typhlosion-Hisui
                if (formId == 1) return {TYPE_FIRE, TYPE_GHOST};
                break;
            case 211: // Qwilfish-Hisui
                if (formId == 1) return {TYPE_DARK, TYPE_POISON};
                break;
            case 215: // Sneasel-Hisui
                if (formId == 1) return {TYPE_FIGHTING, TYPE_POISON};
                break;
            case 503: // Samurott-Hisui
                if (formId == 1) return {TYPE_WATER, TYPE_DARK};
                break;
            case 549: // Lilligant-Hisui
                if (formId == 1) return {TYPE_GRASS, TYPE_FIGHTING};
                break;
            case 570: // Zorua-Hisui
            case 571: // Zoroark-Hisui
                if (formId == 1) return {TYPE_NORMAL, TYPE_GHOST};
                break;
            case 705: // Sliggoo-Hisui
            case 706: // Goodra-Hisui
                if (formId == 1) return {TYPE_STEEL, TYPE_DRAGON};
                break;
            case 713: // Avalugg-Hisui
                if (formId == 1) return {TYPE_ICE, TYPE_ROCK};
                break;
            case 724: // Decidueye-Hisui
                if (formId == 1) return {TYPE_GRASS, TYPE_FIGHTING};
                break;

            // Paldean Forms
            case 128: // Tauros-Paldea
                if (formId == 1) return {TYPE_FIGHTING, TYPE_NONE};  // Combat
                if (formId == 2) return {TYPE_FIGHTING, TYPE_FIRE};  // Blaze
                if (formId == 3) return {TYPE_FIGHTING, TYPE_WATER}; // Aqua
                break;
            case 194: // Wooper-Paldea
                if (formId == 1) return {TYPE_POISON, TYPE_GROUND};
                break;

            // Rotom Forms
            case 479:
                if (formId == 1) return {TYPE_ELECTRIC, TYPE_FIRE};   // Heat
                if (formId == 2) return {TYPE_ELECTRIC, TYPE_WATER};  // Wash
                if (formId == 3) return {TYPE_ELECTRIC, TYPE_ICE};    // Frost
                if (formId == 4) return {TYPE_ELECTRIC, TYPE_FLYING}; // Fan
                if (formId == 5) return {TYPE_ELECTRIC, TYPE_GRASS};  // Mow
                break;

            // Wormadam
            case 413:
                if (formId == 1) return {TYPE_BUG, TYPE_GROUND}; // Sandy
                if (formId == 2) return {TYPE_BUG, TYPE_STEEL};  // Trash
                break;

            // Giratina
            case 487:
                // Both forms same type
                return {TYPE_GHOST, TYPE_DRAGON};

            // Shaymin
            case 492:
                if (formId == 1) return {TYPE_GRASS, TYPE_FLYING}; // Sky
                break;

            // Forces of Nature - Therian forms same type
            case 641: // Tornadus
            case 642: // Thundurus
            case 645: // Landorus
                // All same types in both forms
                break;

            // Kyurem
            case 646:
                if (formId == 1) return {TYPE_DRAGON, TYPE_ICE}; // White
                if (formId == 2) return {TYPE_DRAGON, TYPE_ICE}; // Black
                break;

            // Meloetta
            case 648:
                if (formId == 1) return {TYPE_NORMAL, TYPE_FIGHTING}; // Pirouette
                break;

            // Zygarde forms
            case 718:
                // All Zygarde forms same type
                return {TYPE_DRAGON, TYPE_GROUND};

            // Hoopa
            case 720:
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_DARK}; // Unbound
                break;

            // Oricorio
            case 741:
                if (formId == 0) return {TYPE_FIRE, TYPE_FLYING};     // Baile
                if (formId == 1) return {TYPE_ELECTRIC, TYPE_FLYING}; // Pom-Pom
                if (formId == 2) return {TYPE_PSYCHIC, TYPE_FLYING};  // Pa'u
                if (formId == 3) return {TYPE_GHOST, TYPE_FLYING};    // Sensu
                break;

            // Lycanroc - all forms same type
            case 745:
                return {TYPE_ROCK, TYPE_NONE};

            // Necrozma
            case 800:
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_STEEL};  // Dusk Mane
                if (formId == 2) return {TYPE_PSYCHIC, TYPE_GHOST};  // Dawn Wings
                if (formId == 3) return {TYPE_PSYCHIC, TYPE_DRAGON}; // Ultra
                break;

            // Urshifu
            case 892:
                if (formId == 0) return {TYPE_FIGHTING, TYPE_DARK};  // Single Strike
                if (formId == 1) return {TYPE_FIGHTING, TYPE_WATER}; // Rapid Strike
                break;

            // Calyrex
            case 898:
                if (formId == 1) return {TYPE_PSYCHIC, TYPE_ICE};   // Ice Rider
                if (formId == 2) return {TYPE_PSYCHIC, TYPE_GHOST}; // Shadow Rider
                break;

            // Ursaluna
            case 901:
                if (formId == 1) return {TYPE_GROUND, TYPE_NORMAL}; // Bloodmoon
                break;

            // Ogerpon
            case 1017:
                if (formId == 0) return {TYPE_GRASS, TYPE_NONE};        // Teal Mask
                if (formId == 1) return {TYPE_GRASS, TYPE_WATER};       // Wellspring
                if (formId == 2) return {TYPE_GRASS, TYPE_FIRE};        // Hearthflame
                if (formId == 3) return {TYPE_GRASS, TYPE_ROCK};        // Cornerstone
                break;

            // Terapagos
            case 1024:
                if (formId == 0) return {TYPE_NORMAL, TYPE_NONE};       // Normal
                if (formId == 1) return {TYPE_NORMAL, TYPE_NONE};       // Terastal
                if (formId == 2) return {TYPE_NORMAL, TYPE_NONE};       // Stellar
                break;
        }
    }

    // Return base form types
    return getBaseTypes(speciesId);
}
}
