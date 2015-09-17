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
#include "Timestamp.hh"

namespace karabo {
    namespace util {

        class Validator {

            // Validation flags
            bool m_injectDefaults;
            bool m_allowUnrootedConfiguration;
            bool m_allowAdditionalKeys;
            bool m_allowMissingKeys;
            bool m_injectTimestamps;
            
            karabo::util::Hash m_parametersInWarnOrAlarm;
            karabo::util::Timestamp m_timestamp;
            bool m_hasReconfigurableParameter;
            
        public:
            
            enum ProblemType {
                WARN_LOW,
                WARN_HIGH,
                ALARM_LOW,
                ALARM_HIGH
            };

            struct ValidationRules {

                bool injectDefaults;
                bool allowUnrootedConfiguration;
                bool allowAdditionalKeys;
                bool allowMissingKeys;
                bool injectTimestamps;
            };

            Validator();

            Validator(const ValidationRules rules);

            void setValidationRules(const Validator::ValidationRules& rules);

            Validator::ValidationRules getValidationRules() const;

            std::pair<bool, std::string> validate(const Schema& schema, const Hash& unvalidatedInput, Hash& validatedOutput, const Timestamp& timestamp = Timestamp());
            
            bool hasParametersInWarnOrAlarm() const;
            
            const karabo::util::Hash& getParametersInWarnOrAlarm() const;
            
            bool hasReconfigurableParameter() const;

        private:

            void r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "");

            void validateLeaf(const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, std::string scope);
            
            void attachTimestampIfNotAlreadyThere(Hash::Node& node);

        };
    }
}

#endif	/* VALIDATOR_HH */

