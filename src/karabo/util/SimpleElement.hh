/*
 * $Id$
 *
 * File:   SimpleElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:14 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_SIMPLE_ELEMENT_HH
#define	KARABO_UTIL_SIMPLE_ELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        /**
         * The SimpleElement represents a leaf and can be of any (supported) type
         */
        template<typename ValueType>
        class SimpleElement : public LeafElement<SimpleElement<ValueType>, ValueType > {
        public:

            SimpleElement(Schema& expected) : LeafElement<SimpleElement<ValueType>, ValueType >(expected) {
            }

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param opts A string with space separated values. The values are casted to the proper type.
             * @param sep  A separator symbols. Default values are " ,;"
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::string& opts, const std::string& sep = " ,;") {
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            /**
             * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the vector.
             * This function can be used when space cannot be used as a separator.
             * @param opts vector of strings. The values are casted to the proper type.
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::vector<std::string>& opts) {
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }

            /**
             * The <b>minInc</b> method sets the lowest value accepted for this parameter. Defines the left-closed interval.
             * @param val minimum value
             * @return reference to the SimpleElement
             */
            SimpleElement& minInc(ValueType const& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_INC, value);
                return *this;
            }

            /**
             * The <b>maxInc</b> sets the highest value accepted for this parameter. Defines the right-closed interval.
             * @param val maximum value
             * @return reference to the SimpleElement
             */
            SimpleElement& maxInc(ValueType const& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_INC, value);
                return *this;
            }

            /**
             * The <b>minExc</b> sets the upper limit for this parameter. Defines the left-open interval.
             * @param val upper limit
             * @return reference to the SimpleElement
             */
            SimpleElement& minExc(ValueType const& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_EXC, value);
                return *this;
            }

            /**
             * The <b>maxExc</b> sets the lower limit for this parameter. Defines the right-open interval.
             * @param val lower limit
             * @return reference to the SimpleElement
             */
            SimpleElement& maxExc(ValueType const& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_EXC, value);
                return *this;
            }

            /**
             * The <b>hex</b> tells the GUI to interpret the numeric value as a hex string.
             * @return reference to the SimpleElement to use chaining
             */
            SimpleElement& hex() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "hex");
                return *this;
            }

            /**
             * The <b>oct</b> tells the GUI to interpret the numeric value as a hex string.
             * @return reference to the SimpleElement to use chaining
             */
            SimpleElement& oct() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "oct");
                return *this;
            }

            /**
             * The <b>bin</b> tells the GUI to interpret the numeric value as a bit string.
             * @return reference to the SimpleElement to use chaining
             */
            SimpleElement& bin() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "bin");
                return *this;
            }
            
            /**
             * The <b>bin</b> tells the GUI to interpret the numeric value as a bit string.
             * @param meaning A string which describes the meaning of each bit, the format is
             * 0:isError,1:isMoving,31:isOff
             * NOTE: bits can be described randomly (no need mentioning them all)
             * @return reference to the SimpleElement to use chaining
             */
            SimpleElement& bin(const std::string& meaning) {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "bin|" + meaning);
                return *this;
            }
            

        protected:

            void beforeAddition() {

                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                this->m_node->template setAttribute(KARABO_SCHEMA_VALUE_TYPE, Types::to<ToLiteral>(Types::from<ValueType > ()));

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {

                    //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||
                            this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT ||
                            this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) {

                        this->userAccess();

                    } else { //else set default value of requiredAccessLevel to OBSERVER 
                        this->observerAccess();
                    }

                }

                checkMinExcMaxExc();
                checkMinIncMaxInc();

            }

        private:

            void checkMinIncMaxInc() {
                if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_INC) && this->m_node->hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType > (KARABO_SCHEMA_MIN_INC);
                    const ValueType& max = this->m_node->template getAttribute<ValueType > (KARABO_SCHEMA_MAX_INC);
                    if (min > max) {
                        std::ostringstream msg;
                        msg << "Minimum value (" << min << ") is greater than maximum (" << max
                                << ") on parameter \"" << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            void checkMinExcMaxExc() {
                // this is a default implementation valid for all integral types
                if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_EXC) && this->m_node->hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType > (KARABO_SCHEMA_MIN_EXC);
                    const ValueType& max = this->m_node->template getAttribute<ValueType > (KARABO_SCHEMA_MAX_EXC);
                    if (min >= max) {
                        std::ostringstream msg;
                        msg << "The open range: (" << min << "," << max << ") is empty on parameter \""
                                << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }
        };

        //        template<>
        //        class SimpleElement<Types::Any> {
        //            Schema m_element;
        //
        //            Schema& m_expected;
        //
        //        public:
        //
        //            SimpleElement(Schema& expected) : m_expected(expected) {
        //                m_element.assignment(Schema::INTERNAL_PARAM);
        //                m_element.simpleType(Types::ANY);
        //            }
        //
        //            /**
        //             * The <b>key</b> method serves for setting up a unique name for the parameter.
        //             * @param name Unique name for the key
        //             * @return reference to the <i>SimpleElement</i>
        //             */
        //            SimpleElement& key(const std::string& name) {
        //                m_element.key(name);
        //                m_element.displayedName(name);
        //                return *this;
        //            }
        //
        //            /**
        //             * The <b>description</b> method serves for setting up a description of the element
        //             * @param desc Short description of the element
        //             * @return reference to the Element (to allow method's chaining)
        //             */
        //            SimpleElement& description(const std::string& desc) {
        //                m_element.description(desc);
        //                return *this;
        //            }
        //
        //            SimpleElement& reconfigurable() {
        //                m_element.access(WRITE);
        //                return *this;
        //            }
        //
        //            SimpleElement& readOnly() {
        //                m_element.access(READ);
        //                return *this;
        //            }
        //
        //            SimpleElement& init() {
        //                m_element.access(INIT);
        //                return *this;
        //            }
        //
        //            void commit() {
        //                m_expected.addElement(m_element);
        //            }
        //        };




        //typedef SimpleElement<boost::filesystem::path> PATH_ELEMENT;
        //typedef SimpleElement<Schema> CONFIG_ELEMENT;
        // typedef SimpleElement<Types::Any> INTERNAL_ANY_ELEMENT;

        typedef SimpleElement<bool > BOOL_ELEMENT;
        typedef SimpleElement<signed char> INT8_ELEMENT;
        typedef SimpleElement<char> CHAR_ELEMENT;
        typedef SimpleElement<signed short> INT16_ELEMENT;
        typedef SimpleElement<int> INT32_ELEMENT;
        typedef SimpleElement<long long> INT64_ELEMENT;
        typedef SimpleElement<unsigned char> UINT8_ELEMENT;
        typedef SimpleElement<unsigned short> UINT16_ELEMENT;
        typedef SimpleElement<unsigned int> UINT32_ELEMENT;
        typedef SimpleElement<unsigned long long> UINT64_ELEMENT;
        typedef SimpleElement<float> FLOAT_ELEMENT;
        typedef SimpleElement<double> DOUBLE_ELEMENT;
        typedef SimpleElement<std::string> STRING_ELEMENT;

    }
}

#endif

