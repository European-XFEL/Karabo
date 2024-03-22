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

#ifndef KARABO_UTIL_CUSTOMNODEELMENT_HH
#define KARABO_UTIL_CUSTOMNODEELMENT_HH

#include "NodeElement.hh"
#include "OverwriteElement.hh"

namespace karabo {
    namespace util {

        /**
         * Helper class to construct custom NODE_ELEMENTs for schemas.
         * Usage is best explained by example, say you coded a custom data class
         * (by inheriting protected Hash) describing it's expected parameters like:
         * @code
         * class MyData : protected Hash {
         *     static void expectedParameters(const Schema& s) {
         *         // parameter definition [...]
         *     }
         * };
         * @endcode
         * Then you may generate a NODE_ELEMENT like this:
         * @code{.cpp}
         * class MyDataElement : public CustomNodeElement<MyData> {
         *
         *     MyDataElement(karabo::util::Schema& s) : CustomNodeElement<MyData>(s) {
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
         *
         *
         */
        template <class Derived, class Described>
        class CustomNodeElement {
            std::string m_key;

           protected:
            karabo::util::Schema& m_schema;

           public:
            CustomNodeElement(karabo::util::Schema& s) : m_schema(s) {
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
                using namespace karabo::util;
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
                using namespace karabo::util;
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
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNewDescription(desc).commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& init() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowInit().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& reconfigurable() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowReconfigurable().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>readOnly</b> method serves for setting up an access type property that allows the element
             * to be included  in monitoring schema only.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& readOnly() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowReadOnly().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>observerAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OBSERVER.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& observerAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowObserverAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>userAccess</b> method serves for setting up the <i>required access level</i> attribute to be USER.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& userAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowUserAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>operatorAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * OPERATOR.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& operatorAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowOperatorAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>expertAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * EXPERT.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& expertAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowExpertAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * The <b>adminAccess</b> method serves for setting up the <i>required access level</i> attribute to be
             * ADMIN.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& adminAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowAdminAccess().commit();

                return *(static_cast<Derived*>(this));
            }

            /**
             * Specify one or more actions that are allowed on this node.
             *
             * If a Karabo device specifies allowed actions for a node, that means that it offers a specific slot
             * interface to operate on this node.
             * Which allowed actions require which interface will be defined elsewhere.
             * @return reference to the Element (to allow method's chaining)
             */
            Derived& setAllowedActions(const std::vector<std::string>& actions) {
                m_schema.setAllowedActions(m_key, actions);
                return *(static_cast<Derived*>(this));
            }


            /**
             * Skip this element during validation of configuration against a Schema
             * @return
             */
            Derived& skipValidation() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key).setNowSkipValidation().commit();

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
                using namespace karabo::util;

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
                using namespace karabo::util;
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
            Derived& setUnit(const std::string& subKey, const UnitType& unit) {
                m_schema.setUnit(m_key + "." + subKey, unit);
                return *(static_cast<Derived*>(this));
            }

            /**
             * Set the metric prefix for a subkey of the CustomNodeElement
             * @param subKey
             * @param metricPrefix
             * @return
             */
            Derived& setMetricPrefix(const std::string& subKey, const MetricPrefixType& metricPrefix) {
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
    } // namespace util
} // namespace karabo

#endif
