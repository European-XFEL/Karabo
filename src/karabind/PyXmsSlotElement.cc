/*
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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include <karabo/xms/SlotElement.hh>

#include "HandlerWrap.hh"
#include "Wrapper.hh"
#include "karabo/data/types/State.hh"

namespace py = pybind11;
using namespace karabo::data;
using namespace karabo::xms;

namespace karabind {

    struct PySlotElementBase : SlotElementBase<SLOT_ELEMENT> {
        // Inherit constructor
        using SlotElementBase<SLOT_ELEMENT>::SlotElementBase;

        // Virtual functions require kind of "trampoline"...
        virtual void commit() {
            PYBIND11_OVERRIDE(void,                          // return type
                              SlotElementBase<SLOT_ELEMENT>, // Parent class
                              commit                         // Name of function in C++
            );
        }

        virtual void beforeAddition() {
            PYBIND11_OVERRIDE_PURE(              // 'beforeAddition' is pure virtual
                  void,                          // return type
                  SlotElementBase<SLOT_ELEMENT>, // parent class
                  beforeAddition                 // name of C++ function
                                                 // arguments: no arguments
            );
        }
    };

} // namespace karabind


using namespace karabind;
using namespace std;


void exportPyXmsSlotElement(py::module_& m) {
    py::class_<SlotElementBase<SLOT_ELEMENT>, PySlotElementBase> sl(m, "SlotElementBase");

    sl.def(py::init<Schema&>(), py::arg("expected"));

    sl.def("allowedStates", [](py::args args) {
        std::vector<karabo::data::State> states;
        SLOT_ELEMENT& self = args[0].cast<SLOT_ELEMENT&>();
        for (unsigned int i = 1; i < py::len(args); ++i) {
            const std::string state = args[i].attr("name").cast<std::string>();
            states.push_back(karabo::data::State::fromString(state));
        }
        self.allowedStates(states);
        return args[0];
    });

    sl.def("commit", &SlotElementBase<SLOT_ELEMENT>::commit, py::return_value_policy::reference_internal);

    sl.def("description", &SlotElementBase<SLOT_ELEMENT>::description, py::arg("desc"),
           py::return_value_policy::reference_internal);

    sl.def("displayedName", &SlotElementBase<SLOT_ELEMENT>::displayedName, py::arg("displayedName"),
           py::return_value_policy::reference_internal);

    sl.def("key", &SlotElementBase<SLOT_ELEMENT>::key, py::arg("name"), py::return_value_policy::reference_internal);

    sl.def("alias", &wrapper::AliasAttributePy<SLOT_ELEMENT>::setAlias, py::return_value_policy::reference_internal);

    sl.def("observerAccess", &SlotElementBase<SLOT_ELEMENT>::observerAccess,
           py::return_value_policy::reference_internal);

    sl.def("operatorAccess", &SlotElementBase<SLOT_ELEMENT>::operatorAccess,
           py::return_value_policy::reference_internal);

    sl.def("expertAccess", &SlotElementBase<SLOT_ELEMENT>::expertAccess, py::return_value_policy::reference_internal);

    { // karabo::xms::SLOT_ELEMENT
        py::class_<SLOT_ELEMENT, SlotElementBase<SLOT_ELEMENT>> elem(m, "SLOT_ELEMENT");

        elem.def(py::init<karabo::data::Schema&>(), py::arg("expected"));

        elem.def("commit", &SLOT_ELEMENT::commit);

        elem.def(
              "tags",
              [](SLOT_ELEMENT& self, const py::object& o, const std::string& sep) {
                  if (py::isinstance<py::str>(o)) {
                      const std::string& stags = o.cast<std::string>();
                      self.tags(fromString<std::string, std::vector>(stags, sep));
                  } else if (py::isinstance<py::sequence>(o)) {
                      std::vector<std::string> vtags;
                      for (auto t : o) {
                          vtags.push_back(t.cast<std::string>());
                      }
                      self.tags(vtags);
                  } else {
                      throw KARABO_NOT_SUPPORTED_EXCEPTION("Valid tag types are 'str' or sequence if str");
                  }
                  return self;
              },
              py::arg("tags"), py::arg("sep") = " ,;", py::return_value_policy::reference_internal);
    }
}
