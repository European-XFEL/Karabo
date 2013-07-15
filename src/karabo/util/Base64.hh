/*
 * $Id$
 *
 * Author: <andrea.parenti@xfel.eu>
 *
 * Copyright (c) 2010-2012 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_BASE64_HH
#define KARABO_UTIL_BASE64_HH

#include <string>
#include <vector>

namespace karabo {
    namespace util {

        // Base64 encoder/decoder

        std::string base64Encode(const unsigned char* bytes_to_encode, const size_t len);
        void base64Decode(const std::string& in, std::vector<unsigned char>& out);

    }
}

#endif // KARABO_UTIL_BASE64_HH
