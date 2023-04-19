/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/util/ArrayTools.hh>

#include "DimsWrap.hh"


namespace bp = boost::python;
using namespace karabo::util;
using namespace karathon;
using namespace std;


void exportPyUtilDims() {
    { bp::class_<Dims> d("_DimsIntern", bp::no_init); }

    {
        bp::class_<DimsWrap, bp::bases<Dims> > d("Dims");

        d.def(bp::init<unsigned long long>());
        d.def(bp::init<unsigned long long, unsigned long long>());
        d.def(bp::init<unsigned long long, unsigned long long, unsigned long long>());
        d.def(bp::init<unsigned long long, unsigned long long, unsigned long long, unsigned long long>());
        d.def(bp::init<bp::list>());
        d.def("rank", (size_t(DimsWrap::*)())(&DimsWrap::rank), "");
        d.def("size", (unsigned long long (DimsWrap::*)())(&DimsWrap::size));
        d.def("extentIn", (unsigned long long (DimsWrap::*)(size_t))(&DimsWrap::extentIn));

        d.def("toList", &DimsWrap::toVectorPy);
        d.def("toArray", &DimsWrap::toArrayPy);
    }

    {
        bp::def("setDims", (void (*)(Hash & hash, const std::string&, const Dims&, const char))(&karabo::util::setDims),
                (bp::arg("hash"), bp::arg("path"), bp::arg("dims"), bp::arg("sep") = '.'));
    }
}
