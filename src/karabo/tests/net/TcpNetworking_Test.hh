/*
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
/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Created on Oct 30, 2012, 1:33:46 PM
 */

#ifndef TCPNETWORKING_TEST_HH
#define TCPNETWORKING_TEST_HH
#include <cppunit/extensions/HelperMacros.h>

#include <karabo/net/Channel.hh>

class TcpNetworking_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TcpNetworking_Test);
    CPPUNIT_TEST(testConfig);
    CPPUNIT_TEST(testClientServer);
    CPPUNIT_TEST(testWriteAsync);
    CPPUNIT_TEST(testBufferSet);
    CPPUNIT_TEST(testConsumeBytesAfterReadUntil);
    CPPUNIT_TEST(testAsyncWriteCompleteHandler);
    CPPUNIT_TEST(testConnCloseChannelStop);
    CPPUNIT_TEST_SUITE_END();

   public:
    KARABO_CLASSINFO(TcpNetworking_Test, "TcpNetworking_Test", "1.0");

    TcpNetworking_Test();
    virtual ~TcpNetworking_Test();
    void setUp();
    void tearDown();

   private:
    void testConfig();
    void testClientServer();

    /**
     * Test the set of writeAsync methods of type write and "forget" in the
     * karabo::net::TcpChannel class (acessed through its abstract base
     * class, karabo::net::Channel).
     *
     * The write and "forget" do not call write handlers specified by their
     * callers once they have done their job (or failed at trying to do it).
     */
    void testWriteAsync();

    void testBufferSet();

    void testConsumeBytesAfterReadUntil();

    void testAsyncWriteCompleteHandler();

    void testConnCloseChannelStop();
    void testConnCloseChannelStop(karabo::net::Channel::Pointer& alice, karabo::net::Channel::Pointer& bob,
                                  karabo::net::Connection::Pointer& bobConn);
};

#endif /* TCPNETWORKING_TEST_HH */
