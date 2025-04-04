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
 * File:   FromType.hh
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on January 22, 2013 9:12 PM
 *
 */

#ifndef KARABO_DATA_TYPES_FROMTYPE_HH
#define KARABO_DATA_TYPES_FROMTYPE_HH

#include "Types.hh"


namespace karabo {

    namespace data {

        /**
         * @class FromType
         * @brief Returns a karabo::data::Types::ReferenceType from an alternate representation
         *        as specified by the template parameter, e.g. FromType<Literal>("INT32") will
         *        return karabo::data::Types::INT32.
         *
         */
        template <class Impl>
        class FromType {
            typedef typename Impl::ArgumentType ArgumentType;

            FromType();
            virtual ~FromType();

           public:
            // Concept that must be implemented

            static Types::ReferenceType from(const ArgumentType& type) {
                return Impl::from(type);
            }
        };
    } // namespace data
} // namespace karabo

#endif
