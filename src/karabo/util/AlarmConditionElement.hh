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
         * The AlarmConditionElement represents a leaf and can be of any (supported) type
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
            AlarmConditionElement& initialValue(const karabo::util::AlarmCondition& a) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, a.toString());
                return *this;
            }


        protected:

            void beforeAddition() {
                /**
                 * Schema is a read-only, leaf property
                 */
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::ALARM_CONDITION);
                this->m_node->template setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::STRING);
                this->m_node->template setAttribute(KARABO_SCHEMA_ACCESS_MODE, READ);


            }

        private:


        };



        typedef AlarmConditionElement ALARM_ELEMENT;


    }
}

#endif	/* KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH */

