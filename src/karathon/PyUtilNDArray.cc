/*
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/util.hpp>
#include "Wrapper.hh"

namespace bp = boost::python;
using namespace karathon;

template <class T>
void exportPyUtilNDArray() {
    std::string name("_CppArrayRefHandler_" + karabo::util::Types::convert<karabo::util::FromTypeInfo, karabo::util::ToLiteral>(typeid(T)));
    bp::class_<CppArrayRefHandler<T>, boost::shared_ptr<CppArrayRefHandler<T> > > d(name.c_str(), bp::init<CppArrayRefHandler<T> >());
}

template void exportPyUtilNDArray<char>();
template void exportPyUtilNDArray<unsigned char>();
template void exportPyUtilNDArray<short>();
template void exportPyUtilNDArray<unsigned short>();
template void exportPyUtilNDArray<int>();
template void exportPyUtilNDArray<unsigned int>();
template void exportPyUtilNDArray<long long>();
template void exportPyUtilNDArray<unsigned long long>();
template void exportPyUtilNDArray<float>();
template void exportPyUtilNDArray<double>();
