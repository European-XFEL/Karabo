/*
 * $Id$
 *
 * Author: <wp76@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_TESTDEVICE_HH
#define	KARABO_CORE_TESTDEVICE_HH

#include "ReconfigurableFsm.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class TestDevice : public ReconfigurableFsm {
        public:

            KARABO_CLASSINFO(TestDevice, "TestDevice", "1.0")

            /**
             * Constructor explicitly calling base Device constructor
             */
            TestDevice() : ReconfigurableFsm(this) {
            }

            virtual ~TestDevice();

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected Will contain a description of expected parameters for this device
             */
            static void expectedParameters(karabo::util::Schema& expected);

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * upon construction (can be regarded as a second constructor)
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash& input);
            
        };
    }
}

#endif
