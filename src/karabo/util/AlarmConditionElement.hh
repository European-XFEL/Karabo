/*
 * File:   AlarmConditionElement.hh
 * Author: haufs
 *
 * Created on July 25, 2016, 8:18 AM
 */

#ifndef KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH
#define	KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH

#include "AlarmConditions.hh"
#include "Schema.hh"

namespace karabo {
    namespace util {

        /**
         * The AlarmConditionElement represents a leaf and needs to be of type AlarmCondition
         */
        class AlarmConditionElement : public GenericElement<AlarmConditionElement> {

        public:

            AlarmConditionElement(Schema& expected) : GenericElement<AlarmConditionElement>(expected) {

            }

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            AlarmConditionElement& initialValue(const AlarmCondition& a) {
                m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, a.asString());
                return *this;
            }


        protected:

            void beforeAddition() {
                /**
                 * Schema is a read-only, leaf property
                 */
                m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, Schema::ALARM_CONDITION);
                m_node->setAttribute<std::string>(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::EVERY_EVENT);
                m_node->setAttribute<bool>(KARABO_INDICATE_ALARM_SET, true);
            }

        private:


        };



        typedef AlarmConditionElement ALARM_ELEMENT;


    }
}

#endif	/* KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH */

