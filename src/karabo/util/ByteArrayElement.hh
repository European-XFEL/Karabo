/*
 * File:   ByteArrayElement.hh
 *
 * Created on September 2, 2016
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_BYTEARRAYELEMENT_HH
#define KARABO_UTIL_BYTEARRAYELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        class ByteArrayElement : public LeafElement<ByteArrayElement, ByteArray > {

        public:

            ByteArrayElement(Schema& expected) : LeafElement<ByteArrayElement, ByteArray >(expected) {
            }

        protected:

            void beforeAddition() {

                m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "ByteArray");
                m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<ByteArray >()));

                if (!m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) init(); // This is the default

                if (!m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

                    //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                    if (!m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) || //init element
                        m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT || //init element
                        m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) { //reconfigurable element

                        userAccess();

                    } else { //else set default value of requiredAccessLevel to OBSERVER
                        observerAccess();
                    }
                }
            }
        };

        typedef ByteArrayElement BYTEARRAY_ELEMENT;
    }
}

#endif
