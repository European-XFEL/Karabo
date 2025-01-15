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
 * File:   Exception_Test.cc
 * Author: heisenb
 *
 * Created on September 29, 2016, 5:28 PM
 */

#include "Exception_Test.hh"

#include "karabo/util/Exception.hh"

CPPUNIT_TEST_SUITE_REGISTRATION(Exception_Test);


Exception_Test::Exception_Test() {}


Exception_Test::~Exception_Test() {}

void doNestedThrow() {
    try {
        try {
            throw KARABO_CAST_EXCEPTION("A casting problem");
        } catch (const std::exception&) {
            KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Propagated"));
        }
    } catch (...) {
        // Rethrow without message is also a KARABO_PROPAGATED_EXCEPTION, but with empty message
        KARABO_RETHROW;
    }
}


void Exception_Test::testMethod() {
    CPPUNIT_ASSERT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::util::LogicException);
    CPPUNIT_ASSERT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::util::Exception);
    try {
        throw KARABO_LOGIC_EXCEPTION("error");
    } catch (const std::exception& e) {
        std::string expected("1. Exception =====>  {");
        CPPUNIT_ASSERT(std::string(e.what(), expected.size()) == expected);
    }

    // Test more output.
    // First without propagation:
    try {
        throw KARABO_SIGNALSLOT_EXCEPTION("A nasty problem");
    } catch (const karabo::util::Exception& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("SignalSlot Exception"), e.type());
        CPPUNIT_ASSERT_EQUAL(std::string("A nasty problem"), e.userFriendlyMsg());
        const std::string details(e.detailedMsg());
        // Detailed message looks like this:
        // 1. Exception =====>  {
        //     Exception Type....:  SignalSlot Exception
        //     Message...........:  A signal slotable problem
        //     File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //     Function..........:  void Exception_Test::testMethod()
        //     Line Number.......:  34
        //     Timestamp.........:  2021-Dec-16 15:17:44.697660
        CPPUNIT_ASSERT_MESSAGE(details, details.find("1. Exception =====>  {") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(details,
                               details.find("    Exception Type....:  SignalSlot Exception") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(details, details.find("    Message...........:  A nasty problem") != std::string::npos);
        CPPUNIT_ASSERT_MESSAGE(
              details, details.find("    File..............:  ") != std::string::npos); // Don't mind file if test moved
        CPPUNIT_ASSERT_MESSAGE(details, details.find("    Function..........:  ") != std::string::npos); // nor method
        CPPUNIT_ASSERT_MESSAGE(details,
                               details.find("    Line Number.......:  ") != std::string::npos); // nor line number
        CPPUNIT_ASSERT_MESSAGE(details, details.find("    Timestamp.........:  2") !=
                                              std::string::npos); // and for sure not date except millenium

        CPPUNIT_ASSERT_EQUAL(std::string(e.what()), details);
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Expected exception not thrown", false);
    }

    // Rethrow and tracing
    try {
        doNestedThrow();
    } catch (const karabo::util::Exception& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("Propagated Exception"), e.type());
        // Outer most rethrow without extra message
        // User friendly message skips message-less exceptions, but otherwise we get a new line for each with an
        // indented "because: " prefix
        CPPUNIT_ASSERT_EQUAL(std::string("Propagated\n  because: A casting problem"), e.userFriendlyMsg(false));

        const std::string details = e.detailedMsg();
        // Detailed message looks e.g. like this:
        // Exception with trace (listed from inner to outer):
        // 1. Exception =====>  {
        //     Exception Type....:  Cast Exception
        //     Message...........:  A casting problem
        //     File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //     Function..........:  void doNestedThrow()
        //     Line Number.......:  24
        //     Timestamp.........:  2021-Dec-16 16:21:57.353584
        // }

        //    2. Exception =====>  {
        //        Exception Type....:  Propagated Exception
        //        Message...........:  Propagated
        //        File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //        Function..........:  void doNestedThrow()
        //        Line Number.......:  26
        //        Timestamp.........:  2021-Dec-16 16:21:57.353598
        //    }

        //       3. Exception =====>  {
        //           Exception Type....:  Propagated Exception
        //           File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //           Function..........:  void doNestedThrow()
        //           Line Number.......:  29
        //           Timestamp.........:  2021-Dec-16 16:21:57.353614
        //       }
        const size_t exceptWith = details.find("Exception with trace (listed from inner to outer):");
        const size_t except1 = details.find("1. Exception =====>  {");
        const size_t type1 = details.find("    Exception Type....:  Cast Exception");
        const size_t mesg1 = details.find("    Message...........:  A casting problem");
        const size_t file1 = details.find("    File..............:  "); // skip file
        const size_t func1 = details.find("    Function..........:  void doNestedThrow()");
        const size_t line1 = details.find("    Line Number.......:  ");   // skip exact number
        const size_t stamp1 = details.find("    Timestamp.........:  2"); // skip date except millenium
        // Now three more spaces before
        const size_t except2 = details.find("   2. Exception =====>  {");
        const size_t type2 = details.find("       Exception Type....:  Propagated Exception");
        const size_t mesg2 = details.find("       Message...........:  Propagated");
        const size_t file2 = details.find("       File..............:  "); // skip file
        const size_t func2 = details.find("       Function..........:  void doNestedThrow()");
        const size_t line2 = details.find("       Line Number.......:  ");   // skip exact number
        const size_t stamp2 = details.find("       Timestamp.........:  2"); // skip date except millenium
        // Even three more spaces, no message
        const size_t except3 = details.find("   3. Exception =====>  {");
        const size_t type3 = details.find("          Exception Type....:  Propagated Exception");
        const size_t mesg3 = details.find("          Message...........:  "); // not printed since empty
        const size_t file3 = details.find("          File..............:  "); // skip file
        const size_t func3 = details.find("          Function..........:  void doNestedThrow()");
        const size_t line3 = details.find("          Line Number.......:  ");   // skip exact number
        const size_t stamp3 = details.find("          Timestamp.........:  2"); // skip date except millenium

        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, 0ul, exceptWith);
        // For the following message parts just test that order is as expected.
        // NOTE: If some text would not be found, find(..) returns std::string::npos wich is the biggest possible
        // size_t, i.e. the
        //       test with it on the right hand side would still succeed, but the next test with it on the left would
        //       fail.
        CPPUNIT_ASSERT_GREATER(exceptWith, except1);
        CPPUNIT_ASSERT_GREATER(except1, type1);
        CPPUNIT_ASSERT_GREATER(type1, mesg1);
        CPPUNIT_ASSERT_GREATER(mesg1, file1);
        CPPUNIT_ASSERT_GREATER(file1, func1);
        CPPUNIT_ASSERT_GREATER(func1, line1);
        CPPUNIT_ASSERT_GREATER(line1, stamp1);

        CPPUNIT_ASSERT_GREATER(stamp1, except2);
        CPPUNIT_ASSERT_GREATER(except2, type2);
        CPPUNIT_ASSERT_GREATER(type2, mesg2);
        CPPUNIT_ASSERT_GREATER(mesg2, file2);
        CPPUNIT_ASSERT_GREATER(file2, func2);
        CPPUNIT_ASSERT_GREATER(func2, line2);
        CPPUNIT_ASSERT_GREATER(line2, stamp2);

        CPPUNIT_ASSERT_GREATER(stamp2, except3);
        CPPUNIT_ASSERT_GREATER(except3, type3);
        // Default propagated exception from rethrow has no message
        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, std::string::npos, mesg3);
        CPPUNIT_ASSERT_GREATER(type3, file3);
        CPPUNIT_ASSERT_GREATER(file3, func3);
        CPPUNIT_ASSERT_GREATER(func3, line3);
        CPPUNIT_ASSERT_GREATER(line3, stamp3);
        // The last one we have to check explicitly against npos:
        CPPUNIT_ASSERT_MESSAGE(details, std::string::npos != stamp3);

        // Involved exceptions do not have details:
        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, std::string::npos, details.find("Details...........:"));

        // Call to detailedMsg() cleared the exception stack trace, so we cannot just test details == e.what().
        // Instead we need to test the details twice.
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Expected exception not thrown", false);
    }

    try {
        doNestedThrow();
    } catch (const karabo::util::Exception& e) {
        // Redo exactly the same as for e.detailedMsg(), see above.
        const std::string details = e.what();
        // Detailed message looks e.g. like this:
        // Exception with trace (listed from inner to outer):
        // 1. Exception =====>  {
        //     Exception Type....:  Cast Exception
        //     Message...........:  A casting problem
        //     File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //     Function..........:  void doNestedThrow()
        //     Line Number.......:  24
        //     Timestamp.........:  2021-Dec-16 16:21:57.353584
        // }

        //    2. Exception =====>  {
        //        Exception Type....:  Propagated Exception
        //        Message...........:  Propagated
        //        File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //        Function..........:  void doNestedThrow()
        //        Line Number.......:  26
        //        Timestamp.........:  2021-Dec-16 16:21:57.353598
        //    }

        //       3. Exception =====>  {
        //           Exception Type....:  Propagated Exception
        //           File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //           Function..........:  void doNestedThrow()
        //           Line Number.......:  29
        //           Timestamp.........:  2021-Dec-16 16:21:57.353614
        //       }
        const size_t exceptWith = details.find("Exception with trace (listed from inner to outer):");
        const size_t except1 = details.find("1. Exception =====>  {");
        const size_t type1 = details.find("    Exception Type....:  Cast Exception");
        const size_t mesg1 = details.find("    Message...........:  A casting problem");
        const size_t file1 = details.find("    File..............:  "); // skip file
        const size_t func1 = details.find("    Function..........:  void doNestedThrow()");
        const size_t line1 = details.find("    Line Number.......:  ");   // skip exact number
        const size_t stamp1 = details.find("    Timestamp.........:  2"); // skip date except millenium
        // Now three more spaces before
        const size_t except2 = details.find("   2. Exception =====>  {");
        const size_t type2 = details.find("       Exception Type....:  Propagated Exception");
        const size_t mesg2 = details.find("       Message...........:  Propagated");
        const size_t file2 = details.find("       File..............:  "); // skip file
        const size_t func2 = details.find("       Function..........:  void doNestedThrow()");
        const size_t line2 = details.find("       Line Number.......:  ");   // skip exact number
        const size_t stamp2 = details.find("       Timestamp.........:  2"); // skip date except millenium
        // Even three more spaces, no message
        const size_t except3 = details.find("   3. Exception =====>  {");
        const size_t type3 = details.find("          Exception Type....:  Propagated Exception");
        const size_t mesg3 = details.find("          Message...........:  "); // not printed since empty
        const size_t file3 = details.find("          File..............:  "); // skip file
        const size_t func3 = details.find("          Function..........:  void doNestedThrow()");
        const size_t line3 = details.find("          Line Number.......:  ");   // skip exact number
        const size_t stamp3 = details.find("          Timestamp.........:  2"); // skip date except millenium

        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, 0ul, exceptWith);
        // For the following message parts just test that order is as expected.
        // NOTE: If some text would not be found, find(..) returns std::string::npos wich is the biggest possible
        // size_t, i.e. the
        //       test with it on the right hand side would still succeed, but the next test with it on the left would
        //       fail.
        CPPUNIT_ASSERT_GREATER(exceptWith, except1);
        CPPUNIT_ASSERT_GREATER(except1, type1);
        CPPUNIT_ASSERT_GREATER(type1, mesg1);
        CPPUNIT_ASSERT_GREATER(mesg1, file1);
        CPPUNIT_ASSERT_GREATER(file1, func1);
        CPPUNIT_ASSERT_GREATER(func1, line1);
        CPPUNIT_ASSERT_GREATER(line1, stamp1);

        CPPUNIT_ASSERT_GREATER(stamp1, except2);
        CPPUNIT_ASSERT_GREATER(except2, type2);
        CPPUNIT_ASSERT_GREATER(type2, mesg2);
        CPPUNIT_ASSERT_GREATER(mesg2, file2);
        CPPUNIT_ASSERT_GREATER(file2, func2);
        CPPUNIT_ASSERT_GREATER(func2, line2);
        CPPUNIT_ASSERT_GREATER(line2, stamp2);

        CPPUNIT_ASSERT_GREATER(stamp2, except3);
        CPPUNIT_ASSERT_GREATER(except3, type3);
        // Default propagated exception from rethrow has no message
        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, std::string::npos, mesg3);
        CPPUNIT_ASSERT_GREATER(type3, file3);
        CPPUNIT_ASSERT_GREATER(file3, func3);
        CPPUNIT_ASSERT_GREATER(func3, line3);
        CPPUNIT_ASSERT_GREATER(line3, stamp3);
        // The last one we have to check explicitly against npos:
        CPPUNIT_ASSERT_MESSAGE(details, std::string::npos != stamp3);

        // Involved exceptions do not have details:
        CPPUNIT_ASSERT_EQUAL_MESSAGE(details, std::string::npos, details.find("Details...........:"));
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Expected exception not thrown", false);
    }

    // Rethrow and tracing
    try {
        doNestedThrow();
    } catch (const karabo::util::Exception& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("Propagated Exception"), e.type());
        // Outer most rethrow without extra message
        // User friendly message skips message-less exceptions, but otherwise we get a new line for each with an
        // indented "because: " prefix
        CPPUNIT_ASSERT_EQUAL(std::string("Propagated\n  because: A casting problem"), e.userFriendlyMsg(true));
        // Previous call to userFriendlyMsg(true) cleared the stack trace, so a further call has only the most recent
        // exception Since that was triggered by a simple KARABO_RETHROW it has an empty message, so the exception type
        // is printed.
        CPPUNIT_ASSERT_EQUAL(std::string("Propagated Exception"), e.userFriendlyMsg());
    }
}


