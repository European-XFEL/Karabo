/*
 * Author: <gero.flucke@xfel.eu>
 *
 * Created on March 31, 2025
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
 *
 */

#include "NDArrayElement.hh"

#include "ByteArrayElement.hh"
#include "SimpleElement.hh"
#include "VectorElement.hh"


namespace karabo::util {

    void NDArrayDescription::expectedParameters(karabo::util::Schema& s) {
        BYTEARRAY_ELEMENT(s)
              .key("data")
              .displayedName("Data")
              .description("The data of the array as an untyped buffer of bytes")
              .readOnly()
              .commit();

        VECTOR_UINT64_ELEMENT(s)
              .key("shape")
              .displayedName("Shape")
              .description(
                    "The shape of the array reflects total dimensionality and each element the extension in its "
                    "dimension (0: any extension)")
              .readOnly()
              .commit();

        INT32_ELEMENT(s)
              .key("type")
              .displayedName("Data Type")
              .description("The type of the contained array data")
              .readOnly()
              .initialValue(util::Types::UNKNOWN)
              .commit();

        BOOL_ELEMENT(s)
              .key("isBigEndian")
              .displayedName("Is big-endian")
              .description("A boolean flag which is true if the data is big-endian")
              .readOnly()
              .commit();
    }
} // namespace karabo::util
