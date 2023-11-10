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

#include <karabo/core/Device.hh>
#include <karabo/core/DeviceServer.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/TextSerializer.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Hash.hh>
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

static std::map<std::string, boost::shared_ptr<karabo::core::DeviceServer>> testServersRegistry;

void exportPyKarabindTestUtilities(py::module_& m) {
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
    m.def("cppNDArrayCopy", []() {
        const Dims shape(3, 4);
        std::vector<int> someData(3 * 4, 7);
        for (int i = 0; i < 3; ++i) someData[i] = 100 + i;
        NDArray nda(someData.begin(), someData.end(), shape);
        return karabind::wrapper::copyNDArrayToPy(nda);
    });
    m.def("getTextSerializerHashClass", []() {
        using namespace karabo::util;
        std::vector<std::string> v = Configurator<karabo::io::TextSerializer<Hash>>::getRegisteredClasses();
        return py::cast(v);
    });
    m.def("getTextSerializerSchemaClass", []() {
        using namespace karabo::util;
        std::vector<std::string> v = Configurator<karabo::io::TextSerializer<Schema>>::getRegisteredClasses();
        return py::cast(v);
    });
    m.def("getBinarySerializerHashClass", []() {
        using namespace karabo::util;
        std::vector<std::string> v = Configurator<karabo::io::BinarySerializer<Hash>>::getRegisteredClasses();
        return py::cast(v);
    });
    m.def("getBinarySerializerSchemaClass", []() {
        using namespace karabo::util;
        std::vector<std::string> v = Configurator<karabo::io::BinarySerializer<Schema>>::getRegisteredClasses();
        return py::cast(v);
    });
    m.def("startDeviceServer", [](karabo::util::Hash& config) {
        using namespace karabo::core;
        if (!config.has("serverId")) config.set("serverId", "testDeviceServer");
        if (!config.has("Logger.priority")) config.set("Logger.priority", "FATAL");
        if (!config.has("scanPlugins")) config.set("scanPlugins", false);
        auto deviceServer = DeviceServer::create("DeviceServer", config);
        deviceServer->finalizeInternalInitialization();
        testServersRegistry[config.get<std::string>("serverId")] = deviceServer;
    });
    m.def("stopDeviceServer", [](const std::string& serverId) {
        using namespace karabo::core;
        auto it = testServersRegistry.find(serverId);
        if (it == testServersRegistry.end()) return;
        testServersRegistry.erase(it);
    });
}

KARABO_REGISTER_FOR_CONFIGURATION(karabo::core::DeviceServer)
