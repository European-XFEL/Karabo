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


    boost::shared_ptr<karabo::xms::ImageData > ImageDataWrap::make5(const bp::object& obj,
                                                                    const karabo::util::Dims& dimensions,
                                                                    const karabo::xms::EncodingType encoding,
                                                                    const int bitsPerPixel) {

        using namespace karabo::util;
        using namespace karabo::xms;

        if (obj.is_none()) {
            boost::shared_ptr<ImageData > self(new ImageData());
            return self;
        } else {
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            const NDArray ndarray = Wrapper::fromPyArrayToNDArray(arr);
            boost::shared_ptr<ImageData > self(new ImageData(ndarray, dimensions, encoding, bitsPerPixel));
            return self;
        }
    }


    bp::object ImageDataWrap::getDataPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        return Wrapper::fromNDArrayToPyArray(self->getData());
    }


    void ImageDataWrap::setDataPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj) {
        PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
        self->setData(Wrapper::fromPyArrayToNDArray(arr));
    }


    bp::object ImageDataWrap::getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        karabo::util::Dims dims = self->getDimensions();
        return Wrapper::fromStdVectorToPyTuple(dims.toVector());
    }


    void ImageDataWrap::setDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setDimensions(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims dims(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setDimensions(dims);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            karabo::util::Dims dims(Wrapper::fromPyTupleToStdVector<unsigned long long>(obj));
            self->setDimensions(dims);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
        }
    }


    bp::object ImageDataWrap::getDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        return Wrapper::fromStdVectorToPyTuple(self->getDimensionTypes());
    }


    void ImageDataWrap::setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj) {
        if (bp::extract<bp::list>(obj).check()) {
            bp::ssize_t size = bp::len(obj);
            std::vector<int> dimTypes(size);
            for (int i = 0; i < size; i++) {
                dimTypes[i] = bp::extract<int>(obj[i]);
            }
            self->setDimensionTypes(dimTypes);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            std::vector<int> dimTypes = Wrapper::fromPyTupleToStdVector<int>(obj);
            self->setDimensionTypes(dimTypes);
        } else {
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
        }
    }


    bp::object ImageDataWrap::getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        karabo::util::Dims offsets = self->getROIOffsets();
        return Wrapper::fromStdVectorToPyTuple(offsets.toVector());
    }


    void ImageDataWrap::setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setROIOffsets(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims offsets(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setROIOffsets(offsets);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            karabo::util::Dims offsets(Wrapper::fromPyTupleToStdVector<unsigned long long>(obj));
            self->setROIOffsets(offsets);
        } else
            throw KARABO_PYTHON_EXCEPTION("Unsupported argument type");
    }


    bp::object ImageDataWrap::getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        return bp::object(karabo::xms::EncodingType(self->getEncoding()));
    }


    void ImageDataWrap::setGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& geometry) {
        karabo::util::DetectorGeometry geo = bp::extract<karabo::util::DetectorGeometry>(geometry);
        self->setGeometry(geo);
    }


    karabo::util::DetectorGeometry ImageDataWrap::getGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        return self->getGeometry();
    }


    void ImageDataWrap::setHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& header) {
        karabo::util::Hash head = bp::extract<karabo::util::Hash>(header);
        self->setHeader(head);
    }


    const karabo::util::Hash& ImageDataWrap::getHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self) {
        return self->getHeader();
    }


    ChannelMetaData::ChannelMetaData(const bp::object& src, const bp::object& ts)
    : karabo::xms::Memory::MetaData(bp::extract<std::string>(src), bp::extract<karabo::util::Timestamp>(ts)) {
    }
    

    void ChannelMetaData::setSource(const bp::object& src) {
        karabo::xms::Memory::MetaData::setSource(bp::extract<std::string>(src));
    }


    bp::object ChannelMetaData::getSource() {
        return bp::object(karabo::xms::Memory::MetaData::getSource());
    }


    void ChannelMetaData::setTimestamp(const bp::object& ts) {
        karabo::xms::Memory::MetaData::setTimestamp(bp::extract<karabo::util::Timestamp>(ts));
    }


    bp::object ChannelMetaData::getTimestamp() {
        return bp::object(karabo::xms::Memory::MetaData::getTimestamp());
    }


    void OutputChannelWrap::registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler) {
        self->registerIOEventHandler(boost::bind(OutputChannelWrap::proxyIOEventHandler, handler, _1));
    }


    void OutputChannelWrap::proxyIOEventHandler(const bp::object& handler, const boost::shared_ptr<karabo::xms::OutputChannel>& outputChannel) {
        ScopedGILAcquire gil;
        handler(bp::object(outputChannel));
    }


    void OutputChannelWrap::writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data, const bp::object& meta) {
        if (!bp::extract<karathon::ChannelMetaData>(meta).check()) {
            throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type for parameter 'meta'. Needs to be ChannelMetaData");
        }

        if (!bp::extract<karabo::util::Hash>(data).check()) {
            throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type for parameter 'data'. Needs to be Hash");
        }
        // we need to copy here before the GIL release, otherwise data might be altered during writing.
        const karabo::util::Hash dataHash = bp::extract<karabo::util::Hash>(data);
        const karabo::xms::Memory::MetaData metaData = bp::extract<karathon::ChannelMetaData>(meta);
        ScopedGILRelease nogil;
        self->write(dataHash, metaData);
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


    void InputChannelWrap::proxyInputHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& input) {
        ScopedGILAcquire gil;
        handler(bp::object(input));
    }


    void InputChannelWrap::registerDataHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
        self->registerDataHandler(boost::bind(InputChannelWrap::proxyDataHandler, handler, _1, _2));
    }


    void InputChannelWrap::proxyDataHandler(const bp::object& handler, const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta) {
        //TODO: wrap MetaData to expose full interface
        //right now this only exposes source
        const karabo::util::Hash mdHash("source", meta.getSource());
        ScopedGILAcquire gil;
        handler(bp::object(data), bp::object(mdHash));
    }


    void InputChannelWrap::registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler) {
        self->registerEndOfStreamEventHandler(boost::bind(InputChannelWrap::proxyEndOfStreamEventHandler, handler, _1));
    }


    void InputChannelWrap::proxyEndOfStreamEventHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& input) {
        ScopedGILAcquire gil;
        handler(bp::object(input));
    }


    bp::object InputChannelWrap::getConnectedOutputChannelsPy(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        typedef std::map<std::string, karabo::util::Hash> OutputChannels;
        const OutputChannels& ochannels = self->getConnectedOutputChannels();
        bp::dict d;
        for (OutputChannels::const_iterator it = ochannels.begin(); it != ochannels.end(); ++it) {
            d[it->first] = bp::object(it->second);
        }
        return bp::object(d);
    }


    bp::object InputChannelWrap::readPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx) {
        karabo::util::Hash::Pointer hash;
        {
            // Release the GIL for the potentially blocking read() call.
            ScopedGILRelease nogil;
            hash = self->read(idx);
        }
        return bp::object(hash);
    }


    void InputChannelWrap::connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
        ScopedGILRelease nogil;
        self->connect(outputChannelInfo);
    }


    void InputChannelWrap::disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo) {
        ScopedGILRelease nogil;
        self->disconnect(outputChannelInfo);
    }


    bp::object InputChannelWrap::getMetaData(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        auto ret = boost::make_shared<std::vector<karabo::util::Hash> >();
        {
            ScopedGILRelease nogil;
            std::vector<karabo::xms::InputChannel::MetaData> md = self->getMetaData();
            for (auto it = md.begin(); it != md.end(); ++it) {
                ret->push_back(*reinterpret_cast<karabo::util::Hash*> (&*it));
            }
        }
        return bp::object(ret);
    }

}


