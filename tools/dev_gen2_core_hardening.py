from pathlib import Path

cpp = Path('src/Integration/Gen2/Gen2StagedEditor.cpp')
s = cpp.read_text()
inc = '#include "Integration/Gen2/Gen2PersonalData.h"\n#include "Pokemon/Experience.h"'
if inc in s:
    s = s.replace(inc, '#include "Integration/Gen2/Gen2PersonalData.h"\n#include "Names/SpeciesNames.h"\n#include "Pokemon/Experience.h"', 1)

start = s.index('bool encodeInternationalASCII(')
end = s.index('\nbool validHeldItem(', start)
encoder = r'''bool encodeInternationalASCII(std::string_view name, std::size_t maxChars, std::size_t fieldBytes,
                              std::vector<uint8_t>& encoded, std::string& error,
                              std::string_view fieldLabel) {
    error.clear();
    if (name.empty()) {
        error = std::string(fieldLabel) + " cannot be empty";
        return false;
    }
    encoded.assign(fieldBytes, 0x50);
    std::size_t input = 0;
    std::size_t output = 0;
    while (input < name.size()) {
        if (output >= maxChars) {
            error = std::string(fieldLabel) + " exceeds the Generation II international length limit";
            return false;
        }
        const unsigned char c = static_cast<unsigned char>(name[input]);
        uint8_t value = 0;
        std::size_t consumed = 1;
        if (c >= 'A' && c <= 'Z') value = static_cast<uint8_t>(0x80 + c - 'A');
        else if (c >= 'a' && c <= 'z') value = static_cast<uint8_t>(0xA0 + c - 'a');
        else if (c >= '0' && c <= '9') value = static_cast<uint8_t>(0xF6 + c - '0');
        else if (c < 0x80) {
            switch (c) {
                case ' ': value = 0x7F; break;
                case '(': value = 0x9A; break;
                case ')': value = 0x9B; break;
                case ':': value = 0x9C; break;
                case ';': value = 0x9D; break;
                case '-': value = 0xE3; break;
                case '?': value = 0xE6; break;
                case '!': value = 0xE7; break;
                case '&': value = 0xE9; break;
                case '\'': value = 0xE0; break;
                case '.': value = 0xF2; break;
                case '/': value = 0xF3; break;
                case ',': value = 0xF4; break;
                default:
                    error = std::string(fieldLabel) +
                        " contains a character not representable by the conservative Gen II encoder";
                    return false;
            }
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x99 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x82) {
            value = 0xEF;
            consumed = 3;
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x99 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x80) {
            value = 0xF5;
            consumed = 3;
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x80 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x99) {
            value = 0xE0;
            consumed = 3;
        } else {
            error = std::string(fieldLabel) +
                " contains a character not representable by the conservative Gen II encoder";
            return false;
        }
        encoded[output++] = value;
        input += consumed;
    }
    return true;
}
'''
s = s[:start] + encoder + s[end:]
old = '''    if (pokemon.nickname.empty()) {
        error = "new Pokemon requires a Generation II nickname/species label";
        return false;
    }
'''
s = s.replace(old, '', 1)
old = '    created.nickname = pokemon.nickname;\n'
if old in s:
    s = s.replace(old, '    created.nickname = pokemon.nickname.empty() ? std::string(Names::getSpeciesName(pokemon.species)) : pokemon.nickname;\n', 1)
cpp.write_text(s)

hdr = Path('include/Integration/Gen2/Gen2StagedEditor.h')
h = hdr.read_text().replace(
    '    std::string nickname; // Empty -> species label supplied by the caller/UI before staging.\n',
    '    std::string nickname; // Empty -> generation-correct species display name.\n')
hdr.write_text(h)

mk = Path('Makefile.host.editor')
m = mk.read_text()
needle = '\tsrc/Integration/Gen2/Gen2PersonalData.cpp \\\n\tsrc/Pokemon/Experience.cpp'
if needle in m:
    m = m.replace(needle, '\tsrc/Integration/Gen2/Gen2PersonalData.cpp \\\n\tsrc/Names/SpeciesNames.cpp \\\n\tsrc/Pokemon/Experience.cpp', 1)
mk.write_text(m)

test = Path('tests/test_gsc_pokemon_editor.cpp')
t = test.read_text()
t = t.replace('assert(changed->moves==std::array<uint8_t,4>{84,85,0,0});',
              'assert((changed->moves==std::array<uint8_t,4>{84,85,0,0}));')
t = t.replace('assert(added->statExperience==std::array<uint16_t,5>{0,0,0,0,0});',
              'assert((added->statExperience==std::array<uint16_t,5>{0,0,0,0,0}));')
marker = '''    assert(added->experience==Pokemon::getExpForLevel(5,createdPersonal->experienceGrowth));
    assert((added->statExperience==std::array<uint16_t,5>{0,0,0,0,0}));
'''
if marker in t and 'defaultNamed=created' not in t:
    extra = marker + '''
    BoxPokemonCreate defaultNamed=created;defaultNamed.species=25;defaultNamed.nickname.clear();
    size_t defaultSlot=99;assert(editor->stageAddBoxPokemon(5,defaultNamed,defaultSlot,error));
    auto defaultMon=editor->boxedPokemon(5,0,error);assert(defaultMon&&defaultMon->nickname=="Pikachu");
    BoxPokemonEdit genderGlyph;genderGlyph.nickname="NIDORAN\\xE2\\x99\\x80";
    assert(editor->stageBoxPokemonEdit(5,0,genderGlyph,error));
    auto glyphMon=editor->boxedPokemon(5,0,error);assert(glyphMon&&glyphMon->nickname=="NIDORAN\\xE2\\x99\\x80");
'''
    t = t.replace(marker, extra, 1)
test.write_text(t)
