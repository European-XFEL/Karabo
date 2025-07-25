/*
 * File:   Wrapper.cc
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

#include "Wrapper.hh"

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <filesystem>
#include <karabo/xms/ImageData.hh>

#include "FromNumpy.hh"
#include "PyTypes.hh"
#include "ToNumpy.hh"
#include "karabo/data/types/FromLiteral.hh"
#include "karabo/data/types/Hash.hh"
#include "karabo/data/types/Schema.hh"

using namespace std;


namespace karabind {
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
        py::object getRef(karabo::data::Hash& self, const std::string& path, const std::string& sep) {
            using namespace karabo::data;
            Hash::Node& node = self.getNode(path, sep.at(0));
            if (node.getType() == Types::HASH) {
                std::shared_ptr<Hash> hp = std::shared_ptr<Hash>(&node.getValue<Hash>(), [](const Hash*) {});
                if (node.hasAttribute(KARABO_HASH_CLASS_ID)) {
                    const std::string& classId = node.getAttribute<std::string>(KARABO_HASH_CLASS_ID);
                    if (classId == "NDArray") {
                        return py::cast(self).attr("_get_ndarray_")(*hp);
                    }
                    if (classId == "ImageData") {
                        using namespace karabo::xms;
                        const ImageData& imgData = reinterpret_cast<const ImageData&>(*hp);
                        return py::cast(
                              std::shared_ptr<ImageData>(new ImageData(imgData), std::default_delete<ImageData>()));
                    }
                }
                return py::cast(self).attr("_getref_hash_")(hp);
            }
            if (node.getType() == Types::VECTOR_HASH) {
                std::vector<Hash>* vhp = &node.getValue<std::vector<Hash>>();
                return py::cast(self).attr("_getref_vector_hash_")(vhp);
            }
            return wrapper::castAnyToPy(node.getValueAsAny());
        }

        py::object getRefAttributesByNode(karabo::data::Hash::Node& self) {
            using namespace karabo::data;
            auto hp = std::shared_ptr<Hash::Attributes>(&self.getAttributes(), [](const Hash::Attributes*) {});
            return py::cast(self).attr("_getref_attrs_by_node_")(hp);
        }

        py::object getRefAttributes(karabo::data::Hash& self, const std::string& path, const std::string& sep) {
            using namespace karabo::data;
            Hash::Node& node = self.getNode(path, sep.at(0));
            auto hp = std::shared_ptr<Hash::Attributes>(&node.getAttributes(), [](const Hash::Attributes*) {});
            return py::cast(self).attr("_getref_attrs_")(hp);
        }

        py::object getAs(const karabo::data::Hash& self, const std::string& path,
                         const karabo::data::Types::ReferenceType& target, const std::string& separator) {
            const karabo::data::Hash::Node& node = self.getNode(path, separator.at(0));
            return wrapper::detail::castElementToPy(node, target);
        }

        py::object get(const karabo::data::Hash& self, const std::string& path, const std::string& separator,
                       const py::object& default_return) {
            // This implements the standard Python dictionary behavior for get()
            if (!self.has(path, separator.at(0))) {
                return default_return;
            }
            return getRef(const_cast<karabo::data::Hash&>(self), path, separator);
        }

        const karabo::data::Hash& setPyDictAsHash(karabo::data::Hash& self, const py::dict& dictionary,
                                                  const char sep) {
            std::string separator(1, sep);
            for (auto item : dictionary) {
                const py::object& obj = py::reinterpret_borrow<py::object>(item.second);
                if (py::isinstance<py::dict>(obj)) {
                    const auto& dictobj = obj.cast<py::dict>();
                    karabo::data::Hash h;
                    self.set(item.first.cast<std::string>(), setPyDictAsHash(h, dictobj, sep), sep);
                } else {
                    set(self, item.first.cast<std::string>(), obj, separator);
                }
            }
            return self;
        }

        void set(karabo::data::Hash& self, const std::string& key, const py::object& o, const std::string& separator) {
            using namespace karabo::data;
            if (py::isinstance<Hash>(o)) {
                const Hash& h = o.cast<Hash>();
                self.set(key, h, separator.at(0));
                return;
            }
            // Check if this is ndarray
            if (py::isinstance<py::array>(o)) {
                const NDArray arr = karabind::wrapper::castPyArrayToND(o);
                self.set(key, arr, separator.at(0));
                return;
            }
            // Check if it is ImageData
            if (py::isinstance<karabo::xms::ImageData>(o)) {
                const auto& img = o.cast<karabo::xms::ImageData>();
                self.set(key, img, separator.at(0));
                return;
            }
            // Check if it is a python dictionary
            if (py::isinstance<py::dict>(o)) {
                const py::dict& d = o.cast<py::dict>();
                Hash h;
                self.set(key, karabind::hashwrap::setPyDictAsHash(h, d, separator.at(0)), separator.at(0));
                return;
            }
            std::any any;
            karabind::wrapper::castPyToAny(o, any);
            self.set(key, std::move(any), separator.at(0));
        }
    } // namespace hashwrap


    namespace wrapper {
        namespace detail {

            // Define this macro to guarantee the same conversion rules for 2 types:
            // 'karabo::data::Hash::Node' and 'karabo::data::Hash::Attributes::Node'
            // (both are based on `karabo::data::Element` class with different template args)
#define CAST_ELEMENT_TO_PY(T)                                                                   \
    py::object castElementToPy(const T& self, const karabo::data::Types::ReferenceType& type) { \
        using namespace karabo::data;                                                           \
        switch (type) {                                                                         \
            case Types::BOOL:                                                                   \
                return py::cast(self.getValueAs<bool>());                                       \
            case Types::CHAR:                                                                   \
                return py::cast(self.getValueAs<char>());                                       \
            case Types::INT8:                                                                   \
                return py::cast(self.getValueAs<signed char>());                                \
            case Types::UINT8:                                                                  \
                return py::cast(self.getValueAs<unsigned char>());                              \
            case Types::INT16:                                                                  \
                return py::cast(self.getValueAs<short>());                                      \
            case Types::UINT16:                                                                 \
                return py::cast(self.getValueAs<unsigned short>());                             \
            case Types::INT32:                                                                  \
                return py::cast(self.getValueAs<int>());                                        \
            case Types::UINT32:                                                                 \
                return py::cast(self.getValueAs<unsigned int>());                               \
            case Types::INT64:                                                                  \
                return py::cast(self.getValueAs<long long>());                                  \
            case Types::UINT64:                                                                 \
                return py::cast(self.getValueAs<unsigned long long>());                         \
            case Types::FLOAT:                                                                  \
                return py::cast(self.getValueAs<float>());                                      \
            case Types::DOUBLE:                                                                 \
                return py::cast(self.getValueAs<double>());                                     \
            case Types::COMPLEX_FLOAT:                                                          \
                return py::cast(self.getValueAs<std::complex<float>>());                        \
            case Types::COMPLEX_DOUBLE:                                                         \
                return py::cast(self.getValueAs<std::complex<double>>());                       \
            case Types::STRING:                                                                 \
                return py::cast(self.getValueAs<std::string>());                                \
            case Types::VECTOR_BOOL:                                                            \
                return py::cast(self.getValueAs<bool, std::vector>());                          \
            case Types::VECTOR_CHAR: {                                                          \
                const auto& v = self.getValueAs<char, std::vector>();                           \
                return py::bytearray(v.data(), v.size());                                       \
            }                                                                                   \
            case Types::VECTOR_INT8:                                                            \
                return py::cast(self.getValueAs<signed char, std::vector>());                   \
            case Types::VECTOR_UINT8:                                                           \
                return py::cast(self.getValueAs<unsigned char, std::vector>());                 \
            case Types::VECTOR_INT16:                                                           \
                return py::cast(self.getValueAs<short, std::vector>());                         \
            case Types::VECTOR_UINT16:                                                          \
                return py::cast(self.getValueAs<unsigned short, std::vector>());                \
            case Types::VECTOR_INT32:                                                           \
                return py::cast(self.getValueAs<int, std::vector>());                           \
            case Types::VECTOR_UINT32:                                                          \
                return py::cast(self.getValueAs<unsigned int, std::vector>());                  \
            case Types::VECTOR_INT64:                                                           \
                return py::cast(self.getValueAs<long long, std::vector>());                     \
            case Types::VECTOR_UINT64:                                                          \
                return py::cast(self.getValueAs<unsigned long long, std::vector>());            \
            case Types::VECTOR_FLOAT:                                                           \
                return py::cast(self.getValueAs<float, std::vector>());                         \
            case Types::VECTOR_DOUBLE:                                                          \
                return py::cast(self.getValueAs<double, std::vector>());                        \
            case Types::VECTOR_COMPLEX_FLOAT:                                                   \
                return py::cast(self.getValueAs<std::complex<float>, std::vector>());           \
            case Types::VECTOR_COMPLEX_DOUBLE:                                                  \
                return py::cast(self.getValueAs<std::complex<double>, std::vector>());          \
            case Types::VECTOR_STRING:                                                          \
                return py::cast(self.getValueAs<std::string, std::vector>());                   \
            case Types::BYTE_ARRAY: {                                                           \
                const auto& v = self.getValueAs<ByteArray>();                                   \
                return py::bytearray(reinterpret_cast<const char*>(v.first.get()), v.second);   \
            }                                                                                   \
            default:                                                                            \
                break;                                                                          \
        }                                                                                       \
        std::ostringstream oss;                                                                 \
        oss << "Type " << Types::to<karabo::data::ToLiteral>(type) << " is not yet supported";  \
        throw KARABO_NOT_SUPPORTED_EXCEPTION(oss.str());                                        \
    }

            // NOTE: Use of metaprogramming here directly does not work: compiler failsto deduce substitutions
            // correctly. It can be implemented using some indirection but it is more verbose and we stick with above
            // macro. We need the following Node types ...
            CAST_ELEMENT_TO_PY(karabo::data::Hash::Attributes::Node)
            CAST_ELEMENT_TO_PY(karabo::data::Hash::Node)

        } // namespace detail


        bool fromPyObjectToString(const py::object& o, std::string& s) {
            if (py::isinstance<py::bytes>(o)) {
                s.assign(std::string(o.cast<py::bytes>()));
            } else if (py::isinstance<py::bytearray>(o)) {
                s.assign(std::string(o.cast<py::bytearray>()));
            } else if (py::isinstance<py::str>(o)) {
                s.assign(std::string(o.cast<py::str>()));
            } else {
                PyErr_SetString(PyExc_TypeError, "Python type in parameters is not supported");
                return false;
            }
            return true;
        }


        std::vector<std::string> fromPySequenceToVectorString(const py::object& o) {
            std::vector<std::string> vs;
            if (py::isinstance<py::str>(o)) {
                // Align with karathon: py::str -> list of characters
                const auto& s = o.cast<std::string>();
                vs.reserve(s.size());
                for (size_t i = 0; i < s.size(); ++i) vs.push_back(std::string(1, s.at(i)));
            } else if (py::isinstance<py::dict>(o)) {
                py::dict dict = o.cast<py::dict>();
                for (auto item : dict) {
                    if (!py::isinstance<py::str>(item.first)) {
                        throw KARABO_CAST_EXCEPTION("Dict key is not of 'str' type");
                    }
                    vs.push_back(item.first.cast<std::string>());
                }
            } else if (py::isinstance<py::sequence>(o)) {
                for (const auto& item : o) {
                    if (!py::isinstance<py::str>(item)) {
                        throw KARABO_CAST_EXCEPTION("Sequence element is not of 'str' type");
                    }
                    vs.push_back(item.cast<std::string>());
                }
            } else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("This object is not tuple, list or dict");
            }
            return vs;
        }


        karabo::data::Types::ReferenceType pyObjectToCppType(const py::object& otype) {
            using namespace karabo::data;
            Types::ReferenceType targetType = Types::UNKNOWN;
            if (py::isinstance<py::str>(otype)) {
                const std::string& stype = otype.cast<std::string>();
                targetType = Types::from<FromLiteral>(stype);
            } else if (py::isinstance<PyTypes::ReferenceType>(otype)) {
                PyTypes::ReferenceType ptype = otype.cast<PyTypes::ReferenceType>();
                targetType = PyTypes::to(ptype);
            } else {
                throw KARABO_PARAMETER_EXCEPTION("Argument type is not supported. Valid types: 'str' and 'Types'");
            }
            return targetType;
        }


        py::object castAnyToPy(const std::any& operand) {
            try {
                if (operand.type() == typeid(bool)) {
                    return py::cast(std::any_cast<bool>(operand));
                } else if (operand.type() == typeid(char)) {
                    return py::cast(std::any_cast<char>(operand));
                } else if (operand.type() == typeid(signed char)) {
                    return py::cast(std::any_cast<signed char>(operand));
                } else if (operand.type() == typeid(unsigned char)) {
                    return py::cast(std::any_cast<unsigned char>(operand));
                } else if (operand.type() == typeid(short)) {
                    return py::cast(std::any_cast<short>(operand));
                } else if (operand.type() == typeid(unsigned short)) {
                    return py::cast(std::any_cast<unsigned short>(operand));
                } else if (operand.type() == typeid(int)) {
                    return py::cast(std::any_cast<int>(operand));
                } else if (operand.type() == typeid(unsigned int)) {
                    return py::cast(std::any_cast<unsigned int>(operand));
                } else if (operand.type() == typeid(long long)) {
                    return py::cast(std::any_cast<long long>(operand));
                } else if (operand.type() == typeid(unsigned long long)) {
                    return py::cast(std::any_cast<unsigned long long>(operand));
                } else if (operand.type() == typeid(float)) {
                    return py::cast(std::any_cast<float>(operand));
                } else if (operand.type() == typeid(double)) {
                    return py::cast(std::any_cast<double>(operand));
                } else if (operand.type() == typeid(std::complex<float>)) {
                    return py::cast(std::any_cast<std::complex<float>>(operand));
                } else if (operand.type() == typeid(std::complex<double>)) {
                    return py::cast(std::any_cast<std::complex<double>>(operand));
                } else if (operand.type() == typeid(std::string)) {
                    return py::cast(std::any_cast<std::string>(operand));
                } else if (operand.type() == typeid(std::filesystem::path)) {
                    return py::cast(std::any_cast<std::filesystem::path>(operand).string());
                } else if (operand.type() == typeid(karabo::data::CppNone)) {
                    return py::none();
                } else if (operand.type() == typeid(karabo::data::NDArray)) {
                    return castNDArrayToPy(std::any_cast<karabo::data::NDArray>(operand));
                } else if (operand.type() == typeid(karabo::data::Hash)) {
                    return py::cast(std::any_cast<karabo::data::Hash>(operand));
                } else if (operand.type() == typeid(karabo::data::Hash::Pointer)) {
                    return py::cast(std::any_cast<karabo::data::Hash::Pointer>(operand));
                } else if (operand.type() == typeid(std::vector<bool>)) {
                    return py::cast(std::any_cast<std::vector<bool>>(operand));
                } else if (operand.type() == typeid(std::vector<char>)) {
                    const std::vector<char>& v = std::any_cast<std::vector<char>>(operand);
                    return py::bytes(v.data(), v.size());
                } else if (operand.type() == typeid(std::vector<signed char>)) {
                    return py::cast(std::any_cast<std::vector<signed char>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned char>)) {
                    return py::cast(std::any_cast<std::vector<unsigned char>>(operand));
                } else if (operand.type() == typeid(std::vector<short>)) {
                    return py::cast(std::any_cast<std::vector<short>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned short>)) {
                    return py::cast(std::any_cast<std::vector<unsigned short>>(operand));
                } else if (operand.type() == typeid(std::vector<int>)) {
                    return py::cast(std::any_cast<std::vector<int>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned int>)) {
                    return py::cast(std::any_cast<std::vector<unsigned int>>(operand));
                } else if (operand.type() == typeid(std::vector<long long>)) {
                    return py::cast(std::any_cast<std::vector<long long>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned long long>)) {
                    return py::cast(std::any_cast<std::vector<unsigned long long>>(operand));
                } else if (operand.type() == typeid(std::vector<float>)) {
                    return py::cast(std::any_cast<std::vector<float>>(operand));
                } else if (operand.type() == typeid(std::vector<double>)) {
                    return py::cast(std::any_cast<std::vector<double>>(operand));
                } else if (operand.type() == typeid(std::vector<std::complex<float>>)) {
                    return py::cast(std::any_cast<std::vector<std::complex<float>>>(operand));
                } else if (operand.type() == typeid(std::vector<std::complex<double>>)) {
                    return py::cast(std::any_cast<std::vector<std::complex<double>>>(operand));
                } else if (operand.type() == typeid(std::vector<std::string>)) {
                    return py::cast(std::any_cast<std::vector<std::string>>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::data::CppNone>)) {
                    const auto& v = std::any_cast<std::vector<karabo::data::CppNone>>(operand);
                    std::vector<py::object> vo(v.size(), py::none());
                    return py::cast(vo);
                } else if (operand.type() == typeid(karabo::data::Schema)) {
                    return py::cast(std::any_cast<karabo::data::Schema>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::data::Hash>)) {
                    return py::cast(std::any_cast<std::vector<karabo::data::Hash>>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::data::Hash::Pointer>)) {
                    return py::cast(std::any_cast<std::vector<karabo::data::Hash::Pointer>>(operand));
                } else if (operand.type() == typeid(karabo::data::ByteArray)) {
                    const auto& ba = std::any_cast<karabo::data::ByteArray>(operand);
                    return py::bytes(ba.first.get(), ba.second);
                }
                std::ostringstream oss;
                oss << "Failed to convert inner Hash type: " << operand.type().name() << " to python";
                throw KARABO_PYTHON_EXCEPTION(oss.str());
            } catch (const std::bad_any_cast& e) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
            }
            return py::none(); // make compiler happy -- we never reach this statement
        }


        // Helper for Wrapper::toAny below:
        static karabo::data::Types::ReferenceType bestIntegerType(const py::object& obj) {
            long long const kNegInt32Min = -(1LL << 31);
            long long const kPosUint32Max = (1LL << 32) - 1;
            long long const kPosInt32Max = (1LL << 31) - 1;
            long long const kPosInt64Max = (1ULL << 63) - 1;

            int overflow = 0;
            PY_LONG_LONG value = PyLong_AsLongLongAndOverflow(obj.ptr(), &overflow);
            if (overflow == 0) {
                if (value < 0) {
                    if (value < kNegInt32Min) {
                        return karabo::data::Types::INT64;
                    } else {
                        return karabo::data::Types::INT32;
                    }
                } else {
                    if (value > kPosUint32Max) {
                        if (value <= kPosInt64Max) {
                            return karabo::data::Types::INT64;
                        } else {
                            return karabo::data::Types::UINT64;
                        }
                    } else {
                        if (value <= kPosInt32Max) {
                            return karabo::data::Types::INT32;
                        } else {
                            return karabo::data::Types::UINT32;
                        }
                    }
                }
            } else {
                // So 'long long', i.e. INT64, overflows. Best try is UINT64 that needs PyLong_AsUnsignedLongLong
                // for conversion. Note that that will raise a Python exception if even that overflows.
                return karabo::data::Types::UINT64;
            }
        }

        bool isEnum(const py::handle obj) {
            static const py::object enum_type = py::module_::import("enum").attr("Enum");
            return py::isinstance(obj, enum_type);
        }

        karabo::data::Types::ReferenceType castPyToAny(const py::object& o, std::any& any) {
            if (o.is_none()) {
                any = karabo::data::CppNone();
                return karabo::data::Types::NONE;
            }
            if (py::isinstance<py::bool_>(o)) {
                any = py::cast<bool>(o);
                return karabo::data::Types::BOOL;
            }
            if (py::isinstance<py::int_>(o)) {
                const karabo::data::Types::ReferenceType type = bestIntegerType(o);
                if (type == karabo::data::Types::UINT64) {
                    // Raises a Python exception if it overflows:
                    any = o.cast<unsigned long long>();
                } else {
                    const auto value = o.cast<long long>();
                    switch (type) {
                        case karabo::data::Types::INT32:
                            any = static_cast<int>(value);
                            break;
                        case karabo::data::Types::UINT32:
                            any = static_cast<unsigned int>(value);
                            break;
                        case karabo::data::Types::INT64:
                            any = static_cast<long long>(value);
                            break;
                        default:
                            // Should never come here!
                            throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + karabo::data::toString(type));
                    }
                }
                return type;
            }
            if (py::isinstance<py::float_>(o)) {
                any = o.cast<double>();
                return karabo::data::Types::DOUBLE;
            }
            if (PyComplex_Check(o.ptr())) {
                any = o.cast<std::complex<double>>();
                return karabo::data::Types::COMPLEX_DOUBLE;
            }
            if (py::isinstance<py::str>(o)) {
                any = o.cast<std::string>();
                return karabo::data::Types::STRING;
            }
            if (py::isinstance<py::bytes>(o)) {
                const auto& s = o.cast<std::string>();
                any = std::vector<char>(s.begin(), s.end());
                return karabo::data::Types::VECTOR_CHAR;
            }
            if (py::isinstance<py::bytearray>(o)) {
                const auto& s = o.cast<std::string>();
                any = std::vector<char>(s.begin(), s.end());
                return karabo::data::Types::VECTOR_CHAR;
            }
            if (py::isinstance<py::array>(o)) {
                using namespace karabo::data;
                NDArray nda = wrapper::castPyArrayToND(o);
                any = std::move(nda);
                return karabo::data::Types::HASH;
            }
            if (py::isinstance<karabo::data::Hash>(o)) {
                any = o.cast<karabo::data::Hash>();
                return karabo::data::Types::HASH;
            }
            if (py::isinstance<karabo::data::Hash::Pointer>(o)) {
                any = o.cast<karabo::data::Hash::Pointer>();
                return karabo::data::Types::HASH_POINTER;
            }
            if (py::isinstance<karabo::data::Schema>(o)) {
                any = o.cast<karabo::data::Schema>();
                return karabo::data::Types::SCHEMA;
            }
            if (py::isinstance<std::vector<karabo::data::Hash>>(o)) {
                any = o.cast<std::vector<karabo::data::Hash>>();
                return karabo::data::Types::VECTOR_HASH;
            }
            if (py::isinstance<std::vector<karabo::data::Hash::Pointer>>(o)) {
                any = o.cast<std::vector<karabo::data::Hash::Pointer>>();
                return karabo::data::Types::VECTOR_HASH_POINTER;
            }
            if (py::hasattr(o, "__name__")) { // python function
                any = o;
                return karabo::data::Types::UNKNOWN;
            }

            auto lo = py::list();
            for (auto item : o) lo.append(item);
            size_t size = py::len(lo);
            if (size == 0) {
                any = std::vector<std::string>();
                return karabo::data::Types::VECTOR_STRING;
            }
            py::object list0 = lo[0];
            if (list0.is_none()) {
                any = std::vector<karabo::data::CppNone>(size, karabo::data::CppNone());
                return karabo::data::Types::VECTOR_NONE;
            }
            if (py::isinstance<py::bool_>(list0)) {
                any = lo.cast<std::vector<bool>>();
                return karabo::data::Types::VECTOR_BOOL;
            }
            if (py::isinstance<py::int_>(list0)) {
                // First item is an integer - assume that all items are!
                karabo::data::Types::ReferenceType broadestType = karabo::data::Types::INT32;
                for (size_t i = 0; i < size; ++i) {
                    const karabo::data::Types::ReferenceType type = bestIntegerType(lo[i]);
                    // This relies on the fact that the enums ReferenceType have the order INT32, UINT32, INT64,
                    // UINT64
                    if (type > broadestType) {
                        broadestType = type;
                        // Stop loop if cannot get broader...
                        if (broadestType == karabo::data::Types::UINT64) break;
                    }
                }
                if (broadestType == karabo::data::Types::INT32) {
                    any = lo.cast<std::vector<int>>();
                    return karabo::data::Types::VECTOR_INT32;
                } else if (broadestType == karabo::data::Types::UINT32) {
                    any = lo.cast<std::vector<unsigned int>>();
                    return karabo::data::Types::VECTOR_UINT32;
                } else if (broadestType == karabo::data::Types::INT64) {
                    any = lo.cast<std::vector<long long>>();
                    return karabo::data::Types::VECTOR_INT64;
                } else if (broadestType == karabo::data::Types::UINT64) {
                    any = lo.cast<std::vector<unsigned long long>>();
                    return karabo::data::Types::VECTOR_UINT64;
                } else {
                    // Should never come here!
                    throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + karabo::data::toString(broadestType));
                }
            }
            if (py::isinstance<py::float_>(list0)) {
                any = lo.cast<std::vector<double>>();
                return karabo::data::Types::VECTOR_DOUBLE;
            }
            if (PyComplex_Check(list0.ptr())) {
                any = lo.cast<std::vector<std::complex<double>>>();
                return karabo::data::Types::VECTOR_COMPLEX_DOUBLE;
            }
            if (py::isinstance<py::str>(list0)) {
                any = lo.cast<std::vector<std::string>>();
                return karabo::data::Types::VECTOR_STRING;
            }
            if (py::isinstance<karabo::data::Hash>(list0)) {
                // convert py::list of Hash into VectorHash object since we use `bind_vector`:
                // vho = VectorHash(list_of_Hash) like VectorHash([Hash(...), Hash(...),...])
                // TODO: Check if py::implicitly_convertable can be used...
                auto vho = py::module_::import("karabind").attr("VectorHash")(o);
                any = vho.cast<std::vector<karabo::data::Hash>>();
                return karabo::data::Types::VECTOR_HASH;
            }
            if (py::isinstance<karabo::data::Hash::Pointer>(list0)) {
                any = lo.cast<std::vector<karabo::data::Hash::Pointer>>();
                return karabo::data::Types::VECTOR_HASH_POINTER;
            }
            if (py::isinstance<karabo::data::Schema>(list0)) {
                any = lo.cast<std::vector<karabo::data::Schema>>();
                return karabo::data::Types::VECTOR_SCHEMA;
            }
            // Nothing above ...
            throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
        }


        karabo::data::ByteArray copyPyToByteArray(const py::object& o) {
            if (py::isinstance<py::str>(o) || py::isinstance<py::bytes>(o) || py::isinstance<py::bytearray>(o)) {
                const auto& s = o.cast<std::string>();
                const size_t n = s.size();
                char* cp = new char[n + 1]{};
                std::copy(s.begin(), s.end(), cp);
                return std::make_pair(std::shared_ptr<char>(cp, std::default_delete<char[]>()), n);
            }
            throw KARABO_PYTHON_EXCEPTION("Python type can not be converted to ByteArray");
        }


        void setAttributeAsPy(karabo::data::Hash& self, const std::string& path, const std::string& attr,
                              const py::object& o) {
            if (o.is_none()) {
                self.setAttribute(path, attr, karabo::data::CppNone());
            } else if (py::isinstance<py::bool_>(o)) {
                self.setAttribute(path, attr, py::cast<bool>(o));
            } else if (py::isinstance<py::int_>(o)) {
                const karabo::data::Types::ReferenceType type = bestIntegerType(o);
                if (type == karabo::data::Types::UINT64) {
                    // Raises a Python exception if it overflows:
                    self.setAttribute(path, attr, o.cast<unsigned long long>());
                } else {
                    const auto value = o.cast<long long>();
                    switch (type) {
                        case karabo::data::Types::INT32:
                            self.setAttribute(path, attr, static_cast<int>(value));
                            break;
                        case karabo::data::Types::UINT32:
                            self.setAttribute(path, attr, static_cast<unsigned int>(value));
                            break;
                        case karabo::data::Types::INT64:
                            self.setAttribute(path, attr, static_cast<long long>(value));
                            break;
                        default:
                            // Should never come here!
                            throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + karabo::data::toString(type));
                    }
                }
            } else if (py::isinstance<py::float_>(o)) {
                self.setAttribute(path, attr, o.cast<double>());
            } else if (PyComplex_Check(o.ptr())) {
                self.setAttribute(path, attr, o.cast<std::complex<double>>());
            } else if (py::isinstance<py::str>(o)) {
                self.setAttribute(path, attr, o.cast<std::string>());
            } else if (py::isinstance<py::bytes>(o)) {
                const auto& s = o.cast<std::string>();
                self.setAttribute(path, attr, std::vector<char>(s.begin(), s.end()));
            } else if (py::isinstance<py::bytearray>(o)) {
                const auto& s = o.cast<std::string>();
                self.setAttribute(path, attr, std::vector<char>(s.begin(), s.end()));
            } else if (py::isinstance<karabo::data::Hash>(o)) {
                // self.setAttribute(path, attr, o.cast<karabo::data::Hash>());
                throw KARABO_PYTHON_EXCEPTION("Attribute cannot be Hash");
            } else if (py::isinstance<karabo::data::Hash::Pointer>(o)) {
                // self.setAttribute(path, attr, o.cast<karabo::data::Hash::Pointer>());
                throw KARABO_PYTHON_EXCEPTION("Attribute cannot be Hash::Pointer");
            } else if (py::isinstance<karabo::data::Schema>(o)) {
                // self.setAttribute(path, attr, o.cast<karabo::data::Schema>());
                throw KARABO_PYTHON_EXCEPTION("Attribute cannot be Schema");
            } else if (py::isinstance<std::vector<karabo::data::Hash>>(o)) {
                // self.setAttribute(path, attr, o.cast<std::vector<karabo::data::Hash>>());
                throw KARABO_PYTHON_EXCEPTION("Attribute cannot be vector of Hash");
            } else if (py::isinstance<std::vector<karabo::data::Hash::Pointer>>(o)) {
                // self.setAttribute(path, attr, o.cast<std::vector<karabo::data::Hash::Pointer>>());
                throw KARABO_PYTHON_EXCEPTION("Attribute cannot be vector of Hash:;Pointer");
            } else {
                auto lo = py::list();
                for (auto item : o) lo.append(item);
                size_t size = py::len(lo);
                if (size == 0) {
                    self.setAttribute(path, attr, std::vector<std::string>());
                    return;
                }
                py::object list0 = lo[0];
                if (list0.is_none()) {
                    self.setAttribute(path, attr, std::vector<karabo::data::CppNone>(size, karabo::data::CppNone()));
                } else if (py::isinstance<py::bool_>(list0)) {
                    self.setAttribute(path, attr, lo.cast<std::vector<bool>>());
                } else if (py::isinstance<py::int_>(list0)) {
                    // First item is an integer - assume that all items are!
                    karabo::data::Types::ReferenceType broadestType = karabo::data::Types::INT32;
                    for (size_t i = 0; i < size; ++i) {
                        const karabo::data::Types::ReferenceType type = bestIntegerType(lo[i]);
                        // This relies on the fact that the enums ReferenceType have the order INT32, UINT32, INT64,
                        // UINT64
                        if (type > broadestType) {
                            broadestType = type;
                            // Stop loop if cannot get broader...
                            if (broadestType == karabo::data::Types::UINT64) break;
                        }
                    }
                    if (broadestType == karabo::data::Types::INT32) {
                        self.setAttribute(path, attr, lo.cast<std::vector<int>>());
                    } else if (broadestType == karabo::data::Types::UINT32) {
                        self.setAttribute(path, attr, lo.cast<std::vector<unsigned int>>());
                    } else if (broadestType == karabo::data::Types::INT64) {
                        self.setAttribute(path, attr, lo.cast<std::vector<long long>>());
                    } else if (broadestType == karabo::data::Types::UINT64) {
                        self.setAttribute(path, attr, lo.cast<std::vector<unsigned long long>>());
                    } else {
                        // Should never come here!
                        throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + karabo::data::toString(broadestType));
                    }
                } else if (py::isinstance<py::float_>(list0)) {
                    self.setAttribute(path, attr, lo.cast<std::vector<double>>());
                } else if (PyComplex_Check(list0.ptr())) {
                    self.setAttribute(path, attr, lo.cast<std::vector<std::complex<double>>>());
                } else if (py::isinstance<py::str>(list0)) {
                    self.setAttribute(path, attr, lo.cast<std::vector<std::string>>());
                } else if (py::isinstance<karabo::data::Hash>(list0)) {
                    // auto vho = py::module_::import("karabind").attr("VectorHash")(o);
                    // self.setAttribute(path, attr, vho.cast<std::vector<karabo::data::Hash>>());
                    throw KARABO_PYTHON_EXCEPTION("Attribute cannot be sequence of Hash");
                } else if (py::isinstance<karabo::data::Hash::Pointer>(list0)) {
                    // self.setAttribute(path, attr, lo.cast<std::vector<karabo::data::Hash::Pointer>>());
                    throw KARABO_PYTHON_EXCEPTION("Attribute cannot be sequence of Hash::Pointer");
                } else if (py::isinstance<karabo::data::Schema>(list0)) {
                    // self.setAttribute(path, attr, lo.cast<std::vector<karabo::data::Schema>>());
                    throw KARABO_PYTHON_EXCEPTION("Attribute cannot be sequence of Schema");
                } else {
                    self.setAttribute(path, attr, o);
                }
            }
        }


        void setPyDictAsHashAttributes(karabo::data::Hash::Attributes& self, const py::dict& odict, const char sep) {
            for (auto item : odict) {
                const py::object& obj = py::reinterpret_borrow<py::object>(item.second);
                if (py::isinstance<py::dict>(obj)) {
                    const auto& dictobj = obj.cast<py::dict>();
                    karabo::data::Hash h;
                    hashwrap::setPyDictAsHash(h, dictobj, sep);
                    self.set(item.first.cast<std::string>(), h);
                } else {
                    std::any anyval;
                    wrapper::castPyToAny(obj, anyval);
                    self.set(item.first.cast<std::string>(), anyval);
                }
            }
        }


        void mergeAttributesToHashAttributes(karabo::data::Hash::Attributes& target,
                                             const karabo::data::Hash::Attributes& src) {
            using namespace karabo::data;
            if (target.empty()) {
                target = src;
                return;
            }
            for (Hash::Attributes::const_iterator it = src.begin(); it != src.end(); ++it) {
                target.set(it->getKey(), it->getValueAsAny());
            }
        }

        void castPyToHashAttributes(const py::object& o, karabo::data::Hash::Attributes& attrs,
                                    karabo::data::Hash::MergePolicy policy) {
            using namespace karabo::data;
            if (py::isinstance<py::dict>(o)) {
                wrapper::setPyDictAsHashAttributes(attrs, o, '.');
            } else if (py::isinstance<Hash::Attributes>(o)) {
                if (policy == Hash::REPLACE_ATTRIBUTES) {
                    attrs = o.cast<Hash::Attributes>();
                } else if (policy == Hash::MERGE_ATTRIBUTES) {
                    mergeAttributesToHashAttributes(attrs, o.cast<Hash::Attributes>());
                }
            }
        }


        py::object castNDArrayToPy(const karabo::data::NDArray& ndarray) {
            using namespace karabo::data;
            const Types::ReferenceType krbRefType = ndarray.getType();
            const int typenum = Types::to<ToNumpy>(krbRefType);
            const size_t itemsize = Types::to<ToSize>(krbRefType);
            const Dims dims = ndarray.getShape();
            const int ndims = dims.rank();
            std::vector<ssize_t> shape(ndims, 0);
            for (int i = 0; i < ndims; ++i) shape[i] = dims.extentIn(i);
            const ByteArray& bytearr = ndarray.getByteArray();
            if (dims.size() * itemsize > bytearr.second) {
                throw KARABO_PARAMETER_EXCEPTION("Inconsistent NDArray: " + toString(bytearr.second) +=
                                                 " are too few bytes for shape [" + toString(dims.toVector()) +=
                                                 "] of " + Types::to<ToLiteral>(krbRefType));
            }
            py::object dtype = py::dtype(typenum);
            NDArray::DataPointer dataPtr(bytearr.first);
            auto pBase = std::make_shared<ArrayDataPtrBase>(dataPtr);
            auto base = py::cast(pBase);
            pBase.reset();
            void* ptr = static_cast<void*>(dataPtr.get());
            return py::array(dtype, shape, {}, ptr, base);
        }


        karabo::data::NDArray castPyArrayToND(py::array arr) {
            using namespace karabo::data;
            // dimensions
            size_t ndims = arr.ndim();
            // PyArray shape -> C++ Dims
            const ssize_t* shape = arr.shape();
            std::vector<unsigned long long> dims(ndims);
            for (size_t i = 0; i < ndims; ++i) dims[i] = shape[i];
            // PyArray dtype -> typenum -> C++ reftype
            py::dtype dt = arr.dtype();
            int typenum = dt.num();
            Types::ReferenceType krbRefType = Types::from<FromNumpy>(typenum);
            // number of elements
            size_t nelems = arr.size();
            // compute dataPtr
            NDArray::DataPointer dataPtr;
            // Does input array have a 'base' where reference to array data stored?
            const auto& base = arr.base();
            if (base && !base.is_none()) {
                // The 'arr' array is not the owner of the data (base is not None)
                // Check if it is our special storage and get DataPtr incrementing
                // C++ ref.count (shared_ptr use_count!)
                if (py::isinstance<ArrayDataPtrBase>(base)) {
                    const auto& arrRef = base.cast<ArrayDataPtrBase>();
                    dataPtr = arrRef.getDataPtr(); // here we increase C++ shared_ptr use count
                }
            }
            if (!dataPtr) {
                // Python is an owner of array data (base is None)
                // Create another Python array that will be not an owner (base is not None)
                // Use 'py::array::ensure' which calls 'PyArray_FromAny' namely,
                // PyArray_FromAny(arr, nullptr, 0, 0, NPY_ARRAY_ENSUREARRAY | py::array::c_style, nullptr)
                // Note: c_style ensures that data is copied (if needed!) into C-order as expected by C++ NDArray
                py::array newarr = py::array::ensure(arr, py::array::c_style); // steal reference
                if (newarr) {
                    // Increment Python array ref counter again to compensate decrementing
                    // because of 'newarr' destruction
                    Py_INCREF(newarr.ptr());
                    // data()/mutable_data()  points to the same location in internal buffer
                    // but mutable_data() checks in addition "writable" bit in arr.flags()
                    char* data = static_cast<char*>(const_cast<void*>(newarr.data()));
                    // Store array via PyArrayDeleter constructor
                    auto pydeleter = PyArrayDeleter(newarr.ptr());
                    // Construct dataPtr with deleter which decrements Python refcount
                    dataPtr = NDArray::DataPointer(data, pydeleter);
                }
                // Here 'newarr' goes out of scope, destructor is called (py::object) which
                // decrements ref. count for 'arr'
            }
            if (!dataPtr) {
                throw KARABO_PYTHON_EXCEPTION("Failed conversion of Python ndarray to C++ NDArray.");
            }

            // Construct NDArray
            return NDArray(dataPtr, krbRefType, nelems, Dims(dims));
        }


        py::object copyNDArrayToPy(const karabo::data::NDArray& ndarray) {
            using namespace karabo::data;
            // calculate 'dtype'
            const Types::ReferenceType krbRefType = ndarray.getType();
            const int typenum = Types::to<ToNumpy>(krbRefType);
            py::object dtype = py::dtype(typenum);
            // calculate 'shape'
            const size_t itemsize = Types::to<ToSize>(krbRefType);
            const Dims dims = ndarray.getShape();
            const int ndims = dims.rank();
            std::vector<ssize_t> shape(ndims, 0);
            for (int i = 0; i < ndims; ++i) shape[i] = dims.extentIn(i);
            // calculate 'data' ptr
            const ByteArray& bytearr = ndarray.getByteArray();
            if (dims.size() * itemsize > bytearr.second) {
                throw KARABO_PARAMETER_EXCEPTION("Inconsistent NDArray: " + toString(bytearr.second) +=
                                                 " are too few bytes for shape [" + toString(dims.toVector()) +=
                                                 "] of " + Types::to<ToLiteral>(krbRefType));
            }
            void* ptr = static_cast<void*>(bytearr.first.get());
            // create new array copy since base is not used
            return py::array(dtype, shape, {}, ptr);
        }


        karabo::data::NDArray copyPyArrayToND(py::array arr) {
            using namespace karabo::data;
            // calculate Dims
            size_t ndims = arr.ndim();
            const ssize_t* shape = arr.shape();
            std::vector<unsigned long long> dims(ndims);
            for (size_t i = 0; i < ndims; ++i) dims[i] = shape[i];
            // calculate karabo reference type ...
            py::dtype dt = arr.dtype();
            int typenum = dt.num();
            Types::ReferenceType krbRefType = Types::from<FromNumpy>(typenum);
            // calculate number of elements
            size_t nelems = arr.size();
            // allocate space for data copy ...
            NDArray::DataPointer dataCopy = NDArray::DataPointer(new char[arr.nbytes()]);
            // get mutable data as char* to copy from ...
            char* data = static_cast<char*>(arr.mutable_data());
            // FIXME: If arr is Fortran order, this is wrong...
            // copy 'nbytes' from python array
            std::copy(data, data + arr.nbytes(), dataCopy.get());
            // Construct NDArray using data copy...
            return NDArray(dataCopy, krbRefType, nelems, Dims(dims));
        }


        size_t numArgs(const py::object& o) {
            size_t result = 0;
            size_t numSelfArgs = 0;

            PyObject* function_object = NULL;
            // We expect either
            // * standalone function (== types.FunctionType)
            // * member method (== types.MethodType)
            // * object with __call__ attribute ( ? types.BuiltinFunctionType ?)
            if (PyFunction_Check(o.ptr())) {
                function_object = o.ptr();
            } else if (PyMethod_Check(o.ptr())) {
                function_object = PyMethod_Function(o.ptr());
                numSelfArgs = 1;
            } else if (py::hasattr(o, "__call__")) {
                py::object call = o.attr("__call__");
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


        karabo::data::Hash deepCopy_r(const karabo::data::Hash& h) {
            karabo::data::Hash r;
            // iterate through all entries of the Hash. If the value of the Hash::Node at it
            // is not of a Hash type we insert into our result Hash r, if not we recursivly
            // call deepCopy_r to copy the internal structure
            // We make sure to maintain attributes
            for (karabo::data::Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                if (it->getType() == karabo::data::Types::HASH) {
                    karabo::data::Hash::Node& n = r.set(it->getKey(), deepCopy_r(it->getValue<karabo::data::Hash>()));
                    n.setAttributes(it->getAttributes());
                } else if (it->getType() == karabo::data::Types::VECTOR_HASH) {
                    const std::vector<karabo::data::Hash>& v = it->getValue<std::vector<karabo::data::Hash>>();
                    karabo::data::Hash::Node& n = r.set(it->getKey(), std::vector<karabo::data::Hash>());
                    std::vector<karabo::data::Hash>& vc = n.getValue<std::vector<karabo::data::Hash>>();
                    vc.reserve(v.size());
                    for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                        vc.push_back(deepCopy_r(*vit));
                    }
                    n.setAttributes(it->getAttributes());
                } else if (it->getType() == karabo::data::Types::HASH_POINTER) {
                    karabo::data::Hash::Node& n =
                          r.set(it->getKey(), deepCopy_r(*(it->getValue<karabo::data::Hash::Pointer>())));
                    n.setAttributes(it->getAttributes());
                } else if (it->getType() == karabo::data::Types::VECTOR_HASH_POINTER) {
                    const std::vector<karabo::data::Hash::Pointer>& v =
                          it->getValue<std::vector<karabo::data::Hash::Pointer>>();
                    karabo::data::Hash::Node& n = r.set(it->getKey(), std::vector<karabo::data::Hash>());
                    std::vector<karabo::data::Hash>& vc = n.getValue<std::vector<karabo::data::Hash>>();
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


        py::object deepCopyHashLike(const py::object& obj) {
            using namespace karabo::data;
            // we only check for Hash typed objects, which basically means obj
            // contains a Hash::Node, a Hash, a vector of Hashes or pointers to Hashes
            if (py::isinstance<Hash::Node&>(obj)) {
                // Hash::Node case - check type information of the value and deep copy for aforementioned
                // Hash types
                const karabo::data::Hash::Node& node = obj.cast<Hash::Node&>();
                if (node.getType() == Types::HASH) {
                    return py::cast(deepCopy_r(node.getValue<Hash>()));
                } else if (node.getType() == Types::VECTOR_HASH) {
                    const std::vector<Hash>& v = node.getValue<std::vector<Hash>>();
                    std::vector<Hash> vc;
                    vc.reserve(v.size());
                    for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                        vc.push_back(deepCopy_r(*vit));
                    }
                    return py::cast(vc);
                } else if (node.getType() == Types::HASH_POINTER) {
                    return py::cast(deepCopy_r(*node.getValue<Hash::Pointer>()));
                } else if (node.getType() == Types::VECTOR_HASH_POINTER) {
                    const std::vector<Hash::Pointer>& v = node.getValue<std::vector<Hash::Pointer>>();
                    std::vector<Hash> vc;
                    vc.reserve(v.size());
                    for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                        vc.push_back(deepCopy_r(**vit));
                    }
                    return py::cast(vc);
                } else { // if no Hash like object was found we just return the object
                    return obj;
                }
                // obj contains a Hash
            } else if (py::isinstance<Hash&>(obj)) {
                const Hash& hash = obj.cast<Hash&>();
                return py::cast(deepCopy_r(hash));
                // obj contains a Hash::Pointer
            } else if (py::isinstance<Hash::Pointer&>(obj)) {
                const Hash::Pointer& hp = obj.cast<Hash::Pointer&>();
                return py::cast(deepCopy_r(*hp));
                // obj contains a vector<Hash>
            } else if (py::isinstance<std::vector<Hash>&>(obj)) {
                const std::vector<Hash>& v = obj.cast<std::vector<Hash>&>();
                std::vector<karabo::data::Hash> vc;
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(*vit));
                }
                return py::cast(vc);
                // final scenario to deep copy: vector<Hash::Pointer>
            } else if (py::isinstance<std::vector<Hash::Pointer>&>(obj)) {
                const std::vector<Hash::Pointer> v = obj.cast<std::vector<Hash::Pointer>>();
                std::vector<Hash> vc;
                vc.reserve(v.size());
                for (auto vit = v.cbegin(); vit != v.cend(); ++vit) {
                    vc.push_back(deepCopy_r(**vit));
                }
                return py::cast(vc);
            } else { // nothing to deep-copy
                return obj;
            }
        }


        void attrToStream(std::ostream& os, const karabo::data::Hash& attrs) {
            using namespace karabo::data;
            for (Hash::const_iterator ait = attrs.begin(); ait != attrs.end(); ++ait) {
                if (ait != attrs.begin()) os << ", ";
                os << "'" << ait->getKey() << "': ";
                auto atype = ait->getType();
                if (atype == Types::HASH) {
                    os << "{";
                    attrToStream(os, ait->getValue<Hash>());
                    os << "}";
                } else {
                    if (atype == Types::STRING) os << "'";
                    os << ait->getValueAs<std::string>();
                    if (atype == Types::STRING) os << "'";
                }
            }
        }


        void hashToStream(std::ostream& os, const karabo::data::Hash& hash, int depth) {
            using namespace karabo::data;
            const std::string fill(depth * 4, ' ');

            for (Hash::const_iterator hit = hash.begin(); hit != hash.end(); ++hit) {
                os << fill << hit->getKey();

                os << "{";
                const Hash::Attributes& attrs = hit->getAttributes();
                if (attrs.size() > 0) {
                    for (Hash::Attributes::const_iterator ait = attrs.begin(); ait != attrs.end(); ++ait) {
                        if (ait != attrs.begin()) os << ", ";
                        auto atype = ait->getType();
                        os << "'" << ait->getKey() << "': ";
                        if (atype == Types::HASH) {
                            os << "{";
                            attrToStream(os, ait->getValue<Hash>());
                            os << "}";
                        } else {
                            if (atype == Types::STRING) os << "'";
                            os << ait->getValueAs<std::string>();
                            if (atype == Types::STRING) os << "'";
                        }
                    }
                }
                os << "}";

                Types::ReferenceType type = hit->getType();
                if (type == Types::HASH) {
                    const Hash& hh = hit->getValue<Hash>();
                    if (hh.empty()) {
                        os << ": <> => HASH\n";
                    } else {
                        os << std::endl;
                        hashToStream(os, hh, depth + 1);
                    }
                } else if (type == Types::HASH_POINTER) {
                    const Hash::Pointer& hh = hit->getValue<Hash::Pointer>();
                    if (hh->empty()) {
                        os << ": <> => HASH_POINTER\n";
                    } else {
                        os << std::endl;
                        hashToStream(os, *hh, depth + 1);
                    }
                } else if (type == Types::VECTOR_HASH) {
                    const std::vector<Hash>& hashes = hit->getValue<std::vector<Hash>>();
                    os << std::endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "  [" << i << "]" << std::endl;
                        hashToStream(os, hashes[i], depth + 1);
                    }
                } else if (type == Types::VECTOR_HASH_POINTER) {
                    const std::vector<Hash::Pointer>& hashes = hit->getValue<std::vector<Hash::Pointer>>();
                    os << "  (Pointer)" << std::endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "  [" << i << "]" << std::endl;
                        hashToStream(os, *(hashes[i]), depth + 1);
                    }
                } else if (type == Types::SCHEMA) {
                    // Avoid dependence of Hash on Schema:
                    os << ": <...> => " /* << hit->getValue<Schema>() */ << Types::to<ToLiteral>(type) << std::endl;
                } else if (type == Types::UNKNOWN) {
                    os << ": " << hit->type().name() << " => " << Types::to<ToLiteral>(type) << std::endl;
                } else {
                    os << ": " << hit->getValueAsShortString(100) << " => " << Types::to<ToLiteral>(type) << std::endl;
                }
            }
        }
    } // namespace wrapper


    std::tuple<std::string, std::string> getPythonExceptionStrings(py::error_already_set& e) {
        PyObject *pexceptType(e.type().ptr()), *pexception(e.value().ptr()), *ptraceback(e.trace().ptr());

        // Use the existing code from karathon
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

        // Call the following function may result in SEGFAULT at interpreter epilogue code
        // because ref. count is decrementing ... so we comment this ...
        // PyErr_Restore(pexceptType, pexception, ptraceback); // ref count decremented
        PyErr_Clear();
        // Remove trailing newline
        boost::algorithm::trim_right(pythonErrorMessage);
        boost::algorithm::trim_right(pythonErrorDetails);

        return std::make_tuple(pythonErrorMessage, pythonErrorDetails);
    }

    namespace detail {
        void treatError_already_set(py::error_already_set& e, const py::object& handler, const char* where) {
            std::string errStr, errDetails;
            std::tie(errStr, errDetails) = getPythonExceptionStrings(e);
            const std::string funcName(py::hasattr(handler, "__name__")
                                             ? std::string(handler.attr("__name__").cast<std::string>())
                                             : std::string()); // e.g. 'partial' does not provide __name__
            std::ostringstream oss;
            oss << "Error in ";
            if (funcName.empty()) {
                oss << "python handler for '" << (where ? where : "undefined") << "'";
            } else {
                oss << "'" << funcName << "'";
            }
            oss << ": " << errStr;
            errStr = oss.str();
            std::cerr << '\n' << errStr << '\n' << errDetails << std::endl;
            throw KARABO_PYTHON_EXCEPTION2(errStr, errDetails);
        }
    } // namespace detail
} // namespace karabind
