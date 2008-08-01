#include "../../GLG3D.lib/include/GLG3D/glheaders.h"
#include "../../GLG3D.lib/include/GLG3D/glcalls.h"
#include "G3D/ImageFormat.h"
#include "G3D/Color1.h"
#include "G3D/Color1uint8.h"


namespace G3D {
static void rgb8_to_rgba32f(int w, int h, 
			    const uint8* src, 
			    size_t srcRowPadBytes, 
			    Color4* dst, 
			    const bool invertY) {
    int dstIndex = 0;
    size_t srcByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    dstIndex = w * (h - 1 - y);
	}
	for (int x = 0; x < w; ++x, ++dstIndex, srcByteOffset += 3) {
	    const Color3uint8& s = *reinterpret_cast<const Color3uint8*>(src + srcByteOffset);
	    dst[dstIndex] = Color4(Color3(s), 1.0f);
	}
	srcByteOffset += srcRowPadBytes;
    }
}
    

static void bgr8_to_rgba32f(int w, int h, 
			    const uint8* src, 
			    size_t srcRowPadBytes, 
			    Color4* dst, 
			    const bool invertY) {
    int dstIndex = 0;
    size_t srcByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    dstIndex = w * (h - 1 - y);
	}

	for (int x = 0; x < w; ++x, ++dstIndex, srcByteOffset += 3) {
	    const Color3uint8& s = *reinterpret_cast<const Color3uint8*>(src + srcByteOffset);
	    dst[dstIndex] = Color4(Color3(s).bgr(), 1.0f);
	}
	srcByteOffset += srcRowPadBytes;
    }
}


static void rgba8_to_rgba32f(int w, int h, 
			     const uint8* src, 
			     size_t srcRowPadBytes, 
			     Color4* dst, 
			     const bool invertY) { 
    int dstIndex = 0;
    size_t srcByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    dstIndex = w * (h - 1 - y);
	}

	for (int x = 0; x < w; ++x, ++dstIndex, srcByteOffset += 4) {
	    const Color4uint8& s = *reinterpret_cast<const Color4uint8*>(src + srcByteOffset);
	    dst[dstIndex] = Color4(s);
	}
	srcByteOffset += srcRowPadBytes;
    }
    debugPrintf("\n\n");
}



static void rgb32f_to_rgba32f(int w, int h, 
			      const Color3* _src, 
			      size_t srcRowPadBytes, 
			      Color4* dst, 
			      const bool invertY) {
    int dstIndex = 0;
    size_t srcByteOffset = 0;

    const uint8* src = reinterpret_cast<const uint8*>(_src);

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    dstIndex = w * (h - 1 - y);
	}

	for (int x = 0; x < w; ++x, ++dstIndex, srcByteOffset += 3 * sizeof(float)) {
	    const Color3& s = *reinterpret_cast<const Color3*>(src + srcByteOffset);
	    dst[dstIndex] = Color4(Color3(s), 1.0f);
	}
	srcByteOffset += srcRowPadBytes;
    }
}



static void rgba32f_to_rgb8(int w, int h, 
			    const Color4* src,
			    size_t dstRowPadBytes, 
			    uint8* dst,
			    const bool invertY) {
    int srcIndex = 0;
    size_t dstByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    srcIndex = w * (h - y - 1);
	}
        
	for (int x = 0; x < w; ++x, ++srcIndex, dstByteOffset += 3) {
	    Color3uint8&  d = *reinterpret_cast<Color3uint8*>(dst + dstByteOffset);
	    const Color4& s = src[srcIndex];

	    d = Color3uint8(s.rgb());
	}
	dstByteOffset += dstRowPadBytes;
    } 
    debugPrintf("\n\n\n");
}


static void rgba32f_to_bgr8(int w, int h, 
			    const Color4* src,
			    size_t dstRowPadBytes,
			    uint8* dst,
			    const bool invertY) {
    int srcIndex = 0;
    size_t dstByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    srcIndex = w * (h - y - 1);
	}
        
	for (int x = 0; x < w; ++x, ++srcIndex, dstByteOffset += 3) {
	    Color3uint8&  d = *reinterpret_cast<Color3uint8*>(dst + dstByteOffset);
	    const Color4& s = src[srcIndex];

	    d = Color3uint8(s.rgb()).bgr();
	}
	dstByteOffset += dstRowPadBytes;
    } 
}


