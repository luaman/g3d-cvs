/**
 @file Shader.h
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2004-04-25
 @edited  2009-11-30
 */

#ifndef G3D_Shader_h
#define G3D_Shader_h

#include "GLG3D/glheaders.h"
#include "GLG3D/Texture.h"
#include "G3D/Set.h"
#include "G3D/Vector4.h"
#include "G3D/CoordinateFrame.h"
#include "G3D/Vector3.h"
#include "G3D/Color4.h"
#include "G3D/Color3.h"
#include <string>

namespace G3D {

typedef ReferenceCountedPointer<class VertexAndPixelShader>  VertexAndPixelShaderRef;

#ifdef _DEBUG
    #define DEBUG_SHADER true
#else
    #define DEBUG_SHADER false
#endif

/** Argument to G3D::VertexAndPixelShader and G3D::Shader create methods */
enum PreprocessorStatus {
    PREPROCESSOR_DISABLED,
    PREPROCESSOR_ENABLED};


/**
   This is used internally by Shader.

   \deprecated

  A compatible vertex, geometry, and pixel shader.  Used internally by
  G3D::Shader; see that class for more information.

  Only newer graphics cards with recent drivers (e.g. GeForceFX cards
  with driver version 57 or greater) support this API.  Use the
  VertexAndPixelShader::fullySupported method to determine at run-time
  if your graphics card is compatible.

  For purposes of shading, a "pixel" is technically a "fragment" in OpenGL terminology.

  Pixel and vertex shaders are loaded as text strings written in 
  <A HREF="http://developer.3dlabs.com/openGL2/specs/GLSLangSpec.Full.1.10.59.pdf">GLSL</A>, the high-level
  OpenGL shading language.  

  Typically, the G3D::Shader sets up constants like the object-space position
  of the light source and the object-to-world matrix.  The vertex shader transforms
  input vertices to homogeneous clip space and computes values that are interpolated
  across the surface of a triangle (e.g. reflection vector).  The pixel shader
  computes the final color of a pixel (it does not perform alpha-blending, however).

  Multiple VertexAndPixelShaders may share object, vertex, and pixel shaders.

  Uniform variables that begin with 'gl_' are ignored because they are assumed to 
  be GL built-ins.

  @cite http://oss.sgi.com/projects/ogl-sample/registry/ARB/shader_objects.txt
  @cite http://oss.sgi.com/projects/ogl-sample/registry/ARB/vertex_shader.txt

@deprecated
 */
class VertexAndPixelShader : public ReferenceCountedObject {
public:

    /** If this shader was loaded from disk, reload it. */
    void reload();

    friend class Shader;

    /** Used by Shader */
    class UniformDeclaration {
    public:
        /** If true, this variable is declared in the shader but is not used in its body. */
        bool                dummy;
        
        /** Register location if a sampler. */
        int                 location;
        
        /** Name of the variable.  May include [] and . (e.g.
            "foo[1].normal"). As of 12/18/07, NVIDIA drivers process this incorrectly
            and only return "foo" in the example case. */
        std::string         name;
        
        /** OpenGL type of the variable (e.g. GL_INT) */
        GLenum              type;
        
        /** Unknown... appears to always be 1 */
        int                 size;
        
        /**
           Index of the texture unit in which this value
           is stored.  -1 for uniforms that are not textures. */  
        int                 textureUnit;
    };

protected:

    class GPUShader {
    protected:
        
        /** argument for output on subclasses */
        static std::string          ignore;
        
        /** Filename if loaded from disk */
        std::string                 filename;
        std::string                 _name;
        std::string                 _code;
        bool                        fromFile;
        
        GLhandleARB                 _glShaderObject;
        
        bool                        _ok;
        std::string                 _messages;
        
        /** Returns true on success.  Called from init. */
        void compile();
        
        /** Initialize a shader object and returns object.  
            Called from subclass create methods. */
        static GPUShader*           init(GPUShader* shader, bool debug);
        
        /** Set to true when name and code both == "" */
        bool                        _fixedFunction;
        
        GLenum                      _glShaderType;
        
        std::string                 _shaderType;
       
        /** Checks to ensure that this profile is supported on this
            card. Called from init().*/
        void checkForSupport();

