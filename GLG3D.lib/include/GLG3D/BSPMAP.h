/**
  @file BSPMAP.h
  
  @maintainer Morgan McGuire, http://graphics.cs.williams.edu

  @created 2003-05-22
  @edited  2008-10-07

  @cite http://graphics.stanford.edu/~kekoa/q3/
  @cite http://www.gametutorials.com/Tutorials/OpenGL/Quake3Format.htm
  @cite http://www.nathanostgard.com/tutorials/quake3/collision/
  @cite Kris Taeleman (kris.taeleman@pandora.be)
  @cite http://www.flipcode.com/tutorials/tut_q2levels.shtml
 */

#ifndef G3D_BSPMAP_H
#define G3D_BSPMAP_H

#include "G3D/platform.h"
#include "G3D/Array.h"
#include "G3D/Vector3.h"
#include "G3D/Color4uint8.h"
#include "G3D/AABox.h"
#include "G3D/GCamera.h"
#include "GLG3D/Texture.h"
#include "G3D/Vector3int32.h"
#include <stdlib.h>
#include <memory.h>
#include <math.h>

namespace G3D {

// forward declare heavily dependent classes
class RenderDevice;


namespace _BSPMAP {

/**
 Quake uses a coordinate system that is scaled differently from ours.
 Multiplying by this constant scales down to the G3D system. 
 */
//static const double LOAD_SCALE = 0.03;

/**
 A tightly packed bool array.  Used by Map for tracking
 which faces have already been drawn during rendering.
 */
class BitSet {
private:

    /** Size of the bits array */
    G3D::uint32          size;
    G3D::uint32*         bits;

public:

    BitSet();

    ~BitSet();

    void resize(int count);

    /**
     Enable the ith bit.
     */
    inline void set(int i) {
        debugAssert(i >= 0);
        debugAssert((int)(i >> 5) < (int)size);
        // Note: i >> 5 == i / 32, i & 31 == i % 32
        bits[i >> 5] |= (1 << (i & 31));
    }

    inline int isOn(int i) const {
        debugAssert(i >= 0);
        debugAssert((i >> 5) < (int)size);

        return bits[i >> 5] & (1 << (i & 31));
    }

    inline void clear(int i) {
        debugAssert(i >= 0);
        debugAssert((i >> 5) < (int)size);

        bits[i >> 5] &= ~(1 << (i & 31));
    }

    void clearAll()	{
        System::memset(bits, 0, sizeof(G3D::uint32) * size);
    }
};

//
// BSP structs
//


class Vertex {
public:

    Vector3             position;
    Vector2             textureCoord;
    Vector2             lightmapCoord;
    Vector3             normal;
    Color4uint8         color;

    /**
     Used for bezier patch tesselation
     */
    Vertex operator+(const Vertex& v) const {
        Vertex res;

        res.position      = position        + v.position;
        res.textureCoord  = textureCoord    + v.textureCoord;
        res.lightmapCoord = lightmapCoord   + v.lightmapCoord;
        res.normal        = normal          + v.normal;
        
        return res;
    }

    /**
     Used for bezier patch tesselation
     */
    Vertex operator*(float factor) const	{
        Vertex res;

        res.position      = position        * factor;
        res.textureCoord  = textureCoord    * factor;
        res.lightmapCoord = lightmapCoord   * factor;
        res.normal        = normal          * factor;

        return res;
    }
};


class Brush {
public:
    int                 firstBrushSide;
    int                 brushSidesCount;
    int                 textureID;
};


class BrushSide {
public:
    int                 plane;

    /**
     The textureID is useful for determining the material on
     which a character is walking.  It is not used for rendering.
     */
    int                 textureID;
};


class BSPNode {
public:
    /**
     Index into the plane array.
     */
    int                 plane;
    
    /**
     Front child node. 
     Positive numbers are indices into the nodeArray,
     negative numbers are leaf indices: -(leaf+1) or, equivalently, ~leaf, that 
     index into leafArray.
    */
    int                 front;

