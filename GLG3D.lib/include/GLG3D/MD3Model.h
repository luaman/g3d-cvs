/**
 \file MD3Model.h

 Quake III MD3 model loading and posing

  \created 2009-01-01
  \edited  2010-04-26
 */
#ifndef G3D_MD3Model_h
#define G3D_MD3Model_h

#include "G3D/platform.h"
#include "G3D/Matrix3.h"
#include "G3D/Table.h"
#include "GLG3D/Surface.h"
#include "GLG3D/VertexBuffer.h"
#include "GLG3D/Texture.h"
#include "GLG3D/Material.h"


namespace G3D {

// forward declare MD3Part
class MD3Part;

/**
    Quake III MD3 model loader.

    Quake 3 uses MD3 models for both characters and non-character objects.  
    Character objects contain three individual "models" inside of them with attachment points.
    
    MD3Models are composed of up to four parts, which are named lower (legs), upper (torso), and head.
    The coordinate frame for each relative to its parent can be specified as part of the pose.
    Each part contains a set of triangle lists.  The triangle lists may have different materials and are 
    key-frame animated. A skin is a set of materials for the triangle lists.  The model is created
    with a default skin, although an alternative skin may be provided as part of the pose.  This allows
    sharing geometry over characters with different appearance.

    It also contains a coordinate frame for a weapon's attachment location.

    See http://bit.ly/acgNj9 for some models
    \beta

    \cite http://icculus.org/homepages/phaethon/q3a/formats/md3format.html
    \cite http://www.misfitcode.com/misfitmodel3d/olh_quakemd3.html

    TODO: Implement free blending
    TODO: Implement nice pose animation API
    TODO: Add to starter code
*/
class MD3Model : public ReferenceCountedObject {

public:
    typedef ReferenceCountedPointer<MD3Model> Ref;

    enum PartType {
        PART_LOWER,
        PART_UPPER,
        /** Heads are never animated */
        PART_HEAD
    };

    enum {NUM_PARTS = 3, NUM_ANIMATED_PARTS = 2};
   
    /**
        All standard animation types expected to 
        have parameters in the animation.cfg file.
     */
    enum AnimType {
        BOTH_DEATH1,
        BOTH_DEAD1,
        BOTH_DEATH2,
        BOTH_DEAD2,
        BOTH_DEATH3,
        BOTH_DEAD3,

        UPPER_GESTURE,
        UPPER_ATTACK,
        UPPER_ATTACK2,
        UPPER_DROP,
        UPPER_RAISE,
        UPPER_STAND,
        UPPER_STAND2,

        LOWER_WALKCR,
        LOWER_WALK,
        LOWER_RUN,
        LOWER_BACK,
        LOWER_SWIM,
        LOWER_JUMP,
        LOWER_LAND,
        LOWER_JUMPB,
        LOWER_LANDB,
        LOWER_IDLE,
        LOWER_IDLECR,
        LOWER_TURN,

        NUM_ANIMATIONS,

        START_BOTH = BOTH_DEATH1,
        END_BOTH = BOTH_DEAD3,

        START_UPPER = UPPER_GESTURE,
        END_UPPER = UPPER_STAND2,

        START_LOWER = LOWER_WALKCR,
        END_LOWER = LOWER_TURN
    };

    /** A set of materials for a MD3Model. */
    class Skin : ReferenceCountedObject {
    public:

        typedef ReferenceCountedPointer<Skin> Ref;

    private:
        
        Skin() {}

    public:
        /** Maps triList names to materials.  If a material is specified as NULL
            (which corresponds to Quake's common/nodraw), that means "do not draw this triList". */
        typedef Table<std::string, Material::Ref>   PartSkin;

        /** Table for each part.  Indices are PartTypes.*/
        Array<PartSkin> partSkin;

        static Ref create() {
            return new Skin();
        }

        static Ref create
            (const std::string& commonPath,
            const std::string& lowerSkin, 
            const std::string& upperSkin, 
            const std::string& headSkin);

