/*
 * TestKaraboAuthServer.cc
 *
 * Created on 09.09.2022
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "TestKaraboAuthServer.hh"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include "karabo/data/types/Schema.hh"
#include "karabo/data/types/StringTools.hh"

namespace nl = nlohmann;
using namespace std;
using namespace karabo::data;

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

string TestKaraboAuthServer::VALID_TOKEN{"01234567-89ab-cdef-0123-456789abcdef"};

int TestKaraboAuthServer::VALID_ACCESS_LEVEL{static_cast<int>(Schema::AccessLevel::OPERATOR)};

string TestKaraboAuthServer::INVALID_TOKEN_MSG{"Invalid one-time token!"};

string TestKaraboAuthServer::VALID_USER_ID{"Bob"};

class TestKaraboAuthServer::Impl {
   public:
    Impl(const std::string& addr, unsigned short port) : m_addr(addr), m_port(port) {}

    /**
     * @brief Runs the web server.
     *
     * @note This blocks the executing thread until there are no more handlers queued in the server event loop.
     *       The server event loop is created by this method.
     */
    void run() {
        const net::ip::address addr{net::ip::make_address(m_addr)};
        net::io_context ioc{1 /* num of threads */};

        // Receives incoming connections
        tcp::acceptor acceptor{ioc, {addr, m_port}};

        for (;;) {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session
            do_session(socket);
        }
    }

   private:
    const std::string m_addr;
    unsigned short m_port;

    /**
     * @brief Reports a failure.
     */
    void fail(beast::error_code ec, char const* what) {
        std::cerr << what << ": " << ec.message() << "\n";
    }


    /**
     * @brief Handles a POST request expected to carry a one-time authentication token as the
     * value of the field "tk" in its body.
     */
    template <class Body, class Allocator>
    http::message_generator handle_request(http::request<Body, http::basic_fields<Allocator>>&& req) {
        const std::string& serverId = "TestKaraboAuthServer";
        if (req.method() != http::verb::post) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::server, serverId);
            res.set(http::field::content_type, "text/html");
            res.body() = std::string{"Unsupported method - only POST is supported."};
            res.prepare_payload();
            return res;
        }
        // Handles a POST request.
        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, serverId);
        res.set(http::field::content_type, "application/json");
        nl::json respObj = nl::json::parse(req.body());
        bool validToken = false;
        if (!respObj["tk"].is_null()) {
            const std::string reqTk = respObj["tk"];
            if (reqTk == TestKaraboAuthServer::VALID_TOKEN) {
                validToken = true;
            }
        }
        res.body() = validToken ? R"({"success": true, "username":")" + TestKaraboAuthServer::VALID_USER_ID +
                                        R"(", "error_msg": "", "visibility": )" +
                                        toString(TestKaraboAuthServer::VALID_ACCESS_LEVEL) + "}"
                                : R"({"success": false, "username": "", "visibility": 0, "error_msg": ")" +
                                        TestKaraboAuthServer::INVALID_TOKEN_MSG + "\"}";
        res.prepare_payload();
        return res;
    }

    /**
     * @brief Handles a client connection and (to keep things as simple as possible) a single request.
     * The connection is closed right after the response is sent back to the client.
     */
    void do_session(tcp::socket& socket) {
        beast::error_code ec;

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;

        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec == http::error::end_of_stream) return; // Lost connection to client
        if (ec) return fail(ec, "read");

        // Handle request
        http::message_generator resp = handle_request(std::move(req));

        // Send the response
        beast::write(socket, std::move(resp), ec);

        if (ec) return fail(ec, "write");

        // Send a TCP shutdown
        socket.shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
}; // TestKaraboAuthServer::Impl


TestKaraboAuthServer::TestKaraboAuthServer(const std::string& addr, int port) {
    m_impl = std::make_unique<Impl>(addr, port);
}

TestKaraboAuthServer::~TestKaraboAuthServer() = default;

void TestKaraboAuthServer::run() {
    m_impl->run();
}