        /**
           Replaces all instances of 
           <code>"g3d_sampler2DSize(<i>name</i>)"</code> with 
           <code>"     (g3d_sz2D_<i>name</i>.xy)"</code> and
           
           <code>"g3d_sampler2DInvSize(<i>name</i>)"</code> with
           <code>"        (g3d_sz2D_<i>name</i>.zw)"</code> 
           
          Note that both replacements will leave column numbers the
          same in error messages.  The <code>()</code> wrapper ensures
          that <code>.xy</code> fields are accessible using normal
          syntax off the result; it is the same as the standard
          practice of wrapping macros in parentheses.

          and adds "<code>uniform vec4 g3d_sz2D_<i>name</i>;</code>"
          to the uniform string.

          Called from init.
         */
        void replaceG3DSize(std::string& code, std::string& uniformString);

        /**
         Replaces all instances of <code>g3d_Index[<i>samplername</i>]</code>
         with <code>(g3d_Indx_<i>samplername</i>)</code> Called from init.

         The first time a file is compiled, samplerMappings is empty.
         It must then compiled again with correct mappings, which are
         assigned elsewhere.

         @param defineString New \#defines to insert at the top of the program
         @param code modified in place
         @param secondPass On the scond pass, the samplerMappings must not be empty.
         @return True if there was one replacement, false otherwise
         */
        bool replaceG3DIndex(std::string& code, std::string& defineString, 
                             const Table<std::string, int>& samplerMappings, bool secondPass);
        
        bool            m_usesG3DIndex;

    public:

        const std::string& code() const {
            return _code;
        }

        /** True if this shader uses the g3d_Index extension and
            therefore needs double-compilation to resolve
            dependencies. */
        bool usesG3DIndex() const {
            return m_usesG3DIndex;
        }

        /**
         @param samplerMappings Table mapping sampler names to their gl_TexCoord indices.  
         This may be empty if the mappings are not yet known.
         */
        void init
        (const std::string& name,
         const std::string& code,
         bool               fromFile,
         bool               debug,
         GLenum             glType,
         const std::string& type,
         PreprocessorStatus u,
         const Table<std::string, int>& samplerMappings,
         bool               secondPass);
            
        /** Deletes the underlying glShaderObject.  Between GL's reference
            counting and G3D's reference counting, an underlying object
            can never be deleted while in use. */
        ~GPUShader();
        
        /** Shader type, e.g. GL_VERTEX_SHADER_ARB */
        GLenum glShaderType() const {
            return _glShaderType;
        }
        
        const std::string& shaderType() const {
            return _shaderType;
        }
        
        /** Why compilation failed, or any compiler warnings if it succeeded.*/
        const std::string& messages() const {
            return _messages;
        }
        
        /** Returns true if compilation and loading succeeded.  If they failed,
            check the message string.*/
        bool ok() const {
            return _ok;
        }
        
        /** Returns the underlying OpenGL shader object for this shader */
        GLhandleARB glShaderObject() const {
            return _glShaderObject;
        }
        
        bool fixedFunction() const {
            return _fixedFunction;
        }
    };

    static std::string      ignore;

    GPUShader	            vertexShader;
    GPUShader	            geometryShader;
    GPUShader	            pixelShader;

    GLhandleARB             _glProgramObject;

    bool                    _ok;
    std::string             _messages;

    std::string             _vertCompileMessages;
    std::string             _geomCompileMessages;
    std::string             _fragCompileMessages;
    std::string             _linkMessages;

    int                     lastTextureUnit;

    /** Does not contain g3d_ uniforms if they were compiled away */
    Array<UniformDeclaration>   uniformArray;

    /** Does not contain g3d_ uniforms if they were compiled away */
    Set<std::string>            uniformNames;

    /** Converts from int and bool types to float types (e.g.,
        <code>GL_INT_VEC2_ARB</code> ->
        <code>GL_FLOAT_VEC2_ARB</code>).  Other types are left
        unmodified.*/
    static GLenum canonicalType(GLenum e);

    /** Computes the uniformArray from the current
        program object.  Called from the constructor */
    void computeUniformArray();

    /** Finds any uniform variables in the code that are not already
        in the uniform array that OpenGL returned and adds them to
        that array.  This causes VertexAndPixelShader to surpress
        warnings about setting variables that have been compiled
        away--those warnings are annoying when temporarily commenting
        out code. */
    void addUniformsFromCode(const std::string& code);

