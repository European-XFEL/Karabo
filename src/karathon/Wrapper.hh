/* 
 * File:   Wrapper.hh
 * Author: esenov
 *
 * Created on March 17, 2013, 10:52 PM
 */

#ifndef WRAPPER_HH
#define	WRAPPER_HH

#include <boost/python.hpp>
#include <boost/any.hpp>
#include <karabo/util/Hash.hh>

#define PY_ARRAY_UNIQUE_SYMBOL karabo_ARRAY_API
#define NO_IMPORT_ARRAY
#include <numpy/arrayobject.h>

namespace bp = boost::python;

namespace karathon {

    class PyTypes {

        public:

        enum ReferenceType {


            BOOL = karabo::util::Types::BOOL, // boolean
            VECTOR_BOOL = karabo::util::Types::VECTOR_BOOL, // std::vector<std::bool>

            CHAR = karabo::util::Types::CHAR, // char
            VECTOR_CHAR = karabo::util::Types::VECTOR_CHAR, // std::vector<char>
            INT8 = karabo::util::Types::INT8, // signed char
            VECTOR_INT8 = karabo::util::Types::VECTOR_INT8, // std::vector<std::signed char>
            UINT8 = karabo::util::Types::UINT8, // unsigned char
            VECTOR_UINT8 = karabo::util::Types::VECTOR_UINT8, // std::vector<std::unsigned char>

            INT16 = karabo::util::Types::INT16, // signed short
            VECTOR_INT16 = karabo::util::Types::VECTOR_INT16, // std::vector<std::signed short>
            UINT16 = karabo::util::Types::UINT16, // unsigned short
            VECTOR_UINT16 = karabo::util::Types::VECTOR_UINT16, // std::vector<std::unsigned short>

            INT32 = karabo::util::Types::INT32, // signed int
            VECTOR_INT32 = karabo::util::Types::VECTOR_INT32, // std::vector<std::int>
            UINT32 = karabo::util::Types::UINT32, // unsigned int
            VECTOR_UINT32 = karabo::util::Types::VECTOR_UINT32, // std::vector<std::unsigned int>

            INT64 = karabo::util::Types::INT64, // signed long long
            VECTOR_INT64 = karabo::util::Types::VECTOR_INT64, // std::vector<std::signed long long>
            UINT64 = karabo::util::Types::UINT64, // unsigned long long
            VECTOR_UINT64 = karabo::util::Types::VECTOR_UINT64, // std::vector<std::unsigned long long>

            FLOAT = karabo::util::Types::FLOAT, // float
            VECTOR_FLOAT = karabo::util::Types::VECTOR_FLOAT, // std::vector<std::float>

            DOUBLE = karabo::util::Types::DOUBLE, // double
            VECTOR_DOUBLE = karabo::util::Types::VECTOR_DOUBLE, // std::vector<std::double>

            COMPLEX_FLOAT = karabo::util::Types::COMPLEX_FLOAT, // std::complex<float>
            VECTOR_COMPLEX_FLOAT = karabo::util::Types::VECTOR_COMPLEX_FLOAT, // std::vector<std::complex<float>

            COMPLEX_DOUBLE = karabo::util::Types::COMPLEX_DOUBLE, // std::complex<double>
            VECTOR_COMPLEX_DOUBLE = karabo::util::Types::VECTOR_COMPLEX_DOUBLE, // std::vector<std::complex<double>

            STRING = karabo::util::Types::STRING, // std::string
            VECTOR_STRING = karabo::util::Types::VECTOR_STRING, // std::vector<std::string>

            HASH = karabo::util::Types::HASH, // Hash
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

            ANY = karabo::util::Types::ANY, // unspecified type
            NONE = karabo::util::Types::NONE, // CppNone type used during serialization/de-serialization
            VECTOR_NONE = karabo::util::Types::VECTOR_NONE,

