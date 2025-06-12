/*
 * $Id$
 *
 * File:   LeafElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 5, 2013, 6:14 PM
 *
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


#ifndef KARABO_DATA_SCHEMA_LEAF_ELEMENT_HH
#define KARABO_DATA_SCHEMA_LEAF_ELEMENT_HH

#include "GenericElement.hh"
#include "karabo/data/types/AlarmConditions.hh"
#include "karabo/data/types/State.hh"

namespace karabo {
    namespace data {

        // Forwards
        template <class T, class U>
        class DefaultValue;
        template <class T, class U>
        class ReadOnlySpecific;

        /**
         * The LeafElement represents a leaf and can be of any (supported) type
         */
        template <class Derived, typename ValueType>
        class LeafElement : public GenericElement<Derived> {
            DefaultValue<Derived, ValueType> m_defaultValue; // the default value type depends on the type of element
            ReadOnlySpecific<Derived, ValueType> m_readOnlySpecific;


           public:
            LeafElement(Schema& expected) : GenericElement<Derived>(expected) {
                m_defaultValue.setElement(static_cast<Derived*>(this));
                m_readOnlySpecific.setElement(static_cast<Derived*>(this));
            }

            /**
             * The <b>unit</b> method serves for setting up a name for units
             * @param unitName The name describing units
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& unit(const Unit& unit) {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, static_cast<int>(unit));
                std::pair<std::string, std::string> names = karabo::data::getUnit(unit);
                this->m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                this->m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>metricPrefix</b> describes the metric for the unit (e.g. milli, mega, femto, etc.)
             * @param metricPrefix The metric prefix
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& metricPrefix(const MetricPrefix& metricPrefix) {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM,
                                                         static_cast<int>(metricPrefix));
                std::pair<std::string, std::string> names = karabo::data::getMetricPrefix(metricPrefix);
                this->m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                this->m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& allowedStates(const std::vector<karabo::data::State>& value) {
                const std::string stateString = karabo::data::toString(value);
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES,
                                           karabo::data::fromString<std::string, std::vector>(stateString, ","));
                return *(static_cast<Derived*>(this));
            }

            // overloads for up to six elements

            Derived& allowedStates(const karabo::data::State& s1) {
                const karabo::data::State arr[] = {s1};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 1));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2) {
                const karabo::data::State arr[] = {s1, s2};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 2));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3) {
                const karabo::data::State arr[] = {s1, s2, s3};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 3));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4) {
                const karabo::data::State arr[] = {s1, s2, s3, s4};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 4));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4,
                                   const karabo::data::State& s5) {
                const karabo::data::State arr[] = {s1, s2, s3, s4, s5};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 5));
            }

            Derived& allowedStates(const karabo::data::State& s1, const karabo::data::State& s2,
                                   const karabo::data::State& s3, const karabo::data::State& s4,
                                   const karabo::data::State& s5, const karabo::data::State& s6) {
                const karabo::data::State arr[] = {s1, s2, s3, s4, s5, s6};
                return allowedStates(std::vector<karabo::data::State>(arr, arr + 6));
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual Derived& assignmentMandatory() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>assignmentOptional</b> method serves for setting up a mode that allows the value of
             * element be optional, so it can be omitted in configuration. Default value is injected if defined.
             * If you chain functions for definition of expected parameters the next
             * function may be only defaultValue or noDefaultValue.
             * When the default value is not specified (noDefaultValue) you must always check
             * if the parameter has a value set in delivered User configuration.
             * @return reference to DefaultValue object allowing proper <b>defaultValue</b> method chaining.
             *
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .assignmentOptional().defaultValue("client")
             *         ...
             *         .commit();
             * @endcode
             */
            virtual DefaultValue<Derived, ValueType>& assignmentOptional() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>assignmentInternal</b> method serves for setting up the element to be internal. In the code
             * it behaves like optional parameter but it is not exposed to the user. It is omitted when the schema
             * is serialized to XSD. The value of this parameter should be defined programmatically. Conceptually,
             * internal parameter with internal flag can be treated as an argument to the constructor.
             * @return reference to DefaultValue (to allow method's chaining)
             */
            virtual DefaultValue<Derived, ValueType>& assignmentInternal() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual Derived& init() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual Derived& reconfigurable() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ReadOnlySpecific<Derived, ValueType>& readOnly() {
                if (this->m_node->hasAttribute(KARABO_SCHEMA_ASSIGNMENT)) {
                    const Schema::AssignmentType assignment =
                          Schema::AssignmentType(this->m_node->template getAttribute<int>(KARABO_SCHEMA_ASSIGNMENT));
                    if (assignment == Schema::MANDATORY_PARAM) {
                        std::string msg("Error in element '");
                        msg.append(this->m_node->getKey())
                              .append("': readOnly() is not compatible with assignmentMandatory()");
                        throw KARABO_LOGIC_EXCEPTION(msg);
                    } else if (assignment == Schema::OPTIONAL_PARAM &&
                               this->m_node->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE)) {
                        std::string msg("Error in element '");
                        msg.append(this->m_node->getKey())
                              .append("': readOnly() is not compatible with assignmentOptional().defaultValue(v). ")
                              .append("Use readOnly().defaultValue(v) instead.");
                        throw KARABO_LOGIC_EXCEPTION(msg);
                    }
                }
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a
                // read-only
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, ValueType());
                return m_readOnlySpecific;
            }
        };

        /**
         * The DefaultValue class defines a default value for element.
         */
        template <class Element, class ValueType>
        class DefaultValue {
            Element* m_genericElement;

           public:
            DefaultValue() : m_genericElement(0) {}

            /**
             * Set the element this DefaultValue refers to
             * @param el
             */
            void setElement(Element* el) {
                m_genericElement = el;
            }

            /**
             * The <b>defaultValue</b> method serves for setting up the default value to be used when User
             * configuration does not specify another value.
             * @param val  Default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValue(const ValueType& defaultValue) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, defaultValue);
                return *m_genericElement;
            }

            /**
             * The <b>defaultValueFromString</b> method enables setting up a default value in a form of a string.
             * This may, for example, be convenient for vector elements.
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValueFromString(const std::string& defaultValue) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, defaultValue);
                Types::ReferenceType type = Types::from<FromTypeInfo>(typeid(ValueType));
                Hash::Attributes::Node& attrNode =
                      m_genericElement->getNode().getAttributeNode(KARABO_SCHEMA_DEFAULT_VALUE);
                attrNode.setType(type);
                return *m_genericElement;
            }

            /**
             * The <b>noDefaultValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noDefaultValue() {
                return *m_genericElement;
            }
        };

        /**
         * The ReadOnlySpecific class defines specific values for 'readOnly'-element.
         */
        template <class Element, class ValueType>
        class ReadOnlySpecific {
            typedef ReadOnlySpecific<Element, ValueType> Self;
            Element* m_genericElement;

           public:
            template <class U, class V>
            friend class LeafElement;
            friend class TableElement;

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            ReadOnlySpecific& initialValue(const ValueType& initialValue) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, initialValue);
                return *this;
            }

            /**
             * The <b>defaultValue</b> method is the same as <b>initialValue</b>
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            ReadOnlySpecific& defaultValue(const ValueType& initialValue) {
                return this->initialValue(initialValue);
            }

            /**
             * The <b>initialValueFromString</b> method enables setting up a default value in a form of a string.
             * DEPRECATED! For vectors use list initialisation: initialValue({1, 2, 3})
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            ReadOnlySpecific& initialValueFromString(const std::string& initialValue) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, initialValue);
                return *this;
            }

            /**
             * Set the archiving policy for this element. Available settings
             * are:
             *
             *   EVERY_EVENT,
             *   NO_ARCHIVING
             *
             * @param value
             * @return
             */
            ReadOnlySpecific& archivePolicy(const Schema::ArchivePolicy& value) {
                m_genericElement->getNode().template setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, value);
                return *this;
            }

            /**
             * Registers this element into the Schema
             */
            void commit() {
                m_genericElement->commit();
            }

            /**
             * The <b>observerAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OBSERVER.
             * @return reference to the Element (to allow method's chaining)
             */
            ReadOnlySpecific& observerAccess() {
                m_genericElement->observerAccess();
                return *this;
            }

            /**
             * The <b>operatorAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OPERATOR.
             * @return reference to the Element (to allow method's chaining)
             */
            ReadOnlySpecific& operatorAccess() {
                m_genericElement->operatorAccess();
                return *this;
            }

            /**
             * The <b>expertAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * EXPERT.
             * @return reference to the Element (to allow method's chaining)
             */
            ReadOnlySpecific& expertAccess() {
                m_genericElement->expertAccess();
                return *this;
            }

           private:
            // ReadOnlySpecific object can be only constructed by its friends

            ReadOnlySpecific() : m_genericElement(0) {}

            void setElement(Element* el) {
                m_genericElement = el;
            }

            Element* getElement() {
                return m_genericElement;
            }
        };
    } // namespace data
} // namespace karabo

#endif
