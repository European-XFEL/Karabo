/*
 * File:   HashFilter.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on April 12, 2013, 11:50 AM
 *
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


#ifndef KARABO_DATA_TYPES_HASHFILTER_HH
#define KARABO_DATA_TYPES_HASHFILTER_HH

#include <boost/foreach.hpp>
#include <string>

#include "Hash.hh"
#include "Schema.hh"
#include "karaboDll.hh"

namespace karabo {
    namespace data {

        /**
         * @class HashFilter
         * @brief This class provides methods to filter a configuration Hash by properties of the Schema describing it
         */
        class HashFilter {
           public:
            KARABO_CLASSINFO(HashFilter, "HashFilter", "1.0");

            HashFilter();

            /**
             * Filter a configuration Hash by the tags defined in the Schema describing it and return the filtered
             * result
             * @param schema describing the <i>config</i>uration Hash
             * @param config input Hash to be filtered
             * @param result filtered output Hash
             * @param tags stringified list of tags. Elements in the schema having any of the tags in this list are
             * included in the output Hash
             * @param sep separator used in the list of tags
             */
            static void byTag(const Schema& schema, const Hash& config, Hash& result, const std::string& tags,
                              const std::string& sep = ",");

            /**
             * Filter a configuration Hash by the access mode defined in the Schema describing it and return the
             * filtered result
             * @param schema describing the <i>config</i>uration Hash
             * @param config input Hash to be filtered
             * @param result filtered output Hash
             * @param value AccessMode to filter against
             */
            static void byAccessMode(const Schema& schema, const Hash& config, Hash& result, const AccessType& value);


           private:
            static void r_byTag(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path,
                                const std::set<std::string>& tags);
            static bool processNode(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path,
                                    const std::set<std::string>& tags);

            static void r_byAccessMode(const Hash& master, const Hash::Node& input, Hash& result,
                                       const std::string& path, const AccessType& value);
            static bool processNodeForAccessMode(const Hash& master, const Hash::Node& input, Hash& result,
                                                 const std::string& path, const AccessType& value);
        };
    } // namespace data
} // namespace karabo

#endif
