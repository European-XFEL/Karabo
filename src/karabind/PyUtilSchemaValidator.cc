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

#include "PyTypes.hh"
#include "PyUtilSchemaElement.hh"
#include "Wrapper.hh"
#include "karabo/data/schema/Validator.hh"


namespace py = pybind11;
using namespace karabo::data;
using namespace std;
using namespace karabind;


void exportPyUtilSchemaValidator(py::module_& m) {
    ///////////////////////////////////////////////////////////
    //   Validator
    ///////////////////////////////////////////////////////////

    py::class_<Validator::ValidationRules>(m, "ValidatorValidationRules")
          .def(py::init<>())
          .def_readwrite("injectDefaults", &Validator::ValidationRules::injectDefaults)
          .def_readwrite("allowUnrootedConfiguration", &Validator::ValidationRules::allowUnrootedConfiguration)
          .def_readwrite("allowAdditionalKeys", &Validator::ValidationRules::allowAdditionalKeys)
          .def_readwrite("allowMissingKeys", &Validator::ValidationRules::allowMissingKeys)
          .def_readwrite("injectTimestamps", &Validator::ValidationRules::injectTimestamps)
          .def_readwrite("forceInjectedTimestamp", &Validator::ValidationRules::forceInjectedTimestamp)
          .def_readwrite("strict", &Validator::ValidationRules::strict);

    py::class_<Validator>(m, "Validator")
          .def(py::init<>())
          .def(py::init<const Validator::ValidationRules>())
          .def(
                "validate",
                [](Validator& self, const Schema& schema, const Hash& configuration, const py::object& stamp) {
                    Hash::Pointer validated = Hash::Pointer(new Hash);
                    Timestamp tstamp;
                    if (!stamp.is_none() && py::isinstance<karabo::data::Timestamp>(stamp)) {
                        tstamp = stamp.cast<karabo::data::Timestamp>();
                    }

                    pair<bool, string> result = self.validate(schema, configuration, *validated, tstamp);
                    py::tuple t = py::make_tuple(result.first, result.second, py::cast(validated));
                    return t;
                },
                py::arg("schema"), py::arg("configuration"), py::arg("timestamp") = py::none())
          .def("setValidationRules", &Validator::setValidationRules, py::arg("rules"))
          .def("getValidationRules", &Validator::getValidationRules)
          .def("hasReconfigurableParameter", &Validator::hasReconfigurableParameter);
}
