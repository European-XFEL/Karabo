/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_SINGLEPROCESSOR_HH
#define	KARABO_XIP_SINGLEPROCESSOR_HH

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace xip {

        template <class T>
        class SingleProcessor {

            public:

            KARABO_CLASSINFO(SingleProcessor, "SingleProcessor", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            SingleProcessor(const karabo::util::Hash& input) {
            }

            virtual void processInPlace(T& object) = 0;

            virtual T process(const T& object) = 0;

        };
    }
}

#endif

