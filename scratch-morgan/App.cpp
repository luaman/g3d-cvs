/** \file App.cpp */
#include "App.h"

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    GApp::Settings settings;

    settings.window.width       = 512; 
    settings.window.height      = 512;

    return App(settings).run();
}


App::App(const GApp::Settings& settings) : GApp(settings) {}


void App::onInit() {

    // Turn on the developer HUD
    debugWindow->setVisible(false);
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(true);
    developerWindow->videoRecordDialog->setEnabled(true);
    showRenderingStats = false;
    setDesiredFrameRate(60);
    debugPane->addCheckBox("Enabled", &enabled);
    enabled = false;
    debugWindow->pack();
    debugWindow->moveTo(Vector2(0, window()->height() - debugWindow->rect().height()));

    m_current.resize(100);
    for (int i = 0; i < m_current.size(); ++i) {
        m_current[i].position = Vector2(Random::common().uniform(0, 1), Random::common().uniform(0, 1)) * 0.2f;
    }

    renderDevice->setColorClearValue(Color3::white());

}

void wrap(Vector2& v, float low, float high) {
    v.x = wrap(v.x, low, high);
    v.y = wrap(v.y, low, high);
}

void App::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
    (void)idt; (void)sdt; (void)rdt;

    if (! enabled) {
        return;
    }

    m_previous = m_current;
    // This constant could be much larger; I made it small for visualization purposes
    const float k = 0.001f / m_current.size();
    for (int i = 0; i < m_current.size(); ++i) {
        Vector2 force(0,0);
        for (int j = 0; j < m_previous.size(); ++j) {
            if (i != j) {
                Vector2 diff = m_previous[i].position - m_previous[j].position;
                wrap(diff, -0.5, 0.5);

                // TODO: Measure distance as the sum of the pixel values in the importance map
                // along the line between the points.
                float distance = diff.length();
                force += k * diff.direction() / square(distance);
            }
        }
        m_current[i].position += force;
        wrap(m_current[i].position, 0, 1);
    }
}


void App::onGraphics3D(RenderDevice* rd, Array<Surface::Ref>& surface3D) {
    (void)surface3D;
    (void)rd;
}


void App::onGraphics2D(RenderDevice* rd, Array<Surface2D::Ref>& posed2D) {
    rd->push2D();
    {
        const int s = window()->width();

        rd->setPointSize(11);
        rd->setColor(Color3::black());
        rd->beginPrimitive(PrimitiveType::POINTS);
        for (int i = 0; i < m_current.size(); ++i) {
            rd->sendVertex(m_current[i].position * s);
        }
        rd->endPrimitive();

        rd->setPointSize(9);
        rd->beginPrimitive(PrimitiveType::POINTS);
        for (int i = 0; i < m_current.size(); ++i) {
            rd->setColor(Color3::pastelMap(i));
            rd->sendVertex(m_current[i].position * s);
        }
        rd->endPrimitive();
        Draw::rect2DBorder(Rect2D::xywh(0,0,s,s), rd, Color3::black(),1,0);
    }
    rd->pop2D();

    // Render 2D objects like Widgets.  These do not receive tone mapping or gamma correction
    Surface2D::sortAndRender(rd, posed2D);
}
