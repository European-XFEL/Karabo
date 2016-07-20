/* 
 * File:   OrderedMap.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on December 14, 2012, 11:22 AM
 */

#ifndef KARABO_UTIL_ORDERED_MAP_HH
#define	KARABO_UTIL_ORDERED_MAP_HH

#include <map>
#include <list>
#include <boost/any.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include "Types.hh"
#include "Exception.hh"

namespace karabo {

    namespace util {

        template<class KeyType, class MappedType>
        class OrderedMap {


            typedef std::list<MappedType*> ListType;
            typedef std::map<KeyType, MappedType> MapType;

            ListType m_listNodes;
            MapType m_mapNodes;

        public:

            typedef MappedType Node;

            typedef typename MapType::iterator map_iterator;
            typedef typename MapType::const_iterator const_map_iterator;

            typedef typename ListType::iterator list_iterator;
            typedef typename ListType::const_iterator const_list_iterator;

            typedef boost::indirect_iterator<list_iterator> iterator;
            typedef boost::indirect_iterator<const_list_iterator> const_iterator;

            iterator begin() {
                return m_listNodes.begin();
            }

            const_iterator begin() const {
                return m_listNodes.begin();
            }

            iterator end() {
                return m_listNodes.end();
            }

            const_iterator end() const {
                return m_listNodes.end();
            }


        public:

            OrderedMap();

            OrderedMap(const OrderedMap<KeyType, MappedType>& other);


            template <typename T>
            OrderedMap(const KeyType& key1, const T& value1);

            template <typename T, typename U>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2);

            template <typename T, typename U, typename V>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                       const KeyType& key3, const V& value3);

            template <typename T, typename U, typename V, typename X>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                       const KeyType& key3, const V& value3, const KeyType& key4, const X& value4);

            template <typename T, typename U, typename V, typename X, typename Y>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                       const KeyType& key3, const V& value3, const KeyType& key4, const X& value4,
                       const KeyType& key5, const Y& value5);

            template <typename T, typename U, typename V, typename X, typename Y, typename Z>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                       const KeyType& key3, const V& value3, const KeyType& key4, const X& value4,
                       const KeyType& key5, const Y& value5, const KeyType& key6, const Z& value6);

            // Destructor
            virtual ~OrderedMap();


            OrderedMap<KeyType, MappedType>& operator=(const OrderedMap<KeyType, MappedType>& other);

            list_iterator lbegin();

            const_list_iterator lbegin() const;

            list_iterator lend();

            const_list_iterator lend() const;

            map_iterator mbegin();

            const_map_iterator mbegin() const;

            map_iterator mend();

            const_map_iterator mend() const;

            inline map_iterator find(const KeyType& key);

            inline const_map_iterator find(const KeyType& key) const;

            inline bool has(const KeyType& key) const;

            /**
             * Erase element identified by key if key exists.
             * @param key
             * @return number of elements erased, i.e. 0 or 1.
             */
            inline size_t erase(const KeyType& key);

            /**
             * Erase element identified by map_iterator.
             * @param it - a valid map_iterator
             * @return no
             */
            inline void erase(const map_iterator& it);

            inline size_t size() const;

            inline bool empty() const;

            inline void clear();

            template<class T>
            inline Node& set(const KeyType& key, const T& value);

            template<class T>
            inline const T& get(const KeyType& key) const;

            template<class T>
            inline T& get(const KeyType& key);

            template<class T>
            inline void get(const KeyType& key, T & value) const;

            template<class T>
            inline const T & get(const const_map_iterator & it) const;

            template<class T>
            inline T & get(const map_iterator & it);

            template <typename ValueType>
            inline ValueType getAs(const KeyType& key) const;

            template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
            inline Cont<T> getAs(const KeyType& key) const;

            inline const Node& getNode(const KeyType& key) const;

            inline Node& getNode(const KeyType& key);

            inline const boost::any& getAny(const KeyType& key) const;

            inline boost::any& getAny(const KeyType& key);

            template <typename T> bool is(const KeyType & key) const;

            template <typename T> bool is(const const_map_iterator & it) const;

            bool is(const KeyType& key, const Types::ReferenceType & type) const;

        };
    }
}

namespace karabo {

    namespace util {

        using namespace std;

        /***********************************************************************
         *
         * Implement the Container class methods
         * 
         ***********************************************************************/

        template<class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::OrderedMap() {
        }

        template<class KeyType, class MappedType>
        template <typename T>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1) {
            this->set(key1, value1);
        }

