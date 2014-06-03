/* 
 * File:   ToCppString.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013
 * 
 */

#include "ToType.hh"

#ifndef KARABO_UTIL_TOCPPSTRING_HH
#define	KARABO_UTIL_TOCPPSTRING_HH

namespace karabo {

    namespace util {

        class ToCppString {

        public:

            typedef std::string ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, BOOL, "bool")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_BOOL, "vector<bool>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, CHAR, "char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_CHAR, "vector<char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT8, "signed char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT8, "vector<signed char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT8, "unsigned char")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT8, "vector<unsigned char>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT16, "short")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT16, "vector<short>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT16, "unsigned short")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT16, "vector<unsigned short>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT32, "int")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT32, "vector<int>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT32, "unsigned int")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT32, "vector<unsigned int>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, INT64, "long long")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_INT64, "vector<long long>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UINT64, "unsigned long long")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_UINT64, "vector<unsigned long long>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, FLOAT, "float")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_FLOAT, "vector<float>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, DOUBLE, "double")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_DOUBLE, "vector<double>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, STRING, "string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_STRING, "vector<string>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, HASH, "Hash")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_HASH, "vector<Hash>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, SCHEMA, "Schema")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, COMPLEX_FLOAT, "complex<float>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_COMPLEX_FLOAT, "vector<complex<float> >")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, COMPLEX_DOUBLE, "complex<double>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_COMPLEX_DOUBLE, "vector<complex<double> >")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_BOOL, "bool*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_CHAR, "char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT8, "signed char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT8, "unsigned char*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT16, "short*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT16, "unsigned short*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT32, "int*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT32, "unsigned int*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_INT64, "long long*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_UINT64, "unsigned long long*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_FLOAT, "float*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_DOUBLE, "double*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_COMPLEX_FLOAT, "complex<float>*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_COMPLEX_DOUBLE, "complex<double>*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, PTR_STRING, "string*")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_BOOL, "pair<bool*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_CHAR, "pair<char*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_INT8, "pair<signed char*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_UINT8, "pair<unsigned char*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_INT16, "pair<short*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_UINT16, "pair<unsigned short*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_INT32, "pair<int*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_UINT32, "pair<unsigned int*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_INT64, "pair<long long*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_UINT64, "pair<unsigned long long*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_FLOAT, "pair<float*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, ARRAY_DOUBLE, "pair<double*,size_t>")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, UNKNOWN, "unknown")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, NONE, "None")
        KARABO_MAP_TO_REFERENCE_TYPE(ToCppString, VECTOR_NONE, "vector<None>")
    }
}

#endif

