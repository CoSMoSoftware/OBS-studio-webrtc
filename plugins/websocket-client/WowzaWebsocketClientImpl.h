#include "WebsocketClient.h"

//Use http://think-async.com/ insted of boost
#define ASIO_STANDALONE
#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"

#include <ctime>
#include <string>

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class WowzaWebsocketClientImpl : public WebsocketClient
{
public:
    WowzaWebsocketClientImpl();
    ~WowzaWebsocketClientImpl();
    virtual bool connect(const std::string& url, const std::string& /* room */, const std::string& username, const std::string& token, Listener* listener) override;
    virtual bool open(const std::string& sdp, const std::string& /* codec = "h264" */, const std::string& /* milliId = "" */) override;
    virtual bool trickle(const std::string& /* mid */, int /* index */, const std::string& candidate, bool /* last */) override;
    virtual bool disconnect(bool wait) override;

private:
    std::string appName;
    long long session_id;
    std::string streamName;

    std::future<void> handle;
    std::thread thread;

    Client client;
    Client::connection_ptr connection;
};