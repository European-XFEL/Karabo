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

    class RawImageDataWrap : public karabo::xip::RawImageData {

        //boost::shared_ptr<karabo::xip::RawImageData> m_raw;

    public:

        RawImageDataWrap() : karabo::xip::RawImageData() {
        }

        //        RawImageDataWrap(karabo::xip::RawImageData& other) : m_raw(new karabo::xip::RawImageData()) {
        //            m_raw->swap(other);
        //        }

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

            setData(data, size, copy);
            if (dimensions.size() == 0) setDimensions(karabo::util::Dims(size));
            else setDimensions(dimensions);
            setEncoding(encoding);
            if (channelSpace == karabo::xip::ChannelSpace::UNDEFINED) setChannelSpace(channelSpace);
            else setChannelSpace(channelSpace);

            // Need to fix the data type here
            m_hash.set("type", karabo::util::Types::convert<karabo::xip::FromChannelSpace, karabo::util::ToLiteral>(channelSpace));

            setIsBigEndian(isBigEndian);
        }

        RawImageDataWrap(bp::object& obj,
                         const bool copy = true,
                         const karabo::xip::EncodingType encoding = karabo::xip::Encoding::GRAY,
                         const bool isBigEndian = karabo::util::isBigEndian()) {


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

            setData(data, size, copy);

            // We need to fix the type here
            m_hash.set("type", karabo::util::Types::convert<FromNumpy, karabo::util::ToLiteral>(dtype->type_num));

            if (dimensions.size() == 0) setDimensions(karabo::util::Dims(size));
            else setDimensions(dimensions);
            setEncoding(encoding);
            if (channelSpace == karabo::xip::ChannelSpace::UNDEFINED) setChannelSpace(channelSpace);
            else setChannelSpace(channelSpace);
            setIsBigEndian(isBigEndian);

        }

        static boost::shared_ptr<RawImageData> make(bp::object& obj,
                                                    const bool copy = true,
                                                    const karabo::xip::EncodingType encoding = karabo::xip::Encoding::GRAY,
                                                    const bool isBigEndian = karabo::util::isBigEndian()) {
            return boost::shared_ptr<RawImageDataWrap>(new RawImageDataWrap(obj, copy, encoding, isBigEndian));
        }

        RawImageDataWrap(karabo::util::Hash& hash, bool copiesHash = true) : karabo::xip::RawImageData(hash, copiesHash) {
        }

        static void setDimensionsPy(RawImageData& self, PyArrayObject* arr) {
            // Dimensions (shape)
            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            std::vector<unsigned long long> tmp(rank);
            for (int i = 0; i < rank; ++i) tmp[i] = shapes[i];
            karabo::util::Dims dims;
            dims.fromVector(tmp);
            self.setDimensions(dims);
        }

        static void setChannelSpacePy(RawImageData& self, PyArrayObject* arr) {
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            int channelSpace = karabo::util::Types::convert<FromNumpy, karabo::xip::ToChannelSpace>(dtype->type_num);
            self.setChannelSpace(channelSpace);
        }

        static void setDataPy(RawImageData& self, const bp::object& obj, const bool copy = true) {

            if (PyArray_Check(obj.ptr())) {
                PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());

                // Adjust dimensions and channelSpace
                RawImageDataWrap::setDimensionsPy(self, arr);
                RawImageDataWrap::setChannelSpacePy(self, arr);

                // Data pointer and size          
                size_t size = PyArray_NBYTES(arr);
                char* data = reinterpret_cast<char*> (PyArray_DATA(arr));

                self.setData(data, size, copy);

            } else if (PyByteArray_Check(obj.ptr())) {
                size_t size = PyByteArray_Size(obj.ptr());
                char* data = PyByteArray_AsString(obj.ptr());
                self.setData(data, size, copy);
            } else {
                throw KARABO_PYTHON_EXCEPTION("The argument type in setData is not supported.");
            }
        }

        static bp::object getDataPy(RawImageData& self) {
            std::vector<unsigned long long> dims = self.getDimensions().toVector();
            std::reverse(dims.begin(), dims.end()); // REVERSING
            npy_intp shape[dims.size()];
            for (size_t i = 0; i < dims.size(); ++i) shape[i] = dims[i];
            int npyType = karabo::util::Types::convert<karabo::xip::FromChannelSpace, ToNumpy>(self.getChannelSpace());
            PyObject* pyobj = PyArray_SimpleNew(dims.size(), shape, npyType);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &(self.getData())[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // TODO Check whether a copy is involved here

        }

        static bp::object getDimensionsPy(const RawImageData& self) {
            karabo::util::Dims d = self.getDimensions();
            return karathon::Wrapper::fromStdVectorToPyList(d.toVector());
        }

        //        bp::object getHeader() {
        //            return bp::object(getHeader());
        //        }

        static void setHeaderPy(RawImageData& self, const bp::object & obj) {
            // TODO also support dict here!!
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                const karabo::util::Hash& header = bp::extract<karabo::util::Hash>(obj);
                self.setHeader(header);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setHeader must be Hash");
        }
    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