        /** Loads commonPath + "lower_" + commonSuffix + ".skin", etc. for other parts */
        static Ref create
           (const std::string& commonPath,
            const std::string& commonSuffix);

        /**
          Format is either: 

           - MD3Model::Skin( <list of part skins> )

          Each part skin is either a .skin file relative to the md3 directory or an Any table mapping a tri list name to a material.  It may have an optional name;
          it is optional but convenient to make this the name of the part. For example:

          <pre>
              MD3Model::Skin(
                 "lower_dragon.skin",
                 "upper_dragon.skin",
                 head {
                   h_cap = NONE, 
                   h_head = Material::Specification {
                      diffuse = "Happy.tga"
                   },
                   h_Visor = NONE,
                   h_Helmet = Material::Specification {
                      diffuse = "Knight2A1.tga"
                   }
                 }
             )
          </pre>
        */
        static Ref create(const Any& a);

    private:

        static void loadSkinFile(const std::string& filename, PartSkin& partSkin);
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
        GameTime    time[NUM_ANIMATED_PARTS];
        AnimType    anim[NUM_ANIMATED_PARTS];

        /** Applying a rotation rotates this part and everything 
          attached to it relative to its parent.  Rotations are typically used to
          make the torso point towards a target or the head in the look direction.*/
        Matrix3     rotation[NUM_PARTS];

        /** If NULL, use the model's default skin */
        Skin::Ref   skin;

        Pose();
    };

    /** Advances the pose based on this character's animations. */
    void simulatePose(Pose& pose, GameTime dt) const;

    class Specification {
    public:
        /** Directory containing head.md3, upper.md3, lower.md3, torso.md3, and animation.cfg */
        std::string     directory;

        Skin::Ref       defaultSkin;

        Specification() {}

        /** 
          Format is:
             <pre>
			 MD3Model::Specification {

                // Directory containing the *.md3 files
                directory = "...",

                defaultSkin = MD3Model::Skin( ... )
            }
            </pre>

           or just a string specifying a directory.

        */
        Specification(const Any& any);
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

    Skin::Ref       m_defaultSkin;

    MD3Model();

    void loadSpecification(const Specification& spec);

    void loadAnimationCfg(const std::string& filename);

    /** Calculates relative frame number for part */
    float findFrameNum(AnimType animType, GameTime animTime) const;

    void posePart(PartType partType, const Pose& pose, Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe);

public:

    virtual ~MD3Model();

    /**
        Loads all available parts of a Quake III model in \a modelDir
        as well as the animation.cfg file containing all standard animation values.

        Order of part loading is: lower.md3 -> upper.md3 -> head.md3

        \deprecated Use MD3Model::create()
     */
    static MD3Model::Ref fromDirectory(const std::string& modelDir, const Skin::Ref& defaultSkin = NULL);

	static MD3Model::Ref create(const Specification& spec);

    /**
        Poses then adds all available parts to \a posedModelArray.
        Each part is posed based on the animation parameters then
        positioned and rotated based on the appropriate tag according
        to Quake III model standards.

        The lower.md3 part is the based.  The upper.md3 part is attached
        to "tag_torso" in lower.md3.  The head.md3 part is attached to 
        "tag_head" in upper.md3.

        The initial \a cframe transformation is applied to the base 
        lower.md3 part before the whole model is posed.
     */
    void pose(Array<Surface::Ref>& posedModelArray, const CoordinateFrame& cframe = CoordinateFrame(), const Pose& pose = Pose());

    /** Return the coordinate frame of the tag_weapon; this is where a simulator should place objects carried by the character.*/
    CoordinateFrame weaponFrame(const CFrame& cframe = CoordinateFrame(), const Pose& pose = Pose()) const;

    const Skin::Ref defaultSkin() const {
        return m_defaultSkin;
    }
};


} // namespace G3D

#endif //G3D_MD3Model_h