static void rgba32f_to_rgba8(int w, int h, 
			     const Color4* src,
			     size_t dstRowPadBytes,
			     uint8* dst, 
			     const bool invertY) {
    int srcIndex = 0;
    size_t dstByteOffset = 0;

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    srcIndex = w * (h - 1 - y);
	}
	for (int x = 0; x < w; ++x, ++srcIndex, dstByteOffset += 4) {
	    Color4uint8&  d = *reinterpret_cast<Color4uint8*>(dst + dstByteOffset);
	    const Color4& s = src[srcIndex];

	    d = Color4uint8(s);
	}
	dstByteOffset += dstRowPadBytes;
    } 
}



static void rgba32f_to_rgb32f(int w, int h, 
			      const Color4* src,
			      size_t dstRowPadBytes,
			      Color3* _dst, 
			      const bool invertY) {
    int srcIndex = 0;
    size_t dstByteOffset = 0;
    // size_t srcRowBytes = sizeof(Color4) * w; //TODO: We can't do this.

    uint8* dst = reinterpret_cast<uint8*>(_dst);

    for (int y = 0; y < h; ++y) {
	if (invertY) {
	    srcIndex = w * (h - 1 - y);
	}
	for (int x = 0; x < w; ++x, ++srcIndex, dstByteOffset += 3 * sizeof(float)) {
	    Color3& d = *reinterpret_cast<Color3*>(dst + dstByteOffset);
	    const Color4& s = src[srcIndex];	    
	    d = Color3(s);
	}
	dstByteOffset += dstRowPadBytes;
    } 
    debugPrintf("\n\n\n");
}






// =====================================================================
// rgb8 <--> rgba8
// =====================================================================

void rgb8_to_rgba8(const int w, const int h, 
		   const uint8* src, uint8* dst, const bool invertY) {

    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    int i4 = i3 + i; 

	    dst[i4 + 0] = src[i3 + 0]; 
	    dst[i4 + 1] = src[i3 + 1]; 
	    dst[i4 + 2] = src[i3 + 2]; 
	    dst[i4 + 3] = 255; 
	}
    }
}

void bgr8_to_rgba8(const int w, const int h, 
		   const uint8* src, uint8* dst, const bool invertY) {

    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    int i4 = i3 + i; 

	    dst[i4 + 0] = src[i3 + 2]; 
	    dst[i4 + 1] = src[i3 + 1]; 
	    dst[i4 + 2] = src[i3 + 0]; 
	    dst[i4 + 3] = 255; 
	}
    }
}


static void rgba8_to_rgb8(const int w, const int h, 
			  const uint8* src, uint8* dst, const bool invertY) {

    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    int i4 = i3 + i;

	    dst[i3 + 0] = src[i4 + 0]; 
	    dst[i3 + 1] = src[i4 + 1]; 
	    dst[i3 + 2] = src[i4 + 2]; 
	}
    }
}

static void rgba8_to_bgr8(const int w, const int h, 
			  const uint8* src, uint8* dst, const bool invertY) {

    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    int i4 = i3 + i;

	    dst[i3 + 0] = src[i4 + 2]; 
	    dst[i3 + 1] = src[i4 + 1]; 
	    dst[i3 + 2] = src[i4 + 0]; 
	}
    }
}


// =====================================================================
// rgb8 <--> bgr8
// =====================================================================
static void rgb8_to_bgr8(const int w, const int h, 
			 const uint8* src, uint8* dst, const bool invertY) {

    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    dst[i3 + 0] = src[i3 + 2];
	    dst[i3 + 1] = src[i3 + 1];
	    dst[i3 + 2] = src[i3 + 0];
	}
    }
}


static void bgr8_to_rgb8(const int w, const int h, 
			 const uint8* src, uint8* dst, const bool invertY) {
    for (int y = 0; y < h; ++y) {
	for (int x = 0; x < w; ++x) {
	    int i = (invertY) ? ((h-1-y) * w +x) : (y * w + x);
	    int i3 = i * 3;
	    dst[i3 + 0] = src[i3 + 2];
	    dst[i3 + 1] = src[i3 + 1];
	    dst[i3 + 2] = src[i3 + 0];
	}
    }
}








