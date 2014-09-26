/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/converter/registry.hpp>
#include <boost/any.hpp>
#include <vector>

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/xip/RawImageData.hh>

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
void exportPyUtilTimestamp();
void exportPyUtilTimeDuration();
void exportPyUtilDims();

// io
void exportPyIo();
void exportPyIoFileTools();
template <class T> void exportPyIoOutput();
template <class T> void exportPyIoInput();
template <class T> void exportPyIoTextSerializer();
template <class T> void exportPyIoBinarySerializer();
void exportPyIoH5File();

// webAuth
void exportPyWebAuthenticator();

// xms
void exportPyXmsRequestor();
void exportPyXmsSignalSlotable();
void exportPyXmsSlotElement();

// core
void exportPyCoreDeviceClient();

// log
void exportPyLogLogger();

// net
void exportp2p();

// xip
//template <class T> void exportPyXipImage();
void exportPyXipStatistics();
template <class T> void exportPyXipCpuImage();
void exportPyXipRawImageData();

void *convert_to_cstring(PyObject *obj)
{
    char *ret = PyString_AsString(obj);
    if (!ret)
        PyErr_Clear();
    return ret;
}

void *convertible_string(PyObject *obj)
{
    if (PyUnicode_Check(obj))
        return const_cast<char *>("");
    else
        return 0;
}


void construct_string(PyObject *obj, boost::python::converter::rvalue_from_python_stage1_data* data)
{
    void* storage = ((boost::python::converter::rvalue_from_python_storage<std::string>*)data)->
	storage.bytes;
    char *str;
    Py_ssize_t size;
    PyString_AsStringAndSize(obj, &str, &size);
    new (storage) std::string(str, size);
    data->convertible = storage;
}


PyThreadState *karathon::ScopedGILRelease::m_threadState;
int karathon::ScopedGILRelease::m_counter = 0;


BOOST_PYTHON_MODULE(karathon) {
    PyEval_InitThreads();
    // init Array C-API
    import_array();

    // util
    exportPyUtilHash();
    exportPyUtilClassInfo();
    exportPyUtilTrainstamp();
    exportPyUtilEpochstamp();
    exportPyUtilTimestamp();
    exportPyUtilTimeDuration();
    exportPyUtilDims();
    exportPyUtilSchema();
    exportPyUtilDateTimeString();
    
    // io
    exportPyIo();
    exportPyIoFileTools();
    
    exportPyIoOutput<karabo::util::Hash>();
    exportPyIoOutput<karabo::util::Schema>();
    exportPyIoOutput<karabo::xip::RawImageData>();
    
    exportPyIoInput<karabo::util::Hash>();
    exportPyIoInput<karabo::util::Schema>();
    exportPyIoInput<karabo::xip::RawImageData>();
    
    exportPyIoTextSerializer<karabo::util::Hash>();
    exportPyIoTextSerializer<karabo::util::Schema>();
    
    exportPyIoBinarySerializer<karabo::util::Hash>();
    exportPyIoBinarySerializer<karabo::util::Schema>();
    exportPyIoBinarySerializer<karabo::xip::RawImageData>();

    exportPyIoH5File();
    
    // webAuth
    exportPyWebAuthenticator();
    
    // xms       
    exportPyXmsRequestor();
    exportPyXmsSignalSlotable();
    exportPyXmsSlotElement();
    
    // core
    exportPyCoreDeviceClient();
    
    // log
    exportPyLogLogger();
    
    // net
    exportp2p();
    
    // xip
    //exportPyXipImage<double>();
    //exportPyXipImage<float>();
    exportPyXipStatistics();
    exportPyXipCpuImage<int>();
    exportPyXipCpuImage<unsigned int>();
    exportPyXipCpuImage<long long>();
    exportPyXipCpuImage<unsigned long long>();
    exportPyXipCpuImage<double>();
    exportPyXipCpuImage<char>();
    exportPyXipCpuImage<unsigned char>();
    exportPyXipCpuImage<float>();
    exportPyXipCpuImage<short>();
    exportPyXipCpuImage<unsigned short>();
    exportPyXipRawImageData();

    boost::python::converter::registry::insert(convert_to_cstring,
	boost::python::type_id<char>(),
	&boost::python::converter::wrap_pytype<&PyUnicode_Type>::get_pytype);
    boost::python::converter::registry::insert(convertible_string,
	construct_string, boost::python::type_id<std::string>(),
	&boost::python::converter::wrap_pytype<&PyUnicode_Type>::get_pytype);
}
