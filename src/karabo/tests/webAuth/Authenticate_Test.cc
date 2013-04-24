/*
 *
 * File:   Authenticate_Test.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:24:42 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/webAuth/Authenticator.hh>
#include <karabo/util/Timestamp.hh>
#include "Authenticate_Test.hh"

using namespace std;
using namespace karabo::webAuth;

CPPUNIT_TEST_SUITE_REGISTRATION(Authenticate_Test);


Authenticate_Test::Authenticate_Test() {
}


Authenticate_Test::~Authenticate_Test() {
}


void Authenticate_Test::setUp() {
}


void Authenticate_Test::tearDown() {
}


void Authenticate_Test::testCorrectLogin() {
    string username = "guest";
    string password = "guest";
    string provider = "LOCAL";
    string ipAddress = "c++UnitTestsIpAddress";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    //
    string timeStr = "20130410T145159.789333";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Validate the following parameters were empty before login
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == true);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == true);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == true);

    success = a.login(time);
    CPPUNIT_ASSERT(success == true);

    // Validate the following parameters were populated after login
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == false);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == false);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == false);

    success = a.logout();
    CPPUNIT_ASSERT(success == true);

    // Validate the following parameters were cleaned after logout
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == true);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == true);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == true);
}


void Authenticate_Test::testIncorrectLogin() {
    string username = "guest";
    string password = "guest2";
    string provider = "LOCAL";
    string ipAddress = "c++UnitTestsIpAddress";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    //
    string timeStr = "20130410T145159.789333";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    success = a.login(time);
    CPPUNIT_ASSERT(success == false);

    // Test wrong username
    username = "guest2";
    password = "guest";
    a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    success = a.login(time);
    CPPUNIT_ASSERT(success == false);
}


void Authenticate_Test::testIncorrectUsername() {
    string username = "guest2";
    string password = "guest";
    string provider = "LOCAL";
    string ipAddress = "c++UnitTestsIpAddress";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    //
    string timeStr = "20130410T145159.789333";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    success = a.login(time);
    CPPUNIT_ASSERT(success == false);
}


void Authenticate_Test::testSingleSignOn() {
    string username = "guest";
    string password = "guest";
    string provider = "LOCAL";
    string ipAddress = "c++UnitTestsIpAddress";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    //
    string timeStr = "20130410T145159.789333";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    bool success;
    std::string sessionToken;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    success = a.login(time);
    CPPUNIT_ASSERT(success == true);

    // Validate session with current machine name => Should be OK
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddress");
    if (sessionToken.empty()) {
        success = false;
    } else {
        success = true;
    }
    CPPUNIT_ASSERT(success == true);

    // Validate session with different machine name => Should be NOT OK
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddressXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    if (sessionToken.empty()) {
        success = false;
    } else {
        success = true;
    }
    CPPUNIT_ASSERT(success == false);

    success = a.logout();
    CPPUNIT_ASSERT(success == true);

    // Validate session with current machine name => Should be NOT OK because user made Logout
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddress");
    if (sessionToken.empty()) {
        success = false;
    } else {
        success = true;
    }
    CPPUNIT_ASSERT(success == false);
}