/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
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