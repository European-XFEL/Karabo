/* 
 * File:   ToXsd.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 27, 2013, 1:20 PM
 */

#ifndef KARABO_UTIL_TOXSD_HH
#define	KARABO_UTIL_TOXSD_HH

#include "ToType.hh"

#include <karabo/util/karaboDll.hh>

namespace karabo {
    namespace util {

        class ToXsd {

            public:

            typedef std::string ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, BOOL, "xs:boolean")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_BOOL, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, CHAR, "xs:byte")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_CHAR, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, INT8, "xs:byte")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_INT8, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, UINT8, "xs:unsignedByte")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_UINT8, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, INT16, "xs:short")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_INT16, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, UINT16, "xs:unsignedShort")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_UINT16, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, INT32, "xs:int")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_INT32, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, UINT32, "xs:unsignedInt")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_UINT32, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, INT64, "xs:long")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_INT64, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, UINT64, "xs::unsignedLong")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_UINT64, "xs::string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, FLOAT, "xs:float")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_FLOAT, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, DOUBLE, "xs:double")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_DOUBLE, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, STRING, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_STRING, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, COMPLEX_FLOAT, "xs:float")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_COMPLEX_FLOAT, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, COMPLEX_DOUBLE, "xs:double")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_COMPLEX_DOUBLE, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, NONE, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, VECTOR_NONE, "xs:string")
        KARABO_MAP_TO_REFERENCE_TYPE(ToXsd, UNKNOWN, "undefined")
    }
}
#endif
