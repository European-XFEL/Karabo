/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   FromNumpy.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on May 23, 2014 6:34 PM
 *
 */

#ifndef KARABO_UTIL_FROMNUMPY_HH
#define KARABO_UTIL_FROMNUMPY_HH

#include <karabo/util/Exception.hh>
#include <karabo/util/FromType.hh>
#include <map>

namespace karathon {

    class FromNumpy {
       public:
        typedef int ArgumentType;

        static karabo::util::Types::ReferenceType from(const ArgumentType& type) {
            TypeInfoMap::const_iterator it = FromNumpy::init()._typeInfoMap.find(type);
            if (it == FromNumpy::init()._typeInfoMap.end())
                throw KARABO_PARAMETER_EXCEPTION("Requested argument type not registered");
            return it->second;
        }

       private:
        FromNumpy();

        FromNumpy(const FromNumpy&){};

        virtual ~FromNumpy(){};

        static FromNumpy& init() {
            static FromNumpy singleInstance;
            return singleInstance;
        }


        typedef std::map<ArgumentType, karabo::util::Types::ReferenceType> TypeInfoMap;

        TypeInfoMap _typeInfoMap;
    };

} // namespace karathon

#endif
