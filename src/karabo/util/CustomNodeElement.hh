/*
 * File:   SerializableObjectElement.hh
 * Author: heisenb
 *
 * Created on August 26, 2016, 3:04 PM
 */

#ifndef KARABO_UTIL_CUSTOMNODEELMENT_HH
#define	KARABO_UTIL_CUSTOMNODEELMENT_HH

namespace karabo {
    namespace util {

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

