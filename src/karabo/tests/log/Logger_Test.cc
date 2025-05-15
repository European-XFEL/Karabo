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
 * File:   Logger_Test.cc
 * Author: bheisen
 *
 * Created on Mar 14, 2013, 12:24:04 PM
 */

#include "Logger_Test.hh"

#include <karabo/log/Logger.hh>

#include "karabo/data/io/Input.hh"

using namespace std;
using namespace karabo::log;
using namespace karabo::data;

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
    Hash config("level", "DEBUG");
    Logger::configure(config);
    LoggerStream("", spdlog::level::debug) << "ERROR";

    Logger::useConsole();
    Logger::debug("", "{}", "OK");
    Logger::debug("a1", "{}", "OK");
    Logger::debug("a1.a2", "{}", "OK");
    Logger::info("", "{}", "OK");
    Logger::info("a1", "{}", "OK");
    Logger::info("a1.a2", "{}", "OK");
    LoggerStream("", spdlog::level::debug) << "OK";
    LoggerStream("a1", spdlog::level::debug) << "OK";
    LoggerStream("a1.a2", spdlog::level::debug) << "OK";
    LoggerStream("", spdlog::level::info) << "OK";
    LoggerStream("a1", spdlog::level::info) << "OK";
    LoggerStream("a1.a2", spdlog::level::info) << "OK";

    Logger::reset();
    Logger::debug("", "{}", "ERROR");
    Logger::debug("a1", "{}", "ERROR");
    Logger::debug("a1.a2", "{}", "ERROR");
    Logger::info("", "{}", "ERROR");
    Logger::info("a1", "{}", "ERROR");
    Logger::info("a1.a2", "{}", "ERROR");
    LoggerStream("", spdlog::level::debug) << "ERROR";
    LoggerStream("a1", spdlog::level::debug) << "ERROR";
    LoggerStream("a1.a2", spdlog::level::debug) << "ERROR";
    LoggerStream("", spdlog::level::info) << "ERROR";
    LoggerStream("a1", spdlog::level::info) << "ERROR";
    LoggerStream("a1.a2", spdlog::level::info) << "ERROR";

    Logger::useConsole("a1");
    Logger::debug("", "{}", "ERROR");
    Logger::debug("a1", "{}", "OK");
    Logger::debug("a1.a2", "{}", "OK");
    Logger::info("", "{}", "ERROR");
    Logger::info("a1", "{}", "OK");
    Logger::info("a1.a2", "{}", "OK");
    LoggerStream("", spdlog::level::debug) << "ERROR";
    LoggerStream("a1", spdlog::level::debug) << "OK";
    LoggerStream("a1.a2", spdlog::level::debug) << "OK";
    LoggerStream("", spdlog::level::info) << "ERROR";
    LoggerStream("a1", spdlog::level::info) << "OK";
    LoggerStream("a1.a2", spdlog::level::info) << "OK";

    Logger::setLevel("INFO");
    Logger::debug("", "{}", "ERROR");
    Logger::debug("a1", "{}", "ERROR");
    Logger::debug("a1.a2", "{}", "ERROR");
    Logger::info("", "{}", "ERROR");
    Logger::info("a1", "{}", "OK");
    Logger::info("a1.a2", "{}", "OK");

    Logger::setLevel("WARN");
    Logger::debug("", "{}", "ERROR");
    Logger::debug("a1", "{}", "ERROR");
    Logger::debug("a1.a2", "{}", "ERROR");
    Logger::info("", "{}", "ERROR");
    Logger::info("a1", "{}", "ERROR");
    Logger::info("a1.a2", "{}", "ERROR");
}


void Logger_Test::test2() {
    Logger::reset();
    Hash config("level", "INFO");
    Logger::configure(config);
    Logger::useConsole();
    Logger::useFile("a1");
    Logger::debug("", "{}", "ERROR");
    Logger::debug("a1", "{}", "ERROR");
    Logger::debug("a1.a2", "{}", "ERROR");
    Logger::info("", "{}", "CONSOLE-OK");
    Logger::info("a1", "{}", "FILE-OK");
    Logger::info("a1.a2", "{}", "FILE-OK");
    LoggerStream("", spdlog::level::debug) << "ERROR";
    LoggerStream("a1", spdlog::level::debug) << "ERROR";
    LoggerStream("a1.a2", spdlog::level::debug) << "ERROR";
    LoggerStream("", spdlog::level::info) << "CONSOLE-OK";
    LoggerStream("a1", spdlog::level::info) << "FILE-OK";
    LoggerStream("a1.a2", spdlog::level::info) << "FILE-OK";
}


void Logger_Test::testInClassLogging() {
    Logger::reset();
    Hash config("log.level", "WARN");
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
    Hash config("level", "INFO", "cache.maxNumMessages", maxMsgs);
    Logger::configure(config);
    Logger::useCache();

    // calling Logger::getCachedContent before logging will get an empty vector
    content = Logger::getCachedContent(10);
    CPPUNIT_ASSERT_EQUAL(0ul, content.size());

    // log something
    for (int i = 0; i < 100; i++) {
        Logger::debug("VERBOSE_STUFF", "This should not be logged - {}", i);
        Logger::info("INFORMATIVE_STUFF", "line - {}", i);
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
