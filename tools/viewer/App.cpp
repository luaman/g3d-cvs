/**
 @file App.cpp
 
 App that allows viewing of 3D assets
 
 @author Eric Muller 09edm@williams.edu, Dan Fast 10dpf@williams.edu, Katie Creel 10kac_2@williams.edu
 
 @created 2007-05-31
 @edited  2007-07-08
 */
#include "App.h"
#include "IFSViewer.h"
#include "ArticulatedViewer.h"
#include "TextureViewer.h"
#include "FontViewer.h"
#include "SkyViewer.h"
#include "BSPViewer.h"
#include "MD2Viewer.h"
#include "GUIViewer.h"
#include "EmptyViewer.h"
#include "VideoViewer.h"


App::App(const GApp::Settings& settings, const std::string& file) :
    GApp(settings),
    viewer(NULL),
    filename(file) {

    shadowMap = ShadowMap::create();
    shadowMap->setPolygonOffset(0.0f);
    setDesiredFrameRate(60);
}


void App::onInit() {
    showRenderingStats = false;
    window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");

    sky = Sky::fromFile(System::findDataFile("sky"));
    skyParameters = SkyParameters( G3D::toSeconds(11, 00, 00, AM) );
    lighting = Lighting::fromSky( sky, skyParameters, Color3::white() );
    lighting->ambientTop = Color3::white() * 0.3f;
    lighting->ambientBottom = Color3::white() * 0.1f;
	
    colorClear = Color3::white();
    //modelController = ThirdPersonManipulator::create();

    setViewer(filename);
}


void App::onCleanup() {
    delete viewer;
    viewer = NULL;
}


bool App::onEvent(const GEvent& e) {
    if (GApp::onEvent(e)) {
        return true;
    }

    switch (e.type) {
    case GEventType::FILE_DROP:
        {
            Array<std::string> fileArray;
            window()->getDroppedFilenames(fileArray);
            setViewer(fileArray[0]);
            return true;
        }
        
    default:;
    }
    
    return false;
}


void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    // Make the camera spin when the debug controller is not active
    if (false) {
        static float angle = 0;
        angle += rdt;
        float radius = 5.5;
        defaultCamera.setPosition(Vector3(cos(angle), 0, sin(angle)) * radius);
        defaultCamera.lookAt(Vector3(0,0,0));
    }

    // let viewer sim with time step if needed
    if (viewer != NULL) {
        viewer->onSimulation(rdt, sdt, idt);
    }
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {

    Lighting::Ref localLighting = lighting;
    rd->setProjectionAndCameraMatrix(defaultCamera);

    rd->setColorClearValue(colorClear);
    rd->clear(true, true, true);

    rd->enableLighting();
        rd->setAmbientLightColor(localLighting->ambientTop);
        if (localLighting->ambientBottom != localLighting->ambientTop) {
            rd->setLight(iMin(7, localLighting->lightArray.size()) + 1, GLight::directional(-Vector3::unitY(), 
                localLighting->ambientBottom - localLighting->ambientTop, false)); 
        }
            
        // Lights
        for (int L = 0; L < iMin(7, localLighting->lightArray.size()); ++L) {
            rd->setLight(L, localLighting->lightArray[L]);
        }

        // Render the file that is currently being viewed
        if (viewer != NULL) {
            viewer->onGraphics(rd, this, localLighting);
        }
        
        for (int i = 0; i < posed3D.size(); ++i) {
            posed3D[i]->render(rd);
        }
	rd->disableLighting();

    Surface2D::sortAndRender(rd, posed2D);
}

static bool allCubeMapFacesExist(const std::string& base, const std::string& ext) {
    for (int i = 0; i < 4; ++i) {
        Texture::CubeMapConvention c = (Texture::CubeMapConvention)i;

        const Texture::CubeMapInfo& info = Texture::cubeMapInfo(c);

        bool success = true;
        for (int f = 0; f < 6; ++f) {
            const char* s = info.face[f].suffix;
            if (! fileExists(base + s + "." + ext) &&
                ! fileExists(base + toUpper(s) + "." + ext)) {
                success = false;
                break;
            }
        }
        if (success) {
            return true;
        }
    }
    return false;
}

