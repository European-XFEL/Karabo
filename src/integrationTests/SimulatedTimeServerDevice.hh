/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SimulatedTimeServerDevice.hh
 * Author: steffen.hauf@xfel.eu
 *
 */

#ifndef SIMULATEDTIMESERVERDEVICE_HH
#define SIMULATEDTIMESERVERDEVICE_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class SimulatedTimeServerDevice : public karabo::core::Device<> {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(SimulatedTimeServerDevice, "SimulatedTimeServerDevice", "2.0")

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
        SimulatedTimeServerDevice(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~SimulatedTimeServerDevice();


       private:
        void initialize();
        void tickTock(const boost::system::error_code& e);
        void resetId();

        std::atomic<unsigned long long> m_id;
        unsigned long long m_emitCount;
        boost::asio::deadline_timer m_timeTickerTimer;
        unsigned int m_tickCountdown;
    };
} // namespace karabo

#endif /* SIMULATEDTIMESERVERDEVICE_HH */
