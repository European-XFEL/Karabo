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


void Logger_Test::test1() {

    // We are chatty in this test
    // But the idea is to only see OKs and never ERROR
    // There is no ASSERT unfortunately, so this test needs visual inspection
    Hash config("priority", "DEBUG");
    Logger::configure(config);
    Logger::logDebug() << "ERROR";

    Logger::useOstream();
    Logger::logDebug() << "OK";
    Logger::logDebug("a1") << "OK";
    Logger::logDebug("a1.a2") << "OK";
    Logger::logInfo() << "OK";
    Logger::logInfo("a1") << "OK";
    Logger::logInfo("a1.a2") << "OK";

    Logger::reset();
    Logger::logDebug() << "ERROR";
    Logger::logDebug("a1") << "ERROR";
    Logger::logDebug("a1.a2") << "ERROR";
    Logger::logInfo() << "ERROR";
    Logger::logInfo("a1") << "ERROR";
    Logger::logInfo("a1.a2") << "ERROR";

    Logger::useOstream("a1");
    Logger::logDebug() << "ERROR";
    Logger::logDebug("a1") << "OK";
    Logger::logDebug("a1.a2") << "OK";
    Logger::logInfo() << "ERROR";
    Logger::logInfo("a1") << "OK";
    Logger::logInfo("a1.a2") << "OK";

    Logger::setPriority("INFO");
    Logger::logDebug() << "ERROR";
    Logger::logDebug("a1") << "ERROR";
    Logger::logDebug("a1.a2") << "ERROR";
    Logger::logInfo() << "ERROR";
    Logger::logInfo("a1") << "OK";
    Logger::logInfo("a1.a2") << "OK";

    Logger::setPriority("WARN");
    Logger::logDebug() << "ERROR";
    Logger::logDebug("a1") << "ERROR";
    Logger::logDebug("a1.a2") << "ERROR";
    Logger::logInfo() << "ERROR";
    Logger::logInfo("a1") << "ERROR";
    Logger::logInfo("a1.a2") << "ERROR";
}


void Logger_Test::test2() {
    Logger::reset();
    Hash config("priority", "INFO");
    Logger::configure(config);
    Logger::useOstream();
    Logger::useFile("a1", false); // do not inherit appenders from parents
    Logger::logDebug() << "ERROR";
    Logger::logDebug("a1") << "ERROR";
    Logger::logDebug("a1.a2") << "ERROR";
    Logger::logInfo() << "CONSOLE-OK";
    Logger::logInfo("a1") << "FILE-OK";
    Logger::logInfo("a1.a2") << "FILE-OK";
}


void Logger_Test::testInClassLogging() {
    Logger::reset();
    Hash config("Logger.priority", "WARN");
    auto p = Configurator<LogSomething>::create("LogSomething", config);
    p->doSomeLogging();
}



