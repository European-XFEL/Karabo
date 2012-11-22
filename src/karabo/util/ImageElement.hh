/*
 * $Id$
 *
 * Author: <kerstin.weger@xfel.eu>
 *
 * Created on May 7, 2012, 2:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_IMAGEELEMENT_HH
#define	KARABO_UTIL_IMAGEELEMENT_HH

#include "ComplexElement.hh"
#include "SimpleElement.hh"
#include "VectorElement.hh"

namespace karabo {
    namespace util {

        template <class T = int >
        class ImageElement {
        private:

            ComplexElement m_outerElement;
            VectorElement<unsigned int> m_dims;
            VectorElement<unsigned char> m_pixelArray;
            SimpleElement<std::string> m_format;

        public:

            ImageElement(Schema& expected) : m_outerElement(ComplexElement(expected)) {
                m_outerElement.readOnly();
                m_outerElement.displayType("Image");

                m_dims.key("dims");
                m_dims.displayedName("Dimensions");
                m_dims.description("Vector space containing image pixels");
                m_dims.assignmentOptional().noDefaultValue();
                m_dims.readOnly();

                m_pixelArray.key("pixelArray");
                m_pixelArray.displayedName("Pixel array");
                m_pixelArray.description("Linear representation (row-wise) of the image pixels");
                m_pixelArray.assignmentOptional().noDefaultValue();
                m_pixelArray.readOnly();

                m_format.key("format");
                m_format.displayedName("Image format");
                m_format.description("String description of image format: <tag>-<bytesPerPixel>-<bitsPerPixel>-<endianess>");
                m_format.assignmentOptional().noDefaultValue();
                m_format.readOnly();
            }

            ImageElement& key(const std::string& name) {
                m_outerElement.key(name);
                return *this;
            }

            ImageElement& alias(const T& name) {
                m_outerElement.alias(name);
                return *this;
            }

            ImageElement& displayedName(const std::string& displayedName) {
                m_outerElement.displayedName(displayedName);
                return *this;
            }

            ImageElement& description(const std::string& desc) {
                m_outerElement.description(desc);
                return *this;
            }

            ImageElement& displayType(const std::string& type) {
                m_outerElement.displayType(type);
                return *this;
            }

            void commit() {
                Schema& innerElement = m_outerElement.commit();
                m_dims.commit(innerElement);
                m_pixelArray.commit(innerElement);
                m_format.commit(innerElement);
            }
        };

        typedef ImageElement<> IMAGE_ELEMENT;
    }
}


#endif	

