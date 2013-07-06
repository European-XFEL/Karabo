/*
 * $Id$
 *
 * File:   ImageElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 6, 2013, 12:59 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_IMAGEELEMENT_HH
#define	KARABO_UTIL_IMAGEELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {
        
        class ImageElement : public GenericElement<ImageElement> {

        public:

            ImageElement(Schema& expected) : GenericElement<ImageElement>(expected) {
            }
            
        protected:

            void beforeAddition() {

                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, "VECTOR_CHAR");
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, READ);
                // Set the assignment and defaults here, as the API would look strange to assign something to a read-only
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, "0");
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "image");

            }
        };

        typedef ImageElement IMAGE_ELEMENT;
    }
}

#endif	

