// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include "WebsocketClient.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

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

class JanusWebsocketClientImpl : public WebsocketClient {
public:
    JanusWebsocketClientImpl();
    ~JanusWebsocketClientImpl();

    // WebsocketClient::Listener implementation
    bool connect(
        const std::string & url,
        const std::string & room,
        const std::string & username,
        const std::string & token,
        WebsocketClient::Listener * listener) override;
    bool open(
        const std::string & sdp,
        const std::string & codec,
        const std::string & /* username */) override;
    bool trickle(
        const std::string & mid,
        int index,
        const std::string & candidate,
        bool last) override;
    bool disconnect(bool wait) override;

    void keepConnectionAlive( WebsocketClient::Listener * listener );
    void destroy();

protected:
    std::chrono::time_point<std::chrono::system_clock> last_message_recd_time;

    void handleDisconnect(
        websocketpp::connection_hdl connectionHdl,
        WebsocketClient::Listener * listener);
    void handleFail(
        websocketpp::connection_hdl connectionHdl,
        WebsocketClient::Listener * listener);
    bool hasTimedOut();
    void sendAttachMessage();
    void sendJoinMessage(std::string room);
    void sendLoginMessage(std::string username, std::string token, std::string room);
    bool sendOpenMessage(const std::string &sdp, const std::string &codec);
    bool sendTrickleMessage(
        const std::string &mid,
        int index,
        const std::string &candidate,
        bool last);
    void sendKeepAliveMessage();
    void sendDestroyMessage();

private:
    bool logged;
    long long session_id;
    long long handle_id;

    Client client;
    Client::connection_ptr connection;
    std::thread thread;
    std::thread thread_keepAlive;
    std::atomic<bool> is_running;

    std::string sanitizeString(const std::string & s);
    bool sendMessage(json msg, const char *name);
};
