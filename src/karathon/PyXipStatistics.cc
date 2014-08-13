/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <boost/python.hpp>

#include <karabo/xip/Statistics.hh>

namespace bp = boost::python;
using namespace karabo::xip;
using namespace std;

void exportPyXipStatistics() {
    //exposing karabo::xip::Statistics

    bp::class_<Statistics> st("Statistics", bp::init<>());
    
    st.def(bp::init< double, double, double, double,
       double, double, double, double, double, double>(( bp::arg("min"), bp::arg("max"), 
            bp::arg("mean"), bp::arg("sdev"), bp::arg("xMin"), bp::arg("yMin"), bp::arg("zMin"), bp::arg("xMax"), bp::arg("yMax"),bp::arg("zMax") )) );
    
    st.def("getMin", &Statistics::getMin);
    st.def("getMax", &Statistics::getMax);
    st.def("getMean", &Statistics::getMean);
    st.def("getVariance", &Statistics::getVariance);
    
    st.def("getXmin", &Statistics::getXmin);
    st.def("getYmin", &Statistics::getYmin);
    st.def("getZmin", &Statistics::getZmin);
    
    st.def("getXmax", &Statistics::getXmax);
    st.def("getYmax", &Statistics::getYmax);
    st.def("getZmax", &Statistics::getZmax);
    
    st.def("printStatistics", &Statistics::print);
}