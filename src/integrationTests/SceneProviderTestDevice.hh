/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   SceneProviderTestDevice.hh
 * Author: steffen

 */

#ifndef SCENEPROVIDERTESTDEVICE_HH
#define SCENEPROVIDERTESTDEVICE_HH

#include <karabo/karabo.hpp>

/**
 * The main Karabo namespace
 */
namespace karabo {

    class SceneProviderTestDevice : public karabo::core::Device<> {
       public:
        // Add reflection information and Karabo framework compatibility to this class
        KARABO_CLASSINFO(SceneProviderTestDevice, "SceneProviderTestDevice", "2.0")

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
        SceneProviderTestDevice(const karabo::util::Hash& config);

        /**
         * The destructor will be called in case the device gets killed (i.e. the event-loop returns)
         */
        virtual ~SceneProviderTestDevice();


       private:
        void initialize();

        void slotGetScenes(const karabo::util::Hash& args);
    };
} // namespace karabo

#endif /* SCENEPROVIDERTESTDEVICE_HH */
