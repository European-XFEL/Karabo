/*
 * File:   Authenticate_Test.cc
 * Author: heisenb
 *
 * Created on Apr 12, 2013, 4:24:42 PM
 */

#include <karabo/webAuth/Authenticator.hh>

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
    string time = "20130410145159257";

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Validate the following parameters were empty before login
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == true);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == true);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == true);
    
    success = a.login();
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
    string time = "20130410145159257";

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    success = a.login();
    CPPUNIT_ASSERT(success == false);

    // Test wrong username
    username = "guest2";
    password = "guest";
    a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    success = a.login();
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
    string time = "20130410145159257";

    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    success = a.login();
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
    string time = "20130410145159257";

    bool success;
    std::string sessionToken;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    success = a.login();
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