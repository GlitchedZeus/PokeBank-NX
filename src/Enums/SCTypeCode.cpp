#include "enums/SCTypeCode.h"

size_t getTypeSize(SCTypeCode type)
{
    switch(type)
    {
        case SCTypeCode::Bool3: return sizeof(bool);
        case SCTypeCode::Byte: return sizeof(uint8_t);
        case SCTypeCode::UInt16: return sizeof(uint16_t);
        case SCTypeCode::UInt32: return sizeof(uint32_t);
        case SCTypeCode::UInt64: return sizeof(uint64_t);
        case SCTypeCode::SByte: return sizeof(int8_t);
        case SCTypeCode::Int16: return sizeof(short);
        case SCTypeCode::Int32: return sizeof(int);
        case SCTypeCode::Int64: return sizeof(long);

        case SCTypeCode::Single: return sizeof(float);
        case SCTypeCode::Double: return sizeof(double);

        default: return 0;
    }
}