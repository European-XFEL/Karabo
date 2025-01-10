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

#include <karabo/io/BinaryFileInput.hh>
#include <karabo/io/BinaryFileOutput.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/io/HashBinarySerializer.hh>
#include <karabo/io/HashXmlSerializer.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/Output.hh>
#include <karabo/io/SchemaBinarySerializer.hh>
#include <karabo/io/SchemaXmlSerializer.hh>
#include <karabo/io/TextFileInput.hh>
#include <karabo/io/TextFileOutput.hh>
#include <karabo/io/TextSerializer.hh>

#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace std;
namespace py = pybind11;

namespace karabind {

    template <class T>
    struct OutputWrap {
        static void update(const std::shared_ptr<karabo::io::Output<T>>& self) {
            py::gil_scoped_release release;
            self->update();
        }
    };


    template <class T>
    struct InputWrap {
        static void registerIOEventHandler(const std::shared_ptr<karabo::io::Input<T>>& self,
                                           const py::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void registerEndOfStreamEventHandler(const std::shared_ptr<karabo::io::Input<T>>& self,
                                                    const py::object& handler) {
            self->registerEndOfStreamEventHandler(handler);
        }
    };


    template <class T>
    struct loadFromFileWrap {
        static py::object loadWrap(const std::string& fileName, const karabo::util::Hash& config) {
            T t = T();
            {
                py::gil_scoped_release release;
                karabo::io::loadFromFile(t, fileName, config);
            }
            return py::cast(std::move(t));
        }
    };


    template <class T>
    struct TextSerializerWrap {
        static py::object save(karabo::io::TextSerializer<T>& s, const T& object) {
            std::string archive;
            s.save(object, archive);
            return py::cast(std::move(archive));
        }

        static py::object load(karabo::io::TextSerializer<T>& s, const py::object& obj) {
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
        static py::object save(karabo::io::BinarySerializer<T>& s, const T& object) {
            std::vector<char> v;
            s.save(object, v);
            return py::bytes(v.data(), v.size());
        }

        static py::object load(karabo::io::BinarySerializer<T>& s, const py::object& obj) {
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
          (void (*)(T const&, std::string const&, karabo::util::Hash const&))(&karabo::io::saveToFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::util::Hash());

    m.def(loadFileName.c_str(), &karabind::loadFromFileWrap<T>::loadWrap, py::arg("filename"),
          py::arg("config") = karabo::util::Hash());
}


void exportPyIoFileTools1(py::module_& m) {
    m.def("saveToFile", (void (*)(Schema const&, string const&, Hash const&))(&karabo::io::saveToFile),
          py::arg("object"), py::arg("filename"), py::arg("config") = karabo::util::Hash());

    m.def("saveToFile", (void (*)(Hash const&, string const&, Hash const&))(&karabo::io::saveToFile), py::arg("object"),
          py::arg("filename"), py::arg("config") = karabo::util::Hash());

    m.def("loadFromFile", &karabind::loadFromFileWrap<Hash>::loadWrap, py::arg("filename"),
          py::arg("config") = karabo::util::Hash());

    m.def("loadFromFile", (void (*)(Hash&, string const&, Hash const&))(&karabo::io::loadFromFile), py::arg("object"),
          py::arg("filename"), py::arg("config") = karabo::util::Hash());

    m.def("loadFromFile", (void (*)(Schema&, string const&, Hash const&))(&karabo::io::loadFromFile), py::arg("object"),
          py::arg("filename"), py::arg("config") = karabo::util::Hash());
}


template <class T>
void exportPyIoTextSerializer(py::module_& m) {
    // exposing karabo::io::TextSerializer<T>, where T :  karabo::util::Hash or karabo::util::Schema
    typedef karabo::io::TextSerializer<T> SpecificSerializer;
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
    typedef karabo::io::BinarySerializer<T> SpecificSerializer;
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
    { // exposing karabo::io::Output<karabo::util::Hash>
        typedef karabo::io::Output<T> SpecificOutput;
        py::class_<SpecificOutput, std::shared_ptr<SpecificOutput>>(
              m, string("Output" + T::classInfo().getClassName()).c_str())
              .def("write", (void(SpecificOutput::*)(T const&))(&SpecificOutput::write), py::arg("data"))
              .def("update", &OutputWrap<T>::update) KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificOutput);
    }
    { // exposing karabo::io::Input<karabo::util::Hash>
        typedef karabo::io::Input<T> SpecificInput;
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
    exportPyIoInputOutput<karabo::util::Hash>(m);
    exportPyIoInputOutput<karabo::util::Schema>(m);
    exportPyIoFileTools<karabo::util::Hash>(m);
    exportPyIoFileTools<karabo::util::Schema>(m);
    exportPyIoBinarySerializer<karabo::util::Hash>(m);
    exportPyIoBinarySerializer<karabo::util::Schema>(m);
    exportPyIoTextSerializer<karabo::util::Hash>(m);
    exportPyIoTextSerializer<karabo::util::Schema>(m);
}

// Register all supported IO classes (once per template specialization) ...
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Hash>, karabo::io::BinaryFileOutput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<Schema>, karabo::io::BinaryFileOutput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char>>,
                                  karabo::io::BinaryFileOutput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::util::Hash>,
                                  karabo::io::TextFileOutput<karabo::util::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<karabo::util::Schema>,
                                  karabo::io::TextFileOutput<karabo::util::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Output<std::vector<char>>, karabo::io::TextFileOutput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Hash>, karabo::io::BinaryFileInput<Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<Schema>, karabo::io::BinaryFileInput<Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char>>, karabo::io::BinaryFileInput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::util::Hash>, karabo::io::TextFileInput<karabo::util::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<karabo::util::Schema>,
                                  karabo::io::TextFileInput<karabo::util::Schema>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::Input<std::vector<char>>, karabo::io::TextFileInput<std::vector<char>>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<Hash>, karabo::io::HashBinarySerializer);
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::BinarySerializer<Schema>, karabo::io::SchemaBinarySerializer)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::TextSerializer<karabo::util::Hash>, karabo::io::HashXmlSerializer)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::TextSerializer<karabo::util::Schema>, karabo::io::SchemaXmlSerializer)
