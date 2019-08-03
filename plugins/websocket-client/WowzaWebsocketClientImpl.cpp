#include "WowzaWebsocketClientImpl.h"
#include "restclient-cpp/connection.h"
#include "json.hpp"

#include "obs-defs.h"
#include "util/base.h"

#include <iostream>
#include <math.h>
#include <string>
#include <vector>

#define warn(format, ...)  blog(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  blog(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) blog(LOG_DEBUG,   format, ##__VA_ARGS__)
#define error(format, ...) blog(LOG_ERROR,   format, ##__VA_ARGS__)

using json = nlohmann::json;
typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

WowzaWebsocketClientImpl::WowzaWebsocketClientImpl()
{
    // Set logging to be pretty verbose (everything except message payloads)
    client.set_access_channels(websocketpp::log::alevel::all);
    client.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client.set_error_channels(websocketpp::log::elevel::all);
    // Initialize ASIO
    client.init_asio();
}

WowzaWebsocketClientImpl::~WowzaWebsocketClientImpl()
{
    // Disconnect just in case
    disconnect(false);
}

bool WowzaWebsocketClientImpl::connect(const std::string& url, const std::string& /* room */,
        const std::string& username, const std::string& token, Listener* listener)
{
    debug("WowzaWebsocketClientImpl::connect");

    appName = username;
    streamName = token;

    info("Application Name: %s", appName.c_str());
    info("Stream Name: %s", streamName.c_str());
    info("Server URL: %s", url.c_str());

    try {
        // Register TLS hanlder
        client.set_tls_init_handler([&](websocketpp::connection_hdl /* connection */) {
            // Create context
            auto ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::tlsv12_client);
            try {
                ctx->set_options(asio::ssl::context::default_workarounds |
                                 asio::ssl::context::no_sslv2 |
                                 asio::ssl::context::no_sslv3 |
                                 asio::ssl::context::single_dh_use);
            } catch (std::exception& e) {
                warn("TLS exception: %s", e.what());
            }
            return ctx;
        });

        // Get connection
        websocketpp::lib::error_code ec;
        this->connection = client.get_connection(url, ec);
        if (ec) {
            warn("Error establishing TLS connection: %s", ec.message().c_str());
            return 0;
        }
        if (!this->connection) {
            warn("** NO CONNECTION **");
            return 0;
        }

        // Register message handler
        connection->set_message_handler([=](websocketpp::connection_hdl /* con */, message_ptr frame) {
            bool candidateFound = false;
            const char* x = frame->get_payload().c_str();
            auto msg = json::parse(frame->get_payload());
            std::string sdp = "";
            json sdpData;

            if (msg.find("status") == msg.end())
                return;

            int status = msg["status"].get<int>();

            if (status == 200) {
                if (msg.find("iceCandidates") != msg.end()) {
                    candidateFound = true;
                    sdpData = msg["iceCandidates"];
                }
                if (msg.find("streamInfo") != msg.end()) {
                    std::string session_id_str = msg["streamInfo"]["sessionId"];
                    session_id = std::stoll(session_id_str);
                    info("Session ID: %s", session_id_str.c_str());
                }
                if (msg.find("sdp") != msg.end())
                    sdp = msg["sdp"]["sdp"];

                std::string command = msg["command"];

                if (command.compare("sendOffer") == 0) {
                    info("sendOffer response received:\n%s\n", x);
                    if (!sdp.empty())
                        listener->onOpened(sdp);
                    if (candidateFound) {
                        for (json j : sdpData) {
                            listener->onRemoteIceCandidate(j["candidate"].dump());
                        }
                        listener->onRemoteIceCandidate("");
                    }
                }
            } else {
                info("RECEIVED MESSAGE:\n%s\n", x);
                if (status > 499) {
                    warn("Server returned status code: %d", status);
                    listener->onLoggedError(OBS_OUTPUT_ERROR);
                }
            }
        });

        connection->set_open_handler([=](websocketpp::connection_hdl /* con */) {
            debug("** set_open_handler called **");
            listener->onConnected();
            listener->onLogged(0);
        });

        connection->set_close_handler([=](...) {
            debug("** set_close_handler called **");
            // Call listener
            listener->onDisconnected();
        });

        connection->set_fail_handler([=](...) {
            debug("** set_fail_handler called **");
            // Call listener
            listener->onDisconnected();
        });

        connection->set_interrupt_handler([=](...) {
            debug("** set_interrupt_handler called **");
            // Call listener
            listener->onDisconnected();
        });

        // Request a connection (no network messages are exchanged until event loop starts running)
        client.connect(connection);
        // Async
        thread = std::thread([&]() {
            debug("** starting ASIO io_service run loop **");
            // Start the ASIO io_service run loop (single connection will be made to the server)
            client.run(); // exits when this connection is closed
        });
    } catch (const websocketpp::exception& e) {
        error("WowzaWebsocketClientImpl::connect exception: %s" , e.what());
        return false;
    }
    return true;
}

bool WowzaWebsocketClientImpl::open(const std::string& sdp,
                                    const std::string& /* codec */,
                                    const std::string& /* milliId */)
{
    json offer = {
        { "direction", "publish" },
        { "command", "sendOffer" },
        { "streamInfo",
            {
                { "applicationName", appName },
                { "streamName", streamName },
                { "sessionId", "[empty]" }
            }
        },
        { "sdp",
            {
                { "type", "offer" },
                { "sdp", sdp }
            }
        }
    };
    info("Sending offer:\n%s\n", offer.dump().c_str());
    try {
        if (connection->send(offer.dump()))
            return false;
    } catch (const websocketpp::exception& e) {
        error("Error sending offer: %s", e.what());
        return false;
    }
    return true;
}

bool WowzaWebsocketClientImpl::trickle(const std::string& /* mid */, int /* index */,
                                       const std::string& candidate, bool /* last */)
{
    debug("Trickle candidate: %s", candidate.c_str());
    return true;
}

bool WowzaWebsocketClientImpl::disconnect(bool wait)
{
    debug("WowzaWebsocketClientImpl::disconnect");
    if (!connection)
        return true;
    websocketpp::lib::error_code ec;
    try {
        client.close(connection, websocketpp::close::status::normal, "", ec);
        client.stop();
        if (wait && thread.joinable()) {
            thread.join();
        } else {
            client.set_open_handler([](...){});
            client.set_close_handler([](...){});
            client.set_fail_handler([](...){});
            if (thread.joinable())
                thread.detach();
        }
    } catch (const websocketpp::exception& e) {
        error("WowzaWebsocketClientImpl::disconnect exception: %s", e.what());
        return false;
    }
    return true;
}