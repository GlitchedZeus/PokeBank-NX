#pragma once
// L6 Stage 6: standalone, audit-owned exact-head release evidence protocol.
// A synthetic test PASS is never an original retail-source attestation,
// global Pokémon legality decision, or authorization to merge.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace PokeBank::L6::Evidence {
enum class Lane : std::uint8_t { Encounter, Breeding, Moves, Events, QA, Codex, Count };
enum class Compiler : std::uint8_t { GCC, Clang, Count };
enum class Mode : std::uint8_t { Normal, ASanUBSan, Count };
enum class Outcome : std::uint8_t { Blocked, Malformed, NativeMatrixVerified };
inline constexpr std::size_t kLanes=static_cast<std::size_t>(Lane::Count);
inline constexpr std::size_t kCompilers=static_cast<std::size_t>(Compiler::Count);
inline constexpr std::size_t kModes=static_cast<std::size_t>(Mode::Count);
inline constexpr std::size_t kRequired=kLanes*kCompilers*kModes;
inline constexpr std::size_t kLimit=96;

struct Receipt {
    Lane lane=Lane::Count;
    Compiler compiler=Compiler::Count;
    Mode mode=Mode::Count;
    std::string_view head{};
    std::string_view tree{};
    std::string_view invocation{};
    bool began=false;
    bool completed=false;
    bool timedOut=false;
    bool dryRun=false;
    bool sourcePinsVerified=false;
    bool trackedCheckoutClean=false;
    int exitCode=-1;
    std::uint32_t requiredPrograms=0;
    std::uint32_t executedPrograms=0;
};
struct Result {
    Outcome outcome=Outcome::Blocked;
    std::uint16_t passed=0, failed=0, missing=0, duplicates=0, malformed=0;
    std::uint16_t stale=0, incomplete=0, missingSourcePins=0;
    // Source provenance and production acceptance are separate decisions.
    bool historicalOriginAuthenticated=false;
    bool wholePokemonLegal=false;
    bool wholePokemonInvalid=false;
    bool acceptanceAuthorized=false;
};
constexpr bool sha(std::string_view s) noexcept {
    if(s.size()!=40)return false;
    for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    return true;
}
constexpr bool invocation(std::string_view s) noexcept {
    // Prevent a receipt from claiming test execution without a command.
    if(s.empty()||s.size()>512)return false;
    for(char c:s)if(c=='\0'||c=='\r'||c=='\n')return false;
    return true;
}
constexpr std::size_t index(const Receipt& r) noexcept {
    return (static_cast<std::size_t>(r.lane)*kCompilers+
            static_cast<std::size_t>(r.compiler))*kModes+
            static_cast<std::size_t>(r.mode);
}
constexpr Result evaluate(std::string_view head,std::string_view tree,
                          std::span<const Receipt> entries) noexcept {
    Result result{};
    if(!sha(head)||!sha(tree)||entries.size()>kLimit) {
        result.outcome=Outcome::Malformed;result.malformed=1;return result;
    }
    std::array<bool,kRequired> seen{};
    for(const auto& r:entries) {
        if(static_cast<std::size_t>(r.lane)>=kLanes ||
           static_cast<std::size_t>(r.compiler)>=kCompilers ||
           static_cast<std::size_t>(r.mode)>=kModes ||
           !sha(r.head)||!sha(r.tree)||!invocation(r.invocation)||
           !r.requiredPrograms||r.executedPrograms>r.requiredPrograms) {
            ++result.malformed;continue;
        }
        const auto idx=index(r);
        if(seen[idx]) {++result.duplicates;continue;}
        seen[idx]=true;
        if(r.head!=head || r.tree!=tree) {++result.stale;continue;}
        if(!r.sourcePinsVerified) {++result.missingSourcePins;continue;}
        if(!r.began || !r.completed || r.dryRun || r.timedOut ||
           !r.trackedCheckoutClean || r.exitCode!=0) {
            ++result.failed;continue;
        }
        if(r.executedPrograms!=r.requiredPrograms) {
            ++result.incomplete;continue;
        }
        ++result.passed;
    }
    for(bool a:seen)if(!a)++result.missing;
    if(result.malformed||result.duplicates)result.outcome=Outcome::Malformed;
    else if(result.failed||result.missing||result.stale||
            result.incomplete||result.missingSourcePins)
        result.outcome=Outcome::Blocked;
    else result.outcome=Outcome::NativeMatrixVerified;
    return result;
}
static_assert(kRequired==24);
static_assert(sha("0123456789abcdef0123456789abcdef01234567"));
static_assert(!sha("0123456789ABCDEF0123456789ABCDEF01234567"));
} // namespace PokeBank::L6::Evidence