    /**
     Back child node.
     Positive numbers are indices into the nodeArray,
     negative numbers are leaf indices: -(leaf+1) or, equivalently, ~leaf, that 
     index into leafArray.
    */
    int                 back;
};


/**
  The leafs lump stores the leaves of the map's BSP tree. Each leaf is a convex region
  that contains, among other things, a cluster index (for determining the other leaves's
  potentially visible from within the leaf), a list of faceArray (for rendering), and a list
  of brushes (for collision detection). 
 */
class BSPLeaf {
public:
    int                 cluster;
    int                 area;

    /**
     Bounding box on the leaf itself.  The faces referenced by a leaf
     may extend well beyond this box.
     */
    AABox               bounds;

    /** Redundant with bounds, but faster to read than compute during culling.*/
    Vector3             center;

    /**
     Index into BSP::faceArray of the first face in this leaf.
     The same faces may appear in multiple leaves.
     */
    int                 firstFace;

    /**
     Number of faces in this leaf.
     */
    int                 facesCount;

    int                 firstBrush;
    int                 brushesCount;
};


class BSPPlane {
public:
    Vector3             normal;
    float               distance;
};


class BSPModel {
public:
    Vector3             min;
    Vector3             max;
    int                 faceIndex;
    int                 numOfFaces;
    int                 brushIndex;
    int                 numOfBrushes;
};

/** e.g., a platform, a trigger */
class BSPEntity {
public:
    Vector3             position;
    std::string         name;
    int	                spawnflags;
    std::string         targetName;
    std::string         target;
    
    /** Index into dynamicModels array */
    int                 modelNum;
    std::string         otherInfo;
};


////////////////////////////////////////////////////////////////////////
G3D_BEGIN_PACKED_CLASS(1)
class LightVolume {
public:
    /** Ambient color component. RGB.  */
    Color3uint8              ambient;

    /** Directional color component. RGB. */
    Color3uint8              directional;

    /** Direction to light. 0=phi, 1=theta. in the Q3 coordinate
        system.*/
    G3D::uint8               direction[2];

#if 0
    /** Returns the direction to the light in the G3D coordinate system.*/
    Vector3 lightDirection() const {
        float theta = direction[1] * G3D::pi() / 255.0f;
        float phi = direction[0] * G3D::pi() / 255.0f;
        return Vector3(cos(),);
    }
#endif
}
G3D_END_PACKED_CLASS(1)

////////////////////////////////////////////////////////////////////////


class VisData {
public:
    int                     clustersCount;
    int                     bytesPerCluster;
    G3D::uint8*             bitsets;
};


class BSPCollision {
public:
    float               fraction;
    Vector3             start;
    Vector3             end;
    Vector3             size;
    Vector3             normal;
    bool                isSolid;
};


/**
 Abstract base class for Mesh, Patch, and Billboard.
 */
class FaceSet {
public:
    enum Type {POLYGON = 1, PATCH = 2, MESH = 3, BILLBOARD = 4};

    int                 textureID;
    int                 lightmapID;

    /**
     Depth value used as a sort key.
     */
    float               sortKey;

public:

    virtual ~FaceSet() {}
    virtual void render(class Map* map) const = 0;
    virtual bool isMesh() const = 0;

    virtual FaceSet::Type type() const = 0;

    /** Updates the sort key */
    virtual void updateSortKey(
        class Map*      map,
        const Vector3&  zAxis,
        Vector3&        origin) = 0;
};


class Mesh : public FaceSet {
public:

    int                 firstVertex;
    int                 vertexesCount;

    int                 firstMeshVertex;
    int                 meshVertexesCount;

public:

    virtual bool isMesh() const {
        return true;
    }

    virtual FaceSet::Type type() const {
        return MESH;
    }

    virtual void updateSortKey(
        class Map*      map,
        const Vector3&  zAxis,
        Vector3&        origin);

    virtual void render(class Map* map) const;
};


class Patch : public FaceSet {
public:

    class Bezier2D {
    public:

        /**
         Number of edges each side is split into.
         The total number of trangles will be 2 * level^2.
         */
        int                      level;
        Array<Vertex>            vertex;
        Array<G3D::uint32>       indexes;

        Array<G3D::int32>        trianglesPerRow;