            UNKNOWN = karabo::util::Types::UNKNOWN, // unknown type
            SIMPLE = karabo::util::Types::SIMPLE,
            SEQUENCE = karabo::util::Types::SEQUENCE,
            POINTER = karabo::util::Types::POINTER,
            RAW_ARRAY = karabo::util::Types::RAW_ARRAY,
            NDARRAY = karabo::util::Types::NDARRAY,

            ARRAY_BOOL = karabo::util::Types::ARRAY_BOOL,
            ARRAY_CHAR = karabo::util::Types::ARRAY_CHAR,
            ARRAY_INT8 = karabo::util::Types::ARRAY_INT8,
            ARRAY_UINT8 = karabo::util::Types::ARRAY_UINT8,
            ARRAY_INT16 = karabo::util::Types::ARRAY_INT16,
            ARRAY_UINT16 = karabo::util::Types::ARRAY_UINT16,
            ARRAY_INT32 = karabo::util::Types::ARRAY_INT32,
            ARRAY_UINT32 = karabo::util::Types::ARRAY_UINT32,
            ARRAY_INT64 = karabo::util::Types::ARRAY_INT64,
            ARRAY_UINT64 = karabo::util::Types::ARRAY_UINT64,
            ARRAY_FLOAT = karabo::util::Types::ARRAY_FLOAT,
            ARRAY_DOUBLE = karabo::util::Types::ARRAY_DOUBLE,

            NDARRAY_BOOL = karabo::util::Types::NDARRAY_BOOL,
            NDARRAY_INT8 = karabo::util::Types::NDARRAY_INT8,
            NDARRAY_UINT8 = karabo::util::Types::NDARRAY_UINT8,
            NDARRAY_INT16 = karabo::util::Types::NDARRAY_INT16,
            NDARRAY_UINT16 = karabo::util::Types::NDARRAY_UINT16,
            NDARRAY_INT32 = karabo::util::Types::NDARRAY_INT32,
            NDARRAY_UINT32 = karabo::util::Types::NDARRAY_UINT32,
            NDARRAY_INT64 = karabo::util::Types::NDARRAY_INT64,
            NDARRAY_UINT64 = karabo::util::Types::NDARRAY_UINT64,
            NDARRAY_FLOAT = karabo::util::Types::NDARRAY_FLOAT,
            NDARRAY_DOUBLE = karabo::util::Types::NDARRAY_DOUBLE,

            HASH_POINTER = karabo::util::Types::HASH_POINTER,
            VECTOR_HASH_POINTER = karabo::util::Types::VECTOR_HASH_POINTER,
            LAST_CPP_TYPE = karabo::util::Types::VECTOR_HASH_POINTER + 1,
            PYTHON_DEFAULT, // global switch: treat std::vector as bp::list
            NUMPY_DEFAULT, // global switch: treat std::vector as ndarray
            NDARRAY_COMPLEX_FLOAT,
            NDARRAY_COMPLEX_DOUBLE
        };

