/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:22 PM
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_INPUT_HH
#define KARABO_IO_INPUT_HH

#include "AbstractInput.hh"

namespace karabo {
    namespace io {

        /**
         * @class Input
         * @brief The Input class provides a datatype T specific base for inputs in
         *        the Karabo system. The not type-specific methods are provided by
         *        the AbstractInput class.
         */
        template <class T>
        class Input : public AbstractInput {
           public:
            KARABO_CLASSINFO(Input, "Input", "1.0")
            KARABO_CONFIGURATION_BASE_CLASS

            Input(const karabo::util::Hash& config) : AbstractInput(config) {}

            virtual ~Input() {}

            /**
             * Read data from the input located at a given index
             * @param data
             * @param idx
             */
            virtual void read(T& data, size_t idx = 0) = 0;

            /**
             * Total size (in terms of type T entries) of the input's data soruce
             * @return
             */
            virtual size_t size() = 0;
        };
    } // namespace io
} // namespace karabo


#endif /* KARABO_PACKAGENAME_INPUT_HH */
