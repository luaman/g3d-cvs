#include "App.h"
#include "mongoose.h"
#include "View.h"
#include <time.h>

// The method used to avoid flickering relies on the client keeping a
// cache of the previous frame.  Some browsers (notably
// iPhone/iPod/iPad) will only cache tiny images--15k and under.  So
// we encode the output as a JPEG and render to a small window.  For
// a regular PC/Mac browser we could easily render a much large image.
static const int WIDTH = 340;
static const int HEIGHT = 200;

static const char* TITLE = "G3D Render Server Sample";

App::App() : rd(NULL), ctx(NULL), nd(NULL) {
    startGraphics();
    
    if (! GLCaps::supports_GL_ARB_framebuffer_object()) {
        printf("This sample requires a GPU with support for GL_ARB_framebuffer_object.\n");
        ::exit(-1);
    }

    startNetwork();
    loadScene();

#   ifdef G3D_WIN32
        // On Windows, this thread must release the OpenGL context so that another can grab it.
        glMakeCurrent(NULL);
#   endif
}


void App::startGraphics() {
    OSWindow::Settings osWindowSettings;
    osWindowSettings.visible = false;

    rd = new RenderDevice();
    rd->init(osWindowSettings);
        
    glContext = glGetCurrentContext();
}


void App::startNetwork() {
    // Start Mongoose
    ctx = mg_start();
        
    mg_set_option(ctx, "ports", "8081");

    // Serve from the current directory
    const int N = 4096;
    char dir[N];
    getcwd(dir, N);
    mg_set_option(ctx, "root", dir);

    // Handle 404 explicitly
    mg_set_error_callback(ctx, 404, process404Error, this);

    // Callbacks are processed in the order that they are added
    mg_set_uri_callback(ctx, "/static/*", processStatic, this);
    mg_set_uri_callback(ctx, "/dynamic/*", processDynamic, this);
    mg_set_uri_callback(ctx, "/*", processDefault, this);

    // Start G3D networking
    nd = NetworkDevice::instance();

    printf("G3D RemoteRender Server %s started on port(s) %s\nWeb server root = %s\nPress ENTER to quit.\n\n",
        mg_version(), mg_get_option(ctx, "ports"), mg_get_option(ctx, "root"));

    printf("Connect to:\n");
    Array<NetAddress> myAddrs;
    nd->localHostAddresses(myAddrs);
    for (int i = 0; i < myAddrs.size(); ++i) {
        printf("   http://%s:%s/\n", myAddrs[i].ipString().c_str(), mg_get_option(ctx, "ports"));        
    }
    
    printf("   http://%s:%s/\n\n", nd->localHostName().c_str(), mg_get_option(ctx, "ports"));
}


void App::loadScene() {
    m_map = BSPMap::fromFile(FilePath::concat(System::findDataFile("quake3"), "tremulous/map-atcs-1.1.0.pk3"), "atcs.bsp", 1.0f, "<none>");

    if (m_map.isNull()) {
        printf("Warning: m_map is NULL.\n");
    }
}


void App::sendJPEG(mg_connection* conn, const GImage& image) {
    mg_printf(conn, "HTTP/1.1 200 OK\r\n");

    // We explicitly say that this image never expires, so that the client
    // knows it is safe to cache.

    mg_printf(conn, "Expires: %s\r\n", tomorrow().c_str());
    mg_printf(conn, "Cache-Control: max-age=172800, public\r\n");
    mg_printf(conn, "Content-Type: image/jpeg\r\n\r\n");
    BinaryOutput b("<memory>", G3D_LITTLE_ENDIAN);
    image.encode(GImage::JPEG, b);
    mg_write(conn, b.getCArray(), b.size());
}


void App::processStatic(mg_connection* conn, const mg_request_info* request_info, void* data) {
    printRequest(request_info);

    mg_send_file(conn, request_info->uri);
}


void App::printRequest(const mg_request_info* request_info) {
    NetAddress client(request_info->remote_ip);

    printf("%s %s \"%s\" \"%s\"\n",
                client.ipString().c_str(), request_info->request_method, request_info->uri,
                request_info->query_string ? request_info->query_string : "(NULL)");
    /*
    for (int i = 0; i < request_info->num_headers; ++i) {
        printf("   %s: %s\n", request_info->http_headers[i].name, request_info->http_headers[i].value);
    }
    */
}


