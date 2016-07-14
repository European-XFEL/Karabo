/*
 * File:   NDArrayElement.hh
 *
 * Created on July 14, 2016
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_NDARRAYELEMENT_HH
#define KARABO_UTIL_NDARRAYELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        template<typename T,
        template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector>
        class NDArrayElement : public LeafElement<NDArrayElement<T, CONT>, CONT<T> > {

            public:

            NDArrayElement(Schema& expected) : LeafElement<NDArrayElement, CONT<T> >(expected) {
            }

            NDArrayElement& minSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            NDArrayElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            virtual ReadOnlySpecific<NDArrayElement, CONT<T> >& readOnly() {
                ReadOnlySpecific<NDArrayElement, CONT<T> >& _readOnlySpecific = LeafElement<NDArrayElement, CONT<T> >::readOnly();
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

        typedef NDArrayElement<bool> NDARRAY_BOOL_ELEMENT;
        typedef NDArrayElement<signed char> NDARRAY_INT8_ELEMENT;
        typedef NDArrayElement<signed short> NDARRAY_INT16_ELEMENT;
        typedef NDArrayElement<int> NDARRAY_INT32_ELEMENT;
        typedef NDArrayElement<long long> NDARRAY_INT64_ELEMENT;
        typedef NDArrayElement<unsigned char> NDARRAY_UINT8_ELEMENT;
        typedef NDArrayElement<unsigned short> NDARRAY_UINT16_ELEMENT;
        typedef NDArrayElement<unsigned int> NDARRAY_UINT32_ELEMENT;
        typedef NDArrayElement<unsigned long long> NDARRAY_UINT64_ELEMENT;
        typedef NDArrayElement<float> NDARRAY_FLOAT_ELEMENT;
        typedef NDArrayElement<double> NDARRAY_DOUBLE_ELEMENT;

        typedef NDArrayElement<bool> VECTOR_BOOL_ELEMENT;
        typedef NDArrayElement<signed char> VECTOR_INT8_ELEMENT;
        typedef NDArrayElement<char> VECTOR_CHAR_ELEMENT;
        typedef NDArrayElement<signed short> VECTOR_INT16_ELEMENT;
        typedef NDArrayElement<int> VECTOR_INT32_ELEMENT;
        typedef NDArrayElement<long long> VECTOR_INT64_ELEMENT;
        typedef NDArrayElement<unsigned char> VECTOR_UINT8_ELEMENT;
        typedef NDArrayElement<unsigned short> VECTOR_UINT16_ELEMENT;
        typedef NDArrayElement<unsigned int> VECTOR_UINT32_ELEMENT;
        typedef NDArrayElement<unsigned long long> VECTOR_UINT64_ELEMENT;
        typedef NDArrayElement<float> VECTOR_FLOAT_ELEMENT;
        typedef NDArrayElement<double> VECTOR_DOUBLE_ELEMENT;
        typedef NDArrayElement<std::string> VECTOR_STRING_ELEMENT;
    }
}

#endif
