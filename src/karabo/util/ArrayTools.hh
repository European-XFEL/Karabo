/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
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


#ifndef KARABO_UTIL_ARRAY_HH
#define KARABO_UTIL_ARRAY_HH

#include <vector>

#include "karabo/data/types/Dims.hh"
#include "karabo/data/types/Hash.hh"

using karabo::data::Dims;
using karabo::data::Hash;

namespace karabo {
    namespace util {

        template <class T>
        inline void addPointerToHash(Hash& hash, const std::string& path, T* const& value, const Dims& dims,
                                     const char separator = Hash::k_defaultSep) {
            hash.set(path, value, separator).setAttribute("dims", dims.toVector());
        }

        template <class T>
        inline void getPointerFromHash(const Hash& hash, const std::string& path, T*& value, Dims& dims,
                                       const char separator = data::Hash::k_defaultSep) {
            const Hash::Node& node = hash.getNode(path, separator);
            value = node.getValue<T*>();
            const std::vector<unsigned long long>& vec = node.getAttribute<std::vector<unsigned long long> >("dims");
            dims.fromVector(vec);
        }

        inline void setDims(Hash& hash, const std::string& path, const Dims& dims,
                            const char separator = Hash::k_defaultSep) {
            hash.setAttribute(path, "dims", dims.toVector(), separator);
        }

    } // namespace util
} // namespace karabo


#endif
