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
#include "PyCoreLockWrap.hh"

#include <pybind11/pybind11.h>

#include "Wrapper.hh"


using namespace karabo::core;
namespace py = pybind11;


namespace karabind {
    void LockWrap::lock(bool recursive) {
        m_lock->lock(recursive);
    }


    void LockWrap::unlock() {
        m_lock->unlock();
    }


    bool LockWrap::valid() {
        return m_lock->valid();
    }
} // namespace karabind


void exportPyCoreLock(py::module_& m) {
    py::class_<karabind::LockWrap, std::shared_ptr<karabind::LockWrap>>(m, "Lock")
          .def("lock", &karabind::LockWrap::lock, py::arg("recursive") = false)
          .def("unlock", &karabind::LockWrap::unlock)
          .def("valid", &karabind::LockWrap::valid);
}
