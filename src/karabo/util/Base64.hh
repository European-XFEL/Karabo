/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) European XFEL GmbH Schenefeld. All rights reserved.
 */

#ifndef KARABO_UTIL_BASE64_HH
#define KARABO_UTIL_BASE64_HH

#include <string>
#include <vector>

namespace karabo {
    namespace util {

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

    } // namespace util
} // namespace karabo

#endif // KARABO_UTIL_BASE64_HH