////////////////////////////////////////////////////////////////////////////////////////////////
//
// Bayer conversions
//

// There are two kinds of rows (GR and BG).
// In each row, there are two kinds of pixels (G/R, B/G).
// We express the four kinds of INPUT pixels as:
//    GRG, GRG, BGB, BGG
//
// There are three kinds of OUTPUT pixels: R, G, B.
// Thus there are nominally 12 different I/O combinations, 
// but several are impulses because needed output at that 
// location *is* the input (e.g., G_GRG and G_BGG).
//
// The following 5x5 row-major filters are named as output_input.

// Green
static const float G_GRR[5][5] =
    {{     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f},
     {     0.0f,      0.0f,      2.0f,      0.0f,      0.0f},
     {    -1.0f,      2.0f,      4.0f,      2.0f,     -1.0f},
     {     0.0f,      0.0f,      2.0f,      0.0f,      0.0f},
     {     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f}};

static const float G_BGB[5][5] =
    {{     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f},
     {     0.0f,      0.0f,      2.0f,      0.0f,      0.0f},
     {    -1.0f,      2.0f,      4.0f,      2.0f,     -1.0f},
     {     0.0f,      0.0f,      2.0f,      0.0f,      0.0f},
     {     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f}};

// Red 
//(the caption in the paper is wrong for this case:
// "R row B column really means R row G column"
static const float R_GRG[5][5] =
    {{     0.0f,      0.0f,      0.5f,      0.0f,      0.0f},
     {     0.0f,     -1.0f,      0.0f,     -1.0f,      0.0f},
     {    -1.0f,      4.0f,      5.0f,      4.0f,     -1.0f},
     {     0.0f,     -1.0f,      0.0f,     -1.0f,      0.0f},
     {     0.0f,      0.0f,      0.5f,      0.0f,      0.0f}};

static const float R_BGG[5][5] =
    {{     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f},
     {     0.0f,     -1.0f,      4.0f,     -1.0f,      0.0f},
     {     0.5f,      0.0f,      5.0f,      0.0f,      0.5f},
     {     0.0f,     -1.0f,      4.0f,     -1.0f,      0.0f},
     {     0.0f,      0.0f,     -1.0f,      0.0f,      0.0f}};

static const float R_BGB[5][5] =
    {{     0.0f,      0.0f, -3.0f/2.0f,      0.0f,      0.0f},
     {     0.0f,      2.0f,      0.0f,      2.0f,      0.0f},
     {-3.0f/2.0f,      0.0f,      6.0f,      0.0f, -3.0f/2.0f},
     {     0.0f,      2.0f,      0.0f,      2.0f,      0.0f},
     {     0.0f,      0.0f, -3.0f/2.0f,      0.0f,      0.0f}};


// Blue 
//(the caption in the paper is wrong for this case:
// "B row R column really means B row G column")
#define B_BGG R_GRG
#define B_GRG R_BGG
#define B_GRR R_BGB














// =====================================================================
// Helper methods
// =====================================================================


/** Applies a 5x5 filter to monochrome image I (wrapping at the boundaries) */
static uint8 applyFilter(const uint8*    I,
			 int             x,
			 int             y,
			 int             w,
			 int             h,
			 const float     filter[5][5]) {
    
    debugAssert(isEven(w));
    debugAssert(isEven(h));
    
    float sum = 0.0f;
    float denom = 0.0f;
    
    for (int dy = 0; dy < 5; ++dy) {
	int offset = ((y + dy + h - 2) % h) * w;
    
	for (int dx = 0; dx < 5; ++dx) {
	    float f = filter[dy][dx];
	    sum += f * I[((x + dx + w - 2) % w) + offset];
	    denom += f;
	}
    }
    
    return (uint8)iClamp(iRound(sum / denom), 0, 255);
}

/** Helper method for Bayer grbg and bggr --> rgb8 */
static void swapRedAndBlue(int N, Color3uint8* out) {
    for (int i = N - 1; i >= 0; --i) {
	uint8 tmp = out[i].r;
	out[i].r = out[i].b;
	out[i].b = tmp;
    }
}

