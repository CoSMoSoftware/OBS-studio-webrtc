// Copyright Dr. Alex. Gouaillard (2015, 2020)

#include "EvercastWebsocketClientImpl.h"
#include "nlohmann/json.hpp"

#include <util/base.h>

#include <iostream>
#include <string>

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

EvercastWebsocketClientImpl::EvercastWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

EvercastWebsocketClientImpl::~EvercastWebsocketClientImpl()
{
    // Disconnect just in case
    disconnect(false);
}

bool EvercastWebsocketClientImpl::connect(
        const std::string & raw_url,
        const std::string & raw_room,
        const std::string & raw_username,
        const std::string & raw_token,
        WebsocketClient::Listener * listener)
{
    using namespace std::placeholders;

    websocketpp::lib::error_code ec;
    std::string url      = sanitizeString(raw_url);
    std::string room     = sanitizeString(raw_room);
    std::string username = sanitizeString(raw_username);
    std::string token    = sanitizeString(raw_token);

    // Reset login flag
    logged = false;
    try {
        // --- Message handler
        client.set_message_handler([=](
            websocketpp::connection_hdl /* con */, message_ptr frame) {
            const char* x = frame->get_payload().c_str();
            info("MESSAGE RECEIVED:\n%s\n", x);
            last_message_recd_time = std::chrono::system_clock::now();
            json msg = json::parse(frame->get_payload());

            if (msg.find("janus") == msg.end())
                return;
            // NOTE ALEX: double check if this change is valid,
            // and if it is needed for Janus too.
            // if (msg.find("ack") != msg.end())
            //    return;

            // Check if it is an event
            if (msg.find("jsep") != msg.end()) {
                std::string sdp = msg["jsep"]["sdp"];
                // Event
                listener->onOpened(sdp);
                return;
            }

            // Check type
            std::string event = msg["janus"];

            // NOTE ALEX: double check if this change is valid,
            // and if it is needed for Janus too.
            if (event == "ack")
              return;

            if (event.compare("success") == 0) {
                if (msg.find("transaction") == msg.end())
                    return;
                if (msg.find("data") == msg.end())
                    return;
                // Get the data session
                auto data = msg["data"];
                // Server is sending response twice, ignore second one
                if (!logged) {
                    // Get response code
                    session_id = data["id"];
                    sendAttachMessage();
                    logged = true;
                    // Keep the connection alive
                    is_running.store(true);
                    thread_keepAlive = std::thread([&]() {
                        keepConnectionAlive(listener);
                    });
                } else { // logged
                    handle_id = data["id"];
                    sendJoinMessage(room);
                    // Launch logged event
                    listener->onLogged(session_id);
                }
            }

            // Evercast Error response handling
            if (event == "event")
            {
                int error_code = parsePluginErrorCode(msg);

                if (error_code == EVERCAST_ERR_DUPLICATE_USER)
                {
                    // Someone is already using that ID, probably a previous version of us.  Log in with a fresh ID.
                    logged = false;
                    session_id = 0;
                    handle_id = 0;
                    sendLoginMessage(username, token, room);

                    // Keepalive stuff to prevent doubling-up on keepalive threads
                    is_running.store(false);
                    if (thread_keepAlive.joinable())
                    {
                        thread_keepAlive.join();
                    }

                    // Launch logged event
                    listener->onLogged(session_id);
                    return;
                }
                else if (error_code != EVERCAST_SUCCESS)
                {
                     warn("Unexpected Evercast error response:\n%s\n", x);
                }
            }
        });

        // --- Open handler
        client.set_open_handler([=](websocketpp::connection_hdl /* con */) {
            // Launch event
            listener->onConnected();
            sendLoginMessage(username, token, room);
        });

        // --- Close handler
        client.set_close_handler(
            std::bind(
                &EvercastWebsocketClientImpl::handleDisconnect,
                this, _1, listener
            )
        );

        // --- Failure handler
        client.set_fail_handler(
                std::bind(
                &EvercastWebsocketClientImpl::handleFail,
                this, _1, listener
            )
        );

        // --- TLS handler
        client.set_tls_init_handler([&](websocketpp::connection_hdl /* con */) {
            // Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(
                    asio::ssl::context::tlsv12_client);
            try {
                // Remove support for undesired TLS versions
                ctx->set_options(
                        asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::single_dh_use);
            } catch (std::exception & e) {
                warn("TLS exception: %s", e.what());
            }
            return ctx;
        });

        // Create Evercast websocket url
        std::string wss = url + "/?token=" + token + "&roomId=" + room;
        info("Connection URL:   %s", wss.c_str());

        connection = client.get_connection(wss, ec);
        if (ec) {
            error("Error establishing websocket connection: %s", ec.message().c_str());
            return 0;
        }
        connection->add_subprotocol("janus-protocol");

        // Note that connect here only requests a connection. No network messages
        // exchanged until the event loop starts running in the next line.
        client.connect(connection);
        // Async
        thread = std::thread([&]() {
            // Start ASIO io_service run loop
            // (single connection will be made to the server)
            client.run();  // will exit when this connection is closed
        });
    } catch (const websocketpp::exception & e) {
        warn("connect exception: %s", e.what());
        return false;
    }
    // OK
    return true;
}

