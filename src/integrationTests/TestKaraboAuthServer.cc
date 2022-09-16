/*
 * TestKaraboAuthServer.cc
 *
 * Created on 09.09.2022
 *
 * Copyright(C) European XFEL GmbH Hamburg.All rights reserved.
 */

#include "TestKaraboAuthServer.hh"

#include <karabo/util/Schema.hh>
#include <karabo/util/StringTools.hh>
#include <nlohmann/json.hpp>

namespace Belle = OB::Belle;
namespace nl = nlohmann;
using namespace std;
using namespace karabo::util;

string TestKaraboAuthServer::VALID_TOKEN{"01234567-89ab-cdef-0123-456789abcdef"};

int TestKaraboAuthServer::VALID_ACCESS_LEVEL{static_cast<int>(Schema::AccessLevel::ADMIN)};

string TestKaraboAuthServer::INVALID_TOKEN_MSG{"Invalid one-time token!"};


TestKaraboAuthServer::TestKaraboAuthServer(const std::string& addr, const int port) : m_addr(addr), m_port(port) {}

void TestKaraboAuthServer::run() {
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
        const string respBody = validToken ? R"({"success": true, "user": "John", "error_msg": "", "visib_level": )" +
                                                   toString(TestKaraboAuthServer::VALID_ACCESS_LEVEL) + "}"
                                           : R"({"success": false, "user": "", "visib_level": 0, "error_msg": ")" +
                                                   TestKaraboAuthServer::INVALID_TOKEN_MSG + "\"}";
        ctx.res.set("content-type", "application/json");
        ctx.res.result(Belle::Status::ok);
        ctx.res.body() = move(respBody);
    });

    m_srv.listen();
}
