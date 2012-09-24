/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_CORE_DEMODEVICE2_HH
#define	EXFEL_CORE_DEMODEVICE2_HH

#include "StartStopFsm.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package core
     */
    namespace core {

        class DemoDevice2 : public StartStopFsm {
        public:

            EXFEL_CLASSINFO(DemoDevice2, "DemoDevice2", "1.0")

            /**
             * Constructor explicitly calling base Device constructor
             */
            DemoDevice2() : StartStopFsm(this) {
            }

            virtual ~DemoDevice2();

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected Will contain a description of expected parameters for this device
             */
            static void expectedParameters(exfel::util::Schema& expected);

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * upon construction (can be regarded as a second constructor)
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const exfel::util::Hash& input);
            
            void startedStateOnEntry() {
                EXFEL_LOG_WARN << "Woring hard...";
                boost::this_thread::sleep(boost::posix_time::milliseconds(2000));
            }

        };
    }
}

#endif
