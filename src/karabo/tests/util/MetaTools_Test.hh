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

#ifndef METATOOLS_TEST_HH
#define METATOOLS_TEST_HH

#include <cppunit/extensions/HelperMacros.h>

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>
#include <karabo/net/EventLoop.hh>
#include <karabo/util/MetaTools.hh>
#include <memory>
#include <thread>
#include <unordered_map>

#include "karabo/data/types/Hash.hh"

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
            CPPUNIT_ASSERT(v == 2);

            auto f2 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), 1, _1);
            v = f2(1);
            CPPUNIT_ASSERT(v == 2);

            auto f3 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), _1, 1);
            v = f3(1);
            CPPUNIT_ASSERT(v == 2);

            auto f4 = karabo::util::bind_weak(&BindWeakTest::dummyFunction,
                                              const_cast<const BindWeakTest*>(bindWeakTest.get()), _1);
            v = f4(1);
            CPPUNIT_ASSERT(v == 1);

            auto f5 = karabo::util::bind_weak(&BindWeakTest::add, bindWeakTest.get(), 1, 1);
            v = f5();
            CPPUNIT_ASSERT(v == 2);

            // Since the object was destroyed, the f5 return value should be the function return type default value
            // (in this case 0, since the return type is int)
            bindWeakTest.reset();
            v = f5();
            CPPUNIT_ASSERT(v == 0);
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

class MetaTools_Test : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(MetaTools_Test);
    CPPUNIT_TEST(testMethod);
    CPPUNIT_TEST(testWeakBind);
    CPPUNIT_TEST(testCastResolvers);
    CPPUNIT_TEST(testCallFromTuple);
    CPPUNIT_TEST_SUITE_END();

   public:
    MetaTools_Test();
    virtual ~MetaTools_Test();

   private:
    void testMethod();
    void testWeakBind();
    void testCastResolvers();
    void testCallFromTuple();
};

#endif /* METATOOLS_TEST_HH */
