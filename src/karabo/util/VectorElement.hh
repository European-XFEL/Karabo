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
                class VectorElement : public GenericElement<VectorElement<T, CONT>, CONT<T> > {
        public:

            VectorElement() : GenericElement<VectorElement<T>, CONT<T> >() {
                this->initializeElementPointer(this);
                this->m_node.simpleType(Types::getTypeAsId(CONT<T > ()));
            }

            VectorElement(Schema& expected) : GenericElement<VectorElement, CONT<T> >(expected) {

                this->initializeElementPointer(this);
                if (this->m_schema) {
                    AccessType at = this->m_schema->getAccessMode();
                    this->m_node.setAccessMode(at);
                }
                this->m_node.simpleType(Types::getTypeAsId(CONT<T > ()));
            }

            VectorElement& minInc(T const& val) {
                this->m_node.minInc(val);
                return *this;
            }

            VectorElement& maxInc(T const& val) {
                this->m_node.maxInc(val);
                return *this;
            }

            VectorElement& minExc(T const& val) {
                this->m_node.minExc(val);
                return *this;
            }

            VectorElement& maxExc(T const& val) {
                this->m_node.maxExc(val);
                return *this;
            }

            VectorElement& minSize(const int& value) {
                this->m_node.minSize(value);
                return *this;
            }

            VectorElement& maxSize(const int& value) {
                this->m_node.maxSize(value);
                return *this;
            }

            void checkConsistency() {
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
        typedef VectorElement<boost::filesystem::path> VECTOR_PATH_ELEMENT;

    }
}

#endif	

