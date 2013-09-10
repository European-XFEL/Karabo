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


#ifndef KARABO_IO_OUTPUT_HH
#define	KARABO_IO_OUTPUT_HH

#include "AbstractOutput.hh"
#include <karabo/util/SimpleElement.hh>

namespace karabo {

    namespace io {

        template <class T>
        class Output : public AbstractOutput {

        protected:

            bool m_appendModeEnabled;

        public:

            KARABO_CLASSINFO(Output, "Output", "1.0")

            KARABO_CONFIGURATION_BASE_CLASS

            static void expectedParameters(karabo::util::Schema& expected) {

                using namespace karabo::util;

                BOOL_ELEMENT(expected).key("enableAppendMode")
                        .description("NOTE: Has no effect on Output-Network. If set to true a different internal structure is used, which buffers consecutive "
                                     "calls to write(). The update() function must then be called to trigger final outputting "
                                     "of the accumulated sequence of data.")
                        .displayedName("Enable append mode")
                        .assignmentOptional().defaultValue(false)
                        .init()
                        .commit();
            }

            Output(const karabo::util::Hash& config) : AbstractOutput(config) {
                config.get<bool>("enableAppendMode", m_appendModeEnabled);
            }

            virtual ~Output() {
            }

            virtual void write(const T& object) = 0;
        };
    }
}

#endif	

