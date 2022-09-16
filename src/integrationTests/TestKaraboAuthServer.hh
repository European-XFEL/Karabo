/*
 * TestKaraboAuthServer.hh
 *
 * Created on 09.09.2022
 *
 * Copyright(C) European XFEL GmbH Hamburg.All rights reserved.
 */

#ifndef TESTKARABOAUTHSERVER_HH
#define TESTKARABOAUTHSERVER_HH

#include <belle.hh>
#include <string>

/**
 * @brief A bare-bones HTTP server that implements the one-time token validation endpoint of the KaraboAuthServer.
 * Created to mock the actual KaraboAuthServer for the integration tests of the GUI Server.
 *
 */
class TestKaraboAuthServer {
   public:
    /**
     * @brief Constructs an instance of the server ready to listen for connections on addr:port.
     *
     * @param addr The address (not hostname, as there's no resolver involved) the server should bind to.
     * @param port The port the server should bind to.
     */
    TestKaraboAuthServer(const std::string& addr, const int port);

    /**
     * @brief Runs the web server.
     *
     * @note This blocks the executing thread until there are no more handlers queued in the server event loop. The
     * internal Belle web server starts its own event loop and blocks the calling thread.
     *
     */
    void run();

    // The one and only token value the test server considers as valid.
    static std::string VALID_TOKEN;
    // The access level value returned as a result of the valid token validation.
    static int VALID_ACCESS_LEVEL;
    // The error message returned for any invalid token.
    static std::string INVALID_TOKEN_MSG;

   private:
    OB::Belle::Server m_srv;
    std::string m_addr;
    int m_port;
};

#endif