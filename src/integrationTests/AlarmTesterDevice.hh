/*
 * $Id: AlarmTester.hh 7651 2016-06-16 11:46:40Z haufs $
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Created on June, 2016, 03:03 PM
 *
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

#ifndef KARABO_ALARMSYSTEM_HH
#define KARABO_ALARMSYSTEM_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class AlarmTester : public karabo::core::Device {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(AlarmTester, "AlarmTester", "2.0")

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
        AlarmTester(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~AlarmTester();

        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * but BEFORE this reconfiguration request is actually merged into this device's state.
         *
         * The reconfiguration information is contained in the Hash object provided as an argument.
         * You have a chance to change the content of this Hash before it is merged into the device's current state.
         *
         * NOTE: (a) The incomingReconfiguration was validated before
         *       (b) If you do not need to handle the reconfigured data, there is no need to implement this function.
         *           The reconfiguration will automatically be applied to the current state.
         * @param incomingReconfiguration The reconfiguration information as was triggered externally
         */
        virtual void preReconfigure(karabo::util::Hash& incomingReconfiguration);


        /**
         * This function acts as a hook and is called after an reconfiguration request was received,
         * and AFTER this reconfiguration request got merged into this device's current state.
         * You may access any (updated or not) parameters using the usual getters and setters.
         * @code
         * int i = get<int>("myParam");
         * @endcode
         */
        virtual void postReconfigure();

        void initialize();
        void triggerWarnLowAck();
        void triggerWarnHighAck();
        void triggerAlarmLowAck();
        void triggerAlarmHighAck();

        void triggerWarnLowNoAck();
        void triggerWarnHighNoAck();
        void triggerAlarmLowNoAck();
        void triggerAlarmHighNoAck();

        void triggerWarnLowAckNode();
        void triggerWarnHighAckNode();
        void triggerAlarmLowAckNode();
        void triggerAlarmHighAckNode();

        void triggerWarnLowNoAckNode();
        void triggerWarnHighNoAckNode();
        void triggerAlarmLowNoAckNode();
        void triggerAlarmHighNoAckNode();

        void triggerGlobalWarnAck();
        void triggerGlobalAlarmAck();
        void triggerInterlockAck();

        void triggerGlobalWarn();
        void triggerGlobalAlarm();
        void triggerInterlock();

        void triggerNormalAck();
        void triggerNormalNoAck();

        void triggerNormalAckNode();
        void triggerNormalNoAckNode();

        void triggerGlobalNormal();
        void alarmConditionToResult();

       private:
    };
} // namespace karabo

#endif