        template<class KeyType, class MappedType>
        template <typename T, typename U>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2) {
            this->set(key1, value1);
            this->set(key2, value2);
        }

        template<class KeyType, class MappedType>
        template <typename T, typename U, typename V>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                                                    const KeyType& key3, const V& value3) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
        }

        template<class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                                                    const KeyType& key3, const V& value3, const KeyType& key4, const X& value4) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
        }

        template<class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X, typename Y>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                                                    const KeyType& key3, const V& value3, const KeyType& key4, const X& value4,
                                                    const KeyType& key5, const Y& value5) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
            this->set(key5, value5);
        }

        template<class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X, typename Y, typename Z>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2,
                                                    const KeyType& key3, const V& value3, const KeyType& key4, const X& value4,
                                                    const KeyType& key5, const Y& value5, const KeyType& key6, const Z& value6) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
            this->set(key5, value5);
            this->set(key6, value6);
        }

        template<class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::OrderedMap(const OrderedMap<KeyType, MappedType>& other) {
            *this = other;
        }

        template<class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::~OrderedMap() {
        }

        template<class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>& OrderedMap<KeyType, MappedType>::operator=(const OrderedMap<KeyType, MappedType>& other) {
            if (this != &other) {
                this->m_listNodes.clear();
                this->m_mapNodes.clear();

                if (!other.empty()) {
                    this->m_mapNodes = other.m_mapNodes;
                    // Original order in "other" should be preserved
                    for (typename ListType::const_iterator it = other.m_listNodes.begin(); it != other.m_listNodes.end(); ++it) {
                        this->m_listNodes.push_back(&(find((*it)->getKey())->second));
                    }
                }
            }
            return *this;
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::list_iterator OrderedMap<KeyType, MappedType>::lbegin() {
            return m_listNodes.begin();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_list_iterator OrderedMap<KeyType, MappedType>::lbegin() const {
            return m_listNodes.begin();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::list_iterator OrderedMap<KeyType, MappedType>::lend() {
            return m_listNodes.end();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_list_iterator OrderedMap<KeyType, MappedType>::lend() const {
            return m_listNodes.end();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::mbegin() {
            return m_mapNodes.begin();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::mbegin() const {
            return m_mapNodes.begin();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::mend() {
            return m_mapNodes.end();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::mend() const {
            return m_mapNodes.end();
        }

        template<class KeyType, class MappedType>
        inline size_t OrderedMap<KeyType, MappedType>::size() const {
            return m_mapNodes.size();
        }

        template<class KeyType, class MappedType>
        inline bool OrderedMap<KeyType, MappedType>::empty() const {
            return m_mapNodes.empty();
        }

        template<class KeyType, class MappedType>
        inline void OrderedMap<KeyType, MappedType>::clear() {
            m_mapNodes.clear();
            m_listNodes.clear();
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::find(const KeyType& key) {
            return m_mapNodes.find(/*hash*/(key));
        }

        template<class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::find(const KeyType& key) const {
            return m_mapNodes.find(/*hash*/(key));
        }

        template<class KeyType, class MappedType>
        inline bool OrderedMap<KeyType, MappedType>::has(const KeyType& key) const {
            return find(key) != m_mapNodes.end();
        }

        template<class KeyType, class MappedType>
        inline size_t OrderedMap<KeyType, MappedType>::erase(const KeyType& key) {
            map_iterator it;

            if ((it = find(key)) != m_mapNodes.end()) {
                m_listNodes.remove(&it->second);
                m_mapNodes.erase(it);
                return 1;
            }
            return 0;
        }

        template<class KeyType, class MappedType>
        inline void OrderedMap<KeyType, MappedType>::erase(const map_iterator& it) {
            // it must be valid!
            m_listNodes.remove(&it->second);
            m_mapNodes.erase(it);
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline MappedType& OrderedMap<KeyType, MappedType>::set(const KeyType& key, const T& value) {
            // COUT("__set T");
            map_iterator it;
            if (((it = find(key)) != m_mapNodes.end())) {
                MappedType& node = it->second;
                node.setValue(value);
                return node;
            } else {
                static MappedType local;
                MappedType& node = (m_mapNodes[/*hash*/(key)] = local);
                node.setKey(key);
                node.setValue(value);
                m_listNodes.push_back(&node);
                return node;
            }
        }

        template<class KeyType, class MappedType>
        inline const MappedType& OrderedMap<KeyType, MappedType>::getNode(const KeyType & key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second; //
        }

        template<class KeyType, class MappedType>
        inline MappedType& OrderedMap<KeyType, MappedType>::getNode(const KeyType & key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second; // 
        }

        template<class KeyType, class MappedType>
        inline const boost::any& OrderedMap<KeyType, MappedType>::getAny(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAsAny();
        }

        template<class KeyType, class MappedType>
        inline boost::any& OrderedMap<KeyType, MappedType>::getAny(const KeyType& key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAsAny();
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline const T & OrderedMap<KeyType, MappedType>::get(const KeyType & key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return get<T > (it); //return it->second.template value<T > (); //
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline T & OrderedMap<KeyType, MappedType>::get(const KeyType & key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return get<T > (it);
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline void OrderedMap<KeyType, MappedType>::get(const KeyType& key, T & value) const {
            value = get<T > (key);
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline const T & OrderedMap<KeyType, MappedType>::get(const const_map_iterator & it) const {
            return it->second.template getValue<const T > ();
        }

        template<class KeyType, class MappedType>
        template<class T>
        inline T & OrderedMap<KeyType, MappedType>::get(const map_iterator & it) {
            return it->second.template getValue<T > ();
        }

        template <class KeyType, class MappedType>
        template <class ValueType>
        inline ValueType OrderedMap<KeyType, MappedType>::getAs(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAs<ValueType >();
        }

        template <class KeyType, class MappedType>
        template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
        inline Cont<T> OrderedMap<KeyType, MappedType>::getAs(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAs<T, Cont >();
        }

        template<class KeyType, class MappedType>
        template <typename T>
        bool OrderedMap<KeyType, MappedType>::is(const KeyType & key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return is<T > (it);
        }

        template<class KeyType, class MappedType>
        template <typename T>
        bool OrderedMap<KeyType, MappedType>::is(const const_map_iterator & it) const {
            return it->second.template is<T > ();
        }

        template<class KeyType, class MappedType>
        bool OrderedMap<KeyType, MappedType>::is(const KeyType& key, const Types::ReferenceType & type) const {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("getTypeAsId(key) == type");
            return true;
        }
    }
}

#endif	/* CONTAINER_HH */
