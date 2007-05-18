/**
  @file gfxmeter/Report.h

  @author Morgan McGuire, matrix@graphics3d.com
 */

#ifndef REPORT_H
#define REPORT_H

#include "G3D/G3DAll.h"

#if (defined(main) && ! defined(G3D_WIN32))
#   undef main
#endif

class MD2 {
public:
    CoordinateFrame     cframe;
    MD2ModelRef         model;
    MD2Model::Pose      pose;
    GMaterial           material;

    void load(const std::string& filename) {
        model = MD2Model::fromFile(filename + ".md2");

	Texture::PreProcess preProcess;
	preProcess.brighten = 2.0;
        material.texture.append(Texture::fromFile(filename + ".pcx", 
            TextureFormat::AUTO, Texture::DIM_2D, Texture::Settings(), preProcess));
    }

    void render(RenderDevice* rd) {
        model->pose(cframe, pose, material)->render(rd);
    }

    void renderShadow(RenderDevice* rd) {
        CoordinateFrame cframe2 = cframe;
        cframe2.rotation.setColumn(1, Vector3::zero());
        cframe2.translation.y -= 1.7f;
        rd->setColor(Color3(.9f, .9f, 1));

        PosedModel::Ref m = model->pose(cframe2, pose);

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

/**
 Renders the summary page
 */
class Report : public GApplet {

    MD2                 knight;
    MD2                 ogre;

    enum Popup {NONE, PERFORMANCE};

    Popup               popup;

    Rect2D              performanceButton;
    Rect2D              closeButton;

    class App*          app;

    /** Draw some nice graphics */
    void doFunStuff();

    /** Draws the popup window, but not its contents.  Returns the window bounds. */
    Rect2D drawPopup(const char* title);

public:

    Report(App* app);


    virtual ~Report() {}

    virtual void onInit();

    virtual void onLogic();

	virtual void onNetwork();

    virtual void onSimulation(SimTime sdt, SimTime rdt, SimTime idt);

    virtual void onGraphics(RenderDevice* rd);

    virtual void onCleanup();

};

#endif
