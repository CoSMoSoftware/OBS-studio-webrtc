#include "SpankChainWebsocketClientImpl.h"
#include "restclient-cpp/connection.h"
#include "json.hpp"
using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;


std::string urlencode(const std::string &s)
{
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for (int i = 0, ix = s.length(); i<ix; i++)
	{
		const char& c = s[i];
		if ((48 <= c && c <= 57)  || //0-9
		    (65 <= c && c <= 90)  || //abc...xyz
		    (97 <= c && c <= 122) || //ABC...XYZ
		    (c == '-' || c == '_' || c == '.' || c == '~')
		   )
		{
			e << c;
		} else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}

SpankChainWebsocketClientImpl::SpankChainWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    
    // Initialize ASIO
    client.init_asio();
}

SpankChainWebsocketClientImpl::~SpankChainWebsocketClientImpl()
{
    //Disconnect just in case
    disconnect(false);
}

bool SpankChainWebsocketClientImpl::connect(std::string url, long long room, std::string apiURL, std::string token, WebsocketClient::Listener* listener)
{
    websocketpp::lib::error_code ec;

    try
    {
        // Register our message handler
        client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
            //get response
            auto msg = json::parse(frame->get_payload());

            //Check if it is an event
            if (msg.find("type") == msg.end())
                //Ignore
                return;
            
            std::string type = msg["type"];
            
            //Check type
            if (type.compare("response") == 0)
            {
                //Get  transaction id
                if (msg.find("transId") == msg.end())
                    //Ignore
                    return;
                
                if (msg.find("data") == msg.end()){
                    //Ignore
                    return;
                }

                //Get the Data session
                auto data = msg["data"];
                
                //Get responsedata
                std::string sdp = data["sdp"];
                std::string feedId = data["feedId"];

		//Event
                listener->onOpened(sdp);

                std::cout << "Sending post with feedId: " << feedId << std::endl;

                //Crate body of the rest request
                std::string body = "{\"feedId\": \"" + feedId + "\"}";

                //Create authentication bearer
                std::string bearer = "Bearer " + token;

                //Send request
                RestClient::Connection conn(apiURL);
                conn.AppendHeader("Authorization", bearer);
                conn.AppendHeader("Content-Type", "application/json");
                //Make post
                auto ret = conn.put("/camshows/obs/update-show", body);

                //Keep the connection alive
                is_running.store(true);
            }
        });
        
        
        //When we are open
        client.set_open_handler([=](websocketpp::connection_hdl con){
            //Launch event
            listener->onConnected();
	    //And logged
            listener->onLogged(0);
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
        //Register our tls hanlder
        client.set_tls_init_handler([&](websocketpp::connection_hdl connection) {
            //Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
            
            try {
                ctx->set_options(asio::ssl::context::default_workarounds |
                                 asio::ssl::context::no_sslv2 |
                                 asio::ssl::context::single_dh_use);
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
            return ctx;
        });
	//Create websocket connection and add token and callback parameters
        std::string wss = url + "/?token=" + token + "&callback=" + urlencode(apiURL + "/camshows/auth/token/status");
        //Get connection
        connection = client.get_connection(wss, ec);
        
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

bool SpankChainWebsocketClientImpl::open(const std::string &sdp, const std::string& codec)
{
    try
    {
        //Login command
        json open = {
            { "type"		, "cmd"	    },
            { "name"		, "publish" },
            { "transId"		, 0         },
            { "data" ,
                {
                    { "sdp"    , sdp   },
                    { "name"   , "obs" },
		            { "codec"  , codec },
                    { "tracks" ,
                        {
                            { "audio"     , "audio"     },
                            { "webcam"    , "video"     },
                            { "thumbnail" , "thumbnail" },
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

bool SpankChainWebsocketClientImpl::trickle(const std::string &mid, int index, const std::string &candidate, bool last)
{
  return true;
}

bool SpankChainWebsocketClientImpl::disconnect(bool wait)
{
    
    try
    {
        //Stop client
        client.close(connection, websocketpp::close::status::normal, std::string("disconnect"));
        client.stop();
        
        //Don't wait for connection close
        if (thread.joinable())
        {
            //If sswe have to wait
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
