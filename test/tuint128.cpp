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

        for(int j = 1; j < 10000; ++j) {        
            uint128 c(a.hi, a.lo);
            c *= uint128(0, j);
            b += a;
            debugAssert(b == c);
        }

        b = a;
		a *= uint128(0, 1);
		debugAssert(a == b);

        a += uint128(0, 0);
        debugAssert(a == b);

		a += a;
		b *= uint128(0, 2);
		debugAssert(a == b);

        a *= uint128(0, 0);
        debugAssert(a == uint128(0, 0));
	}
}