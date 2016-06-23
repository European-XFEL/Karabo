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
#include "AlarmConditions.hh"

#include "karaboDll.hh"
#include "Timestamp.hh"

#include "StatisticalEvaluators.hh"
#include <map>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>

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
            
            mutable boost::shared_mutex m_rollingStatMutex;
            std::map<std::string, RollingWindowStatistics::Pointer> m_parameterRollingStats;
            
        public:
            
            enum ProblemType {
                WARN_LOW,
                WARN_HIGH,
                ALARM_LOW,
                ALARM_HIGH
            };

            struct ValidationRules {

                ValidationRules()
                :
                injectDefaults(true), allowUnrootedConfiguration(true),
                allowAdditionalKeys(true), allowMissingKeys(true),
                injectTimestamps(true) { }

                ValidationRules(bool injectDefaults_, bool allowUnrootedConfiguration_,
                                bool allowAdditionalKeys_, bool allowMissingKeys_, bool injectTimestamps_)
                :
                injectDefaults(injectDefaults_), allowUnrootedConfiguration(allowUnrootedConfiguration_),
                allowAdditionalKeys(allowAdditionalKeys_), allowMissingKeys(allowMissingKeys_),
                injectTimestamps(injectTimestamps_) { }

                bool injectDefaults;
                bool allowUnrootedConfiguration;
                bool allowAdditionalKeys;
                bool allowMissingKeys;
                bool injectTimestamps;
            };

            Validator();
            
            Validator(const Validator & other);

            Validator(const ValidationRules rules);

            void setValidationRules(const Validator::ValidationRules& rules);

            Validator::ValidationRules getValidationRules() const;

            std::pair<bool, std::string> validate(const Schema& schema, const Hash& unvalidatedInput, Hash& validatedOutput, const Timestamp& timestamp = Timestamp());
            
            bool hasParametersInWarnOrAlarm() const;
            
            const karabo::util::Hash& getParametersInWarnOrAlarm() const;
            
            bool hasReconfigurableParameter() const;
            
            RollingWindowStatistics::ConstPointer getRollingStatistics(const std::string & scope) const;

        private:

            void r_validate(const Hash& master, const Hash& user, Hash& working, std::ostringstream& report, std::string scope = "");

            void validateLeaf(const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, std::string scope);
            
            void attachTimestampIfNotAlreadyThere(Hash::Node& node);
            
            void assureRollingStatsInitialized(const std::string & scope, const unsigned int & evalInterval);
            
            bool checkThresholdedAlarmCondition(const karabo::util::AlarmCondition& alarmCond,  const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, std::string scope, bool checkGreater);
            
            bool checkThresholdedAlarmCondition(const karabo::util::AlarmCondition& alarmCond, double value, const Hash::Node& masterNode, Hash::Node& workNode, std::ostringstream& report, std::string scope, bool checkGreater);
            
            

        };
    }
}

#endif	/* VALIDATOR_HH */

