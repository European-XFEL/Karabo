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


#ifndef KARABO_UTIL_SIMPLEELEMENT_HH
#define	KARABO_UTIL_SIMPLEELEMENT_HH

#include "GenericElement.hh"

namespace karabo {
    namespace util {

        /**
         * The SimpleElement class inherits from GenericElement with template parameters:
         * SimpleElement<T> - this is used in chained functions to return reference directly to the SimpleElement
         * T - describes the type of default value
         *
         */
        template<typename T>
        class SimpleElement : public GenericElement<SimpleElement<T>, T > {
        public:

            SimpleElement() : GenericElement<SimpleElement<T>, T>() {
                this->initializeElementPointer(this);
                this->m_element.simpleType(Types::getTypeAsId<T > ());
            }

            SimpleElement(Schema& expected) : GenericElement<SimpleElement<T>, T >(expected) {


                // Note:
                // because compilers do not look in dependent base classes when looking for non dependent names
                // we must explicitly use "this" pointer when accessing m_element or m_expected

                this->initializeElementPointer(this);

                // TODO BH Do we have to do this here?? Or can it be anywhere before commit()
                if (this->m_expected) {
                    AccessType at = this->m_expected->getAccessMode();
                    this->m_element.setAccessMode(at);
                }
                this->m_element.simpleType(Types::getTypeAsId(T()));
            }

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param opts A string with space separated values. The values are casted to the proper type.
             * @param sep  A separator symbols. Default values are " ,;"
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::string& opts, const std::string& sep = " ,;") {
                this->m_element.options(opts, sep);
                return *this;
            }

            /**
             * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the vector.
             * This function can be used when space cannot be used as a separator.
             * @param opts vector of strings. The values are casted to the proper type.
             * @return reference to the SimpleElement
             */
            SimpleElement& options(const std::vector<std::string>& opts) {
                this->m_element.options(opts);
                return *this;
            }

            /**
             * The <b>minInc</b> method sets the lowest value accepted for this parameter. Defines the left-closed interval.
             * @param val minimum value
             * @return reference to the SimpleElement
             */
            SimpleElement& minInc(T const& val) {
                this->m_element.minInc(val);
                return *this;
            }

            /**
             * The <b>maxInc</b> sets the highest value accepted for this parameter. Defines the right-closed interval.
             * @param val maximum value
             * @return reference to the SimpleElement
             */
            SimpleElement& maxInc(T const& val) {
                this->m_element.maxInc(val);
                return *this;
            }

            /**
             * The <b>minExc</b> sets the upper limit for this parameter. Defines the left-open interval.
             * @param val upper limit
             * @return reference to the SimpleElement
             */
            SimpleElement& minExc(T const& val) {
                this->m_element.minExc(val);
                return *this;
            }

            /**
             * The <b>maxExc</b> sets the lower limit for this parameter. Defines the right-open interval.
             * @param val lower limit
             * @return reference to the SimpleElement
             */
            SimpleElement& maxExc(T const& val) {
                this->m_element.maxExc(val);
                return *this;
            }

        protected:

            void checkConsistency() {
                checkMinIncMaxInc(this->m_element, T());
                checkMinExcMaxExc(this->m_element, T());
            }

        private:

            template <class U>
            void checkMinIncMaxInc(const Schema& el, const U&) {
                if (el.has("minInc") && el.has("maxInc")) {
                    if (el.get<T > ("minInc") > el.get<T > ("maxInc")) {
                        std::ostringstream msg;
                        msg << "minimum value " << el.get<T > ("minInc")
                                << " is greater than maximum " << el.get<T > ("maxInc");
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            template <class U> 
            void checkMinExcMaxExc(const Schema& el, const U& var) {
                // this is a default implementation valid for all integral types
                if (el.has("minExc") && el.has("maxExc")) {
                    if ((el.get<U > ("minExc") + 1) >= el.get<U > ("maxExc")) {
                        std::ostringstream msg;
                        msg << "The open range: (" << el.get<U > ("minExc") << "," << el.get<U > ("maxExc") << ") is empty";
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }
            
            void checkMinIncMaxInc(const Schema&, const std::vector<std::string>&) {
                // do nothing
            }
            
            void checkMinExcMaxExc(const Schema&, const std::vector<std::string>&) {
                // do nothing
            }

            void checkMinExcMaxExc(const Schema& el, const boost::filesystem::path& var) {
                //TODO: implement
            }

            void checkMinExcMaxExc(const Schema& el, const std::string & var) {
                //TODO: implement
            }

            void checkMinExcMaxExc(const Schema& el, const float& var) {
                if (el.has("minExc") && el.has("maxExc")) {
                    if (el.get<float> ("minExc") >= el.get<float > ("maxExc")) {
                        std::ostringstream msg;
                        msg << "minimum value " << el.get<float> ("minExc")
                                << " is greater or equal to maximum " << el.get<float> ("maxExc");
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            void checkMinExcMaxExc(const Schema& el, const double& var) {
                if (el.has("minExc") && el.has("maxExc")) {
                    if (el.get<float> ("minExc") >= el.get<float > ("maxExc")) {
                        std::ostringstream msg;
                        msg << "minimum value " << el.get<float> ("minExc")
                                << " is greater or equal to maximum " << el.get<float> ("maxExc");
                        throw KARABO_PARAMETER_EXCEPTION(msg.str());
                    }
                }
            }

            //TODO: implement checkMinExcMaxInc, checkMinIncMaxExc

        };

        template<>
        class SimpleElement<Types::Any> {
            Schema m_element;

            Schema& m_expected;

        public:

            SimpleElement(Schema& expected) : m_expected(expected) {
                m_element.assignment(Schema::INTERNAL_PARAM);
                m_element.simpleType(Types::ANY);
            }

            /**
             * The <b>key</b> method serves for setting up a unique name for the parameter.
             * @param name Unique name for the key
             * @return reference to the <i>SimpleElement</i>
             */
            SimpleElement& key(const std::string& name) {
                m_element.key(name);
                m_element.displayedName(name);
                return *this;
            }

            /**
             * The <b>description</b> method serves for setting up a description of the element
             * @param desc Short description of the element
             * @return reference to the Element (to allow method's chaining)
             */
            SimpleElement& description(const std::string& desc) {
                m_element.description(desc);
                return *this;
            }

            SimpleElement& reconfigurable() {
                m_element.access(WRITE);
                return *this;
            }

            SimpleElement& readOnly() {
                m_element.access(READ);
                return *this;
            }

            SimpleElement& init() {
                m_element.access(INIT);
                return *this;
            }

            void commit() {
                m_expected.addElement(m_element);
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
        typedef SimpleElement<boost::filesystem::path> PATH_ELEMENT;
        typedef SimpleElement<Schema> CONFIG_ELEMENT;
        typedef SimpleElement<Types::Any> INTERNAL_ANY_ELEMENT;

    }
}

#endif

