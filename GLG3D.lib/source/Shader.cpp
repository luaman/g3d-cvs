/**
 @file Shader.cpp
 
 @maintainer Morgan McGuire, Jared Hoberock, and Qi Mo, http://graphics.cs.williams.edu
 
 @created 2004-04-24
 @edited  2009-11-31
 */

#include "G3D/fileutils.h"
#include "G3D/Log.h"
#include "GLG3D/Shader.h"
#include "GLG3D/GLCaps.h"
#include "GLG3D/getOpenGLState.h"
#include "GLG3D/RenderDevice.h"
#include "G3D/TextInput.h"
#include "G3D/FileSystem.h"

namespace G3D {

void Shader::reload() {
    _vertexAndPixelShader->reload();
}


bool Shader::hasArgument(const std::string& argname) const {
    const Set<std::string>& uniformNames = _vertexAndPixelShader->uniformNames;
    return uniformNames.contains(argname);
}


bool Shader::ok() const {
    return _vertexAndPixelShader->ok();
}


bool Shader::supportsPixelShaders() {
    return
        GLCaps::supports_GL_ARB_shader_objects() && 
        GLCaps::supports_GL_ARB_shading_language_100() &&
        GLCaps::supports_GL_ARB_fragment_shader();
}


bool Shader::supportsVertexShaders() {
    return
        GLCaps::supports_GL_ARB_shader_objects() && 
        GLCaps::supports_GL_ARB_shading_language_100() &&
        GLCaps::supports_GL_ARB_vertex_shader();
}


bool Shader::supportsGeometryShaders() {
    return
        GLCaps::supports_GL_ARB_shader_objects() && 
        GLCaps::supports_GL_ARB_shading_language_100() &&
        GLCaps::supports_GL_EXT_geometry_shader4();
}


void Shader::beforePrimitive(class RenderDevice* renderDevice) {
    if (_preserveState) {
        renderDevice->pushState();
    }
    debugAssertGLOk();

    if (m_useUniforms) {
        const CoordinateFrame& o2w = renderDevice->objectToWorldMatrix();
        const CoordinateFrame& c2w = renderDevice->cameraToWorldMatrix();

        const Set<std::string>& uniformNames = _vertexAndPixelShader->uniformNames;

        // Bind matrices
        if (uniformNames.contains("g3d_ObjectToWorldMatrix")) {
            args.set("g3d_ObjectToWorldMatrix", o2w);
        }

        if (uniformNames.contains("g3d_CameraToWorldMatrix")) {
            args.set("g3d_CameraToWorldMatrix", c2w);
        }

        if (uniformNames.contains("g3d_ObjectToWorldNormalMatrix")) {
            args.set("g3d_ObjectToWorldNormalMatrix", o2w.rotation);
        }

        if (uniformNames.contains("g3d_WorldToObjectNormalMatrix")) {
            args.set("g3d_WorldToObjectNormalMatrix", o2w.rotation.transpose());
        }

        if (uniformNames.contains("g3d_WorldToObjectMatrix")) {
            args.set("g3d_WorldToObjectMatrix", o2w.inverse());
        }

        if (uniformNames.contains("g3d_WorldToCameraMatrix")) {
            args.set("g3d_WorldToCameraMatrix", c2w.inverse());
        }
    }
    debugAssertGLOk();

    renderDevice->setVertexAndPixelShader(_vertexAndPixelShader, args);
    debugAssertGLOk();
}


void Shader::afterPrimitive(class RenderDevice* renderDevice) {
    if (_preserveState) {
        renderDevice->popState();
    }
}


const std::string& Shader::messages() const {
    return _vertexAndPixelShader->messages();
}

///////////////////////////////////////////////////////////////////////////////

static void replace
(std::string&       code, 
 Set<std::string>&  newNames, 
 const std::string& generatedPrefix,
 const std::string& funcName, 
 const std::string& prefix,
 const std::string& postfix) {

    debugAssertM(generatedPrefix.length() + prefix.length() + postfix.length() == funcName.length() + 1, 
        "Internal error: replacements must contain exactly the same number of characters");

    size_t last = 0;
    while (true) {
        size_t next = code.find(funcName, last);
        if (next == std::string::npos) {
            break;
        }

        size_t nameStart = next + funcName.length();
        size_t nameEnd   = code.find(')', nameStart);

        alwaysAssertM(nameEnd != std::string::npos,
            "Could not find the end of " + funcName + ") expression");

        std::string name = code.substr(nameStart, nameEnd - nameStart);

        std::string decoratedName = generatedPrefix + name;

        std::string replacement = prefix + decoratedName + postfix;
        for (size_t i = 0; i < replacement.size(); ++i) {
            code[i + next] = replacement[i];
        }

        newNames.insert(decoratedName);

        last = nameEnd;
    }
}


bool VertexAndPixelShader::GPUShader::replaceG3DIndex
(std::string&                   code, 
 std::string&                   defineString, 
 const Table<std::string, int>& samplerMappings, 
 bool                           secondPass) {

    // Ensure that defines are only declared once
    Set<std::string> newDefines;

    bool replaced = false;
    std::string funcName = "g3d_Index(";
    std::string prefix   = "(";
    std::string postfix  = "";

    // Prefix contains spaces so that character counts match in the generated code; we replace by overwriting characters exactly.
    replace(code, newDefines, "", "g3d_Index(", "(g3d_Indx_", ")");

    // Add the unique uniforms to the header for the code.
    Set<std::string>::Iterator current = newDefines.begin();
    const Set<std::string>::Iterator end = newDefines.end();
    while (current != end) {
        int i = 0;

        if (secondPass) {
            // Make sure that the sampler was recognized
            debugAssertM(samplerMappings.containsKey(*current), 
                "g3d_Index() used with unknown sampler \"" + *current + "\"");

            i = samplerMappings[*current];
        }

        // Insert the appropriate mapping
        defineString += "#define g3d_Indx_" + *current + " " + format("%d\n", i); 
        replaced = true;

        ++current;
    }

    return replaced;
}

void VertexAndPixelShader::GPUShader::replaceG3DSize
(std::string& code,
 std::string& uniformString) {

    // Ensure that uniforms are only declared once
    Set<std::string> newUniforms;

    std::string funcName = "g3d_sampler2DSize(";
    std::string prefix   = "     (";
    std::string postfix  = ".xy)";

    // Prefix contains spaces so that character counts match in the generated code; we replace by overwriting characters exactly.
    replace(code, newUniforms, "g3d_sz2D_", "g3d_sampler2DSize(", "     (", ".xy)");
    replace(code, newUniforms, "g3d_sz2D_", "g3d_sampler2DInvSize(", "        (", ".zw)");

    // Add the unique uniforms to the header for the code.
    Set<std::string>::Iterator current = newUniforms.begin();
    const Set<std::string>::Iterator end = newUniforms.end();
    while (current != end) {
        uniformString += "uniform vec4 " + *current + "; "; 
        ++current;
    }
}


static int countNewlines(const std::string& s) { 
    int c = 0;
    for (int i = 0; i < (int)s.size(); ++i) {
        if (s[i] == '\n') {
            ++c;
        }
    }
    return c;
}


void VertexAndPixelShader::GPUShader::checkForSupport() {
    switch (_glShaderType) {
    case GL_VERTEX_SHADER_ARB:
        if (! Shader::supportsVertexShaders()) {
            _ok = false;
            _messages = "This graphics card does not support vertex shaders.";
        }
        break;
        
    case GL_FRAGMENT_SHADER_ARB:
        if (! Shader::supportsPixelShaders()) {
            _ok = false;
            _messages = "This graphics card does not support pixel shaders.";
        }
        break;
    }
}


void Shader::processIncludes(const std::string& dir, std::string& code) {
    // Look for #include immediately after a newline.  If it is inside
    // a #IF or a block comment, it will still be processed, but
    // single-line comments will properly disable it.
    bool replaced = false;
    do {
        replaced = false;
        int i;
        if (beginsWith(code, "#include")) {
            i = 0;
        } else {
            i = code.find("\n#include");
        }
        
        if (i != -1) {
            // Remove this line
            int end = code.find("\n", i + 1);
            if (end == -1) {
                end = code.size();
            }
            std::string includeLine = code.substr(i+1, end - i);

            std::string filename;
            TextInput t (TextInput::FROM_STRING, includeLine);
            t.readSymbols("#", "include");
            filename = t.readString();            

            if (! beginsWith(filename, "/")) {
                filename = pathConcat(dir, filename);
            }
            std::string includedFile = readWholeFile(filename);
            if (! endsWith(includedFile, "\n")) {
                includedFile += "\n";
            }

            code = code.substr(0, i + 1) + includedFile + code.substr(end);
            replaced = true;
        }

    } while (replaced);
}


void VertexAndPixelShader::GPUShader::init
(const std::string&	    name,
 const std::string&	    code,
 bool			    _fromFile,
 bool			    debug,	
 GLenum			    glType,
 const std::string&	    type,
 PreprocessorStatus         preprocessor,
 const Table<std::string, int>& samplerMappings,
 bool                       secondPass) {
    
    _name		= name;
    _shaderType		= type;
    _glShaderType	= glType;
    _ok			= true;
    fromFile		= _fromFile;
    _fixedFunction	= (name == "") && (code == "");
    //m_useUniforms       = false;
    
    if (_fixedFunction) {
        return;
    }

    // Directory to use for #includes
    std::string dir = "";
    if (_fromFile) {
        dir = filenamePath(name);
    }

    checkForSupport();
    
    if (fromFile) {
        if (FileSystem::exists(_name)) {
            _code = readWholeFile(_name);
        } else {
            _ok = false;
            _messages = format("Could not load shader file \"%s\".", 
                               _name.c_str());
        }
    } else {
        _code = code;
    }
    
    int shifted = 0;
   
    if ((preprocessor == PREPROCESSOR_ENABLED) && (_code.size() > 0)) {
        // G3D Preprocessor

        // Handle #include directives first, since they may affect
        // what preprocessing is needed in the code. 
        Shader::processIncludes(dir, _code);        

        // Standard uniforms.  We'll add custom ones to this below
        std::string uniformString = 
            STR(
                uniform mat4 g3d_WorldToObjectMatrix;
                uniform mat4 g3d_ObjectToWorldMatrix;
                uniform mat3 g3d_WorldToObjectNormalMatrix;
                uniform mat3 g3d_ObjectToWorldNormalMatrix;
                uniform mat4 g3d_WorldToCameraMatrix;
                uniform mat4 g3d_CameraToWorldMatrix;
                uniform int  g3d_NumLights;
                uniform int  g3d_NumTextures;
                uniform vec4 g3d_ObjectLight0;
                uniform vec4 g3d_WorldLight0;
                );

        
        // See if the program begins with a version pragma
        std::string versionLine;
        if (beginsWith(_code, "#version ")) {
            // Strip off the version line, including the \n. We must keep
            // it in front of everything else. 
            shifted = -1;
            int pos = _code.find("\n") + 1;
            versionLine = _code.substr(0, pos);
            _code = _code.substr(versionLine.length());
        }

        // #defines we'll prepend onto the shader
        std::string defineString;
        
        switch (GLCaps::enumVendor()) {
        case GLCaps::ATI:
            defineString += "#define G3D_ATI\n";
            break;
        case GLCaps::NVIDIA:
            defineString += "#define G3D_NVIDIA\n";
            break;
        case GLCaps::MESA:
            defineString += "#define G3D_MESA\n";
            break;
        default:;
        }

#       ifdef G3D_OSX 
            defineString += "#define G3D_OSX\n";
#       elif defined(G3D_WIN32)
            defineString += "#define G3D_WIN32\n";
#       elif defined(G3D_LINUX)
            defineString += "#define G3D_LINUX\n";
#       elif defined(G3D_FREEBSD)
            defineString += "#define G3D_FREEBSD\n";
#       endif
                
        // Replace g3d_size and g3d_invSize with corresponding magic names
        replaceG3DSize(_code, uniformString);
            
        m_usesG3DIndex = replaceG3DIndex(_code, defineString, samplerMappings, secondPass);
        
        // Correct line numbers
        std::string insertString = defineString + uniformString + "\n";
        shifted += countNewlines(insertString) + 1;
        
        std::string lineDirective = "";
        if (versionLine != "") {
            // We may fix line numbers with a line directive.
            lineDirective = format("#line %d\n", shifted - 2);
            // There's no longer any shifting, since we changed the numbering
            shifted = 0;
        }
        
        _code = versionLine + insertString + lineDirective + _code + "\n";
    }
    
    if (_ok) {
        compile();
    }
    
    if (debug) {
        // Check for compilation errors
        if (! ok()) {
            if (shifted != 0) {
                debugPrintf("\n[Line numbers in the following shader errors are shifted by %d.]\n", shifted);
            }
            logPrintf("Broken shader:\n%s\n", _code.c_str());
            debugPrintf("%s\nin:\n%s\n", messages().c_str(), _code.c_str());
            alwaysAssertM(ok(), messages());
        }
    }
}


void VertexAndPixelShader::GPUShader::compile() {

    GLint compiled = GL_FALSE;
    _glShaderObject = glCreateShaderObjectARB(glShaderType());

    // Compile the shader
    GLint length = _code.length();
    const GLcharARB* codePtr = static_cast<const GLcharARB*>(_code.c_str());
    
    // Show the preprocessed code:
    //printf("\"%s\"\n", _code.c_str());
    glShaderSourceARB(_glShaderObject, 1, &codePtr, &length);
    glCompileShaderARB(_glShaderObject);
    glGetObjectParameterivARB(_glShaderObject, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);

    // Read the result of compilation
    GLint	  maxLength;
    glGetObjectParameterivARB(_glShaderObject, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);
    GLcharARB* pInfoLog = (GLcharARB *)malloc(maxLength * sizeof(GLcharARB));
    glGetInfoLogARB(_glShaderObject, maxLength, &length, pInfoLog);
    
    int c = 0;
    // Copy the result to the output string, prepending the filename
    while (pInfoLog[c] != '\0') {
        _messages += _name;

        // Copy until the next newline or end of string
        std::string line;
        while (pInfoLog[c] != '\n' && pInfoLog[c] != '\r' && pInfoLog[c] != '\0') {
            line += pInfoLog[c];
            ++c;
        }

        if (beginsWith(line, "ERROR: ")) {
            // NVIDIA likes to preface messages with "ERROR: "; strip it off
            line = line.substr(7);

            if (beginsWith(line, "0:")) {
                // Now skip over the line number and wrap it in parentheses.
                line = line.substr(2);
                int i = line.find(':');
                if (i > -1) {
                    // Wrap the line number in parentheses
                    line = "(" + line.substr(0, i) + ")" + line.substr(i);
                } else {
                    // There was no line number, so just add a colon
                    line = ": " + line;
                }
            } else {
                line = ": " + line;
            }
        }

        _messages += line;
        
        if (pInfoLog[c] == '\r' && pInfoLog[c + 1] == '\n') {
            // Windows newline
#           ifdef G3D_WIN32
                _messages += "\r\n";
#           else
                _messages += "\n";
#           endif
            c += 2;
        } else if (pInfoLog[c] == '\r' && pInfoLog[c + 1] != '\n') {
            // Dangling \r; treat it as a newline
            _messages += "\r\n";
            ++c;
        } else if (pInfoLog[c] == '\n') {
            // Newline
#           ifdef G3D_WIN32
                _messages += "\r\n";
#           else
                _messages += "\n";
#           endif
            ++c;
        }
    }
    free(pInfoLog);
    
    _ok = (compiled == GL_TRUE);
}


VertexAndPixelShader::GPUShader::~GPUShader() {
    if (! _fixedFunction) {
        glDeleteObjectARB(_glShaderObject);
    }
}


////////////////////////////////////////////////////////////////////////////////////

void VertexAndPixelShader::reload() {
    debugAssert(false);
    // TODO
}

VertexAndPixelShader::VertexAndPixelShader
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
 PreprocessorStatus  preprocessor) :
    _ok(true) {

    if (! GLCaps::supports_GL_ARB_shader_objects()) {
        _messages = "This graphics card does not support GL_ARB_shader_objects.";
        _ok = false;
        return;
    }

    Table<std::string, int> samplerMappings;

    // While loop used to recompile if samplerMappings is updated
    bool repeat;
    bool secondPass = false;
    do {
        repeat = false;
        vertexShader.init(vsFilename, vsCode, vsFromFile, debug, GL_VERTEX_SHADER_ARB, "Vertex Shader", preprocessor, samplerMappings, secondPass);
        
        geometryShader.init(gsFilename, gsCode, gsFromFile, debug, GL_GEOMETRY_SHADER_ARB, "Geometry Shader", preprocessor, samplerMappings, secondPass);

        pixelShader.init(psFilename, psCode, psFromFile, debug, GL_FRAGMENT_SHADER_ARB, "Pixel Shader", preprocessor, samplerMappings, secondPass);
        
        _vertCompileMessages += vertexShader.messages();
        _messages +=
            std::string("Compiling ") + vertexShader.shaderType() + " " + vsFilename + NEWLINE +
            vertexShader.messages() + NEWLINE + NEWLINE;

        _geomCompileMessages += geometryShader.messages();
        _messages += 
            std::string("Compiling ") + geometryShader.shaderType() + " " + gsFilename + NEWLINE +
            geometryShader.messages() + NEWLINE + NEWLINE;

        _fragCompileMessages += pixelShader.messages();
        _messages += 
            std::string("Compiling ") + pixelShader.shaderType() + " " + psFilename + NEWLINE +
            pixelShader.messages() + NEWLINE + NEWLINE;

        lastTextureUnit = -1;

        if (! vertexShader.ok()) {
            _ok = false;
        }

        if (! geometryShader.ok()) {
            _ok = false;
        }    

        if (! pixelShader.ok()) {
            _ok = false;
        }

        if (_ok) {
            // Create GL object
            _glProgramObject = glCreateProgramObjectARB();

            // Attach vertex and pixel shaders
            if (! vertexShader.fixedFunction()) {
                glAttachObjectARB(_glProgramObject, vertexShader.glShaderObject());
            }

            if (! geometryShader.fixedFunction()) {
                debugAssertM(! vertexShader.fixedFunction(), "Geometry shader requires a vertex shader");
                glAttachObjectARB(_glProgramObject, geometryShader.glShaderObject());

                if (maxGeometryOutputVertices > -1) {
                    // Configure the input and output types.  G3D::Shader simply assumes triangles
                    glProgramParameteriEXT(_glProgramObject, GL_GEOMETRY_INPUT_TYPE_EXT, GL_TRIANGLES);
                    glProgramParameteriEXT(_glProgramObject, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);

                    // Set the maximum output vertices
                    debugAssertM(maxGeometryOutputVertices <= glGetInteger(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT),
                        "maxGeometryOutputVertices exceeds GPU capabilities.");
		            glProgramParameteriEXT(_glProgramObject, GL_GEOMETRY_VERTICES_OUT_EXT, maxGeometryOutputVertices);
                }
            }

            if (! pixelShader.fixedFunction()) {
                glAttachObjectARB(_glProgramObject, pixelShader.glShaderObject());
            }

            // Link
            GLint linked = GL_FALSE;
            glLinkProgramARB(_glProgramObject);

            // Read back messages
            glGetObjectParameterivARB(_glProgramObject, GL_OBJECT_LINK_STATUS_ARB, &linked);
            GLint maxLength = 0, length = 0;
            glGetObjectParameterivARB(_glProgramObject, GL_OBJECT_INFO_LOG_LENGTH_ARB, &maxLength);
            GLcharARB* pInfoLog = (GLcharARB *)malloc(maxLength * sizeof(GLcharARB));
            glGetInfoLogARB(_glProgramObject, maxLength, &length, pInfoLog);

            _messages += std::string("Linking\n") + std::string(pInfoLog) + "\n";
            _linkMessages += std::string(pInfoLog);
            free(pInfoLog);
            _ok = _ok && (linked == GL_TRUE);

            if (debug) {
                alwaysAssertM(_ok, _messages);
            }
        }

        if (_ok) {
            uniformNames.clear();
            computeUniformArray();
            addUniformsFromCode(vertexShader.code());
            addUniformsFromCode(pixelShader.code());
            addUniformsFromCode(geometryShader.code());
            
            // note that the extra uniforms are computed from the original code,
            // not from the code that has the g3d uniforms prepended.
            //vsFromFile ? readWholeFile(vsFilename) : vsCode);
            //addUniformsFromCode(gsFromFile ? readWholeFile(gsFilename) : gsCode);
            //addUniformsFromCode(psFromFile ? readWholeFile(psFilename) : psCode);

            // Add all uniforms to the name list
            for (int i = uniformArray.size() - 1; i >= 0; --i) {
                uniformNames.insert(uniformArray[i].name);
            }
        }

        if (_ok && (pixelShader.usesG3DIndex() 
                    || geometryShader.usesG3DIndex()
                    || vertexShader.usesG3DIndex()) && ! secondPass) {
            // The sampler mappings are needed and have not yet been inserted.  
            // Insert them now and recompile the code.

            // Walk uniforms to construct the sampler mappings.  
            for (int u = 0; u < uniformArray.size(); ++u) {
                const UniformDeclaration& uniform = uniformArray[u];
                if (isSamplerType(uniform.type)){
                    samplerMappings.set(uniform.name, uniform.textureUnit);
                }
            }

            repeat = true;
            secondPass = true;
        }
    } while (repeat);
}


