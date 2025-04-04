/*
 * $Id$
 *
 * File:   GenericElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:12 AM
 * Major re-design on January 30, 2013, 17:22 PM
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

#ifndef KARABO_DATA_SCHEMA_GENERIC_ELEMENT_HH
#define KARABO_DATA_SCHEMA_GENERIC_ELEMENT_HH

#include "OverwriteElement.hh"
#include "karabo/data/types/Schema.hh"

namespace karabo {
    namespace data {

        /**
         * The GenericElement class is a base class for various element types: simple, vector, choice, list
         * and single.
         *
         */
        template <class Derived>
        class GenericElement {
           protected:
            Schema* m_schema;
            std::shared_ptr<Hash::Node> m_node;

           public:
            GenericElement(Schema& expected)
                : m_schema(&expected), m_node(std::shared_ptr<Hash::Node>(new Hash::Node(std::string(), 0))) {}

            virtual ~GenericElement() {}

            /**
             * The <b>key</b> method serves for setting up a unique name for the element.
             * @param name Unique name for the key - can be a nested path if all but its last sub-key are
             *              added as node elements before. Must not be an empty string.
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
            virtual Derived& key(const std::string& name) {
                // Check whether full path (that in fact has to be specified here!) or its last
                // key is empty - empty non-last keys are caught elsewhere.
                // Empty keys or keys with spaces cannot work with instance proxies in Python.
                if (name.empty() || name.back() == Hash::k_defaultSep || name.find(' ') != std::string::npos) {
                    throw KARABO_PARAMETER_EXCEPTION("Bad (sub-)key '" + name + "': empty or with space.");
                }
                m_node->m_key = name;
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>alias</b> method serves for setting up just another name for the element.
             * Note:  this <i>another</i> name may not be necessarily a string. Just any type!
             * @param alias <i>Another</i> name for this element
             * @return reference to the Element (to allow method's chaining)
             */
            template <class AliasType>
            Derived& alias(const AliasType& alias) {
                m_node->setAttribute<AliasType>(KARABO_SCHEMA_ALIAS, alias);
                if (m_node->getKey().empty())
                    throw KARABO_PARAMETER_EXCEPTION(
                          "You have to first assign a key to the expected parameter before you can set any alias");
                m_schema->m_aliasToKey[karabo::data::toString(alias)] = m_node->getKey();
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>tags</b> method allows to tag some expected parameters for later grouping/sorting
             * @param tags a vector of strings
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& tags(const std::vector<std::string>& tags) {
                m_node->setAttribute(KARABO_SCHEMA_TAGS, tags);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>tags/b> method allows to tag some expected parameters for later grouping/sorting
             * @param tags as a string separated by any of the characters in 'sep'
             * @param sep a string interpreted as a list of separator characters
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& tags(const std::string& tags, const std::string& sep = " ,;") {
                return this->tags(fromString<std::string, std::vector>(tags, sep));
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
                m_node->setAttribute(KARABO_SCHEMA_DISPLAYED_NAME, name);
                return *(static_cast<Derived*>(this));
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
                m_node->setAttribute(KARABO_SCHEMA_DESCRIPTION, description);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>observerAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OBSERVER.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& observerAccess() {
                m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>userAccess</b> method serves for setting up the <i>required access level</i> attribute to be USER.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& userAccess() {
                m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::USER);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>operatorAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OPERATOR.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& operatorAccess() {
                m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OPERATOR);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>expertAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * EXPERT.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& expertAccess() {
                m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::EXPERT);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>adminAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * ADMIN.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& adminAccess() {
                m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::ADMIN);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>overWriteRestrictions</b> allows for setting restrictions to overwrite element. Any attributes
             * specified here cannot be altered through use of overwrite element.
             *
             * After execution restrictions contains the new applicable restrictions, e.g. those resulting from merging
             * with previously existing restrictions. This means, one can add restrictions but not cancel existing ones.
             */
            Derived& overwriteRestrictions(OverwriteElement::Restrictions& restrictions) {
                if (m_node->hasAttribute(KARABO_OVERWRITE_RESTRICTIONS)) {
                    OverwriteElement::Restrictions existing;
                    existing.assignFromAttrVector(
                          m_node->getAttribute<std::vector<bool> >(KARABO_OVERWRITE_RESTRICTIONS));
                    // now merge
                    restrictions.merge(existing);
                }
                m_node->setAttribute(KARABO_OVERWRITE_RESTRICTIONS, restrictions.toVectorAttribute());
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>setSpecialDisplayType</b> allows for setting modification of the displayType of the element
             *
             * This attribute is a string and contains a hint to graphical user interfaces that a display
             * mode is possible.
             *
             * @param displayType
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& setSpecialDisplayType(const std::string& displaytype) {
                this->m_node->template setAttribute<std::string>(KARABO_SCHEMA_DISPLAY_TYPE, displaytype);
                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             */
            virtual void commit() {
                beforeAddition();
                if (m_schema) {
                    m_schema->addElement(*m_node);
                } else {
                    throw KARABO_INIT_EXCEPTION("Could not append element to non-initialized Schema object");
                }
            }

            Hash::Node& getNode() {
                return *m_node;
            }

           protected:
            virtual void beforeAddition() {}
        };


    } // namespace data
} // namespace karabo


#endif
