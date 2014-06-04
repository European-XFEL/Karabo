/* 
 * File:   FromChannelSpace.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
 * 
 */

#ifndef KARABO_UTIL_FROMCHANNELSPACE_HH
#define	KARABO_UTIL_FROMCHANNELSPACE_HH

#include <map>
#include <karabo/util/Exception.hh>
#include <karabo/util/FromType.hh>

namespace karabo {

    namespace xip {

        class FromChannelSpace {

        public:

            typedef int ArgumentType;

            static karabo::util::Types::ReferenceType from(const ArgumentType& type) {
                TypeInfoMap::const_iterator it = FromChannelSpace::init()._typeInfoMap.find(type);
                if (it == FromChannelSpace::init()._typeInfoMap.end()) throw KARABO_PARAMETER_EXCEPTION("Requested argument type not registered");
                return it->second;
            }

        private:

            FromChannelSpace();

            FromChannelSpace(const FromChannelSpace&) {
            };

            virtual ~FromChannelSpace() {
            };

            static FromChannelSpace& init() {
                static FromChannelSpace singleInstance;
                return singleInstance;
            }


            typedef std::map<ArgumentType, karabo::util::Types::ReferenceType> TypeInfoMap;

            TypeInfoMap _typeInfoMap;
        };

    }
}

#endif

