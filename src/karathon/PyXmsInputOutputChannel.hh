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
        static bp::object getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& obj);
        static bp::object getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& geometry);
        static karabo::util::DetectorGeometry getGeometryPy(const boost::shared_ptr<karabo::xms::ImageData >& self);
        static void setHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self, const bp::object& header);
        static const karabo::util::Hash& getHeaderPy(const boost::shared_ptr<karabo::xms::ImageData >& self);

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
        static void writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data, const bp::object& meta);
        static void updatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
        static void signalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
    };

    struct InputChannelWrap {

        static void registerDataHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyDataHandler(const bp::object& handler, const karabo::util::Hash& data, const karabo::xms::InputChannel::MetaData& meta);
        static void registerInputHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyInputHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& self);
        static void registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void proxyEndOfStreamEventHandler(const bp::object& handler, const karabo::xms::InputChannel::Pointer& self);
        static bp::object getConnectedOutputChannelsPy(const boost::shared_ptr<karabo::xms::InputChannel>& self);
        static bp::object readPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx);
        static void connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static void disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static bp::object getMetaData(const boost::shared_ptr<karabo::xms::InputChannel>& self);
    };

}


#endif	/* KARATHON_PYXMSINPUTOUTPUTCHANNEL_HH */

