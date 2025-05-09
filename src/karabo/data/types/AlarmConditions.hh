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
 * File:   AlarmConditions.hh
 * Author: haufs
 *
 * Created on June 9, 2016, 9:13 AM
 */

#ifndef KARABO_DATA_TYPES_ALARMCONDITIONS_HH
#define KARABO_DATA_TYPES_ALARMCONDITIONS_HH

#include <memory>
#include <mutex> // for once_flag
#include <string>
#include <unordered_map>
#include <vector>

#define KARABO_ALARM_NONE "none"
#define KARABO_WARN "warn"
#define KARABO_ALARM "alarm"
#define KARABO_INTERLOCK "interlock"

#define KARABO_INDICATE_ALARM_SET "indicateAlarm"

namespace karabo {
    namespace data {

        /**
         * @class AlarmCondition
         * @brief A unified alarm condition class, which holds the alarm conditions known to Karabo
         */
        class AlarmCondition {
           public:
            static const AlarmCondition NONE;
            static const AlarmCondition WARN;
            static const AlarmCondition ALARM;
            static const AlarmCondition INTERLOCK;


            /**
             * Returns the most significant alarm condition out of a list of conditions
             * @param v: the list of alarm conditions
             * @return the most significant condition in the list (it will return the parent condition where applicable)
             *         e.g. WARN_HIGH -> WARN
             */
            static AlarmCondition returnMostSignificant(const std::vector<AlarmCondition>& v);

            /**
             * Returns an alarm condition object matching to the stringified condition
             * @param condition: a known alarm condition
             * @return a reference to an  alarm condition object
             */
            static const AlarmCondition& fromString(const std::string& condition);

            /**
             * Returns a stringified version of the alarm condition
             * @return
             */
            const std::string& asString() const;


            /**
             * Returns a stringified version of the alarm condition or its base if applicable
             * @return
             */
            const std::string& asBaseString() const;

            /**
             * Allows for direct assignment of conditions to strings
             * @return
             */
            operator std::string() const;

            /**
             * Tests whether an alarm condition is more critical than this alarm condition
             * @param other: the condition to test criticality against
             * @return true if this condition has a higher criticality than the other; false otherwise.
             */
            bool isMoreCriticalThan(const AlarmCondition& other) const;

            /**
             * Tests whether two alarm conditions are similar, e.g. are subsets of the same basic condition
             * @param test: the condition to test similarity against
             * @return true if the conditions are subsets of the same base; false otherwise.
             */
            bool isSameCriticality(const AlarmCondition& test) const;

            /**
             * Returns the more significant of the two condtions
             * @param other
             * @return
             */
            const AlarmCondition& returnMoreSignificant(const AlarmCondition& other) const;

            bool operator==(const AlarmCondition& other) const;

            bool operator!=(const AlarmCondition& other) const {
                return !(*this == other);
            }

            AlarmCondition(const AlarmCondition& b) = default;

            AlarmCondition& operator=(const AlarmCondition& other) = default;

           private:
            // constructors are all private. Users should not need
            // to construct alarm conditions, but use the pre-constructed ones.

            AlarmCondition() = delete;

            AlarmCondition(const std::string& cs, unsigned int r);

            AlarmCondition(const std::string& cs, const AlarmCondition& b);

            std::shared_ptr<const AlarmCondition> getBase() const;

            static void initFromString();

            std::string m_conditionString;
            unsigned int m_rank;
            std::shared_ptr<const AlarmCondition> m_base;

            static std::once_flag m_initFromStringFlag;
            static std::unordered_map<std::string, const AlarmCondition&> m_alarmFactory;
        };


    } // namespace data

} // namespace karabo

#define KARABO_ALARM_ATTR "alarmCondition"

#endif /* ALARMCONDITIONS_HH */
