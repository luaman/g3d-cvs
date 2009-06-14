/**
  @file GBuffer.cpp
  @author Morgan McGuire, morgan@cs.williams.edu
 */
#include "GLG3D/GBuffer.h"
#include "GLG3D/RenderDevice.h"
#include "GLG3D/UberBSDF.h"

namespace G3D {

GBuffer::Ref GBuffer::create
(const std::string& name,
 const ImageFormat* depthFormat) {
    return new GBuffer(name, depthFormat);
}


bool GBuffer::supported() {
    int maxAttach = glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT);
    return (maxAttach >= 5) && 
        Shader::supportsVertexShaders() && 
        Shader::supportsPixelShaders();
}


GBuffer::GBuffer(const std::string& name, const ImageFormat* fmt) : 
    m_name(name),
    m_depthFormat(fmt) {

    int maxAttach = glGetInteger(GL_MAX_COLOR_ATTACHMENTS_EXT);
    alwaysAssertM(maxAttach >= 5, 
        format("GBuffer requires a GL_MAX_COLOR_ATTACHMENTS value >= 5 "
               "(%d color attachments on this GPU)", maxAttach));

    alwaysAssertM(Shader::supportsVertexShaders() && Shader::supportsPixelShaders(),
        "GBuffer requires pixel and vertex shaders.");

    // Keep a cached copy of the shader
    static WeakReferenceCountedPointer<Shader> globalShader;

    m_shader = globalShader.createStrongPtr();

    if (m_shader.isNull()) {
        m_shader = Shader::fromFiles
            (System::findDataFile("GBuffer.vrt"), 
             System::findDataFile("GBuffer.pix"));
        m_shader->setPreserveState(false);
        m_shader->args.set("backside", 1.0f);
        globalShader = m_shader;
    }
}


GBuffer::~GBuffer() {
}


int GBuffer::width() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->width();
    } else {
        return 0;
    }
}


int GBuffer::height() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->height();
    } else {
        return 0;
    }
}


Rect2D GBuffer::rect2DBounds() const {
    if (m_framebuffer.notNull()) {
        return m_framebuffer->rect2DBounds();
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
        m_framebuffer = Framebuffer::create(m_name);
    } else {
        m_framebuffer->clear();
    }

    // Discard old textures, allowing them to be garbage collected
    m_lambertian   = NULL;
    m_specular     = NULL;
    m_transmissive = NULL;
    m_position     = NULL;
    m_normal       = NULL;
    m_depth        = NULL;

    const ImageFormat* fmt = ImageFormat::RGBA16F();

    Texture::Settings settings = Texture::Settings::video();
    settings.interpolateMode = Texture::NEAREST_NO_MIPMAP;
    settings.wrapMode = WrapMode::CLAMP;

    // All color buffers have to be in the same format
    m_lambertian   = Texture::createEmpty("Lambertian",   w, h, fmt, Texture::DIM_2D_NPOT, settings);
    m_specular     = Texture::createEmpty("Specular",     w, h, fmt, Texture::DIM_2D_NPOT, settings);
    m_transmissive = Texture::createEmpty("Transmissive", w, h, fmt, Texture::DIM_2D_NPOT, settings);
    m_position     = Texture::createEmpty("Position",     w, h, fmt, Texture::DIM_2D_NPOT, settings);
    m_normal       = Texture::createEmpty("Normal",       w, h, fmt, Texture::DIM_2D_NPOT, settings);
    m_depth        = Texture::createEmpty("Depth",        w, h, m_depthFormat, Texture::DIM_2D_NPOT, settings);

    // Lambertian must be attachmnt 0 because alpha test is keyed off of it.
    m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT0, m_lambertian);
    m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT1, m_specular);
    m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT2, m_transmissive);
    m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT3, m_position);
    m_framebuffer->set(Framebuffer::COLOR_ATTACHMENT4, m_normal);

    m_framebuffer->set(Framebuffer::DEPTH_ATTACHMENT, m_depth);
}


