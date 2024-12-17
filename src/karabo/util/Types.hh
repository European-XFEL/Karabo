/*
 * $Id: Types.hh 4978 2012-01-18 10:43:06Z wegerk@DESY.DE $
 *
 * File:   Types.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 13, 2010, 6:55 PM
 *
 * Major update on January 17, 2013 9:00 AM
 * Contributions by: <djelloul.boukhelef@xfel.eu>
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

#ifndef KARABO_UTIL_TYPES_HH
#define KARABO_UTIL_TYPES_HH

// Compiling packages depending on Karabo using C++11 only
// and link them against the C++14 compiled framework seems to work.
// If these packages are compiled with a gcc versions lower than 7,
// linking problems with libstdc are seen.
// Here we bail out if such a compiler version is detected.

// Accept clang as it is used by many IDE tools.
#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ < 7
#error Compiling Karabo with gcc requires at least gcc version 7.
#endif
#endif

#include <complex>
#include <memory>
#include <string>
#include <vector>

namespace karabo {

    namespace util {

        typedef std::pair<std::shared_ptr<char>, size_t> ByteArray;

        // Forward ToType
        template <class To>
        class ToType;

        // Forward FromType
        template <class To>
        class FromType;

        class CppNone {};

        /**
         * @class Types
         * @brief This class contains the types knonw to the Karabo framework as members
         *        and conversion tools for these
         */
        class Types {
           public:
            enum ReferenceType {

                BOOL,        // bool
                VECTOR_BOOL, // std::vector<std::bool>

                CHAR,         // char
                VECTOR_CHAR,  // std::vector<char>
                INT8,         // signed char
                VECTOR_INT8,  // std::vector<std::signed char>
                UINT8,        // unsigned char
                VECTOR_UINT8, // std::vector<std::unsigned char>

                INT16,         // short
                VECTOR_INT16,  // std::vector<std::short>
                UINT16,        // unsigned short
                VECTOR_UINT16, // std::vector<std::unsigned short>

                INT32,         // int
                VECTOR_INT32,  // std::vector<std::int>
                UINT32,        // unsigned int
                VECTOR_UINT32, // std::vector<std::unsigned int>

                INT64,         // long long
                VECTOR_INT64,  // std::vector<std::long long>
                UINT64,        // unsigned long long
                VECTOR_UINT64, // std::vector<std::unsigned long long>

                FLOAT,        // float
                VECTOR_FLOAT, // std::vector<std::float>

                DOUBLE,        // double
                VECTOR_DOUBLE, // std::vector<std::double>

                COMPLEX_FLOAT,        // std::complex<float>
                VECTOR_COMPLEX_FLOAT, // std::vector<std::complex<float>

                COMPLEX_DOUBLE,        // std::complex<double>
                VECTOR_COMPLEX_DOUBLE, // std::vector<std::complex<double>

                STRING,        // std::string
                VECTOR_STRING, // std::vector<std::string>

                HASH,        // Hash
                VECTOR_HASH, // std::vector<Hash>

                SCHEMA,        // Schema
                VECTOR_SCHEMA, // std::vector<Schema>

                ANY,  // unspecified type
                NONE, // CppNone type used during serialization/de-serialization
                VECTOR_NONE,

                BYTE_ARRAY, // std::pair<shared_ptr<char>, size_t> -> ByteArray

                UNKNOWN, // unknown type
                SIMPLE,
                SEQUENCE,
                POINTER,

                HASH_POINTER,        // Hash::Pointer
                VECTOR_HASH_POINTER, // std::vector<Hash::Pointer>

                PTR_BOOL,
                PTR_CHAR,
                PTR_INT8,
                PTR_UINT8,
                PTR_INT16,
                PTR_UINT16,
                PTR_INT32,
                PTR_UINT32,
                PTR_INT64,
                PTR_UINT64,
                PTR_FLOAT,
                PTR_DOUBLE,
                PTR_COMPLEX_FLOAT,
                PTR_COMPLEX_DOUBLE,
                PTR_STRING
            };

            /**
             * Convert one type representation to another, e.g.
             *
             * @code
             *      size_t size = convert<FromLiteral, ToSize>("INT32");
             *      # size is 4
             * @endcode
             *
             * @param type
             * @return
             */
            template <class From, class To>
            static typename To::ReturnType convert(const typename From::ArgumentType& type) {
                return ToType<To>::to(FromType<From>::from(type));
            }

            /**
             * Return a Types::ReferenceType from an alternate representation
             *
             * @code
             *      Types::ReferenceType r = from<FromLiteral>("INT64");
             * @endcode
             *
             * @param type
             * @return
             */
            template <class From>
            static ReferenceType from(const typename From::ArgumentType& type) {
                return FromType<From>::from(type);
            }

            /**
             * Return an alternate representation of a Types::ReferenceType
             *
             *
             * @param type
             * @return
             */
            template <class To>
            static typename To::ReturnType to(const ReferenceType type) {
                return ToType<To>::to(type);
            }

            template <typename T>
            static ReferenceType from(const T& var = T()) {
                // This function does not compile by purpose. There are template specializations for each allowed data
                // type
                return this_type_is_not_supported_by_purpose(var);
            }

            static ReferenceType category(int type) {
                switch (type) {
                    case Types::CHAR:
                    case Types::INT8:
                    case Types::INT16:
                    case Types::INT32:
                    case Types::INT64:
                    case Types::UINT8:
                    case Types::UINT16:
                    case Types::UINT32:
                    case Types::UINT64:
                    case Types::FLOAT:
                    case Types::DOUBLE:
                    case Types::BOOL:
                    case Types::STRING:
                    case Types::COMPLEX_FLOAT:
                    case Types::COMPLEX_DOUBLE:
                    case Types::BYTE_ARRAY:
                    case Types::NONE:
                        return SIMPLE;
                    case Types::VECTOR_STRING:
                    case Types::VECTOR_CHAR:
                    case Types::VECTOR_INT8:
                    case Types::VECTOR_INT16:
                    case Types::VECTOR_INT32:
                    case Types::VECTOR_INT64:
                    case Types::VECTOR_UINT8:
                    case Types::VECTOR_UINT16:
                    case Types::VECTOR_UINT32:
                    case Types::VECTOR_UINT64:
                    case Types::VECTOR_DOUBLE:
                    case Types::VECTOR_FLOAT:
                    case Types::VECTOR_BOOL:
                    case Types::VECTOR_COMPLEX_FLOAT:
                    case Types::VECTOR_COMPLEX_DOUBLE:
                    case Types::VECTOR_NONE:
                    case Types::PTR_STRING:
                    case Types::PTR_CHAR:
                    case Types::PTR_INT8:
                    case Types::PTR_INT16:
                    case Types::PTR_INT32:
                    case Types::PTR_INT64:
                    case Types::PTR_UINT8:
                    case Types::PTR_UINT16:
                    case Types::PTR_UINT32:
                    case Types::PTR_UINT64:
                    case Types::PTR_DOUBLE:
                    case Types::PTR_FLOAT:
                    case Types::PTR_BOOL:
                    case Types::PTR_COMPLEX_FLOAT:
                    case Types::PTR_COMPLEX_DOUBLE:
                        return SEQUENCE;
                    case Types::VECTOR_HASH:
                        return VECTOR_HASH;
                    case Types::VECTOR_HASH_POINTER:
                        return VECTOR_HASH_POINTER;
                    case Types::HASH:
                        return HASH;
                    case Types::HASH_POINTER:
                        return HASH_POINTER;
                    case Types::SCHEMA:
                        return SCHEMA;
                    case Types::ANY:
                        return ANY;
                    default:
                        return UNKNOWN;
                }
            }

            /**
             * Check if the passed Types::ReferenceType is a pointer
             * @param type
             * @return
             */
            static bool isPointer(int type) {
                //                if(type >= Types::PTR_BOOL && type <= Types::PTR_STRING) return true;
                //                return false;


                switch (type) {
                    case Types::PTR_STRING:
                    case Types::PTR_CHAR:
                    case Types::PTR_INT8:
                    case Types::PTR_INT16:
                    case Types::PTR_INT32:
                    case Types::PTR_INT64:
                    case Types::PTR_UINT8:
                    case Types::PTR_UINT16:
                    case Types::PTR_UINT32:
                    case Types::PTR_UINT64:
                    case Types::PTR_DOUBLE:
                    case Types::PTR_FLOAT:
                    case Types::PTR_BOOL:
                    case Types::PTR_COMPLEX_FLOAT:
                    case Types::PTR_COMPLEX_DOUBLE:
                        return true;
                    default:
                        return false;
                }
            }

            /**
             * Check if the passed Types::ReferenceType is a vector
             * @param type
             * @return
             */
            static bool isVector(int type) {
                switch (type) {
                    case Types::VECTOR_STRING:
                    case Types::VECTOR_CHAR:
                    case Types::VECTOR_INT8:
                    case Types::VECTOR_INT16:
                    case Types::VECTOR_INT32:
                    case Types::VECTOR_INT64:
                    case Types::VECTOR_UINT8:
                    case Types::VECTOR_UINT16:
                    case Types::VECTOR_UINT32:
                    case Types::VECTOR_UINT64:
                    case Types::VECTOR_DOUBLE:
                    case Types::VECTOR_FLOAT:
                    case Types::VECTOR_BOOL:
                    case Types::VECTOR_COMPLEX_FLOAT:
                    case Types::VECTOR_COMPLEX_DOUBLE:
                    case Types::VECTOR_HASH:
                    case Types::VECTOR_NONE:
                        return true;
                    default:
                        return false;
                }
            }

            /**
             * Check if the passed Types::ReferenceType is numeric plain old data type (POD)
             * @param type
             * @return
             */
            static bool isNumericPod(int type) {
                switch (type) {
                    case Types::BOOL:
                    case Types::INT8:
                    case Types::INT16:
                    case Types::INT32:
                    case Types::INT64:
                    case Types::UINT8:
                    case Types::UINT16:
                    case Types::UINT32:
                    case Types::UINT64:
                    case Types::FLOAT:
                    case Types::DOUBLE:
                        return true;
                    default:
                        return false;
                }
            }

            /**
             * Check if the passed Types::ReferenceType is a type representable by a karabo::util::SimpleElement
             * @param type
             * @return
             */
            static bool isSimple(int type) {
                switch (type) {
                    case Types::CHAR:
                    case Types::INT8:
                    case Types::INT16:
                    case Types::INT32:
                    case Types::INT64:
                    case Types::UINT8:
                    case Types::UINT16:
                    case Types::UINT32:
                    case Types::UINT64:
                    case Types::FLOAT:
                    case Types::DOUBLE:
                    case Types::BOOL:
                    case Types::STRING:
                    case Types::COMPLEX_FLOAT:
                    case Types::COMPLEX_DOUBLE:
                    case Types::NONE:
                        return true;
                    default:
                        return false;
                }
            }
        };

        /*
         * use the types templatized. Use as such:
         *
         *     struct Processor {
         *         template <class T>
         *         inline void operator () (T*) {
         *             // do your business here
         *         }
         *     } processor;
         *
         *     templatize(referenceType, processor);
         *
         * This function deals only with numerical data types and string.
         * Everything more complex you need to do by hand. It returns whether
         * it could handle the data type.
         *
         * Note that the parameter to operator () is only there to make
         * templatization possible. It is always 0.
         */
        template <class Processor>
        inline bool templatize(Types::ReferenceType type, Processor& processor) {
            switch (type) {
                case Types::BOOL:
                    processor(static_cast<bool*>(0));
                    break;
                case Types::CHAR:
                    processor(static_cast<char*>(0));
                    break;
                case Types::INT8:
                    processor(static_cast<signed char*>(0));
                    break;
                case Types::UINT8:
                    processor(static_cast<unsigned char*>(0));
                    break;
                case Types::INT16:
                    processor(static_cast<short*>(0));
                    break;
                case Types::UINT16:
                    processor(static_cast<unsigned short*>(0));
                    break;
                case Types::INT32:
                    processor(static_cast<int*>(0));
                    break;
                case Types::UINT32:
                    processor(static_cast<unsigned int*>(0));
                    break;
                case Types::INT64:
                    processor(static_cast<long long*>(0));
                    break;
                case Types::UINT64:
                    processor(static_cast<unsigned long long*>(0));
                    break;
                case Types::FLOAT:
                    processor(static_cast<float*>(0));
                    break;
                case Types::DOUBLE:
                    processor(static_cast<double*>(0));
                    break;
                case Types::COMPLEX_FLOAT:
                    processor(static_cast<std::complex<float>*>(0));
                    break;
                case Types::COMPLEX_DOUBLE:
                    processor(static_cast<std::complex<double>*>(0));
                    break;
                case Types::STRING:
                    processor(static_cast<std::string*>(0));
                    break;
                case Types::VECTOR_BOOL:
                    processor(static_cast<std::vector<bool>*>(0));
                    break;
                case Types::VECTOR_CHAR:
                    processor(static_cast<std::vector<char>*>(0));
                    break;
                case Types::VECTOR_INT8:
                    processor(static_cast<std::vector<signed char>*>(0));
                    break;
                case Types::VECTOR_UINT8:
                    processor(static_cast<std::vector<unsigned char>*>(0));
                    break;
                case Types::VECTOR_INT16:
                    processor(static_cast<std::vector<short>*>(0));
                    break;
                case Types::VECTOR_UINT16:
                    processor(static_cast<std::vector<unsigned short>*>(0));
                    break;
                case Types::VECTOR_INT32:
                    processor(static_cast<std::vector<int>*>(0));
                    break;
                case Types::VECTOR_UINT32:
                    processor(static_cast<std::vector<unsigned int>*>(0));
                    break;
                case Types::VECTOR_INT64:
                    processor(static_cast<std::vector<long long>*>(0));
                    break;
                case Types::VECTOR_UINT64:
                    processor(static_cast<std::vector<unsigned long long>*>(0));
                    break;
                case Types::VECTOR_FLOAT:
                    processor(static_cast<std::vector<float>*>(0));
                    break;
                case Types::VECTOR_DOUBLE:
                    processor(static_cast<std::vector<double>*>(0));
                    break;
                case Types::VECTOR_COMPLEX_FLOAT:
                    processor(static_cast<std::vector<std::complex<float> >*>(0));
                    break;
                case Types::VECTOR_COMPLEX_DOUBLE:
                    processor(static_cast<std::vector<std::complex<double> >*>(0));
                    break;
                case Types::VECTOR_STRING:
                    processor(static_cast<std::vector<std::string>*>(0));
                    break;
                default:
                    return false;
            }
            return true;
        }


