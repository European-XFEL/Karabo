/* 
 * File:   FromInt.h
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 1:12 PM
 * 
 */

#include <boost/assign.hpp>

#include "Hash.hh"

#include "FromInt.hh"

namespace karabo {

    namespace util {

        FromInt::FromInt() {

#if __cplusplus < 201103L

#define _KARABO_HELPER_MACRO(type) (Types::type, Types::type)    

            _typeInfoMap = boost::assign::map_list_of
                    _KARABO_HELPER_MACRO(BOOL)
                    _KARABO_HELPER_MACRO(VECTOR_BOOL)
                    _KARABO_HELPER_MACRO(CHAR)
                    _KARABO_HELPER_MACRO(VECTOR_CHAR)
                    _KARABO_HELPER_MACRO(INT8)
                    _KARABO_HELPER_MACRO(VECTOR_INT8)
                    _KARABO_HELPER_MACRO(UINT8)
                    _KARABO_HELPER_MACRO(VECTOR_UINT8)
                    _KARABO_HELPER_MACRO(INT16)
                    _KARABO_HELPER_MACRO(VECTOR_INT16)
                    _KARABO_HELPER_MACRO(UINT16)
                    _KARABO_HELPER_MACRO(VECTOR_UINT16)
                    _KARABO_HELPER_MACRO(INT32)
                    _KARABO_HELPER_MACRO(VECTOR_INT32)
                    _KARABO_HELPER_MACRO(UINT32)
                    _KARABO_HELPER_MACRO(VECTOR_UINT32)
                    _KARABO_HELPER_MACRO(INT64)
                    _KARABO_HELPER_MACRO(VECTOR_INT64)
                    _KARABO_HELPER_MACRO(UINT64)
                    _KARABO_HELPER_MACRO(VECTOR_UINT64)
                    _KARABO_HELPER_MACRO(FLOAT)
                    _KARABO_HELPER_MACRO(VECTOR_FLOAT)
                    _KARABO_HELPER_MACRO(DOUBLE)
                    _KARABO_HELPER_MACRO(VECTOR_DOUBLE)
                    _KARABO_HELPER_MACRO(STRING)
                    _KARABO_HELPER_MACRO(VECTOR_STRING)
                    _KARABO_HELPER_MACRO(HASH)
                    _KARABO_HELPER_MACRO(VECTOR_HASH)
                    _KARABO_HELPER_MACRO(SCHEMA)
                    _KARABO_HELPER_MACRO(COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(VECTOR_COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(VECTOR_COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(PTR_BOOL)
                    _KARABO_HELPER_MACRO(PTR_CHAR)
                    _KARABO_HELPER_MACRO(PTR_INT8)
                    _KARABO_HELPER_MACRO(PTR_UINT8)
                    _KARABO_HELPER_MACRO(PTR_INT16)
                    _KARABO_HELPER_MACRO(PTR_UINT16)
                    _KARABO_HELPER_MACRO(PTR_INT32)
                    _KARABO_HELPER_MACRO(PTR_UINT32)
                    _KARABO_HELPER_MACRO(PTR_INT64)
                    _KARABO_HELPER_MACRO(PTR_UINT64)
                    _KARABO_HELPER_MACRO(PTR_FLOAT)
                    _KARABO_HELPER_MACRO(PTR_DOUBLE)
                    _KARABO_HELPER_MACRO(PTR_COMPLEX_FLOAT)
                    _KARABO_HELPER_MACRO(PTR_COMPLEX_DOUBLE)
                    _KARABO_HELPER_MACRO(PTR_STRING)
                    _KARABO_HELPER_MACRO(ARRAY_BOOL)
                    _KARABO_HELPER_MACRO(ARRAY_CHAR)
                    _KARABO_HELPER_MACRO(ARRAY_INT8)
                    _KARABO_HELPER_MACRO(ARRAY_UINT8)
                    _KARABO_HELPER_MACRO(ARRAY_INT16)
                    _KARABO_HELPER_MACRO(ARRAY_UINT16)
                    _KARABO_HELPER_MACRO(ARRAY_INT32)
                    _KARABO_HELPER_MACRO(ARRAY_UINT32)
                    _KARABO_HELPER_MACRO(ARRAY_INT64)
                    _KARABO_HELPER_MACRO(ARRAY_UINT64)
                    _KARABO_HELPER_MACRO(ARRAY_FLOAT)
                    _KARABO_HELPER_MACRO(ARRAY_DOUBLE)
                    _KARABO_HELPER_MACRO(NONE)
                    _KARABO_HELPER_MACRO(VECTOR_NONE)

                    ;
#undef _KARABO_HELPER_MACRO
#else
#define _KARABO_HELPER_MACRO(type) {Types::type, Types::type}

            _typeInfoMap = {
                _KARABO_HELPER_MACRO(BOOL)
                , _KARABO_HELPER_MACRO(VECTOR_BOOL)
                , _KARABO_HELPER_MACRO(CHAR)
                , _KARABO_HELPER_MACRO(VECTOR_CHAR)
                , _KARABO_HELPER_MACRO(INT8)
                , _KARABO_HELPER_MACRO(VECTOR_INT8)
                , _KARABO_HELPER_MACRO(UINT8)
                , _KARABO_HELPER_MACRO(VECTOR_UINT8)
                , _KARABO_HELPER_MACRO(INT16)
                , _KARABO_HELPER_MACRO(VECTOR_INT16)
                , _KARABO_HELPER_MACRO(UINT16)
                , _KARABO_HELPER_MACRO(VECTOR_UINT16)
                , _KARABO_HELPER_MACRO(INT32)
                , _KARABO_HELPER_MACRO(VECTOR_INT32)
                , _KARABO_HELPER_MACRO(UINT32)
                , _KARABO_HELPER_MACRO(VECTOR_UINT32)
                , _KARABO_HELPER_MACRO(INT64)
                , _KARABO_HELPER_MACRO(VECTOR_INT64)
                , _KARABO_HELPER_MACRO(UINT64)
                , _KARABO_HELPER_MACRO(VECTOR_UINT64)
                , _KARABO_HELPER_MACRO(FLOAT)
                , _KARABO_HELPER_MACRO(VECTOR_FLOAT)
                , _KARABO_HELPER_MACRO(DOUBLE)
                , _KARABO_HELPER_MACRO(VECTOR_DOUBLE)
                , _KARABO_HELPER_MACRO(STRING)
                , _KARABO_HELPER_MACRO(VECTOR_STRING)
                , _KARABO_HELPER_MACRO(HASH)
                , _KARABO_HELPER_MACRO(VECTOR_HASH)
                , _KARABO_HELPER_MACRO(SCHEMA)
                , _KARABO_HELPER_MACRO(COMPLEX_FLOAT)
                , _KARABO_HELPER_MACRO(VECTOR_COMPLEX_FLOAT)
                , _KARABO_HELPER_MACRO(COMPLEX_DOUBLE)
                , _KARABO_HELPER_MACRO(VECTOR_COMPLEX_DOUBLE)
                , _KARABO_HELPER_MACRO(PTR_BOOL)
                , _KARABO_HELPER_MACRO(PTR_CHAR)
                , _KARABO_HELPER_MACRO(PTR_INT8)
                , _KARABO_HELPER_MACRO(PTR_UINT8)
                , _KARABO_HELPER_MACRO(PTR_INT16)
                , _KARABO_HELPER_MACRO(PTR_UINT16)
                , _KARABO_HELPER_MACRO(PTR_INT32)
                , _KARABO_HELPER_MACRO(PTR_UINT32)
                , _KARABO_HELPER_MACRO(PTR_INT64)
                , _KARABO_HELPER_MACRO(PTR_UINT64)
                , _KARABO_HELPER_MACRO(PTR_FLOAT)
                , _KARABO_HELPER_MACRO(PTR_DOUBLE)
                , _KARABO_HELPER_MACRO(PTR_COMPLEX_FLOAT)
                , _KARABO_HELPER_MACRO(PTR_COMPLEX_DOUBLE)
                , _KARABO_HELPER_MACRO(PTR_STRING)
                , _KARABO_HELPER_MACRO(ARRAY_BOOL)
                , _KARABO_HELPER_MACRO(ARRAY_CHAR)
                , _KARABO_HELPER_MACRO(ARRAY_INT8)
                , _KARABO_HELPER_MACRO(ARRAY_UINT8)
                , _KARABO_HELPER_MACRO(ARRAY_INT16)
                , _KARABO_HELPER_MACRO(ARRAY_UINT16)
                , _KARABO_HELPER_MACRO(ARRAY_INT32)
                , _KARABO_HELPER_MACRO(ARRAY_UINT32)
                , _KARABO_HELPER_MACRO(ARRAY_INT64)
                , _KARABO_HELPER_MACRO(ARRAY_UINT64)
                , _KARABO_HELPER_MACRO(ARRAY_FLOAT)
                , _KARABO_HELPER_MACRO(ARRAY_DOUBLE)
                , _KARABO_HELPER_MACRO(NONE)
                , _KARABO_HELPER_MACRO(VECTOR_NONE)
            };
#endif            
        }

    }
}
