/*
 * $Id$
 *
 * File:   Input.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 14, 2012, 1:22 PM
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_DATA_IO_INPUT_HH
#define KARABO_DATA_IO_INPUT_HH

#include "AbstractInput.hh"

namespace karabo {
    namespace data {

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

            Input(const karabo::data::Hash& config) : AbstractInput(config) {}

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
    } // namespace data
} // namespace karabo


#endif /* KARABO_PACKAGENAME_INPUT_HH */
