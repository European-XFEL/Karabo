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
 * Copyright (C) 2010 European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_TYPES_HH
#define	KARABO_UTIL_TYPES_HH

#include <string>
#include <complex>
#include <vector>

namespace karabo {
    namespace util {

        // Forward ToType
        template<class To>
        class ToType;

        // Forward FromType
        template<class To>
        class FromType;
        
        class CppNone {};

        class Types {

        public:

            enum ReferenceType {

                BOOL, // bool
                VECTOR_BOOL, // std::vector<std::bool>

                CHAR, // char
                VECTOR_CHAR, // std::vector<char>
                INT8, // signed char
                VECTOR_INT8, // std::vector<std::signed char>
                UINT8, // unsigned char
                VECTOR_UINT8, // std::vector<std::unsigned char>

                INT16, // signed short
                VECTOR_INT16, // std::vector<std::signed short>
                UINT16, // unsigned short
                VECTOR_UINT16, // std::vector<std::unsigned short>

                INT32, // signed int
                VECTOR_INT32, // std::vector<std::int>
                UINT32, // unsigned int
                VECTOR_UINT32, // std::vector<std::unsigned int>

                INT64, // signed long long
                VECTOR_INT64, // std::vector<std::signed long long>
                UINT64, // unsigned long long
                VECTOR_UINT64, // std::vector<std::unsigned long long>

                FLOAT, // float
                VECTOR_FLOAT, // std::vector<std::float>

                DOUBLE, // double
                VECTOR_DOUBLE, // std::vector<std::double>

                COMPLEX_FLOAT, // std::complex<float>
                VECTOR_COMPLEX_FLOAT, // std::vector<std::complex<float>

                COMPLEX_DOUBLE, // std::complex<double>
                VECTOR_COMPLEX_DOUBLE, // std::vector<std::complex<double>

                STRING, // std::string
                VECTOR_STRING, // std::vector<std::string>

                HASH, // Hash
                VECTOR_HASH, // std::vector<Hash>

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
                PTR_STRING,              

                SCHEMA, // Schema
                VECTOR_SCHEMA, // std::vector<Schema>

                ANY, // unspecified type
                NONE,  // CppNone type used during serialization/de-serialization
                VECTOR_NONE,

                UNKNOWN, // unknown type
                SIMPLE,
                SEQUENCE,
                POINTER,
                RAW_ARRAY,

                ARRAY_BOOL, // std::pair<bool*, size_t>
                ARRAY_CHAR, // std::pair<char*, size_t>
                ARRAY_INT8, // std::pair<signed char*, size_t>
                ARRAY_UINT8, // std::pair<unsigned char*, size_t>
                ARRAY_INT16, // std::pair<short*, size_t>
                ARRAY_UINT16, // std::pair<unsigned short*, size_t>
                ARRAY_INT32, // std::pair<int*, size_t>
                ARRAY_UINT32, // std::pair<unsigned int*, size_t>
                ARRAY_INT64, // std::pair<long long*, size_t>
                ARRAY_UINT64, // std::pair<unsigned long long*, size_t>
                ARRAY_FLOAT, // std::pair<float*, size_t>
                ARRAY_DOUBLE, // std::pair<double*, size_t>
                        
                HASH_POINTER, // Hash::Pointer 
                VECTOR_HASH_POINTER // std::vector<Hash::Pointer>
                        
            };

            template <class From, class To>
            static typename To::ReturnType convert(const typename From::ArgumentType& type) {
                return ToType<To>::to(FromType<From>::from(type));
            }

            template <class From>
            static ReferenceType from(const typename From::ArgumentType& type) {
                return FromType<From>::from(type);
            }

            template <class To>
            static typename To::ReturnType to(const ReferenceType type) {
                return ToType<To>::to(type);
            }

            template<typename T>
            static ReferenceType from(const T& var = T()) {
                // This function does not compile by purpose. There are template specializations for each allowed data type
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
                    case Types::ARRAY_CHAR:
                    case Types::ARRAY_INT8:
                    case Types::ARRAY_INT16:
                    case Types::ARRAY_INT32:
                    case Types::ARRAY_INT64:
                    case Types::ARRAY_UINT8:
                    case Types::ARRAY_UINT16:
                    case Types::ARRAY_UINT32:
                    case Types::ARRAY_UINT64:
                    case Types::ARRAY_DOUBLE:
                    case Types::ARRAY_FLOAT:
                    case Types::ARRAY_BOOL:                    
                        return RAW_ARRAY;
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

             static bool isRawArray(int type) {

                switch (type) {
                    case Types::ARRAY_CHAR:
                    case Types::ARRAY_INT8:
                    case Types::ARRAY_INT16:
                    case Types::ARRAY_INT32:
                    case Types::ARRAY_INT64:
                    case Types::ARRAY_UINT8:
                    case Types::ARRAY_UINT16:
                    case Types::ARRAY_UINT32:
                    case Types::ARRAY_UINT64:
                    case Types::ARRAY_DOUBLE:
                    case Types::ARRAY_FLOAT:
                    case Types::ARRAY_BOOL:
                        return true;
                    default:
                        return false;
                }
            }



        };

        #define _KARABO_HELPER_MACRO(RefType, CppType) \
         template <> inline Types::ReferenceType Types::from<CppType>(const CppType&) { return Types::RefType; } \
         template <> inline Types::ReferenceType Types::from<std::vector<CppType> > (const std::vector<CppType>&) { return Types::VECTOR_##RefType; }

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

        #define _KARABO_HELPER_MACRO(RefType, CppType) \
         template <> inline Types::ReferenceType Types::from<CppType*>(CppType* const&) { return Types::RefType; }

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

        #define _KARABO_HELPER_MACRO(RefType, CppType) \
         template <> inline Types::ReferenceType Types::from<std::pair<const CppType*, size_t > >(const std::pair<const CppType*, size_t>&) { return Types::RefType; }

        _KARABO_HELPER_MACRO(ARRAY_BOOL, bool)
        _KARABO_HELPER_MACRO(ARRAY_CHAR, char)
        _KARABO_HELPER_MACRO(ARRAY_INT8, signed char)
        _KARABO_HELPER_MACRO(ARRAY_UINT8, unsigned char)
        _KARABO_HELPER_MACRO(ARRAY_INT16, short)
        _KARABO_HELPER_MACRO(ARRAY_UINT16, unsigned short)
        _KARABO_HELPER_MACRO(ARRAY_INT32, int)
        _KARABO_HELPER_MACRO(ARRAY_UINT32, unsigned int)
        _KARABO_HELPER_MACRO(ARRAY_INT64, long long)
        _KARABO_HELPER_MACRO(ARRAY_UINT64, unsigned long long)
        _KARABO_HELPER_MACRO(ARRAY_FLOAT, float)
        _KARABO_HELPER_MACRO(ARRAY_DOUBLE, double)

        #undef _KARABO_HELPER_MACRO

    }
}

#endif