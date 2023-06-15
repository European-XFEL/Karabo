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

#include <pybind11/pybind11.h>

#include <karabo/util/ArrayTools.hh>
#include <karabo/util/Dims.hh>

#include "Wrapper.hh"


namespace karabind {

    class DimsWrap : public karabo::util::Dims {
       public:
        DimsWrap() : karabo::util::Dims() {}

        DimsWrap(const std::vector<unsigned long long>& vec) : karabo::util::Dims(vec) {}

        DimsWrap(const py::sequence& sequence) : karabo::util::Dims(sequence.cast<std::vector<unsigned long long>>()) {}

        DimsWrap(unsigned long long xSize) : karabo::util::Dims(xSize) {}

        DimsWrap(unsigned long long xSize, unsigned long long ySize) : karabo::util::Dims(xSize, ySize) {}

        DimsWrap(unsigned long long xSize, unsigned long long ySize, unsigned long long zSize)
            : karabo::util::Dims(xSize, ySize, zSize) {}

        DimsWrap(unsigned long long x1Size, unsigned long long x2Size, unsigned long long x3Size,
                 unsigned long long x4Size)
            : karabo::util::Dims(x1Size, x2Size, x3Size, x4Size) {}

        py::object toVectorPy() {
            return py::cast(this->toVector());
        }
    };

} /* namespace karabind */

namespace py = pybind11;
using namespace karabo::util;
using namespace karabind;
using namespace std;


void exportPyUtilDims(py::module_& m) {
    py::class_<Dims>(m, "_DimsIntern");

    // ----- parent ------v
    py::class_<DimsWrap, Dims> d(m, "Dims");

    d.def(py::init<>());

    d.def(py::init<unsigned long long>());

    d.def(py::init<unsigned long long, unsigned long long>());

    d.def(py::init<unsigned long long, unsigned long long, unsigned long long>());

    d.def(py::init<unsigned long long, unsigned long long, unsigned long long, unsigned long long>());

    d.def(py::init<py::sequence>());

    d.def("toList", &DimsWrap::toVectorPy);

    d.def("toArray", &DimsWrap::toVectorPy);

    m.def("setDims", &karabo::util::setDims, py::arg("hash"), py::arg("path"), py::arg("dims"), py::arg("sep") = '.');
}
