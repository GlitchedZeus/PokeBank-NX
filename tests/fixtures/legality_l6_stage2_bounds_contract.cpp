// L6 production-shared immutable bounds predicate contract: no retail proof.
#include "Pokemon/NativeRecordBounds.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>

namespace B = Pokemon::NativeRecordBounds;
using size_type = std::size_t;
constexpr auto MAX = std::numeric_limits<size_type>::max();
static_assert(B::contains(100, 0, 100));
static_assert(B::contains(100, 100, 0));
static_assert(!B::contains(100, 100, 1));
static_assert(!B::contains(100, MAX, 2));
static_assert(!B::contains(100, MAX - 1, 4));
static_assert(!B::contains(100, MAX - 3, 4));
static_assert(B::contains(MAX, MAX - 3, 3));
static_assert(!B::contains(MAX, MAX - 3, 4));
static_assert(B::contains(0, 0, 0));
static_assert(!B::contains(0, 0, 1));

int main() {
    std::uint64_t checks=0;
    for (size_type extent=0;extent<=260;++extent) {
        for (size_type offset=0;offset<=270;++offset) {
            for (size_type width=0;width<=8;++width) {
                const bool expected = offset<=extent && width<=extent-offset;
                assert(B::contains(extent,offset,width)==expected);
                ++checks;
            }
        }
        for (size_type offset : {MAX,MAX-1,MAX-2,MAX-3,MAX-7,MAX/2}) {
            for(size_type width : {0u,1u,2u,4u,8u}) {
                const bool expected=offset<=extent && width<=extent-offset;
                assert(B::contains(extent,offset,width)==expected);
                ++checks;
            }
        }
    }
    // A hypothetical reader/writer must not access a low byte via width-end overflow.
    std::array<unsigned char,100> data{};
    data[0]=0x5a;
    auto read16=[&](size_type o)->unsigned {
        if(!B::contains(data.size(),o,2))return 0;
        return data[o] | (static_cast<unsigned>(data[o+1])<<8);
    };
    auto write32=[&](size_type o,std::uint32_t v) {
        if(!B::contains(data.size(),o,4))return;
        for (unsigned i=0;i!=4;++i)data[o+i]=static_cast<unsigned char>(v>>(8*i));
    };
    for(size_type bad : std::array<size_type,7>{MAX,MAX-1,MAX-2,MAX-3,99u,100u,101u}) {
        assert(read16(bad)==0);
        const auto before=data;
        write32(bad,0xffffffffu);
        assert(data==before);
        checks+=2;
    }
    assert(read16(0)==0x5a);
    write32(96,0x11223344u);
    assert(data[96]==0x44 && data[97]==0x33 && data[98]==0x22 && data[99]==0x11);
    checks+=3;
    std::cout<<"L6 Stage 2 production bounds: "<<checks<<" PASS (overflow/canary/read-write)\n";
}