        static const ReferenceType from(const karabo::util::Types::ReferenceType& input) {
            switch (input) {
                case karabo::util::Types::BOOL: return BOOL;
                case karabo::util::Types::VECTOR_BOOL: return VECTOR_BOOL;
                case karabo::util::Types::CHAR: return CHAR;
                case karabo::util::Types::VECTOR_CHAR: return VECTOR_CHAR;
                case karabo::util::Types::INT8: return INT8;
                case karabo::util::Types::VECTOR_INT8: return VECTOR_INT8;
                case karabo::util::Types::UINT8: return UINT8;
                case karabo::util::Types::VECTOR_UINT8: return VECTOR_UINT8;
                case karabo::util::Types::INT16: return INT16;
                case karabo::util::Types::VECTOR_INT16: return VECTOR_INT16;
                case karabo::util::Types::UINT16: return UINT16;
                case karabo::util::Types::VECTOR_UINT16: return VECTOR_UINT16;
                case karabo::util::Types::INT32: return INT32;
                case karabo::util::Types::VECTOR_INT32: return VECTOR_INT32;
                case karabo::util::Types::UINT32: return UINT32;
                case karabo::util::Types::VECTOR_UINT32: return VECTOR_UINT32;
                case karabo::util::Types::INT64: return INT64;
                case karabo::util::Types::VECTOR_INT64: return VECTOR_INT64;
                case karabo::util::Types::UINT64: return UINT64;
                case karabo::util::Types::VECTOR_UINT64: return VECTOR_UINT64;
                case karabo::util::Types::FLOAT: return FLOAT;
                case karabo::util::Types::VECTOR_FLOAT: return VECTOR_FLOAT;
                case karabo::util::Types::DOUBLE: return DOUBLE;
                case karabo::util::Types::VECTOR_DOUBLE: return VECTOR_DOUBLE;
                case karabo::util::Types::COMPLEX_FLOAT: return COMPLEX_FLOAT;
                case karabo::util::Types::VECTOR_COMPLEX_FLOAT: return VECTOR_COMPLEX_FLOAT;
                case karabo::util::Types::COMPLEX_DOUBLE: return COMPLEX_DOUBLE;
                case karabo::util::Types::VECTOR_COMPLEX_DOUBLE: return VECTOR_COMPLEX_DOUBLE;
                case karabo::util::Types::STRING: return STRING;
                case karabo::util::Types::VECTOR_STRING: return VECTOR_STRING;
                case karabo::util::Types::HASH: return HASH;
                case karabo::util::Types::VECTOR_HASH: return VECTOR_HASH;
                case karabo::util::Types::PTR_BOOL: return PTR_BOOL;
                case karabo::util::Types::PTR_CHAR: return PTR_CHAR;
                case karabo::util::Types::PTR_INT8: return PTR_INT8;
                case karabo::util::Types::PTR_UINT8: return PTR_UINT8;
                case karabo::util::Types::PTR_INT16: return PTR_INT16;
                case karabo::util::Types::PTR_UINT16: return PTR_UINT16;
                case karabo::util::Types::PTR_INT32: return PTR_INT32;
                case karabo::util::Types::PTR_UINT32: return PTR_UINT32;
                case karabo::util::Types::PTR_INT64: return PTR_INT64;
                case karabo::util::Types::PTR_UINT64: return PTR_UINT64;
                case karabo::util::Types::PTR_FLOAT: return PTR_FLOAT;
                case karabo::util::Types::PTR_DOUBLE: return PTR_DOUBLE;
                case karabo::util::Types::PTR_COMPLEX_FLOAT: return PTR_COMPLEX_FLOAT;
                case karabo::util::Types::PTR_COMPLEX_DOUBLE: return PTR_COMPLEX_DOUBLE;
                case karabo::util::Types::PTR_STRING: return PTR_STRING;
                case karabo::util::Types::SCHEMA: return SCHEMA;
                case karabo::util::Types::ANY: return ANY;
                case karabo::util::Types::NONE: return NONE;
                case karabo::util::Types::VECTOR_NONE: return VECTOR_NONE;
                case karabo::util::Types::UNKNOWN: return UNKNOWN;
                case karabo::util::Types::SIMPLE: return SIMPLE;
                case karabo::util::Types::SEQUENCE: return SEQUENCE;
                case karabo::util::Types::POINTER: return POINTER;
                case karabo::util::Types::RAW_ARRAY: return RAW_ARRAY;
                case karabo::util::Types::ARRAY_BOOL: return ARRAY_BOOL;
                case karabo::util::Types::ARRAY_CHAR: return ARRAY_CHAR;
                case karabo::util::Types::ARRAY_INT8: return ARRAY_INT8;
                case karabo::util::Types::ARRAY_UINT8: return ARRAY_UINT8;
                case karabo::util::Types::ARRAY_INT16: return ARRAY_INT16;
                case karabo::util::Types::ARRAY_UINT16: return ARRAY_UINT16;
                case karabo::util::Types::ARRAY_INT32: return ARRAY_INT32;
                case karabo::util::Types::ARRAY_UINT32: return ARRAY_UINT32;
                case karabo::util::Types::ARRAY_INT64: return ARRAY_INT64;
                case karabo::util::Types::ARRAY_UINT64: return ARRAY_UINT64;
                case karabo::util::Types::ARRAY_FLOAT: return ARRAY_FLOAT;
                case karabo::util::Types::ARRAY_DOUBLE: return ARRAY_DOUBLE;
                case karabo::util::Types::NDARRAY_BOOL: return NDARRAY_BOOL;
                case karabo::util::Types::NDARRAY_INT8: return NDARRAY_INT8;
                case karabo::util::Types::NDARRAY_UINT8: return NDARRAY_UINT8;
                case karabo::util::Types::NDARRAY_INT16: return NDARRAY_INT16;
                case karabo::util::Types::NDARRAY_UINT16: return NDARRAY_UINT16;
                case karabo::util::Types::NDARRAY_INT32: return NDARRAY_INT32;
                case karabo::util::Types::NDARRAY_UINT32: return NDARRAY_UINT32;
                case karabo::util::Types::NDARRAY_INT64: return NDARRAY_INT64;
                case karabo::util::Types::NDARRAY_UINT64: return NDARRAY_UINT64;
                case karabo::util::Types::NDARRAY_FLOAT: return NDARRAY_FLOAT;
                case karabo::util::Types::NDARRAY_DOUBLE: return NDARRAY_DOUBLE;
                case karabo::util::Types::HASH_POINTER: return HASH_POINTER;
                case karabo::util::Types::VECTOR_HASH_POINTER: return VECTOR_HASH_POINTER;
                default:
                    throw KARABO_PYTHON_EXCEPTION("Unknown type encountered while converting from Types to PyTypes.");
            }
        }

