/**
 @file Discovery.h
  
 @maintainer Morgan McGuire, http://graphics.cs.williams.edu

 @created 2008-11-20
 @edited  2008-11-22
 */

#ifndef G3D_Discovery_h
#define G3D_Discovery_h

#include <cstring>
#include <climits>
#include "G3D/NetAddress.h"
#include "G3D/NetworkDevice.h"
#include "GLG3D/Widget.h"
#include "GLG3D/GuiWindow.h"
#include "GLG3D/OSWindow.h"
#include "GLG3D/GuiTheme.h"

namespace G3D {

class BinaryInput;
class BinaryOutput;

namespace Discovery {

using std::string;

/** @brief Used by G3D::Discovery::Server to advertise its services. 

@beta
*/
class ServerDescription {
public:
    /** Name of the server for display.  This need not have any
        relationship to the hostname of the server.*/
    std::string              serverName;

    /** Address on which the server is listening for incoming
        application (not discovery) connections. */
    NetAddress               applicationAddress;

    /** Name of the application.  Clients only display servers
        for applications that have the same name as themselves.

        Include a version number in this if you wish to distinguish
        between application versions.
    */
    std::string              applicationName;
    
    /** Maximum number of clients the server is willing to accept. */
    int                      maxClients;

    /** Number of clients currently connected to the server.*/
    int                      currentClients;

    /** Application specific data.  This is not displayed by the
     built-in server browser.  It is for storing application specific
     data like the name of the map for a game.

     See G3D::TextInput for parsing if the data is complicated.*/
    std::string              data;

    /** On the client side, the last time this server was checked.  Unused on the server side.*/
    RealTime                 lastUpdateTime;

    inline ServerDescription() : maxClients(INT_MAX), currentClients(0), lastUpdateTime(0) {}

    ServerDescription(BinaryInput& b);
    
    std::string displayText() const;

    void serialize(BinaryOutput& b) const;

    void deserialize(BinaryInput& b);
};


/** 
  Options for configuring the G3D Discovery protocol. 
  These rarely need to be changed except for the client-side
  display options.
@beta
 */
class Settings {
public:
    enum {CLIENT_QUERY_TYPE = 44144,
          SERVER_DESCRIPTION_TYPE = 10101};

    /** Port on which clients broadcast looking for servers. */ 
    uint16       clientBroadcastPort;

    /** Port on which servers advertise themselves.*/
    uint16       serverBroadcastPort;

    /** Servers announce themselves every @a serverAdvertisementPeriod
        seconds, and whenever they hear a client request.  Clients
        assume that any server that has not updated its advertisement
        for three times this period is offline.

        Must be greater than zero.*/
    RealTime     serverAdvertisementPeriod;

    /** For the client side server browser. Uninitialized 
    fields are taken from the theme.*/
    GuiTheme::TextStyle displayStyle;

    /** Server browser prompt on the client side*/
    std::string  prompt;

    inline Settings() : 
        clientBroadcastPort(6173),
        serverBroadcastPort(6174),
        serverAdvertisementPeriod(2),
        prompt("Select server") {}
};


/** @see G3D::Discovery::Client */
typedef ReferenceCountedPointer<class Client> ClientRef; 

/**
   To use the built-in browser UI, call Client::browse or
   Client::browseAndConnect.

   To write your own UI, do not make the client visible or invoke the modal
   methods.  Instead, add to a WidgetManager
   by calling G3D::GApp::addWidget or G3D::WidgetManager::add, and
   then remove when browsing is complete.

   See samples/network for an example of use
@beta
 */
class Client : public GuiWindow {
private:

    /** Renders the client's server list */
    class Display : public Surface2D {
    public:
        Client*    client;
        virtual Rect2D 	bounds () const;
        virtual float 	depth () const;
        virtual void 	render (RenderDevice *rd) const;
    };

    ReferenceCountedPointer<Display>  m_display;

    Settings                  m_settings;

    OSWindow*                 m_osWindow;

    /** Addresses to broadcast to from sendAdvertisement.
     Set in the constructor.*/
    Array<NetAddress>         m_broadcastAddressArray;

