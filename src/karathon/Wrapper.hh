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
 * File:   Wrapper.hh
 * Author: esenov
 *
 * Created on March 17, 2013, 10:52 PM
 */

#ifndef WRAPPER_HH
#define WRAPPER_HH

#include <boost/any.hpp>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <karabo/util/Exception.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/MetaTools.hh>
#include <karabo/util/NDArray.hh>
#include <string>
#include <tuple>
#include <typeinfo>

#include "FromNumpy.hh"
#include "ToNumpy.hh"

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#define KRB_NDARRAY_MIN_DIM 1
#define KRB_NDARRAY_MAX_DIM 6

#define NO_IMPORT_ARRAY
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include "ScopedGILAcquire.hh"

namespace bp = boost::python;

namespace karathon {

    class PyTypes {
       public:
        enum ReferenceType {

            BOOL = karabo::util::Types::BOOL,               // boolean
            VECTOR_BOOL = karabo::util::Types::VECTOR_BOOL, // std::vector<std::bool>

            CHAR = karabo::util::Types::CHAR,                 // char
            VECTOR_CHAR = karabo::util::Types::VECTOR_CHAR,   // std::vector<char>
            INT8 = karabo::util::Types::INT8,                 // signed char
            VECTOR_INT8 = karabo::util::Types::VECTOR_INT8,   // std::vector<std::signed char>
            UINT8 = karabo::util::Types::UINT8,               // unsigned char
            VECTOR_UINT8 = karabo::util::Types::VECTOR_UINT8, // std::vector<std::unsigned char>

            INT16 = karabo::util::Types::INT16,                 // signed short
            VECTOR_INT16 = karabo::util::Types::VECTOR_INT16,   // std::vector<std::signed short>
            UINT16 = karabo::util::Types::UINT16,               // unsigned short
            VECTOR_UINT16 = karabo::util::Types::VECTOR_UINT16, // std::vector<std::unsigned short>

            INT32 = karabo::util::Types::INT32,                 // signed int
            VECTOR_INT32 = karabo::util::Types::VECTOR_INT32,   // std::vector<std::int>
            UINT32 = karabo::util::Types::UINT32,               // unsigned int
            VECTOR_UINT32 = karabo::util::Types::VECTOR_UINT32, // std::vector<std::unsigned int>

            INT64 = karabo::util::Types::INT64,                 // signed long long
            VECTOR_INT64 = karabo::util::Types::VECTOR_INT64,   // std::vector<std::signed long long>
            UINT64 = karabo::util::Types::UINT64,               // unsigned long long
            VECTOR_UINT64 = karabo::util::Types::VECTOR_UINT64, // std::vector<std::unsigned long long>

            FLOAT = karabo::util::Types::FLOAT,               // float
            VECTOR_FLOAT = karabo::util::Types::VECTOR_FLOAT, // std::vector<std::float>

            DOUBLE = karabo::util::Types::DOUBLE,               // double
            VECTOR_DOUBLE = karabo::util::Types::VECTOR_DOUBLE, // std::vector<std::double>

            COMPLEX_FLOAT = karabo::util::Types::COMPLEX_FLOAT,               // std::complex<float>
            VECTOR_COMPLEX_FLOAT = karabo::util::Types::VECTOR_COMPLEX_FLOAT, // std::vector<std::complex<float>

            COMPLEX_DOUBLE = karabo::util::Types::COMPLEX_DOUBLE,               // std::complex<double>
            VECTOR_COMPLEX_DOUBLE = karabo::util::Types::VECTOR_COMPLEX_DOUBLE, // std::vector<std::complex<double>

            STRING = karabo::util::Types::STRING,               // std::string
            VECTOR_STRING = karabo::util::Types::VECTOR_STRING, // std::vector<std::string>

            HASH = karabo::util::Types::HASH,               // Hash
            VECTOR_HASH = karabo::util::Types::VECTOR_HASH, // std::vector<Hash>

