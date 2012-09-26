/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_CORE_DEMODEVICE_HH
#define	KARABO_CORE_DEMODEVICE_HH

#include "ReconfigurableFsm.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package core
     */
    namespace core {

        class DemoDevice1 : public ReconfigurableFsm {
        public:

            KARABO_CLASSINFO(DemoDevice1, "DemoDevice1", "1.0")

            /**
             * Constructor explicitly calling base Device constructor
             */
            DemoDevice1() : ReconfigurableFsm(this) {
            }

            virtual ~DemoDevice1();

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

            void onReconfigure(karabo::util::Hash& incomingReconfiguration);
                        
        };
        
        
    }
}

#endif
