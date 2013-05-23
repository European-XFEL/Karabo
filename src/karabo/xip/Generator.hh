/*
 * $Id$
 *
 * File:   Generator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 17, 2011, 6:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_GENERATOR_HH

#define	KARABO_XIP_GENERATOR_HH

#include <karabo/util/Factory.hh>

namespace karabo {
    namespace xip {

        template <class TImage>
        class Generator {

        public:

            KARABO_CLASSINFO(Generator, "Generator", "1.0")
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
            void configure(const karabo::util::Hash & input) {
            }

            virtual void generate(TImage& image) const = 0;

        };

    }
}
#endif	

