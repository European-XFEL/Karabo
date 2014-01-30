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

    class RawImageDataWrap {

        boost::shared_ptr<karabo::xip::RawImageData> m_raw;

    public:

        RawImageDataWrap() : m_raw(new karabo::xip::RawImageData()) {
        }

        RawImageDataWrap(karabo::xip::RawImageData& other) : m_raw(new karabo::xip::RawImageData()) {
            m_raw->swap(other);
        }
        
        RawImageDataWrap(bp::object& obj,
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

        RawImageDataWrap(bp::object& obj,
                         const karabo::xip::EncodingType encoding,
                         const karabo::util::Hash& header, const bool isBigEndian) {
            if (!PyArray_Check(obj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'numpy array'");
            }
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
            int nd = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
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
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            if (dtype->type_num == NPY_BYTE) {
                char* data = reinterpret_cast<char*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_8_1;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (char),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_UBYTE) {
                unsigned char* data = reinterpret_cast<unsigned char*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_8_1;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned char),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_SHORT) {
                short* data = reinterpret_cast<short*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_16_2;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (short),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_USHORT) {
                unsigned short* data = reinterpret_cast<unsigned short*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_16_2;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned short),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_INT) {
                int* data = reinterpret_cast<int*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (int),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_UINT) {
                unsigned int* data = reinterpret_cast<unsigned int*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (unsigned int),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_LONG) {
                if (sizeof(long) == sizeof(int)) {                      // 32 bit CPU
                int* data = reinterpret_cast<int*> (PyArray_DATA(arr));
                    karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_32_4;
                    m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                            new karabo::xip::RawImageData(data,
                                                          nelems * sizeof (int),
                                                          boost::ref(dimensions),
                                                          encoding,
                                                          channelSpace,
                                                          boost::ref(header),
                                                          isBigEndian));
                } else {                                                // 64 bit CPU
                    long long* data = reinterpret_cast<long long*> (PyArray_DATA(arr));
                    karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_64_8;
                    m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                            new karabo::xip::RawImageData(data,
                                                          nelems * sizeof (long long),
                                                          boost::ref(dimensions),
                                                          encoding,
                                                          channelSpace,
                                                          boost::ref(header),
                                                          isBigEndian));
                }
            } else if (dtype->type_num == NPY_ULONG) {
                if (sizeof(unsigned long) == sizeof(unsigned int)) {   // 32 bit CPU
                    unsigned int* data = reinterpret_cast<unsigned int*> (PyArray_DATA(arr));
                    karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_32_4;
                    m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                            new karabo::xip::RawImageData(data,
                                                          nelems * sizeof (unsigned int),
                                                          boost::ref(dimensions),
                                                          encoding,
                                                          channelSpace,
                                                          boost::ref(header),
                                                          isBigEndian));
                } else {                                                // 64 bit CPU
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (PyArray_DATA(arr));
                    karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::u_64_8;
                    m_raw = boost::shared_ptr<karabo::xip::RawImageData>(   
                            new karabo::xip::RawImageData(data,
                                                          nelems * sizeof (unsigned long long),
                                                          boost::ref(dimensions),
                                                          encoding,
                                                          channelSpace,
                                                          boost::ref(header),
                                                          isBigEndian));
                }
            } else if (dtype->type_num == NPY_FLOAT) {
                float* data = reinterpret_cast<float*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::f_32_4;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (float),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_DOUBLE) {
                double* data = reinterpret_cast<double*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::f_64_8;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (double),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_LONGLONG) {
                long long* data = reinterpret_cast<long long*> (PyArray_DATA(arr));
                karabo::xip::ChannelSpace::ChannelSpaceType channelSpace = karabo::xip::ChannelSpace::s_64_8;
                m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                        new karabo::xip::RawImageData(data,
                                                      nelems * sizeof (long long),
                                                      boost::ref(dimensions),
                                                      encoding,
                                                      channelSpace,
                                                      boost::ref(header),
                                                      isBigEndian));
            } else if (dtype->type_num == NPY_ULONGLONG) {
                unsigned long long* data = reinterpret_cast<unsigned long long*> (PyArray_DATA(arr));
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
            if (PyArray_Check(obj.ptr())) {
                PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
                int nd = PyArray_NDIM(arr);
                npy_intp* shapes = PyArray_DIMS(arr);
                int nelems = 1;
                for (int i = 0; i < nd; i++) nelems *= shapes[i];
                PyArray_Descr* dtype = PyArray_DESCR(arr);
                if (dtype->type_num == NPY_BYTE) {
                    const char* data = reinterpret_cast<const char*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems);
                    return;
                }
                if (dtype->type_num == NPY_UBYTE) {
                    const unsigned char* data = reinterpret_cast<const unsigned char*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems);
                    return;
                }
                if (dtype->type_num == NPY_SHORT) {
                    short* data = reinterpret_cast<short*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (short));
                    return;
                }
                if (dtype->type_num == NPY_USHORT) {
                    const unsigned short* data = reinterpret_cast<const unsigned short*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (unsigned short));
                    return;
                }
                if (dtype->type_num == NPY_INT) {
                    const int* data = reinterpret_cast<const int*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (int));
                    return;
                }
                if (dtype->type_num == NPY_UINT) {
                    const unsigned int* data = reinterpret_cast<const unsigned int*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (unsigned int));
                    return;
                }
                if (dtype->type_num == NPY_FLOAT) {
                    const float* data = reinterpret_cast<const float*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (float));
                    return;
                }
                if (dtype->type_num == NPY_DOUBLE) {
                    const double* data = reinterpret_cast<const double*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (double));
                    return;
                }
                if (dtype->type_num == NPY_LONGLONG) {
                    const long long* data = reinterpret_cast<const long long*> (PyArray_DATA(arr));
                    m_raw->setData(data, nelems * sizeof (long long));
                    return;
                }
                if (dtype->type_num == NPY_ULONGLONG) {
                    const unsigned long long* data = reinterpret_cast<const unsigned long long*> (PyArray_DATA(arr));
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
        
        void swap(karabo::xip::RawImageData& image) {
            m_raw->swap(image);
        }
        
        void toRGBAPremultiplied() {
            m_raw->toRGBAPremultiplied();
        }
    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

