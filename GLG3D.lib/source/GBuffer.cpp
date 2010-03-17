/**
  \file GBuffer.cpp
  \author Morgan McGuire, http://graphics.cs.williams.edu

  TODO: detect surfaces that need alpha testing and compile a special
  shader for them.  Do this for shadow maps as well.

  TODO: packed z
 */
#include "GLG3D/GBuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/SuperBSDF.h"
#include "G3D/fileutils.h"
#include "G3D/FileSystem.h"

namespace G3D {

GBuffer::Indices::Indices(const GBuffer::Specification& spec) 
    : L(-1), s(-1), t(-1), e(-1), wsN(-1), csN(-1), csF(-1), wsF(-1), z(-1), c(-1), csP(-1), wsP(-1) {

    int i = 0;

    if (spec.lambertian) {    L = i;    ++i;   }
    if (spec.specular) {      s = i;    ++i;   }
    if (spec.transmissive) {  t = i;    ++i;   }
    if (spec.emissive) {      e = i;    ++i;   }
    if (spec.csNormal) {      csN = i;  ++i;   }
    if (spec.wsNormal) {      wsN = i;  ++i;   }
    if (spec.csFaceNormal) {  csF = i;  ++i;   }
    if (spec.wsFaceNormal) {  wsF = i;  ++i;   }
    if (spec.packedDepth) {   z = i;    ++i;   }
    if (spec.custom) {        c = i;    ++i;   }
    numPrimaryAttach = i;

    int j = 0;
    if (spec.wsPosition) {   wsP = j;    ++j;   }
    if (spec.csPosition) {   csP = j;    ++j;   }

    numPositionAttach = j;

    int maxAttach = glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT);
    alwaysAssertM(max(numPrimaryAttach, numPositionAttach) >= i, 
        format("GBuffer requires a GL_MAX_COLOR_ATTACHMENTS value >= %d for this specification "
               "(%d color attachments on this GPU)", i, max(numPrimaryAttach, numPositionAttach)));
}

std::string GBuffer::Indices::computeDefines() const {
    return
        format("#define LAMBERTIAN_INDEX (%d)\n"
               "#define SPECULAR_INDEX (%d)\n"
               "#define TRANSMISSIVE_INDEX (%d)\n"
               "#define EMISSIVE_INDEX (%d)\n"
               "#define CS_NORMAL_INDEX (%d)\n"
               "#define WS_NORMAL_INDEX (%d)\n"
               "#define CS_FACE_NORMAL_INDEX (%d)\n"
               "#define WS_FACE_NORMAL_INDEX (%d)\n"
               "#define PACKED_DEPTH_INDEX (%d)\n" 
               "#define CUSTOM_INDEX (%d)\n",
               L, s, t, e, csN, wsN, csF, wsF, z, c);
}


std::string GBuffer::Indices::computePositionDefines() const {
    return 
        format("#define WS_POSITION_INDEX (%d)\n"
               "#define CS_POSITION_INDEX (%d)\n", wsP, csP);
}


GBuffer::Ref GBuffer::create
(const std::string&   name,
 const Specification& specification) {
    return new GBuffer(name, specification);
}


bool GBuffer::supported() {
    return Shader::supportsVertexShaders() && Shader::supportsPixelShaders();
}


Shader::Ref GBuffer::getShader(const GBuffer::Specification& specification, const GBuffer::Indices& indices, const Material::Ref& material) {
    typedef Table<Material::Ref, Shader::Ref, Material::SimilarHashCode, Material::SimilarTo> ShaderCache;
    typedef Table<Specification, ShaderCache, Specification::Similar, Specification::Similar> CacheTable;

    static CacheTable cacheTable;
    
    ShaderCache& cache  = cacheTable.getCreate(specification);
    Shader::Ref& shader = cache.getCreate(material);

    if (shader.isNull()) {

        const std::string& path = FilePath::parentPath(System::findDataFile("SS_NonShadowedPass.vrt"));
        const std::string& currentPath = FileSystem::currentDirectory();

        if (path != currentPath) {
            // We have to be in the right directory for #includes inside the shaders to work correctly.
            chdir(path.c_str());
        }

        const std::string vertexCode = readWholeFile("SS_NonShadowedPass.vrt");
        const std::string pixelCode  = readWholeFile("SS_GBuffer.pix");
        std::string s;
        material->computeDefines(s);
        std::string prefix = s + indices.computeDefines();
        shader = Shader::fromStrings(prefix + vertexCode, prefix + pixelCode);
        shader->setPreserveState(false);

        if (path != currentPath) {
            chdir(currentPath.c_str());
        }
    }

    return shader;
}


