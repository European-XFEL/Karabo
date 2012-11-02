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

namespace karabo {
  namespace util {

    // Base64 encoder/decoder

    std::string base64_encode(char const* bytes_to_encode, size_t len);
    std::string base64_decode(char const* bytes_to_decode, size_t len);

  }
}

#endif // KARABO_UTIL_BASE64_HH
