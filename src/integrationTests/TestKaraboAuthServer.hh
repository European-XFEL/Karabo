/*
 * TestKaraboAuthServer.hh
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

#ifndef TESTKARABOAUTHSERVER_HH
#define TESTKARABOAUTHSERVER_HH

#include <memory>
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

    // The destructor has to be declared even if it is the default
    // to guarantee that the implementation class is fully defined
    // by the time the destructor is defined.
    // The destructor is defined in "TestKaraboAuthServer.cc" after the definition of
    // the "Impl" implementation class. More details about this at:
    // https://stackoverflow.com/questions/34072862/why-is-error-invalid-application-of-sizeof-to-an-incomplete-type-using-uniqu
    ~TestKaraboAuthServer();

    /**
     * @brief Runs the web server.
     *
     * @note The current implementation blocks the calling thread. For further details, please
     * look at the method TestKaraboAuthServer::Impl::run in "TestKaraboAuthServer.cc".
     */
    void run();

    // The one and only token value the test server considers as valid.
    static std::string VALID_TOKEN;
    // The access level value returned as a result of the valid token validation.
    static int VALID_ACCESS_LEVEL;
    // The error message returned for any invalid token.
    static std::string INVALID_TOKEN_MSG;
    // The user Id associated to the valid token
    static std::string VALID_USER_ID;

   private:
    class Impl; // Forward declaration of the implementation class
    std::unique_ptr<Impl> m_impl;
};

#endif
