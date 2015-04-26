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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
            bool* data = reinterpret_cast<bool*>(PyArray_DATA(arr));
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(pyobj);
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
            } else if (operand.type() == typeid (karabo::xip::RawImageData)) {
                karabo::xip::RawImageData raw = boost::any_cast<karabo::xip::RawImageData>(operand);
                return bp::object(boost::shared_ptr<karabo::xip::RawImageData>(new karabo::xip::RawImageData(raw)));
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
        return bp::object();   // make compiler happy -- we never reach this statement
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
            try {
                any = static_cast<int> (bp::extract<int>(obj));
            } catch (...) {
                try {
                    any = static_cast<unsigned int> (bp::extract<unsigned int>(obj));
                } catch (...) {
                    try {
                        any = static_cast<long long> (bp::extract<long long>(obj));
                    } catch (...) {
                        any = static_cast<unsigned long long> (bp::extract<unsigned long long>(obj));
                        return karabo::util::Types::UINT64;
                    }
                    return karabo::util::Types::INT64;
                }
                return karabo::util::Types::UINT32;
            }
            return karabo::util::Types::INT32;
        }
        if (PyFloat_Check(obj.ptr())) {
            double b = bp::extract<double>(obj);
            any = b;
            return karabo::util::Types::DOUBLE;
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
            return karabo::util::Types::PTR_CHAR;       //TODO: Define WCHAR and PTR_WCHAR. Check with Burkhard and Martin
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
            PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj.ptr());
            int nd = PyArray_NDIM(arr);
            npy_intp* shapes = PyArray_DIMS(arr);
            int nelems = 1;
            for (int i = 0; i < nd; i++) nelems *= shapes[i];
            PyArray_Descr* dtype = PyArray_DESCR(arr);
            switch (dtype->type_num) {
                case NPY_BOOL:
                {
                    bool* data = reinterpret_cast<bool*> (PyArray_DATA(arr));
                    std::vector<bool> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_BOOL;
                }
                case NPY_SHORT:
                {
                    short* data = reinterpret_cast<short*> (PyArray_DATA(arr));
                    std::vector<short> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_INT16;
                }
                case NPY_USHORT:
                {
                    unsigned short* data = reinterpret_cast<unsigned short*> (PyArray_DATA(arr));
                    std::vector<unsigned short> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_UINT16;
                }
                case NPY_INT:
                {
                    int* data = reinterpret_cast<int*> (PyArray_DATA(arr));
                    std::vector<int> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_INT32;
                }
                case NPY_UINT:
                {
                    unsigned int* data = reinterpret_cast<unsigned int*> (PyArray_DATA(arr));
                    std::vector<unsigned int> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_UINT32;
                }
                case NPY_LONG:
                {
                    long* data = reinterpret_cast<long*> (PyArray_DATA(arr));
                    if (sizeof (long) == sizeof (int)) {
                        std::vector<int> v(data, data + nelems);
                        any = v;
                    } else if (sizeof (long) == sizeof (long long)) {
                        std::vector<long long> v(data, data + nelems);
                        any = v;
                    }
                    return karabo::util::Types::VECTOR_INT64;
                }
                case NPY_ULONG:
                {
                    unsigned long* data = reinterpret_cast<unsigned long*> (PyArray_DATA(arr));
                    if (sizeof (unsigned long) == sizeof (unsigned int)) {
                        std::vector<unsigned int> v(data, data + nelems);
                        any = v;
                    } else if (sizeof (unsigned long) == sizeof (unsigned long long)) {
                        std::vector<unsigned long long> v(data, data + nelems);
                        any = v;
                    }
                    std::vector<unsigned long long> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_UINT64;
                }
                case NPY_LONGLONG:
                {
                    long long* data = reinterpret_cast<long long*> (PyArray_DATA(arr));
                    std::vector<long long> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_INT64;
                }
                case NPY_ULONGLONG:
                {
                    unsigned long long* data = reinterpret_cast<unsigned long long*> (PyArray_DATA(arr));
                    std::vector<unsigned long long> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_UINT64;
                }
                case NPY_FLOAT:
                {
                    float* data = reinterpret_cast<float*> (PyArray_DATA(arr));
                    std::vector<float> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_FLOAT;
                }
                case NPY_DOUBLE:
                {
                    double* data = reinterpret_cast<double*> (PyArray_DATA(arr));
                    std::vector<double> v(data, data + nelems);
                    any = v;
                    return karabo::util::Types::VECTOR_DOUBLE;
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
        if (hasattr(obj, "__name__")) {// python function
            any = obj;
            return karabo::util::Types::ANY;
        }
        throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
    }
}

