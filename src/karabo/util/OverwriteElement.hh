/*
 * $Id$
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on September 23, 2011, 11:12 AM
 *
 * Major re-design on February 1, 2013, 1:00 PM
 * 
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_OVERWRITEELEMENT_HH
#define	KARABO_UTIL_OVERWRITEELEMENT_HH

#include "Schema.hh"

namespace karabo {
    namespace util {

        class OverwriteElement {
            Schema* m_schema;
            Hash::Node* m_node;

        public:

            OverwriteElement(Schema& expected) : m_schema(&expected) {
            }

            /**
             * Specify the key to be overwritten
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name) {                
                boost::optional<Hash::Node&> node = m_schema->getParameterHash().find(name);
                if (node) { // exists
                    m_node = node.get_ptr();
                } else {
                    throw KARABO_PARAMETER_EXCEPTION("Key \"" + name + "\" was not set before, thus can not be overwritten.");
                }
                return *this;
            }

            template <class AliasType>
            OverwriteElement& setNewAlias(const AliasType& alias) {
                m_node->setAttribute<AliasType > (KARABO_SCHEMA_ALIAS, alias);
                return *this;
            }

            template <class TagType>
            OverwriteElement& setNewTag(const TagType& tag) {
                m_node->setAttribute<TagType > ("tag", tag);
                return *this;
            }

            OverwriteElement& setNewAssignmentMandatory() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentOptional() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentInternal() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return *this;
            }
            
            OverwriteElement& setNowInit() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }
            
            OverwriteElement& setNowReconfigurable() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }
            
            OverwriteElement& setNowReadOnly() {
                m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, value);
                return *this;
            }
            
            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            void commit() {
                // Does nothing, changes happened on existing node
                }
        };
        typedef OverwriteElement OVERWRITE_ELEMENT;
    }
}
#endif	
