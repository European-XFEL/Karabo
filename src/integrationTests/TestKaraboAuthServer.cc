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

// #include <belle.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/StringTools.hh>
#include <nlohmann/json.hpp>

// namespace Belle = OB::Belle;
namespace nl = nlohmann;
using namespace std;
using namespace karabo::util;

string TestKaraboAuthServer::VALID_TOKEN{"01234567-89ab-cdef-0123-456789abcdef"};

int TestKaraboAuthServer::VALID_ACCESS_LEVEL{static_cast<int>(Schema::AccessLevel::ADMIN)};

string TestKaraboAuthServer::INVALID_TOKEN_MSG{"Invalid one-time token!"};

class TestKaraboAuthServer::Impl {
   public:
    Impl(const std::string& addr, int port) : m_addr(addr), m_port(port) {}

    /**
     * @brief Runs the web server.
     *
     * @note This blocks the executing thread until there are no more handlers queued in the server event loop. The
     * internal Belle web server starts its own event loop and blocks the calling thread.
     */
    void run() {
        /*
        // Sets up the server
        m_srv.address(m_addr);
        m_srv.port(m_port);
        m_srv.threads(1);
        m_srv.ssl(false);

        // set default http headers
        Belle::Headers headers;
        headers.set(Belle::Header::server, "Belle");
        headers.set(Belle::Header::cache_control, "private; max-age=0");
        m_srv.http_headers(headers);

        // The handler for token authorization requests
        m_srv.on_http("/authorize_once_tk", Belle::Method::post, [](Belle::Server::Http_Ctx& ctx) {
            nl::json respObj = nl::json::parse(ctx.req.body());
            bool validToken = false;
            if (!respObj["tk"].is_null()) {
                const std::string reqTk = respObj["tk"];
                if (reqTk == TestKaraboAuthServer::VALID_TOKEN) {
                    validToken = true;
                }
            }
            ctx.res.set("content-type", "application/json");
            ctx.res.result(Belle::Status::ok);
            ctx.res.body() = validToken ? R"({"success": true, "user": "John", "error_msg": "", "visib_level": )" +
                                                toString(TestKaraboAuthServer::VALID_ACCESS_LEVEL) + "}"
                                        : R"({"success": false, "user": "", "visib_level": 0, "error_msg": ")" +
                                                TestKaraboAuthServer::INVALID_TOKEN_MSG + "\"}";
        });

        m_srv.listen();
        */
    }

   private:
    const std::string m_addr;
    int m_port;
    // Belle::Server m_srv;
};


TestKaraboAuthServer::TestKaraboAuthServer(const std::string& addr, int port) {
    m_impl = std::make_unique<Impl>(addr, port);
}

TestKaraboAuthServer::~TestKaraboAuthServer() = default;

void TestKaraboAuthServer::run() {
    m_impl->run();
}
