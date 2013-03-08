/* 
 * File:   FromType.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 * 
 */

#include <boost/assign.hpp>

#include "Hash.hh"

#include "FromTypeInfo.hh"

namespace karabo {
    namespace util {

        #define _KARABO_HELPER_MACRO1(ReferenceType, CppType) \
(std::string(typeid (CppType).name()), Types::ReferenceType) \
(std::string(typeid (std::vector<CppType>).name()), Types::VECTOR_##ReferenceType)


        #define _KARABO_HELPER_MACRO(ReferenceType, CppType) \
(std::string(typeid (CppType).name()), Types::ReferenceType) \
(std::string(typeid (std::vector<CppType>).name()), Types::VECTOR_##ReferenceType)\
(std::string(typeid (CppType*).name()), Types::PTR_##ReferenceType)


        FromTypeInfo::FromTypeInfo() {
            _typeInfoMap = boost::assign::map_list_of
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
                    _KARABO_HELPER_MACRO1(HASH, Hash)

                    ;
        }
        #undef _KARABO_HELPER_MACRO
        #undef _KARABO_HELPER_MACRO1
    }
}