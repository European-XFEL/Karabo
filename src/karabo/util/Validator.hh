/* 
 * File:   Validator.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 8, 2013, 6:03 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_VALIDATOR_HH
#define	KARABO_UTIL_VALIDATOR_HH

#include <boost/foreach.hpp>
#include "Schema.hh"
#include "StringTools.hh"
#include "ToLiteral.hh"
#include "Units.hh"

#include "karaboDll.hh"

namespace karabo {
    namespace util {

        class Validator {
            // Validation flags
            bool m_injectDefaults;
            bool m_assumeRootedConfiguration;
            bool m_allowAdditionalKeys;
            bool m_allowMissingKeys;

        public:

            struct ValidationRules {
                bool injectDefaults;
                bool allowUnrootedConfiguration;
                bool allowAdditionalKeys;
                bool allowMissingKeys;
            };

            Validator();
            
            Validator(const ValidationRules rules);
            
            void setValidationRules(const Validator::ValidationRules& rules);
                
            Validator::ValidationRules getValidationRules() const;
            
            std::pair<bool, std::string> validate(const Schema& schema, const Hash& unvalidatedInput, Hash& validatedOutput) const;
                    
        private:
            
            void r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
            void processLeaf(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
            void processNode(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
            void processChoice(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
            void processList(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
        };
    }
}

#endif	/* VALIDATOR_HH */

