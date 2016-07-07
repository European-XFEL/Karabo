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
#include "AlarmConditions.hh"

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
             * The <b>relativeError</b> sets the relative error of this parameter.
             * Ideally, |x * relativeError| > |x - x0|
             * with x the measured value and x0 the real value
             */
            SimpleElement& relativeError(double error) {
                this->m_node->setAttribute(KARABO_SCHEMA_RELATIVE_ERROR, error);
                return *this;
            }

            /**
             * the <b>absoluteError</b> sets the absolute error of this parameter
             * Ideally, absoluteError > |x - x0|
             * with x the measured value and x0 the real value
             */
            SimpleElement& absoluteError(double error) {
                this->m_node->setAttribute(KARABO_SCHEMA_ABSOLUTE_ERROR, error);
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
                checkWarnAndAlarm();

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
            
            //only makes sense for simple element, as we cannot know how to evaluated for vectors etc
            void checkWarnAndAlarm() {
                using namespace karabo::util;
                this->checkAttributeOrder(AlarmCondition::WARN_LOW, AlarmCondition::WARN_HIGH);
                this->checkAttributeOrder(AlarmCondition::WARN_LOW, AlarmCondition::ALARM_HIGH);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::ALARM_HIGH);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::WARN_LOW);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::WARN_HIGH);
                this->checkAttributeOrder(AlarmCondition::WARN_HIGH, AlarmCondition::ALARM_HIGH);
               
            }
            
            
            void checkAttributeOrder(const karabo::util::AlarmCondition& condLow, const karabo::util::AlarmCondition& condHigh){
                const std::string& attributeLow = condLow.asString();
                const std::string& attributeHigh = condHigh.asString();
                if (this->m_node->hasAttribute(attributeLow) && this->m_node->hasAttribute(attributeHigh)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType > (attributeLow);
                    const ValueType& max = this->m_node->template getAttribute<ValueType > (attributeHigh);
                    if (min > max) {
                        std::ostringstream msg;
                        msg << attributeLow << " value (" << min << ") is greater than " << attributeHigh << "(" << max
                                << ") on parameter \"" << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }
            
        };

        

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

