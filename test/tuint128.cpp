#include "G3D/G3DAll.h"

void testuint128() {

	uint128 a(0,0);
	uint128 b(0,0);

    uint32 hiHi;
    uint32 loHi;
    uint32 hiLo;
    uint32 loLo;

	for (int i = 0; i < 1000; ++i) {
        hiHi = uniformRandom(0.0, 1.0) * 0xFFFFFFFF;
        loHi = uniformRandom(0.0, 1.0) * 0xFFFFFFFF;
        hiLo = uniformRandom(0.0, 1.0) * 0xFFFFFFFF;
        loLo = uniformRandom(0.0, 1.0) * 0xFFFFFFFF;

		a = uint128((uint64(hiHi) << 32) + loHi, (uint64(hiLo) << 32) + loLo);
		b = uint128(0, 0);

        // Test multiplication against equivalent addition
        for(int j = 1; j < 10000; ++j) {        
            uint128 c(a.hi, a.lo);
            c *= uint128(0, j);
            b += a;
            debugAssert(b == c);
        }

        // Test multiplication by 1
        b = a;
		a *= uint128(0, 1);
		debugAssert(a == b);

        // Test addition of 0
        a += uint128(0, 0);
        debugAssert(a == b);

        // Test left shift against equivalent addition
        uint128 c = a;
        c <<= 1;
		a += a;
        debugAssert(a == c);

        // Test right shift against unsigned division. C and B should be equal unless the top bit of b was a 1.
        if(!(b.hi & 0x8000000000000000)) {
            c >>= 1;
            debugAssert(c == b);
        }

        // Test multiplication by 2
		b *= uint128(0, 2);
		debugAssert(a == b);

        // Test multiplication by 0
        a *= uint128(0, 0);
        debugAssert(a == uint128(0, 0));
	}
}