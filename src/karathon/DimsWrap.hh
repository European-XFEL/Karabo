/*
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

#ifndef KARATHON_DIMSWRAP_HH
#define KARATHON_DIMSWRAP_HH

#include <boost/python.hpp>
#include <karabo/util/Dims.hh>
#include <karathon/Wrapper.hh>


namespace karathon {

    class DimsWrap : public karabo::util::Dims {
       public:
        DimsWrap() : karabo::util::Dims() {}

        DimsWrap(const std::vector<unsigned long long>& vec) : karabo::util::Dims(vec) {}

        DimsWrap(const bp::list& list) : karabo::util::Dims(Wrapper::fromPyListToStdVector<unsigned long long>(list)) {}

        DimsWrap(const bp::tuple& list)
            : karabo::util::Dims(Wrapper::fromPyTupleToStdVector<unsigned long long>(list)) {}

        DimsWrap(unsigned long long xSize) : karabo::util::Dims(xSize) {}

        DimsWrap(unsigned long long xSize, unsigned long long ySize) : karabo::util::Dims(xSize, ySize) {}

        DimsWrap(unsigned long long xSize, unsigned long long ySize, unsigned long long zSize)
            : karabo::util::Dims(xSize, ySize, zSize) {}

        DimsWrap(unsigned long long x1Size, unsigned long long x2Size, unsigned long long x3Size,
                 unsigned long long x4Size)
            : karabo::util::Dims(x1Size, x2Size, x3Size, x4Size) {}

        bp::object toVectorPy() {
            return Wrapper::fromStdVectorToPyList(this->toVector());
        }

        bp::object toArrayPy() {
            return Wrapper::fromStdVectorToPyArray(this->toVector());
        }

       private:
        std::vector<unsigned long long> convertFromListToVector(bp::list& dims) {
            std::vector<unsigned long long> vec(bp::len(dims), 0);
            for (int i = 0; i < bp::len(dims); ++i) {
                vec[i] = bp::extract<unsigned long long>(dims[i]);
            }
            return vec;
        }
    };

} // namespace karathon


#endif /* KARATHON_DIMSWRAP_HH */