// =====================================================================
// bayer --> rgb8
// =====================================================================

static void bayer_rggb8_to_rgb8_mhc(int w, int h, 
				    const uint8* in, uint8* _out) {
    debugAssert(in != _out);

    Color3uint8* out = (Color3uint8*)_out;

    for (int y = 0; y < h; ++y) {

	// Row beginning in the input array.
	int offset = y * w;

	// RG row
	for (int x = 0; x < w; ++x, ++out) {
	    // R pixel
	    {
		out->r = in[x + offset];
		out->g = applyFilter(in, x, y, w, h, G_GRR);
		out->b = applyFilter(in, x, y, w, h, B_GRR);
	    }
	    ++x; ++out;

	    // G pixel
	    {
		out->r = applyFilter(in, x, y, w, h, R_GRG);
		out->g = in[x + offset];
		out->b = applyFilter(in, x, y, w, h, B_GRG);
	    }
	}

	++y;
	offset += w;

	// GB row
	for (int x = 0; x < w; ++x, ++out) {
	    // G pixel
	    {
		out->r = applyFilter(in, x, y, w, h, R_BGG);
		out->g = in[x + offset];
		out->b = applyFilter(in, x, y, w, h, B_BGG);
	    }
	    ++x; ++out;

	    // B pixel
	    {
		out->r = applyFilter(in, x, y, w, h, R_BGB);
		out->g = applyFilter(in, x, y, w, h, G_BGB);
		out->b = in[x + offset];
	    }
	}
    }
}



static void bayer_gbrg8_to_rgb8_mhc(int w, int h, 
				    const uint8* in, uint8* _out) {

    debugAssert(in != _out);

    Color3uint8* out = (Color3uint8*)_out;

    for (int y = 0; y < h; ++y) {

	// Row beginning in the input array.
	int offset = y * w;

	// GB row
	for (int x = 0; x < w; ++x, ++out) {
	    // G pixel
	    {
		out->r = applyFilter(in, x, y, w, h, R_BGG);
		out->g = in[x + offset];
		out->b = applyFilter(in, x, y, w, h, B_BGG);
	    }
	    ++x; ++out;

	    // B pixel
	    {
		out->r = applyFilter(in, x, y, w, h, R_BGB);
		out->g = applyFilter(in, x, y, w, h, G_BGB);
		out->b = in[x + offset];
	    }
	}
    }
}


static void bayer_grbg8_to_rgb8_mhc(int w, int h, 
				    const uint8* in, uint8* _out) {
    // Run the equivalent function for red
    bayer_gbrg8_to_rgb8_mhc(w, h, in, _out);

    // Now swap red and blue
    swapRedAndBlue(w * h, (Color3uint8*)_out);
}


static void bayer_bggr8_to_rgb8_mhc(int w, int h, 
				    const uint8* in, uint8* _out) {
    // Run the equivalent function for red
    bayer_rggb8_to_rgb8_mhc(w, h, in, _out);

    // Now swap red and blue
    swapRedAndBlue(w * h, (Color3uint8*)_out);
}


    // TODO: The following region is commented out because so far
    // those conversions are not used anywhere else. Until it is
    // decided that such conversions are not needed, this region
    // remains commented out.


// // =====================================================================
// // bayer --> bgr8 
// // =====================================================================

// static void bayer_rggb8_to_bgr8_mhc(int w, int h, 
// 				    const uint8* in, uint8* _out) {
//     debugAssert(in != _out);

//     Color3uint8* out = (Color3uint8*)_out;

//     for (int y = 0; y < h; ++y) {

// 	// Row beginning in the input array.
// 	int offset = y * w;

// 	// RG row
// 	for (int x = 0; x < w; ++x, ++out) {
// 	    // R pixel
// 	    {
// 		out->b = in[x + offset];
// 		out->g = applyFilter(in, x, y, w, h, G_GRR);
// 		out->r = applyFilter(in, x, y, w, h, B_GRR);
// 	    }
// 	    ++x; ++out;

