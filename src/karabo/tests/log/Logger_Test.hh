/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Logger_Test.hh
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#ifndef LOGGER_TEST_HH
#define LOGGER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/NodeElement.hh>

class LogSomething {
   public:
    KARABO_CLASSINFO(LogSomething, "LogSomething", "")

    static void expectedParameters(karabo::util::Schema& expected) {
        using namespace karabo::util;

        NODE_ELEMENT(expected)
              .key("Logger")
              .displayedName("Logger")
              .description("Logger configuration")
              .appendParametersOf<karabo::log::Logger>()
              .commit();
    }

    LogSomething(const karabo::util::Hash& input) {
        using namespace karabo::log;
        Logger::configure(input.get<karabo::util::Hash>("Logger"));
        Logger::useOstream();
    }

    virtual ~LogSomething() {}

    void doSomeLogging() {
        KARABO_LOG_FRAMEWORK_DEBUG << "ERROR";
        KARABO_LOG_FRAMEWORK_INFO << "ERROR";
        KARABO_LOG_FRAMEWORK_WARN << "OK";
        KARABO_LOG_FRAMEWORK_ERROR << "OK";
    }
};

class Logger_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Logger_Test);

    CPPUNIT_TEST(test1);
    CPPUNIT_TEST(test2);
    CPPUNIT_TEST(testInClassLogging);
    CPPUNIT_TEST(testLastMessages);

    CPPUNIT_TEST_SUITE_END();

   public:
    Logger_Test();
    virtual ~Logger_Test();
    void setUp();
    void tearDown();

   private:
    void test1();
    void test2();
    void testInClassLogging();
    void testLastMessages();
};

#endif /* LOGGER_TEST_HH */
