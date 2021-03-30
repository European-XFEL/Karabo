/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karathon;

void exportPyUtilNDArray() {
    bp::class_<CppArrayRefHandler, boost::shared_ptr<CppArrayRefHandler > > d("_CppArrayRefHandler_", bp::init<CppArrayRefHandler >());
}
