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
 * File:   Runner_Test.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on February 21, 2017, 2:37 PM
 */

#include "Runner_Test.hh"

#include <karabo/karabo.hpp>

CPPUNIT_TEST_SUITE_REGISTRATION(Runner_Test);


Runner_Test::Runner_Test() {}


Runner_Test::~Runner_Test() {}


void Runner_Test::setUp() {}


void Runner_Test::tearDown() {}


void Runner_Test::testRunnerSuccess() {
    using namespace karabo::util;
    std::string initval =
          "{\"Karabo_DataLoggerManager_0\": {\"classId\": \"DataLoggerManager\", "
          "\"serverList\": [\"dls1\", \"dls2\", \"dls3\", \"dls4\"], \"param1\": 12, "
          "\"param2\": 99}}";

    std::string initStr = std::string("init=") + initval;
    char const* argv[] = {"SomeExecutable", "serverId=foo", initStr.c_str(), "visibility=5", "Logger.priority=DEBUG"};
    int argc = 5;
    Hash configuration;
    CPPUNIT_ASSERT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));
    CPPUNIT_ASSERT(configuration.get<std::string>("serverId") == "foo");
    CPPUNIT_ASSERT(configuration.get<std::string>("init") == initval);
    CPPUNIT_ASSERT(configuration.get<std::string>("visibility") == "5");
    CPPUNIT_ASSERT(configuration.get<std::string>("Logger.priority") == "DEBUG");
}


void Runner_Test::testRunnerSuccess2() {
    using namespace karabo::util;
    const char* argv[] = {"AnotherExecutable5", "serverId=bingo", "a[0].b=1", "a[0].c=2", "a[0].e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));

    CPPUNIT_ASSERT(configuration.get<std::string>("serverId") == "bingo");
    CPPUNIT_ASSERT(configuration.get<std::string>("a[0].b") == "1");
    CPPUNIT_ASSERT(configuration.get<std::string>("a[0].c") == "2");
    CPPUNIT_ASSERT(configuration.get<std::string>("a[0].e.x") == "15");
    CPPUNIT_ASSERT(configuration.get<std::string>("a[0].e.y") == "88");
}


void Runner_Test::testRunnerFailure1() {
    using namespace karabo::util;
    const char* argv[] = {"AnotherExecutable", "serverId=bar", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure2() {
    using namespace karabo::util;
    const char* argv[] = {"AnotherExecutable2", "serverId=bar", "a=}b=1", "c=2", "d=3", "e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure3() {
    using namespace karabo::util;
    const char* argv[] = {"AnotherExecutable3", "serverId=bla", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}}}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure4() {
    using namespace karabo::util;
    const char* argv[] = {"AnotherExecutable4", "serverId=bang", "a={{b=1}", "c=2", "d=3", "e={x=15", "y=88}}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}