bool VertexAndPixelShader::isSamplerType(GLenum e) {
    return
       (e == GL_SAMPLER_1D_ARB) ||
       (e == GL_UNSIGNED_INT_SAMPLER_1D) ||

       (e == GL_SAMPLER_2D_ARB) ||
       (e == GL_INT_SAMPLER_2D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_2D) ||
       (e == GL_SAMPLER_2D_RECT_ARB) ||

       (e == GL_SAMPLER_3D_ARB) ||
       (e == GL_INT_SAMPLER_3D) ||
       (e == GL_UNSIGNED_INT_SAMPLER_3D) ||

       (e == GL_SAMPLER_CUBE_ARB) ||
       (e == GL_INT_SAMPLER_CUBE) ||
       (e == GL_UNSIGNED_INT_SAMPLER_CUBE) ||

       (e == GL_SAMPLER_1D_SHADOW_ARB) ||

       (e == GL_SAMPLER_2D_SHADOW_ARB) ||
       (e == GL_SAMPLER_2D_RECT_SHADOW_ARB);
}


/** Converts a type name to a GL enum */
static GLenum toGLType(const std::string& s) {
    if (s == "float") {
        return GL_FLOAT;
    } else if (s == "vec2") {
        return GL_FLOAT_VEC2_ARB;
    } else if (s == "vec3") {
        return GL_FLOAT_VEC3_ARB;
    } else if (s == "vec4") {
        return GL_FLOAT_VEC4_ARB;

    } else if (s == "int") {
        return GL_INT;
    } else if (s == "unsigned int") {
        return GL_UNSIGNED_INT;

    } else if (s == "bool") {
        return GL_BOOL_ARB;

    } else if (s == "mat2") {
        return GL_FLOAT_MAT2_ARB;
    } else if (s == "mat3") {
        return GL_FLOAT_MAT3_ARB;
    } else if (s == "mat4") {
        return GL_FLOAT_MAT4_ARB;

    } else if (s == "sampler1D") {
        return GL_SAMPLER_1D_ARB;
    } else if (s == "isampler1D") {
        return GL_INT_SAMPLER_1D_EXT;
    } else if (s == "usampler1D") {
        return GL_UNSIGNED_INT_SAMPLER_1D_EXT;

    } else if (s == "sampler2D") {
        return GL_SAMPLER_2D_ARB;
    } else if (s == "isampler2D") {
        return GL_INT_SAMPLER_2D_EXT;
    } else if (s == "usampler2D") {
        return GL_UNSIGNED_INT_SAMPLER_2D_EXT;

    } else if (s == "sampler3D") {
        return GL_SAMPLER_3D_ARB;
    } else if (s == "isampler3D") {
        return GL_INT_SAMPLER_3D_EXT;
    } else if (s == "usampler3D") {
        return GL_UNSIGNED_INT_SAMPLER_3D_EXT;

    } else if (s == "samplerCube") {
        return GL_SAMPLER_CUBE;
    } else if (s == "isamplerCube") {
        return GL_INT_SAMPLER_CUBE_EXT;
    } else if (s == "usamplerCube") {
        return GL_UNSIGNED_INT_SAMPLER_CUBE_EXT;

    } else if (s == "sampler2DRect") {
        return GL_SAMPLER_2D_RECT_ARB;
    } else if (s == "usampler2DRect") {
        return GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT;
    } else if (s == "sampler2DShadow") {
        return GL_SAMPLER_2D_SHADOW_ARB;
    } else if (s == "sampler2DRectShadow") {
        return GL_SAMPLER_2D_RECT_SHADOW_ARB;
    } else {
        debugAssertM(false, std::string("Unknown type in shader: ") + s);
        return 0;
    }
}


