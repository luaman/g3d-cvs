/** 
  @file Material.cpp


  @created 2005-01-01
  @edited  2007-12-14
  @author  Morgan McGuire, morgan@cs.williams.edu
 */

#include "GLG3D/Material.h"

namespace G3D {

void Material::computeDefines(std::string& defines) {
    defines = "";

    if (diffuse.constant != Color3::black()) {
        if (diffuse.map.notNull()) {
            defines += "#define DIFFUSEMAP\n";

            // If the color is white, don't multiply by it
            if (diffuse.constant != Color3::white()) {
                defines += "#define DIFFUSECONSTANT\n";
            }
        } else {
            defines += "#define DIFFUSECONSTANT\n";
        }
    }

    if (customConstant.isFinite()) {
        defines += "#define CUSTOMCONSTANT\n";
    }

    if (customMap.notNull()) {
        defines += "#define CUSTOMMAP\n";
    }

    if (specular.constant != Color3::black()) {
        if (specular.map.notNull()) {
            defines += "#define SPECULARMAP\n";

            // If the color is white, don't multiply by it
            if (specular.constant != Color3::white()) {
                defines += "#define SPECULARCONSTANT\n";
            }
        } else  {
            defines += "#define SPECULARCONSTANT\n";
        }


        if (specularExponent.constant != Color3::black()) {
            if (specularExponent.map.notNull()) {
                defines += "#define SPECULAREXPONENTMAP\n";
                
                // If the color is white, don't multiply by it
                if (material.specularExponent.constant != Color3::white()) {
                    defines += "#define SPECULAREXPONENTCONSTANT\n";
                }
            } else  {
                defines += "#define SPECULAREXPONENTCONSTANT\n";
            }
        }
    }

    if (emit.constant != Color3::black()) {
        if (emit.map.notNull()) {
            defines += "#define EMITMAP\n";

            // If the color is white, don't multiply by it
            if (emit.constant != Color3::white()) {
                defines += "#define EMITCONSTANT\n";
            }
        } else  {
            defines += "#define EMITCONSTANT\n";
        }
    }

    if (reflect.constant != Color3::black()) {
        if (reflect.map.notNull()) {
            defines += "#define REFLECTMAP\n";

            // If the color is white, don't multiply by it
            if (reflect.constant != Color3::white()) {
                defines += "#define REFLECTCONSTANT\n";
            }
        } else  {
            defines += "#define REFLECTCONSTANT\n";

            if (reflect.constant == Color3::white()) {
                defines += "#define REFLECTWHITE\n";
            }
        }
    }

    if ((bumpMapScale != 0) && material.normalBumpMap.notNull()) {
        defines += "#define NORMALBUMPMAP\n";
    }
}


bool Material::similarTo(const Material& other) const {
    return
        diffuse.similarTo(other.diffuse) &&
        emit.similarTo(other.emit) &&
        specular.similarTo(other.specular) &&
        specularExponent.similarTo(other.specularExponent) &&
        transmit.similarTo(other.transmit) &&
        reflect.similarTo(other.reflect) &&
        (customMap.notNull() == other.customMap.notNull()) &&
        (customConstant.isFinite() == other.customConstant.isFinite()) &&
        (normalBumpMap.isNull() == other.normalBumpMap.isNull());
}


bool Material::Component::similarTo(const Component& other) const{
    // Black and white are only similar to themselves
    if (isBlack()) {
        return other.isBlack();
    } else if (other.isBlack()) {
        return false;
    }
    
    if (isWhite()) {
        return other.isWhite();
    } else if (other.isWhite()) {
        return false;
    }
    
    // Two components are similar if they both have/do not have texture
    // maps.
    return map.isNull() == other.map.isNull();
}


void Material::enforceDiffuseMask() {
    if (! changed) {
        return;
    }

    if (diffuse.map.notNull() && ! diffuse.map->opaque()) {
        // There is a mask.  Extract it.

        Texture::Ref mask = diffuse.map->alphaOnlyVersion();

        static const int numComponents = 5;
        Component* component[numComponents] = {&emit, &specular, &specularExponent, &transmit, &reflect};

        // Spread the mask to other channels that are not black
        for (int i = 0; i < numComponents; ++i) {
            if (! component[i]->isBlack()) {
                if (component[i]->map.isNull()) {
                    // Add a new map that is the mask
                    component[i]->map = mask;
                } else {
                    // TODO: merge instead of replacing!
                    component[i]->map = mask;
                }
            }
        }
    }

    changed = false;
}


}
