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
 * File:   PropertyTest.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on September 5, 2016, 11:08 AM
 */

#ifndef KARABO_DEVICES_PROPERTYTEST_HH
#define KARABO_DEVICES_PROPERTYTEST_HH

#include <boost/asio/deadline_timer.hpp>

#include "karabo/core/Device.hh"
#include "karabo/util/Version.hh"
#include "karabo/xms/InputChannel.hh"

namespace karabo {
    namespace util {
        class Schema;
        class Hash;
    } // namespace util

    namespace devices {


        class NestedClass {
           public:
            KARABO_CLASSINFO(NestedClass, "NestedClass", "1.5")
            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected);

            NestedClass(const karabo::util::Hash& input);

            virtual ~NestedClass();
        };

        /**
         * @class PropertyTest
         * @brief The PropertyTest device includes all types Karabo knows about
         *        in it's expected parameter section. It is a test device to
         *        assure changes to the framework do not result in broken types.
         */
        class PropertyTest : public karabo::core::Device<> {
           public:
            KARABO_CLASSINFO(PropertyTest, "PropertyTest", "karabo-" + karabo::util::Version::getVersion())

            static void expectedParameters(karabo::util::Schema& expected);

            PropertyTest(const karabo::util::Hash& config);

            ~PropertyTest();

           private:
            void initialize();

            void preReconfigure(karabo::util::Hash& incomingReconfiguration);

            void setAlarm();

            void setNoAckAlarm();

            void writeOutput();

            void writeOutputHandler(const boost::system::error_code& e);

            void startWritingOutput();

            void stopWritingOutput();

            void onData(const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta);

            void onEndOfStream(const xms::InputChannel::Pointer& /*unusedInput*/);

            void resetChannelCounters();

            void eosOutput();

            void slotUpdateSchema();

            void node_increment();

            void node_reset();

            void replier(const karabo::xms::SignalSlotable::AsyncReply& areply);

            void slowSlot();

            void logSomething(const karabo::util::Hash& input);


            /**
             * The order test started with this slot works as follows:
             * - 'stringProperty' defines the 'other' PropertyTest device supposed to send messages to us
             * - 'int32Property' defines how many messages it should send
             * - the number of messages and our own id are transferred to the other device
             * - we connect our 'slotCount' to the other's 'signalCount'
             * - we call the other's 'slotStartCount' which will trigger sending messages to us, alternating
             *   between direct calls to our 'slotCount' and emitting 'signalCount' with count arguments starting
             * from 0
             * - we keep track of all counts received and their order (so do not run with billions of counts!)
             * - end of messaging is signaled to us via a call with count = -1
             * - we publish the number of received messages and those counts (well, up to 1000 only) that are not
             *   in order as "orderTest.receivedCounts" and "orderTest.nonConsecutiveCounts", respectively.
             */
            void orderTest_slotStart();

            void startOrderTest();

            void slotStartCount();

            void slotCount(int count);

            bool m_writingOutput;
            boost::asio::steady_timer m_writingOutputTimer;

            std::vector<int> m_counts; // used for message order test
        };
    } // namespace devices
} // namespace karabo

#endif /* KARABO_DEVICES_PROPERTYTEST_HH */
