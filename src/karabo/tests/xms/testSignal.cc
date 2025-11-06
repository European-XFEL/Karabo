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
 * File:   testSignal.cc
 *
 * Created on Feb 9, 2023
 */

#include <gtest/gtest.h>

#include "karabo/xms/Signal.hh"
#include "karabo/xms/SignalSlotable.hh"


class TestSignal : public ::testing::Test {
   protected:
    TestSignal() {}
    ~TestSignal() override {}
    void SetUp() override;
    void TearDown() override {}
};


void TestSignal::SetUp() {
    // Logger::configure(Hash("priority", "ERROR"));
    // Logger::useConsole();
    //  Event loop is started in xmsTestRunner.cc's main()
    //  Store broker environment variable
}


TEST_F(TestSignal, testRegisterSlots) {
    auto sigSlot = karabo::xms::SignalSlotable::MakeShared("one");
    // sigSlot->start(); not needed here to start communication

    karabo::xms::Signal s(sigSlot.get(), sigSlot->getConnection(), sigSlot->getInstanceId(), "mySignal");

    //  test register
    ASSERT_TRUE(s.registerSlot("otherId", "slotA"));
    ASSERT_TRUE(!s.registerSlot("otherId", "slotA")); // cannot register twice
    ASSERT_TRUE(s.registerSlot("otherId", "slotB"));

    // test unregister
    ASSERT_TRUE(!s.unregisterSlot("otherId", "slotC"));  // unknown slot
    ASSERT_TRUE(!s.unregisterSlot("otherId2", "slotA")); // unknown instance
    ASSERT_TRUE(s.unregisterSlot("otherId", "slotA"));
    ASSERT_TRUE(!s.unregisterSlot("otherId", "slotA")); // already unregistered
    ASSERT_TRUE(s.unregisterSlot("otherId", ""));       // all remaining unregistered
    ASSERT_TRUE(!s.unregisterSlot("otherId", "slotB")); // already unregistered as remaining
    ASSERT_TRUE(!s.unregisterSlot("otherId", ""));      // already unregistered
    ASSERT_TRUE(!s.unregisterSlot("otherId2 ", ""));    // was never registered
}
