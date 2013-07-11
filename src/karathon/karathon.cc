/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>, <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/any.hpp>
#include <vector>

#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

namespace bp = boost::python;
// util
void exportPyUtilHash();
void exportPyUtilSchema();
void exportPyUtilClassInfo();
void exportPyUtilTrainstamp();
void exportPyUtilEpochstamp();
void exportPyUtilTimestamp();

// io
void exportPyIoFileTools();
template <class T> void exportPyIoOutput();
template <class T> void exportPyIoInput();
template <class T> void exportPyIoTextSerializer();
template <class T> void exportPyIoBinarySerializer();

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
void exportPyXipImageType();
template <class T> void exportPyXipImage();

BOOST_PYTHON_MODULE(libkarathon) {
    
    // util
    exportPyUtilHash();
    exportPyUtilSchema();
    exportPyUtilClassInfo();
    exportPyUtilTrainstamp();
    exportPyUtilEpochstamp();
    exportPyUtilTimestamp();
    
    // io
    exportPyIoFileTools();
    
    exportPyIoOutput<karabo::util::Hash>();
    exportPyIoOutput<karabo::util::Schema>();
    
    exportPyIoInput<karabo::util::Hash>();
    exportPyIoInput<karabo::util::Schema>();
    
    exportPyIoTextSerializer<karabo::util::Hash>();
    exportPyIoTextSerializer<karabo::util::Schema>();
    
    exportPyIoBinarySerializer<karabo::util::Hash>();
    exportPyIoBinarySerializer<karabo::util::Schema>();
    
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
    exportPyXipImageType();
    exportPyXipImage<float>();
    exportPyXipImage<double>();
}