bool
EvercastWebsocketClientImpl::
open(
        const std::string & sdp,
        const std::string & codec,
        const std::string & /* Id */)
{
    bool result = false;
    try {
        result = sendOpenMessage(sdp, codec);
    } catch (const websocketpp::exception & e) {
        warn("open exception: %s", e.what());
        return false;
    }
    return result;
}

bool
EvercastWebsocketClientImpl::
trickle(
        const std::string & mid,
        int index,
        const std::string & candidate,
        bool last)
{
    bool result = false;
    try {
        result = sendTrickleMessage(mid, index, candidate, last);
    } catch (const websocketpp::exception & e) {
        warn("trickle exception: %s", e.what());
        return false;
    }
    return result;
}

void
EvercastWebsocketClientImpl::
keepConnectionAlive(
  WebsocketClient::Listener * listener
)
{
    while (is_running.load()) {
        if (connection) {
            if (hasTimedOut()) {
                warn("Connection timed Out.");
                listener->onDisconnected();
                break;
            }
            try {
                sendKeepAliveMessage();
            } catch (const websocketpp::exception &e) {
                warn("keepConnectionAlive exception: %s", e.what());
            }
        }
    }
}

void
EvercastWebsocketClientImpl::
destroy()
{
    if (connection) {
        try {
            sendDestroyMessage();
        } catch (const websocketpp::exception & e) {
            warn("destroy exception: %s", e.what());
        }
    }
}

bool
EvercastWebsocketClientImpl::
disconnect(bool wait)
{
    destroy();
    if (!connection)
        return true;
    websocketpp::lib::error_code ec;
    try {
        // Stop keepAlive
        if (thread_keepAlive.joinable()) {
           is_running.store(false);
            thread_keepAlive.join();
        }
        // Stop client
        if (connection->get_state() == websocketpp::session::state::open)
            client.close(connection, websocketpp::close::status::normal, std::string("disconnect"), ec);
        if (ec)
            warn("> Error on disconnect close: %s", ec.message().c_str());
        // Don't wait for connection close
        client.stop();
        if (wait && thread.joinable()) {
            thread.join();
        } else {
            // Remove handlers
            client.set_open_handler([](...) {});
            client.set_close_handler([](...) {});
            client.set_fail_handler([](...) {});
            if (thread.joinable())
                thread.detach();
        }
    } catch (const websocketpp::exception & e) {
        warn("disconnect exception: %s", e.what());
        return false;
    }
    // OK
    return true;
}



//---------------------------------------------------------
//  WebSocket API Handlers / Callbacks
//---------------------------------------------------------

void
EvercastWebsocketClientImpl::
handleDisconnect(
    websocketpp::connection_hdl connectionHdl,
    WebsocketClient::Listener * listener)
{
    UNUSED_PARAMETER(connectionHdl);
    
    info("> set_close_handler > handleDisconnect called");
    if (listener)
    {
        listener->onDisconnected();
    }
}

void
EvercastWebsocketClientImpl::
handleFail(
    websocketpp::connection_hdl connectionHdl,
    WebsocketClient::Listener * listener)
{
    UNUSED_PARAMETER(connectionHdl);

    info("> set_fail_handler > handleFail called");
    if (listener)
    {
        if (hasTimedOut()) {
             warn("Connection Timed out");
             listener->onDisconnected();
         } else {
             listener->onLoggedError(-1);
         }
    }
}

//--------------------------------------
// Janus Core API implementation
//--------------------------------------

// NOTE ALEX: this is specific to a plugin
// and the plugin name is hardcoded. Could be improved
// Also, a Janus core can attach to several plugins.
void
EvercastWebsocketClientImpl::
sendAttachMessage()
{
     // Create handle command
     json attachPlugin = {
             { "janus", "attach" },
             { "transaction", std::to_string(rand()) },
             { "session_id", session_id },
             { "plugin", "janus.plugin.lua" }
     };
     sendMessage(attachPlugin, "attach");
}

// Core or videoRoom? 
void
EvercastWebsocketClientImpl::
sendLoginMessage(std::string username, std::string token, std::string room)
{
     // Login command
     json login = {
             { "janus", "create" },
             { "transaction", std::to_string(rand()) },
             { "payload",
                     {
                             { "username", username },
                             { "token", token },
                             { "room", room }
                     }
             }
     };
     sendMessage(login, "login");
}