        /**
         Pointers into the indexes array.
         */
        Array<G3D::uint32*>      rowIndexes;

    public:

        /**
         The bezier control points.
         */
        Vertex              controls[9];

        void tessellate(int _level);
        void render() const;
    };

    Array<Bezier2D>         bezierArray;

public:

    virtual bool isMesh() const {
        return false;
    }

    virtual FaceSet::Type type() const {
        return PATCH;
    }

    virtual void updateSortKey(
        class Map*          map,
        const Vector3&      zAxis,
        Vector3&            origin);

    virtual void render(class Map* map) const;
};


class Billboard : public FaceSet {
public:

    virtual FaceSet::Type type() const {
        return BILLBOARD;
    }

    bool isMesh() const {
        return false;
    }

    virtual void render(class Map* map) const {
        (void)map;
    }

    virtual void updateSortKey(
        class Map*      map,
        const Vector3&  zAxis,
        Vector3&        origin) {
        (void)origin;
        (void)map;
        (void)zAxis;
    }
};


typedef ReferenceCountedPointer<class Map> MapRef;

/**
 @brief A BSP Map loaded from Quake 3.
 */
class Map : public ReferenceCountedObject {
private:

    
    enum MapFileFormat {
        /** Quake 3 Arena (version 46) and QuakeLive (version 47) */
        Q3 = 0, 

        /** Half-Life 1 */
        HL, 

        NUM_FILE_FORMATS};

    friend class FaceSet;
    friend class Mesh;
    friend class Patch;
    friend class Billboard;

    Array<Vertex>       vertexArray;
    Array<int>          meshVertexArray;
    Array<BSPNode>      nodeArray;
    Array<BSPLeaf>      leafArray;
    
    Array<BSPPlane>     planeArray;
    
    Array<Brush>        brushArray;
    Array<BrushSide>    brushSideArray;

    Array<int>          leafFaceArray;
    Array<int>          leafBrushArray;

    BSPModel            staticModel;
public:
    Array<BSPModel>     dynamicModels;
public:// TODO: make private
    Vector3int32        lightVolumesGrid;
    Vector3             lightVolumesInvSizes;
    int                 lightVolumesCount;

    /** lightVolumes[x + (MAX_Z - z - 1) * MAX_X + y * MAX_X * MAX_Z] */
    LightVolume*        lightVolumes;

private:
    VisData             visData;
    
    /**
       @brief Visible polygons

       The individual faceArray are various subclasses of
      FaceSet, so we store pointers to them.  Allocated
      on load, deleted on destruction of the Map class.*/
    Array<FaceSet*>       faceArray;

    Array<Texture::Ref>   textures;
    BitSet                textureIsHollow;
    Array<Texture::Ref>   lightmaps;
    BitSet                facesDrawn;
    Texture::Ref          defaultTexture;
    Texture::Ref          defaultLightmap;

public:
	Array<BSPEntity>	  entityArray;

private:
    Vector3               startingPosition;

    /** Bounding box on the whole map */
    AABox                 m_bounds;

    /**
     filename has no extension.  JPG, PNG, and TGA files are sought.
     The textures are brightened by a factor of 2.0.

     \param index Index of the texture in the map's texture array; useful for error reporting
     */
    Texture::Ref loadTexture(const std::string& resPath, const std::string& altPath, const std::string& filename, int index);

    /**
     Loads version information from the front of a file.  Called from load.
     */
    static void loadVersion(BinaryInput& bi, MapFileFormat& mapFormat, int& version);

    /** Called from load */
    void loadQ3(BinaryInput& bi, const std::string& resPath,
        const std::string&  altPath);

    /** Called from load */
    void loadHL(BinaryInput& bi, const std::string& resPath,
        const std::string&  altPath);

