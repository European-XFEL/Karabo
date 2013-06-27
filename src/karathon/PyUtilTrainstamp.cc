/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
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
}