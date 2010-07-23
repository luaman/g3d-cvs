#ifndef App_h
#define App_h

#include <G3D/G3DAll.h>

struct mg_request_info;
struct mg_context;
struct mg_connection;

class App {
private:

    RenderDevice*      rd;
    GMutex             gpuMutex;
    GLContext          glContext;
    mg_context*        ctx;
    NetworkDevice*     nd;

    BSPMapRef          m_map;

    static App* instance;

    /** Called from constructor */
    void startGraphics();

    /** Called from constructor */
    void startNetwork();

    /** Called from constructor */
    void loadScene();

    /** Called from destructor */
    void stopNetwork();

    /** Called from destructor */
    void stopGraphics();

    /** Returns a UTC time for tomorrow. */
    static std::string tomorrow();

    /** Writes a complete HTTP response that sends a JPEG image to \a conn. */
    static void sendJPEG(mg_connection* conn, const GImage& image);

    /** This callback function is invoked (on separate threads) for every HTTP request received */
    static void processDynamic(mg_connection* conn, const mg_request_info* request_info, void* data);

    static void process404Error(mg_connection* conn, const mg_request_info* request_info, void* data);

    /** Respond to a request for a specific file.  We only allow this within the "static" subdirectory.*/
    static void processStatic(mg_connection* conn, const mg_request_info* request_info, void* data);

    static void processDefault(mg_connection* conn, const mg_request_info* request_info, void* data);

    void render(const class View& view, GImage& image);

    static void printRequest(const mg_request_info* request_info);

public:

    App();

    ~App();

};

#endif
