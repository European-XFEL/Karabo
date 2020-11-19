/*
 * File:   utilTestRunner.cc
 * Author: heisenb
 *
 * Created on Sep 18, 2012, 6:48:00 PM
 */

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>


int main(int argc, char* argv[]) {
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
    CPPUNIT_NS::Test* test = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();
    if (argc>1) {
        // one can run a single test by passing the class name as an argumnent. e.g. States_Test
        try {
            test = test->findTest(argv[1]);
        } catch (const std::invalid_argument&) {
            // bad argument
            std::cerr << "Test '" << argv[1] << "' not found!" << std::endl;
            return 1;
        }
    }
    runner.addTest(test);
    runner.run(controller);

    // Print test in a compiler compatible format.
    CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
    outputter.write();

    // Output ML for Jenkins CPPunit plugin
    std::ofstream xmlFileOut("testresults/utilTest.xml");
    CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
    xmlOut.write();

    return result.wasSuccessful() ? 0 : 1;
}
