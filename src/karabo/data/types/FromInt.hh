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
 * File:   FromInt
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 11, 2013 7:00 PM
 *
 */

#ifndef KARABO_DATA_TYPES_FROMINT_HH
#define KARABO_DATA_TYPES_FROMINT_HH

#include <map>

#include "Exception.hh"
#include "FromType.hh"

namespace karabo {

    namespace data {

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
    } // namespace data
} // namespace karabo

#endif
