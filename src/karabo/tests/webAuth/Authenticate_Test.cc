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


void Authenticate_Test::testLogin() {

    string username = "maial";
    string password = "pass";
    string provider = "LOCAL";
    string ipAddress = "127.0.0.1";
    string hostname = "127.0.0.1";
    string portNumber = "4444";
    string software = "Karabo";
    string time = "20130410145159257";
    
    bool success;

    Authenticator a = Authenticator(username, password, provider, ipAddress, hostname, portNumber, software);
    
    success = a.login();
    CPPUNIT_ASSERT(success == true);
    
    success = a.logout();
    CPPUNIT_ASSERT(success == true);
    
}


