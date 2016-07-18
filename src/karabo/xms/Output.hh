/*
 * $Id$
 *
 * File:   Output.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on November 29, 2011, 9:44 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_XMS_OUTPUT_HH
#define	KARABO_XMS_OUTPUT_HH

#include <karabo/util/Factory.hh>

#include "AbstractOutput.hh"

namespace karabo {

    namespace xms {

        template <class T>
        class Output : public AbstractOutput {

            public:

            KARABO_CLASSINFO(Output, "Output", "1.0")

            KARABO_FACTORY_BASE_CLASS

            Output() {
            }

            virtual ~Output() {
            }

            /**
             * Necessary method as part of the factory/configuration system
             * @param expected [out] Description of expected parameters for this object (Schema)
             */
            static void expectedParameters(karabo::util::Schema& expected) {

                AbstractOutput::expectedParameters(expected);
            }

            /**
             * If this object is constructed using the factory/configuration system this method is called
             * @param input Validated (@see expectedParameters) and default-filled configuration
             */
            void configure(const karabo::util::Hash & input) {
            }

            virtual void write(const T& object) = 0;
        };


        typedef karabo::xms::Output<karabo::util::Hash > HashOutput;
        typedef karabo::xms::Output<std::string> FileWrapOutput;
    }
}

#endif	

