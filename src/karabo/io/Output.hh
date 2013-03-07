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

namespace karabo {

    namespace io {

        template <class T>
        class Output : public AbstractOutput {
        public:

            KARABO_CLASSINFO(Output, "Output", "1.0")
            
            KARABO_CONFIGURATION_BASE_CLASS

            Output(const karabo::util::Hash& config) : AbstractOutput(config) {
            }

            virtual ~Output() {
            }
            
            virtual void write(const T& object) = 0;
        };
    }
}

#endif	

