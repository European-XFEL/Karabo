/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>,
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include "RequestorWrap.hh"

using namespace exfel::xms;
using namespace exfel::util;
using namespace exfel::pyexfel;
using namespace std;
namespace bp = boost::python;

void exportPyXmsRequestor() {

    bp::class_<RequestorWrap > ("Requestor", bp::no_init)
    .def("waitForReply", (&RequestorWrap::waitForReply), (bp::arg("milliseconds")));
}

