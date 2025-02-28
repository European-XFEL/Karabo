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
 * File:   TimingTestDevice.hh
 * Author: steffen.hauf@xfel.eu
 *
 */

#ifndef TIMINGTESTDEVICE_HH
#define TIMINGTESTDEVICE_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class TimingTestDevice : public karabo::core::Device {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(TimingTestDevice, "TimingTestDevice", "2.0")

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
        TimingTestDevice(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~TimingTestDevice();


       private:
        void start();
        void stop();

        virtual void onTimeUpdate(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                  unsigned long long period);
        virtual void onTimeTick(unsigned long long id, unsigned long long sec, unsigned long long frac,
                                unsigned long long period);

        karabo::util::Epochstamp m_lastTimeStamp;
        bool m_started;
        std::vector<unsigned long long> m_ids;
        std::vector<unsigned long long> m_seconds;
        std::vector<unsigned long long> m_fractions;

        std::vector<unsigned long long> m_idsTick;
        std::vector<unsigned long long> m_secondsTick;
        std::vector<unsigned long long> m_fractionsTick;
    };
} // namespace karabo

#endif /* TIMINGTESTDEVICE_HH */
