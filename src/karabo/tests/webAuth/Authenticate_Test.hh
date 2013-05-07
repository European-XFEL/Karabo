/*
 *
 * File:   Authenticate_Test.hh
 * Author: <luis.maia@xfel.eu>
 *
 * Created on April 12, 2013, 4:24:42 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef AUTHENTICATE_TEST_HH
#define	AUTHENTICATE_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/webAuth/Authenticator.hh>
#include <karabo/util/Timestamp.hh>

class Authenticate_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Authenticate_Test);
    CPPUNIT_TEST(testCorrectLogin);
    CPPUNIT_TEST(testIncorrectLogin);
    CPPUNIT_TEST(testIncorrectUsername);
    CPPUNIT_TEST(testSingleSignOn);
    CPPUNIT_TEST_SUITE_END();

public:
    Authenticate_Test();
    virtual ~Authenticate_Test();
    void setUp();
    void tearDown();

private:
    void testNotLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password, const std::string& provider,
                              const std::string& ipAddress, const std::string& hostname, const std::string& portNumber,
                              const std::string& software);
    void testSuccessfulLoggedContext(karabo::webAuth::Authenticator a, const std::string& username, const std::string& password, const std::string& provider,
                                     const std::string& ipAddress, const std::string& hostname, const std::string& portNumber, const std::string& software,
                                     const long long int expectedSoftwareId, const long long int expectedUserId, const long long int expectedRoleId);
    //
    void testCorrectLogin();
    void testIncorrectLogin();
    void testIncorrectUsername();
    void testSingleSignOn();
};

#endif	/* AUTHENTICATE_TEST_HH */