            PTR_BOOL = karabo::util::Types::PTR_BOOL,
            PTR_CHAR = karabo::util::Types::PTR_CHAR,
            PTR_INT8 = karabo::util::Types::PTR_INT8,
            PTR_UINT8 = karabo::util::Types::PTR_UINT8,
            PTR_INT16 = karabo::util::Types::PTR_INT16,
            PTR_UINT16 = karabo::util::Types::PTR_UINT16,
            PTR_INT32 = karabo::util::Types::PTR_INT32,
            PTR_UINT32 = karabo::util::Types::PTR_UINT32,
            PTR_INT64 = karabo::util::Types::PTR_INT64,
            PTR_UINT64 = karabo::util::Types::PTR_UINT64,
            PTR_FLOAT = karabo::util::Types::PTR_FLOAT,
            PTR_DOUBLE = karabo::util::Types::PTR_DOUBLE,
            PTR_COMPLEX_FLOAT = karabo::util::Types::PTR_COMPLEX_FLOAT,
            PTR_COMPLEX_DOUBLE = karabo::util::Types::PTR_COMPLEX_DOUBLE,
            PTR_STRING = karabo::util::Types::PTR_STRING,

            SCHEMA = karabo::util::Types::SCHEMA, // Schema

            ANY = karabo::util::Types::ANY,   // unspecified type
            NONE = karabo::util::Types::NONE, // CppNone type used during serialization/de-serialization
            VECTOR_NONE = karabo::util::Types::VECTOR_NONE,

            UNKNOWN = karabo::util::Types::UNKNOWN, // unknown type
            SIMPLE = karabo::util::Types::SIMPLE,
            SEQUENCE = karabo::util::Types::SEQUENCE,
            POINTER = karabo::util::Types::POINTER,

            HASH_POINTER = karabo::util::Types::HASH_POINTER,
            VECTOR_HASH_POINTER = karabo::util::Types::VECTOR_HASH_POINTER,
            LAST_CPP_TYPE = karabo::util::Types::VECTOR_HASH_POINTER + 1,
            PYTHON_DEFAULT, // global switch: treat std::vector as bp::list
            NUMPY_DEFAULT,  // global switch: treat std::vector as ndarray
        };

