#include "MillicastWebsocketClientImpl.h"
#include "json.hpp"

using json = nlohmann::json;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

MillicastWebsocketClientImpl::
MillicastWebsocketClientImpl()
{
  // Set logging to be pretty verbose (everything except message payloads)
  client.set_access_channels(websocketpp::log::alevel::all);
  client.clear_access_channels(websocketpp::log::alevel::frame_payload);
  client.set_error_channels(websocketpp::log::elevel::all);
  
  // Initialize ASIO
  client.init_asio();
}

MillicastWebsocketClientImpl::
~MillicastWebsocketClientImpl()
{
  // Disconnect just in case
  disconnect(false);
}

bool
MillicastWebsocketClientImpl::
connect(
  std::string url,
  long long   room,
  std::string apiURL,
  std::string token,
  WebsocketClient::Listener* listener
)
{
  websocketpp::lib::error_code ec;
  try
  {
    // Register our message handler
    client.set_message_handler( [=](websocketpp::connection_hdl con, message_ptr frame) {
      //get response
      auto msg = json::parse(frame->get_payload());

      std::cout << "msg received: " << msg << std::endl;

      // If there is no type, do nothing and get out of here
      if (msg.find("type") == msg.end())
        return;
      
      std::string type = msg["type"];
      
      // If we're not dealing with a response, then act on it
      if (type.compare("response") == 0)
      {
        // If there is no transId, do nothing and get out of here
        if (msg.find("transId") == msg.end())
          return;

        // If there is no data, do nothing and get out of here
        if (msg.find("data") == msg.end()){
          return;
        }

        // Get the Data session
        auto data = msg["data"];
        
        // Get responsedata
        std::string sdp    = data["sdp"];
        std::string feedId = data["feedId"];

        //Event
        listener->onOpened(sdp);

        //Keep the connection alive
        is_running.store(true);
      }
      // If error message
      if (type.compare("error") == 0 ){
        listener->onDisconnected();
      }
    });
    
    // When we are open
    client.set_open_handler([=](websocketpp::connection_hdl con){
      // Launch event
      listener->onConnected();
      // And logged
      listener->onLogged(0);
    });

    // Set close hanlder
    client.set_close_handler([=](...) {
      // Call listener
      listener->onDisconnected();
    });

    // Set failure handler
    client.set_fail_handler([=](...) {
      //Call listener
      listener->onDisconnected();
    });

    // Register our tls hanlder
    client.set_tls_init_handler([&](websocketpp::connection_hdl connection) {
      //Create context
      auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
      
      try {
        ctx->set_options(
          asio::ssl::context::default_workarounds |
          asio::ssl::context::no_sslv2 |
          asio::ssl::context::single_dh_use
        );
      }
      catch (std::exception& e) {
        std::cout << e.what() << std::endl;
      }
      return ctx;
    });

    // Remove space to avoid errors.
    token.erase(remove_if(token.begin(), token.end(), isspace), token.end());

    std::string wss = url + "?token=" + token;

    // Create websocket connection and add token and callback parameters
    std::cout << " Connection URL: " << wss  << std::endl;

    // Get connection
    connection = client.get_connection(wss, ec);
    
    if (ec) {
      std::cout << "could not create connection because: " << ec.message() << std::endl;
      return 0;
    }
    
    // Note that connect here only requests a connection. No network messages are
    // exchanged until the event loop starts running in the next line.
    client.connect(connection);
    // Async
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
  // OK
  return true;
}

bool
MillicastWebsocketClientImpl::
open(
  const std::string& sdp,
  const std::string& codec,
  const std::string& milliId
)
{
  std::cout << "WS-OPEN: milliId: " << milliId << std::endl;

  try
  {
    // Login command
    json open = {
      { "type"    , "cmd"     },
      { "name"    , "publish" },
      { "transId" , 0         },
      { "data" ,
        {
          { "streamId", milliId },
          { "name"    , milliId },
          { "sdp"     , sdp     },
          { "codec"   , codec   },
        }
      }
    };
    // Serialize and send
    if (connection->send(open.dump()))
      return false;
  }
  catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
    return false;
  }
  // OK
  return true;
}

bool MillicastWebsocketClientImpl::trickle(const std::string &mid, int index, const std::string &candidate, bool last)
{
  return true;
}

bool MillicastWebsocketClientImpl::disconnect(bool wait)
{
  
  try
  {
    // Stop client
    client.close(connection, websocketpp::close::status::normal, std::string("disconnect"));
    client.stop();
    
    // Don't wait for connection close
    if (thread.joinable())
    {
      //If sswe have to wait
      if (wait) {
        thread.join();
      } else {
        // Remove handlers
        client.set_open_handler( [](...){});
        client.set_close_handler([](...){});
        client.set_fail_handler( [](...){});
        // Detach thread
        thread.detach();
      }
    }
    
  }
  catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
    return false;
  }
  // OK
  return true;
}
