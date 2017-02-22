/* 
 * File:   Runner_Test.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 * 
 * Created on February 21, 2017, 2:37 PM
 */

#include <karabo/karabo.hpp>
#include "Runner_Test.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Runner_Test);


Runner_Test::Runner_Test() {
}


Runner_Test::~Runner_Test() {
}


void Runner_Test::setUp() {
}


void Runner_Test::tearDown() {
}


void Runner_Test::testRunnerSuccess() {
    using namespace karabo::util;
    char *argv[] = {"SomeExecutable", "serverId=foo",
                   "autostart[0]={DataLoggerManager.serverList=dls1,dls2,dls3,dls4",
                   "DataLoggerManager.param1=12", "DataLoggerManager.param2=99}",
                   "visibility=5", "Logger.priority=DEBUG"};
    int argc = 7;
    Hash configuration;
    RunnerDerived::parseCmd(argc, argv, configuration);
    //std::clog << "**** configuration ...\n" << configuration << std::endl;
    CPPUNIT_ASSERT(configuration.get<std::string>("serverId") == "foo");
    CPPUNIT_ASSERT(configuration.get<std::string>("autostart[0].DataLoggerManager.serverList") == "dls1,dls2,dls3,dls4");
    CPPUNIT_ASSERT(configuration.get<std::string>("autostart[0].DataLoggerManager.param1") == "12");
    CPPUNIT_ASSERT(configuration.get<std::string>("autostart[0].DataLoggerManager.param2") == "99");
    CPPUNIT_ASSERT(configuration.get<std::string>("visibility") == "5");
    CPPUNIT_ASSERT(configuration.get<std::string>("Logger.priority") == "DEBUG");
}


void Runner_Test::testRunnerFailure1() {
    using namespace karabo::util;
    char *argv[] = {"AnotherExecutable", "serverId=bar", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure2() {
    using namespace karabo::util;
    char *argv[] = {"AnotherExecutable2", "serverId=bar", "a=}b=1", "c=2", "d=3", "e={x=15", "y=88}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure3() {
    using namespace karabo::util;
    char *argv[] = {"AnotherExecutable3", "serverId=bla", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}}}"};
    const int argc = sizeof(argv) / sizeof(argv[0]);
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}


void Runner_Test::testRunnerFailure4() {
    using namespace karabo::util;
    char *argv[] = {"AnotherExecutable4", "serverId=bang", "a={b=1", "c=2", "d=3", "e={x=15", "y=88}}"};
    const int argc = sizeof(argv) / sizeof(argv[0]) - 1;
    Hash configuration;
    CPPUNIT_ASSERT_THROW(RunnerDerived::parseCmd(argc, argv, configuration), ParameterException);
}
