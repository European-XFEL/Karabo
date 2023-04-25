/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   Runner_Test.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on February 21, 2017, 2:37 PM
 */

#ifndef RUNNER_TEST_HH
#define RUNNER_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <karabo/karabo.hpp>

class RunnerDerived : public karabo::core::Runner {
   public:
    static void parseCmd(int argc, const char** argv, karabo::util::Hash& configuration) {
        karabo::core::Runner::parseCommandLine(argc, argv, configuration);
    }
};

class Runner_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Runner_Test);
    CPPUNIT_TEST(testRunnerSuccess);
    CPPUNIT_TEST(testRunnerSuccess2);
    CPPUNIT_TEST(testRunnerFailure1);
    CPPUNIT_TEST(testRunnerFailure2);
    CPPUNIT_TEST(testRunnerFailure3);
    CPPUNIT_TEST(testRunnerFailure4);
    CPPUNIT_TEST_SUITE_END();

   public:
    Runner_Test();
    virtual ~Runner_Test();
    void setUp();
    void tearDown();

   private:
    RunnerDerived* m_runner;
    void testRunnerSuccess();
    void testRunnerSuccess2();
    void testRunnerFailure1();
    void testRunnerFailure2();
    void testRunnerFailure3();
    void testRunnerFailure4();
};

#endif /* RUNNER_TEST_HH */
