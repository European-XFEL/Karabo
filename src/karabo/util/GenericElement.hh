/*
 * $Id$
 *
 * File:   GenericElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:12 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_GENERICELEMENT_HH
#define	EXFEL_UTIL_GENERICELEMENT_HH

#include "Schema.hh"

namespace exfel {
    namespace util {

        // forward declaration
        template<class Element, class T> class DefaultValue;

        /**
         * The GenericElement class is a base class for various element types: simple, vector, choice, list
         * and single.
         *
         */
        template<class Element, class TDefaultValue>
        class GenericElement {
        protected:

            Schema m_element; // Schema element
            Element* m_elementPointer; // pointer to concrete element type
            Schema* m_expected; // our Schema element will be added to this parameters list

            DefaultValue<Element, TDefaultValue> m_defValue; // the default value type depends on the type of element

            // this has to be called by constructor of the derived class

            void initializeElementPointer(Element *el) {
                m_elementPointer = el;
                m_defValue.setElement(el);
            }

        public:

            GenericElement(Schema& expected) : m_elementPointer(0), m_expected(&expected) {
            }

            GenericElement() : m_elementPointer(0), m_expected(0) {
            }

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
            Element& key(std::string const& name) {
                m_element.key(name);
                return *m_elementPointer;
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
            Element& displayedName(std::string const& name) {
                m_element.displayedName(name);
                return *m_elementPointer;
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
            Element& description(std::string const& desc) {
                m_element.description(desc);
                return *m_elementPointer;
            }

            /**
             * The <b>advanced</b> method serves for setting up the <i>expert level</i> attribute to be advanced.
             * @return reference to the Element (to allow method's chaining)
             */
            Element& advanced() {
                m_element.expertLevel(Schema::ADVANCED);
                return *m_elementPointer;
            }

            /**
             * The <b>unitName</b> method serves for setting up a name for units
             * @param unitName The name describing units
             * @return reference to the Element (to allow method's chaining)
             */
            Element& unitName(const std::string& unitName) {
                m_element.unitName(unitName);
                return *m_elementPointer;
            }

            /**
             * The <b>unitSymbol</b> method serves for setting up a symbol for units
             * @param unitSymbol The symbol denoting units
             * @return reference to the Element (to allow method's chaining)
             */
            Element& unitSymbol(const std::string& unitSymbol) {
                m_element.unitSymbol(unitSymbol);
                return *m_elementPointer;
            }
            
            /**
             * The <b>alias</b> method serves for setting up just another name for the element.
             * Note:  this <i>another</i> name may not be necessarily a string. Just any type!
             * @param alias <i>Another</i> name for this element
             * @return reference to the Element (to allow method's chaining)
             */
            template <class T>
            Element& alias(const T& alias) {
                m_element.alias(alias);
                return *m_elementPointer;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            Element& reconfigurable() {
                m_element.access(WRITE);
                return *m_elementPointer;
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            Element& readOnly() {
                m_element.access(READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
                m_element.assignment(Schema::OPTIONAL_PARAM);
                m_element.defaultValue("0");
                return *m_elementPointer;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            Element& init() {
                m_element.access(INIT);
                return *m_elementPointer;
            }

            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            Element& allowedStates(const std::string& states, const std::string& sep = " ,;") {
                m_element.allowedStates(states);
                return *m_elementPointer;
            }

            Element& displayType(const std::string& type) {
                m_element.displayType(type);
                return *m_elementPointer;
            }

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            Element& assignmentMandatory() {
                m_element.assignment(Schema::MANDATORY_PARAM);
                return *m_elementPointer;
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
            DefaultValue<Element, TDefaultValue>& assignmentOptional() {
                m_element.assignment(Schema::OPTIONAL_PARAM);
                return m_defValue;
            }

            /**
             * The <b>assignmentInternal</b> method serves for setting up the element to be internal. In the code
             * it behaves like optional parameter but it is not exposed to the user. It is omitted when the schema
             * is serialized to XSD. The value of this parameter should be defined programmatically. Conceptually,
             * internal parameter with internal flag can be treated as an argument to the constructor.
             * @return reference to DefaultValue (to allow method's chaining)
             */
            DefaultValue<Element, TDefaultValue>& assignmentInternal() {
                m_element.assignment(Schema::INTERNAL_PARAM);
                return m_defValue;
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            GenericElement<Element, TDefaultValue>& commit() {
                build();
                checkConsistency();
                if (m_expected) {
                    m_element.setAccessMode(m_expected->getAccessMode());
                    m_expected->addElement(m_element);
                } else {
                    throw LOGIC_EXCEPTION("No expected parameter given to which this element should be applied to (hint: use different constructor)");
                }
                return *this;
            }

            GenericElement<Element, TDefaultValue>& commit(Schema& expected) {
                m_expected = &expected;
                build();
                checkConsistency();
                m_element.setAccessMode(m_expected->getAccessMode());
                m_expected->addElement(m_element);
                return *this;
            }

            Schema* getConfigElement() {
                return &m_element;

            }

        protected:

            virtual void build() {
            }

            virtual void checkConsistency() {
            }
        };

        /**
         * The DefaultValue class defines a default value for element.
         */
        template<class Element, class TDefaultValue>
        class DefaultValue {
            Element* m_genericElement;

        public:

            template< class TElement, class TT> friend class GenericElement;

            /**
             * The <b>defaultValue</b> method serves for setting up the default value to be used when User
             * configuration does not specify another value.
             * @param val  Default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValue(const TDefaultValue& defaultValue) {
                m_genericElement->getConfigElement()->defaultValue(defaultValue);
                return (*m_genericElement);
            }

            /**
             * The <b>defaultValueFromString</b> method enables setting up a default value in a form of a string.
             * This may, for example, be convenient for vector elements.
             * @param defaultValue A string representation of the default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValueFromString(const std::string& defaultValue) {
                m_genericElement->getConfigElement()->defaultValue(defaultValue);
                return (*m_genericElement);
            }

            /**
             * The <b>noDefaultValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noDefaultValue() {
                return (*m_genericElement);
            }

        private:

            // DefaultValue object can be only constructed by its friends

            DefaultValue() {
                m_genericElement = NULL;
            }

            void setElement(Element* el) {
                m_genericElement = el;
            }

        };
    }
}



#endif	

