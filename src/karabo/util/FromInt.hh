/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   FromInt
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 11, 2013 7:00 PM
 *
 */

#ifndef KARABO_UTIL_FROMINT_HH
#define KARABO_UTIL_FROMINT_HH

#include <map>

#include "Exception.hh"
#include "FromType.hh"

namespace karabo {

    namespace util {

        class FromInt {
            typedef std::map<int, Types::ReferenceType> TypeInfoMap;

            TypeInfoMap _typeInfoMap;

           public:
            typedef int ArgumentType;

            static Types::ReferenceType from(const ArgumentType& type) {
                TypeInfoMap::const_iterator it = FromInt::init()._typeInfoMap.find(type);
                if (it == FromInt::init()._typeInfoMap.end())
                    throw KARABO_PARAMETER_EXCEPTION("Requested argument type not registered");
                return it->second;
            }

           private:
            FromInt();

            FromInt(const FromInt&){};

            virtual ~FromInt(){};

            static FromInt& init() {
                static FromInt singleInstance;
                return singleInstance;
            }
        };
    } // namespace util
} // namespace karabo

#endif
