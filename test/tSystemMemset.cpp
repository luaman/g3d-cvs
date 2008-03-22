#include "G3D/G3DAll.h"
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;

void testSystemMemset() {
    printf("System::memset");
    {
        static const int k = 100;
	    static uint8 a[k];
	    
	    int i;

	    for (i = 0; i < k; ++i) {
		    a[i] = i & 255;
	    }

	    System::memset(a, 4, k);

	    for (i = 0; i < k; ++i) {
		    debugAssert(a[i] == 4);
	    }
    }

    {
        // Test the internal debugAssertions
        for (int N = 100; N < 10000; N += 137) {

            void* x = System::malloc(N);
            System::memset(x, 0, N);
            x = System::realloc(x, N * 2);
            System::memset(x, 0, N*2);
            System::free(x);
        }
    }



    printf(" passed\n");
}