    /**
     Loads the header info into an appropriately size lump array.
     */
    void loadLumps          (BinaryInput& bi, class BSPLump* lumps, int numLumps);
    void loadEntities       (BinaryInput& bi, const class BSPLump& lump);
    void loadVertices       (BinaryInput& bi, const class BSPLump& lump);
    void loadMeshVertices   (BinaryInput& bi, const class BSPLump& lump);
    void loadFaces          (BinaryInput& bi, const class BSPLump& lump);
    void loadTextures       (const std::string& resPath, const std::string& altResPath, BinaryInput& bi, const class BSPLump& lump);
    void loadLightMaps      (BinaryInput& bi, const class BSPLump& lump);
    void loadNodes          (BinaryInput& bi, const class BSPLump& lump);
    void loadQ3Leaves       (BinaryInput& bi, const class BSPLump& lump);
    void loadHLLeaves       (BinaryInput& bi, const class BSPLump& lump);
    void loadLeafFaceArray  (BinaryInput& bi, const class BSPLump& lump);
    void loadBrushes        (BinaryInput& bi, const class BSPLump& lump);
    void loadBrushSides     (BinaryInput& bi, const class BSPLump& lump);
    void loadLeafBrushes    (BinaryInput& bi, const class BSPLump& lump);
    void loadPlanes         (BinaryInput& bi, const class BSPLump& lump);
    void loadStaticModel    (BinaryInput& bi, const class BSPLump& lump);
    void loadDynamicModels  (BinaryInput& bi, const class BSPLump& lump);
    void loadLightVolumes   (BinaryInput& bi, const class BSPLump& lump);

    void loadQ1VisData      (BinaryInput& bi, const class BSPLump& lump);
    void loadHLVisData      (BinaryInput& bi, const class BSPLump& lump, const class BSPLump& leafLump);
    void loadQ3VisData      (BinaryInput& bi, const class BSPLump& lump);

    /** Decompresses Q1 run-length encoded vis data (also used by HL) to Q3
        format.  pvsBuffer is the run length encoded data, visOffset is
        the array of offsets into pvsBuffer where each leaf's vis data
        begins.  Q1 does not have clusters, so the number of clusters == number
        of leaves.

        Called from loadQ1VisData and loadHLVisData. 

        Deletes both arrays when done.*/
    void decompressQ1VisData(G3D::uint8*   pvsBuffer, G3D::uint32*  visOffset);

    /** Called from load to verify the integrity of the data that was just loaded. */
    void verifyData();

    /**
     Returns true if testCluster is potentially visible to a viewer within
     visCluster.
     */
    inline bool isClusterVisible(int visCluster, int testCluster) const {

	    if ((visData.bitsets == NULL) || (visCluster < 0)) {
		    return true;
	    }

	    // Note: testCluster >> 3 == testCluster / 8
	    int i = (visCluster * visData.bytesPerCluster) + (testCluster >> 3);

        // uint8 in original implementation; believe uint32 will be faster.
        G3D::uint32 visSet = visData.bitsets[i];

	    return (visSet & (1 << (testCluster & 7))) != 0;
    }
    
    int findLeaf(const Vector3& pos) const;
    
    void slide(Vector3& pos, Vector3& vel, const Vector3& extent);
    
    void collide(Vector3& pos, Vector3& vel, const Vector3& extent);
    
    BSPCollision checkMove(Vector3& pos, Vector3& vel, const Vector3& extent);
    
    void checkMoveLeaf(int leaf, BSPCollision* moveCollision) const;

    void checkMoveNode(
        float start, float end, Vector3 startPos, Vector3 endPos,
        int node, BSPCollision* collision) const;

    void clipBoxToBrush(const Brush* brush, BSPCollision* moveCollision) const;

    void clipVelocity(const Vector3& in, const Vector3& planeNormal, Vector3& out, float overbounce) const;

    /**
     Called by render.  Returns the visible set of translucent faces and
     opaque faces.  The sets are sorted by depth order.
     */
    void getVisibleFaces(
        RenderDevice*               renderDevice,
        const GCamera&              camera,
        Array<FaceSet*>&            translucentFaceArray,
        Array<FaceSet*>&            opaqueFaceArray);
    
    /**
      Called by render
     */
    void renderFaces(
        RenderDevice*               renderDevice,
        const GCamera&              camera,
        const Array<FaceSet*>&      visibleFaceArray);

    Map();

