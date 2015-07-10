/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_XIP_DATA_HH
#define	KARABO_XIP_DATA_HH

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
             * Configuration constructor (for later writing)
             */
            Data(const karabo::util::Hash& config);

            Data(const Data& other);
            
            Data(const std::string& channelName, const karabo::util::Hash& config);
            
            // TODO, add to binding...
            Data(const std::string& key, const Data& other);

            /**
             * Constructor for receiving
             * @param hash
             */
            Data(const karabo::util::Hash::Pointer& hash);
            
            virtual ~Data();
            
            void setNode(const std::string& key, const Data& data);
            
            template <class T>
            T getNode(const std::string& key) {
                if (m_hash->is<karabo::util::Hash>(key)) {
                    karabo::util::Hash::Pointer tmp(new karabo::util::Hash(m_hash->get<karabo::util::Hash>(key)));
                    m_hash->set(key, tmp);
                    return tmp;
                } else {
                    return m_hash->get<karabo::util::Hash::Pointer>(key);
                }
            }

            template <class T>
            void set(const std::string& key, const T& value) {
                // user setp ??
                // Protect using nested calls by changing the separator
                m_hash->set(key, value, '*');
            }

            template <class T>
            T& get(const std::string& key) {
                // use getp ??
                // Protect using nested calls by changing the separator
                return m_hash->get<T>(key, '*');
            }

            template <class T>
            const T& get(const std::string& key) const {
                return m_hash->get<T>(key);
            }

            bool has(const std::string& key) const {
                return m_hash->has(key);
            }
            
            void erase(const std::string& key) {
                m_hash->erase(key);
            }
            
            const karabo::util::Hash::Pointer& hash() const;

            void attachTimestamp(const karabo::util::Timestamp& ts);
            
            friend std::ostream& operator<<(std::ostream& os, const Data& data);

        };

        template <class Derived, class Described>
        class DataElement {
            karabo::util::Schema& m_schema;
            std::string m_key;

        public:

            DataElement(karabo::util::Schema& s) : m_schema(s) {
                m_key = Described::classInfo().getClassId();
            }

            Derived& key(const std::string& key) {
                using namespace karabo::util;
                m_key = key;
                NODE_ELEMENT(m_schema).key(m_key)
                        .appendParametersOfConfigurableClass<Data>(Described::classInfo().getClassId())
                        .commit();                
                return *(static_cast<Derived*> (this));
            }

            template <class T>
            Derived& setDefaultValue(const std::string& subKey, const T& defaultValue) {
                using namespace karabo::util;
                if (m_schema.empty()) {
                    NODE_ELEMENT(m_schema).key(m_key)
                        .appendParametersOfConfigurableClass<Data>(Described::classInfo().getClassId())
                        .commit();      
                }
                OVERWRITE_ELEMENT(m_schema).key(m_key + "." + subKey)
                        .setNewDefaultValue(defaultValue)
                        .commit();

                return *(static_cast<Derived*> (this));
            }

            void commit() {
                using namespace karabo::util;
                if (m_schema.empty()) {
                    NODE_ELEMENT(m_schema).key(m_key)
                            .appendParametersOfConfigurableClass<Data>(Described::classInfo().getClassId())
                            .commit();      
                }
            }

        };
    }
}



#endif