    /** Returns true for types that are textures (e.g., GL_TEXTURE_2D) */
    static bool isSamplerType(GLenum e);

    /** \param maxGeometryOutputVertices Set to -1 if using a layout qualifier for GLSL version 1.5 or later.*/
    VertexAndPixelShader
    (const std::string&  vsCode,
     const std::string&  vsFilename,
     bool                vsFromFile,
     const std::string&  gsCode,
     const std::string&  gsFilename,
     bool                gsFromFile,
     const std::string&  psCode,
     const std::string&  psFilename,
     bool                psFromFile,
     int                 maxGeometryOutputVertices,
     bool                debug,
     PreprocessorStatus  u);

    /** Returns true if Shader will allow this coercion. These should
        be non-canonical types. */
    static bool compatibleTypes(GLenum actual, GLenum formal);

public:

    /** True if this variable is defined. @beta */
    bool definesArgument(const std::string& name) {
        return uniformNames.contains(name);
    }

    /** @beta */
    const Array<UniformDeclaration>& argumentArray() const {
        return uniformArray;
    }

    /** Thrown by validateArgList */
    class ArgumentError {
    public:
        std::string             message;
        
        ArgumentError(const std::string& m) : message(m) {}
    };
    
    static VertexAndPixelShaderRef fromStrings
    (const std::string& vertexShader,
     const std::string& pixelShader,       
     PreprocessorStatus u,
     bool               debugErrors);
    
    /**
       To use the default/fixed-function pipeline for part of the
       shader, pass an empty string.
    */
    static VertexAndPixelShaderRef fromStrings
    (const std::string& vertexShaderName,
     const std::string& vertexShader,
     const std::string& geometryShaderName,
     const std::string& geometryShader,       
     const std::string& pixelShaderName,
     const std::string& pixelShader,       
     PreprocessorStatus u,
     bool               debugErrors);
    
    /**
       To use the fixed function pipeline for part of the
       shader, pass an empty string.
       
       @param debugErrors If true, a debugging dialog will
       appear when there are syntax errors in the shaders.
       If false, failures will occur silently; check
       VertexAndPixelShader::ok() to see if the files
       compiled correctly.
    */
    static VertexAndPixelShaderRef fromFiles
    (const std::string& vertexShader,
     const std::string& geometryShader,
     const std::string& pixelShader,
     int maxGeometryShaderOutputVertices,
     PreprocessorStatus u,
     bool               debugErrors);

    /**
     Bindings of values to uniform variables for a VertexAndPixelShader.
     Be aware that 
     the uniform namespace is global across the pixel and vertex shader.


     If an argument is marked optional then it is only bound when the shader
     requires it.  If a non-optional variable is not declared within the shader
     an error occurs at runtime (so that you can debug the mismatch).
     */
    class ArgList {
    private:
        friend class VertexAndPixelShader;

        class Arg {
        public:

            /** Row-major.  Element [0][0] is a float if this is a GL_FLOAT */ 
            Vector4                    vector[4];

            Texture::Ref               texture;

            /** Stores individual ints and bools */
            int                        intVal;

            GLenum                     type;

            /** If an argument is marked as optional, it is only applied to the shader if it is defined within the shader.*/
            bool                       optional;

            Arg() : optional(false) {}
        };

        Table<std::string, Arg>        argTable;
        int                            m_size;

        /** Adds an argument to the argTable.  Called by all other set methods */
        void set(const std::string& key, const Arg& value);

    public:

        ArgList() : m_size(0) {}
        
        /** Arrays only count as a single argument. */
        int size() const {
            return m_size;
        }

        /** Merges @a a into this list. Values from @a override any currently in the arglist.*/
        void set(const ArgList& a);

        void set(const std::string& var, const Texture::Ref& val, bool optional = false);
        void set(const std::string& var, const CoordinateFrame& val, bool optional = false);
        void set(const std::string& var, const Matrix4& val, bool optional = false);
        void set(const std::string& var, const Matrix3& val, bool optional = false);
        void set(const std::string& var, const Color4&  val, bool optional = false);
        void set(const std::string& var, const Color3&  val, bool optional = false);
        void set(const std::string& var, const Vector4& val, bool optional = false);
        void set(const std::string& var, const Vector3& val, bool optional = false);
        void set(const std::string& var, const Vector2& val, bool optional = false);
        void set(const std::string& var, double         val, bool optional = false);
        void set(const std::string& var, float          val, bool optional = false);
        void set(const std::string& var, int            val, bool optional = false);
        void set(const std::string& var, bool           val, bool optional = false);

