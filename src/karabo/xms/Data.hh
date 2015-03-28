/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_XMS_DATA_HH
#define	KARABO_XMS_DATA_HH

#include <karabo/util.hpp>

namespace karabo {
    namespace xms {

        class Data {
        protected:

            karabo::util::Hash::Pointer m_hash;


        public:

            KARABO_CLASSINFO(Data, "Data", "1.3");

            static void expectedParameters(karabo::util::Schema& expected);


            Data();

            /**
             * Configuration constructor
             */
            Data(const karabo::util::Hash& config);

            void setId(unsigned int i);


            const karabo::util::Hash::Pointer& getHashPointer() const;

        };

        template <class Derived, class Described>
        class DataElement {
            karabo::util::Schema& m_schema;
            std::string m_key;

        public:

            DataElement(karabo::util::Schema& s) : m_schema(s) {

            }

            Derived& key(const std::string& key) {
                using namespace karabo::util;
                m_key = key;
                NODE_ELEMENT(m_schema).key(m_key)
                        .appendParametersOf<Described>()
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
                // Dummy function (commit was never really needed... :-()
            }

        };
    }
}



#endif

