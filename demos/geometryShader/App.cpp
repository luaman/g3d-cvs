/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    settings.window.width       = 960; 
    settings.window.height      = 600;
    settings.window.caption     = "Geometry Shader Demo";

#   ifdef G3D_WIN32
        // On Unix operating systems, icompile automatically copies data files.  
        // On Windows, we just run from the data directory.
        if (fileExists("data-files")) {
            chdir("data-files");
        } else if (fileExists("../demos/geometryShader/data-files")) {
            chdir("../demos/geometryShader/data-files");
        }

#   endif

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {
    ArticulatedModel::Preprocess preprocess;
    preprocess.stripMaterials = true;
    preprocess.xform = Matrix4::scale(3.0f);
    ArticulatedModel::Ref model = ArticulatedModel::fromFile(System::findDataFile("teapot.ifs"), preprocess);
    model->pose(m_sceneGeometry);

    m_extrudeShader = Shader::fromFiles("extrude.vrt", "extrude.geo", "extrude.pix", 12);
    m_extrudeShader->setPreserveState(false);
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    rd->setColorClearValue(Color3::white() * 0.3f);
    rd->clear();

    // Draw the base geometry as gray with black wireframe
    rd->pushState();    
    rd->setPolygonOffset(0.2f);
    rd->setColor(Color3::white() * 0.10f);
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        rd->setObjectToWorldMatrix(surface->coordinateFrame());
        surface->sendGeometry(rd);
    }
    rd->popState();

    rd->pushState();
    rd->setColor(Color3::black());
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        rd->setObjectToWorldMatrix(surface->coordinateFrame());
        surface->sendGeometry(rd);
    }
    rd->popState();

    // Draw the extruded geometry as colored wireframe with "glass" interior
    rd->pushState();
    rd->setBlendFunc(RenderDevice::BLEND_ONE, RenderDevice::BLEND_ONE);
    rd->setDepthWrite(false);
    rd->setShader(m_extrudeShader);
    m_extrudeShader->args.set("intensity", 0.1); 
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        m_extrudeShader->args.set("MVP", 
            rd->invertYMatrix() * rd->projectionMatrix() * (rd->cameraToWorldMatrix().inverse() * 
            surface->coordinateFrame()));

        surface->sendGeometry(rd);
    }
    rd->popState();

    rd->pushState();
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    rd->setCullFace(RenderDevice::CULL_NONE);
    rd->setShader(m_extrudeShader);
    m_extrudeShader->args.set("intensity", 1.0); 
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        m_extrudeShader->args.set("MVP", 
            rd->invertYMatrix() * rd->projectionMatrix() * (rd->cameraToWorldMatrix().inverse() * 
            surface->coordinateFrame()));

        surface->sendGeometry(rd);
    }
    rd->popState();

    drawDebugShapes();
}
