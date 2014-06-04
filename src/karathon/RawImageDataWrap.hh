/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 * Modified by: <sergey.esenov@xfel.eu>, <burkhard.heisen@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARATHON_RAWIMAGEDATAWRAP_HH
#define	KARATHON_RAWIMAGEDATAWRAP_HH

#include <boost/python.hpp>
#include <boost/function.hpp>
#include <karabo/xip/RawImageData.hh>
#include <karabo/xip/ToChannelSpace.hh>
#include <karabo/xip/FromChannelSpace.hh>
#include "FromNumpy.hh"
#include "ToNumpy.hh"
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
                         const bool copy,
                         const karabo::xip::EncodingType encoding,
                         const karabo::xip::ChannelSpaceType channelSpace,                         
                         const bool isBigEndian) {
            if (!PyByteArray_Check(obj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'bytearray'");
            }
            size_t size = PyByteArray_Size(obj.ptr());
            char* data = PyByteArray_AsString(obj.ptr());
            m_raw = boost::shared_ptr<karabo::xip::RawImageData>(
                    new karabo::xip::RawImageData(data,
                                                  size,
                                                  copy,
                                                  dimensions,
                                                  encoding,
                                                  channelSpace,
                                                  isBigEndian));
        }

        RawImageDataWrap(bp::object& obj,
                         const bool copy,
                         const karabo::xip::EncodingType encoding,
                         const bool isBigEndian) {
                         

            if (!PyArray_Check(obj.ptr())) throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'numpy array'");

            // Data pointer and size
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            char* data = reinterpret_cast<char*> (PyArray_DATA(arr));
            size_t size = PyArray_NBYTES(arr);

            // Dimensions (shape)
            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            std::vector<unsigned long long> tmp(rank);
            for (int i = 0; i < rank; ++i) tmp[rank - i - 1] = shapes[i];
            karabo::util::Dims dimensions;
            dimensions.fromVector(tmp);

            // ChannelSpace
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            karabo::xip::ChannelSpaceType channelSpace = karabo::util::Types::convert<FromNumpy, karabo::xip::ToChannelSpace>(dtype->type_num);
            
            m_raw = karabo::xip::RawImageData::Pointer(new karabo::xip::RawImageData(data,
                                                                                     size,
                                                                                     copy,
                                                                                     dimensions,
                                                                                     encoding,
                                                                                     channelSpace,
                                                                                     isBigEndian));
                                                                                     
        }

        RawImageDataWrap(karabo::util::Hash& imageHash, bool copiesHash = true)
        : m_raw(new karabo::xip::RawImageData(boost::ref(imageHash), copiesHash)) {
        }

        boost::shared_ptr<karabo::xip::RawImageData> getRawImageDataPointer() {
            return m_raw;
        }

        void setDimensions(PyArrayObject* arr) {
            // Dimensions (shape)
            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            std::vector<unsigned long long> tmp(rank);
            for (int i = 0; i < rank; ++i) tmp[i] = shapes[i];
            karabo::util::Dims dims;
            dims.fromVector(tmp);
            m_raw->setDimensions(dims);
        }

        void setChannelSpace(PyArrayObject* arr) {
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            int channelSpace = karabo::util::Types::convert<FromNumpy, karabo::xip::ToChannelSpace>(dtype->type_num);
            m_raw->setChannelSpace(channelSpace);
        }

        void setData(const bp::object& obj, const bool copy = true) {

            if (PyArray_Check(obj.ptr())) {
                PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());

                // Adjust dimensions and channelSpace
                setDimensions(arr);
                setChannelSpace(arr);

                // Data pointer and size          
                size_t size = PyArray_NBYTES(arr);
                char* data = reinterpret_cast<char*> (PyArray_DATA(arr));

                m_raw->setData(data, size, copy);

            } else if (PyByteArray_Check(obj.ptr())) {
                size_t size = PyByteArray_Size(obj.ptr());
                char* data = PyByteArray_AsString(obj.ptr());
                m_raw->setData(data, size, copy);
            } else {
                throw KARABO_PYTHON_EXCEPTION("The argument type in setData is not supported.");
            }
        }

        bp::object getData() {
            std::vector<unsigned long long> dims = m_raw->getDimensions().toVector();
            std::reverse(dims.begin(), dims.end()); // REVERSING
            npy_intp shape[dims.size()];
            for (size_t i = 0; i < dims.size(); ++i) shape[i] = dims[i];
            int npyType = karabo::util::Types::convert<karabo::xip::FromChannelSpace, ToNumpy>(m_raw->getChannelSpace());
            PyObject* pyobj = PyArray_SimpleNew(dims.size(), shape, npyType);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &(m_raw->getData())[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // TODO Check whether a copy is involved here

        }

        size_t getSize() {
            return m_raw->getSize();
        }

        size_t getByteSize() {
            return m_raw->getByteSize();
        }

        void setDimensions(const karabo::util::Dims & dimensions) {
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
            // TODO also support dict here!!
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                const karabo::util::Hash& header = bp::extract<karabo::util::Hash>(obj);
                m_raw->setHeader(header);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setHeader must be Hash");
        }

        const karabo::util::Hash & toHash() {
            return m_raw->hash();
        }

        void swap(RawImageDataWrap & image) {
            boost::shared_ptr<karabo::xip::RawImageData> p = image.getRawImageDataPointer();
            m_raw->swap(*p);
        }

        void swap(karabo::xip::RawImageData & image) {
            m_raw->swap(image);
        }

        void toRGBAPremultiplied() {
            m_raw->toRGBAPremultiplied();
        }
    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