void GBuffer::compute
(RenderDevice*                  rd, 
 const GCamera&                 camera,
 const Array<Surface::Ref>&  modelArray) const {

    Array<GenericSurface::Ref> genericArray;
    Array<Surface::Ref> nonGenericArray;

    for (int m = 0; m < modelArray.size(); ++m) {
        const GenericSurface::Ref& model = 
            modelArray[m].downcast<GenericSurface>();

        if (model.notNull()) {
            genericArray.append(model);
        } else {
            nonGenericArray.append(modelArray[m]);
        }
    }


    rd->pushState(m_framebuffer);
    {
        rd->setProjectionAndCameraMatrix(camera);
    
        rd->setColorClearValue(Color4::zero());
        
        // Only clear depth if we're allowed to render to the depth
        // buffer.  This ensures that the m_eyeBuffer's depth does not
        // get wiped after the early z.
        rd->clear(true, rd->depthWrite(), false);
        
        rd->setAlphaTest(RenderDevice::ALPHA_GREATER, 0.5f);

        if (nonGenericArray.size() > 0) {
            computeNonGenericArray(rd, nonGenericArray);
        }

        computeGenericArray(rd, genericArray);
    }
    rd->popState();
}


void GBuffer::computeGenericArray
(RenderDevice* rd, 
 const Array<GenericSurface::Ref>& genericArray) const {
   
    rd->setShader(m_shader);
    rd->beginIndexedPrimitives();
    for (int m = 0; m < genericArray.size(); ++m) {
        computeGeneric(rd, genericArray[m]);
    }
    rd->endIndexedPrimitives(); 
}


void GBuffer::computeNonGenericArray
(RenderDevice* rd,
 const Array<Surface::Ref>& nonGenericArray) const {
    debugAssertM(false, "Not implemented in this G3D build");
    return;
    rd->pushState();
    {
        // TODO: Disable BSDF coefficients
        
        // Render once to get position and normal information
        for (int m = 0; m < nonGenericArray.size(); ++m) {
            computeNonGeneric(rd, nonGenericArray[m]);
        }

        // TODO: Disable everything except lambertian, configure
        // ambient only.

        // Render again to generate lambertian
        for (int m = 0; m < nonGenericArray.size(); ++m) {
            computeNonGeneric(rd, nonGenericArray[m]);
        }
    }
    rd->popState();
}


void GBuffer::computeNonGeneric
(RenderDevice* rd,
 const Surface::Ref& model) const {
    
}


void GBuffer::computeGeneric
(RenderDevice* rd,
 const GenericSurface::Ref& model) const {

    debugAssertGLOk();

    // Configure the shader with the coefficients
    const GenericSurface::GPUGeom::Ref& geom = model->gpuGeom();

    const UberBSDF::Ref& bsdf = geom->material->bsdf();
    m_shader->args.set("lambertianMap",      Texture::whiteIfNull(bsdf->lambertian().texture()));
    m_shader->args.set("lambertianConstant", bsdf->lambertian().constant().rgb());
    m_shader->args.set("specularMap",        Texture::whiteIfNull(bsdf->specular().texture()));
    m_shader->args.set("specularConstant",   bsdf->specular().constant());
    m_shader->args.set("transmissiveMap",    Texture::whiteIfNull(bsdf->transmissive().texture()));
    m_shader->args.set("transmissiveConstant", bsdf->transmissive().constant());

    // Ensure that the GPU never sees a divide by zero, even in a
    // speculative branch that won't be taken
    m_shader->args.set("eta",                max(1.0f, bsdf->etaTransmit()));

    // Render front faces
    rd->setObjectToWorldMatrix(model->coordinateFrame());
    rd->setVARs(geom->vertex, geom->normal, geom->texCoord0, geom->tangent);
    rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);
    
    if (geom->twoSided) {
        // Configure for backfaces
        rd->setCullFace(RenderDevice::CULL_FRONT);
        m_shader->args.set("backside", -1.0f);

        // Render backfaces
        rd->sendIndices((RenderDevice::Primitive)geom->primitive, geom->index);

        // Restore backface state
        rd->setCullFace(RenderDevice::CULL_BACK);
        m_shader->args.set("backside", 1.0f);
    }
}

} // namespace G3D
