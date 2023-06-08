/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   Wrapper.cc
 * Author: esenov
 *
 * Created on March 17, 2013, 11:06 PM
 */

#include "Wrapper.hh"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <karabo/net/Broker.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/xms/ImageData.hh>

using namespace std;

namespace karathon {

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<bool>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_BOOL);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            bool* data = reinterpret_cast<bool*>(PyArray_DATA(arr));
            for (int i = 0; i < PyArray_SIZE(arr); i++) {
                data[i] = v[i];
            }
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<short>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_SHORT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned short>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_USHORT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<int>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_INT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned int>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_UINT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<long long>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_LONGLONG);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned long long>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_ULONGLONG);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<float>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_FLOAT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); //
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<double>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_DOUBLE); // reinterpret_cast<void*> (&v[0]));
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj));
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    bp::object Wrapper::toObject(const boost::any& operand, bool numpyFlag) {
        try {
            if (operand.type() == typeid(bool)) {
                return bp::object(boost::any_cast<bool>(operand));
            } else if (operand.type() == typeid(char)) {
                return bp::object(boost::any_cast<char>(operand));
            } else if (operand.type() == typeid(signed char)) {
                return bp::object(boost::any_cast<signed char>(operand));
            } else if (operand.type() == typeid(unsigned char)) {
                return bp::object(boost::any_cast<unsigned char>(operand));
            } else if (operand.type() == typeid(short)) {
                return bp::object(boost::any_cast<short>(operand));
            } else if (operand.type() == typeid(unsigned short)) {
                return bp::object(boost::any_cast<unsigned short>(operand));
            } else if (operand.type() == typeid(int)) {
                return bp::object(boost::any_cast<int>(operand));
            } else if (operand.type() == typeid(unsigned int)) {
                return bp::object(boost::any_cast<unsigned int>(operand));
            } else if (operand.type() == typeid(long long)) {
                return bp::object(boost::any_cast<long long>(operand));
            } else if (operand.type() == typeid(unsigned long long)) {
                return bp::object(boost::any_cast<unsigned long long>(operand));
            } else if (operand.type() == typeid(float)) {
                return bp::object(boost::any_cast<float>(operand));
            } else if (operand.type() == typeid(double)) {
                return bp::object(boost::any_cast<double>(operand));
            } else if (operand.type() == typeid(std::complex<float>)) {
                return bp::object(boost::any_cast<std::complex<float> >(operand));
            } else if (operand.type() == typeid(std::complex<double>)) {
                return bp::object(boost::any_cast<std::complex<double> >(operand));
            } else if (operand.type() == typeid(std::string)) {
                return bp::object(boost::any_cast<std::string>(operand));
            } else if (operand.type() == typeid(boost::filesystem::path)) {
                return bp::object(boost::any_cast<boost::filesystem::path>(operand).string());
            } else if (operand.type() == typeid(karabo::util::CppNone)) {
                return bp::object();
            } else if (operand.type() == typeid(karabo::util::Hash)) {
                return bp::object(boost::any_cast<karabo::util::Hash>(operand));
            } else if (operand.type() == typeid(karabo::util::Hash::Pointer)) {
                return bp::object(boost::any_cast<karabo::util::Hash::Pointer>(operand));
            } else if (operand.type() == typeid(std::vector<bool>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<bool> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast<std::vector<char> >(operand));
            } else if (operand.type() == typeid(std::vector<signed char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast<std::vector<signed char> >(operand));
            } else if (operand.type() == typeid(std::vector<unsigned char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast<std::vector<unsigned char> >(operand));
            } else if (operand.type() == typeid(std::vector<short>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<short> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<unsigned short>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned short> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<int>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<int> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<unsigned int>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned int> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<long long>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<long long> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<unsigned long long>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned long long> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<float>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<float> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<double>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<double> >(operand), numpyFlag);
            } else if (operand.type() == typeid(std::vector<std::string>)) {
                return fromStdVectorToPyList(boost::any_cast<std::vector<std::string> >(operand));
            } else if (operand.type() == typeid(std::vector<karabo::util::CppNone>)) {
                return fromStdVectorToPyListNone(boost::any_cast<std::vector<karabo::util::CppNone> >(operand));
            } else if (operand.type() == typeid(karabo::util::Schema)) {
                return bp::object(boost::any_cast<karabo::util::Schema>(operand));
            } else if (operand.type() == typeid(std::vector<karabo::util::Hash>)) {
                return bp::object(boost::any_cast<std::vector<karabo::util::Hash> >(operand));
            } else if (operand.type() == typeid(std::vector<karabo::util::Hash::Pointer>)) {
                return bp::object(boost::any_cast<std::vector<karabo::util::Hash::Pointer> >(operand));
            } else if (operand.type() == typeid(bp::object) &&
                       hasattr(boost::any_cast<bp::object>(operand), "__name__")) {
                return boost::any_cast<bp::object>(operand);
            }
            throw KARABO_PYTHON_EXCEPTION("Failed to convert inner Hash type of python object");
        } catch (const boost::bad_any_cast& e) {
            KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
        }
        return bp::object(); // make compiler happy -- we never reach this statement
    }

    bp::object Wrapper::toCustomObject(karabo::util::Hash::Node& node) {
        karabo::util::Hash::Pointer hash =
              karabo::util::Hash::Pointer(&node.getValue<karabo::util::Hash>(), null_deleter());
        if (node.hasAttribute(KARABO_HASH_CLASS_ID)) { // Hash actually holds data for a custom class
            const std::string& classId = node.getAttribute<string>(KARABO_HASH_CLASS_ID);
            if (classId == "NDArray") {
                return fromNDArrayToPyArray(reinterpret_cast<const karabo::util::NDArray&>(*hash));
            }
            if (classId == "ImageData") {
                const karabo::xms::ImageData& imgData = reinterpret_cast<const karabo::xms::ImageData&>(*hash);
                return bp::object(karabo::xms::ImageData::Pointer(new karabo::xms::ImageData(imgData)));
            }
        }
        return bp::object(hash);
    }


    // Helper for Wrapper::toAny below:
    karabo::util::Types::ReferenceType bestIntegerType(const bp::object& obj) {
        long long const kNegInt32Min = -(1LL << 31);
        long long const kPosUint32Max = (1LL << 32) - 1;
        long long const kPosInt32Max = (1LL << 31) - 1;
        long long const kPosInt64Max = (1ULL << 63) - 1;

        int overflow = 0;
        PY_LONG_LONG value = PyLong_AsLongLongAndOverflow(obj.ptr(), &overflow);
        if (overflow == 0) {
            if (value < 0) {
                if (value < kNegInt32Min) {
                    return karabo::util::Types::INT64;
                } else {
                    return karabo::util::Types::INT32;
                }
            } else {
                if (value > kPosUint32Max) {
                    if (value <= kPosInt64Max) {
                        return karabo::util::Types::INT64;
                    } else {
                        return karabo::util::Types::UINT64;
                    }
                } else {
                    if (value <= kPosInt32Max) {
                        return karabo::util::Types::INT32;
                    } else {
                        return karabo::util::Types::UINT32;
                    }
                }
            }
        } else {
            // So 'long long', i.e. INT64, overflows. Best try is UINT64 that needs PyLong_AsUnsignedLongLong
            // for conversion. Note that that will raise a Python exception if even that overflows.
            return karabo::util::Types::UINT64;
        }
    }

    karabo::util::Types::ReferenceType Wrapper::toAny(const bp::object& obj, boost::any& any) {
        if (obj.ptr() == Py_None) {
            any = karabo::util::CppNone();
            return karabo::util::Types::NONE;
        }
        if (PyBool_Check(obj.ptr())) {
            bool b = bp::extract<bool>(obj);
            any = b;
            return karabo::util::Types::BOOL;
        }
        if (PyLong_Check(obj.ptr())) {
            // An integer - let's find out which one fits:
            const karabo::util::Types::ReferenceType type = bestIntegerType(obj);
            if (type == karabo::util::Types::UINT64) {
                // Raises a Python exception if it overflows:
                any = PyLong_AsUnsignedLongLong(obj.ptr());
            } else {
                const PY_LONG_LONG value = PyLong_AsLongLong(obj.ptr());
                switch (type) {
                    case karabo::util::Types::INT32:
                        any = static_cast<int>(value);
                        break;
                    case karabo::util::Types::UINT32:
                        any = static_cast<unsigned int>(value);
                        break;
                    case karabo::util::Types::INT64:
                        any = static_cast<long long>(value);
                        break;
                    default:
                        // Should never come here!
                        throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + toString(type));
                }
            }
            return type;
        }
        if (PyFloat_Check(obj.ptr())) {
            double b = bp::extract<double>(obj);
            any = b;
            return karabo::util::Types::DOUBLE;
        }
        if (PyComplex_Check(obj.ptr())) {
            std::complex<double> b = bp::extract<std::complex<double> >(obj);
            any = b;
            return karabo::util::Types::COMPLEX_DOUBLE;
        }
        if (PyUnicode_Check(obj.ptr())) {
            Py_ssize_t size;
            const char* data = PyUnicode_AsUTF8AndSize(obj.ptr(), &size);
            string b(data, size);
            any = b;
            return karabo::util::Types::STRING;
        }
        if (PyBytes_Check(obj.ptr())) {
            size_t size = PyBytes_Size(obj.ptr());
            char* data = PyBytes_AsString(obj.ptr());
            std::vector<char> b(data, data + size);
            any = b;
            return karabo::util::Types::VECTOR_CHAR;
        }
        if (PyByteArray_Check(obj.ptr())) {
            size_t size = PyByteArray_Size(obj.ptr());
            char* data = PyByteArray_AsString(obj.ptr());
            std::vector<char> b(data, data + size);
            any = b;
            return karabo::util::Types::VECTOR_CHAR;
        }
        if (bp::extract<char* const>(obj).check()) {
            char* const b = bp::extract<char* const>(obj);
            any = b;
            return karabo::util::Types::PTR_CHAR;
        }
        if (bp::extract<wchar_t* const>(obj).check()) {
            wchar_t* const b = bp::extract<wchar_t* const>(obj);
            any = b;
            return karabo::util::Types::PTR_CHAR; // TODO: Define WCHAR and PTR_WCHAR. Check with Burkhard and Martin
        }
        if (bp::extract<std::vector<std::string> >(obj).check()) {
            std::vector<std::string> const b = bp::extract<std::vector<std::string> >(obj);
            any = b;
            return karabo::util::Types::VECTOR_STRING;
        }
        if (bp::extract<karabo::util::Hash>(obj).check()) {
            karabo::util::Hash h = bp::extract<karabo::util::Hash>(obj);
            any = h;
            return karabo::util::Types::HASH;
        }
        if (bp::extract<karabo::util::Hash::Pointer>(obj).check()) {
            karabo::util::Hash::Pointer h = bp::extract<karabo::util::Hash::Pointer>(obj);
            any = h;
            return karabo::util::Types::HASH_POINTER;
        }
        if (bp::extract<karabo::util::Schema>(obj).check()) {
            karabo::util::Schema s = bp::extract<karabo::util::Schema>(obj);
            any = s;
            return karabo::util::Types::SCHEMA;
        }
        if (bp::extract<std::vector<karabo::util::Hash> >(obj).check()) {
            std::vector<karabo::util::Hash> vhash = bp::extract<std::vector<karabo::util::Hash> >(obj);
            any = vhash;
            return karabo::util::Types::VECTOR_HASH;
        }
        if (bp::extract<std::vector<karabo::util::Hash::Pointer> >(obj).check()) {
            std::vector<karabo::util::Hash::Pointer> vhash =
                  bp::extract<std::vector<karabo::util::Hash::Pointer> >(obj);
            any = vhash;
            return karabo::util::Types::VECTOR_HASH_POINTER;
        }
        if (PyArray_Check(obj.ptr())) {
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
            karabo::util::NDArray nd = fromPyArrayToNDArray(arr);
            any = reinterpret_cast<karabo::util::Hash&>(nd);
            return karabo::util::Types::HASH;
        }
        if (PyList_Check(obj.ptr())) {
            bp::ssize_t size = bp::len(obj);
            if (size == 0) {
                any = std::vector<std::string>();
                return karabo::util::Types::VECTOR_STRING;
            }
            bp::object list0 = obj[0];
            if (list0.ptr() == Py_None) {
                std::vector<karabo::util::CppNone> v;
                for (bp::ssize_t i = 0; i < size; ++i) v.push_back(karabo::util::CppNone());
                any = v;
                return karabo::util::Types::VECTOR_NONE;
            }
            if (PyBool_Check(list0.ptr())) {
                std::vector<bool> v(size); // Special case here
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<bool>(obj[i]);
                }
                any = v;
                return karabo::util::Types::VECTOR_BOOL;
            }
            if (PyLong_Check(list0.ptr())) {
                // First item is an integer - assume that all items are!
                karabo::util::Types::ReferenceType broadestType = karabo::util::Types::INT32;
                for (bp::ssize_t i = 0; i < size; ++i) {
                    const karabo::util::Types::ReferenceType type = bestIntegerType(obj[i]);
                    // This relies on the fact that the enums ReferenceType have the order INT32, UINT32, INT64, UINT64
                    if (type > broadestType) {
                        broadestType = type;
                        // Stop loop if cannot get broader...
                        if (broadestType == karabo::util::Types::UINT64) break;
                    }
                }
                if (broadestType == karabo::util::Types::INT32) {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<int>(obj[i]);
                    }
                    any = std::move(v);
                    return karabo::util::Types::VECTOR_INT32;
                } else if (broadestType == karabo::util::Types::UINT32) {
                    std::vector<unsigned int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<unsigned int>(obj[i]);
                    }
                    any = std::move(v);
                    return karabo::util::Types::VECTOR_UINT32;
                } else if (broadestType == karabo::util::Types::INT64) {
                    std::vector<long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<long long>(obj[i]);
                    }
                    any = std::move(v);
                    return karabo::util::Types::VECTOR_INT64;
                } else if (broadestType == karabo::util::Types::UINT64) {
                    std::vector<unsigned long long> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = bp::extract<unsigned long long>(obj[i]);
                    }
                    any = std::move(v);
                    return karabo::util::Types::VECTOR_UINT64;
                } else {
                    // Should never come here!
                    throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + toString(broadestType));
                }
            }
            if (PyFloat_Check(list0.ptr())) {
                std::vector<double> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<double>(obj[i]);
                }
                any = v;
                return karabo::util::Types::VECTOR_DOUBLE;
            }
            if (PyUnicode_Check(list0.ptr())) {
                std::vector<std::string> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    Py_ssize_t size;
                    const char* data = PyUnicode_AsUTF8AndSize(static_cast<bp::object>(obj[i]).ptr(), &size);
                    v[i] = string(data, size);
                }
                any = v;
                return karabo::util::Types::VECTOR_STRING;
            }
            if (bp::extract<karabo::util::Hash>(list0).check()) {
                std::vector<karabo::util::Hash> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<karabo::util::Hash>(obj[i]);
                }
                any = v;
                return karabo::util::Types::VECTOR_HASH;
            }
            if (bp::extract<karabo::util::Hash::Pointer>(list0).check()) {
                std::vector<karabo::util::Hash::Pointer> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<karabo::util::Hash::Pointer>(obj[i]);
                }
                any = v;
                return karabo::util::Types::VECTOR_HASH_POINTER;
            }
            if (bp::extract<karabo::util::Schema>(list0).check()) {
                std::vector<karabo::util::Schema> v(size);
                for (bp::ssize_t i = 0; i < size; ++i) {
                    v[i] = bp::extract<karabo::util::Schema>(obj[i]);
                }
                any = v;
                return karabo::util::Types::VECTOR_SCHEMA;
            }
        }
        if (hasattr(obj, "__name__")) { // python function
            any = obj;
            return karabo::util::Types::ANY;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
    }

    bp::object Wrapper::fromNDArrayToPyArray(const karabo::util::NDArray& ndarray) {
        using namespace karabo::util;
        const Types::ReferenceType karaboType = ndarray.getType();
        const int typenum = Types::to<ToNumpy>(karaboType);
        const size_t bytesPerItem = Types::to<ToSize>(karaboType);
        const karabo::util::Dims shape = ndarray.getShape();
        const int nd = shape.rank();
        std::vector<npy_intp> dims(nd, 0);
        for (int i = 0; i < nd; ++i) {
            dims[i] = shape.extentIn(i);
        }

        const ByteArray& byteArray = ndarray.getByteArray();
        if (shape.size() * bytesPerItem > byteArray.second) {
            throw KARABO_PARAMETER_EXCEPTION("Inconsistent NDArray: " + toString(byteArray.second) +=
                                             " are too few bytes for shape [" + toString(shape.toVector()) +=
                                             "] of " + Types::to<ToLiteral>(karaboType));
        }

        karabo::util::NDArray::DataPointer dataPtr(byteArray.first);
        const boost::shared_ptr<CppArrayRefHandler> refHandler(new CppArrayRefHandler(dataPtr));
        bp::object pyRefHandler(refHandler); // Python reference count starts at 1
        void* data = reinterpret_cast<void*>(dataPtr.get());
        PyObject* pyobj = PyArray_SimpleNewFromData(nd, &dims[0], typenum, data);
        PyArray_SetBaseObject(reinterpret_cast<PyArrayObject*>(pyobj), pyRefHandler.ptr());
        // PyArray_SetBaseObject steals a reference. Increase the refcount to protect bp::object::~object()
        Py_INCREF(pyRefHandler.ptr());
        return bp::object(bp::handle<>(pyobj));
    }

    karabo::util::NDArray Wrapper::fromPyArrayToNDArray(PyArrayObject* arr) {
        // Convert the array shape to a std::vector
        npy_intp* pDims = PyArray_DIMS(arr);
        std::vector<unsigned long long> dims;
        for (int i = 0; i < PyArray_NDIM(arr); i++) {
            dims.push_back(pDims[i]);
        }

        // Get information about the stored type
        PyArray_Descr* dtype = PyArray_DESCR(arr);
        const int pyType = dtype->type_num;
        const karabo::util::Types::ReferenceType krbType = karabo::util::Types::from<FromNumpy>(pyType);

        // Extract number of elements
        const size_t nelems = static_cast<size_t>(PyArray_SIZE(arr));

        // Get a smart DataPointer object which points to the array's data
        karabo::util::NDArray::DataPointer dataPtr;

        PyObject* arrBase = PyArray_BASE(arr);

        // Determine if the array data is owned by a C++ object
        if (arrBase != NULL && arrBase != Py_None) {
            bp::object base(bp::handle<>(bp::borrowed(arrBase)));
            bp::extract<CppArrayRefHandler> maybeArrayRef(base);
            if (maybeArrayRef.check()) {
                const CppArrayRefHandler& arrayRef = maybeArrayRef();
                // The data already has an DataPointer object managing it.
                dataPtr = arrayRef.getDataPtr();
            }
        }

        // Array ref is still empty. Create a new ArrayData
        if (!dataPtr) {
            PyObject* pyobj = reinterpret_cast<PyObject*>(arr);
            // Get a contiguous copy of the array with the correct type (or just a reference if already compatible)
            PyArrayObject* carr = reinterpret_cast<PyArrayObject*>(
                  PyArray_FROMANY(pyobj, pyType, KRB_NDARRAY_MIN_DIM, KRB_NDARRAY_MAX_DIM, NPY_ARRAY_C_CONTIGUOUS));
            if (carr != NULL) {
                char* data = reinterpret_cast<char*>(PyArray_DATA(carr));
                const PyArrayRefHandler refHandler(carr); // Steals the reference to carr
                // Create a new ArrayData<T> which uses PyArrayRefHandler to manage the Python reference count
                dataPtr = boost::shared_ptr<char>(data, refHandler);
            }
        }

        if (!dataPtr) {
            throw KARABO_PYTHON_EXCEPTION("Failed conversion of ndarray to C++ NDArray.");
        }

        // Construct NDArray
        return karabo::util::NDArray(dataPtr, krbType, nelems, karabo::util::Dims(dims));
    }


    size_t Wrapper::numArgs(const bp::object& callable) {
        size_t result = 0ul;
        size_t numSelfArgs = 0ul;

        PyObject* function_object = NULL;
        // We expect either
        // * standalone function
        // * member method
        // * object with __call__ attribute
        if (PyFunction_Check(callable.ptr())) {
            function_object = callable.ptr();
        } else if (PyMethod_Check(callable.ptr())) {
            function_object = PyMethod_Function(callable.ptr());
            numSelfArgs = 1ul;
        } else if (Wrapper::hasattr(callable, "__call__")) {
            bp::object call = callable.attr("__call__");
            if (PyFunction_Check(call.ptr())) {
                function_object = call.ptr();
            } else if (PyMethod_Check(call.ptr())) {
                function_object = PyMethod_Function(call.ptr());
                numSelfArgs = 1ul;
            } else {
                // For a functools.partial objects we end up here...
                throw KARABO_PARAMETER_EXCEPTION(
                      "Attribute __call__ is neither function nor method, try to specify number of arguments.");
            }
        } else {
            throw KARABO_PARAMETER_EXCEPTION("Cannot deduce number of arguments, please specify explicitely.");
        }
        PyCodeObject* pycode = reinterpret_cast<PyCodeObject*>(PyFunction_GetCode(function_object));
        if (pycode) {
            // Note: co_argcount includes arguments with defaults, see nice figure from 'Hzzkygcs' at
            // https://stackoverflow.com/questions/847936/how-can-i-find-the-number-of-arguments-of-a-python-function
            result = pycode->co_argcount - numSelfArgs; // Subtract "self" if any
        } else {                                        // Can we get here?
            throw KARABO_PARAMETER_EXCEPTION("Failed to access PyCode object to deduce number of arguments.");
        }

        return result;
    }


    karabo::util::Hash Wrapper::deepCopy_r(const karabo::util::Hash& h) {
        karabo::util::Hash r;
        // iterate through all entries of the Hash. If the value of the Hash::Node at it
        // is not of a Hash type we insert into our result Hash r, if not we recursivly
        // call deepCopy_r to copy the internal structure
        // We make sure to maintain attributes
        for (karabo::util::Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
            if (it->getType() == karabo::util::Types::HASH) {
                karabo::util::Hash::Node& n = r.set(it->getKey(), deepCopy_r(it->getValue<karabo::util::Hash>()));
                n.setAttributes(it->getAttributes());
            } else if (it->getType() == karabo::util::Types::VECTOR_HASH) {
                const std::vector<karabo::util::Hash>& v = it->getValue<std::vector<karabo::util::Hash> >();
                karabo::util::Hash::Node& n = r.set(it->getKey(), std::vector<karabo::util::Hash>());
                std::vector<karabo::util::Hash>& vc = n.getValue<std::vector<karabo::util::Hash> >();
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(*vit));
                }
                n.setAttributes(it->getAttributes());
            } else if (it->getType() == karabo::util::Types::HASH_POINTER) {
                karabo::util::Hash::Node& n =
                      r.set(it->getKey(), deepCopy_r(*(it->getValue<karabo::util::Hash::Pointer>())));
                n.setAttributes(it->getAttributes());
            } else if (it->getType() == karabo::util::Types::VECTOR_HASH_POINTER) {
                const std::vector<karabo::util::Hash::Pointer>& v =
                      it->getValue<std::vector<karabo::util::Hash::Pointer> >();
                karabo::util::Hash::Node& n = r.set(it->getKey(), std::vector<karabo::util::Hash>());
                std::vector<karabo::util::Hash>& vc = n.getValue<std::vector<karabo::util::Hash> >();
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(**vit));
                }
                n.setAttributes(it->getAttributes());
            } else { // if no Hash type we do not need to recurse
                r.setNode(it);
            }
        }
        return r;
    }

    bp::object Wrapper::deepCopyHashLike(const bp::object& obj) {
        // we only check for Hash typed objects, which basically means obj
        // contains a Hash::Node, a Hash, a vector of Hashes or pointers to Hashes
        if (bp::extract<karabo::util::Hash::Node&>(obj).check()) {
            // Hash::Node case - check type information of the value and deep copy for aforementioned
            // Hash types
            const karabo::util::Hash::Node& node = bp::extract<karabo::util::Hash::Node&>(obj);
            if (node.getType() == karabo::util::Types::HASH) {
                return bp::object(deepCopy_r(node.getValue<karabo::util::Hash>()));
            } else if (node.getType() == karabo::util::Types::VECTOR_HASH) {
                const std::vector<karabo::util::Hash>& v = node.getValue<std::vector<karabo::util::Hash> >();
                std::vector<karabo::util::Hash> vc;
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(*vit));
                }
                return bp::object(vc);
            } else if (node.getType() == karabo::util::Types::HASH_POINTER) {
                return bp::object(deepCopy_r(*node.getValue<karabo::util::Hash::Pointer>()));
            } else if (node.getType() == karabo::util::Types::VECTOR_HASH_POINTER) {
                const std::vector<karabo::util::Hash::Pointer>& v =
                      node.getValue<std::vector<karabo::util::Hash::Pointer> >();
                std::vector<karabo::util::Hash> vc;
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(**vit));
                }
                return bp::object(vc);
            } else { // if no Hash like object was found we just return the object
                return obj;
            }
            // obj contains a Hash
        } else if (bp::extract<karabo::util::Hash&>(obj).check()) {
            const karabo::util::Hash& hash = bp::extract<karabo::util::Hash&>(obj);
            return bp::object(deepCopy_r(hash));
            // obj contains a Hash::Pointer
        } else if (bp::extract<karabo::util::Hash::Pointer&>(obj).check()) {
            const karabo::util::Hash::Pointer& hp = bp::extract<karabo::util::Hash::Pointer&>(obj);
            return bp::object(deepCopy_r(*hp));
            // obj contains a vector<Hash>
        } else if (bp::extract<std::vector<karabo::util::Hash>&>(obj).check()) {
            const std::vector<karabo::util::Hash>& v = bp::extract<std::vector<karabo::util::Hash>&>(obj);
            std::vector<karabo::util::Hash> vc;
            vc.reserve(v.size());
            for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                vc.push_back(deepCopy_r(*vit));
            }
            return bp::object(vc);
            // final scenario to deep copy: vector<Hash::Pointer>
        } else if (bp::extract<std::vector<karabo::util::Hash::Pointer>&>(obj).check()) {
            const std::vector<karabo::util::Hash::Pointer>& v =
                  bp::extract<std::vector<karabo::util::Hash::Pointer>&>(obj);
            std::vector<karabo::util::Hash> vc;
            vc.reserve(v.size());
            for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                vc.push_back(deepCopy_r(**vit));
            }
            return bp::object(vc);
        } else { // nothing to deep-copy
            return obj;
        }
    }

    void HandlerWrapAny1::operator()(const boost::any& a1) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(Wrapper::toObject(a1));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny2::operator()(const boost::any& a1, const boost::any& a2) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(Wrapper::toObject(a1), Wrapper::toObject(a2));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny3::operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapAny4::operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3,
                                     const boost::any& a4) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(Wrapper::toObject(a1), Wrapper::toObject(a2), Wrapper::toObject(a3),
                             Wrapper::toObject(a4));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }

    void HandlerWrapVullVull::operator()(const std::vector<unsigned long long>& v1,
                                         const std::vector<unsigned long long>& v2) const {
        ScopedGILAcquire gil;
        try {
            if (*m_handler) {
                (*m_handler)(Wrapper::fromStdVectorToPyList(v1), Wrapper::fromStdVectorToPyList(v2));
            }
        } catch (const bp::error_already_set& e) {
            karathon::detail::treatError_already_set(*m_handler, m_where);
        } catch (...) {
            KARABO_RETHROW
        }
    }


    std::tuple<std::string, std::string> getPythonExceptionStrings() {
        // Fetch parameters of error indicator ... the error indicator is getting cleared!
        // ... the new references returned!
        PyObject *pexceptType, *pexception, *ptraceback;

        PyErr_Fetch(&pexceptType, &pexception, &ptraceback); // ref count incremented
        PyErr_NormalizeException(&pexceptType, &pexception, &ptraceback);

        std::string pythonErrorMessage;
        std::string pythonErrorDetails;

        if (pexceptType && pexception && ptraceback) {
            // Try to extract full traceback
            PyObject* moduleTraceback = PyImport_ImportModule("traceback");
            if (moduleTraceback != 0) {
                // Letter "O" in format string denotes conversion from Object ... 3 arguments
                PyObject* plist = PyObject_CallMethod(moduleTraceback, "format_exception", "OOO", pexceptType,
                                                      pexception, ptraceback);
                if (plist) {
                    // "format_exception" returns list of strings
                    Py_ssize_t size = PyList_Size(plist); // > 0, see doc of "format_exception"/"print_exception"
                    for (Py_ssize_t i = 0; i < size - 1; ++i) {
                        // All but last line in list is traceback
                        PyObject* pstrItem = PyList_GetItem(plist, i); // this "borrowed reference" - no decref!
                        pythonErrorDetails.append(PyUnicode_AsUTF8(pstrItem));
                    }
                    // Last line is type and message
                    PyObject* pstrItem = PyList_GetItem(plist, size - 1); // this "borrowed reference" - no decref!
                    pythonErrorMessage = PyUnicode_AsUTF8(pstrItem);
                    Py_DECREF(plist);
                }
                Py_DECREF(moduleTraceback);
            } else {
                PyObject* pythonRepr = PyObject_Repr(pexception); // apply repr()
                pythonErrorMessage.assign(PyUnicode_AsUTF8(pythonRepr));
                Py_DECREF(pythonRepr);
            }
        } // else there is no exception, so keep pythonErrorMessage empty

        // we reset it for later processing
        PyErr_Restore(pexceptType, pexception, ptraceback); // ref count decremented
        PyErr_Clear();
        // Remove trailing newline
        boost::algorithm::trim_right(pythonErrorMessage);
        boost::algorithm::trim_right(pythonErrorDetails);

        return std::make_tuple(pythonErrorMessage, pythonErrorDetails);
    }

    namespace detail {
        void treatError_already_set(const bp::object& handler, const char* where) {
            std::string errStr, errDetails;
            if (PyErr_Occurred()) {
                std::tie(errStr, errDetails) = getPythonExceptionStrings();
            }
            const std::string funcName(Wrapper::hasattr(handler, "__name__")
                                             ? std::string(bp::extract<std::string>(handler.attr("__name__")))
                                             : std::string()); // e.g. 'partial' does not provide __name__
            std::ostringstream oss;
            oss << "Error in ";
            if (funcName.empty()) {
                oss << " python handler for '" << (where ? where : "undefined") << "'";
            } else {
                oss << "'" << funcName << "'";
            }
            oss << ": " << errStr;
            errStr = oss.str();
            std::cerr << '\n' << errStr << '\n' << errDetails << std::endl;
            throw KARABO_PYTHON_EXCEPTION2(errStr, errDetails);
        }
    } // namespace detail
} // namespace karathon
