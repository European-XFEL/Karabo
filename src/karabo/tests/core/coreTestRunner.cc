/* 
 * File:   coreTestRunner.cc
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on February 21, 2017, 4:11 PM
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
    runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    runner.run(controller);

    // Print test in a compiler compatible format.
    CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
    outputter.write();

    // Output ML for Jenkins CPPunit plugin
    std::ofstream xmlFileOut("testresults/coreTest.xml");
    CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
    xmlOut.write();

    return result.wasSuccessful() ? 0 : 1;
}
