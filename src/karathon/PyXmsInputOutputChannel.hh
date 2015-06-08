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
#include <karabo/xip.hpp>
#include "PythonFactoryMacros.hh"
#include "ScopedGILRelease.hh"
#include "Wrapper.hh"
#include "DimsWrap.hh"
#include "FromNumpy.hh"
#include "ToNumpy.hh"

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace bp = boost::python;

namespace karathon {


    struct DataWrap {

        static boost::shared_ptr<karabo::xms::Data> make(bp::object& obj);
        static karabo::util::Hash::Pointer getNode(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key);
        static bp::object get(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key);
        static void set(const boost::shared_ptr<karabo::xms::Data>& self, const std::string& key, const bp::object& value);
        static bp::object hash(const boost::shared_ptr<karabo::xms::Data>& self);
        static void attachTimestamp(const boost::shared_ptr<karabo::xms::Data>& self, const bp::object& obj);
        static boost::shared_ptr<karabo::xms::Data> copy(const boost::shared_ptr<karabo::xms::Data>& self);
    };


    struct NDArrayWrap {
        
        static boost::shared_ptr<karabo::xms::NDArray> make2(const bp::object& obj, const bool copy = true);
        static bp::object getDataPy(const boost::shared_ptr<karabo::xms::NDArray>& self);
        static void setDataPy(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& obj, const bool copy);
        static bp::object getDimensionsPy(const boost::shared_ptr<karabo::xms::NDArray>& self);
        static void setDimensionsPy(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& obj);
        static void setDimensionTypesPy(const boost::shared_ptr<karabo::xms::NDArray>& self, const bp::object& obj);
        static bp::object getDimensionTypesPy(const boost::shared_ptr<karabo::xms::NDArray>& self);
    };


    struct NDArrayElementWrap {

        static karabo::xms::NDArrayElement& setDefaultValue(const boost::shared_ptr<karabo::xms::NDArrayElement>& self,
                const std::string& subKey,
                const bp::object& defaultValue);
    };


    struct ImageDataWrap : public karabo::xms::ImageData {

        static boost::shared_ptr<karabo::xms::ImageData> make5(const bp::object& obj, const bool copy = true,
                const karabo::util::Dims& dimensions = karabo::util::Dims(),
                const karabo::xms::EncodingType encoding = karabo::xms::Encoding::UNDEFINED,
                const karabo::xms::ChannelSpaceType channelSpace = karabo::xms::ChannelSpace::UNDEFINED);
        static boost::shared_ptr<karabo::xms::ImageData> make2(const bp::object& obj, const bool copy = true);
        static bp::object getDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self);
        static void setDataPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj, const bool copy);
        static bp::object getDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData>& self);
        static void setDimensionsPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj);
        static void setDimensionTypesPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj);      
        static bp::object getROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self);
        static void setROIOffsetsPy(const boost::shared_ptr<karabo::xms::ImageData>& self, const bp::object& obj);
        static bp::object getEncodingPy(const boost::shared_ptr<karabo::xms::ImageData>& self);
        static bp::object getChannelSpacePy(const boost::shared_ptr<karabo::xms::ImageData>& self);
        static void writePy(const boost::shared_ptr<karabo::xms::ImageData>& self, const std::string& filename, const bool enableAppendMode = false);
    };


    struct ImageDataElementWrap {

        static karabo::xms::ImageDataElement& setDefaultValue(const boost::shared_ptr<karabo::xms::ImageDataElement>& self,
                const std::string& subKey,
                const bp::object& defaultValue);
    };


    struct OutputChannelWrap {

        static void registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& handler);
        static void writePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self, const bp::object& data);
        static void updatePy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
        static void signalEndOfStreamPy(const boost::shared_ptr<karabo::xms::OutputChannel>& self);
    };


    struct InputChannelWrap {

        static void registerIOEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static void registerEndOfStreamEventHandlerPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const bp::object& handler);
        static bp::object getConnectedOutputChannelsPy(const boost::shared_ptr<karabo::xms::InputChannel>& self);
        static bp::object readPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, size_t idx);
        static void connectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static void disconnectPy(const boost::shared_ptr<karabo::xms::InputChannel>& self, const karabo::util::Hash& outputChannelInfo);
        static void updatePy(const boost::shared_ptr<karabo::xms::InputChannel>& self);
    };

}


#endif	/* KARATHON_PYXMSINPUTOUTPUTCHANNEL_HH */

