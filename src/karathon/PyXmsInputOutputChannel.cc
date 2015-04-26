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
#include <karabo/xip.hpp>
#include "PythonFactoryMacros.hh"
#include "ScopedGILRelease.hh"
#include "Wrapper.hh"
#include "DimsWrap.hh"

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace bp = boost::python;

namespace karathon {


    struct DataWrap {


        static bp::object getNode(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key) {
            return Wrapper::toObject(self->getNode<karabo::util::Hash::Pointer>(key));
        }


        static bp::object get(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key) {
            return Wrapper::toObject(self->hash()->getNode(key).getValueAsAny());
        }


        static void set(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key, const bp::object& value) {
            boost::any anyval;
            Wrapper::toAny(value, anyval);
            self->set(key, anyval);
        }


        static bp::object hash(const boost::shared_ptr<karabo::xms::Data>& self) {
            return Wrapper::toObject(self->hash());
        }
    };


    struct NDArrayWrap {


//        static bp::object getDataPointer(const boost::shared_ptr<karabo::xms::NDArray>& self) {
//            return Wrapper::toObject(self->getDataPointer());
//        }


        static bp::object getData(const boost::shared_ptr<karabo::xms::NDArray>& self) {
            return Wrapper::fromStdVectorToPyBytes(self->getData());
        }


        static void setData(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& data, const bool copy) {
            boost::any a;
            Wrapper::toAny(data, a);
            if (a.type() == typeid (std::vector<bool>))
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type: VECTOR_BOOL"); //self->setData(boost::any_cast<std::vector<bool> >(a), copy);
            else if (a.type() == typeid (std::vector<char>))
                self->setData(boost::any_cast<std::vector<char> >(a), copy);
            else if (a.type() == typeid (std::vector<signed char>))
                self->setData(boost::any_cast < std::vector<signed char> >(a), copy);
            else if (a.type() == typeid (std::vector<unsigned char>))
                self->setData(boost::any_cast<std::vector<unsigned char> >(a), copy);
            else if (a.type() == typeid (std::vector<short>))
                self->setData(boost::any_cast<std::vector<short> >(a), copy);
            else if (a.type() == typeid (std::vector<unsigned short>))
                self->setData(boost::any_cast<std::vector<unsigned short> >(a), copy);
            else if (a.type() == typeid (std::vector<int>))
                self->setData(boost::any_cast<std::vector<int> >(a), copy);
            else if (a.type() == typeid (std::vector<unsigned int>))
                self->setData(boost::any_cast<std::vector<unsigned int> >(a), copy);
            else if (a.type() == typeid (std::vector<long long>))
                self->setData(boost::any_cast<std::vector<long long> >(a), copy);
            else if (a.type() == typeid (std::vector<unsigned long long>))
                self->setData(boost::any_cast<std::vector<unsigned long long> >(a), copy);
            else if (a.type() == typeid (std::vector<float>))
                self->setData(boost::any_cast<std::vector<float> >(a), copy);
            else if (a.type() == typeid (std::vector<double>))
                self->setData(boost::any_cast<std::vector<double> >(a), copy);
            else if (a.type() == typeid (std::vector<std::string>))
                self->setData(boost::any_cast<std::vector<std::string> >(a), copy);
            else if (a.type() == typeid (std::vector<karabo::util::CppNone>))
                self->setData(boost::any_cast<std::vector<karabo::util::CppNone> >(a), copy);
            else if (a.type() == typeid (std::vector<karabo::xip::RawImageData>))
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type: RawImageData"); //self->setData(boost::any_cast<std::vector<karabo::xip::RawImageData> >(a), copy);
            else if (a.type() == typeid (std::vector<karabo::util::Hash>))
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type: Hash"); //self->setData(boost::any_cast<std::vector<karabo::util::Hash> >(a), copy);
            else if (a.type() == typeid (std::vector<karabo::util::Hash::Pointer>))
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type: Hash::Pointer"); //self->setData(boost::any_cast<std::vector<karabo::util::Hash::Pointer> >(a), copy);
            else if (a.type() == typeid (std::vector<karabo::util::Schema>))
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type: Schema"); //self->setData(boost::any_cast<std::vector<karabo::util::Schema> >(a), copy);
            else
                throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type");
        }


