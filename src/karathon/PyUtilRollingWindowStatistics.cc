/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
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
