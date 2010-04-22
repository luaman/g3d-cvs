/**
  @file network/App.cpp

  Demonstration of LAN discovery and server browsing using the G3D::Discovery API.

  There are two lines in this program that actually matter.  Everything else is scaffolding for the GUI
  and GApp.  The lines are:

  Client side:

     bool selected = Discovery::Client::browse(APPLICATION_NAME, window(), debugWindow->theme(), description);

  Server side:

     addWidget(Discovery::Server::create(description));
 */
#include "App.h"

// Tells C++ to invoke command-line main() function even on OS X and Win32.
G3D_START_AT_MAIN();

#define APPLICATION_NAME ("Network Demo")

int main(int argc, char** argv) {
    GApp::Settings settings;
    settings.window.width = 600;
    settings.window.height = 400;
    settings.window.caption = APPLICATION_NAME;
    return App(settings).run();
}

App::App(const GApp::Settings& settings) : GApp(settings) {}

class ModeDialog : public GuiWindow {
private:

    GuiButton*  clientButton;
    GuiButton*  serverButton;
    bool        server;

    ModeDialog(OSWindow* osWindow, GuiThemeRef theme) : 
        GuiWindow("Chose Mode", theme, Rect2D::xywh(osWindow->width() / 2 - 120, osWindow->height() / 2 - 50, 240, 100),
            GuiTheme::DIALOG_WINDOW_STYLE, NO_CLOSE), server(false) {

        clientButton = pane()->addButton("Client");
        serverButton = pane()->addButton("Server");

        clientButton->setRect(Rect2D::xywh( 10, 20, 100, 32));
        serverButton->setRect(Rect2D::xywh(130, 20, 100, 32));
    }

public:

    static bool isServer(OSWindow* osWindow, GuiThemeRef theme) {
        ReferenceCountedPointer<ModeDialog> dialog = new ModeDialog(osWindow, theme);
        dialog->showModal(osWindow);
        return dialog->server;
    }
        
    bool onEvent(const GEvent& event) {
        if (GuiWindow::onEvent(event)) {
            return true;
        }

        if (event.type == GEventType::KEY_DOWN) {
            if (event.key.keysym.sym == GKey::ESCAPE) {
                // Cancelled
                exit(0);
                return true;
            }
        }

        if (event.type == GEventType::GUI_ACTION) {
            // Set the state based on whether the user pressed the server button
            server = (event.gui.control == serverButton);

            // Close the window
            setVisible(false);
            return true;
        }

        return false;
    }
};


void App::onInit() {
    showRenderingStats = false;
    developerWindow->setVisible(false);
    developerWindow->cameraControlWindow->setVisible(false);
    setDesiredFrameRate(60);

    // The dialogs will render over whatever is on screen, so here we initially 
    // make the screen white.
    renderDevice->setColorClearValue(Color3::white());
    renderDevice->clear();
    renderDevice->swapBuffers();

    if (ModeDialog::isServer(window(), debugWindow->theme())) {
        Discovery::ServerDescription description;
        description.applicationName = APPLICATION_NAME;
        description.applicationAddress = NetAddress(NetworkDevice::instance()->adapterArray()[0].ip, 10002);
        description.serverName = NetworkDevice::instance()->localHostName();

        addWidget(Discovery::Server::create(description));
        isServer = true;
        renderDevice->setColorClearValue(Color3::black());
    } else {
        isServer = false;
        renderDevice->clear();
        renderDevice->swapBuffers();

        // Client
        browseServers();
    }
}


void App::browseServers() {
    // If you want dynamic rendering behind the browser, add the Discovery::Client 
    // as a widget on the GApp and set visible = true instead of displaying it 
    // modally with a static method.  Here we just clear the screen to white.
    renderDevice->clear();

    Discovery::ServerDescription description;
    bool selected = Discovery::Client::browse(APPLICATION_NAME, window(), debugWindow->theme(), description);
    (void)selected;
    // In an actual program, we would now connect to the specified server.
    // See also browseAndConnect, which performs the connection step as well.
}


void App::onGraphics(RenderDevice* rd, Array<Surface::Ref>& posed3D, Array<Surface2D::Ref>& posed2D) {
    rd->clear();
    GFont::Ref font = debugWindow->theme()->defaultStyle().font;

    rd->push2D();
    if (isServer) {
        font->draw2D(rd, "SERVER", rd->viewport().center(), 30, Color3::white(), Color4::clear(), GFont::XALIGN_CENTER);
    } else {
        font->draw2D(rd, "CLIENT", rd->viewport().center(), 30, Color3::blue(), Color4::clear(), GFont::XALIGN_CENTER);
    }
    rd->pop2D();

    // Render 2D objects like Widgets
    Surface2D::sortAndRender(rd, posed2D);
}