void VertexAndPixelShader::addUniformsFromCode(const std::string& code) {
    TextInput ti(TextInput::FROM_STRING, code);

    while (ti.hasMore()) {
        if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "uniform")) {
            // Read the definition
            ti.readSymbol("uniform");

            // Maybe read "const"
            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "const")) {
                ti.readSymbol("const");
            }

            // Read the type
            std::string variableSymbol = ti.readSymbol();

            // check for "unsigned int"
            if ((variableSymbol == "unsigned") && (ti.peek().string() == "int")) {
                variableSymbol += " " + ti.readSymbol();
            }

            GLenum type = toGLType(variableSymbol);

            // Read the name
            std::string name = ti.readSymbol();
/*
            if ((ti.peek().type() == Token::SYMBOL) && (ti.peek().string() == "[")) {
                ti.readSymbol("[");
                ti.readNumber();
                ti.readSymbol("]");
            }*/

            // Read until the semi-colon
            while (ti.read().string() != ";");


            // See if this variable is already declared.
            bool found = false;

            for (int i = 0; i < uniformArray.size(); ++i) {
                if (uniformArray[i].name == name) {
                    found = true;
                    break;
                }
            } 

            if (! found) {
                // Add the definition
                uniformArray.next();

                uniformArray.last().dummy = true;
                uniformArray.last().location = -1;
                uniformArray.last().name = name;
                uniformArray.last().size = 1;
                uniformArray.last().type = type;

                // Don't allocate texture units for unused variables
                uniformArray.last().textureUnit = -1;
            }

        } else {
            // Consume the token
            ti.read();
        }
    }
}