    std::string               m_applicationName;

    Array<ServerDescription>  m_serverArray;

    Array<std::string>        m_serverDisplayArray;

    /** Parallel array to m_serverDisplayArray giving pixel coords in the 
        server browser.  Updated by render() */
    mutable Array<Rect2D>     m_clickBox;

    LightweightConduitRef     m_net;

    /** Index into m_serverArray of the selected server. */
    int                       m_index;

    /** True if user chose to connect, false if they cancelled. */
    bool                      m_connectPushed; 

    Client(const std::string& applicationName, const Settings& settings,
           OSWindow* osWindow, GuiThemeRef theme);

    /** Called from onNetwork() to receive an incoming message on m_net */
    void receiveDescription();

    /** Called by Display::render() */
    virtual void render(RenderDevice* rd) const;

    /** Implements browse() on an instance. */
    bool browseImpl(ServerDescription& d);

public:

    static ClientRef create(const std::string& applicationName, 
                            OSWindow* osWindow = NULL, 
                            GuiThemeRef theme = NULL,
                            const Settings& settings = Settings());

    virtual void onNetwork();

    /** Launches a modal dialog server browser that runs until the user
        selects a server. At that point, it opens a ReliableConduit to the 
        selected server on the port from the ServerDescription.
        
        Returns NULL if the user cancels.

        @param applicationName Must match the ServerDescription::applicationName
     */
    static ReliableConduitRef browseAndConnect
    (const std::string& applicationName,
     OSWindow* osWindow, 
     GuiThemeRef theme,
     const Settings& settings = Settings());
    
    /** Launches a model dialog server browser that runs until the user 
        selects a server. 

        @return false If the user cancels
     */
    static bool browse
    (const std::string applicationName, 
     OSWindow* osWindow, 
     GuiThemeRef theme,
     ServerDescription& d, 
     const Settings& settings = Settings());

    /** @brief All servers that have been discovered */
    inline const Array<ServerDescription>& serverArray() const {
        return m_serverArray;
    }

    /** 
        Array of server names suitable for use with a G3D::GuiListBox.
        This array is parallel to serverArray().        
     */
    inline const Array<std::string>& serverDisplayArray() const {
        return m_serverDisplayArray;
    }

    inline const Settings& settings() const {
        return m_settings;
    }

    virtual bool onEvent(const GEvent& event);
    virtual void onPose(Array<Surface::Ref>& posedArray, Array<Surface2DRef>& posed2DArray);
};


/** @see G3D::Discovery::Server */
typedef ReferenceCountedPointer<class Server> ServerRef; 
   
/**
   @brief Advertises a service on this machine for other clients.

   Invoke onNetwork() periodically (e.g., at 30 fps or higher) to
   manage network requests.  This can be done automatically by calling
   G3d::GApp::addWidget() or G3D::WidgetManager::add() with the server
   as an argument at the start of a program.

   @beta
*/
class Server : public Widget {
private:

    Settings                   m_settings;

    /** Addresses to broadcast to from sendAdvertisement.
     Set in the constructor.*/
    Array<NetAddress>          m_broadcastAddressArray;

    /** 
        Description of the properties of this server. 
        Update at any time.
     */
    ServerDescription          m_description;

    LightweightConduitRef      m_net;

    /** Last time the server advertised. */
    RealTime                   m_lastAdvertisementTime;

    /** Broadcast the m_description on all adapters */
    void sendAdvertisement();

    Server(const ServerDescription& description, const Settings& settings);

public:
    
    static ServerRef create(const ServerDescription& description, 
                            const Settings& settings = Settings());
    
    inline const Settings& settings() const {
        return m_settings;
    }

    inline const ServerDescription& description() {
        return m_description;
    }

    /** Triggers immediate advertising of this description.
     */
    void setDescription(const ServerDescription& d);

    virtual void onNetwork();

    /** @brief True if this server is advertising itself successfully. */
    inline bool ok() const {
        return m_net.notNull() && m_net->ok() && (m_broadcastAddressArray.size() > 0);
    }

};

} // Discovery
} // G3D

#endif
