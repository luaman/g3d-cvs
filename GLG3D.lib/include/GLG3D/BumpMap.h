/**
 @file BumpMap.h
 @author Morgan McGuire, http://graphics.cs.williams.edu
 @created 2009-02-19
 @edited  2009-11-24
*/
#ifndef G3D_BumpMap_h
#define G3D_BumpMap_h

#include "G3D/platform.h"
#include "GLG3D/Component.h"

namespace G3D {

class Any;

/** 
  @brief Normal + bump map for use with G3D::Material.

  Supports Blinn normal mapping, Kaneko-Welsh parallax mapping,
  and Tatarchuk style parallax occlusion mapping.
 */
class BumpMap : public ReferenceCountedObject {
public:

    class Settings {
    public:
        /** World-space scale to apply to bump height for parallax/displacement mapping.*/
        float           scale;


        /** World-space offset from polygon surface to apply for parallax/displacement mapping.*/
        // Called "bias" instead of "offset" to avoid confusion with the computed parallax offset
        float           bias; 

        /**
          - 0  = Blinn normal map (default)
          - 1  = Kaneko-Welsh parallax map
          - >1 = Tatarchuk parallax occlusion map ("steep parallax map")
         */
        int            iterations;

        inline Settings() : scale(0.05f), bias(0.0f), iterations(0) {}
        Settings(const Any& any);
        operator Any() const;

        bool operator==(const Settings& s) const;
    };

protected:

    /**
       - rgb = tangent-space normal
       - a   = bump height
     */
    MapComponent<Image4>::Ref   m_normalBump;

    Settings                    m_settings;

    BumpMap(const MapComponent<Image4>::Ref& normalBump, const Settings& settings);

public:

    typedef ReferenceCountedPointer<BumpMap> Ref;

    /** @param normalBump Has tangent-space normals in rgb and bump elevation in a
        @param settings Settings 
        */
    static BumpMap::Ref create(const MapComponent<Image4>::Ref& normalBump, const Settings& settings);

    /** @param normalMapWhiteHeightInPixels When loading normal maps, argument used for G3D::GImage::computeNormalMap() whiteHeightInPixels. Default is -0.02f */
    static BumpMap::Ref fromHeightFile
    (const std::string& filename, 
     const Settings& settings = Settings(), 
     float normalMapWhiteHeightInPixels = -0.02f, 
     const Texture::Settings& textureSettings = Texture::Settings(), 
     const Texture::Dimension dim = Texture::defaultDimension());

    //static BumpMap::Ref fromNormalFile(const std::string& filename, );

    inline const Settings& settings() const {
        return m_settings;
    }

    /**
       Packed normal map and bump map.

       - rgb = tangent-space normal
       - a   = bump height
     */
    inline const MapComponent<Image4>::Ref& normalBumpMap() const {
        return m_normalBump;
    }

    inline void setStorage(ImageStorage s) const {
        m_normalBump->setStorage(s);
    }

    /** True if the same SuperShader can be used for both bump maps.*/
    bool similarTo(const BumpMap::Ref& other) const;
};

}

#endif