#define _KARABO_HELPER_MACRO(RefType, CppType)                                                    \
    template <>                                                                                   \
    inline Types::ReferenceType Types::from<CppType>(const CppType&) {                            \
        return Types::RefType;                                                                    \
    }                                                                                             \
    template <>                                                                                   \
    inline Types::ReferenceType Types::from<std::vector<CppType> >(const std::vector<CppType>&) { \
        return Types::VECTOR_##RefType;                                                           \
    }

        _KARABO_HELPER_MACRO(BOOL, bool)
        _KARABO_HELPER_MACRO(CHAR, char)
        _KARABO_HELPER_MACRO(INT8, signed char)
        _KARABO_HELPER_MACRO(UINT8, unsigned char)
        _KARABO_HELPER_MACRO(INT16, short)
        _KARABO_HELPER_MACRO(UINT16, unsigned short)
        _KARABO_HELPER_MACRO(INT32, int)
        _KARABO_HELPER_MACRO(UINT32, unsigned int)
        _KARABO_HELPER_MACRO(INT64, long long)
        _KARABO_HELPER_MACRO(UINT64, unsigned long long)
        _KARABO_HELPER_MACRO(FLOAT, float)
        _KARABO_HELPER_MACRO(DOUBLE, double)
        _KARABO_HELPER_MACRO(COMPLEX_FLOAT, std::complex<float>)
        _KARABO_HELPER_MACRO(COMPLEX_DOUBLE, std::complex<double>)
        _KARABO_HELPER_MACRO(STRING, std::string)
        _KARABO_HELPER_MACRO(NONE, karabo::util::CppNone)

