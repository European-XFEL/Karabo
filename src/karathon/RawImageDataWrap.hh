/*
 * $Id$
 *
 * Author: <irina.kozlova@xfel.eu>
 * Modified by: <sergey.esenov@xfel.eu>, <burkhard.heisen@xfel.eu>,
 *              <andrea.parenti@xfel.eu>
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
                         const bool copy,
                         const karabo::util::Dims& dimensions,
                         const karabo::xip::EncodingType encoding,
                         const karabo::xip::ChannelSpaceType channelSpace,
                         const bool isBigEndian) {
            if (!PyByteArray_Check(obj.ptr())) {
                throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'bytearray'");
            }
            size_t size = PyByteArray_Size(obj.ptr());
            char* data = PyByteArray_AsString(obj.ptr());

            setData(data, size, copy);
            if (dimensions.size() == 0) {
                setDimensions(karabo::util::Dims(size));
                setROIOffsets(karabo::util::Dims(0));
            } else {
                setDimensions(dimensions);
                std::vector<unsigned long long> offsets(dimensions.rank(), 0);
                setROIOffsets(karabo::util::Dims(offsets));
            }
            setEncoding(encoding);
            if (channelSpace == karabo::xip::ChannelSpace::UNDEFINED) setChannelSpace(channelSpace);
            else setChannelSpace(channelSpace);

            // Need to fix the data type here
            m_hash.set("dataType", karabo::util::Types::convert<karabo::xip::FromChannelSpace, karabo::util::ToLiteral>(channelSpace));

            setIsBigEndian(isBigEndian);
        }

        RawImageDataWrap(bp::object& obj,
                         const bool copy = true,
                         const karabo::util::Dims& dimensions = karabo::util::Dims(),
                         const karabo::xip::EncodingType encoding = karabo::xip::Encoding::UNDEFINED,
                         const bool isBigEndian = karabo::util::isBigEndian()) {


            if (!PyArray_Check(obj.ptr())) throw KARABO_PYTHON_EXCEPTION("The 1st argument python type must be 'numpy array'");

            // Get a contiguous copy of the array (or just a reference if already contiguous)
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            PyArrayObject* carr = PyArray_GETCONTIGUOUS(arr);
            // Data pointer and size
            char* data = reinterpret_cast<char*> (PyArray_DATA(carr));
            size_t size = PyArray_NBYTES(arr);
            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);

            // Encoding
            karabo::xip::EncodingType _encoding = encoding;
            if (encoding == karabo::xip::Encoding::UNDEFINED) {
                // No encoding info -> try to guess it from ndarray shape
                if (rank == 2)
                    _encoding = karabo::xip::Encoding::GRAY;
                else if (rank == 3 && shapes[2] == 3)
                    _encoding = karabo::xip::Encoding::RGB;
                else if (rank == 3 && shapes[2] == 4)
                    _encoding = karabo::xip::Encoding::RGBA;
            }

            // Dimensions (shape)
            std::vector<unsigned long long> tmp(rank);
            karabo::util::Dims _dimensions;

            if (_encoding == karabo::xip::Encoding::RGB || _encoding == karabo::xip::Encoding::RGBA ||
                _encoding == karabo::xip::Encoding::BGR || _encoding == karabo::xip::Encoding::BGRA ||
                _encoding == karabo::xip::Encoding::CMYK || _encoding == karabo::xip::Encoding::YUV) {
                // Color images -> use ndarray dimensions

                if (rank != 3) throw KARABO_PYTHON_EXCEPTION("The 'numpy array' has the wrong number of dimensions");

                tmp[2] = shapes[2]; // Number of channels
                tmp[1] = shapes[0]; // Image height
                tmp[0] = shapes[1]; // Image width

                _dimensions.fromVector(tmp);
            } else if (_encoding == karabo::xip::Encoding::GRAY) {
                // Gray-scale images -> use ndarray dimensions

                if ((rank != 2) && !((rank == 3) && (tmp[2] == 1)))
                    throw KARABO_PYTHON_EXCEPTION("The 'numpy array' has the wrong number of dimensions");

                for (int i = 0; i < rank; ++i) tmp[rank - i - 1] = shapes[i];

                _dimensions.fromVector(tmp);
            } else if (_encoding == karabo::xip::Encoding::JPEG || _encoding == karabo::xip::Encoding::PNG ||
                       _encoding == karabo::xip::Encoding::BMP || _encoding == karabo::xip::Encoding::TIFF) {
                // JPEG, PNG, BMP, TIFF -> cannot use ndarray dimensions, use therefore input parameter

                _dimensions = dimensions;
            } else {
                // Other encodings. Likely it will need to be fixed!
                // getDataPy(RawImageData&) will need to be changed accordingly!!!
                for (int i = 0; i < rank; ++i) tmp[rank - i - 1] = shapes[i];

                _dimensions.fromVector(tmp);
            }

            // ChannelSpace
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            karabo::xip::ChannelSpaceType channelSpace = karabo::util::Types::convert<FromNumpy, karabo::xip::ToChannelSpace>(dtype->type_num);

            setData(data, size, copy);

            // We need to fix the type here
            m_hash.set("dataType", karabo::util::Types::convert<FromNumpy, karabo::util::ToLiteral>(dtype->type_num));

            if (_dimensions.size() == 0) {
                setDimensions(karabo::util::Dims(size));
                setROIOffsets(karabo::util::Dims(0));
            } else {
                setDimensions(_dimensions);
                std::vector<unsigned long long> offsets(_dimensions.rank(), 0);
                setROIOffsets(karabo::util::Dims(offsets));
            }
            setEncoding(_encoding);
            if (channelSpace == karabo::xip::ChannelSpace::UNDEFINED) setChannelSpace(channelSpace);
            else setChannelSpace(channelSpace);
            setIsBigEndian(isBigEndian);

        }

        static boost::shared_ptr<RawImageData> make(bp::object& obj,
                                                    const bool copy = true,
                                                    const karabo::util::Dims& dimensions = karabo::util::Dims(),
                                                    const karabo::xip::EncodingType encoding = karabo::xip::Encoding::UNDEFINED,
                                                    const bool isBigEndian = karabo::util::isBigEndian()) {
            return boost::shared_ptr<RawImageDataWrap>(new RawImageDataWrap(obj, copy, dimensions, encoding, isBigEndian));
        }

        RawImageDataWrap(karabo::util::Hash& hash, bool copiesHash = true) : karabo::xip::RawImageData(hash, copiesHash) {
        }

        static void _setDimensionsPy(RawImageData& self, PyArrayObject* arr) {
            // Dimensions (shape)
            int rank = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            std::vector<unsigned long long> tmp(rank);
            for (int i = 0; i < rank; ++i) tmp[i] = shapes[i];
            karabo::util::Dims dims;
            dims.fromVector(tmp);
            self.setDimensions(dims);
        }

        static void _setChannelSpacePy(RawImageData& self, PyArrayObject* arr) {
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            int channelSpace = karabo::util::Types::convert<FromNumpy, karabo::xip::ToChannelSpace>(dtype->type_num);
            self.setChannelSpace(channelSpace);
        }

        static void setROIOffsetsPy(RawImageData& self, const bp::object& offsets) {
            std::vector<unsigned long long> vec = Wrapper::fromPyListToStdVector<unsigned long long>(offsets);
            karabo::util::Dims dims;
            dims.fromVector(vec);
            self.setROIOffsets(dims);
        }

        static void setDataPy(RawImageData& self, const bp::object& obj, const bool copy = true) {

            if (PyArray_Check(obj.ptr())) {
                PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());

                // Adjust dimensions and channelSpace
                RawImageDataWrap::_setDimensionsPy(self, arr);
                RawImageDataWrap::_setChannelSpacePy(self, arr);

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
            npy_intp shape[dims.size()];

            int encoding = self.getEncoding();
            if (encoding == karabo::xip::Encoding::RGB || encoding == karabo::xip::Encoding::RGBA ||
                encoding == karabo::xip::Encoding::BGR || encoding == karabo::xip::Encoding::BGRA ||
                encoding == karabo::xip::Encoding::CMYK || encoding == karabo::xip::Encoding::YUV) {
                // Color images

                if (dims.size() != 3) throw KARABO_PYTHON_EXCEPTION("The 'RawImageData' has the wrong number of dimensions");

                shape[2] = dims[2]; // Number of channels
                shape[0] = dims[1]; // Image height
                shape[1] = dims[0]; // Image width
            } else if (encoding == karabo::xip::Encoding::GRAY) {
                // Gray-scale images

                std::reverse(dims.begin(), dims.end()); // REVERSING
                for (size_t i = 0; i < dims.size(); ++i) shape[i] = dims[i];
            } else if (encoding == karabo::xip::Encoding::JPEG || encoding == karabo::xip::Encoding::PNG ||
                       encoding == karabo::xip::Encoding::BMP || encoding == karabo::xip::Encoding::TIFF) {
                // JPEG, PNG, BMP, TIFF

                // Image is not stored as pixel values => byteSize _is_not_ height*width*bytesPerPixel!
                shape[0] = self.getByteSize();
                for (size_t i = 1; i < dims.size(); ++i) shape[i] = 1;

            } else {
                // Other encodings. Likely it will need to be fixed!
                std::reverse(dims.begin(), dims.end()); // REVERSING
                for (size_t i = 0; i < dims.size(); ++i) shape[i] = dims[i];
            }

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

        static void setDimensionsPy(RawImageData& self, const bp::object& dimensions) {
            std::vector<unsigned long long> vec = Wrapper::fromPyListToStdVector<unsigned long long>(dimensions);
            karabo::util::Dims d;
            d.fromVector(vec);
            self.setDimensions(d);
        }

        static bp::object getROIOffsetsPy(const RawImageData& self) {
            karabo::util::Dims d = self.getROIOffsets();
            return karathon::Wrapper::fromStdVectorToPyList(d.toVector());
        }

        static karabo::util::Hash getHeaderPy(const RawImageData& self) {
            return self.getHeader();
        }

        static void setHeaderPy(RawImageData& self, const bp::object & obj) {
            // TODO also support dict here!!
            if (bp::extract<karabo::util::Hash>(obj).check()) {
                const karabo::util::Hash& header = bp::extract<karabo::util::Hash>(obj);
                self.setHeader(header);
                return;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type of the argument in setHeader must be Hash");
        }

        static void writePy(RawImageData& self, const std::string& filename, const bool enableAppendMode = false) {
            self.write(filename, enableAppendMode);
        }

    };
}

#endif	/* KARATHON_RAWIMAGEDATAWRAP_HH */