void VertexAndPixelShader::computeUniformArray() {
    uniformArray.clear();

    // Length of the longest variable name
    GLint maxLength;

    // Number of uniform variables
    GLint uniformCount;

    // On ATI cards, we are required to call glUseProgramObjectARB before glGetUniformLocationARB,
    // so we always bind the program first.

    // First, store the old program.
    GLenum oldProgram = glGetHandleARB(GL_PROGRAM_OBJECT_ARB);
    glUseProgramObjectARB(glProgramObject());

    // Get the number of uniforms, and the length of the longest name.
    glGetObjectParameterivARB(glProgramObject(), GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB, &maxLength);
    glGetObjectParameterivARB(glProgramObject(), GL_OBJECT_ACTIVE_UNIFORMS_ARB, &uniformCount);

    uniformArray.resize(0);

    GLcharARB* name = (GLcharARB *) malloc(maxLength * sizeof(GLcharARB));
    
    // Loop over glGetActiveUniformARB and store the results away.
    for (int i = 0; i < uniformCount; ++i) {

        GLint size;
        GLenum type;

	    glGetActiveUniformARB(glProgramObject(), 
            i, maxLength, NULL, &size, &type, name);

        uniformArray.next().name = name;

        uniformArray.last().location = glGetUniformLocationARB(glProgramObject(), name);
        
        bool isGLBuiltIn = (uniformArray.last().location == -1) || 
            ((strlen(name) > 3) && beginsWith(std::string(name), "gl_"));

        uniformArray.last().dummy = isGLBuiltIn;

        if (! isGLBuiltIn) {
            uniformArray.last().size = size;
            uniformArray.last().type = type;

            if (isSamplerType(type)) {
                ++lastTextureUnit;
                uniformArray.last().textureUnit = lastTextureUnit;
            } else {
                uniformArray.last().textureUnit = -1;
            }
        }
    }

    free(name);

    glUseProgramObjectARB(oldProgram);
}


