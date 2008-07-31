#include "G3D/G3DAll.h"
// #include "G3D/platform.h"
// #include "G3D/Vector2.h"
// #include "G3D/Vector3.h"
// #include "G3D/Vector3int16.h"
// #include "G3D/Vector4.h"
// #include "G3D/Color1.h"
// #include "G3D/Color1uint8.h"
// #include "G3D/Color3.h"
// #include "G3D/Color3uint8.h"
// #include "G3D/Color4.h"
// #include "G3D/Color4uint8.h"
// #include "G3D/debugAssert.h"
// #include "./glheaders.h"
// #ifdef _DEBUG
// // Needed for debugAssertGLOk
// #   include "./getOpenGLState.h"
// #endif
using G3D::uint8;
using G3D::uint32;
using G3D::uint64;




static void printBoard(const Color3uint8* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (b[i] == Color3uint8(0, 0, 0))
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color4uint8* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (Color3uint8(b[i].r,b[i].g,b[i].b) == Color3uint8(0, 0, 0))
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color3* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (b[i] == Color3(0, 0, 0))
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

static void printBoard(const Color4* b, int S) {
    printf("\n");
    for (int i = 0; i < S * S; ++i) {
	if (Color3(b[i]) == Color3(0, 0, 0))
	    printf("o ");
	else
	    printf("x ");
	if ( (i+1) % 8 == 0)
	    printf("\n");
    }
    printf("\n");
}

#define RECONCAST reinterpret_cast<const void*>
#define RECAST reinterpret_cast<void*>



void testImageConvert() {

    printf("G3D::ImageFormat  ");

    // Set up the checkerboard

    const int S = 8;
    Color3   rgb32f[S * S];
    Color3  _rgb32f[S * S];
    const Color3 black(Color3::black());
    const Color3 white(Color3::white());

    for (int i = 0; i < S * S; ++i) {
	if ( isEven(i/8 + i)) {
	    rgb32f[i] = black;
	} else {
	    rgb32f[i] = white;
	}
    }


    Color3uint8   rgb8[S * S];
    Color3uint8  _rgb8[S * S];

    Color3uint8   bgr8[S * S];
    Color3uint8  _bgr8[S * S];

    Color4        rgba32f[S * S];;
    Color4       _rgba32f[S * S];

    // rgb32f <--> rgba32f
    ImageFormat::convert(RECONCAST(rgb32f), S, S, ImageFormat::RGB32F(), 0,
			 RECAST(rgba32f), ImageFormat::RGBA32F(), 0, false);
    {
	// rgba32f <--> rgb8
	ImageFormat::convert(RECONCAST(rgba32f), S, S, ImageFormat::RGBA32F(), 0,
			     RECAST(rgb8), ImageFormat::RGB8(), 0, false);
	{
	    // rgb8 <--> bgr8
	    ImageFormat::convert(RECONCAST(rgb8), S, S, ImageFormat::RGB8(), 0,
				 RECAST(bgr8), ImageFormat::BGR8(), 0, false);
	    ImageFormat::convert(RECONCAST(bgr8), S, S, ImageFormat::BGR8(), 0,
				 RECAST(_rgb8), ImageFormat::RGB8(), 0, false);
	}
	ImageFormat::convert(RECONCAST(rgb8), S, S, ImageFormat::RGB8(), 0,
			     RECAST(_rgba32f), ImageFormat::RGBA32F(), 0, false);
    }

    ImageFormat::convert(RECONCAST(_rgba32f), S, S, ImageFormat::RGBA32F(), 0,
			 RECAST(_rgb32f), ImageFormat::RGB32F(), 0, false);



    // Compare

    for (int i = 0; i < 64; ++i) {
	if (Color3(rgb32f[i]) != Color3(_rgb32f[i])) {
	    printf("No match at position i = %d \n", i);
	    printf("failed. \n");
	}
    }



    printf("passed\n");
}


void perfTest() {
    printf("ImageFormat::convert performance test is not implemented.");

}
