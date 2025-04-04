/*
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
/*
 * File:   ToSize.hh
 * Author: heisenb
 *
 * Created on August 22, 2016, 3:56 PM
 */

#include "ToType.hh"

#ifndef KARABO_DATA_TYPES_TOSIZE_HH
#define KARABO_DATA_TYPES_TOSIZE_HH

namespace karabo {

    namespace data {

        class ToSize {
           public:
            typedef size_t ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, BOOL, sizeof(bool))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, CHAR, sizeof(char))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, INT8, sizeof(int8_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, UINT8, sizeof(uint8_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, INT16, sizeof(int16_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, UINT16, sizeof(uint16_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, INT32, sizeof(int32_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, UINT32, sizeof(uint32_t))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, INT64, sizeof(long long))           // TODO: To be replaced with int64_t
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, UINT64, sizeof(unsigned long long)) // TODO: To be replaced with uint64_t
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, FLOAT, sizeof(float))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, DOUBLE, sizeof(double))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, COMPLEX_FLOAT, sizeof(std::complex<float>))
        KARABO_MAP_TO_REFERENCE_TYPE(ToSize, COMPLEX_DOUBLE, sizeof(std::complex<double>))
    } // namespace data
} // namespace karabo

#endif /* TOSIZE_HH */
