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
#include "AlarmConditions.hh"

#include "Exception.hh"


namespace karabo {
    namespace data {


        AlarmCondition::AlarmCondition(const std::string& cs, unsigned int r)
            : m_conditionString(cs),
              m_rank(r){

              };


        AlarmCondition::AlarmCondition(const std::string& cs, const AlarmCondition& b)
            : m_conditionString(cs),
              m_rank(b.m_rank),
              m_base(std::make_shared<AlarmCondition>(b)){

              };


        std::shared_ptr<const AlarmCondition> AlarmCondition::getBase() const {
            return m_base;
        }


        const AlarmCondition& AlarmCondition::returnMoreSignificant(const AlarmCondition& other) const {
            if (other.m_rank > this->m_rank) {
                return other;
            } else {
                return *this;
            }
        }


        const std::string& AlarmCondition::asString() const {
            return m_conditionString;
        }


        const std::string& AlarmCondition::asBaseString() const {
            return (m_base.get() ? m_base->asString() : this->asString());
        }


        AlarmCondition::operator std::string() const {
            return this->m_conditionString;
        }


        bool AlarmCondition::isMoreCriticalThan(const AlarmCondition& other) const {
            return m_rank > other.m_rank;
        }


        bool AlarmCondition::isSameCriticality(const AlarmCondition& test) const {
            return test.m_rank == this->m_rank;
        }


        AlarmCondition AlarmCondition::returnMostSignificant(const std::vector<AlarmCondition>& v) {
            if (v.empty()) return NONE;
            const AlarmCondition* s = &(v[0]);
            for (std::vector<AlarmCondition>::const_iterator i = v.begin(); i != v.end(); ++i) {
                s = &(i->returnMoreSignificant(*s));
                if (s->isSameCriticality(INTERLOCK)) break; // can't go higher than this
            }

            return (s->getBase() ? *(s->getBase()) : *s);
        }


        bool AlarmCondition::isValid(const std::string& condition) {
            std::call_once(m_initFromStringFlag, &AlarmCondition::initFromString);
            return (m_alarmFactory.end() != m_alarmFactory.find(condition));
        }

        const AlarmCondition& AlarmCondition::fromString(const std::string& condition) {
            std::call_once(m_initFromStringFlag, &AlarmCondition::initFromString);

            std::unordered_map<std::string, const AlarmCondition&>::const_iterator iter =
                  m_alarmFactory.find(condition);
            if (iter == m_alarmFactory.end()) {
                throw KARABO_LOGIC_EXCEPTION("Alarm condition  " + condition + " does not exist!");
            } else {
                return iter->second;
            }
        }


        void AlarmCondition::initFromString() {
#define KRB_ALARM_INSERT(alarmType)                                                                           \
    m_alarmFactory.insert(std::pair<std::string, const AlarmCondition&>(AlarmCondition::alarmType.asString(), \
                                                                        AlarmCondition::alarmType));

            KRB_ALARM_INSERT(NONE)
            KRB_ALARM_INSERT(WARN)
            KRB_ALARM_INSERT(ALARM)
            KRB_ALARM_INSERT(INTERLOCK)

#undef KRB_ALARM_INSERT
        }

        bool AlarmCondition::operator==(const AlarmCondition& other) const {
            // criticality check might be redundant, but it should be fast and might return false without
            // going through string comparison
            return isSameCriticality(other) && asString() == other.asString();
        }


        const AlarmCondition AlarmCondition::NONE(KARABO_ALARM_NONE, 0);
        const AlarmCondition AlarmCondition::WARN(KARABO_WARN, 1);
        const AlarmCondition AlarmCondition::ALARM(KARABO_ALARM, 2);
        const AlarmCondition AlarmCondition::INTERLOCK(KARABO_INTERLOCK, 3);
        // interlock is assumed to always be the highest conditions and knowledge of this is used in
        // returnMostSignificant

        std::once_flag AlarmCondition::m_initFromStringFlag;
        std::unordered_map<std::string, const AlarmCondition&> AlarmCondition::m_alarmFactory;

    } // namespace data
} // namespace karabo
