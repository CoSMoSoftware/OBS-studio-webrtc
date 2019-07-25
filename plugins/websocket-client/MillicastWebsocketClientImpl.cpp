#include "MillicastWebsocketClientImpl.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

MillicastWebsocketClientImpl::MillicastWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

MillicastWebsocketClientImpl::~MillicastWebsocketClientImpl()
{
    // Disconnect just in case
    disconnect(false);
}

bool MillicastWebsocketClientImpl::connect(
        const std::string & url,
        const std::string & /* room */,
        const std::string & /* username */,
        const std::string & token,
        WebsocketClient::Listener * listener)
{
    websocketpp::lib::error_code ec;
    try {
        // --- TLS handler
        client.set_tls_init_handler([&](websocketpp::connection_hdl /* con */) {
            // Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
            try {
                // Remove support for undesired TLS versions
                ctx->set_options(
                        asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 |
                        asio::ssl::context::no_sslv3 |
                        asio::ssl::context::single_dh_use);
            } catch (std::exception & e) {
                std::cout << "tls exception: " << e.what() << std::endl;
            }
            return ctx;
        });

        // Copy token
        this->token = token;
        // Remove space in the token
        this->token.erase(remove_if(this->token.begin(), this->token.end(), isspace), this->token.end());

        // Create websocket url
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

        // --- Message handler
        connection->set_message_handler([=](websocketpp::connection_hdl /* con */, message_ptr frame) {
            // Get response
            auto msg = json::parse(frame->get_payload());
            std::cout << "msg received: " << msg << std::endl;

            // If there is no type, do nothing and get out of here
            if (msg.find("type") == msg.end())
                return;

            std::string type = msg["type"];
            if (type.compare("response") == 0) {
                if (msg.find("transId") == msg.end())
                    return;
                if (msg.find("data") == msg.end())
                    return;
                // Get the data session
                auto data = msg["data"];
                // Get response data
                std::string sdp = data["sdp"];
                std::string feedId = data["feedId"];
                // Event
                listener->onOpened(sdp);
                // Keep the connection alive
                is_running.store(true);
            } else if (type.compare("error") == 0) {
                listener->onDisconnected();
            }
        });

        // --- Open handler
        connection->set_open_handler([=](websocketpp::connection_hdl /* con */) {
            // Launch event
            listener->onConnected();
            // Launch logged event
            listener->onLogged(0);
        });

        // --- Close handler
        connection->set_close_handler([=](...) {
            std::cout << "> set_close_handler called" << std::endl;
            // Don't wait for connection close
            thread.detach();
            // Remove connection
            connection = nullptr;
            // Call listener
            listener->onDisconnected();
        });

        // -- Failure handler
        connection->set_fail_handler([=](...) {
            listener->onDisconnected();
        });

        // -- HTTP handler
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
    } catch (const websocketpp::exception & e) {
        std::cout << "connect exception: " << e.what() << std::endl;
        return false;
    }
    // OK
    return true;
}

bool MillicastWebsocketClientImpl::open(
        const std::string & sdp,
        const std::string & codec,
        const std::string & milliId)
{
    std::cout << "WS-OPEN: milliId: " << milliId << std::endl;
    std::string video_codec = "vp8";
    if (!codec.empty())
        video_codec = codec;
    try {
        // json data_no_codec = {
        //     { "streamId", milliId },
        //     { "name", milliId },
        //     { "sdp" , sdp }
        // };
        json data_with_codec = {
            { "streamId", milliId },
            { "name", milliId },
            { "sdp" , sdp },
            { "codec", video_codec }
        };
        // Publish command (send offer)
        json open = {
            { "type", "cmd" },
            { "name", "publish" },
            { "transId", 0 },
            { "data", data_with_codec }
        };
        // json open = {
        //     { "type", "cmd" },
        //     { "name", "publish" },
        //     { "transId", 0 },
        //     { "data", codec.empty() ? data_no_codec : data_with_codec }
        // };
        // Serialize and send
        if (connection->send(open.dump()))
            return false;
    } catch (const websocketpp::exception & e) {
        std::cout << "open exception: " << e.what() << std::endl;
        return false;
    }
    // OK
    return true;
}

bool MillicastWebsocketClientImpl::trickle(
        const std::string & /* mid */,
        int /* index */,
        const std::string & /* candidate */,
        bool /* last */)
{
    return true;
}

bool MillicastWebsocketClientImpl::disconnect(bool /* wait */)
{
    if (!connection)
        return true;
    websocketpp::lib::error_code ec;
    try {
        json close = {
            { "type", "cmd" },
            { "name", "unpublish" }
        };
        // Serialize and send
        if (connection->send(close.dump()))
            return false;
        // Wait for unpublish message to be sent
        std::this_thread::sleep_for(std::chrono::seconds(2));
        // Stop client
        if (connection->get_state() == websocketpp::session::state::open)
            client.close(connection, websocketpp::close::status::normal, std::string("disconnect"), ec);
        if (ec)
            std::cout << "> Error on disconnect close: " << ec.message() << std::endl;
        client.stop();
        // Remove handlers
        client.set_open_handler([](...) {});
        client.set_close_handler([](...) {});
        client.set_fail_handler([](...) {});
        // Detach thread
        if (thread.joinable())
            thread.detach();
    } catch (const websocketpp::exception & e) {
        std::cout << "disconnect exception: " << e.what() << std::endl;
        return false;
    }
    return true;
}