        static bp::object getDimensions(const boost::shared_ptr<karabo::xms::NDArray>& self) {
            karabo::util::Dims dims = self->getDimensions();
            return Wrapper::fromStdVectorToPyList(dims.toVector());
        }


        static void setDimensions(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& obj) {
            if (bp::extract<karathon::DimsWrap>(obj).check()) {
                self->setDimensions(static_cast<karabo::util::Dims> (bp::extract<karathon::DimsWrap>(obj)));
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
        }


        //        static void setDimensionTypes(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& obj) {
        //            if (PyList_Check(obj.ptr())) {
        //                bp::ssize_t size = bp::len(obj);
        //                std::vector<int> dimTypes(size);
        //                for (int i = 0; i < size; i++) {
        //                    dimTypes[i] = bp::extract<int>(obj[i]);
        //                }
        //                try {
        //                    self->setDimensionTypes(dimTypes);
        //                } catch (...) {
        //                    KARABO_RETHROW
        //                }
        //                return;
        //            }
        //            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
        //        }
        //
        //
        //        static bp::object getDimensionScales(const boost::shared_ptr<karabo::xms::NDArray>& self) {
        //            std::vector<std::vector<std::string> > table =  self->getDimensionScales();
        //            bp::list result;
        //            for (size_t i = 0; i < table.size(); i++) {
        //                bp::list row;
        //                for (size_t j = 0; j < table[i].size(); j++)  row.append(bp::str(table[i][j]));
        //                result.append(row);
        //            }
        //            return result;
        //        }
    };


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
using namespace karabo::util;
using namespace karabo::xms;


void exportPyXmsInputOutputChannel() {
    {
        bp::class_<Data, boost::shared_ptr<Data> >("Data", bp::init<>())

                .def(bp::init<const Hash&>())

                .def(bp::init<const Hash::Pointer&>())

                .def(bp::init<const Data&>())

                .def("setNode", &Data::setNode, (bp::arg("key"), bp::arg("data")))

                .def("getNode", &karathon::DataWrap().getNode, (bp::arg("key")))

                .def("get", &karathon::DataWrap().get, (bp::arg("key")))

                .def("set", &karathon::DataWrap().set, (bp::arg("key"), bp::arg("value")))

                .def("hash", &karathon::DataWrap().hash)

                .def(bp::self_ns::str(bp::self))

                .def(bp::self_ns::repr(bp::self))

                KARABO_PYTHON_FACTORY_CONFIGURATOR(Data)
                ;
    }

    {
        bp::class_<NDArray, boost::shared_ptr<NDArray> >("NDArray", bp::init<>())

                .def(bp::init<const Hash&>())

                .def(bp::init<const Hash::Pointer&>())

                //.def("getDataPointer", &karathon::NDArrayWrap().getDataPointer)

                //.def("getByteSize", &NDArray::getByteSize)

                .def("getData", &karathon::NDArrayWrap().getData)

                .def("setData", &karathon::NDArrayWrap().setData, (bp::arg("data"), bp::arg("copy_flag")))

                .def("getDimensions", &karathon::NDArrayWrap().getDimensions)

                .def("setDimensions", &karathon::NDArrayWrap().setDimensions, (bp::arg("dims")))

                //.def("setDimensionTypes", &karathon::NDArrayWrap().setDimensionTypes, (bp::arg("listOfDimTypes")))

                .def("getDataType", &NDArray::getDataType, bp::return_value_policy<bp::copy_const_reference > ())

                .def("setIsBigEndian", &NDArray::setIsBigEndian, (bp::arg("isBigEndian")))

                .def("isBigEndian", &NDArray::isBigEndian)

                //.def("getDimensionScales", &karathon::NDArrayWrap().getDimensionScales)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(NDArray)
                ;
    }

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
