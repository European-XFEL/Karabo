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

#include "PythonFactoryMacros.hh"
#include "Wrapper.hh"
#include "ScopedGILAcquire.hh"

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

        static bool needsDeviceConnection(AbstractInput::Pointer self) {
            return self->needsDeviceConnection();
        }
        static bp::object getConnectedOutputChannels(AbstractInput::Pointer self) {
            return Wrapper::fromStdVectorToPyHashList(self->getConnectedOutputChannels());
        }

        static void connectNow(AbstractInput::Pointer self, const karabo::util::Hash& config) {
            self->connectNow(config);
        }
        
        static bool canCompute(AbstractInput::Pointer self) {
            return self->canCompute();
        }
        
        static void update(AbstractInput::Pointer self) {
            self->update();
        }
        
        static void setEndOfStream(AbstractInput::Pointer self) {
            self->setEndOfStream();
        }
        
        static void registerIOEventHandler(AbstractInput::Pointer self, const bp::object& handler) {
            if (handler == bp::object()) {
                self->m_ioEventHandler = bp::object();
                return;
            }
            if (!Wrapper::hasattr(handler, "func_name")) {
                throw KARABO_PYTHON_EXCEPTION("This python object is not a function.");
            }
            self->m_ioEventHandler = handler;
        }


        void registerEndOfStreamEventHandler(AbstractInput::Pointer self, const bp::object& handler) {
            if (handler == bp::object()) {
                self->m_endOfStreamEventHandler = bp::object();
                return;
            }
            if (!Wrapper::hasattr(handler, "func_name")) {
                throw KARABO_PYTHON_EXCEPTION("This python object is not a function.");
            }
            self->m_endOfStreamEventHandler = handler;
        }


        static void triggerIOEvent(AbstractInput::Pointer self) {
            //this->template triggerIOEvent< karabo::io::Input<T> >();
            if (self->m_ioEventHandler.type() != typeid(bp::object))
                throw KARABO_PYTHON_EXCEPTION("triggerIOEvent(): registered not a python object!");
            ScopedGILAcquire gil;
            try {
                bp::object handler = boost::any_cast<bp::object>(self->m_ioEventHandler);
                if (handler != bp::object())
                    handler(bp::object(self));
            } catch (const bp::error_already_set&) {
                PyErr_Print();
            }
        }


        virtual void triggerEndOfStreamEvent(AbstractInput::Pointer self) {
            //this->triggerEndOfStreamEvent();
            if (self->m_endOfStreamEventHandler.type() != typeid(bp::object))
                throw KARABO_PYTHON_EXCEPTION("triggerEndOfStreamEvent(): registered not a python object!");
            ScopedGILAcquire gil;
            try {
                bp::object handler = boost::any_cast<bp::object>(self->m_endOfStreamEventHandler);
                if (handler != bp::object())
                    handler();
            } catch (const bp::error_already_set&) {
                PyErr_Print();
            }
        }

    };

    struct AbstractOutputWrap {

        static bp::object getInstanceId(AbstractOutput::Pointer self) {
            return bp::object(self->getInstanceId());
        }

        static void setInstanceId(AbstractOutput::Pointer self, const std::string& instanceId) {
            self->setInstanceId(instanceId);
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
            self->signalEndOfStream();
        }
        
        static void registerIOEventHandler(AbstractOutput::Pointer self, const bp::object& handler) {
            if (handler == bp::object()) {
                self->m_ioEventHandler = bp::object();
                return;
            }
            if (!Wrapper::hasattr(handler, "func_name")) {
                throw KARABO_PYTHON_EXCEPTION("This python object is not a function.");
            }
            self->m_ioEventHandler = handler;
        }

        static void triggerIOEvent(AbstractOutput::Pointer self) {
            //this->template triggerIOEvent< karabo::io::Output<T> >();.
            if (self->m_ioEventHandler.type() != typeid(bp::object))
                throw KARABO_PYTHON_EXCEPTION("triggerIOEvent(): registered not a python object!");
            ScopedGILAcquire gil;
            try {
                bp::object handler = boost::any_cast<bp::object>(self->m_ioEventHandler);
                if (handler != bp::object())
                    handler(bp::object(self));
            } catch (const bp::error_already_set&) {
                PyErr_Print();
            }
        }
    };
}