// 	    // G pixel
// 	    {
// 		out->b = applyFilter(in, x, y, w, h, R_GRG);
// 		out->g = in[x + offset];
// 		out->r = applyFilter(in, x, y, w, h, B_GRG);
// 	    }
// 	}

// 	++y;
// 	offset += w;

// 	// GB row
// 	for (int x = 0; x < w; ++x, ++out) {
// 	    // G pixel
// 	    {
// 		out->b = applyFilter(in, x, y, w, h, R_BGG);
// 		out->g = in[x + offset];
// 		out->r = applyFilter(in, x, y, w, h, B_BGG);
// 	    }
// 	    ++x; ++out;

// 	    // B pixel
// 	    {
// 		out->b = applyFilter(in, x, y, w, h, R_BGB);
// 		out->g = applyFilter(in, x, y, w, h, G_BGB);
// 		out->r = in[x + offset];
// 	    }
// 	}
//     }
// }


// static void bayer_gbrg8_to_bgr8_mhc(int w, int h, 
// 				    const uint8* in, uint8* _out) {

//     debugAssert(in != _out);

//     Color3uint8* out = (Color3uint8*)_out;

//     for (int y = 0; y < h; ++y) {

// 	// Row beginning in the input array.
// 	int offset = y * w;

// 	// GB row
// 	for (int x = 0; x < w; ++x, ++out) {
// 	    // G pixel
// 	    {
// 		out->b = applyFilter(in, x, y, w, h, R_BGG);
// 		out->g = in[x + offset];
// 		out->r = applyFilter(in, x, y, w, h, B_BGG);
// 	    }
// 	    ++x; ++out;

// 	    // B pixel
// 	    {
// 		out->b = applyFilter(in, x, y, w, h, R_BGB);
// 		out->g = applyFilter(in, x, y, w, h, G_BGB);
// 		out->r = in[x + offset];
// 	    }
// 	}
//     }
// }

// static void bayer_grbg8_to_bgr8_mhc(int w, int h, 
// 				    const uint8* in, uint8* _out) {
//     // Run the equivalent function for red
//     bayer_gbrg8_to_bgr8_mhc(w, h, in, _out);

//     // Now swap red and blue
//     swapRedAndBlue(w * h, (Color3uint8*)_out);
// }

// static void bayer_bggr8_to_bgr8_mhc(int w, int h, 
// 				    const uint8* in, uint8* _out) {
//     // Run the equivalent function for red
//     bayer_rggb8_to_bgr8_mhc(w, h, in, _out);

//     // Now swap red and blue
//     swapRedAndBlue(w * h, (Color3uint8*)_out);
// }







// =====================================================================
// bayer (--> rgb8) --> rgba32f
// =====================================================================

static void bayer_rggb8_to_rgba32f_mhc(int w, int h, 
				       const uint8* in, 
				       Color4* _out, 
				       const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    bayer_rggb8_to_rgb8_mhc(w, h, in, tmp);
    rgb8_to_rgba32f(w, h, tmp, 0, _out, invertY);
}


static void bayer_grbg8_to_rgba32f_mhc(int w, int h, 
				       const uint8* in,
				       Color4* _out,
				       const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    bayer_grbg8_to_rgb8_mhc(w, h, in, tmp);
    rgb8_to_rgba32f(w, h, tmp, 0, _out, invertY);
}


static void bayer_bggr8_to_rgba32f_mhc(int w, int h, 
				       const uint8* in,
				       Color4* _out, 
				       const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];           
    bayer_bggr8_to_rgb8_mhc(w, h, in, tmp);
    rgb8_to_rgba32f(w, h, tmp, 0, _out, invertY);
}


static void bayer_gbrg8_to_rgba32f_mhc(int w, int h, 
				       const uint8* in,
				       Color4* _out,
				       const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    bayer_gbrg8_to_rgb8_mhc(w, h, in, tmp);
    rgb8_to_rgba32f(w, h, tmp, 0, _out, invertY);
}


// =====================================================================
// rgb8 --> bayer
// =====================================================================

