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
#ifdef WITH_BOOST_NUMPY
#include <boost/numpy.hpp>
namespace bn = boost::numpy;
#endif

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

            #ifdef WITH_BOOST_NUMPY
            if (bp::extract<bn::ndarray>(obj).check()) {
                const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
                int nd = a.get_nd();
                Py_intptr_t const * shapes = a.get_shape();
                int nelems = 1;
                for (int i = 0; i < nd; i++) nelems *= shapes[i];

                if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                    double* data = reinterpret_cast<double*> (a.get_data());
                    std::vector<double> v(data, data + nelems);
                    self.setData<double>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                    float* data = reinterpret_cast<float*> (a.get_data());
                    std::vector<float> v(data, data + nelems);
                    self.setData<float>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                    short* data = reinterpret_cast<short*> (a.get_data());
                    std::vector<short> v(data, data + nelems);
                    self.setData<short>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                    unsigned short* data = reinterpret_cast<unsigned short*> (a.get_data());
                    std::vector<unsigned short> v(data, data + nelems);
                    self.setData<unsigned short>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                    int* data = reinterpret_cast<int*> (a.get_data());
                    std::vector<int> v(data, data + nelems);
                    self.setData<int>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                    unsigned int* data = reinterpret_cast<unsigned int*> (a.get_data());
                    std::vector<unsigned int> v(data, data + nelems);
                    self.setData<unsigned int>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                    long long* data = reinterpret_cast<long long*> (a.get_data());
                    std::vector<long long> v(data, data + nelems);
                    self.setData<long long>(v);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (a.get_data());
                    std::vector<unsigned long long> v(data, data + nelems);
                    self.setData<unsigned long long>(v);
                    return;
                }
            }
            #endif       

            throw KARABO_PYTHON_EXCEPTION("Python type of the argument given in setData cannot be recognized");
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

