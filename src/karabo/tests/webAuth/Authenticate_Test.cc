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
#include "karabo/tests/util/Epochstamp_Test.hh"

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


void Authenticate_Test::testNotLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password,
                                             const std::string& provider, const std::string& ipAddress, const std::string& brokerHostname,
                                             const int brokerPortNumber, const std::string& brokerTopic) {

    // Variables that should be correctly assigned
    CPPUNIT_ASSERT(a.getUsername() == username);
    CPPUNIT_ASSERT(a.getPassword() == password);
    CPPUNIT_ASSERT(a.getProvider() == provider);
    CPPUNIT_ASSERT(a.getIpAddress() == ipAddress);
    CPPUNIT_ASSERT(a.getBrokerHostname() == brokerHostname);
    CPPUNIT_ASSERT(a.getBrokerPortNumber() == brokerPortNumber);
    CPPUNIT_ASSERT(a.getBrokerTopic() == brokerTopic);
    CPPUNIT_ASSERT(a.getSoftware() == KARABO_SOFTWARE_DESC);

    // Validate the following parameters are empty before login
    CPPUNIT_ASSERT(a.getSoftwareDesc().empty() == true);
    CPPUNIT_ASSERT(a.getDefaultAccessLevelDesc().empty() == true);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == true);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == true);
    //
    CPPUNIT_ASSERT(a.getSoftwareId() == KARABO_INVALID_ID);
    CPPUNIT_ASSERT(a.getUserId() == KARABO_INVALID_ID);
    CPPUNIT_ASSERT(a.getDefaultAccessLevelId() == KARABO_INVALID_ID);
}


void Authenticate_Test::testSuccessfulLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password,
                                                    const std::string& provider, const std::string& ipAddress, const std::string& brokerHostname,
                                                    const int brokerPortNumber, const std::string& brokerTopic, const long long expectedSoftwareId,
                                                    const long long expectedUserId, const int expectedDefaultAccessLevelId) {

    // Variables that should be correctly assigned
    CPPUNIT_ASSERT(a.getUsername() == username);
    CPPUNIT_ASSERT(a.getPassword() == password);
    CPPUNIT_ASSERT(a.getProvider() == provider);
    CPPUNIT_ASSERT(a.getIpAddress() == ipAddress);
    CPPUNIT_ASSERT(a.getBrokerHostname() == brokerHostname);
    CPPUNIT_ASSERT(a.getBrokerPortNumber() == brokerPortNumber);
    CPPUNIT_ASSERT(a.getBrokerTopic() == brokerTopic);
    CPPUNIT_ASSERT(a.getSoftware() == KARABO_SOFTWARE_DESC);

    // Validate the following parameters were populated after login
    CPPUNIT_ASSERT(a.getSoftwareDesc().empty() == false);
    CPPUNIT_ASSERT(a.getDefaultAccessLevelDesc().empty() == false);
    CPPUNIT_ASSERT(a.getWelcomeMessage().empty() == false);
    CPPUNIT_ASSERT(a.getSessionToken().empty() == false);
    //
    CPPUNIT_ASSERT(a.getSoftwareId() == expectedSoftwareId);
    CPPUNIT_ASSERT(a.getUserId() == expectedUserId);
    CPPUNIT_ASSERT(a.getDefaultAccessLevelId() == expectedDefaultAccessLevelId);
}


void Authenticate_Test::testCorrectLogin() {
    string username = "unitaryTests";
    string password = "karaboUnitaryTestsPass";
    string provider = "LOCAL";
    string brokerHostname = "127.0.0.1";
    int brokerPortNumber = 4444;
    string brokerTopic = "topic";

    karabo::util::Epochstamp current_epochstamp = karabo::util::Epochstamp();
    string ipAddress = "c++UnitTestsIpAddress" + current_epochstamp.toIso8601Ext(karabo::util::ATTOSEC);

    // Expected result values
    const long long expectedSoftwareId = 1;
    const long long expectedUserId = -99;
    const long long expectedDefaultAccessLevelId = 1;


    Authenticator a(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    /************************
     * LOGIN
     ************************/
    // Successful login
    CPPUNIT_ASSERT(a.login() == true);

    // Class instance should be with in new information
    testSuccessfulLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic, expectedSoftwareId, expectedUserId, expectedDefaultAccessLevelId);

    /************************
     * LOGOUT
     ************************/
    // Successful logout
    CPPUNIT_ASSERT(a.logout() == true);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);
}


