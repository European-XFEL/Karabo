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
 * File:   AlarmConditionElement.hh
 * Author: haufs
 *
 * Created on July 25, 2016, 8:18 AM
 */

#ifndef KARABO_DATA_SCHEMA_ALARMCONDITIONELEMENT_HH
#define KARABO_DATA_SCHEMA_ALARMCONDITIONELEMENT_HH

#include "BaseElement.hh"

namespace karabo {
    namespace data {

        class AlarmCondition;
        class Schema;

        /**
         * The AlarmConditionElement represents a leaf and needs to be of type AlarmCondition
         */
        class AlarmConditionElement : public BaseElement<AlarmConditionElement> {
           public:
            AlarmConditionElement(Schema& expected);

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            AlarmConditionElement& initialValue(const AlarmCondition& a);

            AlarmConditionElement& defaultValue(const AlarmCondition& a);

           protected:
            void beforeAddition();
        };


        typedef AlarmConditionElement ALARM_ELEMENT;


    } // namespace data
} // namespace karabo

#endif /* KARABO_DATA_SCHEMA_ALARMCONDITIONELEMENT_HH */
