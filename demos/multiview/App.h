/**
  @file App.h

  Shows two methods for creating multiple views: GuiTextureBox windows
  and multiple explicit viewports.
 */
#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>
#include <GLG3D/GLG3D.h>

#include "Scene.h"

class App : public GApp {

    enum {GUI_HEIGHT = 250};

    GBuffer::Ref        m_gbuffer;
    Scene::Ref          m_scene;
    ShadowMap::Ref      m_shadowMap;

public:
    
    App(const GApp::Settings& settings = GApp::Settings());

    virtual void onInit();
    virtual void onPose(Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D);
    virtual void onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface);
    virtual void onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& surface2D);
};

#endif