void
EvercastWebsocketClientImpl::
sendKeepAliveMessage()
{
    json keepaliveMsg = {
        {"janus",       "keepalive"},
        {"session_id",  session_id},
        {"transaction", "keepalive-" + std::to_string(rand())}
    };
    sendMessage(keepaliveMsg, "keep-alive");
}

void
EvercastWebsocketClientImpl::
sendDestroyMessage()
{
    json destroyMsg = {
             { "janus", "destroy" },
             { "session_id", session_id },
             { "transaction", std::to_string(rand()) }
    };
    sendMessage(destroyMsg, "destroy");
}

//-------------------------------------------
// Janus VideoRoom Plugin API implememtation
//-------------------------------------------


// NOTE ALEX: The display name is hardcoded here. FIXME.
void
EvercastWebsocketClientImpl::
sendJoinMessage(std::string room)
{
    json joinRoom = {
         { "janus", "message" },
         { "transaction", std::to_string(rand()) },
         { "session_id", session_id },
         { "handle_id", handle_id },
         { "body",
                 {
                             { "room", room },
                             { "display", "EBS" },
                             { "ptype", "publisher" },
                             { "request", "join" }
                 }
         }
    };
    sendMessage(joinRoom, "join");
}

bool
EvercastWebsocketClientImpl::
sendTrickleMessage(
  const std::string &mid,
  int index,
  const std::string &candidate,
  bool last
)
{
    json trickle;
    if (!last) {
        trickle = {
                { "janus", "trickle" },
                { "handle_id", handle_id },
                { "session_id", session_id },
                { "transaction", "trickle" + std::to_string(rand()) },
                { "candidate",
                        {
                                { "sdpMid", mid },
                                { "sdpMLineIndex", index },
                                { "candidate", candidate }
                        }
                }
        };
    }
    else
    {
        trickle = {
                { "janus", "trickle" },
                { "handle_id", handle_id },
                { "session_id", session_id },
                { "transaction", "trickle" + std::to_string(rand()) },
                { "candidate",
                        {
                                { "completed", true }
                        }
                }
        };
    }

    return sendMessage(trickle, "trickle");
}

bool
EvercastWebsocketClientImpl::
sendOpenMessage(const std::string &sdp, const std::string &codec)
{
     json body_no_codec = {
             { "request", "configure" },
             { "muted", false },
             { "video", true },
             { "audio", true }
     };
     json body_with_codec = {
             { "request", "configure" },
             { "videocodec", codec },
             { "muted", false },
             { "video", true },
             { "audio", true }
     };
     // Send offer
     json open = {
             { "janus", "message" },
             { "session_id", session_id },
             { "handle_id", handle_id },
             { "transaction", std::to_string(rand()) },
             { "body", codec.empty() ? body_no_codec : body_with_codec },
             { "jsep",
                     {
                             { "type", "offer" },
                             { "sdp", sdp },
                             { "trickle", true }
                     }
             }
     };

      return sendMessage(open, "open");
}

//----------------------------------------
// Helpers
//----------------------------------------

// Helper - Sanitize the streams coming from UI
std::string
EvercastWebsocketClientImpl::
sanitizeString(const std::string & s)
{
    std::string _my_s = s;
    size_t p = _my_s.find_first_not_of(" \n\r\t");
    _my_s.erase(0, p);
    p = _my_s.find_last_not_of(" \n\r\t");
    if (p != std::string::npos)
        _my_s.erase(p + 1);
    return _my_s;
}

// Helper - Serialize and send message, with appropriate logging.
bool
EvercastWebsocketClientImpl::
sendMessage(json msg, const char *name)
{
    info("Sending %s message...", name);

    // Serialize and send
    if (connection->send(msg.dump()))
    {
        // this is a console log
        // do we want to double up with a UI error modal window?
        warn("Error sending %s message", name);
        return false;
    }

    return true;
}

// Helper - Evercast lua Plugin API error message parser
int
EvercastWebsocketClientImpl::
parsePluginErrorCode(json &msg)
 {
    if (msg.find("plugindata") == msg.end())
        return 0;

    auto plugindata = msg["plugindata"];
    if (plugindata.find("data") == plugindata.end())
        return 0;

    auto data = plugindata["data"];

    if (data.find("error_code") == data.end())
        return 0;

    int error_code = data["error_code"];
    return error_code;
}

// Helper - Websocket layer - Check for timeout
bool
EvercastWebsocketClientImpl::
hasTimedOut()
{
    auto current_time = std::chrono::system_clock::now();
    std::chrono::duration<double> gap = current_time - last_message_recd_time;
    return gap.count() > EVERCAST_MESSAGE_TIMEOUT;
}

