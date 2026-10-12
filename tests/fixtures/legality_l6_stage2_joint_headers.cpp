// L6 integration: exact production source headers for PK4 width+intake boundary.
#include "Pokemon/Gen4NativeRecordBoundary.h"
#include "Pokemon/NativeRecordBounds.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>
#include <limits>
int main() {
    using namespace Pokemon;
    std::uint64_t checks=0;
    for(std::size_t size=0;size<=512;++size) {
        std::vector<std::byte> source(size,std::byte{0x5a});
        const auto before=source;
        const auto copied=Gen4NativeRecordBoundary::copyExact(source);
        const bool valid=size==Encryption::SIZE_STORED4 || size==Encryption::SIZE_PARTY4;
        assert(copied.size()==(valid ? size : 0));
        if(valid)assert(copied==source);
        for(auto offset : {std::size_t{0},size,size+1,std::numeric_limits<std::size_t>::max()}) {
            for(std::size_t width : {0u,1u,2u,4u,8u}) {
                auto expected=offset<=copied.size() && width<=copied.size()-offset;
                assert(NativeRecordBounds::contains(copied.size(),offset,width)==expected);
                ++checks;
            }
        }
        assert(source==before);
        checks+=3;
    }
    std::vector<std::byte> oversized(2*1024*1024,std::byte{0xa5});
    assert(Gen4NativeRecordBoundary::copyExact(oversized).empty());
    ++checks;
    assert(checks==11800u);
    std::cout<<"L6 Stage 2 joint production headers: "<<checks<<" PASS (PK4 intake and field range)\n";
}
