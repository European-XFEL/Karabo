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
 * File:   Exception_Test.hh
 * Author: heisenb
 *
 * Created on September 29, 2016, 5:28 PM
 */

#include <gtest/gtest.h>

#include "karabo/data/types/Exception.hh"

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


TEST(TestException, testMethod) {
    EXPECT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::data::LogicException);
    EXPECT_THROW(throw KARABO_LOGIC_EXCEPTION("Some message"), karabo::data::Exception);
    try {
        throw KARABO_LOGIC_EXCEPTION("error");
    } catch (const std::exception& e) {
        std::string expected("1. Exception =====>  {");
        EXPECT_TRUE(std::string(e.what(), expected.size()) == expected);
    }

    // Test more output.
    // First without propagation:
    try {
        throw KARABO_SIGNALSLOT_EXCEPTION("A nasty problem");
    } catch (const karabo::data::Exception& e) {
        EXPECT_EQ(std::string("SignalSlot Exception"), e.type());
        EXPECT_EQ(std::string("A nasty problem"), e.userFriendlyMsg());
        const std::string details(e.detailedMsg());
        // Detailed message looks like this:
        // 1. Exception =====>  {
        //     Exception Type....:  SignalSlot Exception
        //     Message...........:  A signal slotable problem
        //     File..............:  /[...]/src/karabo/tests/util/Exception_Test.cc
        //     Function..........:  void Exception_Test::testMethod()
        //     Line Number.......:  34
        //     Timestamp.........:  2021-Dec-16 15:17:44.697660
        EXPECT_TRUE(details.find("1. Exception =====>  {") != std::string::npos) << details;
        EXPECT_TRUE(details.find("    Exception Type....:  SignalSlot Exception") != std::string::npos) << details;
        EXPECT_TRUE(details.find("    Message...........:  A nasty problem") != std::string::npos) << details;
        EXPECT_TRUE(details.find("    File..............:  ") != std::string::npos)
              << details; // Don't mind file if test moved
        EXPECT_TRUE(details.find("    Function..........:  ") != std::string::npos) << details; // nor method
        EXPECT_TRUE(details.find("    Line Number.......:  ") != std::string::npos) << details; // nor line number
        EXPECT_TRUE(details.find("    Timestamp.........:  2") != std::string::npos)
              << details; // and for sure not date except millenium

        EXPECT_STREQ(e.what(), details.c_str());
    } catch (...) {
        EXPECT_TRUE(false) << "Expected exception not thrown";
    }

    // Rethrow and tracing
    try {
        doNestedThrow();
    } catch (const karabo::data::Exception& e) {
        EXPECT_STREQ("Propagated Exception", e.type().c_str());
        // Outer most rethrow without extra message
        // User friendly message skips message-less exceptions, but otherwise we get a new line for each with an
        // indented "because: " prefix
        EXPECT_STREQ("Propagated\n  because: A casting problem", e.userFriendlyMsg(false).c_str());

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

        EXPECT_EQ(0ul, exceptWith) << details;
        // For the following message parts just test that order is as expected.
        // NOTE: If some text would not be found, find(..) returns std::string::npos wich is the biggest possible
        // size_t, i.e. the
        //       test with it on the right hand side would still succeed, but the next test with it on the left would
        //       fail.
        EXPECT_GT(except1, exceptWith);
        EXPECT_GT(type1, except1);
        EXPECT_GT(mesg1, type1);
        EXPECT_GT(file1, mesg1);
        EXPECT_GT(func1, file1);
        EXPECT_GT(line1, func1);
        EXPECT_GT(stamp1, line1);

        EXPECT_LT(stamp1, except2);
        EXPECT_LT(except2, type2);
        EXPECT_LT(type2, mesg2);
        EXPECT_LT(mesg2, file2);
        EXPECT_LT(file2, func2);
        EXPECT_LT(func2, line2);
        EXPECT_LT(line2, stamp2);

        EXPECT_LT(stamp2, except3);
        EXPECT_LT(except3, type3);
        // Default propagated exception from rethrow has no message
        EXPECT_EQ(std::string::npos, mesg3) << details;
        EXPECT_LT(type3, file3);
        EXPECT_LT(file3, func3);
        EXPECT_LT(func3, line3);
        EXPECT_LT(line3, stamp3);
        // The last one we have to check explicitly against npos:
        EXPECT_TRUE(std::string::npos != stamp3) << details;

        // Involved exceptions do not have details:
        EXPECT_EQ(std::string::npos, details.find("Details...........:")) << details;

        // Call to detailedMsg() cleared the exception stack trace, so we cannot just test details == e.what().
        // Instead we need to test the details twice.
    } catch (...) {
        EXPECT_TRUE(false) << "Expected exception not thrown";
    }

    try {
        doNestedThrow();
    } catch (const karabo::data::Exception& e) {
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

        EXPECT_EQ(0ul, exceptWith) << details;
        // For the following message parts just test that order is as expected.
        // NOTE: If some text would not be found, find(..) returns std::string::npos wich is the biggest possible
        // size_t, i.e. the
        //       test with it on the right hand side would still succeed, but the next test with it on the left would
        //       fail.
        EXPECT_LT(exceptWith, except1);
        EXPECT_LT(except1, type1);
        EXPECT_LT(type1, mesg1);
        EXPECT_LT(mesg1, file1);
        EXPECT_LT(file1, func1);
        EXPECT_LT(func1, line1);
        EXPECT_LT(line1, stamp1);

        EXPECT_LT(stamp1, except2);
        EXPECT_LT(except2, type2);
        EXPECT_LT(type2, mesg2);
        EXPECT_LT(mesg2, file2);
        EXPECT_LT(file2, func2);
        EXPECT_LT(func2, line2);
        EXPECT_LT(line2, stamp2);

        EXPECT_LT(stamp2, except3);
        EXPECT_LT(except3, type3);
        // Default propagated exception from rethrow has no message
        EXPECT_EQ(std::string::npos, mesg3) << details;
        EXPECT_LT(type3, file3);
        EXPECT_LT(file3, func3);
        EXPECT_LT(func3, line3);
        EXPECT_LT(line3, stamp3);
        // The last one we have to check explicitly against npos:
        EXPECT_TRUE(std::string::npos != stamp3) << details;

        // Involved exceptions do not have details:
        EXPECT_EQ(std::string::npos, details.find("Details...........:")) << details;
    } catch (...) {
        EXPECT_TRUE(false) << "Expected exception not thrown";
    }

    // Rethrow and tracing
    try {
        doNestedThrow();
    } catch (const karabo::data::Exception& e) {
        EXPECT_STREQ("Propagated Exception", e.type().c_str());
        // Outer most rethrow without extra message
        // User friendly message skips message-less exceptions, but otherwise we get a new line for each with an
        // indented "because: " prefix
        EXPECT_STREQ("Propagated\n  because: A casting problem", e.userFriendlyMsg(true).c_str());
        // Previous call to userFriendlyMsg(true) cleared the stack trace, so a further call has only the most recent
        // exception Since that was triggered by a simple KARABO_RETHROW it has an empty message, so the exception type
        // is printed.
        EXPECT_STREQ("Propagated Exception", e.userFriendlyMsg().c_str());
    }
}


