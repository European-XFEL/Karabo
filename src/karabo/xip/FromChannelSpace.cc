/* 
 * File:   FromChannelSpace.h
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
 * 
 */

#include <boost/python.hpp>
#include <boost/assign.hpp>
#include "FromChannelSpace.hh"
#include "ImageEnums.hh"

namespace karabo {

    namespace xip {


        FromChannelSpace::FromChannelSpace() {

            #define _KARABO_HELPER_MACRO(fromType, refType) (ChannelSpace::fromType, karabo::util::Types::refType)

            _typeInfoMap = boost::assign::map_list_of
                    _KARABO_HELPER_MACRO(s_8_1, CHAR)
                    _KARABO_HELPER_MACRO(u_8_1, UINT8)
                    _KARABO_HELPER_MACRO(s_16_2, INT16)
                    _KARABO_HELPER_MACRO(u_16_2, UINT16)
                    _KARABO_HELPER_MACRO(s_32_4, INT32)
                    _KARABO_HELPER_MACRO(u_32_4, UINT32)
                    _KARABO_HELPER_MACRO(s_64_8, INT64)
                    _KARABO_HELPER_MACRO(u_64_8, UINT64)
                    _KARABO_HELPER_MACRO(f_32_4, FLOAT)
                    _KARABO_HELPER_MACRO(f_64_8, DOUBLE)
                    ;
            #undef _KARABO_HELPER_MACRO

        }
    }
}
