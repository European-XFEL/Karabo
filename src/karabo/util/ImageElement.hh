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

        template <class T>
        class ImageElement {
        private:

            ComplexElement m_outerElement;
            SimpleElement<unsigned int> m_dimX, m_dimY, m_dimZ, m_dimC;
            VectorElement<T> m_pixelArray;

        public:

            ImageElement(Schema& expected) : m_outerElement(ComplexElement(expected)) {
                m_outerElement.readOnly();
                m_outerElement.displayType("Image");

                m_dimX.key("dimX");
                m_dimX.displayedName("Dimension X");
                m_dimX.description("Dimensionality along X");
                m_dimX.assignmentOptional().noDefaultValue();
                m_dimX.readOnly();

                m_dimY.key("dimY");
                m_dimY.displayedName("Dimension Y");
                m_dimY.description("Dimensionality along Y");
                m_dimY.assignmentOptional().noDefaultValue();
                m_dimY.readOnly();

                m_dimZ.key("dimZ");
                m_dimZ.displayedName("Dimension Z");
                m_dimZ.description("Dimensionality along Z");
                m_dimZ.assignmentOptional().noDefaultValue();
                m_dimZ.readOnly();

                m_dimC.key("dimC");
                m_dimC.displayedName("Dimension C");
                m_dimC.description("Dimensionality along C");
                m_dimC.assignmentOptional().noDefaultValue();
                m_dimC.readOnly();

                m_pixelArray.key("pixelArray");
                m_pixelArray.displayedName("Pixel array");
                m_pixelArray.description("Linear representation (row-wise) of the image pixels");
                m_pixelArray.assignmentOptional().noDefaultValue();
                m_pixelArray.readOnly();
            }

            ImageElement& key(const std::string& name) {
                m_outerElement.key(name);
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
                m_dimX.commit(innerElement);
                m_dimY.commit(innerElement);
                m_dimZ.commit(innerElement);
                m_dimC.commit(innerElement);
                m_pixelArray.commit(innerElement);
            }
        };

        typedef ImageElement<short > INT16_IMAGE_ELEMENT;
        typedef ImageElement<int > INT32_IMAGE_ELEMENT;
        typedef ImageElement<float > FLOAT_IMAGE_ELEMENT;
        typedef ImageElement<double > DOUBLE_IMAGE_ELEMENT;
    }
}


#endif	

