/* 
 * File:   Wrapper.cc
 * Author: esenov
 *
 * Created on March 17, 2013, 11:06 PM
 */

#include "Wrapper.hh"
#include <iostream>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>
#include <karabo/xip/CpuImage.hh>
#include "RawImageDataWrap.hh"
#include "PyXmsInputOutputChannel.hh"

using namespace std;
//using namespace karabo::util;

namespace karathon {


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<bool>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_BOOL);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            bool* data = reinterpret_cast<bool*> (PyArray_DATA(arr));
            for (int i = 0; i < PyArray_SIZE(arr); i++) {
                data[i] = v[i];
            }
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<short>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_SHORT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned short>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_USHORT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<int>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_INT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned int>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_UINT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<long long>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_LONGLONG);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned long long>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_ULONGLONG);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<float>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_FLOAT);
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj)); // 
        }
        return Wrapper::fromStdVectorToPyList(v);
    }


    template<>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<double>& v, bool numpyFlag) {
        if (numpyFlag) {
            int nd = 1;
            npy_intp dims[1];
            dims[0] = v.size();
            PyObject* pyobj = PyArray_SimpleNew(nd, dims, NPY_DOUBLE); // reinterpret_cast<void*> (&v[0]));
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (pyobj);
            memcpy(PyArray_DATA(arr), &v[0], PyArray_NBYTES(arr));
            return bp::object(bp::handle<>(pyobj));
        }
        return Wrapper::fromStdVectorToPyList(v);
    }

    bp::object Wrapper::toObject(const boost::any& operand, bool numpyFlag) {
        try {
            if (operand.type() == typeid (bool)) {
                return bp::object(boost::any_cast<bool>(operand));
            } else if (operand.type() == typeid (char)) {
                return bp::object(boost::any_cast<char>(operand));
            } else if (operand.type() == typeid (signed char)) {
                return bp::object(boost::any_cast<signed char>(operand));
            } else if (operand.type() == typeid (unsigned char)) {
                return bp::object(boost::any_cast<unsigned char>(operand));
            } else if (operand.type() == typeid (short)) {
                return bp::object(boost::any_cast<short>(operand));
            } else if (operand.type() == typeid (unsigned short)) {
                return bp::object(boost::any_cast<unsigned short>(operand));
            } else if (operand.type() == typeid (int)) {
                return bp::object(boost::any_cast<int>(operand));
            } else if (operand.type() == typeid (unsigned int)) {
                return bp::object(boost::any_cast<unsigned int>(operand));
            } else if (operand.type() == typeid (long long)) {
                return bp::object(boost::any_cast<long long>(operand));
            } else if (operand.type() == typeid (unsigned long long)) {
                return bp::object(boost::any_cast<unsigned long long>(operand));
            } else if (operand.type() == typeid (float)) {
                return bp::object(boost::any_cast<float>(operand));
            } else if (operand.type() == typeid (double)) {
                return bp::object(boost::any_cast<double>(operand));
            } else if (operand.type() == typeid (std::complex<float>)) {
                return bp::object(boost::any_cast<std::complex<float> >(operand));
            } else if (operand.type() == typeid (std::complex<double>)) {
                return bp::object(boost::any_cast<std::complex<double> >(operand));
            } else if (operand.type() == typeid (std::string)) {
                return bp::object(boost::any_cast<std::string>(operand));
            } else if (operand.type() == typeid (boost::filesystem::path)) {
                return bp::object(boost::any_cast<boost::filesystem::path>(operand).string());
            } else if (operand.type() == typeid (karabo::util::CppNone)) {
                return bp::object();
            } else if (operand.type() == typeid (karabo::util::Hash)) {
                return bp::object(boost::any_cast<karabo::util::Hash>(operand));
            } else if (operand.type() == typeid (std::vector<bool>)) {
                return fromStdVectorToPyArray(boost::any_cast < std::vector<bool> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast<std::vector<char> >(operand));
            } else if (operand.type() == typeid (std::vector<signed char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast < std::vector<signed char> >(operand));
            } else if (operand.type() == typeid (std::vector<unsigned char>)) {
                return fromStdVectorToPyByteArray(boost::any_cast<std::vector<unsigned char> >(operand));
            } else if (operand.type() == typeid (std::vector<short>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<short> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<unsigned short>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned short> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<int>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<int> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<unsigned int>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned int> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<long long>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<long long> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<unsigned long long>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<unsigned long long> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<float>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<float> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<double>)) {
                return fromStdVectorToPyArray(boost::any_cast<std::vector<double> >(operand), numpyFlag);
            } else if (operand.type() == typeid (std::vector<std::string>)) {
                return fromStdVectorToPyList(boost::any_cast < std::vector<std::string> >(operand));
            } else if (operand.type() == typeid (std::vector<karabo::util::CppNone>)) {
                return fromStdVectorToPyListNone(boost::any_cast < std::vector<karabo::util::CppNone> >(operand));
            } else if (operand.type() == typeid (karabo::util::NDArray<bool>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<bool> >(operand), NPY_BOOL);
            } else if (operand.type() == typeid (karabo::util::NDArray<signed char>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<signed char> >(operand), NPY_INT8);
            } else if (operand.type() == typeid (karabo::util::NDArray<unsigned char>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<unsigned char> >(operand), NPY_UINT8);
            } else if (operand.type() == typeid (karabo::util::NDArray<short>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<short> >(operand), NPY_SHORT);
            } else if (operand.type() == typeid (karabo::util::NDArray<unsigned short>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<unsigned short> >(operand), NPY_USHORT);
            } else if (operand.type() == typeid (karabo::util::NDArray<int>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<int> >(operand), NPY_LONG);
            } else if (operand.type() == typeid (karabo::util::NDArray<unsigned int>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<unsigned int> >(operand), NPY_ULONG);
            } else if (operand.type() == typeid (karabo::util::NDArray<long long>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<long long> >(operand), NPY_LONGLONG);
            } else if (operand.type() == typeid (karabo::util::NDArray<unsigned long long>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<unsigned long long> >(operand), NPY_ULONGLONG);
            } else if (operand.type() == typeid (karabo::util::NDArray<float>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<float> >(operand), NPY_FLOAT);
            } else if (operand.type() == typeid (karabo::util::NDArray<double>)) {
                return fromNDArrayToPyArray(boost::any_cast<karabo::util::NDArray<double> >(operand), NPY_DOUBLE);
            } else if (operand.type() == typeid (karabo::xip::RawImageData)) {
                karabo::xip::RawImageData raw = boost::any_cast<karabo::xip::RawImageData>(operand);
                return bp::object(boost::shared_ptr<karabo::xip::RawImageData>(new karabo::xip::RawImageData(raw)));
            } else if (operand.type() == typeid (karabo::xms::ImageData)) {
                return bp::object(boost::any_cast<karabo::xms::ImageData>(operand));
            } else if (operand.type() == typeid (karabo::xms::Data)) {
                return bp::object(boost::any_cast<karabo::xms::Data>(operand));
            } else if (operand.type() == typeid (karabo::util::Hash)) {
                return bp::object(boost::any_cast<karabo::util::Hash>(operand));
            } else if (operand.type() == typeid (karabo::util::Hash::Pointer)) {
                return bp::object(boost::any_cast<karabo::util::Hash::Pointer>(operand));
            } else if (operand.type() == typeid (karabo::util::Schema)) {
                return bp::object(boost::any_cast<karabo::util::Schema>(operand));
            } else if (operand.type() == typeid (std::vector<karabo::util::Hash>)) {
                return bp::object(boost::any_cast<std::vector<karabo::util::Hash> >(operand));
            } else if (operand.type() == typeid (std::vector<karabo::util::Hash::Pointer>)) {
                return bp::object(boost::any_cast<std::vector<karabo::util::Hash::Pointer> >(operand));
            } else if (operand.type() == typeid (bp::object) && hasattr(boost::any_cast<bp::object >(operand), "__name__")) {
                return boost::any_cast<bp::object >(operand);
            }
            throw KARABO_PYTHON_EXCEPTION("Failed to convert inner Hash type of python object");
        } catch (const boost::bad_any_cast& e) {
            KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
        }
        return bp::object(); // make compiler happy -- we never reach this statement
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
            long long const kNegInt32Min = -(1LL << 31);
            long long const kPosUint32Max = (1LL << 32) - 1;
            long long const kPosInt32Max = (1LL << 31) - 1;
            long long const kPosInt64Max = (1ULL << 63) - 1;
            int overflow = 0;
            PY_LONG_LONG value = PyLong_AsLongLongAndOverflow(obj.ptr(), &overflow);
            if (overflow == 0) {
                if (value < 0) {
                    if (value < kNegInt32Min) {
                        any = static_cast<long long> (value);
                        return karabo::util::Types::INT64;
                    } else {
                        any = static_cast<int> (value);
                        return karabo::util::Types::INT32;
                    }
                } else {
                    if (value > kPosUint32Max) {
                        if (value <= kPosInt64Max) {
                            any = static_cast<long long> (value);
                            return karabo::util::Types::INT64;
                        }
                        any = static_cast<unsigned long long> (value);
                        return karabo::util::Types::UINT64;
                    } else {
                        if (value <= kPosInt32Max) {
                            any = static_cast<int> (value);
                            return karabo::util::Types::INT32;
                        }
                        any = static_cast<unsigned int> (value);
                        return karabo::util::Types::UINT32;
                    }
                }
            }

            // Try UINT64. Raises a Python exception if it overflows...
            unsigned long long val = PyLong_AsUnsignedLongLong(obj.ptr());
            any = val;
            return karabo::util::Types::UINT64;
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
            return karabo::util::Types::PTR_CHAR; //TODO: Define WCHAR and PTR_WCHAR. Check with Burkhard and Martin
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
            std::vector<karabo::util::Hash::Pointer> vhash = bp::extract<std::vector<karabo::util::Hash::Pointer> >(obj);
            any = vhash;
            return karabo::util::Types::VECTOR_HASH_POINTER;
        }
        if (PyArray_Check(obj.ptr())) {
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*> (obj.ptr());
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            switch (dtype->type_num) {
                case NPY_BOOL:
                {
                    any = fromPyArrayToNDArray<bool>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_BOOL;
                }
                case NPY_INT8:
                {
                    any = fromPyArrayToNDArray<signed char>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_INT8;
                }
                case NPY_UINT8:
                {
                    any = fromPyArrayToNDArray<unsigned char>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_UINT8;
                }
                case NPY_SHORT:
                {
                    any = fromPyArrayToNDArray<short>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_INT16;
                }
                case NPY_USHORT:
                {
                    any = fromPyArrayToNDArray<unsigned short>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_UINT16;
                }
                case NPY_INT:
                {
                    any = fromPyArrayToNDArray<int>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_INT32;
                }
                case NPY_UINT:
                {
                    any = fromPyArrayToNDArray<unsigned int>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_UINT32;
                }
                case NPY_LONG:
                {
                    if (sizeof (long) == sizeof (int)) {
                        any = fromPyArrayToNDArray<int>(arr, dtype->type_num);
                    } else if (sizeof (long) == sizeof (long long)) {
                        any = fromPyArrayToNDArray<long long>(arr, dtype->type_num);
                    }
                    return karabo::util::Types::NDARRAY_INT64;
                }
                case NPY_ULONG:
                {
                    if (sizeof (unsigned long) == sizeof (unsigned int)) {
                        any = fromPyArrayToNDArray<unsigned int>(arr, dtype->type_num);
                    } else if (sizeof (unsigned long) == sizeof (unsigned long long)) {
                        any = fromPyArrayToNDArray<unsigned long long>(arr, dtype->type_num);
                    }
                    return karabo::util::Types::NDARRAY_UINT64;
                }
                case NPY_LONGLONG:
                {
                    any = fromPyArrayToNDArray<long long>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_INT64;
                }
                case NPY_ULONGLONG:
                {
                    any = fromPyArrayToNDArray<unsigned long long>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_UINT64;
                }
                case NPY_FLOAT:
                {
                    any = fromPyArrayToNDArray<float>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_FLOAT;
                }
                case NPY_DOUBLE:
                {
                    any = fromPyArrayToNDArray<double>(arr, dtype->type_num);
                    return karabo::util::Types::NDARRAY_DOUBLE;
                }
                default:
                    break;
            }
            return karabo::util::Types::NONE;
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
                try {
                    std::vector<int> v(size);
                    for (bp::ssize_t i = 0; i < size; ++i) {
                        v[i] = static_cast<int> (bp::extract<int>(obj[i]));
                    }
                    any = v;
                    return karabo::util::Types::VECTOR_INT32;
                } catch (...) {
                    try {
                        std::vector<unsigned int> v(size);
                        for (bp::ssize_t i = 0; i < size; ++i) {
                            v[i] = static_cast<unsigned int> (bp::extract<unsigned int>(obj[i]));
                        }
                        any = v;
                        return karabo::util::Types::VECTOR_UINT32;
                    } catch (...) {
                        try {
                            std::vector<long long> v(size);
                            for (bp::ssize_t i = 0; i < size; ++i) {
                                v[i] = static_cast<long long> (bp::extract<long long>(obj[i]));
                            }
                            any = v;
                            return karabo::util::Types::VECTOR_INT64;
                        } catch (...) {
                            std::vector<unsigned long long> v(size);
                            for (bp::ssize_t i = 0; i < size; ++i) {
                                v[i] = static_cast<unsigned long long> (bp::extract<unsigned long long>(obj[i]));
                            }
                            any = v;
                            return karabo::util::Types::VECTOR_UINT64;
                        }
                    }
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
                    const char* data = PyUnicode_AsUTF8AndSize(static_cast<bp::object> (obj[i]).ptr(), &size);
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
        if (hasattr(obj, "__name__")) {// python function
            any = obj;
            return karabo::util::Types::ANY;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
    }
}

