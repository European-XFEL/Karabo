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

#include <gtest/gtest.h>

// Cheat to be able to call Slot::callRegisteredSlotFunctions:
#define protected public

#include "karabo/data/types/Hash.hh"
#include "karabo/util/PackParameters.hh"
#include "karabo/xms/Slot.hh"


struct Foo {
    Foo() {}

    Foo(const Foo&) {
        ++nCopies;
    }

    static int nCopies;
};

int Foo::nCopies = 0;


TEST(TestSlot, testCallSlot) {
    using MySlot = karabo::xms::SlotN<void, int, Foo>;
    MySlot slot("slot");

    const Foo* fooAddressInFunc = nullptr;
    std::string sender;
    auto slotLambda = [&slot, &fooAddressInFunc, &sender](const int& i, const Foo& foo) {
        fooAddressInFunc = &foo;
        sender = slot.getInstanceIdOfSender();
    };
    const MySlot::SlotHandler func = slotLambda;
    slot.registerSlotFunction(func);

    auto h = karabo::data::Hash::MakeShared();
    karabo::util::pack(*h, 1, Foo()); // packing into h under keys "a1" and "a2"
    ASSERT_EQ(1, Foo::nCopies);       // was copied into 'h'
    Foo const* const fooAddressInHash = &(h->get<Foo>("a2"));

    auto header = karabo::data::Hash::MakeShared("signalInstanceId", "senderId");
    slot.callRegisteredSlotFunctions(header, h);

    ASSERT_EQ(1, Foo::nCopies); // no further copy
    ASSERT_EQ(fooAddressInHash, fooAddressInFunc);

    ASSERT_EQ("senderId", sender);

    // Using an intermediate std::function with args by value, there are copies:
    MySlot slot2("slot2");
    std::function<void(int, Foo)> func2 = slotLambda;
    fooAddressInFunc = nullptr;
    slot2.registerSlotFunction(func2);
    auto dummyHeader = karabo::data::Hash::MakeShared();
    slot2.callRegisteredSlotFunctions(dummyHeader, h); // Do not care about header here

    ASSERT_LT(1, Foo::nCopies);                        // In fact I see 3, i.e. two extra copies,
    ASSERT_TRUE(fooAddressInHash != fooAddressInFunc); // and copies lead to a new address.

    // Now a function that has args by value
    MySlot slot3("slot3");
    MySlot::SlotHandler slotFunc3 = [&fooAddressInFunc](int i, Foo foo) { fooAddressInFunc = &foo; };
    fooAddressInFunc = nullptr;
    Foo::nCopies = 0;
    slot3.registerSlotFunction(slotFunc3);

    slot3.callRegisteredSlotFunctions(dummyHeader, h); // Do not care about header here, neither

    ASSERT_EQ(1, Foo::nCopies); // Now there is one copy
    ASSERT_TRUE(fooAddressInHash != fooAddressInFunc);

    // Wrong number of arguments
    h->clear();
    karabo::util::pack(*h, 1); // keys "a1"
    ASSERT_THROW(slot3.callRegisteredSlotFunctions(dummyHeader, h), karabo::data::SignalSlotException);
    karabo::util::pack(*h, 1, Foo(), 3.141596); // keys "a1", "a2" and "a3"
    ASSERT_THROW(slot3.callRegisteredSlotFunctions(dummyHeader, h), karabo::data::SignalSlotException);
    h->clear(); // no arguments
    ASSERT_THROW(slot3.callRegisteredSlotFunctions(dummyHeader, h), karabo::data::SignalSlotException);
}
