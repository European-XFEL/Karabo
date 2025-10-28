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

#include <gtest/gtest.h>

#include <karabo/karabo.hpp>

class RunnerDerived : public karabo::core::Runner {
   public:
    static void parseCmd(int argc, const char** argv, karabo::data::Hash& configuration) {
        karabo::core::Runner::parseCommandLine(argc, argv, configuration);
    }
};

TEST(TestRunner, testRunnerSuccess) {
    using namespace karabo::data;
    std::string initval =
          "{\"KaraboDataLoggerManager\": {\"classId\": \"DataLoggerManager\", "
          "\"serverList\": [\"dls1\", \"dls2\", \"dls3\", \"dls4\"], \"param1\": 12, "
          "\"param2\": 99}}";

    std::string initStr = std::string("init=") + initval;
    char const* argv[] = {"SomeExecutable", "serverId=foo", initStr.c_str(), "log.level=DEBUG"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    EXPECT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));
    EXPECT_TRUE(configuration.get<std::string>("serverId") == "foo");
    EXPECT_TRUE(configuration.get<std::string>("init") == initval);
    EXPECT_TRUE(configuration.get<std::string>("log.level") == "DEBUG");
}


TEST(TestRunner, testRunnerSuccess2) {
    using namespace karabo::data;
    const char* argv[] = {"AnotherExecutable5", "serverId=bingo", "node.subnode.position=20", "node.subnode.value=2,1"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    EXPECT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));
    EXPECT_TRUE(configuration.get<std::string>("serverId") == "bingo");
    EXPECT_TRUE(configuration.get<std::string>("node.subnode.position") == "20");
    EXPECT_TRUE(configuration.get<std::string>("node.subnode.value") == "2,1");
}


TEST(TestRunner, testRunnerSuccess3) {
    using namespace karabo::data;
    const char* argv[] = {"AnotherExecutable", "serverId=bar", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    EXPECT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));
    EXPECT_TRUE(configuration.get<std::string>("serverId") == "bar");
    EXPECT_TRUE(configuration.get<std::string>("a") == "{b=1");
}


TEST(TestRunner, testRunnerSuccess4) {
    using namespace karabo::data;
    // tests an empty string
    const char* argv[] = {"AnotherExecutable", "serverId="};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    EXPECT_NO_THROW(RunnerDerived::parseCmd(argc, argv, configuration));
    EXPECT_TRUE(configuration.get<std::string>("serverId") == "");
}


TEST(TestRunner, testRunnerFailure) {
    using namespace karabo::data;
    // tests non completed kwarg
    const char* argv[] = {"AnotherExecutable", "serverId"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    EXPECT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}
