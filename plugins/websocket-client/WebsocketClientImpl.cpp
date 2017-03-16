#include "WebsocketClientImpl.h"
#include "json.hpp"
using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

WEBSOCKETCLIENT_API WebsocketClient* createWebsocketClient(void)
{
	return new WebsocketClientImpl();
}

WebsocketClientImpl::WebsocketClientImpl()
{
	// Set logging to be pretty verbose (everything except message payloads)
	client.set_access_channels(websocketpp::log::alevel::all);
	client.clear_access_channels(websocketpp::log::alevel::frame_payload);

	// Initialize ASIO
	client.init_asio();
}

WebsocketClientImpl::~WebsocketClientImpl()
{
	//Disconnect just in case
	disconnect(false);
}

bool WebsocketClientImpl::connect(std::string url, std::string room, std::string username, std::string token, WebsocketClient::Listener* listener)
{
	websocketpp::lib::error_code ec;
	
	//reset loggin flag
	logged = false;

	try 
	{
		// Register our message handler
		client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
			const char* x = frame->get_payload().c_str();
			//get response
			auto msg = json::parse(frame->get_payload());

			
			//Check if it is an event
			if (msg.find("id") == msg.end())
				//Ignore
				return;

			//Get id
			std::string id = msg["id"];
			//Get payload
			auto payload = msg["payload"];
			//Get response
			std::string response = msg["response"];

			//Check type
			if (id.compare("login") == 0)
			{
				//If it is an error 
				if (response.compare("error") != 0)
				{
					//Server is sending response twice, ingore second one
					if (!logged)
					{
						//Get response code
						int result = payload["result"];
						//Launch logged event
						listener->onLogged(result);
						//Logged
						logged = true;
					}
				} else {
					//Get error code
					int code = payload["code"];
					//Error event
					listener->onLoggedError(code);
				}
			} 
			else if (id.compare("open") == 0) 
			{
				//If it is an error 
				if (response.compare("error") != 0)
				{
					//Get response code
					std::string sdp = payload["jsep"]["sdp"];
					//Launch logged event
					listener->onOpened(sdp);
				}
				else {
					//Get error code
					int code = payload["code"];
					//Error event
					listener->onOpenedError(code);
				}
			}
		});

		//Register our tls hanlder
		client.set_tls_init_handler([&](websocketpp::connection_hdl connection) {
			//Create context
			auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv1);

			try {
				ctx->set_options(asio::ssl::context::default_workarounds |
					asio::ssl::context::no_sslv2 |
					asio::ssl::context::no_sslv3 |
					asio::ssl::context::single_dh_use);
			}
			catch (std::exception& e) {
				std::cout << e.what() << std::endl;
			}
			return ctx;
		});

		//When we are open
		client.set_open_handler([=](...){
			//Launch event
			listener->onConnected();
			//Login command
			json login = {
				{"request"	, "login"	},	
				{"id"		, "login"	},
				{ "payload" ,
					{
						{ "username"	, username	},
						{ "token"		, token		},
						{ "room"		, room		}
					}
				}
			};
			//Serialize and send
			connection->send(login.dump());
		});
		//Set close hanlder
		client.set_close_handler([=](...) {
			//Call listener
			listener->onDisconnected();
		});
		//Set failure handler
		client.set_fail_handler([=](...) {
			//Call listener
			listener->onDisconnected();
		});
		//Get connection
		connection = client.get_connection(url, ec);

		if (ec) {
			std::cout << "could not create connection because: " << ec.message() << std::endl;
			return 0;
		}

		// Note that connect here only requests a connection. No network messages are
		// exchanged until the event loop starts running in the next line.
		client.connect(connection);

		//Async
		thread = std::thread([&]() {
			// Start the ASIO io_service run loop
			// this will cause a single connection to be made to the server. c.run()
			// will exit when this connection is closed.
			client.run();
		});
		
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	//OK
	return true;
}

bool WebsocketClientImpl::open(const std::string &sdp)
{
	try 
	{
		//Login command
		json open = {
			{ "request"	, "open"	},
			{ "id"		, "open"	},
			{ "payload" ,
				{
					{ "medium"	,"sfuvideo" },
					{ "jsep" ,
						{
							{ "type"	,  "offer"	},
							{ "sdp"		,	sdp		},
						}
					}
				}
			}
		};
		//Serialize and send
		if (connection->send(open.dump()))
			return false;
	} 
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	//OK
	return true;
}
bool WebsocketClientImpl::trickle(const std::string &mid, int index, const std::string &candidate, bool last)
{
	try 
	{
		//Check if it is last
		if (!last)
		{
			//Login command
			json trickle = {
				{ "request"	, "trickle" },
				{ "id"		, "trickle" + std::to_string(rand()) },
				{ "payload" ,
					{
						{ "medium"	,"sfuvideo" },
						{ "candidate" , 
							{
								{ "sdpMid"			, mid		},
								{ "sdpMLineIndex"	, index		} ,
								{ "candidate"		, candidate	}
							}
						}
					}
				}
			};
			//Serialize and send
			if (connection->send(trickle.dump()))
				return false;
			//OK
			return true;
		}
		else
		{
			//Login command
			json trickle = {
				{ "request"	, "trickle" },
				{ "id"		, "trickle" + std::to_string(rand()) },
				{ "payload" ,
					{
						{ "medium"	,"sfuvideo" },
						{ "candidate" ,
							{
								{ "completed"			, true },
							}
						}
					}
				}
			};
			//Serialize and send
			if (connection->send(trickle.dump()))
				return false;
		}
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	//OK
	return true;
}

bool WebsocketClientImpl::disconnect(bool wait)
{
	try
	{
		//Stop
		client.close(connection, websocketpp::close::status::normal, std::string("disconnect"));
		client.stop();
		//Don't wait for connection close
		if (thread.joinable())
		{
			//If we have to wait
			if (wait) {

				thread.join();
			}
			else {
				//Remov hanlders
				client.set_open_handler([](...){});
				client.set_close_handler([](...){});
				client.set_fail_handler([](...) {});
				//Detach trhead
				thread.detach();
			}
		}
	}
	catch (websocketpp::exception const & e) {
		std::cout << e.what() << std::endl;
		return false;
	}
	//OK
	return true;
}