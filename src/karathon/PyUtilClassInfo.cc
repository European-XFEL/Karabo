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
#include <karabo/util/ClassInfo.hh>

using namespace karabo::util;
using namespace std;
namespace bp = boost::python;


void exportPyUtilClassInfo() { // exposing karabo::util::ClassInfo
    bp::class_<ClassInfo>("ClassInfo", bp::init<string const&, string const&, string const&>(
                                             (bp::arg("classId"), bp::arg("signature"), bp::arg("classVersion"))))
          .def("getClassId", (string const& (ClassInfo::*)() const)(&ClassInfo::getClassId),
               bp::return_value_policy<bp::copy_const_reference>())
          .def("getClassName", (string const& (ClassInfo::*)() const)(&ClassInfo::getClassName),
               bp::return_value_policy<bp::copy_const_reference>())
          .def("getVersion", (string const& (ClassInfo::*)() const)(&ClassInfo::getVersion),
               bp::return_value_policy<bp::copy_const_reference>())
          .def("getLogCategory", (string const& (ClassInfo::*)() const)(&ClassInfo::getLogCategory),
               bp::return_value_policy<bp::copy_const_reference>())
          .def("getNamespace", (string const& (ClassInfo::*)() const)(&ClassInfo::getNamespace),
               bp::return_value_policy<bp::copy_const_reference>());
}
