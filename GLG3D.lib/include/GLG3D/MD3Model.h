/**
 \file MD3Model.h

 Quake III MD3 model loading and posing

  \created 2009-01-01
  \edited  2009-11-13
 */
#ifndef G3D_MD3Model_h
#define G3D_MD3Model_h

#include "G3D/platform.h"
#include "G3D/Matrix3.h"
#include "G3D/Table.h"
#include "GLG3D/Surface.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/Texture.h"


namespace G3D {

// forward declare MD3Part
class MD3Part;

/**
    Quake III MD3 model loader.

    Quake 3 uses MD3 models for both characters and non-character objects.  
    Character objects contain three individual "models" inside of them with attachment points.

    TODO:
    - Add pose animation helpers, ala MD2 model (although the function with all of the bool arguments really sucks on MD2Model)
    - Render using SuperSurface [Morgan]

    \beta
*/
class MD3Model : public ReferenceCountedObject {

public:
    typedef ReferenceCountedPointer<MD3Model> Ref;

    enum PartType {
        PART_LEGS,
        PART_TORSO,
        PART_HEAD,
        PART_WEAPON,
        NUM_PARTS
    };

    /**
        All standard animation types expected to 
        have parameters in the animation.cfg file.
     */
    enum AnimType {
        BOTH_DEATH1,
        START_BOTH = BOTH_DEATH1,
        BOTH_DEAD1,
        BOTH_DEATH2,
        BOTH_DEAD2,
        BOTH_DEATH3,
        BOTH_DEAD3,
        END_BOTH = BOTH_DEAD3,

        TORSO_GESTURE,
        START_TORSO = TORSO_GESTURE,
        TORSO_ATTACK,
        TORSO_ATTACK2,
        TORSO_DROP,
        TORSO_RAISE,
        TORSO_STAND,
        TORSO_STAND2,
        END_TORSO = TORSO_STAND2,

        LEGS_WALKCR,
        START_LEGS = LEGS_WALKCR,
        LEGS_WALK,
        LEGS_RUN,
        LEGS_BACK,
        LEGS_SWIM,
        LEGS_JUMP,
        LEGS_LAND,
        LEGS_JUMPB,
        LEGS_LANDB,
        LEGS_IDLE,
        LEGS_IDLECR,
        LEGS_TURN,
        END_LEGS = LEGS_TURN,

        NUM_ANIMATIONS
    };

    /**
        Animation pose based on AnimType and animation time.
        Each animation time ( \a legsTime and \a torsoTime )
        is total time in the current animation which allows for 
        looping based on the parameters in animation.cfg.

        The skins must be the base name of each skin file
        found in the same directory as the model parts.

        Textures for each skin are loaded on first use.
     */
    class Pose {
    public:
        GameTime    legsTime;
        AnimType    legsAnim;

        GameTime    torsoTime;
        AnimType    torsoAnim;

        std::string skinNames[NUM_PARTS];

        inline Pose(GameTime lt, AnimType la, GameTime tt, AnimType ta) :
            legsTime(lt), legsAnim(la), torsoTime(tt), torsoAnim(ta) {}

        inline Pose() : legsTime(0), legsAnim(LEGS_IDLE), torsoTime(0), torsoAnim(TORSO_STAND) {}
    };

private:
    /** Animation data from animation.cfg */
    class AnimFrame {
    public:
        float   start;
        float   num;
        float   loop;
        float   fps;

        AnimFrame() : start(0), num(0), loop(0), fps(0) {}
        AnimFrame(float s, float n, float l, float f) : start(s), num(n), loop(l), fps(f) {}
    };

    MD3Part*        m_parts[NUM_PARTS];

    AnimFrame       m_animations[NUM_ANIMATIONS];

    struct SkinValue { std::string filename; Texture::Ref texture; };
    typedef Table<std::string, SkinValue> PartSkin;
    typedef Table<size_t, PartSkin> SkinCollection;

    // Collection of shared skins across all models to avoid texture re-loading
    static SkinCollection s_skins[NUM_PARTS];

    MD3Model();

    void loadDirectory(const std::string& modelDir);

    void loadAnimationCfg(const std::string& filename);

    /** Loads all skins into s_skins but leaves texture loading to pose. */
    void loadAllSkins(const std::string& skinDir);

    void loadSkin(const std::string& filename, PartSkin& skinCollection);

    /** Calculates relative frame number for part */
    float findFrameNum(AnimType animType, GameTime animTime);

    void posePart(PartType partType, const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe);

public:

    virtual ~MD3Model();

    /**
        Loads all available parts of a Quake III model in \a modelDir
        as well as the animation.cfg file containing all standard animation values.

        Order of part loading is: lower.md3 -> upper.md3 -> head.md3 -> weapon.md3
     */
    static MD3Model::Ref fromDirectory(const std::string& modelDir);

    /**
        Poses then adds all available parts to \a posedModelArray.
        Each part is posed based on the animation parameters then
        positioned and rotated based on the appropriate tag according
        to Quake III model standards.

        The lower.md3 part is the based.  The upper.md3 part is attached
        to "tag_torso" in lower.md3.  The weapon.md3 part is attached to
        "tag_weapon" in upper.md3.  The head.md3 part is attached to 
        "tag_head" in upper.md3.

        The initial \a cframe transformation is applied to the base 
        lower.md3 part before the whole model is posed.
     */
    void pose(const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe = CoordinateFrame());

    /**
        Retrieves all available skin names for \a partType.
     */
    void skinNames(PartType partType, Array<std::string>& names) const;
};


} // namespace G3D

#endif //G3D_MD3Model_h
