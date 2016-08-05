/*
 * File:   AlarmConditions.hh
 * Author: haufs
 *
 * Created on June 9, 2016, 9:13 AM
 */

#ifndef KARABO_UTIL_ALARMCONDITIONS_HH
#define	KARABO_UTIL_ALARMCONDITIONS_HH

#include <vector>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#define KARABO_ALARM_NONE "none"
#define KARABO_WARN "warn"
#define KARABO_WARN_LOW "warnLow"
#define KARABO_WARN_HIGH "warnHigh"

#define KARABO_ALARM "alarm"
#define KARABO_ALARM_LOW "alarmLow"
#define KARABO_ALARM_HIGH "alarmHigh"


#define KARABO_WARN_VARIANCE_LOW "warnVarianceLow"
#define KARABO_WARN_VARIANCE_HIGH "warnVarianceHigh"

#define KARABO_ALARM_VARIANCE_LOW "alarmVarianceLow"
#define KARABO_ALARM_VARIANCE_HIGH "alarmVarianceHigh"

#define KARABO_INTERLOCK "interlock"

#define KARABO_ALARM_ACK "alarmNeedsAck"
#define KARABO_ALARM_INFO "alarmInfo"

#define KARABO_INDICATE_ALARM_SET "indicateAlarm"

namespace karabo {
    namespace util {

        class AlarmCondition {

            /**
             A unified alarm condition class, which holds the alarm conditions known to Karabo
             */

        public:

            static const AlarmCondition NONE;
            static const AlarmCondition WARN;
            static const AlarmCondition WARN_LOW;
            static const AlarmCondition WARN_HIGH;
            static const AlarmCondition WARN_VARIANCE_LOW;
            static const AlarmCondition WARN_VARIANCE_HIGH;
            static const AlarmCondition ALARM;
            static const AlarmCondition ALARM_LOW;
            static const AlarmCondition ALARM_HIGH;
            static const AlarmCondition ALARM_VARIANCE_LOW;
            static const AlarmCondition ALARM_VARIANCE_HIGH;
            static const AlarmCondition INTERLOCK;


            /**
             * Returns the most significant alarm condition out of a list of conditions
             * @param v: the list of alarm conditions
             * @return the most significant condition in the list (it will return the parent condition where applicable)
             *         e.g. WARN_HIGH -> WARN
             */
            static AlarmCondition returnMostSignificant(const std::vector<AlarmCondition> & v);

            /**
             * Returns an alarm condition object matching to the stringified condition
             * @param condition: a known alarm condition
             * @return a reference to an  alarm condition object
             */
            static const AlarmCondition & fromString(const std::string & condition);

            /**
             * Returns a stringified version of the alarm condition
             * @return
             */
            const std::string & asString() const;


            /**
             * Returns a stringified version of the alarm condition or its base if applicable
             * @return
             */
            const std::string & asBaseString() const;

            /**
             * Allows for direct assignment of conditions to strings
             * @return
             */
            operator std::string() const;



            /**
             * Tests whether two alarm conditions are similar, e.g. are subsets of the same basic condition
             * @param test: the condition to test similarity against
             * @return true if the conditions are subsets of the same base; false otherwise.
             */
            bool isSameCriticality(const AlarmCondition & test) const;

            /**
             * Returns the more significant of the two condtions
             * @param other
             * @return
             */
            const AlarmCondition & returnMoreSignificant(const AlarmCondition & other) const;



        private:

            //constructors are all private. Users should not need
            //to construct alarm conditions, but use the pre-constructed ones.

            AlarmCondition();

            AlarmCondition(std::string cs, unsigned int r);

            AlarmCondition(std::string cs, const AlarmCondition & b);

            boost::shared_ptr<const AlarmCondition> getBase() const;


            std::string m_conditionString;
            unsigned int m_rank;
            boost::shared_ptr<const AlarmCondition> m_base;
            static std::map<std::string, const AlarmCondition & > m_alarmFactory;


        };




    }

}

#define KARABO_ALARM_ATTR "alarmCondition"

#endif	/* ALARMCONDITIONS_HH */

