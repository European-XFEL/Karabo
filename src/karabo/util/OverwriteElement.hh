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
            Hash::Node m_node;

        public:

            OverwriteElement(Schema& expected) : m_schema(&expected) {
            }

            /**
             * Specify the key to be overwritten
             * @param name unique key name
             * @return  reference to the Element
             */
            OverwriteElement& key(std::string const& name) {
                m_node = Hash::Node(name, 0);
                m_node.setAttribute("nodeType", "leaf");
                return *this;
            }

            template <class AliasType>
            OverwriteElement& setNewAlias(const AliasType& alias) {
                m_node.setAttribute<AliasType > ("alias", alias);
                return *this;
            }

            template <class TagType>
            OverwriteElement& setNewTag(const TagType& tag) {
                m_node.setAttribute<TagType > ("tag", tag);
                return *this;
            }

            OverwriteElement& setNewAssignmentMandatory() {
                m_node.setAttribute<int>("assignment", Schema::MANDATORY_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentOptional() {
                m_node.setAttribute<int>("assignment", Schema::OPTIONAL_PARAM);
                return *this;
            }

            OverwriteElement& setNewAssignmentInternal() {
                m_node.setAttribute<int>("assignment", Schema::INTERNAL_PARAM);
                return *this;
            }
            
            OverwriteElement& setNowInit() {
                m_node.setAttribute<int>("accessMode", INIT);
                return *this;
            }
            
            OverwriteElement& setNowReconfigurable() {
                m_node.setAttribute<int>("accessMode", WRITE);
                return *this;
            }
            
            OverwriteElement& setNowReadOnly() {
                m_node.setAttribute<int>("accessMode", READ);
                return *this;
            }

            template <class ValueType>
            OverwriteElement& setNewDefaultValue(const ValueType& value) {
                m_node.setAttribute("default", value);
                return *this;
            }
            
            /**
             * The <b>commit</b> method injects the element to the expected parameters list. If not called
             * the element is not usable. This must be called after the element is fully defined.
             * @return reference to the GenericElement
             */
            void commit() {
                if (m_schema) {
                    m_schema->addElement(m_node);
                } else {
                    throw KARABO_INIT_EXCEPTION("Could not append element to non-initialized Schema object");
                }
            }
        };
    }
    
    typedef util::OverwriteElement OVERWRITE_ELEMENT;
    
}
#endif	
