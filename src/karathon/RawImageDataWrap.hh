/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARATHON_RAWIMAGEDATAWRAP_HH
#define	KARATHON_RAWIMAGEDATAWRAP_HH

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/xip/RawImageData.hh>
#include "Wrapper.hh"

namespace bp = boost::python;

namespace karathon {


    class RawImageDataWrap : public karabo::xip::RawImageData {
    
    public:

        RawImageDataWrap() : karabo::xip::RawImageData() {
        }

        RawImageDataWrap(size_t const byteSize,
                karabo::util::Dims const & dimensions,
                karabo::xip::EncodingType const encoding,
                karabo::xip::ChannelSpaceType const channelSpace,
                karabo::util::Hash const & header = karabo::util::Hash(),
                bool const isBigEndian = karabo::util::isBigEndian())
        : karabo::xip::RawImageData(byteSize, boost::ref(dimensions), encoding, channelSpace, boost::ref(header), isBigEndian) {
        }

        RawImageDataWrap(karabo::util::Hash & imageHash, bool sharesData = false)
        : karabo::xip::RawImageData(boost::ref(imageHash), sharesData) {
        }

        RawImageDataWrap(karabo::xip::RawImageData const & image)
        : karabo::xip::RawImageData(boost::ref(image)) {
        }

        RawImageDataWrap(const bp::object& obj,
                size_t const byteSize,
                karabo::util::Dims const & dimensions,
                karabo::xip::EncodingType const encoding,
                karabo::xip::ChannelSpaceType const channelSpace,
                bp::object const & header = bp::object(karabo::util::Hash()),
                bool const isBigEndian = karabo::util::isBigEndian()) {
            setData(*this, obj, byteSize);
            setDimensions(*this, dimensions);
            setEncoding(*this, encoding);
            setChannelSpace(*this, channelSpace);
            setIsBigEndian(*this, isBigEndian);
            setHeader(*this, header);
        }

        static void setData(karabo::xip::RawImageData& self, const bp::object & obj) {

            if (PyByteArray_Check(obj.ptr())) {
                size_t size = PyByteArray_Size(obj.ptr());
                char* data = PyByteArray_AsString(obj.ptr());
                self.setData<char>(data, size);
                return;
            } 

            if (PyList_Check(obj.ptr())) {
                bp::object list0 = obj[0];
                if (PyInt_Check(list0.ptr())) {
                    std::cout << "CPP IN PyInt_Check(list0.ptr())" << std::endl;
                    std::vector<int> v = karathon::Wrapper::fromPyListToStdVector<int>(obj);
                    self.setData<int>(v);
                    return;
                } else if (PyFloat_Check(list0.ptr())) {
                    std::vector<float> v = karathon::Wrapper::fromPyListToStdVector<float>(obj);
                    self.setData<float>(v);
                    return;
                } else if (PyLong_Check(list0.ptr())) {
                    std::vector<long> v = karathon::Wrapper::fromPyListToStdVector<long>(obj);
                    self.setData<long>(v);
                    return;
                }
            } 

            if (PyString_Check(obj.ptr())) {
                std::vector<char> v = karathon::Wrapper::fromPyListToStdVector<char>(obj);
                self.setData<char>(v);
                return;
            }
            
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setData is byteArray");   
        }

        static void setData(karabo::xip::RawImageData& self, const bp::object & obj, const size_t byteSize) {

            if (PyByteArray_Check(obj.ptr())) {
                char* data = PyByteArray_AsString(obj.ptr());
                //size_t size = PyByteArray_Size(obj.ptr());
                self.setData<char>(data, byteSize);
                return;
            } 
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setData is byteArray");
        }

        static bp::object getData(karabo::xip::RawImageData& self) {
            std::vector<char> ch = self.getData();
            return karathon::Wrapper::fromStdVectorToPyList(ch);
        }

        static void setDimensions(karabo::xip::RawImageData& self, const karabo::util::Dims& dimensions) {
            self.setDimensions(dimensions);
        }

        static void setEncoding(karabo::xip::RawImageData& self, const karabo::xip::Encoding::EncodingType encoding) {
            self.setEncoding(encoding);
        }

        static void setChannelSpace(karabo::xip::RawImageData& self, const karabo::xip::ChannelSpace::ChannelSpaceType channelSpace) {
            self.setChannelSpace(channelSpace);
        }

        static void setHeader(karabo::xip::RawImageData& self, const bp::object & obj) {
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                karabo::util::Hash header = bp::extract<karabo::util::Hash>(obj);
                self.setHeader(header);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setHeader must be Hash");
        }

        static void setIsBigEndian(karabo::xip::RawImageData& self, const bool isBigEndian) {
            self.setIsBigEndian(isBigEndian);
        }

        static bp::object getDimensions(karabo::xip::RawImageData& self) {
            karabo::util::Dims d = self.getDimensions();
            return karathon::Wrapper::fromStdVectorToPyList(d.toVector());
        }

    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