static void rgb8_to_bayer_rggb8(const int w, const int h, 
				const uint8* src, uint8* dst) {
    Color3uint8* srcColor = (Color3uint8*)src;
    Color1uint8* dstColor = (Color1uint8*)dst;

    // Top row pixels
    for (int y = 0; y < h - 1; y += 2) {
	int offset = y * w;

	// Top left pixels
	for(int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].r);
	}

	// Top right pixels
	for(int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}
    }

    // Bottom row pixels
    for (int y = 1; y < h - 1; y += 2) {
	int offset = y * w;

	// Bottom left pixels
	for (int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}

	// Bottom right pixels
	for (int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].b);
	}
    }
}


static void rgb8_to_bayer_grbg8(const int w, const int h, 
				const uint8* src, uint8* dst) {
    Color3uint8* srcColor = (Color3uint8*)src;
    Color1uint8* dstColor = (Color1uint8*)dst;

    // Top row pixels
    for (int y = 0; y < h - 1; y += 2) {
	int offset = y * w;

	// Top left pixels
	for (int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}

	// Top right pixels
	for (int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].r);
	}
    }

    // Bottom row pixels
    for (int y = 1; y < h - 1; y += 2) {
	int offset = y * w;

	// Bottom left pixels
	for (int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].b);
	}

	// Bottom right pixels
	for (int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}
    }
}


static void rgb8_to_bayer_bggr8(const int w, const int h, 
				const uint8* src, uint8* dst) {
    Color3uint8* srcColor = (Color3uint8*)src;
    Color1uint8* dstColor = (Color1uint8*)dst;

    // Top row pixels
    for (int y = 0; y < h - 1; y += 2) {
	int offset = y * w;

	// Top left pixels
	for (int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].b);
	}

	// Top right pixels
	for (int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}
    }

    // Bottom row pixels
    for (int y = 1; y < h - 1; y += 2) {
	int offset = y * w;

	// Bottom left pixels
	for(int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}

	// Bottom right pixels
	for(int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].r);
	}
    }
}


static void rgb8_to_bayer_gbrg8(const int w, const int h, 
				const uint8* src, uint8* dst) {
    Color3uint8* srcColor = (Color3uint8*)src;
    Color1uint8* dstColor = (Color1uint8*)dst;

    // Top row pixels
    for(int y = 0; y < h - 1; y += 2) {
	int offset = y * w;

	// Top left pixels
	for(int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}

	// Top right pixels
	for(int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].b);
	}
    }

    // Bottom row pixels
    for(int y = 1; y < h - 1; y += 2) {
	int offset = y * w;

	// Bottom left pixels
	for(int x = 0; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].r);
	}

	// Bottom right pixels
	for(int x = 1; x < w - 1; x += 2) {
	    dstColor[x + offset] = Color1(srcColor[x + offset].g);
	}
    }
}


// =====================================================================
// rgba32f (-->rgb8) --> bayer
// =====================================================================


static void rgba32f_to_bayer_rggb8(int w, int h, const Color4* in, 
				   uint8* _out, const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    rgba32f_to_rgb8(w, h, in, 0, tmp, invertY);
    rgb8_to_bayer_rggb8(w, h, tmp, _out);
}


static void rgba32f_to_bayer_grbg8(int w, int h, const Color4* in, 
				   uint8* _out, const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    rgba32f_to_rgb8(w, h, in, 0, tmp, invertY);
    rgb8_to_bayer_grbg8(w, h, tmp, _out);
}


static void rgba32f_to_bayer_bggr8(int w, int h, const Color4* in, 
				   uint8* _out, const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];	
    rgba32f_to_rgb8(w, h, in, 0, tmp, invertY);
    rgb8_to_bayer_bggr8(w, h, tmp, _out);
}


static void rgba32f_to_bayer_gbrg8(int w, int h, const Color4* in, 
				   uint8* _out, const bool invertY) {
    uint8 tmp[w * h * sizeof(Color3uint8)];
    rgba32f_to_rgb8(w, h, in, 0, tmp, invertY);
    rgb8_to_bayer_gbrg8(w, h, tmp, _out);
}



///////////////////////////////////////////////////



