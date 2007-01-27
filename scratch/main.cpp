/**
  @file demos/main.cpp

  http://kryshen.pp.ru/games/bmtron.html

  @author Morgan McGuire, matrix@graphics3d.com
 */

//#include <G3D/G3DAll.h>
#include <G3D/G3D.h>
#include <GLG3D/GLG3D.h>
using namespace G3D;

#define SCALE  (10)
#define WIDTH  (64)
#define HEIGHT (64)

/**
 This simple demo applet uses the debug mode as the regular
 rendering mode so you can fly around the scene.
 */
class Demo : public GApplet {
public:

    // Add state that should be visible to this applet.
    // If you have multiple applets that need to share
    // state, put it in the App.

    class App*          app;

    /** Map of the world */
    GImage              map;
    TextureRef          texture;

    class Player {
    public:
        Vector2int16        position;
        Vector2int16        velocity;
        Color3uint8         color;

        void onSimulation(GImage& map) {
            position += velocity;

            position = position.clamp(Vector2int16(0, 0), Vector2int16(WIDTH - 1, HEIGHT - 1));

            // Update the board
            map.pixel3(position.x, position.y) = color;
        }
    };

    Array<Player>       player;

    Demo(App* app);

    virtual ~Demo() {}

    virtual void onInit();

    virtual void onLogic();

	virtual void onNetwork();

    virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);

    virtual void onGraphics(RenderDevice* rd);

    virtual void onUserInput(UserInput* ui);

    virtual void onCleanup();

};



class App : public GApp {
protected:
    int main();
public:
    Demo*               applet;

    App(const GApp::Settings& settings);

    ~App();
};


Demo::Demo(App* _app) : GApplet(_app), app(_app), map(WIDTH, HEIGHT, 3) {
    player.resize(2);

    player[0].position.x = 0;
    player[0].position.y = HEIGHT / 2;
    player[0].velocity.x = 1;
    player[0].velocity.y = 0;
    player[0].color = Color3::blue();
    
    player[1].position.x = WIDTH - 1;
    player[1].position.y = HEIGHT / 2;
    player[1].velocity.x = -1;
    player[1].velocity.y = 0;
    player[1].color = Color3::yellow();
}


void Demo::onInit()  {
    // Called before Demo::run() beings
    GApplet::onInit();
}


void Demo::onCleanup() {
    // Called when Demo::run() exits
}


void Demo::onLogic() {
    // Add non-simulation game logic and AI code here
}


void Demo::onNetwork() {
	// Poll net messages here
}


void Demo::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// Add physical simulation here.  You can make your time advancement
    // based on any of the three arguments.
    
    static int x = 0;
    x = (x + 1) % 4;
    if (x != 0) {
      return;
    }

    for (int p = 0; p < player.size(); ++p) {
        player[p].onSimulation(map);        
    }
}


void Demo::onUserInput(UserInput* ui) {
    if (ui->keyPressed(SDLK_ESCAPE)) {
        // Even when we aren't in debug mode, quit on escape.
        endApplet = true;
        app->endProgram = true;
    }

	// Add other key handling here
    if (ui->getX() > 0.5) {
        player[0].velocity = Vector2int16(1, 0);
    } else if (ui->getX() < -0.5) {
        player[0].velocity = Vector2int16(-1, 0);
    } else if (ui->getY() > 0.5) {
        player[0].velocity = Vector2int16(0, -1);
    } else if (ui->getY() < -0.5) {
        player[0].velocity = Vector2int16(0, 1);
    }
	
	//must call GApplet::onUserInput
	GApplet::onUserInput(ui);
}


void Demo::onGraphics(RenderDevice* rd) {

    // Cyan background
    rd->setColorClearValue(Color3(0.1f, 0.5f, 1.0f));
    rd->clear();

    rd->setProjectionAndCameraMatrix(app->debugCamera);
    LightingParameters lighting(G3D::toSeconds(11, 00, 00, AM));

    //    Draw::axes(rd);
    rd->enableLighting();
		rd->setLight(0, GLight::directional(lighting.lightDirection, lighting.lightColor));
		rd->setAmbientLightColor(lighting.ambient);

        Draw::ray(Ray::fromOriginAndDirection(Vector3::zero(), Vector3::unitY()), rd);
    rd->disableLighting();
    /*
    rd->push2D();

        Texture::Settings settings;
        settings.interpolateMode = Texture::NEAREST_NO_MIPMAP;
        settings.wrapMode = Texture::CLAMP;
        settings.autoMipMap = false;

        texture = Texture::fromGImage("Game world", map, TextureFormat::AUTO, Texture::DIM_2D, settings);

        rd->setTexture(0, texture);
        Draw::rect2D(rd->viewport(), rd);
    rd->pop2D();
    */
}


int App::main() {
	setDebugMode(true);
	debugController->setActive(true);
    
    applet->run();
    return 1;
}


App::App(const GApp::Settings& settings) : GApp(settings) {
    applet = new Demo(this);
    applet->setDesiredFrameRate(40);
}


App::~App() {
    delete applet;
}

G3D_START_AT_MAIN();

int main(int argc, char** argv) {
	GApp::Settings settings;
    settings.window.width = WIDTH * SCALE;
    settings.window.width = HEIGHT * SCALE;
    App(settings).run();
    return 0;
}