        /** Removes an argument from the list.  Error if that argument does not exist. */
        void remove(const std::string& var);

        /** Returns true if an argument named var */
        bool contains(const std::string& var) const {
            return argTable.containsKey(var);
        }
        
        /** Returns a newline separated list of arguments specified in this list. */
        std::string toString() const;
        
        void clear();
    };

    ~VertexAndPixelShader();

    /**
     Returns GLCaps::supports_GL_ARB_shader_objects() && 
        GLCaps::supports_GL_ARB_shading_language_100() &&
        GLCaps::supports_GL_ARB_fragment_shader() &&
        GLCaps::supports_GL_ARB_vertex_shader()
    */
    static bool fullySupported();

    inline bool ok() const {
        return _ok;
    }

    /**
     All compilation and linking messages, with additional formatting.
     For details about a specific part of the process, see 
     vertexErrors, pixelErrors, and linkErrors.
     */
    inline const std::string& messages() const {
        return _messages;
    }

    inline const std::string& vertexErrors() const {
        return _vertCompileMessages;
    }

    inline const std::string& pixelErrors() const {
        return _fragCompileMessages;
    }

    inline const std::string& linkErrors() const {
        return _linkMessages;
    }

    /** The underlying OpenGL object for the vertex/pixel shader pair.

        To bind a shader with RenderDevice, call renderDevice->setShader(s);
        To bind a shader <B>without</B> RenderDevice, call
        glUseProgramObjectARB(s->glProgramObject());

    */
    GLhandleARB glProgramObject() const {
        return _glProgramObject;
    }

    int numArgs() const {
        return uniformArray.size();
    }

    /** Checks the actual values of uniform variables against those 
        expected by the program.
        If one of the arguments does not match, an ArgumentError
        exception is thrown.
    */
    void validateArgList(const ArgList& args) const;

    /**
       Makes renderDevice calls to bind this argument list.
       Calls validateArgList.
     */
    void bindArgList(class RenderDevice* rd, const ArgList& args) const;

