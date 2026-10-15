#ifndef POKEBANK_TEST_GEN6_SUPPORT_H
#define POKEBANK_TEST_GEN6_SUPPORT_H

#include <cstdio>
#include <cstdlib>

namespace Gen6Test {
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
} // namespace Gen6Test

#define GEN6_CHECK(cond)                                                                \
    do {                                                                                \
        ++Gen6Test::checks;                                                             \
        if (!(cond)) {                                                                  \
            ++Gen6Test::failures;                                                       \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                               \
    } while (0)

#endif
