/*
 * File:   Wrapper.cc
 * Author: CONTROLS DEV group
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Wrapper.hh"

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <boost/filesystem.hpp>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Schema.hh>

#include "PyTypes.hh"

using namespace std;


namespace karabind {
    namespace wrapper {
        namespace detail {

            // Define this macro to guarantee the same conversion rules for 2 types:
            // 'karabo::util::Hash::Node' and 'karabo::util::Hash::Attributes::Node'
            // (both are based on `karabo::util::Element` class with different template args)
#define CAST_ELEMENT_TO_PY(T)                                                                   \
    py::object castElementToPy(const T& self, const karabo::util::Types::ReferenceType& type) { \
        using namespace karabo::util;                                                           \
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
            case Types::VECTOR_INT8: {                                                          \
                const auto& v = self.getValueAs<signed char, std::vector>();                    \
                return py::bytearray(reinterpret_cast<const char*>(v.data()), v.size());        \
            }                                                                                   \
            case Types::VECTOR_UINT8: {                                                         \
                const auto& v = self.getValueAs<unsigned char, std::vector>();                  \
                return py::bytearray(reinterpret_cast<const char*>(v.data()), v.size());        \
            }                                                                                   \
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
            default:                                                                            \
                break;                                                                          \
        }                                                                                       \
        std::ostringstream oss;                                                                 \
        oss << "Type " << Types::to<karabo::util::ToLiteral>(type) << " is not yet supported";  \
        throw KARABO_NOT_SUPPORTED_EXCEPTION(oss.str());                                        \
    }

            // NOTE: Use of metaprogramming here directly does not work: compiler failsto deduce substitutions
            // correctly. It can be implemented using some indirection but it is more verbose and we stick with above
            // macro. We need the following Node types ...
            CAST_ELEMENT_TO_PY(karabo::util::Hash::Attributes::Node)
            CAST_ELEMENT_TO_PY(karabo::util::Hash::Node)

        } // namespace detail


        karabo::util::Types::ReferenceType pyObjectToCppType(const py::object& otype) {
            using namespace karabo::util;
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


        py::object castAnyToPy(const boost::any& operand) {
            try {
                if (operand.type() == typeid(bool)) {
                    return py::cast(boost::any_cast<bool>(operand));
                } else if (operand.type() == typeid(char)) {
                    return py::cast(boost::any_cast<char>(operand));
                } else if (operand.type() == typeid(signed char)) {
                    return py::cast(boost::any_cast<signed char>(operand));
                } else if (operand.type() == typeid(unsigned char)) {
                    return py::cast(boost::any_cast<unsigned char>(operand));
                } else if (operand.type() == typeid(short)) {
                    return py::cast(boost::any_cast<short>(operand));
                } else if (operand.type() == typeid(unsigned short)) {
                    return py::cast(boost::any_cast<unsigned short>(operand));
                } else if (operand.type() == typeid(int)) {
                    return py::cast(boost::any_cast<int>(operand));
                } else if (operand.type() == typeid(unsigned int)) {
                    return py::cast(boost::any_cast<unsigned int>(operand));
                } else if (operand.type() == typeid(long long)) {
                    return py::cast(boost::any_cast<long long>(operand));
                } else if (operand.type() == typeid(unsigned long long)) {
                    return py::cast(boost::any_cast<unsigned long long>(operand));
                } else if (operand.type() == typeid(float)) {
                    return py::cast(boost::any_cast<float>(operand));
                } else if (operand.type() == typeid(double)) {
                    return py::cast(boost::any_cast<double>(operand));
                } else if (operand.type() == typeid(std::complex<float>)) {
                    return py::cast(boost::any_cast<std::complex<float>>(operand));
                } else if (operand.type() == typeid(std::complex<double>)) {
                    return py::cast(boost::any_cast<std::complex<double>>(operand));
                } else if (operand.type() == typeid(std::string)) {
                    return py::cast(boost::any_cast<std::string>(operand));
                } else if (operand.type() == typeid(boost::filesystem::path)) {
                    return py::cast(boost::any_cast<boost::filesystem::path>(operand).string());
                } else if (operand.type() == typeid(karabo::util::CppNone)) {
                    return py::none();
                } else if (operand.type() == typeid(karabo::util::Hash)) {
                    return py::cast(boost::any_cast<karabo::util::Hash>(operand));
                } else if (operand.type() == typeid(karabo::util::Hash::Pointer)) {
                    return py::cast(boost::any_cast<karabo::util::Hash::Pointer>(operand));
                } else if (operand.type() == typeid(std::vector<bool>)) {
                    return py::cast(boost::any_cast<std::vector<bool>>(operand));
                } else if (operand.type() == typeid(std::vector<char>)) {
                    return py::cast(boost::any_cast<std::vector<char>>(operand));
                } else if (operand.type() == typeid(std::vector<signed char>)) {
                    return py::cast(boost::any_cast<std::vector<signed char>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned char>)) {
                    return py::cast(boost::any_cast<std::vector<unsigned char>>(operand));
                } else if (operand.type() == typeid(std::vector<short>)) {
                    return py::cast(boost::any_cast<std::vector<short>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned short>)) {
                    return py::cast(boost::any_cast<std::vector<unsigned short>>(operand));
                } else if (operand.type() == typeid(std::vector<int>)) {
                    return py::cast(boost::any_cast<std::vector<int>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned int>)) {
                    return py::cast(boost::any_cast<std::vector<unsigned int>>(operand));
                } else if (operand.type() == typeid(std::vector<long long>)) {
                    return py::cast(boost::any_cast<std::vector<long long>>(operand));
                } else if (operand.type() == typeid(std::vector<unsigned long long>)) {
                    return py::cast(boost::any_cast<std::vector<unsigned long long>>(operand));
                } else if (operand.type() == typeid(std::vector<float>)) {
                    return py::cast(boost::any_cast<std::vector<float>>(operand));
                } else if (operand.type() == typeid(std::vector<double>)) {
                    return py::cast(boost::any_cast<std::vector<double>>(operand));
                } else if (operand.type() == typeid(std::vector<std::complex<float>>)) {
                    return py::cast(boost::any_cast<std::vector<std::complex<float>>>(operand));
                } else if (operand.type() == typeid(std::vector<std::complex<double>>)) {
                    return py::cast(boost::any_cast<std::vector<std::complex<double>>>(operand));
                } else if (operand.type() == typeid(std::vector<std::string>)) {
                    return py::cast(boost::any_cast<std::vector<std::string>>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::util::CppNone>)) {
                    const auto& v = boost::any_cast<std::vector<karabo::util::CppNone>>(operand);
                    std::vector<py::object> vo(v.size(), py::none());
                    return py::cast(vo);
                } else if (operand.type() == typeid(karabo::util::Schema)) {
                    return py::cast(boost::any_cast<karabo::util::Schema>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::util::Hash>)) {
                    return py::cast(boost::any_cast<std::vector<karabo::util::Hash>>(operand));
                } else if (operand.type() == typeid(std::vector<karabo::util::Hash::Pointer>)) {
                    return py::cast(boost::any_cast<std::vector<karabo::util::Hash::Pointer>>(operand));
                }
                std::ostringstream oss;
                oss << "Failed to convert inner Hash type: " << operand.type().name() << " to python";
                throw KARABO_PYTHON_EXCEPTION(oss.str());
            } catch (const boost::bad_any_cast& e) {
                KARABO_RETHROW_AS(KARABO_CAST_EXCEPTION(e.what()));
            }
            return py::none(); // make compiler happy -- we never reach this statement
        }


        // Helper for Wrapper::toAny below:
        static karabo::util::Types::ReferenceType bestIntegerType(const py::object& obj) {
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


        karabo::util::Types::ReferenceType castPyToAny(const py::object& o, boost::any& any) {
            if (o.is_none()) {
                any = karabo::util::CppNone();
                return karabo::util::Types::NONE;
            }
            if (py::isinstance<py::bool_>(o)) {
                any = py::cast<bool>(o);
                return karabo::util::Types::BOOL;
            }
            if (py::isinstance<py::int_>(o)) {
                const karabo::util::Types::ReferenceType type = bestIntegerType(o);
                if (type == karabo::util::Types::UINT64) {
                    // Raises a Python exception if it overflows:
                    any = o.cast<unsigned long long>();
                } else {
                    const auto value = o.cast<long long>();
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
            if (py::isinstance<py::float_>(o)) {
                any = o.cast<double>();
                return karabo::util::Types::DOUBLE;
            }
            if (PyComplex_Check(o.ptr())) {
                any = o.cast<std::complex<double>>();
                return karabo::util::Types::COMPLEX_DOUBLE;
            }
            if (py::isinstance<py::str>(o)) {
                any = o.cast<std::string>();
                return karabo::util::Types::STRING;
            }
            if (py::isinstance<py::bytes>(o)) {
                const auto& s = o.cast<std::string>();
                any = std::vector<char>(s.begin(), s.end());
                return karabo::util::Types::VECTOR_CHAR;
            }
            if (py::isinstance<py::bytearray>(o)) {
                const auto& s = o.cast<std::string>();
                any = std::vector<char>(s.begin(), s.end());
                return karabo::util::Types::VECTOR_CHAR;
            }
            if (py::isinstance<karabo::util::Hash>(o)) {
                any = o.cast<karabo::util::Hash>();
                return karabo::util::Types::HASH;
            }
            if (py::isinstance<karabo::util::Hash::Pointer>(o)) {
                any = o.cast<karabo::util::Hash::Pointer>();
                return karabo::util::Types::HASH_POINTER;
            }
            if (py::isinstance<karabo::util::Schema>(o)) {
                any = o.cast<karabo::util::Schema>();
                return karabo::util::Types::SCHEMA;
            }
            if (py::isinstance<std::vector<karabo::util::Hash>>(o)) {
                any = o.cast<std::vector<karabo::util::Hash>>();
                return karabo::util::Types::VECTOR_HASH;
            }
            if (py::isinstance<std::vector<karabo::util::Hash::Pointer>>(o)) {
                any = o.cast<std::vector<karabo::util::Hash::Pointer>>();
                return karabo::util::Types::VECTOR_HASH_POINTER;
            }
            if (py::isinstance<py::list>(o)) {
                // python list object ...
                py::list lo = o.cast<py::list>();
                size_t size = py::len(lo);
                if (size == 0) {
                    any = std::vector<std::string>();
                    return karabo::util::Types::VECTOR_STRING;
                }
                py::object list0 = lo[0];
                if (list0.is_none()) {
                    any = std::vector<karabo::util::CppNone>(size, karabo::util::CppNone());
                    return karabo::util::Types::VECTOR_NONE;
                }
                if (py::isinstance<py::bool_>(list0)) {
                    any = o.cast<std::vector<bool>>();
                    return karabo::util::Types::VECTOR_BOOL;
                }
                if (py::isinstance<py::int_>(list0)) {
                    // First item is an integer - assume that all items are!
                    karabo::util::Types::ReferenceType broadestType = karabo::util::Types::INT32;
                    for (size_t i = 0; i < size; ++i) {
                        const karabo::util::Types::ReferenceType type = bestIntegerType(lo[i]);
                        // This relies on the fact that the enums ReferenceType have the order INT32, UINT32, INT64,
                        // UINT64
                        if (type > broadestType) {
                            broadestType = type;
                            // Stop loop if cannot get broader...
                            if (broadestType == karabo::util::Types::UINT64) break;
                        }
                    }
                    if (broadestType == karabo::util::Types::INT32) {
                        any = o.cast<std::vector<int>>();
                        return karabo::util::Types::VECTOR_INT32;
                    } else if (broadestType == karabo::util::Types::UINT32) {
                        any = o.cast<std::vector<unsigned int>>();
                        return karabo::util::Types::VECTOR_UINT32;
                    } else if (broadestType == karabo::util::Types::INT64) {
                        any = o.cast<std::vector<long long>>();
                        return karabo::util::Types::VECTOR_INT64;
                    } else if (broadestType == karabo::util::Types::UINT64) {
                        any = o.cast<std::vector<unsigned long long>>();
                        return karabo::util::Types::VECTOR_UINT64;
                    } else {
                        // Should never come here!
                        throw KARABO_PYTHON_EXCEPTION("Unsupported int type " + toString(broadestType));
                    }
                }
                if (py::isinstance<py::float_>(list0)) {
                    any = o.cast<std::vector<double>>();
                    return karabo::util::Types::VECTOR_DOUBLE;
                }
                if (PyComplex_Check(list0.ptr())) {
                    any = o.cast<std::vector<std::complex<double>>>();
                    return karabo::util::Types::VECTOR_COMPLEX_DOUBLE;
                }
                if (py::isinstance<py::str>(list0)) {
                    any = o.cast<std::vector<std::string>>();
                    return karabo::util::Types::VECTOR_STRING;
                }
                if (py::isinstance<karabo::util::Hash>(list0)) {
                    // convert py::list of Hash into VectorHash object since we use `bind_vector`:
                    // vho = VectorHash(list_of_Hash) like VectorHash([Hash(...), Hash(...),...])
                    // TODO: Check if py::implicitly_convertable can be used...
                    auto vho = py::module_::import("karabind").attr("VectorHash")(o);
                    any = vho.cast<std::vector<karabo::util::Hash>>();
                    return karabo::util::Types::VECTOR_HASH;
                }
                if (py::isinstance<karabo::util::Hash::Pointer>(list0)) {
                    any = o.cast<std::vector<karabo::util::Hash::Pointer>>();
                    return karabo::util::Types::VECTOR_HASH_POINTER;
                }
                if (py::isinstance<karabo::util::Schema>(list0)) {
                    any = o.cast<std::vector<karabo::util::Schema>>();
                    return karabo::util::Types::VECTOR_SCHEMA;
                }
            }
            if (py::hasattr(o, "__name__")) { // python function
                any = o;
                return karabo::util::Types::ANY;
            }
            throw KARABO_PYTHON_EXCEPTION("Python type can not be mapped into Hash");
        }
    } // namespace wrapper

} // namespace karabind
