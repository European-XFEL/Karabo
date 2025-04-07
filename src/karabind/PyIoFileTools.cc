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

#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"
#include "karabo/data/io/BinaryFileInput.hh"
#include "karabo/data/io/BinaryFileOutput.hh"
#include "karabo/data/io/BinarySerializer.hh"
#include "karabo/data/io/FileTools.hh"
#include "karabo/data/io/HashBinarySerializer.hh"
#include "karabo/data/io/HashXmlSerializer.hh"
#include "karabo/data/io/Input.hh"
#include "karabo/data/io/Output.hh"
#include "karabo/data/io/SchemaBinarySerializer.hh"
#include "karabo/data/io/SchemaXmlSerializer.hh"
#include "karabo/data/io/TextFileInput.hh"
#include "karabo/data/io/TextFileOutput.hh"
#include "karabo/data/io/TextSerializer.hh"

using namespace karabo::data;
using namespace std;
namespace py = pybind11;

namespace karabind {

    template <class T>
    struct OutputWrap {
        static void update(const std::shared_ptr<karabo::data::Output<T>>& self) {
            py::gil_scoped_release release;
            self->update();
        }
    };


    template <class T>
    struct InputWrap {
        static void registerIOEventHandler(const std::shared_ptr<karabo::data::Input<T>>& self,
                                           const py::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void registerEndOfStreamEventHandler(const std::shared_ptr<karabo::data::Input<T>>& self,
                                                    const py::object& handler) {
            self->registerEndOfStreamEventHandler(handler);
        }
    };


    template <class T>
    struct loadFromFileWrap {
        static py::object loadWrap(const std::string& fileName, const karabo::data::Hash& config) {
            T t = T();
            {
                py::gil_scoped_release release;
                karabo::data::loadFromFile(t, fileName, config);
            }
            return py::cast(std::move(t));
        }
    };


    template <class T>
    struct TextSerializerWrap {
        static py::object save(karabo::data::TextSerializer<T>& s, const T& object) {
            std::string archive;
            s.save(object, archive);
            return py::cast(std::move(archive));
        }

        static py::object load(karabo::data::TextSerializer<T>& s, const py::object& obj) {
            // Support python "bytes", "bytearray" and "str"
            T object;
            if (PyBytes_Check(obj.ptr())) {
                PyObject* bytes = obj.ptr();
                size_t size = PyBytes_Size(bytes);
                char* data = PyBytes_AsString(bytes);
                s.load(object, data, size);
            } else if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                s.load(object, data, size);
            } else if (py::isinstance<py::str>(obj)) {
                s.load(object, obj.cast<std::string>());
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python object must be either of type bytes, bytearray or string");
            }
            return py::cast(std::move(object));
        }
    };


    template <class T>
    class BinarySerializerWrap {
       public:
        static py::object save(karabo::data::BinarySerializer<T>& s, const T& object) {
            std::vector<char> v;
            s.save(object, v);
            return py::bytes(v.data(), v.size());
        }

        static py::object load(karabo::data::BinarySerializer<T>& s, const py::object& obj) {
            T object;
            if (py::isinstance<py::bytes>(obj)) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyBytes_Size(bytearray);
                char* data = PyBytes_AsString(bytearray);
                s.load(object, data, size);
            } else if (py::isinstance<py::bytearray>(obj)) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                s.load(object, data, size);
            } else if (py::isinstance<py::str>(obj)) {
                const std::string& tmp = obj.cast<std::string>();
                s.load(object, tmp.c_str(), tmp.size());
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python object type must be either string, bytes or bytearray!");
            }
            return py::cast(std::move(object));
        }
    };

} // namespace karabind


using namespace karabind;


