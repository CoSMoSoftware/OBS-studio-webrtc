#include "MillicastWebsocketClientImpl.h"
#include "json.hpp"

using json = nlohmann::json;

typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

MillicastWebsocketClientImpl::
MillicastWebsocketClientImpl()
{
  // Set logging to be pretty verbose (everything except message payloads)
  client.set_access_channels(   websocketpp::log::alevel::all           );
  client.clear_access_channels( websocketpp::log::alevel::frame_payload );
  client.set_error_channels(    websocketpp::log::elevel::all           );
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
  const std::string & url,
  const std::string & /* unused room        */,
  const std::string & /* unused username    */,
  const std::string & token,
  Listener*           listener
)
{
  websocketpp::lib::error_code ec;
  try
  {

    // --- Handler
    client.set_tls_init_handler([&](websocketpp::connection_hdl /* unused con */) {
      // Create context
      auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);

      try {
        // Removes support for undesired TLS versions
        ctx->set_options(
          asio::ssl::context::default_workarounds |
          asio::ssl::context::no_sslv2 |
          asio::ssl::context::no_sslv3 |
          asio::ssl::context::single_dh_use
        );

      } catch (std::exception &e) {
        std::cout << "> exception: " << e.what() << std::endl;
      }
      return ctx;
    });

    // Copy token
    this->token = token;
    // remove space in the token
    this->token.erase(remove_if(this->token.begin(), this->token.end(), isspace), this->token.end());
    // Create websocket connection and add token and callback parameters
    std::string wss = url + "?token=" + this->token;
    std::cout << " Connection URL: " << wss  << std::endl;
    // Get connection
    this->connection = client.get_connection(wss, ec);

    if (!this->connection)
      std::cout << "Print NOT NULLL" << std::endl;
    
    connection->set_close_handshake_timeout(5000);

    if (ec) {
      std::cout << "could not create connection because: " << ec.message() << std::endl;
      return 0;
    }

    // --- Register our message handler
    connection->set_message_handler( [=](websocketpp::connection_hdl /* unused con */, message_ptr frame) {
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
        if (msg.find("data") == msg.end())
          return;

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
      // NOTE ALEX: simpler with an elseif here)
      // If error message
      if (type.compare("error") == 0 ){
        listener->onDisconnected();
      }
    });  // --- Handler


    // --- open Hanlder
    connection->set_open_handler([=](websocketpp::connection_hdl /* unused con */){
      // Launch event
      listener->onConnected();
      std::cout << "> Error ON Disconnect close: " << ec.message() << std::endl;
      // And logged
      listener->onLogged(0);
    });

    // ---  Set close hanlder
    connection->set_close_handler([=](...) {
      // Call listener
      std::cout << "> set_close_handler called" << std::endl; 
      // Don't wait for connection close
      thread.detach();
      // Remove connection
      connection = nullptr;
      listener->onDisconnected();
    });

    // --- Set failure handler
    connection->set_fail_handler([=](...) {
      //Call listener
      listener->onDisconnected();
    });

    connection->set_http_handler([=](...) {
      std::cout << "> https called" << std::endl; 
    });

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

  return true;
}

bool
MillicastWebsocketClientImpl::
trickle(
  const std::string& /* unused mid       */,
  int                /* unused index     */,
  const std::string& /* unused candidate */,
  bool               /* unused last      */
)
{
  return true;
}

bool
MillicastWebsocketClientImpl::
disconnect(
  bool /* unused wait */
)
{  
  websocketpp::lib::error_code ec;
  if (!connection){
    return true;
  }

  try
  { 
    json close = {
      { "type"    , "cmd" },
      { "name"    , "unpublish" },
    };

    if (connection->send(close.dump()))
      return false;

    // wait for unpublish message is sent
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    client.close(connection, websocketpp::close::status::normal, "", ec);
    client.stop();
    
    client.set_open_handler([](...){});
    client.set_close_handler([](...){});
    client.set_fail_handler([](...) {});
    //Detach trhead
    thread.detach();
  }  catch (websocketpp::exception const & e) {
    std::cout << e.what() << std::endl;
    return false;
  }

  return true;
}
