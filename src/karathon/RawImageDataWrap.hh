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
#include <boost/numpy.hpp>
#include <boost/function.hpp>
#include <karabo/xip/RawImageData.hh>
#include <boost/numpy.hpp>
#include "Wrapper.hh"

namespace bn = boost::numpy;
namespace bp = boost::python;
namespace bn = boost::numpy;

namespace karathon {

    class RawImageDataWrap {

        boost::shared_ptr<karabo::xip::RawImageData> m_raw;

    public:

        RawImageDataWrap() : m_raw(new karabo::xip::RawImageData()) {
        }

        RawImageDataWrap(const bp::object& obj,
                         const karabo::util::Dims& dimensions,
                         const karabo::xip::EncodingType encoding,
                         const karabo::xip::ChannelSpaceType channelSpace,
                         const karabo::util::Hash& header, const bool isBigEndian) {
            if (!PyByteArray_Check(obj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'bytearray'");
            }
            size_t size = PyByteArray_Size(obj.ptr());
            char* data = PyByteArray_AsString(obj.ptr());
            m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                    new karabo::xip::RawImageData(data, size, boost::ref(dimensions),
                                                  encoding, channelSpace,
                                                  boost::ref(header), isBigEndian));
        }

        RawImageDataWrap(const bp::object& obj,
                         const karabo::xip::EncodingType encoding,
                         const karabo::util::Hash& header, const bool isBigEndian) {
            if (!bp::extract<bn::ndarray>(obj).check()) {
                throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'numpy array'");
            }
            const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
            int nd = a.get_nd();
            Py_intptr_t const * shapes = a.get_shape();
            unsigned long long x, y, z;
            if (nd >= 3) {
                x = shapes[0];
                y = shapes[1];
                z = shapes[2];
            } else if (nd == 2) {
                x = shapes[0];
                y = shapes[1];
                z = 1;
            } else if (nd == 1) {
                x = shapes[0];
                y = 1;
                z = 1;
            } else {
                x = 1;
                y = 1;
                z = 1;
            }
            karabo::util::Dims dimensions(x, y, z);
            int nelems = 1;
            for (int i = 0; i < nd; i++) nelems *= shapes[i];
            if (a.get_dtype() == bn::dtype::get_builtin<char>()) {
                char* data = reinterpret_cast<char*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_8_1;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (char),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned char>()) {
                unsigned char* data = reinterpret_cast<unsigned char*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_8_1;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned char),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                short* data = reinterpret_cast<short*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_16_2;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (short),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                unsigned short* data = reinterpret_cast<unsigned short*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_16_2;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned short),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                int* data = reinterpret_cast<int*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (int),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                unsigned int* data = reinterpret_cast<unsigned int*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned int),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                float* data = reinterpret_cast<float*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::f_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (float),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                double* data = reinterpret_cast<double*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::f_64_8;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (double),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                long long* data = reinterpret_cast<long long*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_64_8;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (long long),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                unsigned long long* data = reinterpret_cast<unsigned long long*> (a.get_data());
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_64_8;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned long long),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else {
                throw KARABO_PYTHON_EXCEPTION("Unsupported numpy array type.");
            }
        }

        RawImageDataWrap(karabo::util::Hash & imageHash, bool sharesData = false)
        : m_raw(new karabo::xip::RawImageData(boost::ref(imageHash), sharesData)) {
        }

        //        RawImageDataWrap(karabo::xip::RawImageData const & image)
        //        : m_raw(new karabo::xip::RawImageData(boost::ref(image))) {
        //        }

        boost::shared_ptr<karabo::xip::RawImageData> getRawImageDataPointer() {
            return m_raw;
        }

        void allocateData(const size_t byteSize) {
            m_raw->allocateData(byteSize);
        }

        void setData(const bp::object & obj) {
            if (bp::extract<bn::ndarray>(obj).check()) {
                const bn::ndarray& a = bp::extract<bn::ndarray>(obj);
                int nd = a.get_nd();
                Py_intptr_t const * shapes = a.get_shape();
                int nelems = 1;
                for (int i = 0; i < nd; i++) nelems *= shapes[i];
                if (a.get_dtype() == bn::dtype::get_builtin<char>()) {
                    const char* data = reinterpret_cast<const char*> (a.get_data());
                    m_raw->setData(data, nelems);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned char>()) {
                    const unsigned char* data = reinterpret_cast<const unsigned char*> (a.get_data());
                    m_raw->setData(data, nelems);
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<short>()) {
                    short* data = reinterpret_cast<short*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (short));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned short>()) {
                    const unsigned short* data = reinterpret_cast<const unsigned short*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (unsigned short));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<int>()) {
                    const int* data = reinterpret_cast<const int*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (int));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned int>()) {
                    const unsigned int* data = reinterpret_cast<const unsigned int*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (unsigned int));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<float>()) {
                    const float* data = reinterpret_cast<const float*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (float));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<double>()) {
                    const double* data = reinterpret_cast<const double*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (double));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<long long>()) {
                    const long long* data = reinterpret_cast<const long long*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (long long));
                    return;
                }
                if (a.get_dtype() == bn::dtype::get_builtin<unsigned long long>()) {
                    const unsigned long long* data = reinterpret_cast<const unsigned long long*> (a.get_data());
                    m_raw->setData(data, nelems * sizeof (unsigned long long));
                    return;
                }
                throw KARABO_PYTHON_EXCEPTION("This ndarray type is not supported");
            }
            if (PyByteArray_Check(obj.ptr())) {
                size_t size = PyByteArray_Size(obj.ptr());
                char* data = PyByteArray_AsString(obj.ptr());
                m_raw->setData(data, size);

                throw KARABO_PYTHON_EXCEPTION("The argument type in setData is not supported.");
            }
        }

        bp::object getData() {
            return karathon::Wrapper::toObject(m_raw->getData());
        }

        size_t size() {
            return m_raw->size();
        }

        void setByteSize(const size_t& byteSize) {
            m_raw->setByteSize(byteSize);
        }

        size_t getByteSize() {
            return m_raw->getByteSize();
        }

        void setDimensions(const karabo::util::Dims& dimensions) {
            m_raw->setDimensions(dimensions);
        }

        bp::object getDimensions() {
            karabo::util::Dims d = m_raw->getDimensions();
            return karathon::Wrapper::fromStdVectorToPyList(d.toVector());
        }

        void setEncoding(const karabo::xip::Encoding::EncodingType encoding) {
            m_raw->setEncoding(encoding);
        }

        int getEncoding() {
            return m_raw->getEncoding();
        }

        void setChannelSpace(const karabo::xip::ChannelSpace::ChannelSpaceType channelSpace) {
            m_raw->setChannelSpace(channelSpace);
        }

        int getChannelSpace() {
            return m_raw->getChannelSpace();
        }

        void setIsBigEndian(const bool isBigEndian) {
            m_raw->setIsBigEndian(isBigEndian);
        }

        bool isBigEndian() {
            return m_raw->isBigEndian();
        }

        bp::object getHeader() {
            return bp::object(m_raw->getHeader());
        }

        void setHeader(const bp::object & obj) {
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                const karabo::util::Hash& header = bp::extract<karabo::util::Hash>(obj);
                m_raw->setHeader(header);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setHeader must be Hash");
        }

        const karabo::util::Hash& toHash() {
            return m_raw->toHash();
        }

        void swap(RawImageDataWrap& image) {
            boost::shared_ptr<karabo::xip::RawImageData> p = image.getRawImageDataPointer();
            m_raw->swap(*p);
        }
    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