        static const ReferenceType from(const karabo::util::Types::ReferenceType& input) {
            switch (input) {
                case karabo::util::Types::BOOL:
                    return BOOL;
                case karabo::util::Types::VECTOR_BOOL:
                    return VECTOR_BOOL;
                case karabo::util::Types::CHAR:
                    return CHAR;
                case karabo::util::Types::VECTOR_CHAR:
                    return VECTOR_CHAR;
                case karabo::util::Types::INT8:
                    return INT8;
                case karabo::util::Types::VECTOR_INT8:
                    return VECTOR_INT8;
                case karabo::util::Types::UINT8:
                    return UINT8;
                case karabo::util::Types::VECTOR_UINT8:
                    return VECTOR_UINT8;
                case karabo::util::Types::INT16:
                    return INT16;
                case karabo::util::Types::VECTOR_INT16:
                    return VECTOR_INT16;
                case karabo::util::Types::UINT16:
                    return UINT16;
                case karabo::util::Types::VECTOR_UINT16:
                    return VECTOR_UINT16;
                case karabo::util::Types::INT32:
                    return INT32;
                case karabo::util::Types::VECTOR_INT32:
                    return VECTOR_INT32;
                case karabo::util::Types::UINT32:
                    return UINT32;
                case karabo::util::Types::VECTOR_UINT32:
                    return VECTOR_UINT32;
                case karabo::util::Types::INT64:
                    return INT64;
                case karabo::util::Types::VECTOR_INT64:
                    return VECTOR_INT64;
                case karabo::util::Types::UINT64:
                    return UINT64;
                case karabo::util::Types::VECTOR_UINT64:
                    return VECTOR_UINT64;
                case karabo::util::Types::FLOAT:
                    return FLOAT;
                case karabo::util::Types::VECTOR_FLOAT:
                    return VECTOR_FLOAT;
                case karabo::util::Types::DOUBLE:
                    return DOUBLE;
                case karabo::util::Types::VECTOR_DOUBLE:
                    return VECTOR_DOUBLE;
                case karabo::util::Types::COMPLEX_FLOAT:
                    return COMPLEX_FLOAT;
                case karabo::util::Types::VECTOR_COMPLEX_FLOAT:
                    return VECTOR_COMPLEX_FLOAT;
                case karabo::util::Types::COMPLEX_DOUBLE:
                    return COMPLEX_DOUBLE;
                case karabo::util::Types::VECTOR_COMPLEX_DOUBLE:
                    return VECTOR_COMPLEX_DOUBLE;
                case karabo::util::Types::STRING:
                    return STRING;
                case karabo::util::Types::VECTOR_STRING:
                    return VECTOR_STRING;
                case karabo::util::Types::HASH:
                    return HASH;
                case karabo::util::Types::VECTOR_HASH:
                    return VECTOR_HASH;
                case karabo::util::Types::PTR_BOOL:
                    return PTR_BOOL;
                case karabo::util::Types::PTR_CHAR:
                    return PTR_CHAR;
                case karabo::util::Types::PTR_INT8:
                    return PTR_INT8;
                case karabo::util::Types::PTR_UINT8:
                    return PTR_UINT8;
                case karabo::util::Types::PTR_INT16:
                    return PTR_INT16;
                case karabo::util::Types::PTR_UINT16:
                    return PTR_UINT16;
                case karabo::util::Types::PTR_INT32:
                    return PTR_INT32;
                case karabo::util::Types::PTR_UINT32:
                    return PTR_UINT32;
                case karabo::util::Types::PTR_INT64:
                    return PTR_INT64;
                case karabo::util::Types::PTR_UINT64:
                    return PTR_UINT64;
                case karabo::util::Types::PTR_FLOAT:
                    return PTR_FLOAT;
                case karabo::util::Types::PTR_DOUBLE:
                    return PTR_DOUBLE;
                case karabo::util::Types::PTR_COMPLEX_FLOAT:
                    return PTR_COMPLEX_FLOAT;
                case karabo::util::Types::PTR_COMPLEX_DOUBLE:
                    return PTR_COMPLEX_DOUBLE;
                case karabo::util::Types::PTR_STRING:
                    return PTR_STRING;
                case karabo::util::Types::SCHEMA:
                    return SCHEMA;
                case karabo::util::Types::ANY:
                    return ANY;
                case karabo::util::Types::NONE:
                    return NONE;
                case karabo::util::Types::VECTOR_NONE:
                    return VECTOR_NONE;
                case karabo::util::Types::UNKNOWN:
                    return UNKNOWN;
                case karabo::util::Types::SIMPLE:
                    return SIMPLE;
                case karabo::util::Types::SEQUENCE:
                    return SEQUENCE;
                case karabo::util::Types::POINTER:
                    return POINTER;
                case karabo::util::Types::HASH_POINTER:
                    return HASH_POINTER;
                case karabo::util::Types::VECTOR_HASH_POINTER:
                    return VECTOR_HASH_POINTER;
                default:
                    throw KARABO_PYTHON_EXCEPTION("Unknown type encountered while converting from Types to PyTypes.");
            }
        }

