/**
  @file gfxmeter/App.h

  @author Morgan McGuire, morgan@cs.williams.edu
 */
#ifndef APP_H
#define APP_H

#include "G3D/G3DAll.h"

class MD2 {
public:
    CoordinateFrame     cframe;
    MD2Model::Ref       model;
    MD2Model::Pose      pose;
    GMaterial           material;

    void load(const std::string& filename) {
        model = MD2Model::fromFile(filename + ".md2");

	    Texture::PreProcess preProcess;
	    preProcess.brighten = 2.0;
        material.texture.append(Texture::fromFile(filename + ".pcx", 
            ImageFormat::AUTO(), Texture::DIM_2D, Texture::Settings(), preProcess));
    }

    void render(RenderDevice* rd) {
        model->pose(cframe, pose, material)->render(rd);
    }

    void renderShadow(RenderDevice* rd) {
        CoordinateFrame cframe2 = cframe;
        cframe2.rotation.setColumn(1, Vector3::zero());
        cframe2.translation.y -= 1.7f;
        rd->setColor(Color3(.9f, .9f, 1));

        Surface::Ref m = model->pose(cframe2, pose);

        // Intentionally render a lot of shadows to gauge rendering performance
        for (int i = 0; i < 20; ++i) {
            m->render(rd);
        }
    }

    void doSimulation(GameTime dt) {
		GameTime t = System::time();
		debugAssert(t > 0);
        pose = MD2Model::Pose(MD2Model::STAND, t);
        /*
        pose.doSimulation(dt,
            false, false, false, 
            true,  false, false, 
            false, false, false, 
            false, false, false,
            false, false, false,
            false);
            */
    }
};

class App : public GApp {
public:
    enum Popup {NONE, PERFORMANCE};

private:
    Popup               popup;

    Rect2D              performanceButton;
    Rect2D              closeButton;

    MD2                 knight;
    MD2                 ogre;

public:
    SkyRef                  sky;

    Texture::Ref            cardLogo;
    Texture::Ref            chipLogo;
    std::string             chipSpeed;
    Texture::Ref            osLogo;

    GFontRef                titleFont;
    GFontRef                reportFont;

    std::string             combineShader;
    std::string             asmShader;
    std::string             glslShader;

    int                     featureRating;
    float                   performanceRating;
    int                     bugCount;

    struct VertexPerformance {
        int                 numTris;
        float               beginEndFPS[2];
        float               drawElementsRAMFPS[2]; 
        float               drawElementsVBOFPS[2]; 
        float               drawElementsVBO16FPS[2]; 

        /** glInterleavedArrays interleaved */
        float               drawElementsVBOIFPS[2];
        
        /** Manually interleaved */
        float               drawElementsVBOIMFPS[2];

        /** Turn shading off and just slam vertices through */
        float               drawElementsVBOPeakFPS[2];
        float               drawArraysVBOPeakFPS;
    };

    VertexPerformance       vertexPerformance;

    void computeFeatureRating();
    void countBugs();

	// To Do: these may need "virtual"
    virtual void onInit();

    virtual void onUserInput(UserInput* ui);

    virtual void onSimulation(SimTime sdt, SimTime rdt, SimTime idt);

    virtual void onGraphics (RenderDevice *rd, Array< SurfaceRef > &posed3D, Array< Surface2DRef > &posed2D);

    virtual void onCleanup();

    /** Draw some nice graphics */
    void doFunStuff();

    /** Draws the popup window, but not its contents.  Returns the window bounds. */
    Rect2D drawPopup(const char* title);

    /** Also loads reportFont and gfxMeterTexture */
    void showSplashScreen();

    App(const GApp::Settings& settings);

    ~App();
};

#endif
