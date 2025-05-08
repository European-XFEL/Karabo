/*
 * $Id$
 *
 * File:   SimpleElement.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 1, 2011, 11:14 AM
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


#ifndef KARABO_DATA_SCHEMA_SIMPLE_ELEMENT_HH
#define KARABO_DATA_SCHEMA_SIMPLE_ELEMENT_HH

#include "LeafElement.hh"
#include "karabo/data/types/AlarmConditions.hh"

namespace karabo {
    namespace data {

        /**
         * @class SimpleElement
         * @brief The SimpleElement represents a leaf and can be of any (supported) type
         */
        template <typename ValueType>
        class SimpleElement : public LeafElement<SimpleElement<ValueType>, ValueType> {
           public:
            SimpleElement(Schema& expected) : LeafElement<SimpleElement<ValueType>, ValueType>(expected) {}

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param opts A string with space separated values. The values are casted to the proper type.
             * @param sep  A separator symbols. Default values are " ,;"
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::string& opts, const std::string& sep = " ,;") {
                return options(karabo::data::fromStringForSchemaOptions<ValueType>(opts, sep));
            }

            /**
             * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the
             * vector. This function can be used when space cannot be used as a separator.
             * @param opts vector of strings. The values are casted to the proper type.
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::vector<ValueType>& opts) {
                if (opts.empty()) {
                    throw KARABO_PARAMETER_EXCEPTION("Empty list of options rejected for " + this->m_node->getKey());
                }
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }

            /**
             * The <b>minInc</b> method sets the lowest value accepted for this parameter. Defines the left-closed
             * interval.
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
#if __GNUC__ >= 13
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, static_cast<int>(Schema::LEAF));
#else
                constexpr int schemaLeaf = static_cast<int>(Schema::LEAF);
                this->m_node->template setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, schemaLeaf);
#endif
                this->m_node->template setAttribute(KARABO_SCHEMA_VALUE_TYPE,
                                                    Types::to<ToLiteral>(Types::from<ValueType>()));

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {
                    // for init, reconfigurable elements - set default value of requiredAccessLevel to OPERATOR
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT ||
                        this->m_node->template getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE) {
                        this->operatorAccess();

                    } else { // else set default value of requiredAccessLevel to OBSERVER
                        this->observerAccess();
                    }
                }

                checkMinExcMaxExc();
                checkMinIncMaxInc();
                checkWarnAndAlarm();
                checkDefaultValue();
            }

           private:
            void checkMinIncMaxInc() {
                if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_INC) &&
                    this->m_node->hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MIN_INC);
                    const ValueType& max = this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MAX_INC);
                    if (min > max) {
                        std::ostringstream msg;
                        msg << "Minimum value (" << min << ") is greater than maximum (" << max << ") on parameter \""
                            << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            void checkMinExcMaxExc() {
                // this is a default implementation valid for all integral types
                if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_EXC) &&
                    this->m_node->hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MIN_EXC);
                    const ValueType& max = this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MAX_EXC);
                    if (min >= max) {
                        std::ostringstream msg;
                        msg << "The open range: (" << min << "," << max << ") is empty on parameter \""
                            << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            // only makes sense for simple element, as we cannot know how to evaluated for vectors etc

            void checkWarnAndAlarm() {
                using namespace karabo::data;
                this->checkAttributeOrder(AlarmCondition::WARN_LOW, AlarmCondition::WARN_HIGH);
                this->checkAttributeOrder(AlarmCondition::WARN_LOW, AlarmCondition::ALARM_HIGH);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::ALARM_HIGH);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::WARN_LOW);
                this->checkAttributeOrder(AlarmCondition::ALARM_LOW, AlarmCondition::WARN_HIGH);
                this->checkAttributeOrder(AlarmCondition::WARN_HIGH, AlarmCondition::ALARM_HIGH);
            }

            void checkAttributeOrder(const karabo::data::AlarmCondition& condLow,
                                     const karabo::data::AlarmCondition& condHigh) {
                const std::string& attributeLow = condLow.asString();
                const std::string& attributeHigh = condHigh.asString();
                if (this->m_node->hasAttribute(attributeLow) && this->m_node->hasAttribute(attributeHigh)) {
                    const ValueType& min = this->m_node->template getAttribute<ValueType>(attributeLow);
                    const ValueType& max = this->m_node->template getAttribute<ValueType>(attributeHigh);
                    if (min > max) {
                        std::ostringstream msg;
                        msg << attributeLow << " value (" << min << ") is greater than " << attributeHigh << "(" << max
                            << ") on parameter \"" << this->m_node->getKey() << "\"";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }


            // If a default value is defined, check that it is within limits and
            // is among the valid options; throw an exception otherwise.
            void checkDefaultValue() {
                if (this->m_node->hasAttribute(KARABO_SCHEMA_DEFAULT_VALUE)) {
                    const ValueType& defaultVal =
                          this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_DEFAULT_VALUE);
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_EXC)) {
                        const ValueType& minExcVal =
                              this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MIN_EXC);
                        if (defaultVal <= minExcVal) {
                            std::ostringstream oss;
                            oss << "Default value, '" << defaultVal << "', is smaller than minExc limit, '" << minExcVal
                                << "' for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MIN_INC)) {
                        const ValueType& minIncVal =
                              this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MIN_INC);
                        if (defaultVal < minIncVal) {
                            std::ostringstream oss;
                            oss << "Default value, '" << defaultVal << "', is smaller than minInc limit, '" << minIncVal
                                << "' for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MAX_EXC)) {
                        const ValueType& maxExcVal =
                              this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MAX_EXC);
                        if (defaultVal >= maxExcVal) {
                            std::ostringstream oss;
                            oss << "Default value, '" << defaultVal << "', is greater than maxExc limit, '" << maxExcVal
                                << "' for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_MAX_INC)) {
                        const ValueType& maxIncVal =
                              this->m_node->template getAttribute<ValueType>(KARABO_SCHEMA_MAX_INC);
                        if (defaultVal > maxIncVal) {
                            std::ostringstream oss;
                            oss << "Default value, '" << defaultVal << "', is greater than maxInc limit, '" << maxIncVal
                                << "' for parameter '" << this->m_node->getKey() << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                    if (this->m_node->hasAttribute(KARABO_SCHEMA_OPTIONS)) {
                        const std::vector<ValueType>& optionsVals =
                              this->m_node->template getAttribute<std::vector<ValueType>>(KARABO_SCHEMA_OPTIONS);
                        if (std::find(optionsVals.begin(), optionsVals.end(), defaultVal) == optionsVals.end()) {
                            std::ostringstream oss;
                            oss << "Default value, '" << defaultVal
                                << "', is not among the valid options for parameter '" << this->m_node->getKey()
                                << "'.";
                            throw KARABO_PARAMETER_EXCEPTION(oss.str());
                        }
                    }
                }
            }
        };


        typedef SimpleElement<bool> BOOL_ELEMENT;
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

    } // namespace data
} // namespace karabo

#endif