GBuffer::GBuffer(const std::string& name, const Specification& specification) :
    m_name(name),
    m_specification(specification),
    m_indices(specification) {

    alwaysAssertM(supported(), "GBuffer requires pixel and vertex shaders.");

    m_positionShader = makePositionShader();

}


Shader::Ref GBuffer::makePositionShader() {
    // Compute the position shader
    typedef Table<std::string, Shader::Ref> ShaderCache;

    static ShaderCache shaderCache;
    
    const std::string& macros = m_indices.computePositionDefines();

    Shader::Ref& shader = shaderCache.getCreate(macros);

    if (shader.isNull()) {

        const std::string& path = FilePath::parentPath(System::findDataFile("SS_NonShadowedPass.vrt"));
        const std::string& currentPath = FileSystem::currentDirectory();

        if (path != currentPath) {
            // We have to be in the right directory for #includes inside the shaders to work correctly.
            chdir(path.c_str());
        }

        const std::string& vertexCode = readWholeFile("SS_NonShadowedPass.vrt");
        const std::string& pixelCode  = readWholeFile("SS_GBufferPosition.pix");
        shader = Shader::fromStrings(macros + vertexCode, macros + pixelCode);
        shader->setPreserveState(false);

        if (path != currentPath) {
            chdir(currentPath.c_str());
        }
    }

    return shader;
}


GBuffer::~GBuffer() {
}


int GBuffer::width() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->width();
    } else if (m_positionFramebuffer.notNull()) {
        return m_positionFramebuffer->width();
    } else {
        return 0;
    }
}


int GBuffer::height() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->height();
    } else if (m_positionFramebuffer.notNull()) {
        return m_positionFramebuffer->height();
    } else {
        return 0;
    }
}


Rect2D GBuffer::rect2DBounds() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->rect2DBounds();
    } else if (m_positionFramebuffer.notNull()) {
        return m_positionFramebuffer->rect2DBounds();
    } else {
        return Rect2D::xywh(0, 0, 0, 0);
    }
}


