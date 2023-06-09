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

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

int run_test(int argc, char* argv[]) {
    // USAGE:
    // TestRunner (argv[0])
    // # will run the tests and generate a file named TestRunner.xml in the current directory
    // TestRunner path/to/filename.xml
    // # will run the tests and generate a file path/to/filename.xml in the specified path
    // TestRunner path/to/filename.xml testname
    // # will run the tests matching the argument testname and generate a file.

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
    if (argc > 2) {
        // one can run a single test by passing the class name as an argumnent. e.g. States_Test
        try {
            test = test->findTest(argv[2]);
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

    std::ostringstream filename;
    if (argc > 1) {
        filename << argv[1];
    } else {
        filename << argv[0] << ".xml";
    }
    std::clog << "Writing " << filename.str() << std::endl;
    // Output XML for Jenkins CPPunit plugin
    std::ofstream xmlFileOut(filename.str());
    CPPUNIT_NS::XmlOutputter xmlOut(&result, xmlFileOut);
    xmlOut.write();
    return result.wasSuccessful() ? 0 : 1;
}