        static const karabo::util::Types::ReferenceType to(const ReferenceType& input) {
            switch (input) {
                case BOOL: return karabo::util::Types::BOOL;
                case VECTOR_BOOL: return karabo::util::Types::VECTOR_BOOL;
                case CHAR: return karabo::util::Types::CHAR;
                case VECTOR_CHAR: return karabo::util::Types::VECTOR_CHAR;
                case INT8: return karabo::util::Types::INT8;
                case VECTOR_INT8: return karabo::util::Types::VECTOR_INT8;
                case UINT8: return karabo::util::Types::UINT8;
                case VECTOR_UINT8: return karabo::util::Types::VECTOR_UINT8;
                case INT16: return karabo::util::Types::INT16;
                case VECTOR_INT16: return karabo::util::Types::VECTOR_INT16;
                case UINT16: return karabo::util::Types::UINT16;
                case VECTOR_UINT16: return karabo::util::Types::VECTOR_UINT16;
                case INT32: return karabo::util::Types::INT32;
                case VECTOR_INT32: return karabo::util::Types::VECTOR_INT32;
                case UINT32: return karabo::util::Types::UINT32;
                case VECTOR_UINT32: return karabo::util::Types::VECTOR_UINT32;
                case INT64: return karabo::util::Types::INT64;
                case VECTOR_INT64: return karabo::util::Types::VECTOR_INT64;
                case UINT64: return karabo::util::Types::UINT64;
                case VECTOR_UINT64: return karabo::util::Types::VECTOR_UINT64;
                case FLOAT: return karabo::util::Types::FLOAT;
                case VECTOR_FLOAT: return karabo::util::Types::VECTOR_FLOAT;
                case DOUBLE: return karabo::util::Types::DOUBLE;
                case VECTOR_DOUBLE: return karabo::util::Types::VECTOR_DOUBLE;
                case COMPLEX_FLOAT: return karabo::util::Types::COMPLEX_FLOAT;
                case VECTOR_COMPLEX_FLOAT: return karabo::util::Types::VECTOR_COMPLEX_FLOAT;
                case COMPLEX_DOUBLE: return karabo::util::Types::COMPLEX_DOUBLE;
                case VECTOR_COMPLEX_DOUBLE: return karabo::util::Types::VECTOR_COMPLEX_DOUBLE;
                case STRING: return karabo::util::Types::STRING;
                case VECTOR_STRING: return karabo::util::Types::VECTOR_STRING;
                case HASH: return karabo::util::Types::HASH;
                case VECTOR_HASH: return karabo::util::Types::VECTOR_HASH;
                case PTR_BOOL: return karabo::util::Types::PTR_BOOL;
                case PTR_CHAR: return karabo::util::Types::PTR_CHAR;
                case PTR_INT8: return karabo::util::Types::PTR_INT8;
                case PTR_UINT8: return karabo::util::Types::PTR_UINT8;
                case PTR_INT16: return karabo::util::Types::PTR_INT16;
                case PTR_UINT16: return karabo::util::Types::PTR_UINT16;
                case PTR_INT32: return karabo::util::Types::PTR_INT32;
                case PTR_UINT32: return karabo::util::Types::PTR_UINT32;
                case PTR_INT64: return karabo::util::Types::PTR_INT64;
                case PTR_UINT64: return karabo::util::Types::PTR_UINT64;
                case PTR_FLOAT: return karabo::util::Types::PTR_FLOAT;
                case PTR_DOUBLE: return karabo::util::Types::PTR_DOUBLE;
                case PTR_COMPLEX_FLOAT: return karabo::util::Types::PTR_COMPLEX_FLOAT;
                case PTR_COMPLEX_DOUBLE: return karabo::util::Types::PTR_COMPLEX_DOUBLE;
                case PTR_STRING: return karabo::util::Types::PTR_STRING;
                case SCHEMA: return karabo::util::Types::SCHEMA;
                case ANY: return karabo::util::Types::ANY;
                case NONE: return karabo::util::Types::NONE;
                case VECTOR_NONE: return karabo::util::Types::VECTOR_NONE;
                case UNKNOWN: return karabo::util::Types::UNKNOWN;
                case SIMPLE: return karabo::util::Types::SIMPLE;
                case SEQUENCE: return karabo::util::Types::SEQUENCE;
                case POINTER: return karabo::util::Types::POINTER;
                case RAW_ARRAY: return karabo::util::Types::RAW_ARRAY;
                case ARRAY_BOOL: return karabo::util::Types::ARRAY_BOOL;
                case ARRAY_CHAR: return karabo::util::Types::ARRAY_CHAR;
                case ARRAY_INT8: return karabo::util::Types::ARRAY_INT8;
                case ARRAY_UINT8: return karabo::util::Types::ARRAY_UINT8;
                case ARRAY_INT16: return karabo::util::Types::ARRAY_INT16;
                case ARRAY_UINT16: return karabo::util::Types::ARRAY_UINT16;
                case ARRAY_INT32: return karabo::util::Types::ARRAY_INT32;
                case ARRAY_UINT32: return karabo::util::Types::ARRAY_UINT32;
                case ARRAY_INT64: return karabo::util::Types::ARRAY_INT64;
                case ARRAY_UINT64: return karabo::util::Types::ARRAY_UINT64;
                case ARRAY_FLOAT: return karabo::util::Types::ARRAY_FLOAT;
                case ARRAY_DOUBLE: return karabo::util::Types::ARRAY_DOUBLE;
                case NDARRAY_BOOL: return karabo::util::Types::NDARRAY_BOOL;
                case NDARRAY_INT8: return karabo::util::Types::NDARRAY_INT8;
                case NDARRAY_UINT8: return karabo::util::Types::NDARRAY_UINT8;
                case NDARRAY_INT16: return karabo::util::Types::NDARRAY_INT16;
                case NDARRAY_UINT16: return karabo::util::Types::NDARRAY_UINT16;
                case NDARRAY_INT32: return karabo::util::Types::NDARRAY_INT32;
                case NDARRAY_UINT32: return karabo::util::Types::NDARRAY_UINT32;
                case NDARRAY_INT64: return karabo::util::Types::NDARRAY_INT64;
                case NDARRAY_UINT64: return karabo::util::Types::NDARRAY_UINT64;
                case NDARRAY_FLOAT: return karabo::util::Types::NDARRAY_FLOAT;
                case NDARRAY_DOUBLE: return karabo::util::Types::NDARRAY_DOUBLE;
                case HASH_POINTER: return karabo::util::Types::HASH_POINTER;
                case VECTOR_HASH_POINTER: return karabo::util::Types::VECTOR_HASH_POINTER;
                default:
                    throw KARABO_PYTHON_EXCEPTION("Unsupported type encountered while converting from PyTypes to Types.");
            }
        }

