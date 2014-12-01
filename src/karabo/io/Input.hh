/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_INPUT_HH
#define	KARABO_IO_INPUT_HH

#include "AbstractInput.hh"

namespace karabo {
    namespace io {

        template <class T>
        class Input : public AbstractInput {

        public:

            KARABO_CLASSINFO(Input, "Input", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            Input(const karabo::util::Hash& config) : AbstractInput(config) {
            }

            virtual ~Input() {
            }

            virtual void read(T& data, size_t idx = 0) = 0;

            virtual size_t size() = 0;

        };
    }
}



#endif	/* KARABO_PACKAGENAME_INPUT_HH */

