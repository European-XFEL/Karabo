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
 * File:   LockTestDevice.cc
 * Author: steffen
 *
 * Created on October 2, 2016, 1:23 PM
 */
#include "LockTestDevice.hh"

#include <chrono>
#include <thread>

#include "karabo/net/EventLoop.hh"
#include "karabo/util/MetaTools.hh"

using namespace std::chrono;
using namespace std::literals::chrono_literals;
using namespace std;

USING_KARABO_NAMESPACES

namespace karabo {


    KARABO_REGISTER_FOR_CONFIGURATION(BaseDevice, Device, LockTestDevice)

    void LockTestDevice::expectedParameters(Schema& expected) {
        STRING_ELEMENT(expected).key("controlledDevice").assignmentOptional().defaultValue("").commit();

        INT32_ELEMENT(expected).key("intProperty").assignmentOptional().defaultValue(0).reconfigurable().commit();

        SLOT_ELEMENT(expected).key("lockAndWait").commit();
    }


    LockTestDevice::LockTestDevice(const karabo::util::Hash& config) : Device(config) {
        KARABO_SLOT(lockAndWait);
        KARABO_SLOT(lockAndWaitLong);
        KARABO_SLOT(lockAndWaitTimeout);
        KARABO_SLOT(lockAndWaitRecursive);
        KARABO_SLOT(lockAndWaitRecursiveFail);
        KARABO_INITIAL_FUNCTION(initialize);
    }


    LockTestDevice::~LockTestDevice() {}


    void LockTestDevice::initialize() {}


    void LockTestDevice::lockAndWait() {
        const AsyncReply reply(this);

        // A slot should never do actions that take a significant amount of time, but just trigger them:
        karabo::net::EventLoop::getIOService().post(util::bind_weak(&LockTestDevice::lockAndWait_impl, this, reply));
    }


    void LockTestDevice::lockAndWait_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply) {
        const std::string deviceId = get<std::string>("controlledDevice");

        try {
            Lock lk = remote().lock(deviceId, false, 0);
            for (int i = 0; i < 5; ++i) {
                if (lk.valid()) {
                    remote().set(deviceId, "intProperty", i);
                    std::this_thread::sleep_for(200ms);
                }
            }
        } catch (const std::exception& e) {
            aReply.error(e.what());
            return;
        }
        aReply();
    }


    void LockTestDevice::lockAndWaitLong() {
        const AsyncReply reply(this);

        // A slot should never do actions that take a significant amount of time, but just trigger them:
        karabo::net::EventLoop::getIOService().post(
              util::bind_weak(&LockTestDevice::lockAndWaitLong_impl, this, reply));
    }


    void LockTestDevice::lockAndWaitLong_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply) {
        const std::string deviceId = get<std::string>("controlledDevice");

        try {
            Lock lk = remote().lock(deviceId, false, 0);
            for (int i = 0; i < 5; ++i) {
                if (lk.valid()) {
                    remote().set(deviceId, "intProperty", i);
                    std::this_thread::sleep_for(5000ms);
                }
            }
        } catch (const std::exception& e) {
            aReply.error(e.what());
            return;
        }
        aReply();
    }


    void LockTestDevice::lockAndWaitTimeout() {
        const AsyncReply reply(this);

        // A slot should never do actions that take a significant amount of time, but just trigger them:
        karabo::net::EventLoop::getIOService().post(
              util::bind_weak(&LockTestDevice::lockAndWaitTimeout_impl, this, reply));
    }


    void LockTestDevice::lockAndWaitTimeout_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply) {
        try {
            Lock lk = remote().lock(get<std::string>("controlledDevice"), false, 1);
        } catch (const std::exception& e) {
            // karabo::util::LockException is likely...
            aReply.error(e.what());
            return;
        }
        aReply();
    }


    void LockTestDevice::lockAndWaitRecursive() {
        const AsyncReply reply(this);

        // A slot should never do actions that take a significant amount of time, but just trigger them:
        karabo::net::EventLoop::getIOService().post(
              util::bind_weak(&LockTestDevice::lockAndWaitRecursive_impl, this, reply));
    }


    void LockTestDevice::lockAndWaitRecursive_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply) {
        const std::string deviceId = get<std::string>("controlledDevice");

        try {
            Lock lk = remote().lock(deviceId, true, 5);
            for (int i = 0; i < 5; ++i) {
                Lock lk = remote().lock(deviceId, true, 0);
                remote().set(deviceId, "intProperty", i);
                std::this_thread::sleep_for(500ms);
            }
        } catch (const std::exception& e) {
            // karabo::util::LockException is likely...
            aReply.error(e.what());
            return;
        }
        aReply();
    }

    void LockTestDevice::lockAndWaitRecursiveFail() {
        const AsyncReply reply(this);

        // A slot should never do actions that take a significant amount of time, but just trigger them:
        karabo::net::EventLoop::getIOService().post(
              util::bind_weak(&LockTestDevice::lockAndWaitRecursiveFail_impl, this, reply));
    }


    void LockTestDevice::lockAndWaitRecursiveFail_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply) {
        const std::string deviceId = get<std::string>("controlledDevice");

        try {
            Lock lk = remote().lock(deviceId, false, 1);
            for (int i = 0; i < 5; ++i) {
                Lock lk = remote().lock(deviceId, false, 0);
                remote().set(deviceId, "intProperty", i);
                std::this_thread::sleep_for(500ms);
            }
        } catch (const std::exception& e) {
            // karabo::util::LockException is likely...
            aReply.error(e.what());
            return;
        }
        aReply();
    }
} // namespace karabo
