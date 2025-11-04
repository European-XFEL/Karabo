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
 * File:   MetaTools_Test.hh
 * Author: heisenb
 *
 * Created on August 11, 2016, 11:18 AM
 */

#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>

#include "karabo/data/types/Hash.hh"
#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"
#include "karabo/util/PackParameters.hh"


using namespace karabo::data;
using namespace karabo::util;

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;


struct MyPublicHash : public karabo::data::Hash {};

struct MyProtectedHash : protected karabo::data::Hash {};

struct MyPrivateHash : private karabo::data::Hash {};


struct PointerTest {
    template <class T>
    static bool isSharedPointer() {
        return isSharedPointer<T>(karabo::data::details::is_shared_ptr<T>());
    }

    template <class T>
    static bool isSharedPointer(std::true_type) {
        return true;
    }

    template <class T>
    static bool isSharedPointer(std::false_type) {
        return false;
    }
};


struct BindWeakTest : public std::enable_shared_from_this<BindWeakTest> {
    int add(const int a, const int b) {
        return a + b;
    }
    int dummyFunction(const int a) const {
        return a;
    }
};


struct Test_SignalSlotable : public std::enable_shared_from_this<Test_SignalSlotable> {
    virtual ~Test_SignalSlotable() {}
};


struct Test_Device : public virtual Test_SignalSlotable {
    boost::asio::steady_timer m_timer;
    std::vector<std::string>* m_messages;

    Test_Device(std::vector<std::string>* messages)
        : m_timer(karabo::net::EventLoop::getIOService()), m_messages(messages) {
        m_messages->push_back("Test_Device created");
    }

    void init() {
        // This is just testing that binding a const member function compiles - both with a this which is const or not:
        using std::placeholders::_1;
        using std::placeholders::_2;
        m_timer.expires_after(std::chrono::milliseconds(100));
        m_timer.async_wait(karabo::util::bind_weak(&Test_Device::dummyConstFunction, this, 0, _1));
        m_timer.async_wait(
              karabo::util::bind_weak(&Test_Device::dummyConstFunction, const_cast<const Test_Device*>(this), 0, _1));
        m_timer.cancel();

        // This is just testing that binding a member function that returns a value works
        {
            std::shared_ptr<BindWeakTest> bindWeakTest = std::make_shared<BindWeakTest>();

            auto f1 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), _1, _2);
            int v = f1(1, 1);
            EXPECT_TRUE(v == 2);

            auto f2 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), 1, _1);
            v = f2(1);
            EXPECT_TRUE(v == 2);

            auto f3 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), _1, 1);
            v = f3(1);
            EXPECT_TRUE(v == 2);

            auto f4 = karabo::util::bind_weak(&BindWeakTest::dummyFunction,
                                              const_cast<const BindWeakTest*>(bindWeakTest.get()), _1);
            v = f4(1);
            EXPECT_TRUE(v == 1);

            auto f5 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), 1, 1);
            v = f5();
            EXPECT_TRUE(v == 2);

            // Since the object was destroyed, the f5 return value should be the function return type default value
            // (in this case 0, since the return type is int)
            bindWeakTest.reset();
            v = f5();
            EXPECT_TRUE(v == 0);
        }

        // Now the real test starts:
        m_timer.expires_after(std::chrono::milliseconds(100));
        m_timer.async_wait(karabo::util::bind_weak(&Test_Device::executeStepFunction, this, 5, _1));
    }

    ~Test_Device() {
        m_messages->push_back("Test_Device deleted");
        m_timer.cancel();
    }

    void dummyConstFunction(int arg, const boost::system::error_code& error) const {
        // nothing to do
    }

    void executeStepFunction(int arg, const boost::system::error_code& error) {
        // Canceled timer
        if (error == boost::asio::error::operation_aborted) {
            m_messages->push_back("Timer aborted");
            return;
        }
        // Schedule next step
        m_messages->push_back("Tick " + boost::lexical_cast<std::string>(arg));

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        m_timer.expires_after(std::chrono::milliseconds(500));

        m_timer.async_wait(
              karabo::util::bind_weak(&Test_Device::executeStepFunction, this, arg + 1, std::placeholders::_1));
    }
};


struct Test_DeviceServer {
    boost::asio::steady_timer m_deviceDestructTimer;
    std::map<std::string, std::shared_ptr<Test_Device> > m_devices;

    Test_DeviceServer(std::vector<std::string>* messages)
        : m_deviceDestructTimer(karabo::net::EventLoop::getIOService()) {
        m_devices["someTest_Device"] = std::shared_ptr<Test_Device>(new Test_Device(messages));
        m_devices["someTest_Device"]->init();

        m_deviceDestructTimer.expires_after(std::chrono::milliseconds(1500));
        m_deviceDestructTimer.async_wait(
              std::bind(&Test_DeviceServer::killTest_Device, this, std::placeholders::_1, "someTest_Device"));
    }

