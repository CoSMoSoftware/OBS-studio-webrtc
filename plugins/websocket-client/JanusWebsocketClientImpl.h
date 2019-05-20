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

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class JanusWebsocketClientImpl : public WebsocketClient
{
public:
    JanusWebsocketClientImpl();
    ~JanusWebsocketClientImpl();
    virtual bool connect(const std::string& url, long long room, const std::string& username, const std::string & token, Listener* listener) override;
    virtual bool open(const std::string &sdp, const std::string& codec = "vp8", const std::string& milliId = "") override;
    virtual bool trickle(const std::string &mid, int index, const std::string &candidate, bool last) override;
    virtual bool disconnect(bool wait)  override;
    void keepConnectionAlive();

private:
    bool logged;
    long long session_id;
    long long handle_id;

    std::atomic<bool> is_running;
    std::future<void> handle;
    std::thread thread;
    std::thread thread_keepAlive;
   
    Client client;
    Client::connection_ptr connection;
};