VertexAndPixelShaderRef VertexAndPixelShader::fromStrings
(const std::string& vs,
 const std::string& ps,
 PreprocessorStatus s,
 bool               debugErrors) {
    return new VertexAndPixelShader(vs, "", false, 
                                    "", "", false, 
                                    ps, "", false, 
                                    -1, debugErrors, s);
}


VertexAndPixelShaderRef VertexAndPixelShader::fromStrings
(
 const std::string& vsName,
 const std::string& vs,
 const std::string& gsName,
 const std::string& gs,
 const std::string& psName,
 const std::string& ps,
 PreprocessorStatus s,
 bool               debugErrors) {

     (void)gs;
     (void)gsName;
    return new VertexAndPixelShader
        (vs, vsName, false, 
         "", "", false,
         ps, psName, false, -1,
         debugErrors, s);
}


VertexAndPixelShaderRef VertexAndPixelShader::fromFiles
(const std::string& vsFilename,
 const std::string& gsFilename,
 const std::string& psFilename,
 int                maxGeometryOutputVertices,
 PreprocessorStatus s,
 bool               debugErrors) {
    
    std::string vs, gs, ps;

    if (vsFilename != "") {
        vs = readWholeFile(vsFilename);
    }

    if (gsFilename != "") {
        gs = readWholeFile(gsFilename);
    }
    
    if (psFilename != "") {
        ps = readWholeFile(psFilename);
    }
    
    return new VertexAndPixelShader
        (vs, vsFilename, vsFilename != "", 
         gs, gsFilename, gsFilename != "", 
         ps, psFilename, psFilename != "",
         maxGeometryOutputVertices,
         debugErrors, s);
}


