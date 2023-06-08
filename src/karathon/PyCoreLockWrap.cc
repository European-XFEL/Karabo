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
/*
 * File:   LockWrap.cc
 * Author: haufs
 *
 * Created on October 13, 2016, 1:12 PM
 */
#include "PyCoreLockWrap.hh"

#include "boost/python.hpp"


using namespace karabo::core;
namespace bp = boost::python;


namespace karathon {
    void LockWrap::lock(bool recursive) {
        m_lock->lock(recursive);
    }


    void LockWrap::unlock() {
        m_lock->unlock();
    }


    bool LockWrap::valid() {
        return m_lock->valid();
    }
} // namespace karathon


void exportPyCoreLock() {
    bp::class_<karathon::LockWrap, boost::shared_ptr<karathon::LockWrap> >("Lock", bp::no_init)
          .def("lock", &karathon::LockWrap::lock, bp::arg("recursive") = false)
          .def("unlock", &karathon::LockWrap::unlock)
          .def("valid", &karathon::LockWrap::valid);
}
