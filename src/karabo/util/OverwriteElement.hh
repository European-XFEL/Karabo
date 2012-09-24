/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 23, 2011, 11:12 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_UTIL_OVERWRITEELEMENT_HH
#define	EXFEL_UTIL_OVERWRITEELEMENT_HH

#include "Schema.hh"

namespace exfel {
    namespace util {

        class OverwriteElement {
        protected:

            Schema m_element; // Schema element
            Schema* m_expected; // our Schema element will be added to this parameters list

        public:

            OverwriteElement() : m_expected(0) {
            }

            OverwriteElement(Schema& expected) : m_expected(&expected) {
            }

            /**
             * Set the unique key for the parameter
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name) {
                m_element.key(name);
                m_element.displayedName(name);
                m_element.access(INIT|READ|WRITE);
                m_element.assignment(Schema::OPTIONAL_PARAM);
                m_element.simpleType(Types::BOOL);
                return *this;
            }

            /**
             * The <b>alias</b> method serves for setting up just another name for the element.
             * Note:  this <i>another</i> name may not be necessarily a string. Just any type!
             * @param alias <i>Another</i> name for this element
             * @return reference to the Element (to allow method's chaining)
             */
            template <class T>
            OverwriteElement& setNewAlias(const T& previousAlias) {
                m_element.overwriteAlias(previousAlias);
                return *this;
            }

            template <class T>
            OverwriteElement& setNewDefault(const T& defaultValue) {
                m_element.overwriteDefault(defaultValue);
                return *this;
            }

            /**
             * This function injects the parameter to the expected parameters list. If not called
             * the parameter is not usable. The function must be called after the parameter is fully defined.
             * @return reference to the OverwriteElement
             */
            OverwriteElement& commit() {
                if (m_expected) {
                    m_expected->addElement(m_element);
                } else {
                    throw LOGIC_EXCEPTION("No expected parameter given to which this element should be applied to (hint: use different constructor)");
                }
                return *this;
            }

            /**
             * This function injects the parameter to the expected parameters list. If not called
             * the parameter is not usable. The function must be called after the parameter is fully defined.
             * @param expected Reference to an expected parameter object
             * @return reference to the OverwriteElement
             */
            OverwriteElement& commit(Schema& expected) {
                m_expected = &expected;
                m_expected->addElement(m_element);
                return *this;
            }

        };

        typedef OverwriteElement OVERWRITE_ELEMENT;

    }
}
#endif	
