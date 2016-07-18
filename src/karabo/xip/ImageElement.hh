/*
 * $Id$
 *
 * Author: <kerstin.weger@xfel.eu>
 *
 * Created on May 7, 2012, 2:22 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_XIP_IMAGEELEMENT_HH
#define	EXFEL_XIP_IMAGEELEMENT_HH

#include <exfel/util/ComplexElement.hh>
#include <exfel/util/VectorElement.hh>
#include <exfel/util/SimpleElement.hh>

namespace exfel {
    namespace xip {

        template <class T>
        class ImageElement {

            private:

            exfel::util::ComplexElement m_outerElement;
            exfel::util::SimpleElement<unsigned int> m_dimX, m_dimY, m_dimZ, m_dimC;
            exfel::util::VectorElement<T> m_pixelArray;

        public:

            ImageElement(exfel::util::Schema& expected) : m_outerElement(exfel::util::ComplexElement(expected)) {
                m_outerElement.readOnly();
                m_outerElement.displayType("Image");

                m_dimX.key("dimX");
                m_dimX.displayedName("Dimension X");
                m_dimX.assignmentOptional().defaultValue(1);
                m_dimX.readOnly();

                m_dimY.key("dimY");
                m_dimY.displayedName("Dimension Y");
                m_dimY.assignmentOptional().defaultValue(1);
                m_dimY.readOnly();

                m_dimZ.key("dimZ");
                m_dimZ.displayedName("Dimension Z");
                m_dimZ.assignmentOptional().defaultValue(1);
                m_dimZ.readOnly();

                m_dimC.key("dimC");
                m_dimC.displayedName("Dimension C");
                m_dimC.assignmentOptional().defaultValue(1);
                m_dimC.readOnly();

                m_pixelArray.key("pixelArray");
                m_pixelArray.displayedName("Pixel array");
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
                exfel::util::Schema& innerElement = m_outerElement.commit();
                m_dimX.commit(innerElement);
                m_dimY.commit(innerElement);
                m_dimZ.commit(innerElement);
                m_dimC.commit(innerElement);
                m_pixelArray.commit(innerElement);
            }
        };

        typedef ImageElement<int > INT32_IMAGE_ELEMENT;
        typedef ImageElement<float > FLOAT_IMAGE_ELEMENT;
    }
}


#endif	

