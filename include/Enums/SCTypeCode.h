#ifndef ENUMS_H
#define ENUMS_H

#include <cstdint>

#include <switch.h>

enum class SCTypeCode
{
    None = 0,

    Bool1 = 1, // False?
    Bool2 = 2, // True?
    Bool3 = 3, // Either? (Array boolean type)

    Object = 4,

    Array = 5,

    Byte = 8,
    UInt16 = 9,
    UInt32 = 10,
    UInt64 = 11,
    SByte = 12,
    Int16 = 13,
    Int32 = 14,
    Int64 = 15,
    Single = 16,
    Double = 17,
};

// Union representing all possible scalar values (like a manual variant)
union SCValueUnion {
    u8    u8_val;
    u16   u16_val;
    u32   u32_val;
    u64   u64_val;
    s8    s8_val;
    s16   s16_val;
    s32   s32_val;
    s64   s64_val;
    float float_val;
    double double_val;
};

/// Gets the number of bytes occupied by a variable of a given type.
/// Type of the value
size_t getTypeSize(SCTypeCode type);

#endif