#undef _KARABO_HELPER_MACRO

#define _KARABO_HELPER_MACRO(RefType, CppType)                           \
    template <>                                                          \
    inline Types::ReferenceType Types::from<CppType*>(CppType* const&) { \
        return Types::RefType;                                           \
    }

        _KARABO_HELPER_MACRO(PTR_BOOL, bool)
        _KARABO_HELPER_MACRO(PTR_CHAR, char)
        _KARABO_HELPER_MACRO(PTR_INT8, signed char)
        _KARABO_HELPER_MACRO(PTR_UINT8, unsigned char)
        _KARABO_HELPER_MACRO(PTR_INT16, short)
        _KARABO_HELPER_MACRO(PTR_UINT16, unsigned short)
        _KARABO_HELPER_MACRO(PTR_INT32, int)
        _KARABO_HELPER_MACRO(PTR_UINT32, unsigned int)
        _KARABO_HELPER_MACRO(PTR_INT64, long long)
        _KARABO_HELPER_MACRO(PTR_UINT64, unsigned long long)
        _KARABO_HELPER_MACRO(PTR_FLOAT, float)
        _KARABO_HELPER_MACRO(PTR_DOUBLE, double)
        _KARABO_HELPER_MACRO(PTR_COMPLEX_FLOAT, std::complex<float>)
        _KARABO_HELPER_MACRO(PTR_COMPLEX_DOUBLE, std::complex<double>)
        _KARABO_HELPER_MACRO(PTR_STRING, std::string)

#undef _KARABO_HELPER_MACRO

        template <>
        inline Types::ReferenceType Types::from<ByteArray>(const ByteArray&) {
            return Types::BYTE_ARRAY;
        }
    } // namespace util
} // namespace karabo

#endif
