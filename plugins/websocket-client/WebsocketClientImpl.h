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

class WebsocketClientImpl : public WebsocketClient
{
public:
	WebsocketClientImpl();
	~WebsocketClientImpl();
	virtual bool connect(std::string url, std::string room, std::string username, std::string token, WebsocketClient::Listener* listener);
	virtual bool open(const std::string &sdp);
	virtual bool trickle(const std::string &mid, int index, const std::string &candidate, bool last);
	virtual bool disconnect(bool wait);
private:
	bool logged;
	std::thread thread;
	Client client;
	Client::connection_ptr connection;
};

