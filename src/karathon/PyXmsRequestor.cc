/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>,
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include "RequestorWrap.hh"

using namespace karabo::xms;
using namespace karabo::util;
using namespace karathon;
using namespace std;
namespace bp = boost::python;


void exportPyXmsRequestor() {

    bp::class_<RequestorWrap > ("Requestor", bp::no_init)
            .def("waitForReply", (&RequestorWrap::waitForReply), (bp::arg("milliseconds")));
}

