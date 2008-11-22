
#include <G3D/platform.h>
#include <G3D/BinaryInput.h>
#include <G3D/BinaryOutput.h>
#include <G3D/Log.h>
#include "Discovery2.h"


namespace G3D { namespace Discovery2 {

ServerDescription::ServerDescription(BinaryInput& b) {
    deserialize(b);
}


void ServerDescription::serialize(BinaryOutput& b) const {
    b.writeString(serverName);
    applicationAddress.serialize(b);
    b.writeString(applicationName);
    b.writeInt32(maxClients);
    b.writeInt32(currentClients);
    b.writeString(data);
}


void ServerDescription::deserialize(BinaryInput& b) {
    serverName      = b.readString();
    applicationAddress.deserialize(b);
    applicationName = b.readString();
    maxClients      = b.readInt32();
    debugAssert(maxClients >= 0);
    currentClients  = b.readInt32();
    data            = b.readString();
    lastUpdateTime  = System::time();
}


std::string ServerDescription::displayText() const {
    if (maxClients == INT_MAX) {
        // Infinite clients
        return 
            format("%16s (%7d) %s:%-5d", 
                   serverName.c_str(),
                   currentClients,
                   NetworkDevice::formatIP(applicationAddress.ip()).c_str(),
                   applicationAddress.port());
    } else {
        // Finite clients
        return 
            format("%16s (%3d/%-3d) %s:%-5d", 
                   serverName.c_str(),
                   currentClients,
                   maxClients,
                   NetworkDevice::formatIP(applicationAddress.ip()).c_str(),
                   applicationAddress.port());
    }
}

/////////////////////////////////////////////////////////////

Client::Client(const std::string& applicationName, const Settings& settings,
               OSWindow* osWindow, GuiThemeRef theme) :
    GuiWindow("Server Browser", theme, 
              Rect2D::xywh(100,100, 500, 500), GuiTheme::NO_WINDOW_STYLE,NO_CLOSE),
    m_settings(settings),
    m_osWindow(osWindow),
    m_applicationName(applicationName) {

    // Construct broadcast array
    const Array<uint32> ipArray = NetworkDevice::instance()->broadcastAddressArray();
    m_broadcastAddressArray.resize(ipArray.size());
    for (int i = 0; i < m_broadcastAddressArray.size(); ++i) {
        m_broadcastAddressArray[i] = NetAddress(ipArray[i], m_settings.clientBroadcastPort);
    }

    m_net = LightweightConduit::create(m_settings.serverBroadcastPort, true, true);
}


ClientRef Client::create
(
 const std::string& applicationName, 
 OSWindow* osWindow,
 GuiThemeRef theme,
 const Settings& settings) {

    return new Client(applicationName, settings, osWindow, theme);
}


void Client::onNetwork() {
    // Check for server announcements
    switch (m_net->waitingMessageType()) {
    case 0:
        // No incoming message
        break;

    case Settings::SERVER_DESCRIPTION_TYPE:
        receiveDescription();
        break;

    default:
        // Some unknown message
        logPrintf("Discovery::Client ignored an unexpected "
                  "packet of type %d on port %d\n",
                  m_net->waitingMessageType(),
                  m_settings.serverBroadcastPort);
        m_net->receive();
        break;
    }

    // Remove old servers    
    const RealTime tooOld = System::time() - 3.0 * m_settings.serverAdvertisementPeriod;
    for (int i = 0; i < m_serverArray.size(); ++i) {
        if (m_serverArray[i].lastUpdateTime < tooOld) {
            m_serverArray.remove(i);
            m_serverDisplayArray.remove(i);
            --i;
        }
    }
}


void Client::receiveDescription() {
    NetAddress sender;
    ServerDescription s;
    m_net->receive(sender, s);
    
    // See if this server is already known to us
    int i = 0;
    for (int i = 0; i < m_serverArray.size(); ++i) {
        if (m_serverArray[i].applicationAddress == s.applicationAddress) {
            // This is the server
            break;
        }
    }

    if (i == m_serverArray.size()) {
        // Not found-- append to the end of the list
        m_serverArray.append(s);
        m_serverDisplayArray.append("");
    } else {
        m_serverArray[i] = s;
    }

    // Update the time in this entry
    m_serverArray[i].lastUpdateTime = System::time();

    // Update the display entry
    m_serverDisplayArray[i] = m_serverArray[i].displayText();
}


ReliableConduitRef Client::browseAndConnect
(const std::string& applicationName, 
 OSWindow* osWindow, 
 GuiThemeRef theme, 
 const Settings& settings) {

    ServerDescription server;

    while (browse(applicationName, osWindow, theme, server, settings)) {
        
        // Try to connect to the selected server
        ReliableConduitRef connection = ReliableConduit::create(server.applicationAddress);
        
        if (connection.notNull() && connection->ok()) {
            // Successful connection; return
            return connection;
        } else {
            // Display failure message and repeat
            //TODO
        }
    }
    
    // Cancelled
    return NULL;
}


bool Client::browse
(const std::string applicationName, 
 OSWindow* osWindow, 
 GuiThemeRef theme,
 ServerDescription& d, const Settings& settings) {
    //TODO
    return false;
}



//////////////////////////////////////////////////////////////////////////////////////////////

ServerRef Server::create(const ServerDescription& description, 
                         const Settings& settings) {
    return new Server(description, settings);
}


Server::Server(const ServerDescription& description, const Settings& settings) :
    m_settings(settings),
    m_description(description) {

    const Array<uint32> ipArray = NetworkDevice::instance()->broadcastAddressArray();
    m_broadcastAddressArray.resize(ipArray.size());
    for (int i = 0; i < m_broadcastAddressArray.size(); ++i) {
        m_broadcastAddressArray[i] = NetAddress(ipArray[i], m_settings.serverBroadcastPort);
    }

    m_net = LightweightConduit::create(m_settings.clientBroadcastPort, true, true);
    
    debugAssert(m_settings.serverAdvertisementPeriod > 0);
    
    sendAdvertisement();
}


void Server::setDescription(const ServerDescription& d) {
    m_description = d;
    sendAdvertisement();
}


void Server::sendAdvertisement() {
    // Send to all clients
    m_net->send(m_broadcastAddressArray, Settings::SERVER_DESCRIPTION_TYPE, m_description);
    m_lastAdvertisementTime = System::time();
}


void Server::onNetwork() {
    switch (m_net->waitingMessageType()) {
    case 0:
        // No incoming message
        break;

    case Settings::CLIENT_QUERY_TYPE:
        // A client is requesting servers to advertise themselves
        sendAdvertisement();
        m_net->receive();
        break;

    default:
        // Some unknown message
        logPrintf("Discovery::Server ignored an unexpected "
                  "packet of type %d on port %d\n",
                  m_net->waitingMessageType(),
                  m_settings.clientBroadcastPort);
        m_net->receive();
        break;
    }

    // See if it is time to send an unsolicited advertisement again.
    if (m_lastAdvertisementTime + m_settings.serverAdvertisementPeriod >= System::time()) {
        sendAdvertisement();
    }
}

}}