void exportPyIo() {
    {
        bp::class_<AbstractInput, boost::shared_ptr<AbstractInput>, boost::noncopyable>("AbstractInput", bp::init<>())
                .def("reconfigure",   &karathon::AbstractInputWrap::reconfigure, (bp::arg("input")))
                .def("setInstanceId", &karathon::AbstractInputWrap::setInstanceId, (bp::arg("instanceId")))
                .def("getInstanceId", &karathon::AbstractInputWrap::getInstanceId)
                .def("needsDeviceConnection", &karathon::AbstractInputWrap::needsDeviceConnection)
                .def("getConnectedOutputChannels", &karathon::AbstractInputWrap::getConnectedOutputChannels)
                .def("connectNow",  &karathon::AbstractInputWrap::connectNow, (bp::arg("outputChannelInfo")))
                .def("canCompute",  &karathon::AbstractInputWrap::canCompute)
                .def("update",      &karathon::AbstractInputWrap::update)
                .def("setEndOfStream", &karathon::AbstractInputWrap::setEndOfStream)
                .def("registerIOEventHandler", &karathon::AbstractInputWrap::registerIOEventHandler, (bp::arg("handler")))
                .def("registerEndOfStreamEventHandler", &karathon::AbstractInputWrap::registerEndOfStreamEventHandler, (bp::arg("handler")))
                .def("triggerIOEvent", &karathon::AbstractInputWrap::triggerIOEvent)
                .def("triggerEndOfStreamEvent", &karathon::AbstractInputWrap::triggerEndOfStreamEvent)
                KARABO_PYTHON_FACTORY_CONFIGURATOR(AbstractInput)
                ;
    }
    {
        bp::class_<std::map<std::string, boost::shared_ptr<AbstractInput> > >("InputChannels")
                .def(bp::map_indexing_suite<std::map<std::string, boost::shared_ptr<AbstractInput> > >());
    }
    {
        bp::class_<AbstractOutput, boost::shared_ptr<AbstractOutput>, boost::noncopyable>("AbstractOutput", bp::no_init)
                .def("setInstanceId",  &karathon::AbstractOutputWrap::setInstanceId, (bp::arg("instanceId")))
                .def("getInstanceId",  &karathon::AbstractOutputWrap::getInstanceId)
                .def("getInformation", &karathon::AbstractOutputWrap::getInformation)
                .def("canCompute", &karathon::AbstractOutputWrap::canCompute)
                .def("update",     &karathon::AbstractOutputWrap::update)
                .def("signalEndOfStream", &karathon::AbstractOutputWrap::signalEndOfStream)
                .def("registerIOEventHandler", &karathon::AbstractOutputWrap::registerIOEventHandler, (bp::arg("handler")))
                .def("triggerIOEvent", &karathon::AbstractOutputWrap::triggerIOEvent)
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

    bp::def("loadFromFile"
            , (void (*) (Hash &, string const &, Hash const &))(&karabo::io::loadFromFile)
            , (bp::arg("object"), bp::arg("filename"), bp::arg("config") = karabo::util::Hash())
            );
}


template <class T>
void exportPyIoOutput() {

    {//exposing karabo::io::Output<karabo::util::Hash>
        typedef karabo::io::Output<T> SpecificOutput;
        bp::class_<SpecificOutput, boost::noncopyable >(string("Output" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("write"
                     , (void (SpecificOutput::*)(T const &))(&SpecificOutput::write)
                     , (bp::arg("data")))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificOutput)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<SpecificOutput> >();
    }
}
template void exportPyIoOutput<karabo::util::Hash>();
template void exportPyIoOutput<karabo::util::Schema>();


template <class T>
void exportPyIoInput() {

    {//exposing karabo::io::Input<karabo::util::Hash>
        typedef karabo::io::Input<T> SpecificInput;
        bp::class_<SpecificInput, boost::noncopyable >(string("Input" + T::classInfo().getClassName()).c_str(), bp::no_init)
                .def("read"
                     , (void (SpecificInput::*)(T &, size_t))(&SpecificInput::read)
                     , (bp::arg("data"), bp::arg("idx") = 0))
                .def("size"
                     , (size_t(SpecificInput::*)() const) (&SpecificInput::size))
                KARABO_PYTHON_FACTORY_CONFIGURATOR(SpecificInput)
                ;
        bp::register_ptr_to_python< boost::shared_ptr<SpecificInput> >();
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
            PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
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
            PyObject* bytearray = PyByteArray_FromObject(obj.ptr());
            size_t size = PyByteArray_Size(bytearray);
            char* data = PyByteArray_AsString(bytearray);
            T object;
            s.load(object, data, size);
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