template <class T>
void exportPyIoFileTools(py::module_& m) {
    std::string saveFuncName = std::string("save") + T::classInfo().getClassName() + "ToFile";
    std::string loadFileName = std::string("load") + T::classInfo().getClassName() + "FromFile";

    m.def(saveFuncName.c_str(),
          (void (*)(T const&, std::string const&, karabo::data::Hash const&))(&karabo::data::saveToFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::data::Hash());

    m.def(loadFileName.c_str(), &karabind::loadFromFileWrap<T>::loadWrap, py::arg("filename"),
          py::arg("config") = karabo::data::Hash());
}


void exportPyIoFileTools1(py::module_& m) {
    m.def("saveToFile", (void (*)(Schema const&, string const&, Hash const&))(&karabo::data::saveToFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::data::Hash());

    m.def("saveToFile", (void (*)(Hash const&, string const&, Hash const&))(&karabo::data::saveToFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::data::Hash());

    m.def("loadFromFile", &karabind::loadFromFileWrap<Hash>::loadWrap, py::arg("filename"),
          py::arg("config") = karabo::data::Hash());

    m.def("loadFromFile", (void (*)(Hash&, string const&, Hash const&))(&karabo::data::loadFromFile), py::arg("object"),
          py::arg("filename"), py::arg("config") = karabo::data::Hash());

    m.def("loadFromFile", (void (*)(Schema&, string const&, Hash const&))(&karabo::data::loadFromFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::data::Hash());
}


template <class T>
void exportPyIoTextSerializer(py::module_& m) {
    // exposing karabo::data::TextSerializer<T>, where T :  karabo::data::Hash or karabo::data::Schema
    typedef karabo::data::TextSerializer<T> SpecificSerializer;
    std::string clsId = "TextSerializer" + T::classInfo().getClassName();

    py::class_<SpecificSerializer, std::shared_ptr<SpecificSerializer>>(m, clsId.c_str())

          KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificSerializer)

                .def("save", &karabind::TextSerializerWrap<T>().save, py::arg("object"),
                     "Saves an object as a string.\nExample:\n\t"
                     "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\tarchive = "
                     "ser.save(h)\n\tassert archive.__class__.__name__ == 'str'")

                .def("load", &karabind::TextSerializerWrap<T>().load, py::arg("archive"),
                     "Loads \"bytearray\" or \"str\" archive and returns a new object.\nExample:\n\th = "
                     "Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\t"
                     "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert fullyEqual(h, h2)")

                .attr("__karabo_cpp_classid__") = clsId;
}


template <class T>
void exportPyIoBinarySerializer(py::module_& m) {
    typedef karabo::data::BinarySerializer<T> SpecificSerializer;
    std::string clsId = "BinarySerializer" + T::classInfo().getClassName();

    py::class_<SpecificSerializer, std::shared_ptr<SpecificSerializer>>(m, clsId.c_str())

          KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificSerializer)

                .def("save", &karabind::BinarySerializerWrap<T>().save, py::arg("object"),
                     "Saves an object as a bytearray.\nExample:\n\t"
                     "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = BinarySerializerHash()\n\tarchive = "
                     "ser.save(h)\n\tassert archive.__class__.__name__ == 'bytearray'")

                .def("load", &karabind::BinarySerializerWrap<T>().load, py::arg("archive"),
                     "Loads \"bytearray\" archive and returns new object.\nExample:\n\th = "
                     "Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = BinarySerializerHash()\n\t"
                     "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert fullyEqual(h, h2)")

                .attr("__karabo_cpp_classid__") = clsId;
}


template <class T>
void exportPyIoInputOutput(py::module_& m) {
    { // exposing karabo::data::Output<karabo::data::Hash>
        typedef karabo::data::Output<T> SpecificOutput;
        py::class_<SpecificOutput, std::shared_ptr<SpecificOutput>>(
              m, string("Output" + T::classInfo().getClassName()).c_str())
              .def("write", (void(SpecificOutput::*)(T const&))(&SpecificOutput::write), py::arg("data"))
              .def("update", &OutputWrap<T>::update) KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificOutput);
    }
    { // exposing karabo::data::Input<karabo::data::Hash>
        typedef karabo::data::Input<T> SpecificInput;
        py::class_<SpecificInput, std::shared_ptr<SpecificInput>>(
              m, string("Input" + T::classInfo().getClassName()).c_str())
              .def("read", (void(SpecificInput::*)(T&, size_t))(&SpecificInput::read), py::arg("data"),
                   py::arg("idx") = 0)
              .def("size", (size_t(SpecificInput::*)() const)(&SpecificInput::size))
              .def("update", (void(SpecificInput::*)())(&SpecificInput::update))
              .def("registerReadHandler", &InputWrap<T>::registerIOEventHandler, py::arg("handler") = py::none())
              .def("registerEndOfStreamHandler", &InputWrap<T>::registerEndOfStreamEventHandler,
                   py::arg("eosHandler") = py::none())

                    KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificInput);
    }
}


void exportPyIoFileToolsAll(py::module_& m) {
    exportPyIoFileTools1(m);
    exportPyIoInputOutput<karabo::data::Hash>(m);
    exportPyIoInputOutput<karabo::data::Schema>(m);
    exportPyIoFileTools<karabo::data::Hash>(m);
    exportPyIoFileTools<karabo::data::Schema>(m);
    exportPyIoBinarySerializer<karabo::data::Hash>(m);
    exportPyIoBinarySerializer<karabo::data::Schema>(m);
    exportPyIoTextSerializer<karabo::data::Hash>(m);
    exportPyIoTextSerializer<karabo::data::Schema>(m);
}

// Register all supported IO classes (once per template specialization) ...
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<Hash>, karabo::data::BinaryFileOutput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<Schema>, karabo::data::BinaryFileOutput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<std::vector<char>>,
                                  karabo::data::BinaryFileOutput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<karabo::data::Hash>,
                                  karabo::data::TextFileOutput<karabo::data::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<karabo::data::Schema>,
                                  karabo::data::TextFileOutput<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Output<std::vector<char>>,
                                  karabo::data::TextFileOutput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<Hash>, karabo::data::BinaryFileInput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<Schema>, karabo::data::BinaryFileInput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<std::vector<char>>,
                                  karabo::data::BinaryFileInput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<karabo::data::Hash>,
                                  karabo::data::TextFileInput<karabo::data::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<karabo::data::Schema>,
                                  karabo::data::TextFileInput<karabo::data::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::Input<std::vector<char>>,
                                  karabo::data::TextFileInput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::BinarySerializer<Hash>, karabo::data::HashBinarySerializer);
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::BinarySerializer<Schema>, karabo::data::SchemaBinarySerializer)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::TextSerializer<karabo::data::Hash>, karabo::data::HashXmlSerializer)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::data::TextSerializer<karabo::data::Schema>, karabo::data::SchemaXmlSerializer)
