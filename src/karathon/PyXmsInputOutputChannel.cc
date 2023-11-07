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

#include "PyXmsInputOutputChannel.hh"

namespace bp = boost::python;

namespace karathon {


    boost::shared_ptr<karabo::xms::ImageData> ImageDataWrap::make5(const bp::object& obj,
                                                                   const karabo::util::Dims& dimensions,
                                                                   const karabo::xms::EncodingType encoding,
                                                                   const int bitsPerPixel) {
        using namespace karabo::util;
        using namespace karabo::xms;

        if (obj.is_none()) {
            auto self = boost::make_shared<ImageData>();
            return self;
        } else {
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
            const NDArray ndarray = Wrapper::fromPyArrayToNDArray(arr);
            auto self = boost::make_shared<ImageData>(ndarray, dimensions, encoding, bitsPerPixel);
            return self;
        }
    }


    bp::object ImageDataWrap::getDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return Wrapper::fromNDArrayToPyArray(self->getData());
    }


    void ImageDataWrap::setDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
        self->setData(Wrapper::fromPyArrayToNDArray(arr));
    }


    bp::object ImageDataWrap::getTypePy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return bp::object(static_cast<int>(self->getDataType()));
    }


    void ImageDataWrap::setTypePy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        using namespace karabo::util;
        Types::ReferenceType reftype = Types::UNKNOWN;

        if (bp::extract<std::string>(obj).check()) {
            std::string type = bp::extract<std::string>(obj);
            reftype = Types::from<FromLiteral>(type);
        } else if (bp::extract<karathon::PyTypes::ReferenceType>(obj).check()) {
            karathon::PyTypes::ReferenceType type = bp::extract<karathon::PyTypes::ReferenceType>(obj);
            reftype = karathon::PyTypes::to(type);
        } else {
            throw KARABO_PYTHON_EXCEPTION(
                  "Python type of setType() of ImageData must be a string or Types enumerated value.");
        }
        self->setDataType(reftype);
    }


    bp::object ImageDataWrap::getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        karabo::util::Dims dims = self->getDimensions();
        return Wrapper::fromStdVectorToPyTuple(dims.toVector());
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
        } else {
            throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") + obj.ptr()->ob_type->tp_name +
                                          "'");
        }
    }


    bp::object ImageDataWrap::getDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return Wrapper::fromStdVectorToPyTuple(self->getDimensionTypes());
    }


    void ImageDataWrap::setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData>& self,
                                            const bp::object& obj) {
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
            throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") + obj.ptr()->ob_type->tp_name +
                                          "'");
        }
    }


    bp::object ImageDataWrap::getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        karabo::util::Dims offsets = self->getROIOffsets();
        return Wrapper::fromStdVectorToPyTuple(offsets.toVector());
    }


    void ImageDataWrap::setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setROIOffsets(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims offsets(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setROIOffsets(offsets);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            karabo::util::Dims offsets(Wrapper::fromPyTupleToStdVector<unsigned long long>(obj));
            self->setROIOffsets(offsets);
        } else {
            throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") + obj.ptr()->ob_type->tp_name +
                                          "'");
        }
    }


    bp::object ImageDataWrap::getBinningPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        karabo::util::Dims binning = self->getBinning();
        return Wrapper::fromStdVectorToPyTuple(binning.toVector());
    }


    void ImageDataWrap::setBinningPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj) {
        if (bp::extract<karabo::util::Dims>(obj).check()) {
            self->setBinning(bp::extract<karabo::util::Dims>(obj));
        } else if (bp::extract<bp::list>(obj).check()) {
            karabo::util::Dims binning(Wrapper::fromPyListToStdVector<unsigned long long>(obj));
            self->setBinning(binning);
        } else if (bp::extract<bp::tuple>(obj).check()) {
            karabo::util::Dims binning(Wrapper::fromPyTupleToStdVector<unsigned long long>(obj));
            self->setBinning(binning);
        } else {
            throw KARABO_PYTHON_EXCEPTION(std::string("Unsupported argument type '") + obj.ptr()->ob_type->tp_name +
                                          "'");
        }
    }


    bp::object ImageDataWrap::getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData>& self) {
        return bp::object(karabo::xms::EncodingType(self->getEncoding()));
    }


    ChannelMetaData::ChannelMetaData(const bp::object& src, const bp::object& ts)
        : karabo::xms::Memory::MetaData(bp::extract<std::string>(src), bp::extract<karabo::util::Timestamp>(ts)) {}


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


    void OutputChannelWrap::registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self,
                                                     const bp::object& handler) {
        using Wrap = HandlerWrap<const karabo::xms::OutputChannel::Pointer&>;
        self->registerIOEventHandler(Wrap(handler, "IOEvent"));
    }


    void OutputChannelWrap::registerShowConnectionsHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self,
                                                             const bp::object& handler) {
        HandlerWrap<const std::vector<karabo::util::Hash>&> wrappedHandler(handler, "show connections");
        ScopedGILRelease noGil;
        // Setting the handler might overwrite and thus destruct a previous handler.
        // That one's destruction might acquire the GIL via the destructor of HandlerWrap.
        // So better release the GIL here (although the deadlock has not been seen without releasing).
        self->registerShowConnectionsHandler(
              std::move(wrappedHandler)); // move for once when handler registration will take rvalue reference...
    }

    void OutputChannelWrap::registerShowStatisticsHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self,
                                                            const bp::object& handler) {
        HandlerWrapVullVull wrappedHandler(handler, "show statistics");
        ScopedGILRelease noGil;
        // Setting the handler might overwrite and thus destruct a previous handler.
        // That one's destruction might acquire the GIL via the destructor of HandlerWrap.
        self->registerShowStatisticsHandler(wrappedHandler);
    }

    void OutputChannelWrap::registerSharedInputSelectorPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self,
                                                          const bp::object& handler) {
        karabo::xms::SharedInputSelector selector;
        if (!handler.is_none()) {
            selector = ReturnHandlerWrap<std::string, const std::vector<std::string>&>(handler, "sharedInputSelector");
        } // else we reset the selection
        self->registerSharedInputSelector(std::move(selector));
    }

    void OutputChannelWrap::writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data,
                                    const bp::object& meta, bool copyAllData) {
        if (!bp::extract<karathon::ChannelMetaData>(meta).check()) {
            throw KARABO_PYTHON_EXCEPTION(
                  "Unsupported parameter type for parameter 'meta'. Needs to be ChannelMetaData");
        }

        if (!bp::extract<karabo::util::Hash>(data).check()) {
            throw KARABO_PYTHON_EXCEPTION("Unsupported parameter type for parameter 'data'. Needs to be Hash");
        }
        // we need to copy here before the GIL release, otherwise data might be altered during writing.
        const karabo::util::Hash dataHash = bp::extract<karabo::util::Hash>(data);
        const karabo::xms::Memory::MetaData metaData = bp::extract<karathon::ChannelMetaData>(meta);
        ScopedGILRelease nogil;
        self->write(dataHash, metaData, copyAllData);
    }


    void OutputChannelWrap::updatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, bool safeNDArray) {
        ScopedGILRelease nogil;
        self->update(safeNDArray);
    }

    void OutputChannelWrap::asyncUpdatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, bool safeNDArray,
                                          const bp::object& readyHandler) {
        boost::function<void()> handler;
        if (readyHandler.is_none()) {
            handler = []() {};
        } else {
            handler = HandlerWrap<>(readyHandler, "asyncUpdate");
        }
        ScopedGILRelease nogil; // This asyncUpdate is only partially async and can block, so release GIL
        self->asyncUpdate(safeNDArray, std::move(handler));
    }


    void OutputChannelWrap::signalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self) {
        ScopedGILRelease nogil;
        self->signalEndOfStream();
    }


    void OutputChannelWrap::asyncSignalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self,
                                                     const bp::object& readyHandler) {
        boost::function<void()> handler;
        if (readyHandler.is_none()) {
            handler = []() {};
        } else {
            handler = HandlerWrap<>(readyHandler, "asyncSignalEndOfStream");
        }
        ScopedGILRelease nogil;
        self->asyncSignalEndOfStream(std::move(handler));
    }


    bp::object OutputChannelWrap::create(const std::string& instanceId, const std::string& channelName,
                                         const karabo::util::Hash& channelConfig) {
        using namespace karabo::xms;
        using namespace karabo::util;
        OutputChannel::Pointer channel = Configurator<OutputChannel>::create("OutputChannel", channelConfig, 0);
        channel->setInstanceIdAndName(instanceId, channelName);
        channel->initialize();
        return bp::object(channel);
    }


    void InputChannelWrap::registerInputHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                                  const bp::object& handler) {
        self->registerInputHandler(HandlerWrap<const karabo::xms::InputChannel::Pointer&>(handler, "input"));
    }


    void InputChannelWrap::registerConnectionTrackerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                                       const bp::object& statusTracker) {
        using Wrap = HandlerWrap<const std::string&, karabo::net::ConnectionStatus>;
        self->registerConnectionTracker(Wrap(statusTracker, "channelStatusTracker"));
    }


    void InputChannelWrap::registerDataHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                                 const bp::object& handler) {
        self->registerDataHandler(InputChannelWrap::DataHandlerWrap(handler, "data"));
    }


    void InputChannelWrap::registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                                             const bp::object& handler) {
        self->registerEndOfStreamEventHandler(HandlerWrap<const karabo::xms::InputChannel::Pointer&>(handler, "EOS"));
    }


    bp::object InputChannelWrap::getConnectionStatusPy(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        typedef std::unordered_map<std::string, karabo::net::ConnectionStatus> ConnStatusMap;
        ConnStatusMap csmap = self->getConnectionStatus();
        bp::dict d;
        for (auto it = csmap.begin(); it != csmap.end(); ++it) {
            d[it->first] = bp::object(it->second);
        }
        return bp::object(d);
    }


    bp::object InputChannelWrap::getConnectedOutputChannelsPy(
          const boost::shared_ptr<karabo::xms::InputChannel>& self) {
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


    bp::object InputChannelWrap::connectSyncPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                               const karabo::util::Hash& outputChannelInfo) {
        std::string msg;
        {
            ScopedGILRelease nogil;
            auto promi = std::make_shared<std::promise<boost::system::error_code>>();
            auto futur = promi->get_future();
            self->connect(outputChannelInfo, [promi](const boost::system::error_code& e) { promi->set_value(e); });
            auto e = futur.get();
            if (e.value()) msg = e.message();
        }
        return bp::object(msg);
    }


    void InputChannelWrap::connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                     const karabo::util::Hash& outputChannelInfo, const bp::object& handler) {
        auto wrappedHandler = HandlerWrap<const std::string&>(handler, "Connect");
        ScopedGILRelease nogil;
        self->connect(outputChannelInfo, [wrappedHandler](const boost::system::error_code& e) {
            std::string msg;
            if (e.value()) msg = e.message();
            wrappedHandler(msg);
        });
    }


    void InputChannelWrap::disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self,
                                        const karabo::util::Hash& outputChannelInfo) {
        ScopedGILRelease nogil;
        self->disconnect(outputChannelInfo);
    }


    bp::object InputChannelWrap::getMetaData(const boost::shared_ptr<karabo::xms::InputChannel>& self) {
        auto ret = boost::make_shared<std::vector<karabo::util::Hash>>();
        {
            ScopedGILRelease nogil;
            std::vector<karabo::xms::InputChannel::MetaData> md = self->getMetaData();
            for (auto it = md.begin(); it != md.end(); ++it) {
                // TODO: Properly wrap MetaData object - currently this will be visible in Python as hash
                ret->push_back(*reinterpret_cast<karabo::util::Hash*>(&*it));
            }
        }
        return bp::object(ret);
    }


    InputChannelWrap::DataHandlerWrap::DataHandlerWrap(const bp::object& handler, char const* const where)
        : HandlerWrap<const karabo::util::Hash&, const karabo::xms::InputChannel::MetaData&>(handler, where) {}


    void InputChannelWrap::DataHandlerWrap::operator()(const karabo::util::Hash& data,
                                                       const karabo::xms::InputChannel::MetaData& meta) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                // TODO: wrap MetaData to expose full interface, right now this makes it look like a Hash within Python
                //       (Then one can get rid of InputChannelWrap::DataHandlerWrap and directly
                //        use HandlerWrap<const karabo::util::Hash&, const karabo::xms::InputChannel::MetaData&>)
                (*m_handler)(data, *(reinterpret_cast<const karabo::util::Hash*>(&meta)));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where); // from Wrapper.hh
        } catch (...) {
            KARABO_RETHROW
        }
    }


    bp::object InputChannelWrap::create(const std::string& instanceId, const std::string& channelName,
                                        const karabo::util::Hash& channelConfig) {
        using namespace karabo::xms;
        using namespace karabo::util;
        InputChannel::Pointer channel = Configurator<InputChannel>::create("InputChannel", channelConfig);
        channel->setInstanceId(instanceId + ":" + channelName);
        return bp::object(channel);
    }
} // namespace karathon