TEST(TestException, testDetails) {
    try {
        throw KARABO_PYTHON_EXCEPTION("Some message");
    } catch (const karabo::data::PythonException& e) {
        EXPECT_STREQ("Some message", e.userFriendlyMsg(true).c_str());
        // No second argument given, so no details:
        EXPECT_STREQ("", e.details().c_str());
    } catch (...) {
        EXPECT_TRUE(false) << "Missed PythonException";
    }

    try {
        throw KARABO_PYTHON_EXCEPTION2("Some message", "...with details!");
    } catch (const karabo::data::PythonException& e) {
        EXPECT_STREQ("Some message", e.userFriendlyMsg(false).c_str());
        EXPECT_STREQ("...with details!", e.details().c_str());
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        EXPECT_TRUE(std::string::npos != fullMsg.find("Some message")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("Details...........:")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("...with details!")) << fullMsg;
    } catch (...) {
        EXPECT_TRUE(false) << "Missed PythonException";
    }


    try {
        throw karabo::data::RemoteException("A message", "bob", "Details are usually the trace. Not now...");
    } catch (const karabo::data::RemoteException& e) {
        EXPECT_STREQ("Remote Exception from bob", e.type().c_str());
        EXPECT_STREQ("Details are usually the trace. Not now...", e.details().c_str());
        EXPECT_STREQ("A message", e.userFriendlyMsg(false).c_str());
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        EXPECT_TRUE(std::string::npos != fullMsg.find("A message")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("Details...........:")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("Details are usually the trace. Not now...")) << fullMsg;
    } catch (...) {
        EXPECT_TRUE(false) << "Missed RemoteException";
    }


    try {
        throw karabo::data::IOException("A message", "filename", "function", 42,
                                        "Details are usually the trace, e.g. from hdf5 code");
    } catch (const karabo::data::IOException& e) {
        EXPECT_STREQ("IO Exception", e.type().c_str());
        EXPECT_STREQ("Details are usually the trace, e.g. from hdf5 code", e.details().c_str());
        EXPECT_STREQ("A message", e.userFriendlyMsg(false).c_str());
        // Now check that both, message and details are in the trace:
        const std::string fullMsg(e.detailedMsg());
        EXPECT_TRUE(std::string::npos != fullMsg.find("A message")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("Details...........:")) << fullMsg;
        EXPECT_TRUE(std::string::npos != fullMsg.find("Details are usually the trace, e.g. from hdf5 code")) << fullMsg;
    } catch (...) {
        EXPECT_TRUE(false) << "Missed IOException";
    }
}


TEST(TestException, testTraceOrder) {
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
    } catch (const karabo::data::Exception& e) {
        shortMsg = e.userFriendlyMsg(false);
        stackMsg = e.detailedMsg();
    }

    // Short message is an "argumentation chain", so last exception first, i.e. here:
    // "Exception 3\n  because: Exception 2\n     because: Exception 1"
    // Here we just test the order, not the indentation or the "because:" prefix:
    const size_t pos1Short = shortMsg.find("Exception 1");
    const size_t pos2Short = shortMsg.find("Exception 2");
    const size_t pos3Short = shortMsg.find("Exception 3");
    EXPECT_TRUE(pos3Short < pos2Short) << shortMsg;          // 3 is before 2
    EXPECT_TRUE(pos2Short < pos1Short) << shortMsg;          // 2 is before 1
    EXPECT_TRUE(pos1Short != std::string::npos) << shortMsg; // 1 exists (npos is the biggest size_t)

    // In detailed message, the exception stack is ordered from inner to outer as can be seen in
    // Exception_Test::testMethod Here we just test the order, not all the other stack print formatting.
    const size_t pos1Stack = stackMsg.find("Exception 1");
    const size_t pos2Stack = stackMsg.find("Exception 2");
    const size_t pos3Stack = stackMsg.find("Exception 3");
    EXPECT_TRUE(pos1Stack < pos2Stack) << stackMsg;
    EXPECT_TRUE(pos2Stack < pos3Stack) << stackMsg;
    EXPECT_TRUE(pos3Stack != std::string::npos) << stackMsg;
}