    bool load(
        const std::string&  resPath,
        const std::string&  filename,
        const std::string&  altPath,
        const std::string&  defaultTextureFile);

public:

    
    ~Map();
    
    /** 
      Move an object, sliding where it collides with walls (as is done in Quake and most FPS games).

      \param pos Initial pos, which is updated to the new position.
      \param extent World-space axis aligned extents of the object.
      \param vel Movement step size.  This is updated based on the actual step taken.

     \sa checkCollision
     */
    void slideCollision(Vector3& pos, Vector3& vel, const Vector3& extent);

    /** 
     \sa slideCollision
     */
    void checkCollision(Vector3& pos, Vector3& vel, const Vector3& extent);

    /**
     Returns NULL if an error occurs while loading.

     @param path to the Quake 3 resource directory (i.e., the
     directory that contains the "maps" subdir. This is the .pk3 file
     if working from a compressed map file.

     @param fileName Name of the .bsp file; include the extension

     @param scale Multiply all vertices by this scale factor on load.

     @param altLoad The root of a directory to search for missing textures. When loading 
     Quake3 maps that use default textures, this should be the pak0.pk3 file that
     comes with Quake3 Arena.  Note that this file is copyrighted by id software and
     is not redistributable. It is not part of G3D.   If set to "<none>", avoids the
     (slow) process of looking in the pk3 file for textures.  If set to "", the system
     looks in the root of drives (on Windows) and in System::findDataFile locations
     for pak0.pk3 or mini-pak0.pk3.
     
     You can obtain a limited version of this file by downloading the Q3A demo from:
     http://www.idsoftware.com/games/quake/quake3-arena/index.php?game_section=demo
     On Windows, the relevant file is at <code>C:\\Q3Ademo\\demoq3\\pak0.pk3</code>

     \param defaultTextureFile If a texture is missing, load this texture.  if "",
    use the default texture specified at runtime.

     */
    static MapRef fromFile(const std::string& path, const std::string& fileName, float scale = 1.0f, std::string altLoad = "",
         const std::string& defaultTextureFile = "");

    void setDefaultTexture(const Texture::Ref& txt) {
        defaultTexture = txt;
    }

    Vector3 getStartingPosition() const {
        return startingPosition;
    }
    const Array<BSPEntity>& getEntityList() const {
        return entityArray;
    }
    const Array<BSPModel>& getModelList() const {
		return dynamicModels;
    }

    /**
     @brief Draws the scene from the perspective of the camera.

     @param brightnessScale Multiply precomputed lightmap intensities by this value.
     Passed as an OpenGL color, so this may be clamped to [0,1] on the GPU.
     */
    void render(RenderDevice* renderDevice, const GCamera& camera, float brightnessScale = 1.0f);

    /** Reserved for future use. Do not call.*/
    void render(GCamera& camera, void* object);

    /** @brief Returns the triangles in the map for use outside of this class.

        The @a outVertexArray, @a outNormalArray,  @a outTexCoordArray, and @a outLightCoordArray
        are parallel arrays
        that are the source data for an indexed triangle list.
        Every three sequential values in @a outIndexArray are the indices into those

        @a textureMapIndexArray has length <code>outIndexArray.size()/3</code>. It specifies
        the index of the texture in @a outTextureMapArray to use for each triangle. 
        @a outLightMapIndexArray is a parallel array to @a textureMapIndexArray that specifies
        the index of the light map.
        */
    void getTriangles(
        Array<Vector3>&       outVertexArray,
        Array<Vector3>&       outNormalArray,
        Array<int>&           outIndexArray,
        Array<Vector2>&       outTexCoordArray,
        Array<int>&           outTextureMapIndexArray,
        Array<Vector2>&       outLightCoordArray,
        Array<int>&           outLightMapIndexArray,
        Array<Texture::Ref>&  outTextureMapArray,
        Array<Texture::Ref>&  outLightMapArray) const;

    /** Returns a bounding box on the whole map */
    const AABox& bounds() const {
        return m_bounds;
    }

};

} // _BSPMAP

typedef _BSPMAP::Map BSPMap;
typedef _BSPMAP::MapRef BSPMapRef;

} // G3D

#endif
