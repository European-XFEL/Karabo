/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/util/RollingWindowStatistics.hh>


namespace bp = boost::python;
using namespace karabo::util;


void exportPyUtilRollingWindowStatistics() {
    bp::class_<RollingWindowStatistics, boost::shared_ptr<RollingWindowStatistics>, boost::noncopyable> d(
          "RollingWindowStatistics", bp::no_init);
    d.def(bp::init<const unsigned int>(bp::args("evaluationInterval")));
    d.def("update", &RollingWindowStatistics::update, bp::arg("value"));
    d.def("getRollingWindowVariance", &RollingWindowStatistics::getRollingWindowVariance);
    d.def("getRollingWindowMean", &RollingWindowStatistics::getRollingWindowMean);
}
