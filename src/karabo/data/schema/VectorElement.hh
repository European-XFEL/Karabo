/*
 * File:   VectorElement.hh
 *
 * Created on July 14, 2016
 *
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */


#ifndef KARABO_DATA_SCHEMA_VECTORELEMENT_HH
#define KARABO_DATA_SCHEMA_VECTORELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace data {

        /**
         * @class VectorElement
         * @brief The VectorElement can be used to hold a sequence of SimpleElements
         */
        template <typename T, template <typename ELEM, typename = std::allocator<ELEM>> class CONT = std::vector>
        class VectorElement : public LeafElement<VectorElement<T, CONT>, CONT<T>> {
           public:
            VectorElement(Schema& expected) : LeafElement<VectorElement, CONT<T>>(expected) {}

            /**
             * Specify a minimum number of entries the vector element needs to have to pass
             * validation
             * @param value
             * @return
             */
            VectorElement& minSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            /**
             * Specify a maximum number of entries the vector element needs to have to pass
             * validation
             * @param value
             * @return
             */
            VectorElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual ReadOnlySpecific<VectorElement, CONT<T>>& readOnly() {
                ReadOnlySpecific<VectorElement, CONT<T>>& _readOnlySpecific =
                      LeafElement<VectorElement, CONT<T>>::readOnly();
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, CONT<T>());
                return _readOnlySpecific;
            }

           protected:
            void beforeAddition() {
#if __GNUC__ >= 12
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, static_cast<int>(Schema::LEAF));
#else
                constexpr int schemaLeaf = static_cast<int>(Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, schemaLeaf);
#endif
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_DISPLAY_TYPE)) {
                    // for backward-compatibility displayType is "Curve" on vectors
                    this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Curve");
                }
                this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<CONT<T>>()));

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {
                    // for init, reconfigurable elements - set default value of requiredAccessLevel to OPERATOR
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||                      // init element
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT || // init element
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) ==
                              WRITE) { // reconfigurable element

                        this->operatorAccess();

                    } else { // else set default value of requiredAccessLevel to OBSERVER
                        this->observerAccess();
                    }
                }

                // If a default value is defined, check that it is within the
                // limits specified by [minSize, maxSize]; throw an exception
                // otherwise.
                if (this->m_node->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE)) {
                    const CONT<T>& defaultVal =
                          this->m_node->template getAttribute<CONT<T>>(KARABO_SCHEMA_DEFAULT_VALUE);
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_SIZE)) {
                        const unsigned int minSizeVal =
                              this->m_node->template getAttribute<unsigned int>(KARABO_SCHEMA_MIN_SIZE);
                        if (defaultVal.size() < minSizeVal) {
                            std::ostringstream oss;
                            oss << "Value has less elements, '" << defaultVal.size() << "' than allowed by "
                                << "minSize, '" << minSizeVal << "', for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MAX_SIZE)) {
                        const unsigned int maxSizeVal =
                              this->m_node->template getAttribute<unsigned int>(KARABO_SCHEMA_MAX_SIZE);
                        if (defaultVal.size() > maxSizeVal) {
                            std::ostringstream oss;
                            oss << "Value has more elements, '" << defaultVal.size() << "' than allowed by "
                                << "maxSize, '" << maxSizeVal << "', for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
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

    } // namespace data
} // namespace karabo

#endif
