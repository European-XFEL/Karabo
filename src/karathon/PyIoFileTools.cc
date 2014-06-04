/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 * Modified (2 May 2013): <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/pointer_cast.hpp>

#include <karabo/io/Output.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/TextSerializer.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/FileTools.hh>
#include <karabo/io/InputElement.hh>
#include <karabo/io/OutputElement.hh>
#include <karabo/xip/RawImageData.hh>

#include "PythonFactoryMacros.hh"
#include "PythonMacros.hh"
#include "Wrapper.hh"
#include "ScopedGILAcquire.hh"
#include "ScopedGILRelease.hh"

using namespace karabo::util;
using namespace karabo::io;
using namespace karabo::xip;
using namespace std;
namespace bp = boost::python;

namespace karathon {

    struct AbstractInputWrap {


        static void reconfigure(AbstractInput::Pointer self, const karabo::util::Hash& configuration) {
            self->reconfigure(configuration);
        }


        static bp::object getInstanceId(AbstractInput::Pointer self) {
            return bp::object(self->getInstanceId());
        }


        static void setInstanceId(AbstractInput::Pointer self, const std::string& instanceId) {
            self->setInstanceId(instanceId);
        }


        static void setInputHandlerType(AbstractInput::Pointer self, const std::string& language, const std::string& inputType) {
            self->setInputHandlerType(language, inputType);
        }


        static bool needsDeviceConnection(AbstractInput::Pointer self) {
            return self->needsDeviceConnection();
        }


        static bp::object getConnectedOutputChannels(AbstractInput::Pointer self) {
            return Wrapper::fromStdVectorToPyHashList(self->getConnectedOutputChannels());
        }


        static void connectNow(AbstractInput::Pointer self, const karabo::util::Hash& config) {
            ScopedGILRelease nogil;
            self->connectNow(config);
        }


        static bool canCompute(AbstractInput::Pointer self) {
            return self->canCompute();
        }


        static void update(AbstractInput::Pointer self) {
            ScopedGILRelease nogil;
            self->update();
        }


        static void setEndOfStream(AbstractInput::Pointer self) {
            ScopedGILRelease nogil;
            self->setEndOfStream();
        }


        static void registerIOEventHandler(AbstractInput::Pointer self, const bp::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void registerEndOfStreamEventHandler(AbstractInput::Pointer self, const bp::object& handler) {
            self->registerEndOfStreamEventHandler(handler);
        }
    };

    struct AbstractOutputWrap {


        static bp::object getInstanceId(AbstractOutput::Pointer self) {
            return bp::object(self->getInstanceId());
        }


        static void setInstanceId(AbstractOutput::Pointer self, const std::string& instanceId) {
            self->setInstanceId(instanceId);
        }


        static void setOutputHandlerType(AbstractOutput::Pointer self, const std::string& type) {
            self->setOutputHandlerType(type);
        }


        static bp::object getInformation(AbstractOutput::Pointer self) {
            return bp::object(self->getInformation());
        }


        static bool canCompute(AbstractOutput::Pointer self) {
            return self->canCompute();
        }


        static void update(AbstractOutput::Pointer self) {
            ScopedGILRelease nogil;
            self->update();
        }


        static void signalEndOfStream(AbstractOutput::Pointer self) {
            ScopedGILRelease nogil;
            self->signalEndOfStream();
        }


        static void registerIOEventHandler(AbstractOutput::Pointer self, const bp::object& handler) {
            self->registerIOEventHandler(handler);
        }
    };

    template <class T>
    struct OutputWrap {

        static void update(const boost::shared_ptr<karabo::io::Output<T> >& self) {
            ScopedGILRelease nogil;
            self->update();
        }
    };


    template <class T>
    struct InputWrap {

        static void registerIOEventHandler(const boost::shared_ptr<karabo::io::Input<T> >& self, const bp::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void registerEndOfStreamEventHandler(const boost::shared_ptr<karabo::io::Input<T> >& self, const bp::object& handler) {
            self->registerEndOfStreamEventHandler(handler);
        }
    };


