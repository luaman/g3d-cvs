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
    - SKINS:
       - Load all .skin files when the model is loaded
       - Have the upper and lower skin names be part of Pose (as a std::string); let them be separate
       - Lazy loading the actual textures on first use is fine
       - Add an interface for enumerating the skins (e.g., const Array<std::string>& skins(which = {UPPER_EXCLUSIVE_FULLNAME, LOWER_EXCLUSIVE_FULLNAME, ALL_FULLNAME, BOTH_POSTFIX) const)
       - Make the first skin found by getFiles the default, so that the model always has *some* valid skin; right now the model is invisible by default.
    - Add pose animation helpers, ala MD2 model (although the function with all of the bool arguments really sucks on MD2Model)
    - Render using SuperSurface [Morgan]

    \beta
*/
class MD3Model : public ReferenceCountedObject {

public:
    typedef ReferenceCountedPointer<MD3Model> Ref;
    
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

    class Pose {
    public:
        GameTime    legsTime;
        AnimType    legsAnim;

        GameTime    torsoTime;
        AnimType    torsoAnim;

        inline Pose(GameTime lTime, AnimType lAnim, GameTime tTime, AnimType tAnim)
            : legsTime(lTime), legsAnim(lAnim), torsoTime(tTime), torsoAnim(tAnim) {
        }

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

    enum PartType {
        PART_LEGS,
        PART_TORSO,
        PART_HEAD,
        PART_WEAPON,
        NUM_PARTS
    };

    /** Individual loaded .md3 files representing each part of a full quake model.
        Parts are loaded in order up to the last .md3 found */
    MD3Part*        m_parts[NUM_PARTS];

    /** Pre-parsed animation data for all frames and types.
        Starting frame numbers are relative to model (e.g., legs and torso) */
    AnimFrame       m_animations[NUM_ANIMATIONS];

    /** Skin to (surface, texture) mapping.  TODO: Make these materials */
    Table< std::string, Table<std::string, Texture::Ref> > m_skins;

    MD3Model();

    /** Load complete set of model parts from modelDir. */
    void loadDirectory(const std::string& modelDir);

    /** Load parameters for each standard animation type. */
    void loadAnimationCfg(const std::string filename);

    void posePart(PartType partType, const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe);

    /** Calculates frame number from animation type and animation time.
        Frame number is relative to model. */
    float findFrameNum(AnimType animType, GameTime animTime);

public:

    virtual ~MD3Model();

    /** Loads all model parts from \a modelDir if they exist.  This is the directory containing animation.cfg.

        Also loads animation.cfg parameters.
     */
    static MD3Model::Ref fromDirectory(const std::string& modelDir);

    /** Load skin for all model parts.
        \param skinName Base skin name for all model parts. e.g., head_"skinName".skin
     */
    void setSkin(const std::string& skinName);

    /** Pose all model parts based on animations selected in pose.
        Each part may add multiple surfaces to posedModelArray.
     */
    void pose(const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe = CoordinateFrame());
};


} // namespace G3D

#endif //G3D_MD3Model_h
