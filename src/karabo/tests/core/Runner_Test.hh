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
    static void parseCmd(int argc, const char** argv, karabo::data::Hash& configuration) {
        karabo::core::Runner::parseCommandLine(argc, argv, configuration);
    }
};

class Runner_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(Runner_Test);
    CPPUNIT_TEST(testRunnerSuccess);
    CPPUNIT_TEST(testRunnerSuccess2);
    CPPUNIT_TEST(testRunnerSuccess3);
    CPPUNIT_TEST(testRunnerSuccess4);
    CPPUNIT_TEST(testRunnerFailure);
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
    void testRunnerSuccess3();
    void testRunnerSuccess4();
    void testRunnerFailure();
};

#endif /* RUNNER_TEST_HH */