        static const karabo::util::Types::ReferenceType to(const ReferenceType& input) {
            switch (input) {
                case BOOL:
                    return karabo::util::Types::BOOL;
                case VECTOR_BOOL:
                    return karabo::util::Types::VECTOR_BOOL;
                case CHAR:
                    return karabo::util::Types::CHAR;
                case VECTOR_CHAR:
                    return karabo::util::Types::VECTOR_CHAR;
                case INT8:
                    return karabo::util::Types::INT8;
                case VECTOR_INT8:
                    return karabo::util::Types::VECTOR_INT8;
                case UINT8:
                    return karabo::util::Types::UINT8;
                case VECTOR_UINT8:
                    return karabo::util::Types::VECTOR_UINT8;
                case INT16:
                    return karabo::util::Types::INT16;
                case VECTOR_INT16:
                    return karabo::util::Types::VECTOR_INT16;
                case UINT16:
                    return karabo::util::Types::UINT16;
                case VECTOR_UINT16:
                    return karabo::util::Types::VECTOR_UINT16;
                case INT32:
                    return karabo::util::Types::INT32;
                case VECTOR_INT32:
                    return karabo::util::Types::VECTOR_INT32;
                case UINT32:
                    return karabo::util::Types::UINT32;
                case VECTOR_UINT32:
                    return karabo::util::Types::VECTOR_UINT32;
                case INT64:
                    return karabo::util::Types::INT64;
                case VECTOR_INT64:
                    return karabo::util::Types::VECTOR_INT64;
                case UINT64:
                    return karabo::util::Types::UINT64;
                case VECTOR_UINT64:
                    return karabo::util::Types::VECTOR_UINT64;
                case FLOAT:
                    return karabo::util::Types::FLOAT;
                case VECTOR_FLOAT:
                    return karabo::util::Types::VECTOR_FLOAT;
                case DOUBLE:
                    return karabo::util::Types::DOUBLE;
                case VECTOR_DOUBLE:
                    return karabo::util::Types::VECTOR_DOUBLE;
                case COMPLEX_FLOAT:
                    return karabo::util::Types::COMPLEX_FLOAT;
                case VECTOR_COMPLEX_FLOAT:
                    return karabo::util::Types::VECTOR_COMPLEX_FLOAT;
                case COMPLEX_DOUBLE:
                    return karabo::util::Types::COMPLEX_DOUBLE;
                case VECTOR_COMPLEX_DOUBLE:
                    return karabo::util::Types::VECTOR_COMPLEX_DOUBLE;
                case STRING:
                    return karabo::util::Types::STRING;
                case VECTOR_STRING:
                    return karabo::util::Types::VECTOR_STRING;
                case HASH:
                    return karabo::util::Types::HASH;
                case VECTOR_HASH:
                    return karabo::util::Types::VECTOR_HASH;
                case PTR_BOOL:
                    return karabo::util::Types::PTR_BOOL;
                case PTR_CHAR:
                    return karabo::util::Types::PTR_CHAR;
                case PTR_INT8:
                    return karabo::util::Types::PTR_INT8;
                case PTR_UINT8:
                    return karabo::util::Types::PTR_UINT8;
                case PTR_INT16:
                    return karabo::util::Types::PTR_INT16;
                case PTR_UINT16:
                    return karabo::util::Types::PTR_UINT16;
                case PTR_INT32:
                    return karabo::util::Types::PTR_INT32;
                case PTR_UINT32:
                    return karabo::util::Types::PTR_UINT32;
                case PTR_INT64:
                    return karabo::util::Types::PTR_INT64;
                case PTR_UINT64:
                    return karabo::util::Types::PTR_UINT64;
                case PTR_FLOAT:
                    return karabo::util::Types::PTR_FLOAT;
                case PTR_DOUBLE:
                    return karabo::util::Types::PTR_DOUBLE;
                case PTR_COMPLEX_FLOAT:
                    return karabo::util::Types::PTR_COMPLEX_FLOAT;
                case PTR_COMPLEX_DOUBLE:
                    return karabo::util::Types::PTR_COMPLEX_DOUBLE;
                case PTR_STRING:
                    return karabo::util::Types::PTR_STRING;
                case SCHEMA:
                    return karabo::util::Types::SCHEMA;
                case ANY:
                    return karabo::util::Types::ANY;
                case NONE:
                    return karabo::util::Types::NONE;
                case VECTOR_NONE:
                    return karabo::util::Types::VECTOR_NONE;
                case UNKNOWN:
                    return karabo::util::Types::UNKNOWN;
                case SIMPLE:
                    return karabo::util::Types::SIMPLE;
                case SEQUENCE:
                    return karabo::util::Types::SEQUENCE;
                case POINTER:
                    return karabo::util::Types::POINTER;
                case HASH_POINTER:
                    return karabo::util::Types::HASH_POINTER;
                case VECTOR_HASH_POINTER:
                    return karabo::util::Types::VECTOR_HASH_POINTER;
                default:
                    throw KARABO_PYTHON_EXCEPTION(
                          "Unsupported type encountered while converting from PyTypes to Types.");
            }
        }

        static const ReferenceType category(int type) {
            return from(karabo::util::Types::category(type));
        }
    };

    // Handler class for arrays originating in C++ code

    class CppArrayRefHandler {
       private:
        karabo::util::NDArray::DataPointer m_dataPtr;

       public:
        explicit CppArrayRefHandler(const karabo::util::NDArray::DataPointer& dataPtr) : m_dataPtr(dataPtr) {}

        karabo::util::NDArray::DataPointer getDataPtr() const {
            return m_dataPtr;
        }
    };

    // Handler class for arrays originating in Python code

    class PyArrayRefHandler {
        PyArrayObject* m_arrayRef;

       public:
        explicit PyArrayRefHandler(PyArrayObject* obj) : m_arrayRef(obj) {}

        void operator()(const char*) {
            Py_DECREF(m_arrayRef);
        }
    };