    /** Returns information about one of the arguments expected
        by this VertexAndPixelShader.  There are VertexAndPixelShader::numArgs()
        total.*/
    const UniformDeclaration& arg(int i) const {
        return uniformArray[i];
    }
};

class Shader;
typedef ReferenceCountedPointer<Shader> ShaderRef;

/**
  @brief A set of functions written in GLSL that are invoked by the
  GPU per vertex, per geometric primitive, and per pixel.

  Abstraction of the programmable hardware pipeline.  
  Use with G3D::RenderDevice::setShader.
  G3D::Shader allows you to specify C++ code (by overriding
  the methods) that executes for each group of primitives and
  OpenGL Shading Language (<A HREF="http://developer.3dlabs.com/openGL2/specs/GLSLangSpec.Full.1.10.59.pdf">GLSL</A>) 
  code that executes for each vertex and each pixel.
    
  <P>
  Uses G3D::VertexAndPixelShader internally.  What we call pixel shaders
  are really "fragment shaders" in OpenGL terminology.

  <P>
  
  Unless G3D_PREPROCESSOR_DISABLED is specified to the static
  constructor, the following additional features will be available
  inside the shaders:

  <PRE>
    uniform mat4 g3d_WorldToObjectMatrix;
    uniform mat4 g3d_ObjectToWorldMatrix;
    uniform mat4 g3d_WorldToCameraMatrix;
    uniform mat4 g3d_CameraToWorldMatrix;
    uniform mat3 g3d_ObjectToWorldNormalMatrix; // Upper 3x3 matrix (assumes that the transformation is RT so that the inverse transpose of the upper 3x3 is just R)
    uniform mat3 g3d_WorldToObjectNormalMatrix; // Upper 3x3 matrix (assumes that the transformation is RT so that the inverse transpose of the upper 3x3 is just R)
   </pre>

   Macros:
   <pre>
    vec2 g3d_sampler2DSize(sampler2D t);        // Returns the x and y dimensions of t
    vec2 g3d_sampler2DInvSize(sampler2D t);     // Returns vec2(1.0, 1.0) / g3d_size(t) at no additional cost

    int g3d_Index(sampler t); // Replaced at compile-time with the OpenGL index of the texture unit for samplerName (which must be a uniform).
    // This is needed when reading from textures rendered using Framebuffer, which are likely upside down and have inverted texture coordinates.
    // Typical usage : gl_TexCoord[g3d_Index(sampler)]

    \#include "file"
  </PRE>

  The macros that take a sampler argument must not have anything (even
  spaces!) inside the parentheses and their argument must be the name
  of a sampler uniform.

  \#include may not appear inside a block comment (it may appear inside
  a single-line comment, however), and must be the first statement on the 
  line in which it appears.  There may be no space between the # and the include.

  The macros <code>G3D_OSX, G3D_WIN32, G3D_FREEBSD, G3D_LINUX,
  G3D_ATI, G3D_NVIDIA, G3D_MESA</code> are defined on the relevant
  platforms.

  <code>g3d_sampler2DSize</code> and <code>g3d_sampler2DInvSize</code>
  require that there be no additional space between the function name
  and parens and no space between the parens and sampler name.  There
  is no cost for definining and then not using any of these; unused
  variables do not increase the runtime cost of the shader.


  If your GLSL 1.1 shader begins with <CODE>\#include</CODE> or <CODE>\#define</CODE> the
  line numbers will be off by 1 to 3 in error messages because the G3D uniforms are 
  inserted on the first line.  GLSL 1.2 shaders do not have this problem.

  <P>

  <P>
  ATI's RenderMonkey is an excellent tool for writing GLSL shaders under an IDE.
  http://www.ati.com/developer/rendermonkey/index.html
  You can then load those shaders into G3D::Shader and use them with your application.

  <B>Example</B>

  The following example computes lambertian + ambient shading in camera space,
  on the vertex processor.  Although the shader could easily have been loaded from a file,
  the example loads it from a string (which is conveniently created with the STR macro)

  Vertex
  shaders are widely supported, so this will run on any graphics card produced since 2001
  (e.g. GeForce3 and up).  Pixel shaders are available on "newer" cards 
  (e.g. GeForceFX 5200 and up).

  <PRE>
   IFSModelRef model;
   ShaderRef   lambertian;

   ...

   // Initialization
   model = IFSModel::create(app->dataDir + "ifs/teapot.ifs");
   lambertian = Shader::creates(Shader::VERTEX_STRING, STR(

     uniform vec3 k_A;

     void main(void) {
        gl_Position = ftransform();
        gl_FrontColor.rgb = max(dot(gl_Normal, g3d_ObjectLight0.xyz), 0.0) * gl_LightSource[0].diffuse + k_A;
     }));

    ...

    // Rendering loop
    app->renderDevice->setLight(0, GLight::directional(Vector3(1,1,1), Color3::white() - Color3(.2,.2,.3)));

    app->renderDevice->setShader(lambertian);
    lambertian->args.set("k_A", Color3(.2,.2,.3));
    model->pose()->render(app->renderDevice);
  </PRE>

  This example explicitly sets the ambient light color to demonstrate how
  arguments are used.  That could instead have been read from gl_LightModel.ambient.
  Note the use of g3d_ObjectLight0.  Had we not used that variable, Shader would
  not have computed or set it.

  See also: http://developer.download.nvidia.com/opengl/specs/GL_EXT_geometry_shader4.txt  

  <B>BETA API</B>
  This API is subject to change.
 */
class Shader  : public ReferenceCountedObject {
public:
    typedef ReferenceCountedPointer<Shader>   Ref;

    /** Replaces all \#includes in @a code with the contents of the appropriate files.
        It is called recursively, so included files may have includes themselves.
        This is called automatically by the preprocessor, but is public so as to be
        accessible to code like SuperShader that directly manipulates source strings.
        
        @param dir The directory from which the parent was loaded.
      */
    static void processIncludes(const std::string& dir, std::string& code);

protected:

    VertexAndPixelShaderRef         _vertexAndPixelShader;