void Exception_Test::testDetails() {
    try {
        throw KARABO_PYTHON_EXCEPTION("Some message");
    } catch (const karabo::util::PythonException& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("Some message"), e.userFriendlyMsg(true));
        // No second argument given, so no details:
        CPPUNIT_ASSERT_EQUAL(std::string(), e.details());
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Missed PythonException", false);
    }

    try {
        throw KARABO_PYTHON_EXCEPTION2("Some message", "...with details!");
    } catch (const karabo::util::PythonException& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("Some message"), e.userFriendlyMsg(false));
        CPPUNIT_ASSERT_EQUAL(std::string("...with details!"), e.details());
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("Some message"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("Details...........:"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("...with details!"));
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Missed PythonException", false);
    }


    try {
        throw karabo::util::RemoteException("A message", "bob", "Details are usually the trace. Not now...");
    } catch (const karabo::util::RemoteException& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("Remote Exception from bob"), e.type());
        CPPUNIT_ASSERT_EQUAL(std::string("Details are usually the trace. Not now..."), e.details());
        CPPUNIT_ASSERT_EQUAL(std::string("A message"), e.userFriendlyMsg(false));
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("A message"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("Details...........:"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("Details are usually the trace. Not now..."));
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Missed RemoteException", false);
    }


    try {
        throw karabo::util::IOException("A message", "filename", "function", 42,
                                        "Details are usually the trace, e.g. from hdf5 code");
    } catch (const karabo::util::IOException& e) {
        CPPUNIT_ASSERT_EQUAL(std::string("IO Exception"), e.type());
        CPPUNIT_ASSERT_EQUAL(std::string("Details are usually the trace, e.g. from hdf5 code"), e.details());
        CPPUNIT_ASSERT_EQUAL(std::string("A message"), e.userFriendlyMsg(false));
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("A message"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg, std::string::npos != fullMsg.find("Details...........:"));
        CPPUNIT_ASSERT_MESSAGE(fullMsg,
                               std::string::npos != fullMsg.find("Details are usually the trace, e.g. from hdf5 code"));
    } catch (...) {
        CPPUNIT_ASSERT_MESSAGE("Missed IOException", false);
    }
}