    template <class T>
    struct loadFromFileWrap {
        static bp::object loadWrap(const bp::object& fileNameObj, const bp::object& conf) {
            if (PyString_Check(fileNameObj.ptr())) {
                string fileName = bp::extract<string>(fileNameObj);
                Hash config = bp::extract<Hash>(conf);
                T t = T();
                {
                    ScopedGILRelease nogil;
                    karabo::io::loadFromFile(t, fileName, config);
                }
                return bp::object(t);
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python first argument in 'loadFromFile' must be a string, second optional argument is a Hash");
            }
        }
    };

    template <class T>
    struct TextSerializerWrap {

        static bp::object save(karabo::io::TextSerializer<T>& s, const T& object) {
            std::string archive;
            s.save(object, archive);
            return bp::object(archive);
        }


        static bp::object load(karabo::io::TextSerializer<T>& s, const bp::object& obj) {
            if (PyByteArray_Check(obj.ptr())) {
                PyObject* bytearray = obj.ptr();
                size_t size = PyByteArray_Size(bytearray);
                char* data = PyByteArray_AsString(bytearray);
                T object;
                s.load(object, data, size);
                return bp::object(object);
            } else if (bp::extract<std::string>(obj).check()) {
                T object;
                s.load(object, bp::extract<std::string>(obj));
                return bp::object(object);
            }
            throw KARABO_PYTHON_EXCEPTION("Python object must be either of type bytearray or string");
        }
    };
}


void exportPyIo() {
    {
        bp::class_<AbstractInput, boost::shared_ptr<AbstractInput>, boost::noncopyable>("AbstractInput", bp::init<>())
                .def("reconfigure", &karathon::AbstractInputWrap::reconfigure, (bp::arg("input")))
                .def("setInstanceId", &karathon::AbstractInputWrap::setInstanceId, (bp::arg("instanceId")))
                .def("getInstanceId", &karathon::AbstractInputWrap::getInstanceId)
                .def("setInputHandlerType", &karathon::AbstractInputWrap::setInputHandlerType, (bp::arg("type")))
                .def("needsDeviceConnection", &karathon::AbstractInputWrap::needsDeviceConnection)
                .def("getConnectedOutputChannels", &karathon::AbstractInputWrap::getConnectedOutputChannels)
                .def("connectNow", &karathon::AbstractInputWrap::connectNow, (bp::arg("outputChannelInfo")))
                .def("canCompute", &karathon::AbstractInputWrap::canCompute)
                .def("update", &karathon::AbstractInputWrap::update)
                .def("setEndOfStream", &karathon::AbstractInputWrap::setEndOfStream)
                .def("registerIOEventHandler", &karathon::AbstractInputWrap::registerIOEventHandler, (bp::arg("handler")))
                .def("registerEndOfStreamEventHandler", &karathon::AbstractInputWrap::registerEndOfStreamEventHandler, (bp::arg("handler")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(AbstractInput)
                ;
    }
}

// TODO: DEPRECATE THIS
void exportPyIoFileTools() {

    bp::def("saveToFile"
            , (void (*) (Schema const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("saveToFile"
            , (void (*) (Hash const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile", &karathon::loadFromFileWrap<Hash>::loadWrap
            , (bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile"
            , (void (*) (Hash &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile"
            , (void (*) (Schema &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );
}

template <class T>
void exportPyIOFileTools1() {
    string className = T::classInfo().getClassName();

    bp::def(string("save" + className + "ToFile").c_str()
            , (void (*) (T const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def(string("load" + className + "FromFile").c_str(), &karathon::loadFromFileWrap<T>::loadWrap
            , (bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

}


template <class T>
void exportPyIoOutput() {

    {//exposing karabo::io::Output<karabo::util::Hash>
        typedef karabo::io::Output<T> SpecificOutput;
        bp::class_<SpecificOutput, boost::shared_ptr<SpecificOutput>, boost::noncopyable >(string("Output" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("write"
                     , (void (SpecificOutput::*)(T const &))(&SpecificOutput::write)
                     , (bp::arg("data")))
                .def("update", &karathon::OutputWrap<T>::update)
                .def("use_count", &boost::shared_ptr<SpecificOutput>::use_count)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificOutput)
                ;
    }
}



template <class T>
void exportPyIoInput() {

    {//exposing karabo::io::Input<karabo::util::Hash>
        typedef karabo::io::Input<T> SpecificInput;
        bp::class_<SpecificInput, boost::shared_ptr<SpecificInput>, boost::noncopyable >(string("Input" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("read"
                     , (void (SpecificInput::*)(T &, size_t))(&SpecificInput::read)
                     , (bp::arg("data"), bp::arg("idx") = 0))
                .def("size", (size_t(SpecificInput::*)() const) (&SpecificInput::size))
                .def("update", (void (SpecificInput::*)()) (&SpecificInput::update))
                .def("registerReadHandler", &karathon::InputWrap<T>::registerIOEventHandler)
                .def("registerEndOfStreamHandler", &karathon::InputWrap<T>::registerEndOfStreamEventHandler)
                .def("use_count", &boost::shared_ptr<SpecificInput>::use_count)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificInput)
                ;
    }
}



template <class T>
void exportPyIoTextSerializer() {

    {//exposing karabo::io::TextSerializer<T>, where T :  karabo::util::Hash or karabo::util::Schema
        typedef karabo::io::TextSerializer<T> SpecificSerializer;
        bp::class_<SpecificSerializer, boost::noncopyable >(string("TextSerializer" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("save"
                     , &karathon::TextSerializerWrap<T>().save
                     , (bp::arg("object"))
                     , "Saves an object as a string.\nExample:\n\t"
                     "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\tarchive = ser.save(h)\n\tassert archive.__class__.__name__ == 'str'")
                .def("load"
                     , &karathon::TextSerializerWrap<T>().load
                     , (bp::arg("archive"))
                     , "Loads \"bytearray\" or \"str\" archive and returns a new object.\nExample:\n\th = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\t"
                     "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert similar(h, h2)")
                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificSerializer)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<SpecificSerializer> >();
    }
}



template <class T>
class BinarySerializerWrap {

public:


    static bp::object save(karabo::io::BinarySerializer<T>& s, const T& object) {
        std::vector<char> v;
        s.save(object, v);
        return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(&v[0], v.size())));
    }


    static bp::object load(karabo::io::BinarySerializer<T>& s, const bp::object& obj) {
        if (PyByteArray_Check(obj.ptr())) {
            PyObject* bytearray = obj.ptr();
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            T object;
            s.load(object, data, size);
            return bp::object(object);
            // TODO: Check whether there is a better way to go from python string to vector<char> or the like...
        } else if (bp::extract<std::string>(obj).check()) {
            T object;
            const string& tmp = bp::extract<std::string>(obj);
            s.load(object, tmp.c_str(), tmp.size() - 1);
            return bp::object(object);
        }
        throw KARABO_PYTHON_EXCEPTION("Python object type is not a bytearray!");
    }
};


template <class T>
void exportPyIoBinarySerializer() {
    bp::docstring_options docs(true, true, false);
    typedef karabo::io::BinarySerializer<T> SpecificSerializer;
    bp::class_<SpecificSerializer, boost::noncopyable >(string("BinarySerializer" + T::classInfo().getClassName()).c_str(), bp::no_init)
            .def("save"
                 , &BinarySerializerWrap<T>().save
                 , (bp::arg("object"))
                 , "Saves an object as a bytearray.\nExample:\n\t"
                 "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = BinarySerializerHash()\n\tarchive = ser.save(h)\n\tassert archive.__class__.__name__ == 'bytearray'")

            .def("load"
                 , &BinarySerializerWrap<T>().load
                 , (bp::arg("archive"))
                 , "Loads \"bytearray\" archive and returns new object.\nExample:\n\th = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = BinarySerializerHash()\n\t"
                 "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert similar(h, h2)")
            KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificSerializer)
            ;
    bp::register_ptr_to_python< boost::shared_ptr<SpecificSerializer> >();
}


// **** EXPLICIT TEMPLATE INSTANTIATIONS ****

template void exportPyIOFileTools1<Hash>();
template void exportPyIOFileTools1<Schema>();
template void exportPyIOFileTools1<RawImageData>();

template void exportPyIoOutput<Hash>();
template void exportPyIoOutput<Schema>();
template void exportPyIoOutput<RawImageData>();

template void exportPyIoInput<Hash>();
template void exportPyIoInput<Schema>();
template void exportPyIoInput<RawImageData>();

template void exportPyIoBinarySerializer<Hash>();
template void exportPyIoBinarySerializer<Schema>();
template void exportPyIoBinarySerializer<RawImageData>();

template void exportPyIoTextSerializer<Hash>();
template void exportPyIoTextSerializer<Schema>();
