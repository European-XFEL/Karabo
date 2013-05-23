/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */



#ifndef KARABO_XIP_SINGLEPROCESSOR_HH
#define	KARABO_XIP_SINGLEPROCESSOR_HH

#include <karabo/util/Factory.hh>

namespace karabo {
    namespace xip {

        template <class TImage>
        class SingleProcessor {

        public:

            KARABO_CLASSINFO(SingleProcessor, "SingleProcessor", "1.0")
            KARABO_FACTORY_BASE_CLASS

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash& input) {
            }

            virtual void processInPlace(TImage& image) = 0;

            virtual TImage process(const TImage& image) = 0;

        };



    }
}

#endif

