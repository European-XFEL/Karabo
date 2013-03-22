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

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        template<typename T,
        template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector>
        class VectorElement : public LeafElement<VectorElement<T, CONT>, CONT<T> > {

        public:

            VectorElement(Schema& expected) : LeafElement<VectorElement, CONT<T> >(expected) {
            }

            VectorElement& minSize(const unsigned int& value) {
                this->m_node->setAttribute("minSize", value);
                return *this;
            }

            VectorElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute("maxSize", value);
                return *this;
            }

        protected:

            void beforeAddition() {

                this->m_node->template setAttribute<int>("nodeType", Schema::LEAF);
                this->m_node->setAttribute("valueType", Types::to<ToLiteral>(Types::from<CONT<T> >()));

                if (this->m_node->hasAttribute("accessMode")) this->init(); // This is the default
            }
        };

        typedef VectorElement<bool> VECTOR_BOOL_ELEMENT;
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

