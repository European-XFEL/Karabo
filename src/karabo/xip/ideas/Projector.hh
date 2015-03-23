/*
 * $Id$
 *
 * File:   Projector.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 17, 2011, 6:00 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XIP_PROJECTOR_HH

#define	KARABO_XIP_PROJECTOR_HH

#include <karabo/util/Factory.hh>

namespace karabo {
    namespace xip {

        template <class TImage>
        class Projector {

        public:

            KARABO_CLASSINFO(Projector, "Projector", "1.0")
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

            virtual void projectInPlace(TImage& image3d) = 0;

            virtual TImage project(const TImage& image3d) const = 0;

        };

    }
}



#endif	