void GBuffer::resize(int w, int h) {
    if (w == width() && h == height()) {
        // Already allocated
        return;
    }

    if (m_framebuffer.isNull()) {
        if (m_indices.numPrimaryAttach > 0) {
            m_framebuffer = Framebuffer::create(m_name);
        }
    } else {
        m_framebuffer->clear();
    }

    if (m_indices.numPositionAttach > 0) {
        if (m_positionFramebuffer.isNull()) {
            m_positionFramebuffer = Framebuffer::create(m_name + " position");
        } else {
            m_positionFramebuffer->clear();
        }
    }

    // Discard old textures, allowing them to be garbage collected
    m_lambertian   = NULL;
    m_specular     = NULL;
    m_transmissive = NULL;
    m_wsPosition   = NULL;
    m_csPosition   = NULL;
    m_emissive     = NULL;
    m_packedDepth  = NULL;
    m_csNormal     = NULL;
    m_wsNormal     = NULL;
    m_csFaceNormal = NULL;
    m_wsFaceNormal = NULL;
    m_depth        = NULL;

    Texture::Settings settings = Texture::Settings::buffer();

#   define BUFFER(buf, ind)\
    if (m_indices.ind >= 0) {\
        m_##buf   = Texture::createEmpty(#buf, w, h, m_specification.format, Texture::DIM_2D_NPOT, settings);\
        m_framebuffer->set(Framebuffer::AttachmentPoint(Framebuffer::COLOR0 + m_indices.ind), m_##buf); \
    }

    BUFFER(lambertian, L);
    BUFFER(specular, s);
    BUFFER(transmissive, t);
    BUFFER(emissive, e);
    BUFFER(csNormal, csN);
    BUFFER(wsNormal, wsN);
    BUFFER(csFaceNormal, csF);
    BUFFER(wsFaceNormal, wsF);
    BUFFER(packedDepth, z);

#   undef BUFFER

    m_depth = Texture::createEmpty("Depth", w, h, m_specification.depthFormat, Texture::DIM_2D_NPOT, settings);
    m_framebuffer->set(Framebuffer::DEPTH, m_depth);

    if (m_specification.wsPosition || m_specification.csPosition) {

        if (m_indices.wsP >= 0) {
            m_wsPosition     = Texture::createEmpty("wsPosition", w, h, m_specification.positionFormat, Texture::DIM_2D_NPOT, settings);
            m_positionFramebuffer->set(Framebuffer::AttachmentPoint(Framebuffer::COLOR0 + m_indices.wsP), m_wsPosition);
        }
        if (m_indices.csP >= 0) {
            m_csPosition     = Texture::createEmpty("csPosition", w, h, m_specification.positionFormat, Texture::DIM_2D_NPOT, settings);
            m_positionFramebuffer->set(Framebuffer::AttachmentPoint(Framebuffer::COLOR0 + m_indices.csP), m_csPosition);
        }

        m_positionFramebuffer->set(Framebuffer::DEPTH, m_depth);
    }
}


void GBuffer::compute
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<Surface::Ref>&     modelArray) const {

    m_camera = camera;
    Array<SuperSurface::Ref>  genericArray;
    Array<Surface::Ref>       nonGenericArray;

    for (int m = 0; m < modelArray.size(); ++m) {
        const SuperSurface::Ref& model = 
            modelArray[m].downcast<SuperSurface>();

        if (model.notNull()) {
            genericArray.append(model);
        } else {
            nonGenericArray.append(modelArray[m]);
        }
    }
    
    SuperSurface::sortFrontToBack(genericArray, camera.coordinateFrame().lookVector());

    if (m_indices.numPrimaryAttach > 0) {
        rd->pushState(m_framebuffer);
        {
            rd->setProjectionAndCameraMatrix(camera);
            
            rd->setColorClearValue(Color4::zero());
            
            // Only clear depth if we're allowed to render to the depth
            // buffer.  This ensures that the m_eyeBuffer's depth does not
            // get wiped after the early z.
            rd->clear(true, rd->depthWrite(), false);
            
            computeArray(rd, genericArray);
        }
        rd->popState();
    }

    if (m_indices.numPositionAttach > 0) {
        rd->pushState(m_positionFramebuffer);
        {
            // Only clear depth if it was not previously written 
            bool d = rd->depthWrite() && (m_indices.numPrimaryAttach == 0);
            rd->setColorClearValue(Color4::zero());
            rd->clear(true, d, false);
            rd->setDepthWrite(d);
            rd->setDepthTest(RenderDevice::DEPTH_LEQUAL);

            rd->setProjectionAndCameraMatrix(camera);
            
            // Render front faces
            rd->setShader(m_positionShader);
            rd->beginIndexedPrimitives();
            for (int i = 0; i < genericArray.size(); ++i) {
                const SuperSurface::Ref& model = genericArray[i];
                const SuperSurface::GPUGeom::Ref& geom = model->gpuGeom();
                const SuperBSDF::Ref& bsdf = geom->material->bsdf();

                m_positionShader->args.set("lambertianConstant", bsdf->lambertian().constant());
                m_positionShader->args.set("lambertianMap", Texture::blackIfNull(bsdf->lambertian().texture()));

                rd->setObjectToWorldMatrix(model->coordinateFrame());
                rd->setVARs(geom->vertex, geom->normal, geom->texCoord0, geom->packedTangent);
                rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);
            
                if (geom->twoSided) {
                    // Configure for backfaces
                    rd->setCullFace(RenderDevice::CULL_FRONT);
                    
                    // Render backfaces
                    rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);
                    
                    // Restore backface state
                    rd->setCullFace(RenderDevice::CULL_BACK);
                }
            } // for each
            rd->endIndexedPrimitives();
        }
        rd->popState();
    }
}


void GBuffer::computeArray
(RenderDevice* rd, 
 const Array<SuperSurface::Ref>& genericArray) const {
   
    rd->beginIndexedPrimitives();
    for (int m = 0; m < genericArray.size(); ++m) {
        compute(rd, genericArray[m]);
    }
    rd->endIndexedPrimitives();
}




void GBuffer::compute
(RenderDevice* rd,
 const SuperSurface::Ref& model) const {

    debugAssertGLOk();

    // Configure the shader with the coefficients
    const SuperSurface::GPUGeom::Ref& geom = model->gpuGeom();

    const SuperBSDF::Ref& bsdf = geom->material->bsdf();
    Shader::Ref shader = getShader(m_specification, m_indices, geom->material);
    geom->material->configure(shader->args);

    if (m_indices.t >= 0) {
        shader->args.set("transmissiveMap",      Texture::whiteIfNull(bsdf->transmissive().texture()));
        shader->args.set("transmissiveConstant", bsdf->transmissive().constant());
        shader->args.set("eta",                  max(0.01f, bsdf->etaTransmit()));
    }

    shader->args.set("backside", 1.0f);

    // Render front faces
    rd->setShader(shader);
    rd->setObjectToWorldMatrix(model->coordinateFrame());
    rd->setVARs(geom->vertex, geom->normal, geom->texCoord0, geom->packedTangent);
    rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);
    
    if (geom->twoSided) {
        // Configure for backfaces
        rd->setCullFace(RenderDevice::CULL_FRONT);
        shader->args.set("backside", -1.0f);

        // Render backfaces
        rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);

        // Restore backface state
        rd->setCullFace(RenderDevice::CULL_BACK);
        shader->args.set("backside", 1.0f);
    }
}

} // namespace G3D
