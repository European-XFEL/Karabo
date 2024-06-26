/*
 * Author: <krzysztof.wrona@xfel.eu>, <irina.kozlova@xfel.eu>
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

#include <boost/any.hpp>
#include <boost/python.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/type_id.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <vector>

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#include <numpy/arrayobject.h>

#include "ScopedGILRelease.hh"

namespace bp = boost::python;
// util
void exportPyUtilHash();
void exportPyUtilSchema();
void exportPyUtilClassInfo();
void exportPyUtilTrainstamp();
void exportPyUtilDateTimeString();
void exportPyUtilEpochstamp();
void exportPyUtilException();
void exportPyUtilTimestamp();
void exportPyUtilTimeDuration();
void exportPyUtilDims();
void exportPyUtilNDArray();
void exportPyUtilRollingWindowStatistics();
void exportPyUtilStateElement();
void exportPyUtilAlarmConditionElement();
void exportPyUtilJsonToHashParser();

// io
void exportPyIo();
void exportPyIoFileTools();
template <class T>
void exportPyIOFileTools1();
template <class T>
void exportPyIoOutput();
template <class T>
void exportPyIoInput();
template <class T>
void exportPyIoTextSerializer();
template <class T>
void exportPyIoBinarySerializer();

// xms
void exportPyXmsInputOutputChannel();
void exportPyXmsSignalSlotable();
void exportPyXmsSlotElement();

// core
void exportPyCoreDeviceClient();
void exportPyCoreLock();

// log
void exportPyLogLogger();

// net
void exportp2p();

// utilities
void exportPyKarathonTestUtilities();


void* convert_to_cstring(PyObject* obj) {
    const void* ret = PyUnicode_AsUTF8(obj);
    if (!ret) PyErr_Clear();

    return const_cast<void*>(ret);
}


void* convertible_string(PyObject* obj) {
    if (PyUnicode_Check(obj)) return const_cast<char*>("");
    else return 0;
}


void construct_string(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data) {
    void* storage = ((boost::python::converter::rvalue_from_python_storage<std::string>*)data)->storage.bytes;
    Py_ssize_t size;
    const char* str = PyUnicode_AsUTF8AndSize(obj, &size);
    new (storage) std::string(str, size);
    data->convertible = storage;
}


void* import_numpy() {
    // init Array C-API
    import_array();
    return 0;
}


BOOST_PYTHON_MODULE(karathon) {
    PyEval_InitThreads();
    import_numpy();

    // util
    exportPyUtilHash();
    exportPyUtilClassInfo();
    exportPyUtilTrainstamp();
    exportPyUtilEpochstamp();
    exportPyUtilException();
    exportPyUtilTimestamp();
    exportPyUtilTimeDuration();
    exportPyUtilDims();
    exportPyUtilSchema();
    exportPyUtilDateTimeString();
    exportPyUtilRollingWindowStatistics();
    exportPyUtilStateElement();
    exportPyUtilAlarmConditionElement();
    exportPyUtilNDArray();
    exportPyUtilJsonToHashParser();

    // io
    exportPyIo();
    exportPyIoFileTools();
    exportPyIOFileTools1<karabo::util::Hash>();
    exportPyIOFileTools1<karabo::util::Schema>();

    exportPyIoOutput<karabo::util::Hash>();
    exportPyIoOutput<karabo::util::Schema>();

    exportPyIoInput<karabo::util::Hash>();
    exportPyIoInput<karabo::util::Schema>();

    exportPyIoTextSerializer<karabo::util::Hash>();
    exportPyIoTextSerializer<karabo::util::Schema>();

    exportPyIoBinarySerializer<karabo::util::Hash>();
    exportPyIoBinarySerializer<karabo::util::Schema>();


    // xms
    exportPyXmsInputOutputChannel();
    exportPyXmsSignalSlotable();
    exportPyXmsSlotElement();

    // core
    exportPyCoreLock();
    exportPyCoreDeviceClient();

    // log
    exportPyLogLogger();

    // net
    exportp2p();

    // utilities
    exportPyKarathonTestUtilities();

    boost::python::converter::registry::insert(convert_to_cstring, boost::python::type_id<char>(),
                                               &boost::python::converter::wrap_pytype<&PyUnicode_Type>::get_pytype);
    boost::python::converter::registry::insert(convertible_string, construct_string,
                                               boost::python::type_id<std::string>(),
                                               &boost::python::converter::wrap_pytype<&PyUnicode_Type>::get_pytype);
}