static void convertToColor4(int w, int h, 
			    const void* src, 
			    const ImageFormat* srcFormat, 
			    size_t srcRowPadBits, 
			    Color4* dst, 
			    const bool invertY,
			    ImageFormat::BayerAlgorithm bayer) {

    size_t srcRowPadBytes = srcRowPadBits / 8;

    switch (srcFormat->code) {
    case ImageFormat::CODE_RGB8:
	alwaysAssertM(srcRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgb8_to_rgba32f(w, h, reinterpret_cast<const uint8*>(src), srcRowPadBytes, dst, invertY);
	break;

    case ImageFormat::CODE_RGBA8:
	alwaysAssertM(srcRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgba8_to_rgba32f(w, h, reinterpret_cast<const uint8*>(src), srcRowPadBytes, dst, invertY);
	break;

    case ImageFormat::CODE_BGR8:
	alwaysAssertM(srcRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	bgr8_to_rgba32f(w, h, reinterpret_cast<const uint8*>(src), srcRowPadBytes, dst, invertY);
	break;

    case ImageFormat::CODE_RGB32F:
	alwaysAssertM(srcRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgb32f_to_rgba32f(w, h, reinterpret_cast<const Color3*>(src), srcRowPadBytes, dst, invertY);
	break;

    case ImageFormat::CODE_BAYER_RGGB8:
	bayer_rggb8_to_rgba32f_mhc(w, h, reinterpret_cast<const uint8*>(src), dst, invertY);
	break;
    case ImageFormat::CODE_BAYER_GBRG8:
	bayer_gbrg8_to_rgba32f_mhc(w, h, reinterpret_cast<const uint8*>(src), dst, invertY);
	break;
    case ImageFormat::CODE_BAYER_GRBG8:
	bayer_grbg8_to_rgba32f_mhc(w, h, reinterpret_cast<const uint8*>(src), dst, invertY);
	break;
    case ImageFormat::CODE_BAYER_BGGR8:
	bayer_bggr8_to_rgba32f_mhc(w, h, reinterpret_cast<const uint8*>(src), dst, invertY);
	break;

	// TODO: ADD MORE

    default:
	alwaysAssertM(false, std::string("Unsupported format: ") + srcFormat->name());
    }
}


static void convertFromColor4(int w, int h, 
			      const Color4* src, 
			      const ImageFormat* dstFormat, 
			      size_t dstRowPadBits, 
			      void* dst,
			      const bool invertY,
			      ImageFormat::BayerAlgorithm bayer) {

    size_t dstRowPadBytes = dstRowPadBits / 8;

    switch(dstFormat->code) {
    case ImageFormat::CODE_RGB8:
	alwaysAssertM(dstRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgba32f_to_rgb8(w, h, src, dstRowPadBytes, reinterpret_cast<uint8*>(dst), invertY);
	break;

    case ImageFormat::CODE_RGBA8:
	alwaysAssertM(dstRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgba32f_to_rgba8(w, h, src, dstRowPadBytes, reinterpret_cast<uint8*>(dst), invertY);
	break;

    case ImageFormat::CODE_BGR8:
	alwaysAssertM(dstRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgba32f_to_bgr8(w, h, src, dstRowPadBytes, reinterpret_cast<uint8*>(dst), invertY);
	break;

    case ImageFormat::CODE_RGB32F:
	alwaysAssertM(dstRowPadBits % 8 == 0, 
		      "Row padding must be a multiple of 8 bits for this format");
	rgba32f_to_rgb32f(w, h, src, dstRowPadBytes, reinterpret_cast<Color3*>(dst), invertY);
	break;



    case ImageFormat::CODE_BAYER_RGGB8:
	rgba32f_to_bayer_rggb8(w, h, src, reinterpret_cast<uint8*>(dst), invertY);
	break;
    case ImageFormat::CODE_BAYER_GBRG8:
	rgba32f_to_bayer_gbrg8(w, h, src, reinterpret_cast<uint8*>(dst), invertY);
	break;
    case ImageFormat::CODE_BAYER_GRBG8:
	rgba32f_to_bayer_grbg8(w, h, src, reinterpret_cast<uint8*>(dst), invertY);
	break;
    case ImageFormat::CODE_BAYER_BGGR8:
	rgba32f_to_bayer_bggr8(w, h, src, reinterpret_cast<uint8*>(dst), invertY);
	break;
            

	//... TODO: add more

    default:
	alwaysAssertM(false, std::string("Unsupported format: ") + dstFormat->name());
    }
}


void ImageFormat::convert(const void* src, 
			  int w, int h, 
			  const ImageFormat* srcFormat,
			  size_t srcRowPadBits,
			  void* dst, 
			  const ImageFormat* dstFormat,
			  size_t dstRowPadBits,
			  const bool invertY,
			  ImageFormat::BayerAlgorithm conversionAlg) {

    // Special optimized cases

    if ((srcFormat->code == dstFormat->code) && 
	(srcRowPadBits == dstRowPadBits) && 
	(srcFormat->packedBitsPerTexel > 0)) {
	// Same format, so just memcpy        
	System::memcpy(dst, src, iCeil(((w * srcFormat->packedBitsPerTexel + srcRowPadBits) * h) / 8.0f));
	return;
    }

    if ((srcFormat->code == ImageFormat::CODE_RGBA32F) && 
	(srcRowPadBits == 0)) {
	// rgba32f is the intermediate format. But if the source
	// format is already rgba32f, we do not need to go through
	// any intermediate conversion; we can convert it directly
	// into the destination format.
	convertFromColor4(w, h, reinterpret_cast<const Color4*>(src), 
			  dstFormat, dstRowPadBits, 
			  reinterpret_cast<uint8*>(dst), 
			  invertY, conversionAlg);
	return;
    }

    if ((dstFormat->code == ImageFormat::CODE_RGBA32F) && (dstRowPadBits == 0)) {
	// rgba32f is the intermediate format. But if the
	// destination format is already rgba32f, we can simply
	// stop after intermediate conversion.
	convertToColor4(w, h, reinterpret_cast<const uint8*>(src), 
			srcFormat, srcRowPadBits, 
			reinterpret_cast<Color4*>(dst), 
			invertY, conversionAlg);
	return;
    }

    if ((srcRowPadBits == 0) && (dstRowPadBits == 0)) {
	// See if there is a specially optimized conversion

	switch (srcFormat->code) {
	case ImageFormat::CODE_RGB8:
	    switch (dstFormat->code) {
	    case ImageFormat::CODE_BGR8:
		rgb8_to_bgr8(w, h, reinterpret_cast<const uint8*>(src), 
			     reinterpret_cast<uint8*>(dst), invertY);
		return;

	    case ImageFormat::CODE_RGBA8:
		rgb8_to_rgba8(w, h, reinterpret_cast<const uint8*>(src), 
			      reinterpret_cast<uint8*>(dst), invertY);
		return;
	    default:;
	    }
	    break;


	case ImageFormat::CODE_BGR8:
	    switch (dstFormat->code) {
	    case ImageFormat::CODE_RGB8:
		bgr8_to_rgb8(w, h, reinterpret_cast<const uint8*>(src), 
			     reinterpret_cast<uint8*>(dst), invertY);
		return;

	    case ImageFormat::CODE_RGBA8:
		bgr8_to_rgba8(w, h, reinterpret_cast<const uint8*>(src), 
			      reinterpret_cast<uint8*>(dst), invertY);
		return;
	    default:;
	    }
	    break;

	case ImageFormat::CODE_RGBA8:
	    switch (dstFormat->code) {
	    case ImageFormat::CODE_RGB8:
		rgba8_to_rgb8(w, h, reinterpret_cast<const uint8*>(src), 
			      reinterpret_cast<uint8*>(dst), invertY);
		return;
	    case ImageFormat::CODE_BGR8:
		rgba8_to_bgr8(w, h, reinterpret_cast<const uint8*>(src), 
			      reinterpret_cast<uint8*>(dst), invertY);
		return;
	    default:;
	    }
	    break;


	default:;

	}
    }

    // General case
    Color4* tmp = (Color4*)System::malloc(w * h * sizeof(Color4));

    convertToColor4(w, h, reinterpret_cast<const uint8*>(src), 
		    srcFormat, srcRowPadBits, tmp, 
		    invertY, conversionAlg);

    convertFromColor4(w, h, tmp, dstFormat, dstRowPadBits, 
		      reinterpret_cast<uint8*>(dst), 
		      invertY, conversionAlg);

    System::free(tmp);
    tmp = NULL;
}


}
