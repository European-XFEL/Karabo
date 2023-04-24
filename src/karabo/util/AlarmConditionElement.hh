/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
/*
 * File:   AlarmConditionElement.hh
 * Author: haufs
 *
 * Created on July 25, 2016, 8:18 AM
 */

#ifndef KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH
#define KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        class AlarmCondition;
        class Schema;

        /**
         * The AlarmConditionElement represents a leaf and needs to be of type AlarmCondition
         */
        class AlarmConditionElement : public GenericElement<AlarmConditionElement> {
           public:
            AlarmConditionElement(Schema& expected);

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            AlarmConditionElement& initialValue(const AlarmCondition& a);

           protected:
            void beforeAddition();
        };


        typedef AlarmConditionElement ALARM_ELEMENT;


    } // namespace util
} // namespace karabo

#endif /* KARABO_SCHEMA_ALARMCONDITIONELEMENT_HH */