VertexAndPixelShader::~VertexAndPixelShader() {
    glDeleteObjectARB(_glProgramObject);
}


bool VertexAndPixelShader::fullySupported() {
    return
        GLCaps::supports_GL_ARB_shader_objects() && 
        GLCaps::supports_GL_ARB_shading_language_100() &&
        GLCaps::supports_GL_ARB_fragment_shader() &&
        GLCaps::supports_GL_ARB_vertex_shader();
}


bool VertexAndPixelShader::compatibleTypes(GLenum actual, GLenum formal) {
    return
        (canonicalType(actual) == canonicalType(formal)) ||
        (((actual == GL_FLOAT) || (actual == GL_INT) || (actual == GL_UNSIGNED_INT) || (actual == GL_BOOL)) &&
         ((formal == GL_FLOAT) || (formal == GL_INT) || (formal == GL_UNSIGNED_INT) || (formal == GL_BOOL)));
}


GLenum VertexAndPixelShader::canonicalType(GLenum e) {

    switch (e) {
    case GL_BOOL_VEC2_ARB:
        return GL_FLOAT_VEC2_ARB;

    case GL_BOOL_VEC3_ARB:
        return GL_FLOAT_VEC3_ARB;

    case GL_BOOL_VEC4_ARB:
        return GL_FLOAT_VEC4_ARB;

    case GL_SAMPLER_1D_ARB:
    case GL_INT_SAMPLER_1D_EXT:
    case GL_UNSIGNED_INT_SAMPLER_1D_EXT:
        return GL_TEXTURE_1D;

    case GL_SAMPLER_2D_SHADOW_ARB:
    case GL_SAMPLER_2D_ARB:
    case GL_INT_SAMPLER_2D_EXT:
    case GL_UNSIGNED_INT_SAMPLER_2D_EXT:
        return GL_TEXTURE_2D;
        
    case GL_SAMPLER_CUBE_ARB:
    case GL_INT_SAMPLER_CUBE_EXT:
    case GL_UNSIGNED_INT_SAMPLER_CUBE_EXT:
        return GL_TEXTURE_CUBE_MAP_ARB;
        
    case GL_SAMPLER_2D_RECT_SHADOW_ARB:
    case GL_SAMPLER_2D_RECT_ARB:
        return GL_TEXTURE_RECTANGLE_EXT;

    case GL_SAMPLER_3D_ARB:
    case GL_INT_SAMPLER_3D_EXT:
    case GL_UNSIGNED_INT_SAMPLER_3D_EXT:
        return GL_TEXTURE_3D;

    default:
        // Return the input
        return e;    
    }
}


