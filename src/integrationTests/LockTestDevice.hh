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
 * File:   LockTestDevice.hh
 * Author: steffen
 *
 * Created on October 2, 2016, 1:23 PM
 */

#ifndef LOCKTESTDEVICE_HH
#define LOCKTESTDEVICE_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class LockTestDevice : public karabo::core::Device {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(LockTestDevice, "LockTestDevice", "2.0")

        /**
         * Necessary method as part of the factory/configuration system
         * @param expected Will contain a description of expected parameters for this device
         */
        static void expectedParameters(karabo::util::Schema& expected);

        /**
         * Constructor providing the initial configuration in form of a Hash object.
         * If this class is constructed using the configuration system the Hash object will
         * already be validated using the information of the expectedParameters function.
         * The configuration is provided in a key/value fashion.
         */
        LockTestDevice(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~LockTestDevice();


       private:
        void lockAndWait();
        void lockAndWait_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply);

        void lockAndWaitLong();
        void lockAndWaitLong_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply);

        void lockAndWaitTimeout();
        void lockAndWaitTimeout_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply);

        void lockAndWaitRecursive();
        void lockAndWaitRecursive_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply);

        void lockAndWaitRecursiveFail();
        void lockAndWaitRecursiveFail_impl(const karabo::xms::SignalSlotable::AsyncReply& aReply);

        void initialize();
    };
} // namespace karabo

#endif /* LOCKTESTDEVICE_HH */
