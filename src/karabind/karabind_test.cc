/*
 * Author: CONTROLS DEV group
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

#include <pybind11/pybind11.h>

#include <karabo/util/NDArray.hh>
#include <karabo/util/Schema.hh>

#include "ConfigurationTestClasses.hh"
#include "Wrapper.hh"


namespace py = pybind11;

// util
// void exportPyUtilSchemaTest(py::module_ &);
//
//
// Build one big module, 'karabind.so', similar to how we build 'karathon' module
//
// PYBIND11_MODULE(karabind_test, m) {
//     util
//     exportPyUtilSchemaTest(m);
//
//     io
//
//     xms
//
//     core
//
//     log
//
//     net
// }

using namespace karabo::util;
using namespace configurationTest;

void exportPyUtilSchemaTest(py::module_& m) {
    m.def("cppShapeSchemaCircle", []() {
        auto schema = Configurator<Shape>::getSchema("Circle");
        return py::cast(schema);
    });

    m.def("cppShapeSchemaEditableCircle", []() {
        auto schema = Configurator<Shape>::getSchema("EditableCircle");
        return py::cast(schema);
    });

    m.def("cppGraphicsRendererSchemaTest", []() {
        Schema schema("test");
        GraphicsRenderer::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppGraphicsRenderer1SchemaTest", []() {
        Schema schema("test");
        GraphicsRenderer1::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppOtherSchemaElementsSchemaOtherSchemaElements", []() {
        Schema schema("OtherSchemaElements", Schema::AssemblyRules(READ | WRITE | INIT));
        OtherSchemaElements::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppTestStruct1SchemaMyTest", []() {
        Schema schema = Schema("MyTest", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppTestStruct1SchemaTestStruct1", []() {
        Schema schema = Schema("TestStruct1", Schema::AssemblyRules(READ | WRITE | INIT));
        TestStruct1::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppSomeClassSchemaSomeClassId", []() {
        Schema schema = Schema("SomeClassId", Schema::AssemblyRules(READ | WRITE | INIT));
        SomeClass::expectedParameters(schema);
        return py::cast(schema);
    });
    m.def("cppNDArray", []() {
        const Dims shape(3, 4);
        std::vector<int> someData(3 * 4, 7);
        for (int i = 0; i < 3; ++i) someData[i] = 100 + i;
        NDArray nda(someData.begin(), someData.end(), shape);
        return karabind::wrapper::castNDArrayToPy(nda);
    });
}