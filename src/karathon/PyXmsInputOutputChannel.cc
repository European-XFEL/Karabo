/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>
 * 
 * Created on April 24, 2015, 2:32 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/python.hpp>
#include <karabo/xms.hpp>
#include "PythonFactoryMacros.hh"
#include "ScopedGILRelease.hh"
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {


    struct OutputChannelWrap {


        static void registerIOEventHandler(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void write(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data) {
            boost::any any;
            Wrapper::toAny(data, any);
            if (any.type() == typeid (karabo::util::Hash)) {
                ScopedGILRelease nogil;
                self->write(boost::any_cast<karabo::util::Hash>(any));
            } else if (any.type() == typeid (boost::shared_ptr<karabo::util::Hash>)) {
                ScopedGILRelease nogil;
                self->write(boost::any_cast<boost::shared_ptr<karabo::util::Hash> >(any));
            } else if (any.type() == typeid (karabo::xms::Data)) {
                ScopedGILRelease nogil;
                self->write(boost::any_cast<karabo::xms::Data>(any));
            } else
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type");
        }


        static void update(const boost::shared_ptr<karabo::xms::OutputChannel>& self) {
            ScopedGILRelease nogil;
            self->update();
        }


        static void signalEndOfStream(const boost::shared_ptr<karabo::xms::OutputChannel>& self) {
            ScopedGILRelease nogil;
            self->signalEndOfStream();
        }
    };


    struct InputChannelWrap {


        static void registerIOEventHandler(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
            self->registerIOEventHandler(handler);
        }


        static void registerEndOfStreamEventHandler(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
            self->registerEndOfStreamEventHandler(handler);
        }


        static bp::object getConnectedOutputChannels(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
            return Wrapper::toObject(self->getConnectedOutputChannels());
        }


        static bp::object read(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx) {
            karabo::util::Hash hash;
            {
                ScopedGILRelease nogil;
                self->read(hash, idx);
            }
            return Wrapper::toObject(hash);
        }


        static void connect(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
            ScopedGILRelease nogil;
            self->connect(outputChannelInfo);
        }


        static void disconnect(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
            ScopedGILRelease nogil;
            self->disconnect(outputChannelInfo);
        }


        static void update(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
            ScopedGILRelease nogil;
            self->update();
        }

    };

}


using namespace std;
using namespace karabo::xms;


void exportPyInputOutputChannel() {
    {
        bp::class_<OutputChannel, boost::shared_ptr<OutputChannel>, boost::noncopyable >("OutputChannel", bp::no_init)

                .def("setInstanceId", &OutputChannel::setInstanceId, (bp::arg("instanceId")))

                .def("getInstanceId", &OutputChannel::getInstanceId, bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerIOEventHandler", &karathon::OutputChannelWrap().registerIOEventHandler)

                .def("getInformation", &OutputChannel::getInformation)

                .def("write", &karathon::OutputChannelWrap().write, (bp::arg("data")))

                .def("update", &karathon::OutputChannelWrap().update)

                .def("signalEndOfStream", &karathon::OutputChannelWrap().signalEndOfStream)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(OutputChannel)
                ;

    }
    {
        bp::class_<InputChannel, boost::shared_ptr<InputChannel>, boost::noncopyable >("InputChannel", bp::no_init)

                .def("reconfigure", &InputChannel::reconfigure, (bp::arg("configuration")))

                .def("setInstanceId", &InputChannel::setInstanceId, (bp::arg("instanceId")))

                .def("getInstanceId", &InputChannel::getInstanceId, bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerIOEventHandler", &karathon::InputChannelWrap().registerIOEventHandler)

                .def("registerEndOfStreamEventHandler", &karathon::InputChannelWrap().registerEndOfStreamEventHandler)

                .def("triggerIOEvent", &InputChannel::triggerIOEvent)

                .def("triggerEndOfStreamEvent", &InputChannel::triggerEndOfStreamEvent)

                .def("getConnectedOutputChannels", &karathon::InputChannelWrap().getConnectedOutputChannels)

                .def("read", &karathon::InputChannelWrap().read, (bp::arg("idx")))

                .def("size", &InputChannel::size)

                .def("getMinimumNumberOfData", &InputChannel::getMinimumNumberOfData)

                .def("connect", &karathon::InputChannelWrap().connect, (bp::arg("outputChannelInfo")))

                .def("disconnect", &karathon::InputChannelWrap().disconnect, (bp::arg("outputChannelInfo")))

                .def("canCompute", &InputChannel::canCompute)

                .def("update", &karathon::InputChannelWrap().update)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(OutputChannel)
                ;
    }
}
