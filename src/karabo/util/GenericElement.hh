/*
 * $Id$
 *
 * File:   GenericElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:12 AM
 * Major re-design on January 30, 2013, 17:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_GENERIC_ELEMENT_HH
#define	KARABO_UTIL_GENERIC_ELEMENT_HH

#include "Schema.hh"

namespace karabo {
    namespace util {

        // Forwards
        template<class T, class U> class DefaultValue;
        template<class T, class U> class ReadOnlySpecific; 

        /**
         * The GenericElement class is a base class for various element types: simple, vector, choice, list
         * and single.
         *
         */
        template<class Derived, class ValueType>
        class GenericElement {
        protected:

            Schema* m_schema;
            Hash::Node m_node;

            DefaultValue<Derived, ValueType> m_defaultValue; // the default value type depends on the type of element
            ReadOnlySpecific<Derived, ValueType> m_readOnlySpecific;

        public:

            GenericElement(Schema& expected) : m_schema(&expected) {
                m_defaultValue.setElement(static_cast<Derived*> (this));
            }

            //            GenericElement() : m_schema(0) {
            //            }

            virtual ~GenericElement() {
            }

            /**
             * The <b>key</b> method serves for setting up a unique name for the element.
             * @param name Unique name for the key
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         .key("type")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& key(const std::string& name) {
                m_node = Hash::Node(name, ValueType());
                
                // Set some defaults here
                this->init(); 
                
                return *(static_cast<Derived*> (this));
            }
            
             /**
             * The <b>alias</b> method serves for setting up just another name for the element.
             * Note:  this <i>another</i> name may not be necessarily a string. Just any type!
             * @param alias <i>Another</i> name for this element
             * @return reference to the Element (to allow method's chaining)
             */
            template <class AliasType>
            Derived& alias(const AliasType& alias) {
                m_node.setAttribute<AliasType>("alias", alias);
                return *(static_cast<Derived*> (this));
            }
            
            /**
             * The <b>tag</b> method allows to tag some expected parameters for later grouping/sorting
             * @param tag of any type
             * @return reference to the Element (to allow method's chaining)
             */
            template <class TagType>
            Derived& tag(const TagType& tag) {
                m_node.setAttribute<TagType>("tag", tag);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>displayedName</b> method serves for setting up an user friendly name for the element
             * to be used by GUI
             * @param name User friendly name for the element
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .displayedName("Connection Type")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& displayedName(const std::string& name) {
                m_node.setAttribute("displayedName", name);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>description</b> method serves for setting up a description of the element
             * @param desc Short description of the element
             * @return reference to the Element (to allow method's chaining)
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& description(const std::string& description) {
                m_node.setAttribute("description", description);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>advanced</b> method serves for setting up the <i>expert level</i> attribute to be advanced.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& advanced() {
                m_node.setAttribute<int>("expertLevel", Schema::ADVANCED);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>unit</b> method serves for setting up a name for units
             * @param unitName The name describing units
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& unit(const Units::SiUnit& unit) {
                m_node.setAttribute<int>("unitEnum", unit);
                // TODO unitName, unitSymbol here
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>metricPrefix</b> describes the metric for the unit (e.g. milli, mega, femto, etc.)
             * @param metricPrefix The metric prefix
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& metricPrefix(const Units::MetricPrefix metricPrefix) {
                m_node.setAttribute<int>("metricPrefixEnum", metricPrefix);
                return *(static_cast<Derived*> (this));
            }

           

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& reconfigurable() {
                m_node.setAttribute<int>("accessMode", WRITE);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            DefaultValue<Derived, ValueType>& readOnly() {
                m_node.setAttribute<int>("accessMode", READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
                m_node.setAttribute<int>("assignment", Schema::OPTIONAL_PARAM);
                m_node.setAttribute("default", "0");
                return m_defaultValue;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& init() {
                m_node.setAttribute<int>("accessMode", INIT);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& allowedStates(const std::string& states, const std::string& sep = " ,;") {
                m_node.setAttribute("allowedStates", karabo::util::fromString<std::string, std::vector>(states, sep));
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>allowedRoles</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& allowedRoles(const std::string& roles, const std::string& sep = " ,;") {
                m_node.setAttribute("allowedRoles", karabo::util::fromString<std::string, std::vector>(roles, sep) );
                return *(static_cast<Derived*> (this));
            }

            Derived& displayType(const std::string& type) {
                m_node.setAttribute("displayType", type);
                return *(static_cast<Derived*> (this));
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& assignmentMandatory() {
                m_node.setAttribute<int>("assignment", Schema::MANDATORY_PARAM);
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
            DefaultValue<Derived, ValueType>& assignmentOptional() {
                m_node.setAttribute<int>("assignment", Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>assignmentInternal</b> method serves for setting up the element to be internal. In the code
             * it behaves like optional parameter but it is not exposed to the user. It is omitted when the schema
             * is serialized to XSD. The value of this parameter should be defined programmatically. Conceptually,
             * internal parameter with internal flag can be treated as an argument to the constructor.
             * @return reference to DefaultValue (to allow method's chaining)
             */
            DefaultValue<Derived, ValueType>& assignmentInternal() {
                m_node.setAttribute<int>("assignment", Schema::INTERNAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            GenericElement<Derived, ValueType>& commit() {
                beforeAddition();
                if (m_schema) {
                    m_schema->addElement(m_node);
                } else {
                    throw KARABO_INIT_EXCEPTION("Could not append element to non-initialized Schema object");
                }
                return *this;
            }

            Hash::Node& getElement() {
                return m_node;

            }

        protected:

            virtual void beforeAddition() {
            }
        };

        /**
         * The DefaultValue class defines a default value for element.
         */
        template<class Element, class ValueType>
        class DefaultValue {
            Element* m_genericElement;

        public:

            template< class U, class V> friend class GenericElement;

            /**
             * The <b>defaultValue</b> method serves for setting up the default value to be used when User
             * configuration does not specify another value.
             * @param val  Default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValue(const ValueType& defaultValue) {
                m_genericElement->getElement().setAttribute("default", defaultValue);
                return *m_genericElement;
            }

            /**
             * The <b>defaultValueFromString</b> method enables setting up a default value in a form of a string.
             * This may, for example, be convenient for vector elements.
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValueFromString(const std::string& defaultValue) {
                m_genericElement->getElement().setAttribute("default", defaultValue);
                return *m_genericElement;
            }

            /**
             * The <b>noDefaultValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noDefaultValue() {
                return *m_genericElement;
            }

        private:

            // DefaultValue object can be only constructed by its friends

            DefaultValue() : m_genericElement(0) {
            }

            void setElement(Element* el) {
                m_genericElement = el;
            }

        };
        
        /**
         * The DefaultValue class defines a default value for element.
         */
        template<class Element, class ValueType>
        class ReadOnlySpecific {
            Element* m_genericElement;

        public:

            template< class U, class V> friend class GenericElement;

            /**
             * The <b>initialValue</b> method serves for setting up the initial value reported for this parameter.
             * @param val  Initial value
             * @return reference to the Element for proper methods chaining
             */
            Element& initialValue(const ValueType& initialValue) {
                m_genericElement->getElement().setAttribute("default", initialValue);
                return *m_genericElement;
            }

            /**
             * The <b>initialValueFromString</b> method enables setting up a default value in a form of a string.
             * This may, for example, be convenient for vector elements.
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            Element& initialValueFromString(const std::string& initialValue) {
                m_genericElement->getElement().setAttribute("default", initialValue);
                return *m_genericElement;
            }

            /**
             * The <b>noInitialValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noInitialValue() {
                return *m_genericElement;
            }
            
            Element& warnLow(const ValueType& value) {
                m_genericElement->getElement().setAttribute("warnLow", value);
                return *m_genericElement;
            }
            
            Element& warnHigh(const ValueType& value) {
                m_genericElement->getElement().setAttribute("warnHigh", value);
                return *m_genericElement;
            }
            
            Element& alarmLow(const ValueType& value) {
                m_genericElement->getElement().setAttribute("alarmLow", value);
                return *m_genericElement;
            }
            
            Element& alarmHigh(const ValueType& value) {
                m_genericElement->getElement().setAttribute("alarmHigh", value);
                return *m_genericElement;
            }
            
            
            

        private:

            // DefaultValue object can be only constructed by its friends

            ReadOnlySpecific() : m_genericElement(0) {
            }

            void setElement(Element* el) {
                m_genericElement = el;
            }

        };
    }
}



#endif	

