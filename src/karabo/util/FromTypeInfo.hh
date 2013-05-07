/* 
 * File:   FromTypeInfo.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 * 
 */

#ifndef KARABO_UTIL_FROMTYPEINFO_HH
#define	KARABO_UTIL_FROMTYPEINFO_HH

#include "FromType.hh"

namespace karabo {

    namespace util {

        class FromTypeInfo {

            typedef std::map<std::string, Types::ReferenceType> TypeInfoMap;

            TypeInfoMap _typeInfoMap;

        public:

            typedef std::type_info ArgumentType;

            static Types::ReferenceType from(const ArgumentType& type) {
                TypeInfoMap::const_iterator it = FromTypeInfo::init()._typeInfoMap.find(std::string(type.name()));
                if (it == FromTypeInfo::init()._typeInfoMap.end()) return Types::UNKNOWN; //throw KARABO_PARAMETER_EXCEPTION("Requested argument type " + std::string(typeid(type).name()) + " not registered");
                return it->second;
            }

        private:

            FromTypeInfo();

            FromTypeInfo(const FromTypeInfo&) {
            };

            virtual ~FromTypeInfo() {
            };

            static FromTypeInfo& init() {
                static FromTypeInfo singleInstance;
                return singleInstance;
            }

        };
    }
}

#endif

