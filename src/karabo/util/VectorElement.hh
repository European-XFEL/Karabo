/*
 * $Id$
 *
 * File:   VectorElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:21 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_VECTORELEMENT_HH
#define	KARABO_UTIL_VECTORELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        template<typename T,
                template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector>
                class VectorElement : public LeafElement<VectorElement<T, CONT>, CONT<T> > {
        public:

            VectorElement(Schema& expected) : LeafElement<VectorElement>(expected) {
            }
            
             /**
             * The <b>minInc</b> method sets the lowest value accepted for this parameter. Defines the left-closed interval.
             * @param val minimum value
             * @return reference to the VectorElement
             */
            VectorElement& minInc(ValueType const& value) {
                this->m_node.setAttribute("minInc", value);
                return *this;
            }

            /**
             * The <b>maxInc</b> sets the highest value accepted for this parameter. Defines the right-closed interval.
             * @param val maximum value
             * @return reference to the VectorElement
             */
            VectorElement& maxInc(ValueType const& value) {
                this->m_node.setAttribute("maxInc", value);
                return *this;
            }

            /**
             * The <b>minExc</b> sets the upper limit for this parameter. Defines the left-open interval.
             * @param val upper limit
             * @return reference to the VectorElement
             */
            VectorElement& minExc(ValueType const& value) {
                this->m_node.setAttribute("minExc", value);
                return *this;
            }

            /**
             * The <b>maxExc</b> sets the lower limit for this parameter. Defines the right-open interval.
             * @param val lower limit
             * @return reference to the VectorElement
             */
            VectorElement& maxExc(ValueType const& value) {
                this->m_node.setAttribute("maxExc", value);
                return *this;
            }
            
            VectorElement& minSize(const int& value) {
                this->m_node.setAttribute("minSize", value);
                return *this;
            }

            VectorElement& maxSize(const int& value) {
                 this->m_node.setAttribute("maxSize", value);
                return *this;
            }
            
             protected:

            void beforeAddition() {

                this->m_node->setAttribute<int>("nodeType", Schema::LEAF);
                this->m_node->setAttribute<int>("valueType", Types::to<ToLiteral>(Types::from<CONT<T>>()));
                
                if (this->m_node->hasAttribute("accessMode")) this->init(); // This is the default
            }
        };

        typedef VectorElement<bool, std::deque> VECTOR_BOOL_ELEMENT;
        typedef VectorElement<signed char> VECTOR_INT8_ELEMENT;
        typedef VectorElement<char> VECTOR_CHAR_ELEMENT;
        typedef VectorElement<signed short> VECTOR_INT16_ELEMENT;
        typedef VectorElement<int> VECTOR_INT32_ELEMENT;
        typedef VectorElement<long long> VECTOR_INT64_ELEMENT;
        typedef VectorElement<unsigned char> VECTOR_UINT8_ELEMENT;
        typedef VectorElement<unsigned short> VECTOR_UINT16_ELEMENT;
        typedef VectorElement<unsigned int> VECTOR_UINT32_ELEMENT;
        typedef VectorElement<unsigned long long> VECTOR_UINT64_ELEMENT;
        typedef VectorElement<float> VECTOR_FLOAT_ELEMENT;
        typedef VectorElement<double> VECTOR_DOUBLE_ELEMENT;
        typedef VectorElement<std::string> VECTOR_STRING_ELEMENT;
    }
}

#endif	

