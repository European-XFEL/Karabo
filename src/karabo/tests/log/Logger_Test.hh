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
 * File:   Logger_Test.hh
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#ifndef LOGGER_TEST_HH
#define LOGGER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/log/Logger.hh>

#include "karabo/data/schema/Configurator.hh"
#include "karabo/data/schema/NodeElement.hh"

class LogSomething {
   public:
    KARABO_CLASSINFO(LogSomething, "LogSomething", "")

    static void expectedParameters(karabo::data::Schema& expected) {
        using namespace karabo::data;

        NODE_ELEMENT(expected)
              .key("log")
              .displayedName("Logger")
              .description("Logger configuration")
              .appendParametersOf<karabo::log::Logger>()
              .commit();
    }

    LogSomething(const karabo::data::Hash& input) {
        using namespace karabo::log;
        Logger::configure(input.get<karabo::data::Hash>("log"));
        Logger::useConsole();
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
