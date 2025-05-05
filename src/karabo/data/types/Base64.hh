/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
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

#ifndef KARABO_DATA_TYPES_BASE64_HH
#define KARABO_DATA_TYPES_BASE64_HH

#include <string>
#include <vector>

namespace karabo {
    namespace data {

        // Base64 Index Table - contains all characters appearing in encoded strings (besides the padding symbol '=')
        extern const std::string b64_char;

        /**
         * Base64 encode bytes
         * @param bytes_to_encode
         * @param len
         * @return
         */
        std::string base64Encode(const unsigned char* bytes_to_encode, const size_t len);

        /**
         * Base64 decode a string
         * @param in
         * @param out vector of bytes
         */
        void base64Decode(const std::string& in, std::vector<unsigned char>& out);

    } // namespace data
} // namespace karabo

#endif // KARABO_DATA_TYPES_BASE64_HH
