# Standalone lane: deliberately not included by shared Makefile.host yet.
# make -f Makefile.host.gen5-create test
# make -f Makefile.host.gen5-create CXX=clang++ sanitize
CXX ?= g++
CREATE_BUILD ?= build-host/gen5-create
CREATE_FLAGS := -std=c++20 -Wall -Wextra -Wpedantic -Werror $(CPPFLAGS) -Iinclude
CREATE_HEADERS := $(wildcard include/Integration/Gen5/*.h) include/Encryption/Encryption5.h include/Pokemon/Pokemon5ReadOnly.h
.PHONY: test debug sanitize integration integration-sanitize
test: $(CREATE_BUILD)/optimized
	./$<
debug: $(CREATE_BUILD)/debug
	./$<
sanitize: $(CREATE_BUILD)/sanitize
	ASAN_OPTIONS=$${ASAN_OPTIONS:-detect_leaks=0} UBSAN_OPTIONS=halt_on_error=1 ./$<
integration: $(CREATE_BUILD)/integration
	./$<
integration-sanitize: $(CREATE_BUILD)/integration-sanitize
	ASAN_OPTIONS=$${ASAN_OPTIONS:-detect_leaks=0} UBSAN_OPTIONS=halt_on_error=1 ./$<
$(CREATE_BUILD):
	mkdir -p $@
$(CREATE_BUILD)/optimized: tests/test_gen5_native_create.cpp $(CREATE_HEADERS) | $(CREATE_BUILD)
	$(CXX) $(CREATE_FLAGS) -O2 $< -o $@
$(CREATE_BUILD)/debug: tests/test_gen5_native_create.cpp $(CREATE_HEADERS) | $(CREATE_BUILD)
	$(CXX) $(CREATE_FLAGS) -O0 -g3 -D_GLIBCXX_ASSERTIONS $< -o $@
$(CREATE_BUILD)/sanitize: tests/test_gen5_native_create.cpp $(CREATE_HEADERS) | $(CREATE_BUILD)
	$(CXX) $(CREATE_FLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@
$(CREATE_BUILD)/integration: tests/test_gen5_create_integration.cpp $(CREATE_HEADERS) | $(CREATE_BUILD)
	$(CXX) $(CREATE_FLAGS) -O2 $< -o $@
$(CREATE_BUILD)/integration-sanitize: tests/test_gen5_create_integration.cpp $(CREATE_HEADERS) | $(CREATE_BUILD)
	$(CXX) $(CREATE_FLAGS) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $< -o $@
