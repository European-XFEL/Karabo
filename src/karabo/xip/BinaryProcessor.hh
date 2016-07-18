/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_XIP_BINARYPROCESSOR_HH
#define	KARABO_XIP_BINARYPROCESSOR_HH

#include <karabo/util/Configurator.hh>

namespace karabo {
    namespace xip {

        class BinaryProcessor {

            public:

            KARABO_CLASSINFO(BinaryProcessor, "BinaryProcessor", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            BinaryProcessor(const karabo::util::Hash& input) {
            }

            virtual void processInPlace(const char* data, const size_t nBytes) = 0;

            virtual std::pair<const char*, size_t> process(const char* const data, const size_t nBytes) = 0;

        };
    }
}

#endif

