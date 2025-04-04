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
 * File:   FromTypeInfo.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 *
 */

#ifndef KARABO_DATA_TYPES_FROMTYPEINFO_HH
#define KARABO_DATA_TYPES_FROMTYPEINFO_HH

#include <map>

#include "FromType.hh"

namespace karabo {

    namespace data {

        class FromTypeInfo {
            typedef std::map<std::string, Types::ReferenceType> TypeInfoMap;

            TypeInfoMap _typeInfoMap;

           public:
            typedef std::type_info ArgumentType;

            static Types::ReferenceType from(const ArgumentType& type) {
                TypeInfoMap::const_iterator it = FromTypeInfo::init()._typeInfoMap.find(std::string(type.name()));
                if (it == FromTypeInfo::init()._typeInfoMap.end())
                    return Types::UNKNOWN; // throw KARABO_PARAMETER_EXCEPTION("Requested argument type " +
                                           // std::string(typeid(type).name()) + " not registered");
                return it->second;
            }

           private:
            FromTypeInfo();

            FromTypeInfo(const FromTypeInfo&){};

            virtual ~FromTypeInfo(){};

            static FromTypeInfo& init() {
                static FromTypeInfo singleInstance;
                return singleInstance;
            }
        };
    } // namespace data
} // namespace karabo

#endif