void VertexAndPixelShader::validateArgList(const ArgList& args) const {
    // Number of distinct variables declared in the shader.
    int numVariables = 0;

    /*
    logPrintf("Declared parameters:\n");
    for (int u = 0; u < uniformArray.size(); ++u) {
        if (uniformArray[u].dummy) {
            logPrintf("  ( %s ) - not used\n", uniformArray[u].name.c_str());
        } else {
            logPrintf("  %s\n", uniformArray[u].name.c_str());
        }
    }

    logPrintf("\nProvided parameters:\n");
    logPrintf("%s\n", args.toString().c_str());
    */

    for (int u = 0; u < uniformArray.size(); ++u) {
        const UniformDeclaration& decl = uniformArray[u];

        // Only count variables that need to be set in code
        if (! decl.dummy) {
            ++numVariables;
        }

        bool declared = false;
        bool performTypeCheck = true;

        if (beginsWith(decl.name, "g3d_sz2D_")) {
            // This is the autogenerated dimensions of a texture.
            std::string textureName  = decl.name.substr(9, decl.name.length() - 9);
            declared = args.contains(textureName);
            performTypeCheck = false;

            if (! declared && ! decl.dummy) {
                throw ArgumentError(
                    format("No value provided for VertexAndPixelShader uniform variable %s.",
                        textureName.c_str()));
            }

        } else {

            // See if this variable was declared
            declared = args.contains(decl.name);
            
            if (! declared && ! decl.dummy && ! beginsWith(decl.name, "_noset_") ) {
                throw ArgumentError(
                    format("No value provided for VertexAndPixelShader uniform variable %s of type %s.",
                        decl.name.c_str(), GLenumToString(decl.type)));
            }
        }

        if (declared && performTypeCheck) {
            const ArgList::Arg& arg = args.argTable[decl.name];

            // check the type
            if (! compatibleTypes(arg.type, decl.type)) {
                std::string v1  = GLenumToString(decl.type);
                std::string v2  = GLenumToString(arg.type);
                std::string v1c = GLenumToString(canonicalType(decl.type));
                std::string v2c = GLenumToString(canonicalType(arg.type));
                throw ArgumentError(
                format("Variable %s was declared as type %s (%s) and the value provided at runtime had type %s (%s).",
                        decl.name.c_str(), v1.c_str(), v1c.c_str(), v2.c_str(), v2.c_str()));
            }
        }
    }

    if (numVariables < args.size()) {

        /*
          logPrintf("numVariables = %d, args.size = %d\n\n", numVariables, args.size());
          // Iterate through formal bindings
          logPrintf("Formals:\n"); 
          for (int u = 0; u < uniformArray.size(); ++u) {
          const UniformDeclaration& decl = uniformArray[u];
          if (! decl.dummy) {
          logPrintf("  %s\n",decl.name.c_str());
          }
          }
          logPrintf("\nActuals:\n");
          {
          Table<std::string, ArgList::Arg>::Iterator        arg = args.argTable.begin();
          const Table<std::string, ArgList::Arg>::Iterator& end = args.argTable.end();
          while (arg != end) {
          logPrintf("  %s\n",arg->key.c_str());
          ++arg;
          }
          }
        */

        // Some variables were unused.  Figure out which they were.
        Table<std::string, ArgList::Arg>::Iterator        arg = args.argTable.begin();
        const Table<std::string, ArgList::Arg>::Iterator& end = args.argTable.end();

        while (arg != end) {
            // See if this arg was in the formal binding list
            const ArgList::Arg& value = arg->value;

            if (! value.optional) {
                bool foundArgument = false;

                for (int u = 0; u < uniformArray.size(); ++u) {
                    if (uniformArray[u].name == arg->key) {
                        foundArgument = true;
                        break;
                    }
                }

                if (! foundArgument) {
                    // Put into a string so that it is visible in the debugger
                    std::string msg = "Extra VertexAndPixelShader uniform variable provided at run time: " +
                         arg->key + ".";
#if 1
// Debugging code for particularly tricky shader errors
debugPrintf("%s \n\n %s\n", vertexShader.code().c_str(), pixelShader.code().c_str());
debugPrintf("Uniform args found in the shader:\n");
for (int u = 0; u < uniformArray.size(); ++u) {
     debugPrintf(" %s\n", uniformArray[u].name.c_str());
}
#endif

                    throw ArgumentError(msg);
                }
            }

            ++arg;
        }
    }
}


void VertexAndPixelShader::bindArgList(RenderDevice* rd, const ArgList& args) const {
    rd->forceVertexAndPixelShaderBind();

#   if defined(G3D_DEBUG)
        validateArgList(args);
#   endif

    // Iterate through the formal parameter list
    for (int u = 0; u < uniformArray.size(); ++u) {
        const UniformDeclaration& decl  = uniformArray[u];
    
        if (decl.dummy) {
            // Don't set this variable; it is unused.
            continue;
        }

        int location = uniformArray[u].location;

        if (beginsWith(decl.name, "g3d_sz2D_")) {
            // This is the autogenerated dimensions of a texture.

            std::string textureName  = decl.name.substr(9, decl.name.length() - 9);

            const ArgList::Arg& value = args.argTable.get(textureName); 

            // Compute the vector of size and inverse size
            float w = value.texture->width();
            float h = value.texture->height();
            Vector4 v(w, h, 1.0f / w, 1.0f / h);

            glUniform4fvARB(location, 1, reinterpret_cast<const float*>(&v));

        } else if (!beginsWith(decl.name, "_noset_")) {

            // Normal user defined variable

            const ArgList::Arg&       value = args.argTable.get(decl.name); 

            // Bind based on the *declared* type
            switch (canonicalType(decl.type)) {
            case GL_TEXTURE_1D:
                debugAssertM(false, "1D texture binding not implemented");
                break;

            case GL_TEXTURE_2D:
            case GL_TEXTURE_CUBE_MAP_ARB:
            case GL_TEXTURE_RECTANGLE_EXT:
            case GL_TEXTURE_3D:
                // Textures are bound as if they were integers.  The
                // value of the texture is the texture unit into which
                // the texture is placed.
                debugAssert(decl.textureUnit >= 0);
                glUniform1iARB(location, decl.textureUnit);
                rd->setTexture(decl.textureUnit, value.texture);
                break;

            case GL_INT:
            case GL_BOOL:
                {
                    int i = (int)value.vector[0][0];
                    if ((value.type == GL_INT) || (value.type == GL_BOOL)) {
                        i = value.intVal;
                    }
                    glUniform1iARB(location, i);
                }
                break;

            case GL_UNSIGNED_INT:
                {
                    unsigned int i = static_cast<unsigned int>(value.vector[0][0]);
                    if ((value.type == GL_INT) || (value.type == GL_BOOL)) {
                        i = static_cast<unsigned int>(value.intVal);
                    }
                    glUniform1uiEXT(location, i);
                }
                break;

            case GL_FLOAT:
                {
                    float f = value.vector[0][0];
                    if ((value.type == GL_INT) || (value.type == GL_BOOL)) {
                        f = value.intVal;
                    }
                    glUniform1fvARB(location, 1, &f);
                }
                break;

            case GL_FLOAT_VEC2_ARB:
                glUniform2fvARB(location, 1,  reinterpret_cast<const float*>(&value.vector[0]));
                break;

            case GL_FLOAT_VEC3_ARB:
                glUniform3fvARB(location, 1,  reinterpret_cast<const float*>(&value.vector[0]));
                break;

            case GL_FLOAT_VEC4_ARB:
                glUniform4fvARB(location, 1,  reinterpret_cast<const float*>(&value.vector[0]));
                break;

            case GL_INT_VEC2_ARB:
            case GL_BOOL_VEC2_ARB:
                glUniform2iARB(location, (int)value.vector[0].x, (int)value.vector[0].y);
                break;

            case GL_INT_VEC3_ARB:
            case GL_BOOL_VEC3_ARB:
                glUniform3iARB(location, (int)value.vector[0].x, (int)value.vector[0].y,
                    (int)value.vector[0].z);
                break;

            case GL_INT_VEC4_ARB:
            case GL_BOOL_VEC4_ARB:
                glUniform4iARB(location, (int)value.vector[0].x, (int)value.vector[1].y,
                    (int)value.vector[2].z, (int)value.vector[3].w);
                break;

            case GL_FLOAT_MAT2_ARB:
                {
                    float m[4];
                    for (int i = 0, c = 0; c < 2; ++c) {
                        for (int r = 0; r < 2; ++r, ++i) {
                            m[i] = value.vector[r][c];
                        }
                    }
                    glUniformMatrix2fvARB(location, 1, GL_FALSE, m);
                }            
                break;

            case GL_FLOAT_MAT3_ARB:
                {
                    float m[9];
                    for (int i = 0, c = 0; c < 3; ++c) {
                        for (int r = 0; r < 3; ++r, ++i) {
                            m[i] = value.vector[r][c];
                        }
                    }
                    glUniformMatrix3fvARB(location, 1, GL_FALSE, m);
                }            
                break;

            case GL_FLOAT_MAT4_ARB:
                {
                    float m[16];
                    for (int i = 0, c = 0; c < 4; ++c) {
                        for (int r = 0; r < 4; ++r, ++i) {
                            m[i] = value.vector[r][c];
                        }
                    }
                    glUniformMatrix4fvARB(location, 1, GL_FALSE, m);
                }
                break;

            default:
                alwaysAssertM(false, format("Unsupported argument type: %s", GLenumToString(decl.type)));
            } // switch on type
        } // if g3d_sz2D_ variable
    }
}


