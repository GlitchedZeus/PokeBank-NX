#ifndef POKEBANK_TEST_GEN8_SUPPORT_H
#define POKEBANK_TEST_GEN8_SUPPORT_H

#include <cstdio>
#include <cstdlib>

namespace Gen8Test {
inline int failures = 0;
inline int checks = 0;
inline int finish(const char* name) {
    if (failures) {
        std::fprintf(stderr, "%s: %d of %d checks FAILED\n", name, failures, checks);
        return EXIT_FAILURE;
    }
    std::printf("%s: all %d checks passed\n", name, checks);
    return EXIT_SUCCESS;
}
} // namespace Gen8Test

#define GEN8_CHECK(cond)                                                                \
    do {                                                                                \
        ++Gen8Test::checks;                                                             \
        if (!(cond)) {                                                                  \
            ++Gen8Test::failures;                                                       \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                               \
    } while (0)

#endif
