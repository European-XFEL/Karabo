/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Logger_Test.cc
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#include "Logger_Test.hh"

#include <karabo/io/Input.hh>
#include <karabo/log/Logger.hh>

using namespace std;
using namespace karabo::log;
using namespace karabo::util;
using namespace karabo::io;

CPPUNIT_TEST_SUITE_REGISTRATION(Logger_Test);


KARABO_REGISTER_FOR_CONFIGURATION(LogSomething)


Logger_Test::Logger_Test() {}


Logger_Test::~Logger_Test() {}


void Logger_Test::setUp() {}


void Logger_Test::tearDown() {}


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


void Logger_Test::testLastMessages() {
    Logger::reset();

    // calling Logger::getCachedContent before calling Logger::useCache is legal
    // but an empty vector is returned.
    std::vector<Hash> content = Logger::getCachedContent(10);
    CPPUNIT_ASSERT_EQUAL(0ul, content.size());

    // set up the Logger
    const unsigned int maxMsgs = 20;
    Hash config("priority", "INFO", "cache.maxNumMessages", maxMsgs);
    Logger::configure(config);
    Logger::useCache();

    // calling Logger::getCachedContent before logging will get an empty vector
    content = Logger::getCachedContent(10);
    CPPUNIT_ASSERT_EQUAL(0ul, content.size());

    // log something
    for (int i = 0; i < 100; i++) {
        Logger::logDebug("VERBOSE_STUFF") << "This should not be logged - " << i;
        Logger::logInfo("INFORMATIVE_STUFF") << "line - " << i;
    }

    // get the last 10 entries
    content = Logger::getCachedContent(10u);

    CPPUNIT_ASSERT_EQUAL(10ul, content.size());
    int index = 90;
    for (const Hash& entry : content) {
        // check that the timestamp is in, but do not check
        CPPUNIT_ASSERT(entry.has("timestamp"));
        CPPUNIT_ASSERT_EQUAL(std::string("INFORMATIVE_STUFF"), entry.get<std::string>("category"));
        CPPUNIT_ASSERT_EQUAL(std::string("INFO"), entry.get<std::string>("type"));
        std::ostringstream expected;
        expected << "line - " << index++;
        CPPUNIT_ASSERT_EQUAL(expected.str(), entry.get<std::string>("message"));
    }

    // One can request more than the max config["cache.maxNumMessages"] but will not get more than that
    content = Logger::getCachedContent(200);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(maxMsgs), content.size());
    index = 100 - maxMsgs;
    for (const Hash& entry : content) {
        // check that the timestamp is in, but do not check
        CPPUNIT_ASSERT(entry.has("timestamp"));
        CPPUNIT_ASSERT_EQUAL(std::string("INFORMATIVE_STUFF"), entry.get<std::string>("category"));
        CPPUNIT_ASSERT_EQUAL(std::string("INFO"), entry.get<std::string>("type"));
        std::ostringstream expected;
        expected << "line - " << index++;
        CPPUNIT_ASSERT_EQUAL(expected.str(), entry.get<std::string>("message"));
    }
}
