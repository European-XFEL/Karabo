/* 
 * File:   ToChannelSpace.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on May 26, 2014, 11:04 AM
 */

#ifndef KARABO_XIP_TOCHANNELSPACE_HH
#define	KARABO_XIP_TOCHANNELSPACE_HH

#include <karabo/util/ToType.hh>

namespace karabo {

    namespace xip {

        class ToChannelSpace {

        public:

            typedef ChannelSpaceType ReturnType;

            template <int RefType>
            static ReturnType to() {
                throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to required type not implemented");
            }
        };

        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, BOOL, ChannelSpace::u_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, CHAR, ChannelSpace::s_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT8, ChannelSpace::s_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT8, ChannelSpace::u_8_1)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT16, ChannelSpace::s_16_2)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT16, ChannelSpace::u_16_2)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT32, ChannelSpace::s_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT32, ChannelSpace::u_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, INT64, ChannelSpace::s_64_8)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, UINT64, ChannelSpace::u_64_8)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, FLOAT, ChannelSpace::f_32_4)
        KARABO_MAP_TO_REFERENCE_TYPE(ToChannelSpace, DOUBLE, ChannelSpace::f_64_8)

    }
}

#endif