void Exception_Test::testTraceOrder() {
    // Check ordering of exception stack in detailedMsg() and userFriendlyMsg()
    std::string shortMsg, stackMsg;
    try {
        try {
            try {
                throw KARABO_CAST_EXCEPTION("Exception 1");
            } catch (const std::exception&) {
                KARABO_RETHROW_MSG("Exception 2");
            }
        } catch (const std::exception&) {
            KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Exception 3"));
        }
    } catch (const karabo::util::Exception& e) {
        shortMsg = e.userFriendlyMsg(false);
        stackMsg = e.detailedMsg();
    }

    // Short message is an "argumentation chain", so last exception first, i.e. here:
    // "Exception 3\n  because: Exception 2\n     because: Exception 1"
    // Here we just test the order, not the indentation or the "because:" prefix:
    const size_t pos1Short = shortMsg.find("Exception 1");
    const size_t pos2Short = shortMsg.find("Exception 2");
    const size_t pos3Short = shortMsg.find("Exception 3");
    CPPUNIT_ASSERT_MESSAGE(shortMsg, pos3Short < pos2Short);          // 3 is before 2
    CPPUNIT_ASSERT_MESSAGE(shortMsg, pos2Short < pos1Short);          // 2 is before 1
    CPPUNIT_ASSERT_MESSAGE(shortMsg, pos1Short != std::string::npos); // 1 exists (npos is the biggest size_t)

    // In detailed message, the exception stack is ordered from inner to outer as can be seen in
    // Exception_Test::testMethod Here we just test the order, not all the other stack print formatting.
    const size_t pos1Stack = stackMsg.find("Exception 1");
    const size_t pos2Stack = stackMsg.find("Exception 2");
    const size_t pos3Stack = stackMsg.find("Exception 3");
    CPPUNIT_ASSERT_MESSAGE(shortMsg, pos1Stack < pos2Stack);
    CPPUNIT_ASSERT_MESSAGE(shortMsg, pos2Stack < pos3Stack);
    CPPUNIT_ASSERT_MESSAGE(stackMsg, pos3Stack != std::string::npos);
}
