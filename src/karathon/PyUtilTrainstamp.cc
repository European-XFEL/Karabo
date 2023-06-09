/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
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
#include <karabo/util/Trainstamp.hh>

namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


void exportPyUtilTrainstamp() {
    bp::class_<Trainstamp> t("Trainstamp");
    t.def(bp::init<>());
    t.def(bp::init<const unsigned long long>(bp::arg("trainId")));

    t.def("getTrainId", (unsigned long long const &(Trainstamp::*)() const)(&Trainstamp::getTrainId),
          bp::return_value_policy<bp::copy_const_reference>());

    t.def("hashAttributesContainTimeInformation", &Trainstamp::hashAttributesContainTimeInformation,
          bp::arg("attributes"));
    t.staticmethod("hashAttributesContainTimeInformation");

    t.def("fromHashAttributes", (Trainstamp(*)(Hash::Attributes const))(&Trainstamp::fromHashAttributes),
          bp::arg("attributes"));
    t.staticmethod("fromHashAttributes");

    t.def("toHashAttributes", (void(Trainstamp::*)(Hash::Attributes &) const)(&Trainstamp::toHashAttributes),
          bp::arg("attributes"));
}
