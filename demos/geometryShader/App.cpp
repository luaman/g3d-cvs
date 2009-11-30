/** @file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;
    
    settings.window.width       = 960; 
    settings.window.height      = 600;

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


App::App(const GApp::Settings& settings) : GApp(settings) {
#   ifdef G3D_DEBUG
        // Let the debugger catch unhandled exceptions
        catchCommonExceptions = false;
#   endif
}


void App::onInit() {
    ArticulatedModel::PreProcess preprocess;
    preprocess.stripMaterials = true;
    preprocess.xform = Matrix4::scale(3.0f);

    ArticulatedModel::Ref model = 
        ArticulatedModel::fromFile(
            System::findDataFile("knot.ifs"), 
            preprocess);
    model->pose(m_sceneGeometry);

    m_extrudeShader = Shader::fromFiles("extrude.vrt", "extrude.geo", "extrude.pix", 12);
    m_extrudeShader->setPreserveState(false);
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    rd->setColorClearValue(Color3::white() * 0.3f);
    rd->clear();

    // Draw the base geometry as gray with black wireframe
    rd->pushState();
    rd->setColor(Color3::white() * 0.10f);
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        rd->setObjectToWorldMatrix(surface->coordinateFrame());
        surface->sendGeometry(rd);
    }
    rd->popState();

    rd->pushState();
    rd->setColor(Color3::black());
    rd->setPolygonOffset(-0.2f);
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        rd->setObjectToWorldMatrix(surface->coordinateFrame());
        surface->sendGeometry(rd);
    }
    rd->popState();

    // Draw the extruded geometry as colored wireframe
    rd->pushState();
    rd->setRenderMode(RenderDevice::RENDER_WIREFRAME);
    rd->setShader(m_extrudeShader);
    for (int i = 0; i < m_sceneGeometry.size(); ++i) {
        const Surface::Ref& surface = m_sceneGeometry[i];
        m_extrudeShader->args.set("MVP", 
            rd->projectionMatrix() * (rd->cameraToWorldMatrix().inverse() * 
            surface->coordinateFrame()));

        surface->sendGeometry(rd);
    }
    rd->popState();

    Draw::axes(CFrame(), rd);

    drawDebugShapes();
}