////////////////////////////////////////////////////////////////////////

std::string VertexAndPixelShader::ArgList::toString() const {
    Table<std::string, Arg>::Iterator it = argTable.begin();
    const Table<std::string, Arg>::Iterator& end = argTable.end();

    std::string s = "";
    while (it != end) {
        if (it->value.optional) {
            s += "  ( " + it->key + " ) - optional\n";
        } else {
            s += "  " + it->key + "\n";
        }
        ++it;
    }

    return s;
}


void VertexAndPixelShader::ArgList::set(const ArgList& a) {
    debugAssert(&a != this);
    Table<std::string, Arg>::Iterator it = a.argTable.begin();
    const Table<std::string, Arg>::Iterator& end = a.argTable.end();

    while (it != end) {
        set(it->key, it->value);
        ++it;
    }
}


void VertexAndPixelShader::ArgList::set(const std::string& key, const Arg& value) {
    debugAssert(key != "");

    if (! contains(key)) {
        // New argument
        ++m_size;
    }

    argTable.set(key, value);
}


void VertexAndPixelShader::ArgList::remove(const std::string& key) {
     --m_size;
    argTable.remove(key);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Texture::Ref& val, bool optional) {
    Arg arg;
    debugAssert(val.notNull());
    arg.type    = val->openGLTextureTarget();
    arg.texture = val;
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const CoordinateFrame& val, bool optional) {
    set(var, Matrix4(val), optional);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Matrix4& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_MAT4_ARB;
    for (int r = 0; r < 4; ++r) {
        arg.vector[r] = val.row(r);
    }
    arg.optional = optional;

    set(var, arg);
}

void VertexAndPixelShader::ArgList::set(const std::string& var, const Matrix3& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_MAT3_ARB;
    for (int r = 0; r < 3; ++r) {
        arg.vector[r] = Vector4(val.row(r), 0);
    }
    arg.optional = optional;

    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Vector4& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_VEC4_ARB;
    arg.vector[0] = val;
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Vector3& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_VEC3_ARB;
    arg.vector[0] = Vector4(val, 0);
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Color4& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_VEC4_ARB;
    arg.vector[0] = Vector4(val.r, val.g, val.b, val.a);
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, const Color3& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_VEC3_ARB;
    arg.vector[0] = Vector4(val.r, val.g, val.b, 0);
    arg.optional = optional;
    set(var, arg);
}

void VertexAndPixelShader::ArgList::set(const std::string& var, const Vector2& val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT_VEC2_ARB;
    arg.vector[0] = Vector4(val, 0, 0);
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, double          val, bool optional) {
    set(var, (float)val, optional);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, float          val, bool optional) {
    Arg arg;
    arg.type = GL_FLOAT;
    arg.vector[0] = Vector4(val, 0, 0, 0);
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, int val, bool optional) {
    Arg arg;
    arg.type = GL_INT;
    arg.intVal = val;
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::set(const std::string& var, bool val, bool optional) {
    Arg arg;
    arg.type = GL_BOOL;
    arg.intVal = val;
    arg.optional = optional;
    set(var, arg);
}


void VertexAndPixelShader::ArgList::clear() {
    argTable.clear();
    m_size = 0;
}


}
