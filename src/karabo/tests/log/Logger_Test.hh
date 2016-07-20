/*
 * File:   Logger_Test.hh
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#ifndef LOGGER_TEST_HH
#define	LOGGER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/Configurator.hh>
#include <karabo/util/NodeElement.hh>
#include <karabo/log/Logger.hh>

class LogSomething {


public:

    KARABO_CLASSINFO(LogSomething, "LogSomething", "1.0")
    KARABO_CONFIGURATION_BASE_CLASS

    static void expectedParameters(karabo::util::Schema& expected) {
        using namespace karabo::util;

        NODE_ELEMENT(expected).key("logger")
                .displayedName("Logger")
                .description("Logger configuration")
                .appendParametersOfConfigurableClass<karabo::log::Logger>("Logger")
                .commit();
    }

    LogSomething(const karabo::util::Hash& input) {
        using namespace karabo::log;
        Logger::configure(input.get<karabo::util::Hash>("logger"));
    }

    virtual ~LogSomething() {
    }

    void doSomeLogging() {
        KARABO_LOG_FRAMEWORK_TRACE << "This is a trace message" << std::endl;
        KARABO_LOG_FRAMEWORK_DEBUG << "This is a debug message";
        KARABO_LOG_FRAMEWORK_INFO << "This is an info message";
        KARABO_LOG_FRAMEWORK_WARN << "This is a warn message";
        KARABO_LOG_FRAMEWORK_ERROR << "This is an error message";
    }
};

class Logger_Test : public CPPUNIT_NS::TestFixture {


    CPPUNIT_TEST_SUITE(Logger_Test);

    CPPUNIT_TEST(testLogging);
    CPPUNIT_TEST(testInClassLogging);

    CPPUNIT_TEST_SUITE_END();

public:
    Logger_Test();
    virtual ~Logger_Test();
    void setUp();
    void tearDown();

private:
    void testLogging();
    void testInClassLogging();
};

#endif	/* LOGGER_TEST_HH */

