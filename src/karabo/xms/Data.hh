/* 
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on March 23, 2015, 10:17 AM
 */

#ifndef KARABO_XIP_DATA_HH
#define	KARABO_XIP_DATA_HH

#include <karabo/util.hpp>
// redundant: #include"karabo/util/Exception.hh

namespace karabo {
    namespace xms {
        /**
         * Class Data is used to send hierarchical data structures point-to-point.
         * 
         * Data is similar to karabo::util::Hash in the sense that simple data
         * structures like scalars and vectors are added via Data::set(key, value)
         * and received via Data::get<StructureType>(key).
         * But do not use values of type Hash or Hash::Pointer to create nodes
         * in a hierarchical data structure. Instead, use Data::setNode(key, data)
         * with data being a Data object (or something inheriting from Data without
         * adding new data members and providing a constructor from a Hash::Pointer,
         * e.g. NDArray or ImageData).
         * Internally, all nodes are stored as Hash::Pointer or are converted
         * from Hash to Hash::Pointer when one tries to get them out of a Data
         * object.
         * To get back the nodes, use e.g. Data d(data.getNode<Data>(key));
         * or the equivalent for NDArray or ImageData replacing Data.
         */
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
            
            /**
             * Get a node and create a T object from it,
             * e.g. Data, NDArray, ImageData.
             */
            template <class T>
            T getNode(const std::string& key) const {
                if (m_hash->is<karabo::util::Hash>(key)) {
                    // Replace Hash by Hash::Pointer
                    karabo::util::Hash::Pointer ptr(new karabo::util::Hash(m_hash->get<karabo::util::Hash>(key)));
                    // const_cast to achieve class promise:
                    // There are only Hash::Pointer inside.
                    const_cast<karabo::util::Hash&>(*m_hash).set(key, ptr);
                    return ptr;
                } else {
                    return m_hash->get<karabo::util::Hash::Pointer>(key);
                }
            }

            template <class T>
            void set(const std::string& key, const T& value) {
                // Protect using nested calls by changing the separator
                m_hash->set(key, value, '*');
            }
            
            template <class T>
            // inline needed to make the specialization (see below) work
            inline T& get(const std::string& key) {
                // Protect using nested calls by changing the separator
                return m_hash->get<T>(key, '*');
            }

            template <class T>
            // inline needed to make the specialization (see below) work
            inline const T& get(const std::string& key) const {
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

        template <>
        inline karabo::util::Hash& Data::get<karabo::util::Hash>(const std::string& key)
        {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Cannot get Hash out of Data, only Hash::Pointer");
        }
        
        template <>
        inline const karabo::util::Hash& Data::get<karabo::util::Hash>(const std::string& key) const
        {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Cannot get Hash out of Data, only Hash::Pointer");
        }

        template <>
        inline karabo::util::Hash::Pointer& Data::get<karabo::util::Hash::Pointer>(const std::string& key)
        {
            if (m_hash->is<karabo::util::Hash>(key)) {
                // Replace Hash by Hash::Pointer
                karabo::util::Hash::Pointer ptr(new karabo::util::Hash(m_hash->get<karabo::util::Hash>(key)));
                m_hash->set(key, ptr);
            }
            return m_hash->get<karabo::util::Hash::Pointer>(key);
        }
        
        template <>
        inline const karabo::util::Hash::Pointer& Data::get<karabo::util::Hash::Pointer>(const std::string& key) const
        {
            if (m_hash->is<karabo::util::Hash>(key)) {
                // Replace Hash by Hash::Pointer - const_cast is fair to achieve
                // class promise: There are only Hash::Pointer inside.
                karabo::util::Hash::Pointer ptr(new karabo::util::Hash(m_hash->get<karabo::util::Hash>(key)));
                const_cast<karabo::util::Hash&>(*m_hash).set(key, ptr);
            }
            return m_hash->get<karabo::util::Hash::Pointer>(key);
        }


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

