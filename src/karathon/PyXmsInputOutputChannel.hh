/*
 * File:   PyXmsInputOutputChannel.hh
 * Author: Sergey Esenov <serguei.essenov at xfel.eu>
 *
 * Created on June 8, 2015, 5:38 PM
 */

#ifndef KARATHON_PYXMSINPUTOUTPUTCHANNEL_HH
#define	KARATHON_PYXMSINPUTOUTPUTCHANNEL_HH

#include <boost/python.hpp>
#include <karabo/xms.hpp>
#include "PythonFactoryMacros.hh"
#include "ScopedGILRelease.hh"
#include "ScopedGILAcquire.hh"
#include "Wrapper.hh"
#include "DimsWrap.hh"
#include "FromNumpy.hh"
#include "ToNumpy.hh"
#include <karabo/util/DetectorGeometry.hh>
#include <karabo/util/NDArray.hh>

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace bp = boost::python;

namespace karathon {

    struct ImageDataWrap : public karabo::xms::ImageData {

        static boost::shared_ptr<karabo::xms::ImageData > make5(const bp::object& obj,
                                                                const karabo::util::Dims& dimensions = karabo::util::Dims(),
                                                                const karabo::xms::EncodingType encoding = karabo::xms::Encoding::UNDEFINED,
                                                                const int bitsPerPixel = 0);
        static bp::object getDataPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setDataPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getTypePy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setTypePy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getBinningPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setBinningPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& geometry);
        static karabo::util::DetectorGeometry getGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& header);
        static const karabo::util::Hash& getHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self);

    };

    struct ImageDataElementWrap {

        static karabo::xms::ImageDataElement& setType(karabo::xms::ImageDataElement& self, const bp::object& obj) {
            using namespace karabo::util;
            Types::ReferenceType reftype = Types::UNKNOWN;

            // If data type was given as string
            if (bp::extract<std::string>(obj).check()) {
                std::string type = bp::extract<std::string>(obj);
                reftype = Types::from<FromLiteral>(type);
            // If data type was given as Type
            } else if (bp::extract<karathon::PyTypes::ReferenceType>(obj).check()) {
                karathon::PyTypes::ReferenceType type = bp::extract<karathon::PyTypes::ReferenceType>(obj);
                reftype = karathon::PyTypes::to(type);
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python type of setType() of ImageDataElement must be a string or Types enumerated value.");
            }
            return self.setType(reftype);
        }

        static karabo::xms::ImageDataElement& setDimensions(karabo::xms::ImageDataElement& self, const bp::object& obj) {
            std::string dimStr("{0,0}");

            // If image dimensions were given as string
            if (PyUnicode_Check(obj.ptr())) {
                dimStr = bp::extract<std::string>(obj);
            // If image dimensions were given as list
            } else if (PyList_Check(obj.ptr())) {
                const std::vector<long long> v = karathon::Wrapper::fromPyListToStdVector<long long>(obj);
                dimStr = karabo::util::toString<long long>(v);
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python type of setDimensions() of ImageDataElement must be a list or a string");
            }
            return self.setDimensions(dimStr);
        }

        static karabo::xms::ImageDataElement& setEncoding(karabo::xms::ImageDataElement& self, const bp::object& obj) {
            using namespace karabo::xms;
            EncodingType encType = EncodingType::UNDEFINED;
            
            // If image encoding type is given as string
            if (PyUnicode_Check(obj.ptr())) {
                // Create look-up table to translate string-keys to EncodingType
                std::map<std::string, EncodingType> encLuT;
                encLuT["UNDEFINED"] = EncodingType::UNDEFINED;
                encLuT["GRAY"] = EncodingType::GRAY;
                encLuT["RGB"] = EncodingType::RGB;
                encLuT["RGBA"] = EncodingType::RGBA;
                encLuT["BGR"] = EncodingType::BGR;
                encLuT["BGRA"] = EncodingType::BGRA;
                encLuT["CMYK"] = EncodingType::CMYK;
                encLuT["YUV"] = EncodingType::YUV;
                encLuT["BAYER"] = EncodingType::BAYER;
                encLuT["JPEG"] = EncodingType::JPEG;
                encLuT["PNG"] = EncodingType::PNG;
                encLuT["BMP"] = EncodingType::BMP;
                encLuT["TIFF"] = EncodingType::TIFF;
                // Look up the supplied key in the LUT
                encType = encLuT[bp::extract<std::string>(obj)];
                // If data type was given as integer
            } else if (bp::extract<EncodingType>(obj).check()) {
                encType = bp::extract<EncodingType>(obj);
            } else {
                throw KARABO_PYTHON_EXCEPTION("Python type of setEncoding() of ImageDataElement must be an unsigned integer or string");
            }
            return self.setEncoding(encType);
        }

        static karabo::xms::ImageDataElement& setAllowedActions(karabo::xms::ImageDataElement& self,
                                                                const bp::object& actions) {
            // Accept any Python iterable that provides strings
            return self.setAllowedActions(karathon::Wrapper::fromPyIterableToCppContainer<std::string>(actions));

        }
    };

    class ChannelMetaData : public karabo::xms::Memory::MetaData {
    public:
        ChannelMetaData(const bp::object& src, const bp::object& ts);
        void setSource(const bp::object& src);
        bp::object getSource();
        void setTimestamp(const bp::object& ts);
        bp::object getTimestamp();
    };

    struct OutputChannelWrap {

        static void registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler);
        static void proxyIOEventHandler(const bp::object& handler, const boost::shared_ptr<karabo::xms::OutputChannel>& self);
        static void writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data, const bp::object& meta, bool copyAllData);
        static void updatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
        static void signalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
        static void registerShowConnectionsHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler);
        static void proxyShowConnectionsHandler(const bp::object& handler, const std::vector<karabo::util::Hash>& connections);
    };

    struct InputChannelWrap {

        static void registerDataHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyDataHandler(const bp::object& handler, const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta);
        static void registerInputHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyInputHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& self);
        static void registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyEndOfStreamEventHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& self);
        static void registerConnectionTrackerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyChannelStatusTracker(const bp::object& handler, const std::string& name, karabo::net::ConnectionStatus status);
        static bp::object getConnectedOutputChannelsPy(const boost::shared_ptr<karabo::xms::InputChannel>& self);
        static bp::object readPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx);
        static void connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static void disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static bp::object getMetaData(const boost::shared_ptr<karabo::xms::InputChannel>& self);
    };

}


#endif	/* KARATHON_PYXMSINPUTOUTPUTCHANNEL_HH */

