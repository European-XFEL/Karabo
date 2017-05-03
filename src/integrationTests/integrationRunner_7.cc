/*
 * File:   integrationRunner.cc
 * Author: haufs
 *
 * Created on Aug 8, 2016, 3:22:00 PM
 */

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>


int main() {
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
    // JW: Save a pointer to the test suite object.
    CPPUNIT_NS::Test* test = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();
    runner.addTest(test);
    runner.run(controller);

    // Print test in a compiler compatible format.
    CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
    outputter.write();

    // Get a vector containing all the tests in the test suite
    const std::vector<CPPUNIT_NS::Test*>& tests = dynamic_cast<CPPUNIT_NS::TestSuite*>(test)->getTests();
    // Generate a results file name based on the first test's name.
    std::ostringstream filename;
    filename << "testresults/" << tests[0]->getName() << ".xml";

    // Output XML for Jenkins CPPunit plugin
    std::ofstream xmlFileOut(filename.str());
    CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
    xmlOut.write();

    return result.wasSuccessful() ? 0 : 1;
}