template <typename T>
void exportPyXmsImageData() {
    const std::string typeName(
          karabo::util::Types::convert<karabo::util::FromTypeInfo, karabo::util::ToLiteral>(typeid(T)));
}


void exportPyXmsInputOutputChannel() {
    {
        bp::enum_<karabo::xms::Encoding::EncodingType>("Encoding")
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
              .export_values();
    }

    {
        bp::enum_<karabo::xms::Rotation::RotationType>("Rotation")
              .value("UNDEFINED", karabo::xms::Rotation::UNDEFINED)
              .value("ROT_0", karabo::xms::Rotation::ROT_0)
              .value("ROT_90", karabo::xms::Rotation::ROT_90)
              .value("ROT_180", karabo::xms::Rotation::ROT_180)
              .value("ROT_270", karabo::xms::Rotation::ROT_270)
              .export_values();
    }

    {
        bp::class_<karabo::xms::ImageData, boost::shared_ptr<karabo::xms::ImageData>>("ImageData", bp::init<>())

              .def("__init__", bp::make_constructor(&karathon::ImageDataWrap::make5, bp::default_call_policies(),
                                                    (bp::arg("array"), bp::arg("dims") = karabo::util::Dims(),
                                                     bp::arg("encoding") = karabo::xms::Encoding::UNDEFINED,
                                                     bp::arg("bitsPerPixel") = 0)))

              .def("getData", &karathon::ImageDataWrap::getDataPy)

              .def("setData", &karathon::ImageDataWrap::setDataPy, (bp::arg("data")))

              .def("getType", &karathon::ImageDataWrap::getTypePy)

              .def("setType", &karathon::ImageDataWrap::setTypePy, (bp::arg("dtype")))

              .def("getDimensions", &karathon::ImageDataWrap::getDimensionsPy)

              .def("setDimensions", &karathon::ImageDataWrap::setDimensionsPy, (bp::arg("dims")))

              .def("getDimensionTypes", &karathon::ImageDataWrap::getDimensionTypesPy)

              .def("setDimensionTypes", &karathon::ImageDataWrap::setDimensionTypesPy, (bp::arg("listOfDimTypes")))

              .def("getROIOffsets", &karathon::ImageDataWrap::getROIOffsetsPy)

              .def("setROIOffsets", &karathon::ImageDataWrap::setROIOffsetsPy, (bp::arg("offsets")))

              .def("getBinning", &karathon::ImageDataWrap::getBinningPy)

              .def("setBinning", &karathon::ImageDataWrap::setBinningPy, (bp::arg("binning")))

              .def("getRotation", &karabo::xms::ImageData::getRotation)

              .def("setRotation", &karabo::xms::ImageData::setRotation, (bp::arg("rotation")))

              .def("getFlipX", &karabo::xms::ImageData::getFlipX)

              .def("setFlipX", &karabo::xms::ImageData::setFlipX, (bp::arg("flipX")))

              .def("getFlipY", &karabo::xms::ImageData::getFlipY)

              .def("setFlipY", &karabo::xms::ImageData::setFlipY, (bp::arg("flipY")))

              .def("getBitsPerPixel", &karabo::xms::ImageData::getBitsPerPixel)

              .def("setBitsPerPixel", &karabo::xms::ImageData::setBitsPerPixel, (bp::arg("bitsPerPixel")))

              .def("getEncoding", &karathon::ImageDataWrap::getEncodingPy)

              .def("setEncoding", &karabo::xms::ImageData::setEncoding, (bp::arg("encoding")))

              .def("isIndexable", &karabo::xms::ImageData::isIndexable)

              .def("getDimensionScales", &karabo::xms::ImageData::getDimensionScales,
                   bp::return_value_policy<bp::copy_const_reference>())

              .def("setDimensionScales", &karabo::xms::ImageData::setDimensionScales, (bp::arg("scales")));
    }
    {
        bp::implicitly_convertible<karabo::util::Schema&, karabo::xms::ImageDataElement>();
        bp::class_<karabo::xms::ImageDataElement>("IMAGEDATA_ELEMENT",
                                                  bp::init<karabo::util::Schema&>((bp::arg("expected"))))

              .def("key", &karabo::xms::ImageDataElement::key, (bp::arg("key")), bp::return_internal_reference<>())

              .def("displayedName", &karabo::xms::ImageDataElement::displayedName, bp::arg("name"),
                   bp::return_internal_reference<>())

              .def("description", &karabo::xms::ImageDataElement::description, bp::arg("desc"),
                   bp::return_internal_reference<>())

              // .def("setDefaultValue", &karathon::ImageDataElementWrap().setDefaultValue
              //     , (bp::arg("subKey"), bp::arg("defaultValue"))
              //     , bp::return_internal_reference<> ())


              .def("observerAccess", &karabo::xms::ImageDataElement::observerAccess, bp::return_internal_reference<>())

              .def("userAccess", &karabo::xms::ImageDataElement::userAccess, bp::return_internal_reference<>())

              .def("operatorAccess", &karabo::xms::ImageDataElement::operatorAccess, bp::return_internal_reference<>())

              .def("expertAccess", &karabo::xms::ImageDataElement::expertAccess, bp::return_internal_reference<>())

              .def("adminAccess", &karabo::xms::ImageDataElement::adminAccess, bp::return_internal_reference<>())

              .def("skipValidation", &karabo::xms::ImageDataElement::skipValidation, bp::return_internal_reference<>())

              .def("commit", &karabo::xms::ImageDataElement::commit, bp::return_internal_reference<>())

              .def("setDimensionScales", &karabo::xms::ImageDataElement::setDimensionScales, (bp::arg("scales")),
                   bp::return_internal_reference<>())

              .def("setDimensions", &karathon::ImageDataElementWrap::setDimensions, (bp::arg("dims")),
                   bp::return_internal_reference<>(),
                   "h.setDimensions(dims) set the shape of an image in the schema. This is required by the DAQ,"
                   "otherwise silent data-loss or segfaults can occur. The shape can be a list or string with 2 or 3 "
                   "dimensions (for color images).\nExample:\n\t"
                   "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setDimensions('480,640,3')\n\t\t.commit()\n\t"
                   "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setDimensions([480,640])\n\t\t.commit()")

              .def("setType", &karathon::ImageDataElementWrap::setType, (bp::arg("type")),
                   bp::return_internal_reference<>(),
                   "h.setType(type) set the datatype of an image in the schema. This is required by the DAQ,"
                   "otherwise silent data-loss or segfaults can occur. The type can be a member of the 'Types' "
                   "class or a string (all capitals).\nExample:\n\t"
                   "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setType('UINT16')\n\t\t.commit()\n\t"
                   "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setType(Types.UINT16)\n\t\t.commit()")

              .def("setEncoding", &karathon::ImageDataElementWrap::setEncoding, (bp::arg("encoding")),
                   bp::return_internal_reference<>(),
                   "h.setEncoding(encoding) set the encoding of an image in the schema. The encoding name must be a "
                   "string (all capitals).\nExample:\n\t"
                   "IMAGEDATA_ELEMENT(s)\n\t\t.key('data.image')\n\t\t.setEncoding('RGB')\n\t\t.commit()")

              .def("setAllowedActions", &karathon::ImageDataElementWrap::setAllowedActions, (bp::arg("actions")),
                   bp::return_internal_reference<>(),
                   "Specify one or more actions that are allowed on this node.\n"
                   "If a Karabo device specifies allowed actions for a node,\n"
                   "that means that it offers a specific slot interface to operate\n"
                   "on this node. Which allowed actions require which interface\n"
                   "is defined elsewhere.");
    }


    {
        bp::class_<karathon::ChannelMetaData, boost::noncopyable>("ChannelMetaData",
                                                                  bp::init<bp::object const&, bp::object const&>())
              .def("setSource", &karathon::ChannelMetaData::setSource, (bp::arg("source")))
              .def("getSource", &karathon::ChannelMetaData::getSource)
              .def("setTimestamp", &karathon::ChannelMetaData::setTimestamp, (bp::arg("timestamp")))
              .def("getTimestamp", &karathon::ChannelMetaData::getTimestamp);
    }

    {
        bp::class_<karabo::xms::OutputChannel, boost::shared_ptr<karabo::xms::OutputChannel>, boost::noncopyable>(
              "OutputChannel", bp::no_init)

              .def("setInstanceIdAndName", &karabo::xms::OutputChannel::setInstanceIdAndName,
                   (bp::arg("instanceId"), bp::arg("name")))

              .def("getInstanceId", &karabo::xms::OutputChannel::getInstanceId,
                   bp::return_value_policy<bp::copy_const_reference>())

              .def("registerIOEventHandler", &karathon::OutputChannelWrap().registerIOEventHandlerPy)

              .def("getInformation", &karabo::xms::OutputChannel::getInformation)

              .def("hasRegisteredCopyInputChannel", &karabo::xms::OutputChannel::hasRegisteredCopyInputChannel,
                   (bp::arg("instanceId")))

              .def("hasRegisteredSharedInputChannel", &karabo::xms::OutputChannel::hasRegisteredSharedInputChannel,
                   (bp::arg("instanceId")))

              .def("write", &karathon::OutputChannelWrap().writePy,
                   (bp::arg("data"), bp::arg("meta"), bp::arg("copyAllData") = false),
                   "data - a Hash with the data to write\n"
                   "meta - ChannelMetaData\n"
                   "copyAllData - unused\n\n"
                   "Note 1: Any NDArray/ImageData inside data must stay untouched at least\n"
                   "        until update() has been called. See also the documentation of the\n"
                   "        safeNDArray flag of the update() method.\n"
                   "Note 2: The methods 'write(..)', 'update()' and 'signalEndOfStream()'\n"
                   "        must not be called concurrently")

              .def("update", &karathon::OutputChannelWrap().updatePy, (bp::arg("safeNDArray") = false),
                   "Update the output channel, i.e. send all data over the wire that was\n"
                   "previously written by calling write(..).\n\n"
                   "safeNdarray - boolean to indicate whether all ndarrays inside the Hash\n"
                   "              passed to write(..) before are 'safe', i.e. their memory\n"
                   "              will not be referred to elsewhere after update is finished.\n"
                   "              Default is 'false', 'true' can avoid safety copies of NDArray\n"
                   "              content when data is queued or sent locally.\n"
                   "Note: The methods 'write(..)', 'update()' and 'signalEndOfStream()' must\n"
                   "      not be called concurrently")

              .def("asyncUpdate", &karathon::OutputChannelWrap().asyncUpdatePy,
                   (bp::arg("safeNDArray") = false, bp::arg("readyHandler") = bp::object()),
                   "Semi-asynchronously update the output channel, i.e. asynchronously send all\n"
                   "data over the wire that was previously written by calling write(...), but\n"
                   "block as long as any of the connected InputChannels requires if wait is\n"
                   "configured.\n"
                   "safeNDArray - boolean to indicate whether all NDArrays inside the Hash\n"
                   "              passed to write(..) before are 'safe', i.e. their memory will\n"
                   "              not be referred to elsewhere after update is finished.\n"
                   "              Default is 'false', 'true' can avoid safety copies of NDArray\n"
                   "              content when data is queued or sent locally.\n"
                   "readyHandler - callback when data (that is not queued) has been sent and\n"
                   "               thus even NDArray data inside it can be re-used again\n"
                   "               (except if safeNDArray was set to 'true' in which case its\n"
                   "               memory may still be used in a queue).\n\n"
                   "Thread safety:\n"
                   "All the 'write(..)' methods, '[async]Update(..)' and\n"
                   "'[async]SignalEndOfStream(..)' must not be called concurrently.")

              .def("signalEndOfStream", &karathon::OutputChannelWrap().signalEndOfStreamPy,
                   "Send end-of-stream (EOS) notification to all connected input channels to\n"
                   "indicate a logical break in the data stream.\n"
                   "Note: The methods 'write(..)', 'update()' and 'signalEndOfStream()' must not\n"
                   "      be called concurrently")

              .def("asyncSignalEndOfStream", &karathon::OutputChannelWrap().asyncSignalEndOfStreamPy,
                   (bp::arg("readyHandler") = bp::object()),
                   "Asynchonously send end-of-stream (EOS) notification to all connected input\n"
                   "channels to indicate a logical break in the data stream.\n\n"
                   "readyHandler - callback when notification has been sent or queued\n\n"
                   "Thread safety:\n"
                   "All the 'write(..)' methods, '[async]Update(..)' and\n"
                   "'[async]SignalEndOfStream(..)' must not be called concurrently.")

              .def("registerShowConnectionsHandler", &karathon::OutputChannelWrap().registerShowConnectionsHandlerPy,
                   (bp::arg("handler")),
                   "Register a handler to be called when the 'connection' table changes.\n"
                   "Argument of the handler is a list of Hash as described by the row schema\n"
                   "of the 'connection' table")

              .def("registerShowStatisticsHandler", &karathon::OutputChannelWrap().registerShowStatisticsHandlerPy,
                   (bp::arg("handler")),
                   "Register a handler to be regularly called to update written and read bytes.\n"
                   "Argument of the handler are two lists of numbers: bytes read from and written to\n"
                   "connected channels, in the same order as in the connection table.")

              .def("registerSharedInputSelector", &karathon::OutputChannelWrap().registerSharedInputSelectorPy,
                   (bp::arg("selector")),
                   R"(Register handler that selects which of the connected input channels that have
dataDistribution = "shared" is to be served.

The handler will be called during update() with the ids of the connected
"shared" input channels (e.g. "deviceId:input") as argument. The returned
channel id will receive the data. If an empty string or an unknown id is
returned, the data will be dropped.

:param selector takes a list of str as argument and shall return str)")
              .def("create", &karathon::OutputChannelWrap::create,
                   (bp::arg("instanceId"), bp::arg("channelName"), bp::arg("channelConfig")))
              .staticmethod("create");
    }

    {
        bp::implicitly_convertible<karabo::util::Schema&, karabo::xms::OutputChannelElement>();
        bp::class_<karabo::xms::OutputChannelElement>("OUTPUT_CHANNEL",
                                                      bp::init<karabo::util::Schema&>((bp::arg("expected"))))

              .def("key", &karabo::xms::OutputChannelElement::key, (bp::arg("key")), bp::return_internal_reference<>())

              .def("displayedName", &karabo::xms::OutputChannelElement::displayedName, (bp::arg("name")),
                   bp::return_internal_reference<>())

              .def("description", &karabo::xms::OutputChannelElement::description, (bp::arg("description")),
                   bp::return_internal_reference<>())

              .def("dataSchema", &karabo::xms::OutputChannelElement::dataSchema, (bp::arg("schema")),
                   bp::return_internal_reference<>())

              .def("commit", &karabo::xms::OutputChannelElement::commit, bp::return_internal_reference<>())

              ;
    }

    {
        bp::class_<karabo::xms::InputChannel, boost::shared_ptr<karabo::xms::InputChannel>, boost::noncopyable>(
              "InputChannel", bp::no_init)

              .def("reconfigure", &karabo::xms::InputChannel::reconfigure, (bp::arg("configuration")))

              .def("setInstanceId", &karabo::xms::InputChannel::setInstanceId, (bp::arg("instanceId")))

              .def("getInstanceId", &karabo::xms::InputChannel::getInstanceId,
                   bp::return_value_policy<bp::copy_const_reference>())

              .def("registerDataHandler", &karathon::InputChannelWrap().registerDataHandlerPy)

              .def("registerInputHandler", &karathon::InputChannelWrap().registerInputHandlerPy)

              .def("registerEndOfStreamEventHandler", &karathon::InputChannelWrap().registerEndOfStreamEventHandlerPy)

              .def("registerConnectionTracker", &karathon::InputChannelWrap().registerConnectionTrackerPy,
                   "Register a handler to track the status of the connections to\nthe configured output channels.\n"
                   "The handler has two arguments:\n"
                   " * the output channel string like '<deviceId>:<outChannel>'\n"
                   " * the ConnectionStatus")

              .def("getConnectionStatus", &karathon::InputChannelWrap().getConnectionStatusPy)

              .def("getConnectedOutputChannels", &karathon::InputChannelWrap().getConnectedOutputChannelsPy)

              .def("read", &karathon::InputChannelWrap().readPy, (bp::arg("idx")))

              .def("size", &karabo::xms::InputChannel::size)

              .def("getMinimumNumberOfData", &karabo::xms::InputChannel::getMinimumNumberOfData)

              .def("connectSync", &karathon::InputChannelWrap().connectSyncPy, (bp::arg("outputChannelInfo")))

              .def("connect", &karathon::InputChannelWrap().connectPy,
                   (bp::arg("outputChannelInfo"), bp::arg("handler") = bp::object()))

              .def("disconnect", &karathon::InputChannelWrap().disconnectPy, (bp::arg("outputChannelInfo")))

              .def("getMetaData", &karathon::InputChannelWrap().getMetaData)

              .def("create", &karathon::InputChannelWrap::create,
                   (bp::arg("instanceId"), bp::arg("channelName"), bp::arg("channelConfig")))
              .staticmethod("create");
    }

    {
        bp::implicitly_convertible<karabo::util::Schema&, karabo::xms::InputChannelElement>();
        bp::class_<karabo::xms::InputChannelElement>("INPUT_CHANNEL",
                                                     bp::init<karabo::util::Schema&>((bp::arg("expected"))))

              .def("key", &karabo::xms::InputChannelElement::key, (bp::arg("key")), bp::return_internal_reference<>())

              .def("displayedName", &karabo::xms::InputChannelElement::displayedName, (bp::arg("name")),
                   bp::return_internal_reference<>())

              .def("description", &karabo::xms::InputChannelElement::description, (bp::arg("description")),
                   bp::return_internal_reference<>())

              .def("dataSchema", &karabo::xms::InputChannelElement::dataSchema, (bp::arg("schema")),
                   bp::return_internal_reference<>())

              .def("commit", &karabo::xms::InputChannelElement::commit, bp::return_internal_reference<>())

              ;
    }
}
