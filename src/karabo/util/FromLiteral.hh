/* 
 * File:   FromLiteral
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 1:12 PM
 * 
 */

#ifndef KARABO_UTIL_FROMLITERAL_HH
#define	KARABO_UTIL_FROMLITERAL_HH

#include "FromType.hh"

namespace karabo {

    namespace util {

        class FromLiteral {
            typedef std::map<std::string, Types::ReferenceType> TypeInfoMap;

            TypeInfoMap _typeInfoMap;

        public:

            typedef std::string ArgumentType;

            static Types::ReferenceType from(const ArgumentType& type) {
                TypeInfoMap::const_iterator it = FromLiteral::init()._typeInfoMap.find(type);
                if (it == FromLiteral::init()._typeInfoMap.end()) throw KARABO_PARAMETER_EXCEPTION("Requested argument type not registered");
                return it->second;
            }

        private:

            FromLiteral();

            FromLiteral(const FromLiteral&) {
            };

            virtual ~FromLiteral() {
            };

            static FromLiteral& init() {
                static FromLiteral singleInstance;
                return singleInstance;
            }
        };
    }
}

#endif