    /** If true, needs the built-in G3D uniforms that appear in the
        code to be bound */
    bool                            m_useUniforms;

    bool                            _preserveState;

    inline Shader(VertexAndPixelShaderRef v, PreprocessorStatus s) : 
        _vertexAndPixelShader(v), 
        m_useUniforms(s == PREPROCESSOR_ENABLED),
        _preserveState(true) {}

    /** For subclasses to invoke */
    inline Shader() {}

public:

    /** True if this variable is defined. @beta */
    bool definesArgument(const std::string& name) {
        return _vertexAndPixelShader->definesArgument(name);
    }

    /** @beta */
    const Array<VertexAndPixelShader::UniformDeclaration>& argumentArray() const {
        return _vertexAndPixelShader->argumentArray();
    }

    /** If this shader was loaded from disk, reload it */
    void reload();

    /** Arguments to the vertex and pixel shader.  You may change these either
        before or after the shader is set on G3D::RenderDevice-- either way
        they will take effect immediately.*/
    VertexAndPixelShader::ArgList   args;

    /** Returns true if this shader is declared to accept the specified argument. */
    bool hasArgument(const std::string& argname) const;

        
    inline static ShaderRef fromFiles(
        const std::string& vertexFile, 
        const std::string& pixelFile,
        PreprocessorStatus s = PREPROCESSOR_ENABLED) {
        return new Shader(VertexAndPixelShader::fromFiles(vertexFile, "", pixelFile, -1, s, DEBUG_SHADER), s);
    }

    /** If a geometry shader is specified, a vertex shader must also be specified.
    \param maxGeometryShaderOutputVertices Set to -1 if using GLSL 1.5 with a layout qualifier.*/
    inline static ShaderRef fromFiles(
        const std::string& vertexFile, 
        const std::string& geomFile,
        const std::string& pixelFile,
        int maxGeometryShaderOutputVertices = -1,
        PreprocessorStatus s = PREPROCESSOR_ENABLED) {
        return new Shader(VertexAndPixelShader::fromFiles(vertexFile, geomFile, pixelFile, maxGeometryShaderOutputVertices, s, DEBUG_SHADER), s);
    }

    inline static ShaderRef fromStrings(
        const std::string& vertexCode,
        const std::string& pixelCode,
        PreprocessorStatus s = PREPROCESSOR_ENABLED) {
        return new Shader(VertexAndPixelShader::fromStrings(vertexCode, pixelCode, s, DEBUG_SHADER), s);
    }

    /** Names are purely for debugging purposes */
    inline static ShaderRef fromStrings(
        const std::string& vertexName,
        const std::string& vertexCode,
        const std::string& pixelName,
        const std::string& pixelCode,
        PreprocessorStatus s = PREPROCESSOR_ENABLED) {
        return new Shader(VertexAndPixelShader::fromStrings(vertexName, vertexCode, "", "", pixelName, pixelCode, s, DEBUG_SHADER), s);
    }

    /** When true, any RenderDevice state that the shader configured before a primitive it restores at
        the end of the primitive.  When false, the shader is allowed to corrupt state.  Setting to false can
        lead to faster operation
        when you know that the next primitive will also be rendered with a shader, since shaders tend
        to set all of the state that they need.

        Defaults to true */
    virtual void setPreserveState(bool s) {
        _preserveState = s;
    }

    virtual bool preserveState() const {
        return _preserveState;
    }

    virtual bool ok() const;

    /** Returns true if this card supports vertex shaders */
    static bool supportsVertexShaders();

    /** Returns true if this card supports geometry shaders */
    static bool supportsGeometryShaders();

    /** Returns true if this card supports pixel shaders.  */
    static bool supportsPixelShaders();

	/**
	 Invoked by RenderDevice immediately before a primitive group.
	 Override to set state on the RenderDevice (including the underlying
     vertex and pixel shader).

     If overriding, do not call RenderDevice::setShader from this routine.

     Default implementation pushes state, sets the g3d_ uniforms,
     and loads the vertex and pixel shader.
	 */
    virtual void beforePrimitive(class RenderDevice* renderDevice);

    /**
     Default implementation pops state.
     */
    virtual void afterPrimitive(class RenderDevice* renderDevice);

    virtual const std::string& messages() const;
};


}

#endif
