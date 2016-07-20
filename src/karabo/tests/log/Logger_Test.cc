/*
 * File:   Logger_Test.cc
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#include <karabo/log/Logger.hh>
#include <karabo/io/Input.hh>
#include "Logger_Test.hh"

using namespace std;
using namespace karabo::log;
using namespace karabo::util;
using namespace karabo::io;


CPPUNIT_TEST_SUITE_REGISTRATION(Logger_Test);


KARABO_REGISTER_FOR_CONFIGURATION(LogSomething)

Logger_Test::Logger_Test() {
}


Logger_Test::~Logger_Test() {
}


void Logger_Test::setUp() {
}


void Logger_Test::tearDown() {
}


void Logger_Test::testLogging() {
    Hash s1("Category.name", "s1", "Category.priority", "DEBUG");
    Hash conf("categories[0]", s1, "appenders[0].Ostream.layout", "Pattern");
    Logger::configure(conf);
    //p->initialize();
    KARABO_LOG_FRAMEWORK_DEBUG_C("s1") << "Some test message";

}


void Logger_Test::testInClassLogging() {
    Logger::reset();
    Hash config("logger.priority", "INFO", "logger.appenders[0].File.layout.Pattern.format", "%c %m %n");
    LogSomething::Pointer p = LogSomething::create("LogSomething", config);
    p->doSomeLogging();
}