        static const ReferenceType category(int type) {
            return from(karabo::util::Types::category(type));
        }
    };

    template <typename T>
    class CppArrayRefHandler {
    public:
        typedef boost::shared_ptr<karabo::util::ArrayData<T> > ArrayDataPtr;

    private:
        ArrayDataPtr m_arrayData;

    public:
        explicit CppArrayRefHandler(const ArrayDataPtr& arr) : m_arrayData(arr) {}
        ArrayDataPtr getDataPtr() const { return m_arrayData; }
    };

    template <typename T>
    class PyArrayRefHandler {
        PyArrayObject* m_arrayRef;

    public:
        explicit PyArrayRefHandler(PyArrayObject* obj) : m_arrayRef(obj) {}

        void operator ()(const T*) {
            Py_DECREF(m_arrayRef);
        }
    };

    struct Wrapper {

        static bool hasattr(bp::object obj, const std::string& attrName) {
            // NOTE: There seems to be different implementations of the Python C-API around
            // Some use a char* some other a const char* -> char* is the always compiling alternative
            return PyObject_HasAttrString(obj.ptr(), const_cast<char*> (attrName.c_str()));
        }

        template <class T, class U>
        static bp::tuple fromStdPairToPyTuple(const std::pair<T, U>& p) {
            return bp::make_tuple(p.first, p.second);
        }

