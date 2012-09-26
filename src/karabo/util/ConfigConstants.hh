/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_ACCESSTYPE_HH
#define	KARABO_UTIL_ACCESSTYPE_HH

#include <string>

namespace karabo {
    namespace util {

        enum AccessType {
            INIT = 1 << 0,
            READ = 1 << 1,
            WRITE = 1 << 2,
        };

        inline AccessType operator|(AccessType __a, AccessType __b) {
            return AccessType(static_cast<int> (__a) | static_cast<int> (__b));
        }

        inline AccessType & operator|=(AccessType& __a, AccessType __b) {
            return __a = __a | __b;
        }

        inline AccessType operator&(AccessType __a, AccessType __b) {
            return AccessType(static_cast<int> (__a) & static_cast<int> (__b));
        }

        inline AccessType & operator&=(AccessType& __a, AccessType __b) {
            return __a = __a & __b;
        }
    }
}


#endif	/* KARABO_UTIL_ACCESSTYPE_HH */

