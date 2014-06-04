/* 
 * File:   FromNumpy.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
 * 
 */

#ifndef KARABO_UTIL_FROMNUMPY_HH
#define	KARABO_UTIL_FROMNUMPY_HH

#include <map>
#include <karabo/util/Exception.hh>
#include <karabo/util/FromType.hh>

namespace karathon {

    class FromNumpy {

    public:

        typedef int ArgumentType;

        static karabo::util::Types::ReferenceType from(const ArgumentType& type) {
            TypeInfoMap::const_iterator it = FromNumpy::init()._typeInfoMap.find(type);
            if (it == FromNumpy::init()._typeInfoMap.end()) throw KARABO_PARAMETER_EXCEPTION("Requested argument type not registered");
            return it->second;
        }

    private:

        FromNumpy();

        FromNumpy(const FromNumpy&) {
        };

        virtual ~FromNumpy() {
        };

        static FromNumpy& init() {
            static FromNumpy singleInstance;
            return singleInstance;
        }


        typedef std::map<ArgumentType, karabo::util::Types::ReferenceType> TypeInfoMap;

        TypeInfoMap _typeInfoMap;
    };

}

#endif

