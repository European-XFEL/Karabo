/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   NonSceneProviderTestDevice.hh
 * Author: steffen.hauf@xfel.eu
 *
 */

#ifndef NONSCENEPROVIDERTESTDEVICE_HH
#define NONSCENEPROVIDERTESTDEVICE_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class NonSceneProviderTestDevice : public karabo::core::Device<> {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(NonSceneProviderTestDevice, "NonSceneProviderTestDevice", "2.0")

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
        NonSceneProviderTestDevice(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~NonSceneProviderTestDevice();


       private:
        void initialize();
    };
} // namespace karabo

#endif /* NONSCENEPROVIDERTESTDEVICE_HH */