/**
 @param ext Does not contain the period, must be an image extension
 */
static bool isCubeMapFace(const std::string& _base, const std::string& _ext) {
    std::string base = toLower(_base);
    std::string ext = toLower(_ext);
    for (int i = 0; i < 4; ++i) {
        Texture::CubeMapConvention c = (Texture::CubeMapConvention)i;

        const Texture::CubeMapInfo& info = Texture::cubeMapInfo(c);
        for (int f = 0; f < 6; ++f) {
            if (endsWith(base, std::string(info.face[f].suffix) + "." + ext)) {
                return true;
            }
        }
    }
    return false;
}


void App::setViewer(const std::string& newFilename) {
    filename = newFilename;
    defaultCamera.setCoordinateFrame(CoordinateFrame(Vector3(0,0,5)));
    defaultController->setFrame(CoordinateFrame(Vector3(0,0,5)));
    //modelController->setFrame(CoordinateFrame(Matrix3::fromAxisAngle(Vector3(0,1,0), toRadians(180))));
    delete viewer;
    viewer = NULL;
    shadowMap->setSize(0);
    
    std::string ext = toLower(filenameExt(filename));
    
    if ((ext == "3ds") ||
        (ext == "ifs") ||
        (ext == "ply2") ||
        (ext == "off")) {
        
        shadowMap->setSize(2048);
        
        viewer = new ArticulatedViewer();
        
    } else if (Texture::isSupportedImage(filename)) {
        
        // Images can be either a Texture or a Sky: figure out which we have
	
        // If the image represents a skybox, there will be 5 others just like it,
        // corresponding to the rest of the cube
        if (isCubeMapFace(filename, ext)) {
            
            std::string skyRoot = filename.substr(0, filename.length() - 6);
            
            if (allCubeMapFacesExist(skyRoot, ext)) {
                viewer = new SkyViewer();
            } else {
                viewer = new TextureViewer();
            }
        } else {
            viewer = new TextureViewer();
        }
    } else if (ext == "fnt") {
        
        viewer = new FontViewer(debugFont);
        
    } else if (ext == "bsp") {
        
        viewer = new BSPViewer();
        
    } else if (ext == "md2") {
        
        viewer = new MD2Viewer();
        
    } else if (ext == "skn") {
        
        viewer = new GUIViewer(this);
        
    } else if (ext == "pk3") {
        // Something in Quake format - figure out what we should load
        Array <std::string> files;
        bool set = false;
        
        // First, try for a .bsp map
        std::string search = filename + "/maps/*";
        getFiles(search, files, true);

        if (files.length() == 0) {
            // Try another case
            search = filename + "/Maps/*";
            getFiles(search, files, true);
        }
        if (files.length() == 0) {
            // Try another case
            search = filename + "/MAPS/*";
            getFiles(search, files, true);
        }

        for (int t = 0; t < files.length(); ++t) {
            
            if (filenameExt(files[t]) == "bsp") {
                
                filename = files[t];
                viewer = new BSPViewer();
                set = true;
            }
        }
        if (!set) {
            viewer = new EmptyViewer();
        }

    } else if (ext == "avi" || ext == "wmv" || ext == "mp4" || ext == "asf" || 
               (ext == "mov") || (ext == "dv") || (ext == "qt") || (ext == "asf") ||
               (ext == "mpg")) {
        viewer = new VideoViewer();

    } else {
        
        viewer = new EmptyViewer();
	
    }

    if (viewer != NULL) {
        viewer->onInit(filename);
    }
    
    window()->setCaption(filenameBaseExt(filename) + " - G3D Viewer");
}
