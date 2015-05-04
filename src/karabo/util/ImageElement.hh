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

#include "Schema.hh"


#include "GenericElement.hh"
#include "VectorElement.hh"
#include "SimpleElement.hh"

namespace karabo {
    namespace util {
        
        class ImageElement : public GenericElement<ImageElement> {

        protected:

            karabo::util::Hash m_child;

        public:

            ImageElement(Schema& expected) : GenericElement<ImageElement>(expected) {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, karabo::util::READ);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, karabo::util::Schema::NODE);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Image"); // Reserved displayType for commands
                
                //default value of requiredAccessLevel for Image element: OBSERVER
                this->observerAccess();
                
                Schema inner;
                VECTOR_CHAR_ELEMENT(inner).key("data")
                        .description("Pixel array")                        
                        .readOnly()                        
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                VECTOR_UINT32_ELEMENT(inner).key("dims")
                        .displayedName("Dimensions")
                        .description("The length of the array reflects total dimensionality and each element the extension in this dimension")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                VECTOR_UINT32_ELEMENT(inner).key("roiOffsets")
                        .displayedName("ROI Offsets")
                        .description("Describes the offset of the Region-of-Interest; it will contain zeros if the image has no ROI defined")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                INT32_ELEMENT(inner).key("encoding")
                        .displayedName("Encoding")
                        .description("Describes the color space of pixel encoding of the data (e.g. GRAY, RGB, JPG, PNG etc.")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                INT32_ELEMENT(inner).key("channelSpace")
                        .displayedName("Channel space")
                        .description("Describes the channel encoding, i.e. signed/unsigned/floating point, bits per channel and bytes per pixel")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                STRING_ELEMENT(inner).key("dataType")
                        .displayedName("Type")
                        .description("Describes the underlying data type")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                BOOL_ELEMENT(inner).key("isBigEndian")
                        .displayedName("Is big endian")
                        .description("Flags whether the raw data are in big or little endian")
                        .readOnly()
                        .archivePolicy(Schema::NO_ARCHIVING)
                        .commit();
                
                m_child = inner.getParameterHash();
            }

        protected:

            void beforeAddition() {
                this->m_node->setValue(this->m_child);
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ARCHIVE_POLICY)) {
                    this->m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::NO_ARCHIVING);
                }
            }
        };

        typedef ImageElement IMAGE_ELEMENT;
    }
}

#endif	

