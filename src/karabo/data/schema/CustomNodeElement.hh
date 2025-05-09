/*
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
/*
 * File:   CustomNodeElement.hh
 * Author: heisenb
 *
 * Created on August 26, 2016, 3:04 PM
 */

#ifndef KARABO_DATA_SCHEMA_CUSTOMNODEELMENT_HH
#define KARABO_DATA_SCHEMA_CUSTOMNODEELMENT_HH

#include "NodeElement.hh"
#include "OverwriteElement.hh"

namespace karabo {
    namespace data {

        /**
         * Helper class to construct custom NODE_ELEMENTs for schemas.
         * Usage is best explained by example, say you coded a custom data class
         * (by inheriting protected Hash) like:
         * @code
         * class MyData : protected Hash {
         *     MyData(...) {
         *         setInternal(DataType()));
         *     }
         *     void setInternal(const SomeType& data) {
         *         set("internal", data);
         *     }
         * };
         * @endcode
         * Then you need a class describing the content of your class
         * @code
         * class MyDataDescription {
         *       public:
         *         // 2nd arg should match the classId of the class we describe (not of the describing class),
         *         // so that Schema::getCustomNodeClass(..) returns that one.
         *         KARABO_CLASSINFO(MyDataDescription, MyData::classInfo().getClassId(), "3.0");
         *         static void expectedParameters(const Schema& s) {
         *             // parameter definition [...]
         *         }
         * };
         * @endcode
         * and you may generate a NODE_ELEMENT like this:
         * @code{.cpp}
         * class MyDataElement : public CustomNodeElement<MyDataElement, MyDataDescription> {
         *     typedef karabo::data::CustomNodeElement<MyDataElement, MyDataDescription> ParentType;
         *     MyDataElement(karabo::data::Schema& s) : CustomNodeElement<MyDataElement, MyDataDescription>(s) {
         *     }
         *
         *     // If you want to expose parameters for setting defaults do like:
         *     CustomNodeElement<ImageData>& setFoo(const std::string& bar) {
         *         return CustomNodeElement<ImageData >::setDefaultValue("foo", bar);
         *     }
         * };
         * typedef MyDataElement MYDATA_ELEMENT;
         * @endcode
         *
         * Note that it is possible to integrate the description into the custom data class, i.e. instead of providing
         * MyDataDescription class, one can add the 'expectedParameters(..)' to 'MyData' and then use directly
         * `MyData` as second template of the CustomNodeElement.
         *
         */
        template <class Derived, class Described>
        class CustomNodeElement {
            std::string m_key;

           protected:
            karabo::data::Schema& m_schema;

           public:
            CustomNodeElement(karabo::data::Schema& s) : m_schema(s) {
                m_key = Described::classInfo().getClassId();
            }

            /**
             * The <b>key</b> method serves for setting up a unique name for the element.
             * @param name Unique name for the key
             * @return reference to the Element (to allow method's chaining)
             *
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         .key("type")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& key(const std::string& key) {
                using namespace karabo::data;
                m_key = key;
                NODE_ELEMENT(m_schema).key(m_key).template appendParametersOf<Described>().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>displayedName</b> method serves for setting up an user friendly name for the element
             * to be used by GUI
             * @param name User friendly name for the element
             * @return reference to the Element (to allow method's chaining)
             *
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .displayedName("Connection Type")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& displayedName(const std::string& name) {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNewDisplayedName(name).commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>description</b> method serves for setting up a description of the element
             * @param desc Short description of the element
             * @return reference to the Element (to allow method's chaining)
             *
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .description("Decide whether the connection is used to implement a TCP Server or TCP Client")
             *         ...
             *         .commit();
             * @endcode
             */
            Derived& description(const std::string& desc) {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNewDescription(desc).commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& init() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowInit().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& reconfigurable() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowReconfigurable().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& readOnly() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowReadOnly().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>observerAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OBSERVER.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& observerAccess() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowObserverAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>operatorAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OPERATOR.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& operatorAccess() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowOperatorAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>expertAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * EXPERT.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& expertAccess() {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowExpertAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * Set the default value for a subkey of the CustomNodeElement
             * @param subKey
             * @param defaultValue
             * @return
             */
            template <class T>
            Derived& setDefaultValue(const std::string& subKey, const T& defaultValue) {
                using namespace karabo::data;

                OVERWRITE_ELEMENT(m_schema).key(m_key + "." + subKey).setNewDefaultValue(defaultValue).commit();
                return *(static_cast<Derived*>(this));
            }

            /**
             * Set the maximum size of a subkey of the CustomNodeElement. This is required by the DAQ for all vector
             * attributes if its not assigned automatically or just to use a different value then the DAQ's default
             * length of 1000.
             *
             * @param subKey: Key of the vector attribute (like <b>"pixels.shape"</b>)
             * @param maxSize: Number as the maximum size (like <b>3</b>)
             * @return
             */
            Derived& setMaxSize(const std::string& subKey, const unsigned int maxSize) {
                using namespace karabo::data;
                OVERWRITE_ELEMENT(m_schema)
                      .key(m_key + "." + subKey)
                      .template setNewMaxSize<unsigned int>(maxSize)
                      .commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * Set the unit for a subkey of the CustomNodeElement
             * @param subKey
             * @param unit
             * @return
             */
            Derived& setUnit(const std::string& subKey, const Unit& unit) {
                m_schema.setUnit(m_key + "." + subKey, unit);
                return *(static_cast<Derived*>(this));
            }

            /**
             * Set the metric prefix for a subkey of the CustomNodeElement
             * @param subKey
             * @param metricPrefix
             * @return
             */
            Derived& setMetricPrefix(const std::string& subKey, const MetricPrefix& metricPrefix) {
                m_schema.setMetricPrefix(m_key + "." + subKey, metricPrefix);
                return *(static_cast<Derived*>(this));
            }

            /**
             * Registers this element into the Schema
             */
            void commit() {
                m_schema.getParameterHash().setAttribute(m_key, KARABO_SCHEMA_CLASS_ID,
                                                         Described::classInfo().getClassId());
            }
        };
    } // namespace data
} // namespace karabo

#endif