        template<class ValueType>
        static bp::object fromStdVectorToPyList(const std::vector<ValueType>& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::object(v[i]));
            return pylist;
        }

        template <class T, class U>
        static bp::object fromStdVectorToPyList(const std::vector< std::pair<T, U> >& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::make_tuple(v[i].first, v[i].second));
            return pylist;
        }

        static bp::object fromStdVectorToPyHashList(const std::vector<karabo::util::Hash>& v) {
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
            const char* data = reinterpret_cast<const char*> (&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyBytes_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyBytes(const std::vector<unsigned char>& v) {
            const char* data = reinterpret_cast<const char*> (&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyBytes_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<char>& v) {
            const char* data = &v[0];
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<signed char>& v) {
            const char* data = reinterpret_cast<const char*> (&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::object fromStdVectorToPyByteArray(const std::vector<unsigned char>& v) {
            const char* data = reinterpret_cast<const char*> (&v[0]);
            Py_ssize_t size = v.size();
            return bp::object(bp::handle<>(PyByteArray_FromStringAndSize(data, size)));
        }

        static bp::str fromStdVectorToPyStr(const std::vector<char>& v) {
            return bp::str(&v[0], v.size());
        }

        static bp::str fromStdVectorToPyStr(const std::vector<signed char>& v) {
            return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
        }

        static bp::str fromStdVectorToPyStr(const std::vector<unsigned char>& v) {
            return bp::str(reinterpret_cast<const char*> (&v[0]), v.size());
        }

        template<class T>
        static std::vector<T> fromPyListToStdVector(const bp::object & obj) {
            const bp::list& lst = bp::extract<bp::list > (obj);
            bp::ssize_t size = bp::len(lst);

            std::vector<T> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<T > (lst[i]);
            }
            return v;
        }

        template<class T>
        static std::vector<T> fromPyTupleToStdVector(const bp::object & obj) {
            const bp::tuple& tpl = bp::extract<bp::tuple > (obj);
            bp::ssize_t size = bp::len(tpl);

            std::vector<T> v(size);
            for (bp::ssize_t i = 0; i < size; ++i) {
                v[i] = bp::extract<T > (tpl[i]);
            }
            return v;
        }

        template<typename T>
        static karabo::util::NDArray<T> fromPyArrayToNDArray(PyArrayObject* arr) {
            // Convert the array shape to a std::vector
            npy_intp* pDims = PyArray_DIMS(arr);
            std::vector<unsigned long long> dims;
            for (int i = 0; i < PyArray_NDIM(arr); i++) {
                dims.push_back(pDims[i]);
            }

            // Get an ArrayData object which points to the array's data
            boost::shared_ptr<karabo::util::ArrayData<T> > array;
            bp::object base(bp::handle<>(bp::borrowed(PyArray_BASE(arr))));

            // Determine if the array data is owned by a C++ object
            if (base != bp::object()) {
                if (bp::extract<CppArrayRefHandler<T> >(base).check()) {
                    const CppArrayRefHandler<T>& arrayRef = bp::extract<CppArrayRefHandler<T>& >(base);
                    array = arrayRef.getDataPtr();
                }
            }

            // Array ref is still empty. Create a new ArrayData
            if (!array) {
                // Get a contiguous copy of the array (or just a reference if already contiguous)
                PyArrayObject* carr = PyArray_GETCONTIGUOUS(arr);
                const T* data = reinterpret_cast<T*> (PyArray_DATA(carr));
                const npy_intp nelems = PyArray_SIZE(carr);
                const PyArrayRefHandler<T> refHandler(carr); // Steals the reference to carr
                array = boost::shared_ptr<karabo::util::ArrayData<T> >(new karabo::util::ArrayData<T>(data, nelems, refHandler));
            }

            return karabo::util::NDArray<T>(array, karabo::util::Dims(dims));
        }

        static bp::object toObject(const boost::any& operand, bool numpyFlag = false);
        static karabo::util::Types::ReferenceType toAny(const bp::object& operand, boost::any& any);

        static bp::object fromStdVectorToPyListNone(const std::vector<karabo::util::CppNone>& v) {
            bp::list pylist;
            for (size_t i = 0; i < v.size(); i++) pylist.append(bp::object());
            return pylist;
        }

        template<class ValueType>
        static bp::object fromStdVectorToPyArray(const std::vector<ValueType>& v, bool numpyFlag = false) {
            return fromStdVectorToPyList(v);
        }

        template<typename T>
        static bp::object fromNDArrayToPyArray(const karabo::util::NDArray<T>& a, const int typenum) {
            const karabo::util::Dims shape = a.getShape();
            const int nd = shape.rank();
            std::vector<npy_intp> dims(nd, 0);
            for (int i = 0; i < nd; ++i) {
                dims[i] = shape.extentIn(i);
            }

            boost::shared_ptr<karabo::util::ArrayData<T> > arrayData(a.getData());
            const boost::shared_ptr<CppArrayRefHandler<T> > refHandler(new CppArrayRefHandler<T>(arrayData));
            bp::object pyRefHandler(refHandler); // Python reference count starts a 1
            void* data = reinterpret_cast<void*> (arrayData->data());
            PyObject* pyobj = PyArray_SimpleNewFromData(nd, &dims[0], typenum, data);
            PyArray_SetBaseObject(reinterpret_cast<PyArrayObject*>(pyobj), pyRefHandler.ptr());
            // PyArray_SetBaseObject steals a reference. Increase the refcount to protect bp::object::~object()
            Py_INCREF(pyRefHandler.ptr());
            return bp::object(bp::handle<>(pyobj));
        }
    };

    // Specializations
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<bool>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<short>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned short>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<int>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned int>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<long long>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<unsigned long long>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<float>& v, bool numpyFlag);
    template<> bp::object Wrapper::fromStdVectorToPyArray(const std::vector<double>& v, bool numpyFlag);
}

#endif	/* WRAPPER_HH */

