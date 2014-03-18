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
#include <karabo/io/Output.hh>
#include <karabo/io/Input.hh>
#include <karabo/io/TextSerializer.hh>
#include <karabo/io/BinarySerializer.hh>
#include <karabo/io/FileTools.hh>
#include <boost/pointer_cast.hpp>

#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"
#include "ScopedGILAcquire.hh"
#include "ScopedGILRelease.hh"

using namespace karabo::util;
using namespace karabo::io;
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

        static void setInputHandlerType(AbstractInput::Pointer self, const std::string& type) {
            self->setInputHandlerType(type);
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

        void registerEndOfStreamEventHandler(AbstractInput::Pointer self, const bp::object& handler) {
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


    struct loadFromFileWrap{
        static bp::object loadWrap(const bp::object& fileNameObj, const bp::object& hashObj){
            if (PyString_Check(fileNameObj.ptr())) {
                string fileName = bp::extract<string>(fileNameObj);
                Hash hash = bp::extract<Hash>(hashObj);
                Hash h = Hash();
                {
                    ScopedGILRelease nogil;
                    karabo::io::loadFromFile(h, fileName, hash);
                }
                return bp::object(h);
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python first argument in 'loadFromFile' must be a string, second optional argument is a Hash");
            }
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
    {
        bp::class_<std::map<std::string, boost::shared_ptr<AbstractInput> > >("InputChannels")
                .def(bp::map_indexing_suite<std::map<std::string, boost::shared_ptr<AbstractInput> > >());
    }
    {
        bp::class_<AbstractOutput, boost::shared_ptr<AbstractOutput>, boost::noncopyable>("AbstractOutput", bp::no_init)
                .def("setInstanceId", &karathon::AbstractOutputWrap::setInstanceId, (bp::arg("instanceId")))
                .def("getInstanceId", &karathon::AbstractOutputWrap::getInstanceId)
                .def("setOutputHandlerType", &karathon::AbstractOutputWrap::setOutputHandlerType, (bp::arg("type")))
                .def("getInformation", &karathon::AbstractOutputWrap::getInformation)
                .def("canCompute", &karathon::AbstractOutputWrap::canCompute)
                .def("update", &karathon::AbstractOutputWrap::update)
                .def("signalEndOfStream", &karathon::AbstractOutputWrap::signalEndOfStream)
                .def("registerIOEventHandler", &karathon::AbstractOutputWrap::registerIOEventHandler, (bp::arg("handler")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(AbstractOutput)
                ;
    }
    {
        bp::class_<std::map<std::string, AbstractOutput::Pointer> >("OutputChannels")
                .def(bp::map_indexing_suite<std::map<std::string, AbstractOutput::Pointer> >());
    }
}

void exportPyIoFileTools() {

    bp::def("saveToFile"
            , (void (*) (Schema const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("saveToFile"
            , (void (*) (Hash const &, string const &, Hash const &))(&karabo::io::saveToFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile", &karathon::loadFromFileWrap::loadWrap
            , (bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );

    bp::def("loadFromFile"
            , (void (*) (Hash &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
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
                .def("update"
                     , (void (SpecificOutput::*)()) (&SpecificOutput::update))
                .def("use_count", &boost::shared_ptr<SpecificOutput>::use_count)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificOutput)
                ;
    }
}
template void exportPyIoOutput<karabo::util::Hash>();
template void exportPyIoOutput<karabo::util::Schema>();

template <class T>
void exportPyIoInput() {

    {//exposing karabo::io::Input<karabo::util::Hash>
        typedef karabo::io::Input<T> SpecificInput;
        bp::class_<SpecificInput, boost::shared_ptr<SpecificInput>, boost::noncopyable >(string("Input" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("read"
                     , (void (SpecificInput::*)(T &, size_t))(&SpecificInput::read)
                     , (bp::arg("data"), bp::arg("idx") = 0))
                .def("size"
                     , (size_t(SpecificInput::*)() const) (&SpecificInput::size))
                .def("update"
                     , (void (SpecificInput::*)()) (&SpecificInput::update))
                .def("use_count", &boost::shared_ptr<SpecificInput>::use_count)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificInput)
                ;
    }
}
template void exportPyIoInput<karabo::util::Hash>();
template void exportPyIoInput<karabo::util::Schema>();


template <class T>
class TextSerializerWrap {

public:

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

template <class T>
void exportPyIoTextSerializer() {

    {//exposing karabo::io::TextSerializer<T>, where T :  karabo::util::Hash or karabo::util::Schema
        typedef karabo::io::TextSerializer<T> SpecificSerializer;
        bp::class_<SpecificSerializer, boost::noncopyable >(string("TextSerializer" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("save"
                     , &TextSerializerWrap<T>().save
                     , (bp::arg("object"))
                     , "Saves an object as a string.\nExample:\n\t"
                     "h = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\tarchive = ser.save(h)\n\tassert archive.__class__.__name__ == 'str'")
                .def("load"
                     , &TextSerializerWrap<T>().load
                     , (bp::arg("archive"))
                     , "Loads \"bytearray\" or \"str\" archive and returns a new object.\nExample:\n\th = Hash('a.b.c',1,'x.y.z',[1,2,3,4,5,6,7])\n\tser = TextSerializerHash()\n\t"
                     "archive = ser.save(h)\n\th2 = ser.load(archive)\n\tassert similar(h, h2)")
                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificSerializer)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<SpecificSerializer> >();
    }
}

template void exportPyIoTextSerializer<karabo::util::Hash>();
template void exportPyIoTextSerializer<karabo::util::Schema>();


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

template void exportPyIoBinarySerializer<karabo::util::Hash>();
template void exportPyIoBinarySerializer<karabo::util::Schema>();




