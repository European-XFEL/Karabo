/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_UTIL_ARRAY_HH
#define KARABO_UTIL_ARRAY_HH

#include <vector>

#include "Dims.hh"
#include "Hash.hh"

namespace karabo {
    namespace util {

        template <class T>
        inline void addPointerToHash(Hash& hash, const std::string& path, T* const& value, const Dims& dims,
                                     const char separator = '.') {
            hash.set(path, value, separator).setAttribute("dims", dims.toVector());
        }

        template <class T>
        inline void getPointerFromHash(const Hash& hash, const std::string& path, T*& value, Dims& dims,
                                       const char separator = '.') {
            const Hash::Node& node = hash.getNode(path, separator);
            value = node.getValue<T*>();
            const std::vector<unsigned long long>& vec = node.getAttribute<std::vector<unsigned long long> >("dims");
            dims.fromVector(vec);
        }

        inline void setDims(Hash& hash, const std::string& path, const Dims& dims, const char separator = '.') {
            hash.setAttribute(path, "dims", dims.toVector(), separator);
        }

    } // namespace util
} // namespace karabo


#endif