template <typename T>
void exportPyXmsImageData() {

    const std::string typeName(karabo::util::Types::convert<karabo::util::FromTypeInfo, karabo::util::ToLiteral>(typeid (T)));

}


void exportPyXmsInputOutputChannel() {
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
    }

    {
        bp::class_<karabo::xms::ImageData, boost::shared_ptr<karabo::xms::ImageData > >("ImageData", bp::init<>())

                .def("__init__", bp::make_constructor(&karathon::ImageDataWrap::make5,
                                                      bp::default_call_policies(),
                                                      (bp::arg("array"),
                                                       bp::arg("dims") = karabo::util::Dims(),
                                                       bp::arg("encoding") = karabo::xms::Encoding::UNDEFINED,
                                                       bp::arg("bitsPerPixel") = 0)))

                .def("getData", &karathon::ImageDataWrap::getDataPy)

                .def("setData", &karathon::ImageDataWrap::setDataPy, (bp::arg("data")))

                .def("getDimensions", &karathon::ImageDataWrap::getDimensionsPy)

                .def("setDimensions", &karathon::ImageDataWrap::setDimensionsPy, (bp::arg("dims")))

                .def("getDimensionTypes", &karathon::ImageDataWrap::getDimensionTypesPy)

                .def("setDimensionTypes", &karathon::ImageDataWrap::setDimensionTypesPy, (bp::arg("listOfDimTypes")))

                .def("getROIOffsets", &karathon::ImageDataWrap::getROIOffsetsPy)

                .def("setROIOffsets", &karathon::ImageDataWrap::setROIOffsetsPy, (bp::arg("offsets")))

                .def("getBitsPerPixel", &karabo::xms::ImageData::getBitsPerPixel)

                .def("setBitsPerPixel", &karabo::xms::ImageData::setBitsPerPixel, (bp::arg("bitsPerPixel")))

                .def("getEncoding", &karathon::ImageDataWrap::getEncodingPy)

                .def("setEncoding", &karabo::xms::ImageData::setEncoding, (bp::arg("encoding")))

                .def("isIndexable", &karabo::xms::ImageData::isIndexable)

                .def("getDimensionScales", &karabo::xms::ImageData::getDimensionScales, bp::return_value_policy< bp::copy_const_reference >())

                .def("setDimensionScales", &karabo::xms::ImageData::setDimensionScales, (bp::arg("scales")))

                .def("setGeometry", &karathon::ImageDataWrap::setGeometryPy, (bp::arg("geometry")))

                .def("getGeometry", &karathon::ImageDataWrap::getGeometryPy)

                .def("getHeader", &karathon::ImageDataWrap::getHeaderPy, bp::return_value_policy< bp::copy_const_reference >())

                .def("setHeader", &karathon::ImageDataWrap::setHeaderPy, (bp::arg("header")))
                ;
    }
    {
        bp::implicitly_convertible< karabo::util::Schema &, karabo::xms::ImageDataElement >();
        bp::class_<karabo::xms::ImageDataElement > ("IMAGEDATA_ELEMENT", bp::init<karabo::util::Schema & >((bp::arg("expected"))))

                .def("key", &karabo::xms::ImageDataElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("displayedName", &karabo::xms::ImageDataElement::displayedName
                     , bp::arg("name")
                     , bp::return_internal_reference<> ())

                .def("description", &karabo::xms::ImageDataElement::description
                     , bp::arg("desc")
                     , bp::return_internal_reference<> ())

                // .def("setDefaultValue", &karathon::ImageDataElementWrap().setDefaultValue
                //     , (bp::arg("subKey"), bp::arg("defaultValue"))
                //     , bp::return_internal_reference<> ())


                .def("observerAccess", &karabo::xms::ImageDataElement::observerAccess
                     , bp::return_internal_reference<> ())

                .def("userAccess", &karabo::xms::ImageDataElement::userAccess
                     , bp::return_internal_reference<> ())

                .def("operatorAccess", &karabo::xms::ImageDataElement::operatorAccess
                     , bp::return_internal_reference<> ())

                .def("expertAccess", &karabo::xms::ImageDataElement::expertAccess
                     , bp::return_internal_reference<> ())

                .def("adminAccess", &karabo::xms::ImageDataElement::adminAccess
                     , bp::return_internal_reference<> ())

                .def("skipValidation", &karabo::xms::ImageDataElement::skipValidation
                     , bp::return_internal_reference<> ())

                .def("commit", &karabo::xms::ImageDataElement::commit
                     , bp::return_internal_reference<> ())

                .def("setDimensionScales", &karabo::xms::ImageDataElement::setDimensionScales
                     , (bp::arg("scales"))
                     , bp::return_internal_reference<> ())

                .def("setDimensions", &karabo::xms::ImageDataElement::setDimensions
                     , (bp::arg("dims"))
                     , bp::return_internal_reference<> ())

                .def("setEncoding", &karabo::xms::ImageDataElement::setEncoding
                     , (bp::arg("encoding"))
                     , bp::return_internal_reference<> ())

                .def("setGeometry", &karabo::xms::ImageDataElement::setGeometry
                     , (bp::arg("geometry"))
                     , bp::return_internal_reference<>())
                ;
    }


    {
        bp::class_<karathon::ChannelMetaData, boost::noncopyable>("ChannelMetaData", bp::init<bp::object const &, bp::object const &>())
                .def("setSource", &karathon::ChannelMetaData::setSource, (bp::arg("source")))
                .def("getSource", &karathon::ChannelMetaData::getSource)
                .def("setTimestamp", &karathon::ChannelMetaData::setTimestamp, (bp::arg("timestamp")))
                .def("getTimestamp", &karathon::ChannelMetaData::getTimestamp)
                ;
    }

    {
        bp::class_<karabo::xms::OutputChannel, boost::shared_ptr<karabo::xms::OutputChannel>, boost::noncopyable >("OutputChannel", bp::no_init)

                .def("setInstanceIdAndName", &karabo::xms::OutputChannel::setInstanceIdAndName, (bp::arg("instanceId"), bp::arg("name")))

                .def("getInstanceId", &karabo::xms::OutputChannel::getInstanceId, bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerIOEventHandler", &karathon::OutputChannelWrap().registerIOEventHandlerPy)

                .def("getInformation", &karabo::xms::OutputChannel::getInformation)

                .def("write", &karathon::OutputChannelWrap().writePy, (bp::arg("data"), bp::arg("meta")))

                .def("update", &karathon::OutputChannelWrap().updatePy)

                .def("signalEndOfStream", &karathon::OutputChannelWrap().signalEndOfStreamPy)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(karabo::xms::OutputChannel)
                ;

    }

    {
        bp::implicitly_convertible< karabo::util::Schema &, karabo::xms::OutputChannelElement >();
        bp::class_<karabo::xms::OutputChannelElement> ("OUTPUT_CHANNEL", bp::init<karabo::util::Schema & >((bp::arg("expected"))))

                .def("key", &karabo::xms::OutputChannelElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("displayedName", &karabo::xms::OutputChannelElement::displayedName
                     , (bp::arg("name"))
                     , bp::return_internal_reference<> ())

                .def("description", &karabo::xms::OutputChannelElement::description
                     , (bp::arg("description"))
                     , bp::return_internal_reference<> ())

                .def("dataSchema", &karabo::xms::OutputChannelElement::dataSchema
                     , (bp::arg("schema"))
                     , bp::return_internal_reference<> ())

                .def("commit", &karabo::xms::OutputChannelElement::commit
                     , bp::return_internal_reference<> ())

                ;
    }

    {
        bp::class_<karabo::xms::InputChannel, boost::shared_ptr<karabo::xms::InputChannel>, boost::noncopyable >("InputChannel", bp::no_init)

                .def("reconfigure", &karabo::xms::InputChannel::reconfigure
                     , (bp::arg("configuration")))

                .def("setInstanceId", &karabo::xms::InputChannel::setInstanceId
                     , (bp::arg("instanceId")))

                .def("getInstanceId", &karabo::xms::InputChannel::getInstanceId
                     , bp::return_value_policy<bp::copy_const_reference > ())

                .def("registerDataHandler", &karathon::InputChannelWrap().registerDataHandlerPy)

                .def("registerInputHandler", &karathon::InputChannelWrap().registerInputHandlerPy)

                .def("registerEndOfStreamEventHandler", &karathon::InputChannelWrap().registerEndOfStreamEventHandlerPy)

                .def("triggerIOEvent", &karabo::xms::InputChannel::triggerIOEvent)

                .def("triggerEndOfStreamEvent", &karabo::xms::InputChannel::triggerEndOfStreamEvent)

                .def("getConnectedOutputChannels", &karathon::InputChannelWrap().getConnectedOutputChannelsPy)

                .def("read", &karathon::InputChannelWrap().readPy
                     , (bp::arg("idx")))

                .def("size", &karabo::xms::InputChannel::size)

                .def("getMinimumNumberOfData", &karabo::xms::InputChannel::getMinimumNumberOfData)

                .def("connect", &karathon::InputChannelWrap().connectPy
                     , (bp::arg("outputChannelInfo")))

                .def("disconnect", &karathon::InputChannelWrap().disconnectPy
                     , (bp::arg("outputChannelInfo")))

                .def("canCompute", &karabo::xms::InputChannel::canCompute)

                .def("getMetaData", &karathon::InputChannelWrap().getMetaData)

                KARABO_PYTHON_FACTORY_CONFIGURATOR(karabo::xms::InputChannel)
                ;
    }

    {
        bp::implicitly_convertible< karabo::util::Schema &, karabo::xms::InputChannelElement >();
        bp::class_<karabo::xms::InputChannelElement> ("INPUT_CHANNEL", bp::init<karabo::util::Schema & >((bp::arg("expected"))))

                .def("key", &karabo::xms::InputChannelElement::key
                     , (bp::arg("key"))
                     , bp::return_internal_reference<> ())

                .def("displayedName", &karabo::xms::InputChannelElement::displayedName
                     , (bp::arg("name"))
                     , bp::return_internal_reference<> ())

                .def("description", &karabo::xms::InputChannelElement::description
                     , (bp::arg("description"))
                     , bp::return_internal_reference<> ())

                .def("dataSchema", &karabo::xms::InputChannelElement::dataSchema
                     , (bp::arg("schema"))
                     , bp::return_internal_reference<> ())

                .def("commit", &karabo::xms::InputChannelElement::commit
                     , bp::return_internal_reference<> ())

                ;
    }
}