    namespace detail {
        /// Helper when catching Python exceptions
        void treatError_already_set(const bp::object& handler, const char* where);
    } // namespace detail

    struct Wrapper {
        struct null_deleter {
            void operator()(void const*) const {}
        };

        static bool hasattr(bp::object obj, const std::string& attrName) {
            // NOTE: There seems to be different implementations of the Python C-API around
            // Some use a char* some other a const char* -> char* is the always compiling alternative
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*>(attrName.c_str()));
        }

        template <class T, class U>
        static bp::tuple fromStdPairToPyTuple(const std::pair<T, U>& p) {
            return bp::make_tuple(p.first, p.second);
        }

        template <class ValueType>
        static bp::list fromStdVectorToPyList(const std::vector<ValueType>& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::object(v[i]));
            return pylist;
        }

        template <class T, class U>
        static bp::list fromStdVectorToPyList(const std::vector<std::pair<T, U> >& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::make_tuple(v[i].first, v[i].second));
            return pylist;
        }

        template <class ValueType>
        static bp::tuple fromStdVectorToPyTuple(const std::vector<ValueType>& v) {
            return bp::tuple(fromStdVectorToPyList(v));
        }

        static bp::list fromStdVectorToPyHashList(const std::vector<karabo::util::Hash>& v) {
            bp::object it = bp::iterator<std::vector<karabo::util::Hash> >();
            bp::object iter = it(v);
            bp::list l(iter);
            return l;
        }

        static bp::object fromStdVectorToPyBytes(const std::vector<char>& v) {
            const char* data = &v[0];
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyBytes_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyBytes(const std::vector<signed char>& v) {
            const char* data = reinterpret_cast<const char*>(&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyBytes_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyBytes(const std::vector<unsigned char>& v) {
            const char* data = reinterpret_cast<const char*>(&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyBytes_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<char>& v) {
            const char* data = &v[0];
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<signed char>& v) {
            const char* data = reinterpret_cast<const char*>(&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<unsigned char>& v) {
            const char* data = reinterpret_cast<const char*>(&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::str fromStdVectorToPyStr(const std::vector<char>& v) {
            return bp::str(&v[0], v.size());
        }

        static bp::str fromStdVectorToPyStr(const std::vector<signed char>& v) {
            return bp::str(reinterpret_cast<const char*>(&v[0]), v.size());
        }

        static bp::str fromStdVectorToPyStr(const std::vector<unsigned char>& v) {
            return bp::str(reinterpret_cast<const char*>(&v[0]), v.size());
        }

        template <class T>
        static std::vector<T> fromPyListToStdVector(const bp::object& obj) {
            const bp::list& lst = bp::extract<bp::list>(obj);
            bp::ssize_t size = bp::len(lst);

            std::vector<T> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<T>(lst[i]);
            }
            return v;
        }

        template <class T>
        static std::vector<T> fromPyTupleToStdVector(const bp::object& obj) {
            const bp::tuple& tpl = bp::extract<bp::tuple>(obj);
            bp::ssize_t size = bp::len(tpl);

            std::vector<T> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<T>(tpl[i]);
            }
            return v;
        }

        /**
         * Convert a Python iterable containing objects convertible to T to C++ container CONT<T>.
         * The container type must be constructable from an iterator range.
         *
         * Example:
         * const bp::object& obj = ...; // list, tuple, dictionary [where you get the keys],...
         * const std::set<int> setOfInt = fromPyIterableToContainer<int, std::set>(o);
         *
         * @param obj the Python object
         * @return C++ container of type CONT<T>
         */
        template <typename T, template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector>
        static CONT<T> fromPyIterableToCppContainer(const bp::object& obj) {
            bp::stl_input_iterator<T> begin(obj), end;
            return CONT<T>(begin, end);
        }

        static bp::object toObject(const boost::any& operand, bool numpyFlag = false);

        static bp::object toCustomObject(karabo::util::Hash::Node& node);

        /**
         * Convert obj to desired IntegerType which should be unsigned short, int, long long, etc.
         *
         * @param obj must be a Python object representing an integer - if not, a karabo::util::CastException is thrown
         * @return the properly typed integer value
         */
        template <class IntegerType>
        static IntegerType toInteger(const bp::object& obj);

        static karabo::util::Types::ReferenceType toAny(const bp::object& operand, boost::any& any);

        static bp::object fromStdVectorToPyListNone(const std::vector<karabo::util::CppNone>& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::object());
            return pylist;
        }

        template <class ValueType>
        static bp::object fromStdVectorToPyArray(const std::vector<ValueType>& v, bool numpyFlag = false) {
            return fromStdVectorToPyList(v);
        }

        static karabo::util::NDArray fromPyArrayToNDArray(PyArrayObject* arr);

        static bp::object fromNDArrayToPyArray(const karabo::util::NDArray& ndarray);

        /**
         * How many non-keyword arguments does 'callable' expect?
         *
         * Works for free functions, member methods and objects with simple __call__ method (even with @staticmethod),
         * but not for functools.partial objects.
         *
         */
        static size_t numArgs(const bp::object& callable);

        /**
         * Inner recursive copier for deepCopy
         * @param h
         * @return
         */
        static karabo::util::Hash deepCopy_r(const karabo::util::Hash& h);

        /**
         * A recursively copying function, that guarantees that no references or
         * pointers remain in a Hash. Use if correct object lifetime management
         * when passing to python cannot be guaranteed.
         * @param obj potentially containing Hash reference or pointer as well as vectors thereof to be deep copied
         * @return a deep copy of obj
         */
        static bp::object deepCopyHashLike(const bp::object& obj);

        /**
         * A proxy around a Python handler to be called with an arbitrary number of arguments
         *
         * @param handler A callable Python object
         * @param which A C-string used to better identify the point of failure in case the Python
         *              code throws an exception
         * @param args An arbitrary number of arguments passed to the handler - each has to be a valid
         *             argument to the constructor of the boost Python object.
         */
        template <typename... Args>
        static void proxyHandler(const bp::object& handler, const char* which, const Args&... args) {
            ScopedGILAcquire gil;
            try {
                if (handler) {
                    // Just call handler with individually unpacked arguments:
                    handler(bp::object(args)...);
                }
            } catch (const bp::error_already_set& e) {
                detail::treatError_already_set(handler, which);
            } catch (...) {
                KARABO_RETHROW
            }
        }
    };

    /** A functor to wrap a Python handler such that
     *  - handler is called with GIL,
     *  - exceptions in handler calls are properly treated and forwarded,
     *  - handler's destructor is called under GIL.
     *
     * Template types are the C++ arguments to be passed to the handler.
     * Caveat:
     * For types where the automatic conversion does not work (e.g. boost::any and std:vector<..>,)
     * a specialised implementation for the operator() might be needed, using e.g. the
     * Wrapper::toObject or Wrapper::from[...]ToPy[...] methods for conversion, see e.g. class HandlerWrapAny1.
     */
    template <typename... Args> // if needed, could specify return type of operator() as fixed first template argument
    class HandlerWrap {
       public:
        /**
         * Construct a wrapper for a Python handler
         * @param handler the Python callable to wrap
         * @param where a C string identifying which handler is wrapped,
         *              for debugging only, i.e. used if a call to the handler raises an exception
         */
        HandlerWrap(const bp::object& handler, char const* const where)
            : m_handler(std::make_shared<bp::object>(handler)), // new object on the heap to control its destruction
              m_where(where) {}


        ~HandlerWrap() {
            // Ensure that destructor of Python handler object is called with GIL
            ScopedGILAcquire gil;
            m_handler.reset();
        }


        void operator()(Args... args) const {
            ScopedGILAcquire gil;
            try {
                if (*m_handler) {
                    // Just call handler with individually unpacked arguments:
                    (*m_handler)(bp::object(args)...); // std::forward(args)?
                }
            } catch (const bp::error_already_set& e) {
                karathon::detail::treatError_already_set(*m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
        }

       protected: // not private - needed by HandlerWrapAny<N> and InputChannelWrap::DataHandlerWrap
        std::shared_ptr<bp::object> m_handler;
        char const* const m_where;
    };

    template <typename Ret, typename... Args>
    class ReturnHandlerWrap {
       public:
        /**
         * Construct a wrapper for a Python handler whose return value is of interest.
         * The return value of the handler must be castable to 'Ret'.
         * @param handler the Python callable to wrap
         * @param where a C string identifying which handler is wrapped,
         *              for debugging only, i.e. used if a call to the handler raises an exception
         */
        ReturnHandlerWrap(const bp::object& handler, char const* const where)
            : m_handler(std::make_shared<bp::object>(handler)), // new object on the heap to control its destruction
              m_where(where) {}


        ~ReturnHandlerWrap() {
            // Ensure that destructor of Python handler object is called with GIL
            ScopedGILAcquire gil;
            m_handler.reset();
        }


        Ret operator()(Args... args) const {
            ScopedGILAcquire gil;
            bp::object pyResult;
            try {
                if (*m_handler) {
                    // Just call handler with individually unpacked arguments
                    pyResult = (*m_handler)(bp::object(args)...); // std::forward(args)?
                }
            } catch (bp::error_already_set& e) {
                detail::treatError_already_set(*m_handler, m_where);
            } catch (...) {
                KARABO_RETHROW
            }
            return bp::extract<Ret>(pyResult);
        }

       private: // may become protected if a derived class needs to overwrite operator()
        std::shared_ptr<bp::object> m_handler;
        char const* const m_where;
    };

    /**
     * Specialisation of HandlerWrap for one boost::any argument
     *
     * The argument is converted to boost::object before passed to the Python handler.
     */
    class HandlerWrapAny1 : public HandlerWrap<const boost::any&> {
       public:
        HandlerWrapAny1(const bp::object& handler, char const* const where)
            : HandlerWrap<const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1) const;
    };

    /**
     * Specialisation of HandlerWrap for two boost::any arguments
     *
     * The arguments are converted to boost::object before passed to the Python handler.
     */
    class HandlerWrapAny2 : public HandlerWrap<const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny2(const bp::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2) const;
    };

    /**
     * Specialisation of HandlerWrap for three boost::any arguments
     *
     * The arguments are converted to boost::object before passed to the Python handler.
     */
    class HandlerWrapAny3 : public HandlerWrap<const boost::any&, const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny3(const bp::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3) const;
    };

    /**
     * Specialisation of HandlerWrap for four boost::any arguments
     *
     * The arguments are converted to boost::object before passed to the Python handler.
     */
    class HandlerWrapAny4
        : public HandlerWrap<const boost::any&, const boost::any&, const boost::any&, const boost::any&> {
       public:
        HandlerWrapAny4(const bp::object& handler, char const* const where)
            : HandlerWrap<const boost::any&, const boost::any&, const boost::any&, const boost::any&>(handler, where) {}

        void operator()(const boost::any& a1, const boost::any& a2, const boost::any& a3, const boost::any& a4) const;
    };

    /**
     * Specialisation of HandlerWrap for two vector<unsigned long long> arguments
     *
     * The arguments are converted to boost::object before passed to the Python handler.
     */
    class HandlerWrapVullVull
        : public HandlerWrap<const std::vector<unsigned long long>&, const std::vector<unsigned long long>&> {
       public:
        HandlerWrapVullVull(const bp::object& handler, char const* const where)
            : HandlerWrap<const std::vector<unsigned long long>&, const std::vector<unsigned long long>&>(handler,
                                                                                                          where) {}

        void operator()(const std::vector<unsigned long long>& v1, const std::vector<unsigned long long>& v2) const;
    };

    /**
     * Provide exception text and details (i.e. traceback)
     * To be called from within a "catch (const bp::error_already_set& e)" block.
     *
     * @return tuple of two strings:
     *         - first is (Python) exception text,
     *         - second the multiline traceback like from traceback.print_exception (skipping last line)
     */
    std::tuple<std::string, std::string> getPythonExceptionStrings();

    // Specializations
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<bool>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<short>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned short>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<int>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned int>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<long long>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned long long>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<float>& v, bool numpyFlag);
    template <>
    bp::object Wrapper::fromStdVectorToPyArray(const std::vector<double>& v, bool numpyFlag);

    // Implementations of template code
    template <class IntegerType>
    IntegerType Wrapper::toInteger(const bp::object& obj) {
        if (PyLong_Check(obj.ptr())) {
            const PY_LONG_LONG value = PyLong_AsLongLong(obj.ptr());
            return static_cast<IntegerType>(value);
        } else {
            throw KARABO_CAST_EXCEPTION("Cannot cast Python object to '" + std::string(typeid(IntegerType).name()) +=
                                        "'");
            return static_cast<IntegerType>(0); // please the compiler
        }
    }
} // namespace karathon

#endif /* WRAPPER_HH */