    void killTest_Device(const boost::system::error_code& error, const std::string& deviceName) {
        m_devices.erase(deviceName);
    }
};


TEST(TestMetaTools, testMethod) {
    EXPECT_TRUE(PointerTest::isSharedPointer<std::shared_ptr<int> >());
    EXPECT_TRUE(!PointerTest::isSharedPointer<int>());

    EXPECT_TRUE((std::is_base_of<Hash, MyPublicHash>::value));
    EXPECT_TRUE((std::is_base_of<Hash, MyProtectedHash>::value));
    EXPECT_TRUE((std::is_base_of<Hash, MyPrivateHash>::value));
    EXPECT_TRUE((!std::is_base_of<Hash, int>::value));
}


TEST(TestMetaTools, testWeakBind) {
    std::vector<std::string> messages;
    std::unique_ptr<Test_DeviceServer> d(new Test_DeviceServer(&messages));
    karabo::net::EventLoop::addThread(4);
    karabo::net::EventLoop::run();

    ASSERT_TRUE(messages.size() >= 4);
    EXPECT_TRUE(messages[0] == "Test_Device created");
    EXPECT_TRUE(messages[1] == "Tick 5");
    EXPECT_TRUE(messages[2] == "Tick 6");
    EXPECT_TRUE(messages[messages.size() - 1] == "Test_Device deleted");
}


struct Base : public std::enable_shared_from_this<Base> {
    virtual ~Base() {}
};


struct VirtualBase : public std::enable_shared_from_this<VirtualBase> {
    virtual void foo() {};
    virtual ~VirtualBase(){};
};


struct FinalInterim : public Base {
    virtual ~FinalInterim(){};
};


struct FinalInterimVirtual : public virtual VirtualBase {
    virtual ~FinalInterimVirtual(){};
};


struct Final : public std::enable_shared_from_this<Final> {};


struct FinalVirtual : public std::enable_shared_from_this<FinalVirtual> {
    virtual void foo() {};
    virtual ~FinalVirtual(){};
};


TEST(TestMetaTools, testCastResolvers) {
    // note that we verify compile-time functionality here. This will simply not compile if the cast resolvers
    // do not treat cases appropriately.
    std::shared_ptr<Final> f(new Final());
    { std::shared_ptr<Final> sf = karabo::util::cond_dyn_cast<std::true_type>::cast(f.get()); }

    std::shared_ptr<FinalVirtual> fv(new FinalVirtual());
    { std::shared_ptr<FinalVirtual> sfv = karabo::util::cond_dyn_cast<std::true_type>::cast(fv.get()); }

    std::shared_ptr<FinalInterim> fi(new FinalInterim());
    { std::shared_ptr<FinalInterim> sfi = karabo::util::cond_dyn_cast<std::false_type>::cast(fi.get()); }

    std::shared_ptr<FinalInterimVirtual> fiv(new FinalInterimVirtual());

    { std::shared_ptr<FinalInterimVirtual> sfiv = karabo::util::cond_dyn_cast<std::false_type>::cast(fiv.get()); }

    EXPECT_TRUE(true);
}


struct Foo {
    Foo() {}

    Foo(const Foo&) {
        nCopies++;
    }

    static int nCopies;
};


int Foo::nCopies = 0;


struct Bar {
    Bar() : gotCalled(false) {}

    void bar(int, const std::string&, const Foo& f) {
        gotCalled = true;
    }

    bool gotCalled;
};


TEST(TestMetaTools, testCallFromTuple) {
    int i(42);
    std::string s("test");
    Foo f;
    Bar b;
    EXPECT_EQ(0, Foo::nCopies);

    Hash h;
    pack(h, i, s, f); // We will copy f once here!!
    EXPECT_EQ(1, Foo::nCopies);
    auto barFn = std::bind(&Bar::bar, &b, _1, _2, _3);
    call(barFn, unpack<int, std::string, Foo>(h)); // But not here!!

    EXPECT_TRUE(b.gotCalled == true);
    EXPECT_EQ(1, Foo::nCopies);

    // But we copy if we go via function with arguments by value
    b.gotCalled = false;
    std::function<void(int, std::string, Foo)> funcWithArgsByValue = barFn;
    call(funcWithArgsByValue, unpack<int, std::string, Foo>(h));
    EXPECT_TRUE(b.gotCalled == true);
    EXPECT_LT(1, Foo::nCopies); // At least one extra copy
}
