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

            static void byTag(Hash& result, const Schema& schema, const Hash& config, const std::string& tags, const std::string& sep = ",");


        private:

            static void r_byTag(const Hash& master, const Hash::Node& input, const std::string& path, const std::set<std::string>& tags, Hash& result);
            
//            static void r_byTag(const Schema& master, const Hash& input, std::string& path, const std::set<std::string>& tags, Hash& result);
            
                        
        };
    }
}

#endif	

