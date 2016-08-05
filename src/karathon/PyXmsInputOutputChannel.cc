/*
 * $Id$
 *
 * Author: <serguei.essenov@xfel.eu>
 * 
 * Created on April 24, 2015, 2:32 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "PyXmsInputOutputChannel.hh"

namespace bp = boost::python;

namespace karathon {


    boost::shared_ptr<karabo::xms::Data> DataWrap::make(bp::object& obj) {
        if (bp::extract<karabo::util::Hash::Pointer>(obj).check()) {
            return boost::shared_ptr<karabo::xms::Data>(new karabo::xms::Data(bp::extract<karabo::util::Hash::Pointer>(obj)));
        } else if (bp::extract<karabo::xms::Data>(obj).check()) {
            return boost::shared_ptr<karabo::xms::Data>(new karabo::xms::Data(bp::extract<karabo::xms::Data>(obj)));
        } else if (bp::extract<karabo::util::Hash>(obj).check()) {
            return boost::shared_ptr<karabo::xms::Data>(new karabo::xms::Data(bp::extract<karabo::util::Hash>(obj)));
        } else
            throw KARABO_PYTHON_EXCEPTION("Object type is not \"Hash\", \"HashPointer\" or \"Data\"");
    }


    karabo::util::Hash::Pointer DataWrap::getNode(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key) {
        return self->getNode<karabo::util::Hash::Pointer>(key);
    }


    bp::object DataWrap::get(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key) {
        return Wrapper::toObject(self->hash()->getNode(key).getValueAsAny(), true);
    }


    void DataWrap::set(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key, const bp::object& value) {
        boost::any anyval;
        Wrapper::toAny(value, anyval);
        self->set(key, anyval);
    }


    bp::object DataWrap::hash(const boost::shared_ptr<karabo::xms::Data>& self) {
        return Wrapper::toObject(self->hash());
    }


    void DataWrap::attachTimestamp(const boost::shared_ptr<karabo::xms::Data>& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Timestamp>(obj).check()) {
            const karabo::util::Timestamp& ts = bp::extract<karabo::util::Timestamp>(obj);
            self->attachTimestamp(ts);
        }
    }


    boost::shared_ptr<karabo::xms::Data> DataWrap::copy(const boost::shared_ptr<karabo::xms::Data>& self) {
        return boost::shared_ptr<karabo::xms::Data>(new karabo::xms::Data(*self));
    }


    boost::shared_ptr<karabo::xms::ImageData> ImageDataWrap::make5(const bp::object& obj, const bool copy,
                                                                    const karabo::util::Dims& dimensions, const karabo::xms::EncodingType encoding,
                                                                    const int bitsPerPixel) {

        using namespace karabo::util;
        using namespace karabo::xms;

        boost::shared_ptr<ImageData> self(new ImageData());

        if (obj == bp::object())
            return self;

        if (bp::extract<Hash::Pointer>(obj).check()) {
            self = boost::shared_ptr<ImageData>(new ImageData(bp::extract<Hash::Pointer>(obj)));
        } else if (bp::extract<Hash>(obj).check()) {
            self = boost::shared_ptr<ImageData>(new ImageData(bp::extract<Hash>(obj)));
        } else if (PyArray_Check(obj.ptr())) {
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            const NDArray<unsigned char> ndarray = Wrapper::fromPyArrayToNDArray<unsigned char>(arr, NPY_UINT8);
            Dims _dimensions = ndarray.getShape();
            const int rank = _dimensions.rank();

            // Encoding
            EncodingType _encoding = encoding;
            if (encoding == Encoding::UNDEFINED) {
                // No encoding info -> try to guess it from ndarray shape
                if (rank == 2 || (rank == 3 && _dimensions.x3() == 1))
                    _encoding = Encoding::GRAY;
                else if (rank == 3 && _dimensions.x3() == 3)
                    _encoding = Encoding::RGB;
                else if (rank == 3 && _dimensions.x3() == 4)
                    _encoding = Encoding::RGBA;
            }

            // Dimensions (shape)

            if (_encoding == Encoding::RGB || _encoding == Encoding::RGBA ||
                _encoding == Encoding::BGR || _encoding == Encoding::BGRA ||
                _encoding == Encoding::CMYK || _encoding == Encoding::YUV) {
                // Color images -> use ndarray dimensions
                if (rank != 3) {
                    throw KARABO_PYTHON_EXCEPTION("The 'numpy array' has the wrong number of dimensions");
                }
                // Flip X & Y dimensions
                _dimensions = Dims(_dimensions.x2(), _dimensions.x1(), _dimensions.x3());
            } else if (_encoding == Encoding::GRAY) {
                // Gray-scale images -> use ndarray dimensions
                if ((rank != 2) && !((rank == 3) && (_dimensions.x3() == 1))) {
                    throw KARABO_PYTHON_EXCEPTION("The 'numpy array' has the wrong number of dimensions");
                }
                // Flip X & Y dimensions
                if (rank == 2) {
                    _dimensions.reverse();
                }
                else {
                    _dimensions = Dims(_dimensions.x2(), _dimensions.x1(), _dimensions.x3());
                }
            } else if (_encoding == Encoding::JPEG || _encoding == Encoding::PNG ||
                        _encoding == Encoding::BMP || _encoding == Encoding::TIFF) {
                // JPEG, PNG, BMP, TIFF -> cannot use ndarray dimensions, use therefore input parameter
                _dimensions = dimensions;
                if (dimensions.size() == 0) {
                    throw KARABO_PYTHON_EXCEPTION("Dimensions must be supplied for encoded images");
                }
            } else {
                // Other encodings. Likely it will need to be fixed!
            }

            // XXX: Bits per pixel?

            if (copy) {
                // Guarantee a copy is made.
                const boost::shared_ptr<ArrayData<unsigned char> > arrayDataPtr(ndarray.getData());
                self->setData(arrayDataPtr->data(), arrayDataPtr->size(), true);
            }
            else {
                // No copy
                self->setData(ndarray);
            }

            std::vector<unsigned long long> offsets(_dimensions.rank(), 0);
            self->setDimensions(_dimensions);
            self->setROIOffsets(Dims(offsets));

            self->setEncoding(_encoding);
            self->setIsBigEndian(false);
        } else {
            throw KARABO_PARAMETER_EXCEPTION("Object type expected to be ndarray or Hash");
        }
        return self;
    }


    bp::object ImageDataWrap::getDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return Wrapper::fromNDArrayToPyArray(self->getData(), NPY_UINT8);
    }


    void ImageDataWrap::setDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj, const bool copy) {
        if (PyBytes_Check(obj.ptr())) {
            size_t size = PyBytes_Size(obj.ptr());
            char* data = PyBytes_AsString(obj.ptr());
            self->setData(reinterpret_cast<unsigned char*> (data), size, copy);
            return;
        }
        if (PyByteArray_Check(obj.ptr())) {
            size_t size = PyByteArray_Size(obj.ptr());
            char* data = PyByteArray_AsString(obj.ptr());
            self->setData(reinterpret_cast<unsigned char*> (data), size, copy);
            return;
        }
        if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            self->setData(reinterpret_cast<const unsigned char*> (data), size, copy);
            return;
        }
        if (PyArray_Check(obj.ptr())) {
            // No copy IFF array type matches
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            const karabo::util::NDArray<unsigned char> ndarray = Wrapper::fromPyArrayToNDArray<unsigned char>(arr, NPY_UINT8);

            if (copy) {
                // Guarantee a copy is made.
                const boost::shared_ptr<karabo::util::ArrayData<unsigned char> > arrayData(ndarray.getData());
                self->setData(arrayData->data(), arrayData->size(), true);
            }
            else {
                self->setData(ndarray);
            }
            return;
        }
        throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type");
    }


    bp::object ImageDataWrap::getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        karabo::util::Dims dims = self->getDimensions();
        return Wrapper::fromStdVectorToPyList(dims.toVector());
    }


    void ImageDataWrap::setDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setDimensions(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims dims(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setDimensions(dims);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            karabo::util::Dims dims(Wrapper::fromPyTupleToStdVector<unsigned long long>(obj));
            self->setDimensions(dims);
        } else
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
    }

    bp::object ImageDataWrap::getDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return Wrapper::fromStdVectorToPyList(self->getDimensionTypes());
    }

    void ImageDataWrap::setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        if (bp::extract<bp::list>(obj).check()) {
            bp::ssize_t size = bp::len(obj);
            std::vector<int> dimTypes(size);
            for (int i = 0; i < size; i++) {
                dimTypes[i] = bp::extract<int>(obj[i]);
            }
            self->setDimensionTypes(dimTypes);
        } else
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
    }


    bp::object ImageDataWrap::getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        karabo::util::Dims offsets = self->getROIOffsets();
        return Wrapper::fromStdVectorToPyList(offsets.toVector());
    }


    void ImageDataWrap::setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setROIOffsets(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims offsets(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setROIOffsets(offsets);
        } else
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
    }


    bp::object ImageDataWrap::getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return bp::object(karabo::xms::EncodingType(self->getEncoding()));
    }


    void ImageDataWrap::writePy(const boost::shared_ptr<karabo::xms::ImageData>& self, const std::string& filename, const bool enableAppendMode) {
        self->write(filename, enableAppendMode);
    }


    void ImageDataWrap::setGeometryPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& geometry) {
        karabo::util::DetectorGeometry geo = bp::extract<karabo::util::DetectorGeometry>(geometry);
        self->setGeometry(geo);
    }


    karabo::util::DetectorGeometry ImageDataWrap::getGeometryPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return self->getGeometry();
    }


    void ImageDataWrap::setHeaderPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& header){
        karabo::util::Hash head = bp::extract<karabo::util::Hash>(header);
        self->setHeader(head);
    }


    const karabo::util::Hash& ImageDataWrap::getHeaderPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return self->getHeader();
    }


    karabo::xms::ImageDataElement& ImageDataElementWrap::setDefaultValue(const boost::shared_ptr<karabo::xms::ImageDataElement>& self,
                                                                         const std::string& subKey,
                                          const bp::object& defaultValue) {
        boost::any anyValue;
        karathon::Wrapper::toAny(defaultValue, anyValue);
        return self->setDefaultValue(subKey, anyValue);
    }


    void OutputChannelWrap::registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler) {
        self->registerIOEventHandler(boost::bind(OutputChannelWrap::proxyIOEventHandler, handler, _1));
    }


    void OutputChannelWrap::proxyIOEventHandler(const bp::object& handler, const boost::shared_ptr<karabo::xms::OutputChannel>& outputChannel) {
        ScopedGILAcquire gil;
        handler(bp::object(outputChannel));
    }


    void OutputChannelWrap::writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data) {
        if (bp::extract<karabo::xms::ImageData>(data).check()) {
            ScopedGILRelease nogil;
            self->write(bp::extract<karabo::xms::ImageData>(data));
        } else if (bp::extract<karabo::xms::Data>(data).check()) {
            ScopedGILRelease nogil;
            self->write(bp::extract<karabo::xms::Data>(data));
        } else if (bp::extract<karabo::util::Hash::Pointer>(data).check()) {
            ScopedGILRelease nogil;
            self->write(bp::extract<karabo::util::Hash::Pointer>(data));
        } else
            throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type");
    }


    void OutputChannelWrap::updatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self) {
        ScopedGILRelease nogil;
        self->update();
    }


    void OutputChannelWrap::signalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self) {
        ScopedGILRelease nogil;
        self->signalEndOfStream();
    }


    void InputChannelWrap::registerInputHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
        self->registerInputHandler(boost::bind(InputChannelWrap::proxyInputHandler, handler, _1));
    }


    void InputChannelWrap::proxyInputHandler(const bp::object& handler, const boost::shared_ptr<karabo::xms::InputChannel>& input) {
        ScopedGILAcquire gil;
        handler(bp::object(input));
    }


    void InputChannelWrap::registerDataHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
        self->registerDataHandler(boost::bind(InputChannelWrap::proxyDataHandler, handler, _1));
    }


    void InputChannelWrap::proxyDataHandler(const bp::object& handler, const karabo::xms::Data& data) {
        ScopedGILAcquire gil;
        handler(bp::object(data));
    }


    void InputChannelWrap::registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
        self->registerEndOfStreamEventHandler(boost::bind(InputChannelWrap::proxyEndOfStreamEventHandler, handler, _1));
    }


    void InputChannelWrap::proxyEndOfStreamEventHandler(const bp::object& handler, const boost::shared_ptr<karabo::xms::InputChannel>& input) {
        ScopedGILAcquire gil;
        handler(bp::object(input));
    }


    bp::object InputChannelWrap::getConnectedOutputChannelsPy(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        return Wrapper::toObject(self->getConnectedOutputChannels());
    }


    bp::object InputChannelWrap::readPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx) {
        ScopedGILRelease nogil;
        return Wrapper::toObject(self->read(idx));
    }


    void InputChannelWrap::connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
        ScopedGILRelease nogil;
        self->connect(outputChannelInfo);
    }


    void InputChannelWrap::disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
        ScopedGILRelease nogil;
        self->disconnect(outputChannelInfo);
    }


    void InputChannelWrap::updatePy(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        ScopedGILRelease nogil;
        self->update();
    }

}


using namespace std;
using namespace karabo::util;
using namespace karabo::xms;


void exportPyXmsInputOutputChannel() {
    {
        bp::class_<Data, boost::shared_ptr<Data> >("Data", bp::init<>())

                .def(bp::init<const string&, const Hash&>((bp::arg("channel"), bp::arg("config"))))

                .def(bp::init<const string&, const Data&>((bp::arg("key"), bp::arg("other"))))

                .def("__init__", bp::make_constructor(&karathon::DataWrap::make, bp::default_call_policies(), (bp::arg("data"))))

                .def("setNode", &Data::setNode, (bp::arg("key"), bp::arg("data")))

                .def("getNode", &karathon::DataWrap().getNode, (bp::arg("key")))

                .def("get", &karathon::DataWrap().get, (bp::arg("key")))

                .def("__getitem__", &karathon::DataWrap().get)

                .def("set", &karathon::DataWrap().set, (bp::arg("key"), bp::arg("value")))

                .def("__setitem__", &karathon::DataWrap().set)

                .def("has", &Data::has, (bp::arg("key")))

                .def("__contains__", &Data::has, (bp::arg("key")))

                .def("erase", &Data::erase, (bp::arg("key")))

                .def("__delitem__", &Data::erase, (bp::arg("key")))

                .def("hash", &karathon::DataWrap().hash)

                .def("attachTimestamp", &karathon::DataWrap().attachTimestamp, (bp::arg("timestamp")))

                .def(bp::self_ns::str(bp::self))

                .def(bp::self_ns::repr(bp::self))

                .def("__copy__", &karathon::DataWrap().copy)

                .def("__deepcopy__", &karathon::DataWrap().copy)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(Data)
                ;
    }

    {
        bp::enum_< karabo::xms::Encoding::EncodingType>("Encoding")
                .value("UNDEFINED", karabo::xms::Encoding::UNDEFINED)
                .value("GRAY", karabo::xms::Encoding::GRAY)
                .value("RGB", karabo::xms::Encoding::RGB)
                .value("RGBA", karabo::xms::Encoding::RGBA)
                .value("BGR", karabo::xms::Encoding::BGR)
                .value("BGRA", karabo::xms::Encoding::BGRA)
                .value("CMYK", karabo::xms::Encoding::CMYK)
                .value("YUV", karabo::xms::Encoding::YUV)
                .value("BAYER", karabo::xms::Encoding::BAYER)
                .value("JPEG", karabo::xms::Encoding::JPEG)
                .value("PNG", karabo::xms::Encoding::PNG)
                .value("BMP", karabo::xms::Encoding::BMP)
                .value("TIFF", karabo::xms::Encoding::TIFF)
                .export_values()
                ;

        bp::class_<ImageData, boost::shared_ptr<ImageData>, bp::bases<Data> >("ImageData", bp::init<>())

                .def("__init__", bp::make_constructor(&karathon::ImageDataWrap::make5,
                                                        bp::default_call_policies(),
                                                        (bp::arg("array"),
                                                        bp::arg("copy") = true,
                                                        bp::arg("dims") = karabo::util::Dims(),
                                                        bp::arg("encoding") = karabo::xms::Encoding::UNDEFINED,
                                                        bp::arg("bitsPerPixel") = 8)))

                .def("getData", &karathon::ImageDataWrap::getDataPy)

                .def("setData", &karathon::ImageDataWrap::setDataPy, (bp::arg("data"), bp::arg("copy_flag") = true))

                .def("getDimensions", &karathon::ImageDataWrap::getDimensionsPy)

                .def("setDimensions", &karathon::ImageDataWrap::setDimensionsPy, (bp::arg("dims")))

                .def("getDimensionTypes", &karathon::ImageDataWrap::getDimensionTypesPy)

                .def("setDimensionTypes", &karathon::ImageDataWrap::setDimensionTypesPy, (bp::arg("listOfDimTypes")))

                .def("setIsBigEndian", &ImageData::setIsBigEndian, (bp::arg("isBigEndian")))

                .def("getROIOffsets", &karathon::ImageDataWrap::getROIOffsetsPy)

                .def("setROIOffsets", &karathon::ImageDataWrap::setROIOffsetsPy, (bp::arg("offsets")))

                .def("getEncoding", &karathon::ImageDataWrap::getEncodingPy)

                .def("setEncoding", &ImageData::setEncoding, (bp::arg("encoding")))

                .def("toBigEndian", &ImageData::toBigEndian)

                .def("toLittleEndian", &ImageData::toLittleEndian)

                .def("getDimensionScales", &ImageData::getDimensionScales, bp::return_value_policy< bp::copy_const_reference >())

                .def("setDimensionScales", &ImageData::setDimensionScales, (bp::arg("scales")))

                .def("write", &karathon::ImageDataWrap::writePy, (bp::arg("filename"), bp::arg("enableAppendMode") = false))

                .def("setGeometry", &karathon::ImageDataWrap::setGeometryPy, (bp::arg("geometry")))

                .def("getGeometry", &karathon::ImageDataWrap::getGeometryPy)

                .def("getHeader", &karathon::ImageDataWrap::getHeaderPy, bp::return_value_policy< bp::copy_const_reference >())

                .def("setHeader", &karathon::ImageDataWrap::setHeaderPy, (bp::arg("header")))
                //KARABO_PYTHON_FACTORY_CONFIGURATOR(ImageData)
                ;
    }

    {
        bp::implicitly_convertible< Schema &, ImageDataElement >();
        bp::class_<ImageDataElement > ("IMAGEDATA", bp::init<Schema & >((bp::arg("expected"))))

                .def("key", &ImageDataElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("setDefaultValue", &karathon::ImageDataElementWrap().setDefaultValue
                     , (bp::arg("subKey"), bp::arg("defaultValue"))
                     , bp::return_internal_reference<> ())

                .def("commit", &ImageDataElement::commit, bp::return_internal_reference<> ())

                .def("setDimensionScales", &ImageDataElement::setDimensionScales
                     , (bp::arg("scales"))
                     , bp::return_internal_reference<> ())

                .def("setDimensions", &ImageDataElement::setDimensions
                     , (bp::arg("dims"))
                     , bp::return_internal_reference<> ())

                .def("setEncoding", &ImageDataElement::setEncoding
                     , (bp::arg("encoding"))
                     , bp::return_internal_reference<> ())

                .def("setGeometry", &ImageDataElement::setGeometry
                     , (bp::arg("geometry"))
                     , bp::return_internal_reference<>())
                ;
        ;

    }

    {
        bp::class_<OutputChannel, boost::shared_ptr<OutputChannel>, boost::noncopyable >("OutputChannel", bp::no_init)

                .def("setInstanceId", &OutputChannel::setInstanceId, (bp::arg("instanceId")))

                .def("getInstanceId", &OutputChannel::getInstanceId, bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerIOEventHandler", &karathon::OutputChannelWrap().registerIOEventHandlerPy)

                .def("getInformation", &OutputChannel::getInformation)

                .def("write", &karathon::OutputChannelWrap().writePy, (bp::arg("data")))

                .def("update", &karathon::OutputChannelWrap().updatePy)

                .def("signalEndOfStream", &karathon::OutputChannelWrap().signalEndOfStreamPy)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(OutputChannel)
                ;

    }

    {
        bp::implicitly_convertible< Schema &, OutputChannelElement >();
        bp::class_<OutputChannelElement> ("OUTPUT_CHANNEL", bp::init<Schema & >((bp::arg("expected"))))

                .def("key", &OutputChannelElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("displayedName", &OutputChannelElement::displayedName
                     , (bp::arg("name"))
                     , bp::return_internal_reference<> ())

                .def("description", &OutputChannelElement::description
                     , (bp::arg("description"))
                     , bp::return_internal_reference<> ())

                .def("dataSchema", &OutputChannelElement::dataSchema
                     , (bp::arg("schema"))
                     , bp::return_internal_reference<> ())

                .def("commit", &OutputChannelElement::commit, bp::return_internal_reference<> ())

                ;
    }

    {
        bp::class_<InputChannel, boost::shared_ptr<InputChannel>, boost::noncopyable >("InputChannel", bp::no_init)

                .def("reconfigure", &InputChannel::reconfigure, (bp::arg("configuration")))

                .def("setInstanceId", &InputChannel::setInstanceId, (bp::arg("instanceId")))

                .def("getInstanceId", &InputChannel::getInstanceId, bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerDataHandler", &karathon::InputChannelWrap().registerDataHandlerPy)

                .def("registerInputHandler", &karathon::InputChannelWrap().registerInputHandlerPy)

                .def("registerEndOfStreamEventHandler", &karathon::InputChannelWrap().registerEndOfStreamEventHandlerPy)

                .def("triggerIOEvent", &InputChannel::triggerIOEvent)

                .def("triggerEndOfStreamEvent", &InputChannel::triggerEndOfStreamEvent)

                .def("getConnectedOutputChannels", &karathon::InputChannelWrap().getConnectedOutputChannelsPy)

                .def("read", &karathon::InputChannelWrap().readPy, (bp::arg("idx")))

                .def("size", &InputChannel::size)

                .def("getMinimumNumberOfData", &InputChannel::getMinimumNumberOfData)

                .def("connect", &karathon::InputChannelWrap().connectPy, (bp::arg("outputChannelInfo")))

                .def("disconnect", &karathon::InputChannelWrap().disconnectPy, (bp::arg("outputChannelInfo")))

                .def("canCompute", &InputChannel::canCompute)

                .def("update", &karathon::InputChannelWrap().updatePy)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(InputChannel)
                ;
    }

    {
        bp::implicitly_convertible< Schema &, InputChannelElement >();
        bp::class_<InputChannelElement> ("INPUT_CHANNEL", bp::init<Schema & >((bp::arg("expected"))))

                .def("key", &InputChannelElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("displayedName", &InputChannelElement::displayedName
                     , (bp::arg("name"))
                     , bp::return_internal_reference<> ())

                .def("description", &InputChannelElement::description
                     , (bp::arg("description"))
                     , bp::return_internal_reference<> ())

                .def("dataSchema", &InputChannelElement::dataSchema
                     , (bp::arg("schema"))
                     , bp::return_internal_reference<> ())

                .def("commit", &InputChannelElement::commit, bp::return_internal_reference<> ())

                ;
    }
}
