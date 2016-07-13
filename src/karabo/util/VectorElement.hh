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
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            VectorElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            virtual ReadOnlySpecific<VectorElement, CONT<T> >& readOnly() {
                ReadOnlySpecific<VectorElement, CONT<T> >& _readOnlySpecific = LeafElement<VectorElement, CONT<T> >::readOnly();
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, CONT<T>());
                return _readOnlySpecific;
            }

        protected:

            void beforeAddition() {

                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Curve");
                this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<CONT<T> >()));

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

                    //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) || //init element
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT || //init element
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) { //reconfigurable element

                        this->userAccess();

                    } else { //else set default value of requiredAccessLevel to OBSERVER 
                        this->observerAccess();
                    }
                }
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

