/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   PropertyTest_Test.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 6, 2016, 3:05 PM
 */

#ifndef PROPERTYTEST_TEST_HH
#define PROPERTYTEST_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include "karabo/karabo.hpp"

class PropertyTest_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(PropertyTest_Test);
    CPPUNIT_TEST(allTestRunner);
    CPPUNIT_TEST_SUITE_END();

   public:
    PropertyTest_Test();
    virtual ~PropertyTest_Test();
    void setUp();
    void tearDown();

   private:
    void allTestRunner();

    void testPropertyTest();
    void testSimpleProperties();

    // Asserts that read-only properties cannot be
    // written.
    void testReadOnlyProperties();

    void testVectorProperties();
    void testTableProperties();

    /**
     * Verifies that read-only table properties can
     * only be read.
     */
    void testReadOnlyTableProperties();

    void testAttributeEditing();
    void testNodedSlots();

   private:
    karabo::core::DeviceServer::Pointer m_deviceServer;

    boost::thread m_eventLoopThread;

    karabo::core::DeviceClient::Pointer m_deviceClient;
};

#endif /* PROPERTYTEST_TEST_HH */
