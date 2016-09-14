/*
 * File:   CustomNodeElement.hh
 * Author: heisenb
 *
 * Created on August 26, 2016, 3:04 PM
 */

#ifndef KARABO_UTIL_CUSTOMNODEELMENT_HH
#define	KARABO_UTIL_CUSTOMNODEELMENT_HH

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

            Derived& key(const std::string& key) {
                using namespace karabo::util;
                m_key = key;
                NODE_ELEMENT(m_schema).key(m_key)
                        .appendParametersOf<Described>()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& displayedName(const std::string& name) {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNewDisplayedName(name)
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& description(const std::string& desc) {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNewDescription(desc)
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& init() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowInit()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& reconfigurable() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowReconfigurable()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& readOnly() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowReadOnly()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& observerAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowObserverAccess()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& userAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowUserAccess()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& operatorAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowOperatorAccess()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& expertAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowExpertAccess()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& adminAccess() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowAdminAccess()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            Derived& skipValidation() {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key)
                        .setNowSkipValidation()
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            template <class T>
            Derived& setDefaultValue(const std::string& subKey, const T& defaultValue) {
                using namespace karabo::util;
                OVERWRITE_ELEMENT(m_schema).key(m_key + "." + subKey)
                        .setNewDefaultValue(defaultValue)
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            void commit() {
            }
        };
    }
}

#endif

