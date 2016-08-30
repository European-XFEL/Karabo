/*
 * File:   CustomNodeElement.hh
 * Author: heisenb
 *
 * Created on August 26, 2016, 3:04 PM
 */

#ifndef KARABO_UTIL_CUSTOMNODEELMENT_HH
#define	KARABO_UTIL_CUSTOMNODEELMENT_HH

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
        template <class Described>
        class CustomNodeElement {

            std::string m_key;

        protected:
            karabo::util::Schema& m_schema;

        public:

            CustomNodeElement(karabo::util::Schema& s) : m_schema(s) {
                m_key = Described::classInfo().getClassId();
            }

            CustomNodeElement& key(const std::string& key) {
                using namespace karabo::util;
                m_key = key;
                NODE_ELEMENT(m_schema).key(m_key)
                        .appendParametersOf<Described>()
                        .commit();
                return *this;
            }

            template <class T>
            CustomNodeElement& setDefaultValue(const std::string& subKey, const T& defaultValue) {
                using namespace karabo::util;
                if (m_schema.empty()) {
                    NODE_ELEMENT(m_schema).key(m_key)
                            .appendParametersOf<Described>()
                            .commit();
                }
                OVERWRITE_ELEMENT(m_schema).key(m_key + "." + subKey)
                        .setNewDefaultValue(defaultValue)
                        .commit();

                return *this;
            }

            void commit() {
                using namespace karabo::util;
                if (m_schema.empty()) {
                    NODE_ELEMENT(m_schema).key(m_key)
                            .appendParametersOf<Described>()
                            .commit();
                }
            }
        };
    }
}

#endif

