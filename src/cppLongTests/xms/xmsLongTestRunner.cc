/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   xmsTestRunner.cc
 * Author: heisenb
 *
 * Created on Apr 4, 2013, 1:24:22 PM
 */

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "boost/thread.hpp"
#include "karabo/net/EventLoop.hh"


int main() {
    // uncomment this if ever testing against a local broker
    // setenv("KARABO_BROKER", "tcp://localhost:7777", true);

    boost::thread t(karabo::net::EventLoop::work);

    // Create the event manager and test controller
    CPPUNIT_NS::TestResult controller;

    // Add a listener that colllects test result
    CPPUNIT_NS::TestResultCollector result;
    controller.addListener(&result);

    // Add a listener that print dots as test run.
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener(&progress);

    // Add the top suite to the test runner
    CPPUNIT_NS::TestRunner runner;
    runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller);

    // Print test in a compiler compatible format.
    CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
    outputter.write();

    // Output ML for Jenkins CPPunit plugin - the name of the file must follow the
    // pattern "[dir_name_in_cppLongTests]Test.xml". To see why, please take a
    // look at the function "runCppLongTests()" in file "auto_build_all.sh" at
    // the root of the Git repository that contains this project.
    std::ofstream xmlFileOut("testresults/xmsTest.xml");
    CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
    xmlOut.write();

    karabo::net::EventLoop::stop();
    t.join();

    return result.wasSuccessful() ? 0 : 1;
}
