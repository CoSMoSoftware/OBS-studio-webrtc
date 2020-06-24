// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include "WebsocketClient.h"

//Use http://think-async.com/ instead of boost
#define ASIO_STANDALONE

#define _WEBSOCKETPP_CPP11_STL_
#define _WEBSOCKETPP_CPP11_THREAD_
#define _WEBSOCKETPP_CPP11_FUNCTIONAL_
#define _WEBSOCKETPP_CPP11_SYSTEM_ERROR_
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_
#define _WEBSOCKETPP_CPP11_MEMORY_

#include "Evercast.h"
// NOTE ALEX: might need to be moved to Evercast.h
#define EVERCAST_MESSAGE_TIMEOUT 5.0

#include <websocketpp/common/connection_hdl.hpp>
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "nlohmann/json.hpp"

typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;

class EvercastWebsocketClientImpl : public WebsocketClient {
public:
  EvercastWebsocketClientImpl();
  ~EvercastWebsocketClientImpl();

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
          const std::string & /* Id */) override;
  bool trickle(
          const std::string & mid,
          const int index,
          const std::string & candidate,
          const bool last) override;
  bool disconnect(const bool wait) override;

  void keepConnectionAlive(WebsocketClient::Listener * listener);
  void destroy();

private:
  bool logged;
  long long session_id;
  long long handle_id;

  Client client;
  Client::connection_ptr connection;
  std::thread thread;
  std::thread thread_keepAlive;
  std::atomic<bool> is_running;

  std::chrono::time_point<std::chrono::system_clock> last_message_recd_time;

  std::string sanitizeString(const std::string & s);
  void handleDisconnect(
    websocketpp::connection_hdl connectionHdl,
    WebsocketClient::Listener * listener
  );
  void handleFail(
    websocketpp::connection_hdl connectionHdl,
    WebsocketClient::Listener * listener
  );
  void sendKeepAliveMessage();
  bool sendTrickleMessage(const std::string &, int, const std::string &, bool);
  bool sendOpenMessage(const std::string &sdp, const std::string &codec);
  void sendLoginMessage(std::string username, std::string token, std::string room);
  void sendAttachMessage();
  void sendJoinMessage(std::string room);
  void sendDestroyMessage();
  bool sendMessage(nlohmann::json msg, const char *name);
  int parsePluginErrorCode(nlohmann::json &msg);
  bool hasTimedOut();


};

