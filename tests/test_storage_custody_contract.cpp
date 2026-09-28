#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    const std::string source = read("src/UI/TrainerViewScreenBase.inc");
    const auto begin = source.find("void TrainerViewScreen::returnHeldToOrigin()");
    const auto end = source.find(
        "std::unique_ptr<Pokemon::Pokemon>& TrainerViewScreen::storageSlot", begin);
    assert(begin != std::string::npos);
    assert(end != std::string::npos && end > begin);

    const std::string body = source.substr(begin, end - begin);

    // A02 regression: returning a carried block must be planned atomically.
    // The accepted pre-audit implementation moved what it could and then
    // unconditionally moveMon.clear()'d the rest, destroying any Pokemon
    // for which no return slot existed.
    assert(body.find("returnPlan.complete") != std::string::npos);
    assert(body.find("returnPlan.destinations") != std::string::npos);
    assert(body.find("Could not return the held Pokemon safely") != std::string::npos);

    const auto failureGuard = body.find("if (!returnPlan.complete)");
    const auto destructiveClear = body.find("moveMon.clear()");
    assert(failureGuard != std::string::npos);
    assert(destructiveClear != std::string::npos);
    assert(failureGuard < destructiveClear);

    std::cout << "Storage custody rollback contract: PASS\n";
}
