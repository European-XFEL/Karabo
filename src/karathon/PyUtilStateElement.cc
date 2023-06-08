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

#include <karabo/util/State.hh>
#include <karabo/util/StateElement.hh>

#include "PythonMacros.hh"
#include "boost/python.hpp"
#include "boost/python/raw_function.hpp"


namespace bp = boost::python;
using namespace karabo::util;
using namespace std;


class StateElementWrap {
   public:
    static StateElement &initialValuePy(StateElement &self, const bp::object &value) {
        const std::string className = bp::extract<std::string>(value.attr("__class__").attr("__name__"));
        if (className == "State") {
            const std::string state = bp::extract<std::string>(value.attr("name"));
            return self.initialValue(karabo::util::State::fromString(state));
        } else {
            throw KARABO_PYTHON_EXCEPTION("defaultValue expects parameter of type State.");
        }
    }

    static bp::object optionsPy(bp::tuple args, bp::dict kwargs) {
        StateElement &self = bp::extract<StateElement &>(args[0]);
        std::vector<karabo::util::State> states;
        for (unsigned int i = 1; i < bp::len(args); ++i) {
            const std::string className = bp::extract<std::string>(args[i].attr("__class__").attr("__name__"));
            if (className == "State") {
                const std::string state = bp::extract<std::string>(args[i].attr("name"));
                states.push_back(karabo::util::State::fromString(state));
            } else {
                throw KARABO_PYTHON_EXCEPTION("options() an arbitrary number of arguments of type State.");
            }
        }
        self.options(states);
        return args[0];
    }
};


void exportPyUtilStateElement() {
    bp::implicitly_convertible<Schema &, StateElement>();
    bp::class_<StateElement>("STATE_ELEMENT", bp::init<Schema &>((bp::arg("expected"))))
          .def("alias", &AliasAttributeWrap<StateElement>::aliasPy, bp::return_internal_reference<>())
          .def("commit", &StateElement::commit, bp::return_internal_reference<>())
          .def("commit", (StateElement & (StateElement::*)(karabo::util::Schema &))(&StateElement::commit),
               bp::arg("expected"), bp::return_internal_reference<>())
          .def("description", &StateElement::description, bp::return_internal_reference<>())
          .def("displayedName", &StateElement::displayedName, bp::return_internal_reference<>())
          .def("key", &StateElement::key, bp::return_internal_reference<>())
          .def("tags",
               (StateElement & (StateElement::*)(std::string const &, std::string const &))(&StateElement::tags),
               (bp::arg("tags"), bp::arg("sep") = " ,;"), bp::return_internal_reference<>())
          .def("tags", (StateElement & (StateElement::*)(std::vector<std::string> const &))(&StateElement::tags),
               (bp::arg("tags")), bp::return_internal_reference<>())
          .def("options", bp::raw_function(&StateElementWrap::optionsPy, 2))
          .def("daqPolicy", &StateElement::daqPolicy, bp::return_internal_reference<>())
          .def("defaultValue", &StateElementWrap::initialValuePy, (bp::arg("value")), bp::return_internal_reference<>())
          .def("initialValue", &StateElementWrap::initialValuePy, (bp::arg("value")),
               bp::return_internal_reference<>());
}
