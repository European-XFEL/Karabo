/*
 * File:   Wrapper.hh
 * Author: CONTROLS DEV group
 *
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

#ifndef KARABIND_WRAPPER_HH
#define KARABIND_WRAPPER_HH

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <any>

#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/NDArray.hh"
#include "karabo/data/types/ToLiteral.hh"
#include "karabo/data/types/Types.hh"

namespace py = pybind11;


PYBIND11_MAKE_OPAQUE(std::vector<karabo::data::Hash>);
PYBIND11_MAKE_OPAQUE(std::vector<karabo::data::Hash::Pointer>);


namespace karabind {

    // The helper class that is a holder for C++ data array shared pointer and serves as
    // a base (PyArray_BASE) in Numpy C-API

    class ArrayDataPtrBase {
       private:
        // Array data shared pointer that uses specific deleter if memory allocated in Python
        // or default deleter if allocated in C++
        karabo::data::NDArray::DataPointer m_dataPtr;

       public:
        explicit ArrayDataPtrBase(const karabo::data::NDArray::DataPointer& dataPtr) : m_dataPtr(dataPtr) {}

        karabo::data::NDArray::DataPointer getDataPtr() const {
            return m_dataPtr;
        }
    };

    // The helper class that serves as a specific Deleter of data array shared pointer for managing
    // the memory allocated in Python code.

    class PyArrayDeleter {
        PyObject* m_arrayRef;

       public:
        // Store Python object
        explicit PyArrayDeleter(PyObject* obj) : m_arrayRef(obj) {}

        // C++ Deleter should ignore input argument and decrement refcount for stored Python array
        void operator()(const char*) {
            py::gil_scoped_acquire gil;
            Py_DECREF(m_arrayRef);
        }

        // helper method for debugging Python reference counter
        Py_ssize_t refcount() {
            return Py_REFCNT(m_arrayRef);
        }
    };


    namespace hashwrap {

        /**
         * Get either a reference to the Hash subtree for Hash and VectorHash or get a value for
         * leaf element.
         *
         * @param self Hash
         * @param path path string
         * @param sep separator string
         * @return Hash/VectorHash reference to subtree in parent
         */
        py::object getRef(karabo::data::Hash& self, const std::string& path, const std::string& sep)
              __attribute__((visibility("default")));

        py::object getAs(const karabo::data::Hash& self, const std::string& path,
                         const karabo::data::Types::ReferenceType& target, const std::string& separator)
              __attribute__((visibility("default")));

        py::object get(const karabo::data::Hash& self, const std::string& path, const std::string& separator = ".",
                       const py::object& default_return = py::none()) __attribute__((visibility("default")));

        const karabo::data::Hash& setPyDictAsHash(karabo::data::Hash& self, const py::dict& dictionary, const char sep)
              __attribute__((visibility("default")));

        void set(karabo::data::Hash& self, const std::string& key, const py::object& o,
                 const std::string& separator = ".") __attribute__((visibility("default")));


    } // namespace hashwrap


    namespace detail {

        /// Helper when catching Python exceptions
        // Force to make this function visible outside the libkarabind.so, since we compile
        // karabind code with -fvisibility=hidden option, so LTO can do optimization ...
        void treatError_already_set(py::error_already_set& e, const py::object& handler, const char* where)
              __attribute__((visibility("default")));

        inline void packPy_r(karabo::data::Hash& hash, char i) {}

        template <class Tfirst, class... Trest>
        inline void packPy_r(karabo::data::Hash& hash, char i, const Tfirst& first, const Trest&... rest) {
            char name[4] = "a ";
            name[1] = i;
            // Besides the following line, 'packPy_r' is identical to the C++ version 'karabo::util::pack_r'.
            hashwrap::set(hash, name, first);
            detail::packPy_r(hash, i + 1, rest...);
        }

    } // namespace detail

    /**
     * Pack the parameters into a hash for transport over the network.
     * @param hash Will be filled with keys a1, a2, etc. and associated values
     * @param args Any type and number of arguments to associated to hash keys
     */
    template <class... Ts>
    inline void packPy(karabo::data::Hash& hash, const Ts&... args) {
        detail::packPy_r(hash, '1', args...);
    }


    namespace wrapper {

        template <class IntegerType>
        static IntegerType toInteger(const py::object& obj) {
            if (PyLong_Check(obj.ptr())) {
                const PY_LONG_LONG value = PyLong_AsLongLong(obj.ptr());
                return static_cast<IntegerType>(value);
            } else {
                throw KARABO_CAST_EXCEPTION("Cannot cast Python object to '" +
                                                  std::string(typeid(IntegerType).name()) += "'");
                return static_cast<IntegerType>(0); // please the compiler
            }
        }

        bool fromPyObjectToString(const py::object& o, std::string& s) __attribute__((visibility("default")));

        std::vector<std::string> fromPySequenceToVectorString(const py::object& o)
              __attribute__((visibility("default")));

        karabo::data::Types::ReferenceType pyObjectToCppType(const py::object& otype)
              __attribute__((visibility("default")));

        py::object castAnyToPy(const std::any& operand) __attribute__((visibility("default")));

        karabo::data::Types::ReferenceType castPyToAny(const py::object& operand, std::any& a)
              __attribute__((visibility("default")));

        karabo::data::ByteArray copyPyToByteArray(const py::object& o) __attribute__((visibility("default")));

        /**
         * Create py::array from C++ NDArray without data copying
         * No change in data ownership
         */
        py::object castNDArrayToPy(const karabo::data::NDArray& nda) __attribute__((visibility("default")));

        /**
         * Create NDArray from python numpy array without data copying
         * No change in data ownership
         */
        karabo::data::NDArray castPyArrayToND(py::array arr) __attribute__((visibility("default")));

        /**
         * Create py::array from C++ NDArray with data copying
         * As a result, python is data owner.  This function is not needed
         * since we can use 'castNDArrayToPy' and apply `copy` method in python
         */
        py::object copyNDArrayToPy(const karabo::data::NDArray& nda) __attribute__((visibility("default")));

        /**
         * Create NDArray from python numpy array with data copying
         * As a result, C++ is data owner
         */
        karabo::data::NDArray copyPyArrayToND(py::array arr) __attribute__((visibility("default")));

        namespace detail {

            py::object castElementToPy(const karabo::data::Hash::Attributes::Node& self,
                                       const karabo::data::Types::ReferenceType& type);

            py::object castElementToPy(const karabo::data::Hash::Node& self,
                                       const karabo::data::Types::ReferenceType& type);
        } // namespace detail

        void setAttributeAsPy(karabo::data::Hash& self, const std::string& path, const std::string& attr,
                              const py::object& o);

        template <typename T>
        struct AliasAttributePy {
            static T& setAlias(T& self, const py::object& obj) {
                if (py::isinstance<py::int_>(obj)) {
                    return self.alias(obj.cast<long long>());
                }
                if (py::isinstance<py::str>(obj)) {
                    return self.alias(obj.cast<std::string>());
                }
                if (py::isinstance<py::float_>(obj)) {
                    return self.alias(obj.cast<double>());
                }
                if (py::isinstance<py::list>(obj)) {
                    py::ssize_t size = py::len(obj);
                    if (size == 0) {
                        return self.alias(std::vector<std::string>());
                    }
                    const auto& vo = obj.cast<std::vector<py::object>>();
                    py::object list0 = vo[0];
                    if (list0.is_none()) {
                        std::vector<karabo::data::CppNone> v;
                        for (py::ssize_t i = 0; i < size; ++i) v.push_back(karabo::data::CppNone());
                        return self.alias(v);
                    }
                    if (py::isinstance<py::bool_>(list0)) {
                        std::vector<bool> v(size); // Special case here
                        for (py::ssize_t i = 0; i < size; ++i) v[i] = vo[i].cast<bool>();
                        return self.alias(v);
                    }
                    if (py::isinstance<py::int_>(list0)) {
                        std::vector<long long> v(size);
                        for (py::ssize_t i = 0; i < size; ++i) {
                            v[i] = vo[i].cast<long long>();
                        }
                        return self.alias(v);
                    }
                    if (py::isinstance<py::float_>(list0)) {
                        std::vector<double> v(size);
                        for (py::ssize_t i = 0; i < size; ++i) {
                            v[i] = vo[i].cast<double>();
                        }
                        return self.alias(v);
                    }
                    if (py::isinstance<py::str>(list0)) {
                        std::vector<std::string> v(size);
                        for (py::ssize_t i = 0; i < size; ++i) {
                            v[i] = vo[i].cast<std::string>();
                        }
                        return self.alias(v);
                    }
                }
                throw KARABO_PYTHON_EXCEPTION("Unknown data type of the 'alias' element");
            }
        };

        template <typename T>
        std::vector<T> castPySequenceToStdVector(const py::sequence& sequence) {
            std::vector<T> vt;
            vt.reserve(sequence.size());
            for (const auto& item : sequence) vt.push_back(item.cast<T>());
            return vt;
        }

        /**
         * How many non-keyword arguments does 'callable' expect?
         *
         * Works for free functions, member methods and objects with simple __call__ method (even with @staticmethod),
         * but not for functools.partial objects.
         *
         */
        size_t numArgs(const py::object& o);

        karabo::data::Hash deepCopy_r(const karabo::data::Hash& h);

        py::object deepCopyHashLike(const py::object& o);

    } /* namespace wrapper */

    /**
     * Provide exception text and details (i.e. traceback)
     * To be called from within a "catch (py::error_already_set& e)" block.
     *
     * @return tuple of two strings:
     *         - first is (Python) exception text,
     *         - second the multiline traceback like from traceback.print_exception (skipping last line)
     */
    std::tuple<std::string, std::string> getPythonExceptionStrings(py::error_already_set& e);

} // namespace karabind

#endif /* KARABIND_WRAPPER_HH */
