/* 
 * File:   HashFilter.hh
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Created on April 12, 2013, 11:50 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_HASHFILTER_HH
#define	KARABO_UTIL_HASHFILTER_HH

#include <boost/foreach.hpp>
#include "Schema.hh"
#include "Hash.hh"
#include <string>


#include "karaboDll.hh"

namespace karabo {
    namespace util {

        class HashFilter {

            public:
            KARABO_CLASSINFO(HashFilter, "HashFilter", "1.0");

            HashFilter();

            static void byTag(const Schema& schema, const Hash& config, Hash& result, const std::string& tags, const std::string& sep = ",");

            static void byAccessMode(const Schema& schema, const Hash& config, Hash& result, const AccessType& value);


        private:

            static void r_byTag(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path, const std::set<std::string>& tags);
            static bool processNode(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path, const std::set<std::string>& tags);

            static void r_byAccessMode(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path, const AccessType& value);
            static bool processNodeForAccessMode(const Hash& master, const Hash::Node& input, Hash& result, const std::string& path, const AccessType& value);

        };
    }
}

#endif	

