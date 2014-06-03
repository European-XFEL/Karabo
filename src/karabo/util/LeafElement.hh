/*
 * $Id$
 *
 * File:   LeafElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 5, 2013, 6:14 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_LEAF_ELEMENT_HH
#define	KARABO_UTIL_LEAF_ELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        // Forwards
        template<class T, class U> class DefaultValue;
        template<class T, class U> class ReadOnlySpecific;

        /**
         * The LeafElement represents a leaf and can be of any (supported) type
         */
        template<class Derived, typename ValueType>
        class LeafElement : public GenericElement<Derived> {

            DefaultValue<Derived, ValueType> m_defaultValue; // the default value type depends on the type of element
            ReadOnlySpecific<Derived, ValueType> m_readOnlySpecific;

        public:

            LeafElement(Schema& expected) : GenericElement<Derived>(expected) {
                m_defaultValue.setElement(static_cast<Derived*> (this));
                m_readOnlySpecific.setElement(static_cast<Derived*> (this));
            }

            /**
             * The <b>unit</b> method serves for setting up a name for units
             * @param unitName The name describing units
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& unit(const UnitType& unit) {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_UNIT_ENUM, unit);
                std::pair<std::string, std::string> names = karabo::util::getUnit(unit);
                this->m_node->setAttribute(KARABO_SCHEMA_UNIT_NAME, names.first);
                this->m_node->setAttribute(KARABO_SCHEMA_UNIT_SYMBOL, names.second);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>metricPrefix</b> describes the metric for the unit (e.g. milli, mega, femto, etc.)
             * @param metricPrefix The metric prefix
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& metricPrefix(const MetricPrefixType& metricPrefix) {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_METRIC_PREFIX_ENUM, metricPrefix);
                std::pair<std::string, std::string> names = karabo::util::getMetricPrefix(metricPrefix);
                this->m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
                this->m_node->setAttribute(KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& allowedStates(const std::string& states, const std::string& sep = " ,;") {
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector>(states, sep));
                return *(static_cast<Derived*> (this));
            }
            

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual Derived& assignmentMandatory() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *(static_cast<Derived*> (this));
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
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual Derived& reconfigurable() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ReadOnlySpecific<Derived, ValueType>& readOnly() {
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, "0");
                return m_readOnlySpecific;
            }
        };

        /**
         * The DefaultValue class defines a default value for element.
         */
        template<class Element, class ValueType>
        class DefaultValue {

            Element* m_genericElement;

        public:

            DefaultValue() : m_genericElement(0) {
            }

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
                Hash::Attributes::Node& attrNode = m_genericElement->getNode().getAttributeNode(KARABO_SCHEMA_DEFAULT_VALUE);
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
        template<class Element, class ValueType>
        class ReadOnlySpecific {

            Element* m_genericElement;

        public:

            template< class U, class V> friend class LeafElement;

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
             * The <b>initialValueFromString</b> method enables setting up a default value in a form of a string.
             * This may, for example, be convenient for vector elements.
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            ReadOnlySpecific& initialValueFromString(const std::string& initialValue) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, initialValue);
                return *this;
            }

            ReadOnlySpecific& warnLow(const ValueType& value) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_WARN_LOW, value);
                return *this;
            }

            ReadOnlySpecific& warnHigh(const ValueType& value) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_WARN_HIGH, value);
                return *this;
            }

            ReadOnlySpecific& alarmLow(const ValueType& value) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_ALARM_LOW, value);
                return *this;
            }

            ReadOnlySpecific& alarmHigh(const ValueType& value) {
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_ALARM_HIGH, value);
                return *this;
            }
            
            ReadOnlySpecific& archivePolicy(const Schema::ArchivePolicy& value) {
                m_genericElement->getNode().template setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, value);
                return *this;
            }

            void commit() {
                m_genericElement->commit();
            }

        private:

            // ReadOnlySpecific object can be only constructed by its friends

            ReadOnlySpecific() : m_genericElement(0) {
            }

            void setElement(Element* el) {
                m_genericElement = el;
            }

        };
    }
}

#endif

