/*
 *
 * File:   Authenticate_Test.cc
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:24:42 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


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


void Authenticate_Test::testNotLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password, const std::string& provider,
                                             const std::string& ipAddress, const std::string& hostname, const std::string& portNumber,
                                             const std::string& software) {

    // Variables that should be correctly assigned
    CPPUNIT_ASSERT(a.getUsername() == username);
    CPPUNIT_ASSERT(a.getPassword() == password);
    CPPUNIT_ASSERT(a.getProvider() == provider);
    CPPUNIT_ASSERT(a.getIpAddress() == ipAddress);
    CPPUNIT_ASSERT(a.getHostname() == hostname);
    CPPUNIT_ASSERT(a.getPortNumber() == portNumber);
    CPPUNIT_ASSERT(a.getSoftware() == software);

    // Validate the following parameters are empty before login
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == true);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == true);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == true);
    //
    CPPUNIT_ASSERT(a.getSoftwareId() == -100);
    CPPUNIT_ASSERT(a.getUserId() == -100);
    CPPUNIT_ASSERT(a.getRoleId() == -100);
}


void Authenticate_Test::testSuccessfulLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password, const std::string& provider,
                                                    const std::string& ipAddress, const std::string& hostname, const std::string& portNumber, const std::string& software,
                                                    const long long int expectedSoftwareId, const long long int expectedUserId, const long long int expectedRoleId) {

    // Variables that should be correctly assigned
    CPPUNIT_ASSERT(a.getUsername() == username);
    CPPUNIT_ASSERT(a.getPassword() == password);
    CPPUNIT_ASSERT(a.getProvider() == provider);
    CPPUNIT_ASSERT(a.getIpAddress() == ipAddress);
    CPPUNIT_ASSERT(a.getHostname() == hostname);
    CPPUNIT_ASSERT(a.getPortNumber() == portNumber);
    CPPUNIT_ASSERT(a.getSoftware() == software);

    // Validate the following parameters were populated after login
    CPPUNIT_ASSERT(a.getRoleDesc().empty() == false);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == false);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == false);
    //
    CPPUNIT_ASSERT(a.getSoftwareId() == expectedSoftwareId);
    CPPUNIT_ASSERT(a.getUserId() == expectedUserId);
    CPPUNIT_ASSERT(a.getRoleId() == expectedRoleId);
}


void Authenticate_Test::testCorrectLogin() {
    string username = "guest";
    string password = "guest";
    string provider = "LOCAL";
    string ipAddress = "c++UnitTestsIpAddress";
    string hostname = "127.0.0.1";
    string portNumber = "44444";
    string software = "Karabo";
    //
    string timeStr = "20130120T122059.259188123";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    // Expected result values
    const long long int expectedSoftwareId = 1;
    const long long int expectedUserId = -1;
    const long long int expectedRoleId = 3;


    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);

    /************************
     * LOGIN
     ************************/
    // Successful login
    CPPUNIT_ASSERT(a.login(time) == true);

    // Class instance should be with in new information
    testSuccessfulLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software, expectedSoftwareId, expectedUserId, expectedRoleId);

    /************************
     * LOGOUT
     ************************/
    // Successful logout
    CPPUNIT_ASSERT(a.logout() == true);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);
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
    string timeStr = "20130120T122059.259188";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    CPPUNIT_ASSERT(a.login(time) == false);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong username
    username = "guest2";
    password = "guest";
    a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);

    CPPUNIT_ASSERT(a.login(time) == false);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);
    
    // Unsuccessful logout
    CPPUNIT_ASSERT(a.logout() == false);
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
    string timeStr = "20130120T122059.259188";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);

    // Test wrong password
    CPPUNIT_ASSERT(a.login(time) == false);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, hostname, portNumber, software);
    
    // Unsuccessful logout
    CPPUNIT_ASSERT(a.logout() == false);
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
    string timeStr = "20130120T122059.259188";
    karabo::util::Timestamp time = karabo::util::Timestamp(timeStr);

    std::string sessionToken;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);

    // Successful login
    CPPUNIT_ASSERT(a.login(time) == true);

    // Validate session with current machine name => Should be OK
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddress");
    CPPUNIT_ASSERT(sessionToken.empty() == false);

    // Validate session with different machine name => Should be NOT OK
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddressXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    CPPUNIT_ASSERT(sessionToken.empty() == true);

    // Successful logout
    CPPUNIT_ASSERT(a.logout() == true);

    // Validate session with current machine name => Should be NOT OK because user made Logout
    sessionToken = a.getSingleSignOn("c++UnitTestsIpAddress");
    CPPUNIT_ASSERT(sessionToken.empty() == true);
}