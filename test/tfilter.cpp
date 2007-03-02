#include "G3D/G3DAll.h"

void testfilter() {
    printf("G3D::gaussian1D  ");
    Array<float> coeff;

    gaussian1D(coeff, 5, 0.5f);
    debugAssert(fuzzyEq(coeff[0], 0.00026386508));
    debugAssert(fuzzyEq(coeff[1], 0.10645077));
    debugAssert(fuzzyEq(coeff[2], 0.78657067));
    debugAssert(fuzzyEq(coeff[3], 0.106645077));
    debugAssert(fuzzyEq(coeff[4], 0.00026386508));

    printf("passed\n");
}
