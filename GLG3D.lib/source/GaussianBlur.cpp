#include "GLG3D/GaussianBlur.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/filter.h"
#include "G3D/Rect2D.h"
#include "GLG3D/Draw.h"

namespace G3D {

Table<int, ShaderRef> GaussianBlur::shaderCache;

void GaussianBlur::apply(RenderDevice* rd, const TextureRef& source, const Vector2& direction, int N) {
    debugAssert(isOdd(N));

    ShaderRef gaussian1DShader = getShader(17);

    gaussian1DShader->args.set("source", source);
    gaussian1DShader->args.set("pixelStep", direction / source->vector2Bounds());
    rd->setShader(gaussian1DShader);
    Draw::rect2D(source->rect2DBounds(), rd);
}


ShaderRef GaussianBlur::getShader(int N) {
    if (! shaderCache.containsKey(N)) {
        if (shaderCache.size() >= MAX_CACHE_SIZE) {
            // Remove a random element to keep the cache
            // size limited.
            Array<int> key;
            shaderCache.getKeys(key);
            shaderCache.remove(key.randomElement());
        }

        shaderCache.set(N, makeShader(N));
    }

    return shaderCache[N];
}

ShaderRef GaussianBlur::makeShader(int N) {

    // Make a string of the coefficients to insert into the shader
    Array<float> coeff;
    float stddev = N / 2.0f;
    gaussian1D(coeff, N, stddev);

    std::string gaussCoef = "{";
    for (int i = 0; i < N; ++i) {
        gaussCoef += format("%10.8f", coeff[i]);
        if (i < N - 1) {
            gaussCoef += ", ";
        }
    }

    gaussCoef += "};\n";

    // Note: ATI drivers don't let us declare the float array as "const"

    return Shader::fromStrings("",  
        format("const int kernelSize = %d;\n"
               "float gaussCoef[kernelSize] = %s\n", N, gaussCoef.c_str()) +
        STR(
            uniform sampler2D source;

            // vec2(dx, dy) / (source.width, source.height)
            uniform vec2      pixelStep;
            
            void main() {
                
                vec2 pixel = gl_TexCoord[0].xy;         
                vec4 sum = texture2D(source, pixel) * gaussCoef[0];
                
                for (int tap = 1; tap < kernelSize; ++tap) {
                    sum += texture2D(source, pixelStep * (float(tap) - float(kernelSize - 1) * 0.5) + pixel) * gaussCoef[tap];
                }
                
                gl_FragColor = sum;
            }));
}
    
}