void Authenticate_Test::testCorrectLoginAccessLevelZero() {
    string username = "observer";
    string password = "karabo";
    string provider = "LOCAL";
    string brokerHostname = "127.0.0.1";
    int brokerPortNumber = 4444;
    string brokerTopic = "topic";

    karabo::util::Epochstamp current_epochstamp = karabo::util::Epochstamp();
    string ipAddress = "c++UnitTestsIpAddress" + current_epochstamp.toIso8601(karabo::util::ATTOSEC);

    // Expected result values
    const long long expectedSoftwareId = 1;
    const long long expectedUserId = 0;
    const long long expectedDefaultAccessLevelId = 0;


    Authenticator a(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    /************************
     * LOGIN
     ************************/
    // Successful login
    CPPUNIT_ASSERT(a.login() == true);

    // Class instance should be with in new information
    testSuccessfulLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic, expectedSoftwareId, expectedUserId, expectedDefaultAccessLevelId);

    /************************
     * LOGOUT
     ************************/
    // Successful logout
    CPPUNIT_ASSERT(a.logout() == true);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);
}


void Authenticate_Test::testIncorrectLogin() {
    string username = "unitaryTests";
    string password = "karaboUnitaryTestsPass222";
    string provider = "LOCAL";
    string brokerHostname = "127.0.0.1";
    int brokerPortNumber = 4444;
    string brokerTopic = "topic";

    karabo::util::Epochstamp current_epochstamp = karabo::util::Epochstamp();
    string ipAddress = "c++UnitTestsIpAddress" + current_epochstamp.toIso8601(karabo::util::ATTOSEC);

    Authenticator a = Authenticator(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Test wrong password
    try {
        CPPUNIT_ASSERT(a.login() == false);
    } catch(karabo::util::NetworkException& e) {
        CPPUNIT_ASSERT(true);
    }

    // Test wrong password (case unsuccess in the getUserNonce function)
    username = "heisenb";
    password = "karaboUnitaryTestsPass";
    a = Authenticator(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Test wrong password
    try {
        CPPUNIT_ASSERT(a.login() == false);
    } catch(karabo::util::NetworkException& e) {
        CPPUNIT_ASSERT(true);
    }

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Test wrong username
    username = "unitaryTests2";
    password = "karaboUnitaryTestsPass";
    a = Authenticator(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    try {
        CPPUNIT_ASSERT(a.login() == false);
    } catch(karabo::util::NetworkException& e) {
        CPPUNIT_ASSERT(true);
    }
        

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Unsuccessful logout
    try {
        CPPUNIT_ASSERT(a.logout() == false);
    } catch(karabo::util::NetworkException& e) {
        CPPUNIT_ASSERT(true);
    }
}


void Authenticate_Test::testIncorrectUsername() {
    string username = "unitaryTests2";
    string password = "karaboUnitaryTestsPass";
    string provider = "LOCAL";
    string brokerHostname = "127.0.0.1";
    int brokerPortNumber = 4444;
    string brokerTopic = "topic";

    karabo::util::Epochstamp current_epochstamp = karabo::util::Epochstamp();
    string ipAddress = "c++UnitTestsIpAddress" + current_epochstamp.toIso8601(karabo::util::ATTOSEC);

    Authenticator a = Authenticator(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Test wrong password
    CPPUNIT_ASSERT(a.login() == false);

    // Class instance should be in the initial state
    testNotLoggedContext(a, username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Unsuccessful logout
    CPPUNIT_ASSERT(a.logout() == false);
}


void Authenticate_Test::testSingleSignOn() {
    string username = "unitaryTests";
    string password = "karaboUnitaryTestsPass";
    string provider = "LOCAL";
    string brokerHostname = "127.0.0.1";
    int brokerPortNumber = 4444;
    string brokerTopic = "topic";
    std::string sessionToken, sessionTokenOrig;

    karabo::util::Epochstamp current_epochstamp = karabo::util::Epochstamp();
    string ipAddress = "c++UnitTestsIpAddress" + current_epochstamp.toIso8601(karabo::util::ATTOSEC);
    string equalIpAddress = ipAddress;
    string differentIpAddress = "c++UnitTestsIpAddressXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

    Authenticator a = Authenticator(username, password, provider, ipAddress, brokerHostname, brokerPortNumber, brokerTopic);

    // Successful login
    CPPUNIT_ASSERT(a.login() == true);

    // Validate session with current machine name => Should be OK
    sessionTokenOrig = a.getSingleSignOn(equalIpAddress);
    CPPUNIT_ASSERT(sessionTokenOrig.empty() == false);

    // Validate session with different machine name => Should be NOT OK
    sessionToken = a.getSingleSignOn(differentIpAddress);
    CPPUNIT_ASSERT(sessionToken.empty() == true);

    // Successful logout
    CPPUNIT_ASSERT(a.logout() == true);

    // TODO: Understand if it makes sense to make this test!!!
    // Validate session with current machine name is NULL or different from session "destroyed" => Because user made Logout
    //sessionToken = a.getSingleSignOn("c++UnitTestsIpAddress");
    //CPPUNIT_ASSERT( (sessionToken.empty() || sessionToken != sessionTokenOrig) == true);
}