void App::processDynamic(mg_connection* conn, const mg_request_info* request_info, void* data) {
    printRequest(request_info);

    App* app = static_cast<App*>(data);

    const std::string& url  = request_info->uri;
    const std::string& base = FilePath::base(url);
    const std::string& ext  = toLower(FilePath::ext(url));

    View view(base);

    if (ext == "png") {

        GImage image;
        app->render(view, image);
        sendJPEG(conn, image);
        
    } else if (ext == "html") {

        // When reloading the page, there would be flicker while the
        // new image loads if we did not take preventative steps.  The 
        // solution used here is to tell the browser that the .png
        // images never expire and then always load a new page with
        // the old image placed underneath the new one.  We pass the
        // old image in the query string and perform the overlapping
        // of images using an HTML table background image.

        const std::string& thisView = view.unparse();
        const std::string& oldView = View(request_info->query_string ? request_info->query_string : "").unparse();

        mg_printf(conn, "HTTP/1.1 200 OK\r\n");
        mg_printf(conn, "Expires: %s\r\n", tomorrow().c_str());
        mg_printf(conn, "Cache-Control: max-age=172800, public\r\n");
        mg_printf(conn, "Content-Type: text/html\r\n\r\n");
        mg_printf(conn, "<html>\n");
        mg_printf(conn, " <head>\n");
        mg_printf(conn, "  <title>%s</title>\n", TITLE);
        mg_printf(conn, " </head>\n");
        mg_printf(conn, " <body bgcolor=#FFFFFF background=\"/static/carbon-fiber.png\">\n");
        mg_printf(conn, "  <center>\n");
        mg_printf(conn, "   <table cellpadding=0 cellspacing=0 border=1>\n");
        mg_printf(conn, "    <tr><td><img src=\"/static/title.png\"/></td></tr>\n");
        mg_printf(conn, "   </table><br>\n");
        mg_printf(conn, "   <table cellpadding=0 cellspacing=0 border=1 width=%d height=%d background=\"/dynamic/%s.png\">\n", WIDTH, HEIGHT, oldView.c_str());
        mg_printf(conn, "    <tr><td>\n");
        mg_printf(conn, "     <img src=\"/dynamic/%s.png\" width=%d height=%d border=0/><br>\n", thisView.c_str(), WIDTH, HEIGHT);
        mg_printf(conn, "    </td></tr>\n");
        mg_printf(conn, "   </table>\n");
        mg_printf(conn, "   <img src=\"/static/buttons.png\" usemap=\"#navButtonMap\" border=0/>\n");
        mg_printf(conn, "   <map name=\"navButtonMap\">\n");
        mg_printf(conn, "    <area shape=rect coords=\"11,29, 75,78\" href=\"/dynamic/%s.html?/dynamic/%s.png\"/>\n", 
                  view.left(app->m_map).unparse().c_str(), thisView.c_str());
        mg_printf(conn, "    <area shape=rect coords=\"81,3, 145,72\" href=\"/dynamic/%s.html?/dynamic/%s.png\"/>\n", 
                  view.forward(app->m_map).unparse().c_str(), thisView.c_str());
        mg_printf(conn, "    <area shape=rect coords=\"149,29, 213,78\" href=\"/dynamic/%s.html?/dynamic/%s.png\"/>\n", 
                  view.right(app->m_map).unparse().c_str(), thisView.c_str());
        mg_printf(conn, "   </map>\n");
        mg_printf(conn, "   <br><br>[<a href=\"3d.html\"><font face=\"Arial\">Reset</font></a>]\n");
        mg_printf(conn, "  </center>\n");
        mg_printf(conn, "  <br><br><font size=2>Powered by <a href=\"http://g3d.sf.net\">G3D</a> and <a href=\"http://code.google.com/p/mongoose/\">Mongoose</a>.  Map 'ATCS' from <a href=\"http://tremulous.net/\">Tremulous</a>.</font>\n");
        mg_printf(conn, " </body>\n");
        mg_printf(conn, "</html>\n");
        
    } else {
        mg_printf(conn, "HTTP/1.1 200 OK\r\n");
        mg_printf(conn, "Content-Type: text/html\r\n\r\n");
        mg_printf(conn, "Your request was received.\n");
    }
}


void App::processDefault(mg_connection* conn, const mg_request_info* request_info, void* data) {
    mg_send_file(conn, "/static/start.html");
}


void App::process404Error(mg_connection* conn, const mg_request_info* request_info, void* data) {
    mg_printf(conn, "%s", "HTTP/1.1 200 OK\r\n");
    mg_printf(conn, "%s", "Content-Type: text/html\r\n\r\n");
    mg_printf(conn, "%s", "<html><head><title>Illegal URL</title></head><body>Illegal URL</body></html>\n");
}


std::string App::tomorrow() {
    const time_t seconds = ::time(NULL) + (60 * 60 * 12);
    tm* t = gmtime(&seconds);

    static const char* day[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static const char* month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    return format("%s, %02d %s %d %02d:%02d:%02d GMT", day[t->tm_wday], t->tm_mday, month[t->tm_mon], t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
}


void App::render(const View& view, GImage& image) {
    gpuMutex.lock();

    // Must set the context before rendering to OpenGL when on a different thread
    // http://developer.apple.com/mac/library/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_threading/opengl_threading.html
    GLContext oldContext = glGetCurrentContext();
    glMakeCurrent(glContext);
    debugAssertGLOk();

    alwaysAssertM(rd, "OpenGL not initialized");
    FrameBuffer::Ref fb = FrameBuffer::create("FrameBuffer");
        
    Texture::Ref color = Texture::createEmpty("Color", WIDTH, HEIGHT, ImageFormat::RGB8());
    Texture::Ref depth = Texture::createEmpty("Depth", WIDTH, HEIGHT, ImageFormat::DEPTH24());
    fb->set(FrameBuffer::COLOR0, color);
    fb->set(FrameBuffer::DEPTH,  depth);
        
    GCamera camera;
    CFrame cframe = view.toCFrame();
    if (m_map.notNull()) {
        // Hardcoded for ATCS map
        cframe.translation += Vector3(65.5f, -0.6f, -1.7f); //m_map->getStartingPosition();
    }
    camera.setCoordinateFrame(cframe);

    rd->pushState(fb);
    {
        rd->setColorClearValue(Color3::white());
        rd->clear();

        rd->setProjectionAndCameraMatrix(camera);
        rd->setObjectToWorldMatrix(CFrame());
        
        if (m_map.notNull()) {
            m_map->render(rd, camera, 1.0f);
        }

    }
    rd->popState();
        
    // Read back to the CPU
    color->getImage(image);
    glMakeCurrent(oldContext);
    gpuMutex.unlock();
}


void App::stopNetwork() {
    printf("Waiting for threads to shut down...\n");
    mg_stop(ctx);

    NetworkDevice::cleanup();
}


void App::stopGraphics() {
    glMakeCurrent(glContext);

    m_map = NULL;

    rd->cleanup();
    delete rd;
    rd = NULL;
}


App::~App() {
    stopNetwork();
    stopGraphics();
}
