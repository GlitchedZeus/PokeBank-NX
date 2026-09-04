#include "Utils/FileUtilities.h"
#include <cstdio>
#include <memory>
#include <new>

namespace Utils {
namespace { struct CloseFile { void operator()(FILE* file) const { fclose(file); } }; }
// All callers own a new[] allocation, including file-copy helpers and Bank.
uint8_t* readAllBytes(const char* path, size_t* outSize) {
    if (outSize) *outSize = 0;
    if (!path) return nullptr;
    std::unique_ptr<FILE, CloseFile> file(fopen(path, "rb"));
    if (!file || fseek(file.get(), 0, SEEK_END) != 0) return nullptr;
    const long length = ftell(file.get());
    if (length < 0 || fseek(file.get(), 0, SEEK_SET) != 0) return nullptr;
    const auto size = static_cast<size_t>(length);
    std::unique_ptr<uint8_t[]> bytes(new (std::nothrow) uint8_t[size ? size : 1]);
    if (!bytes || fread(bytes.get(), 1, size, file.get()) != size) return nullptr;
    if (outSize) *outSize = size;
    return bytes.release();
}
}
