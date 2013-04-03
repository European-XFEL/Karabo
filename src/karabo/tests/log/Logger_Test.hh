/*
 * File:   Logger_Test.hh
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#ifndef LOGGER_TEST_HH
#define	LOGGER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>
#include <karabo/util/util.hh>
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
    
    LogSomething(const karabo::util::Hash& config) {
        using namespace karabo::log;
        Logger::Pointer p = Logger::createNode("logger", "Logger", config); 
        p->initialize();
    }
    
    void doSomeLogging() {
        KARABO_LOG_TRACE << "This is a trace message" << std::endl;
        KARABO_LOG_DEBUG << "This is a debug message";
        KARABO_LOG_INFO << "This is an info message";
        KARABO_LOG_WARN << "This is a warn message";
        KARABO_LOG_ERROR << "This is an error message";
    }
};


class Logger_Test : public CPPUNIT_NS::TestFixture {

    CPPUNIT_TEST_SUITE(Logger_Test);

    CPPUNIT_TEST(testLogging);
    //CPPUNIT_TEST(testInClassLogging);
    
    CPPUNIT_TEST_SUITE_END();

public:
    Logger_Test();
    virtual ~Logger_Test();
    void setUp();
    void tearDown();

private:
    void testLogging();
    //void testInClassLogging();
};

#endif	/* LOGGER_TEST_HH */

