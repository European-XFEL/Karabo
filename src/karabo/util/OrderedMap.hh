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
 * File:   OrderedMap.hh
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on December 14, 2012, 11:22 AM
 */

#ifndef KARABO_UTIL_ORDERED_MAP_HH
#define KARABO_UTIL_ORDERED_MAP_HH

#include <any>
#include <boost/iterator/indirect_iterator.hpp>
#include <list>
#include <map>

#include "Exception.hh"
#include "Types.hh"

namespace karabo {

    namespace util {

        /**
         * @class OrderedMap
         * @brief a associative key-value container maintaining insertion order
         *
         * The OrderedMap provides for an insertion-order aware key-value container
         * which has similar access, modification and query methods as std::map.
         * The differences are that knowledge of insertion order is maintained,
         * values may be of different and any type and iterator for both key
         * and insertion order are provided.
         */
        template <class KeyType, class MappedType>
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

            /**
             * Return an iterator to the first element of the OrderedMap
             * @return
             */
            iterator begin() {
                return m_listNodes.begin();
            }

            const_iterator begin() const {
                return m_listNodes.begin();
            }

            /**
             * Return an iterator past the last element of the OrderedMap
             * @return
             */
            iterator end() {
                return m_listNodes.end();
            }

            const_iterator end() const {
                return m_listNodes.end();
            }


           public:
            /**
             * Construct an empty OrderedMap
             */
            OrderedMap();

            /**
             * OrderedMap copy constructr
             * @param other
             */
            OrderedMap(const OrderedMap<KeyType, MappedType>& other);

            /**
             * Construct an ordered map with one element in it identified by key
             * @param key1
             * @param value1
             */
            template <typename T>
            OrderedMap(const KeyType& key1, const T& value1);

            /**
             * Construct an ordered map with two elements in it identified by keys
             * @param key1
             * @param value1
             * @param key2
             * @param value2
             */
            template <typename T, typename U>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2);

            /**
             * Construct an ordered map with three elements in it identified by keys
             * @param key1
             * @param value1
             * @param key2
             * @param value2
             * @param key3
             * @param value3
             */
            template <typename T, typename U, typename V>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2, const KeyType& key3,
                       const V& value3);

            /**
             * Construct an ordered map with four elements in it identified by keys
             * @param key1
             * @param value1
             * @param key2
             * @param value2
             * @param key3
             * @param value3
             * @param key4
             * @param value4
             */
            template <typename T, typename U, typename V, typename X>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2, const KeyType& key3,
                       const V& value3, const KeyType& key4, const X& value4);

            /**
             * Construct an ordered map with five elements in it identified by keys
             * @param key1
             * @param value1
             * @param key2
             * @param value2
             * @param key3
             * @param value3
             * @param key4
             * @param value4
             * @param key5
             * @param value5
             */
            template <typename T, typename U, typename V, typename X, typename Y>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2, const KeyType& key3,
                       const V& value3, const KeyType& key4, const X& value4, const KeyType& key5, const Y& value5);

            /**
             * Construct an ordered map with six elements in it identified by keys
             * @param key1
             * @param value1
             * @param key2
             * @param value2
             * @param key3
             * @param value3
             * @param key4
             * @param value4
             * @param key5
             * @param value5
             * @param key6
             * @param value6
             */
            template <typename T, typename U, typename V, typename X, typename Y, typename Z>
            OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2, const U& value2, const KeyType& key3,
                       const V& value3, const KeyType& key4, const X& value4, const KeyType& key5, const Y& value5,
                       const KeyType& key6, const Z& value6);

            // Destructor
            virtual ~OrderedMap();

            /**
             * Assignment operator. Clears this map and then assigns the entries of other to it
             * @param other
             * @return
             */
            OrderedMap<KeyType, MappedType>& operator=(const OrderedMap<KeyType, MappedType>& other);

            /**
             * Move constructor. Moves the entries of other to this map, other is then empty
             * @param other
             * @return
             */
            OrderedMap(
                  OrderedMap<KeyType, MappedType>&&
                        other) noexcept; // noexcept: http://thbecker.net/articles/rvalue_references/section_09.html

            /**
             * Move assignment. Moves the entries of other to this map, other is then empty
             * @param other
             * @return
             */
            OrderedMap<KeyType, MappedType>& operator=(
                  OrderedMap<KeyType, MappedType>&&
                        other) noexcept; // noexcept: http://thbecker.net/articles/rvalue_references/section_09.html

            /**
             * Return an iterator iterating from the first element over elements in insertion order
             * @return
             */
            list_iterator lbegin();

            const_list_iterator lbegin() const;

            /**
             * Return an iterator past the last element in insertion order
             * @return
             */
            list_iterator lend();

            const_list_iterator lend() const;

            /**
             * Return an iterator iterating from the first element over elements in key sorting order
             * @return
             */
            map_iterator mbegin();

            const_map_iterator mbegin() const;

            /**
             * Return an iterator past the last element in key sorting order
             * @return
             */
            map_iterator mend();

            const_map_iterator mend() const;

            /**
             * Return a key-sorted iterator to the element identified by key.
             * Returns OrderedMap::mend if the element is not found
             * @param key
             * @return
             */
            inline map_iterator find(const KeyType& key);

            inline const_map_iterator find(const KeyType& key) const;

            /**
             * Query if the element identified by key exists in the OrderedMap
             * @param key
             * @return
             */
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

            /**
             * Return the number or elements in this map
             * @return
             */
            inline size_t size() const;

            /**
             * Query if this map is empty
             * @return
             */
            inline bool empty() const;

            /**
             * Empty or clear the map
             */
            inline void clear();

            /**
             * Set the element identified by key to value
             * @param key
             * @param value
             * @return
             */
            template <class T>
            inline Node& set(const KeyType& key, const T& value);

            template <class T>
            inline Node& set(const KeyType& key, T& value);

            template <class T>
            inline Node& set(const KeyType& key, T&& value);

            /**
             * Return the element identified by key. Raises an exception if
             * T is not of the type of the inserted element
             * @param key
             * @return
             */
            template <class T>
            inline const T& get(const KeyType& key) const;

            template <class T>
            inline T& get(const KeyType& key);

            /**
             * Fill the reference value with the element identified by key.
             * Raises an exception if T is not of the type of the inserted element
             * @param key
             * @param value
             * @return
             */
            template <class T>
            inline void get(const KeyType& key, T& value) const;

            /**
             * Return the element for the key-associative iterator to the OrderedMap
             *  Raises an exception if T is not of the type of the inserted element
             * @param key
             * @return
             */
            template <class T>
            inline const T& get(const const_map_iterator& it) const;

            template <class T>
            inline T& get(const map_iterator& it);

            /**
             * Return the element identified by key. Raises an exception if
             * T is not of the type of the inserted element or the value
             * cannot be casted to T
             * @param key
             * @return
             */
            template <typename ValueType>
            inline ValueType getAs(const KeyType& key) const;

            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            inline Cont<T> getAs(const KeyType& key) const;

            /**
             * Get the Element identified by key as a Node
             * @param key
             * @return
             */
            inline const Node& getNode(const KeyType& key) const;

            inline Node& getNode(const KeyType& key);

            /**
             * Get the Element identified by key as a std::any value
             * @param key
             * @return
             */
            inline const std::any& getAny(const KeyType& key) const;

            inline std::any& getAny(const KeyType& key);

            /**
             * Check if the element at key is of type T
             * @param key
             * @return
             */
            template <typename T>
            bool is(const KeyType& key) const;

            /**
             * Check if the element the iterator refers to is of type T
             * @param key
             * @return
             */
            template <typename T>
            bool is(const const_map_iterator& it) const;

            /**
             * Check if the element at key is of the given Karabo reference type
             * @param key
             * @return
             */
            bool is(const KeyType& key, const Types::ReferenceType& type) const;
        };
    } // namespace util
} // namespace karabo

namespace karabo {

    namespace util {

        /***********************************************************************
         *
         * Implement the Container class methods
         *
         ***********************************************************************/

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::OrderedMap() {}

        template <class KeyType, class MappedType>
        template <typename T>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1) {
            this->set(key1, value1);
        }

        template <class KeyType, class MappedType>
        template <typename T, typename U>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2,
                                                    const U& value2) {
            this->set(key1, value1);
            this->set(key2, value2);
        }

        template <class KeyType, class MappedType>
        template <typename T, typename U, typename V>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2,
                                                    const U& value2, const KeyType& key3, const V& value3) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
        }

        template <class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2,
                                                    const U& value2, const KeyType& key3, const V& value3,
                                                    const KeyType& key4, const X& value4) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
        }

        template <class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X, typename Y>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2,
                                                    const U& value2, const KeyType& key3, const V& value3,
                                                    const KeyType& key4, const X& value4, const KeyType& key5,
                                                    const Y& value5) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
            this->set(key5, value5);
        }

        template <class KeyType, class MappedType>
        template <typename T, typename U, typename V, typename X, typename Y, typename Z>
        OrderedMap<KeyType, MappedType>::OrderedMap(const KeyType& key1, const T& value1, const KeyType& key2,
                                                    const U& value2, const KeyType& key3, const V& value3,
                                                    const KeyType& key4, const X& value4, const KeyType& key5,
                                                    const Y& value5, const KeyType& key6, const Z& value6) {
            this->set(key1, value1);
            this->set(key2, value2);
            this->set(key3, value3);
            this->set(key4, value4);
            this->set(key5, value5);
            this->set(key6, value6);
        }

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::OrderedMap(const OrderedMap<KeyType, MappedType>& other) {
            *this = other;
        }

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::~OrderedMap() {}

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>& OrderedMap<KeyType, MappedType>::operator=(
              const OrderedMap<KeyType, MappedType>& other) {
            if (this != &other) {
                this->m_listNodes.clear();
                this->m_mapNodes.clear();

                if (!other.empty()) {
                    this->m_mapNodes = other.m_mapNodes;
                    // Original order in "other" should be preserved
                    for (typename ListType::const_iterator it = other.m_listNodes.begin();
                         it != other.m_listNodes.end(); ++it) {
                        this->m_listNodes.push_back(&(find((*it)->getKey())->second));
                    }
                }
            }
            return *this;
        }

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>::OrderedMap(OrderedMap<KeyType, MappedType>&& other) noexcept
            : m_listNodes(std::move(other.m_listNodes)), m_mapNodes(std::move(other.m_mapNodes)) {
            other.m_listNodes.clear();
            other.m_mapNodes.clear();
        }

        template <class KeyType, class MappedType>
        OrderedMap<KeyType, MappedType>& OrderedMap<KeyType, MappedType>::operator=(
              OrderedMap<KeyType, MappedType>&& other) noexcept {
            if (this != &other) {
                m_listNodes.clear();
                m_mapNodes.clear();
                m_listNodes = std::move(other.m_listNodes);
                m_mapNodes = std::move(other.m_mapNodes);
                other.m_listNodes.clear();
                other.m_mapNodes.clear();
            }
            return *this;
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::list_iterator OrderedMap<KeyType, MappedType>::lbegin() {
            return m_listNodes.begin();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_list_iterator OrderedMap<KeyType, MappedType>::lbegin()
              const {
            return m_listNodes.begin();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::list_iterator OrderedMap<KeyType, MappedType>::lend() {
            return m_listNodes.end();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_list_iterator OrderedMap<KeyType, MappedType>::lend()
              const {
            return m_listNodes.end();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::mbegin() {
            return m_mapNodes.begin();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::mbegin()
              const {
            return m_mapNodes.begin();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::mend() {
            return m_mapNodes.end();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::mend()
              const {
            return m_mapNodes.end();
        }

        template <class KeyType, class MappedType>
        inline size_t OrderedMap<KeyType, MappedType>::size() const {
            return m_mapNodes.size();
        }

        template <class KeyType, class MappedType>
        inline bool OrderedMap<KeyType, MappedType>::empty() const {
            return m_mapNodes.empty();
        }

        template <class KeyType, class MappedType>
        inline void OrderedMap<KeyType, MappedType>::clear() {
            m_mapNodes.clear();
            m_listNodes.clear();
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::map_iterator OrderedMap<KeyType, MappedType>::find(
              const KeyType& key) {
            return m_mapNodes.find(/*hash*/ (key));
        }

        template <class KeyType, class MappedType>
        inline typename OrderedMap<KeyType, MappedType>::const_map_iterator OrderedMap<KeyType, MappedType>::find(
              const KeyType& key) const {
            return m_mapNodes.find(/*hash*/ (key));
        }

        template <class KeyType, class MappedType>
        inline bool OrderedMap<KeyType, MappedType>::has(const KeyType& key) const {
            return find(key) != m_mapNodes.end();
        }

        template <class KeyType, class MappedType>
        inline size_t OrderedMap<KeyType, MappedType>::erase(const KeyType& key) {
            map_iterator it;

            if ((it = find(key)) != m_mapNodes.end()) {
                m_listNodes.remove(&it->second);
                m_mapNodes.erase(it);
                return 1;
            }
            return 0;
        }

        template <class KeyType, class MappedType>
        inline void OrderedMap<KeyType, MappedType>::erase(const map_iterator& it) {
            // it must be valid!
            m_listNodes.remove(&it->second);
            m_mapNodes.erase(it);
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline MappedType& OrderedMap<KeyType, MappedType>::set(const KeyType& key, const T& value) {
            // Take care - any code change is likely to be done to the overload with 'T&& value' argument as well.
            MappedType* nodePtr = nullptr;
            map_iterator it = find(key);
            if (it != m_mapNodes.end()) {
                nodePtr = &(it->second);
            } else {
                nodePtr = &(m_mapNodes[/*hash*/ (key)]);
                nodePtr->setKey(key);
                m_listNodes.push_back(nodePtr);
            }
            nodePtr->setValue(value);
            return *nodePtr;
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline MappedType& OrderedMap<KeyType, MappedType>::set(const KeyType& key, T& value) {
            // This is an overload for T& to avoid to take the set(const KeyType& key, T&& value) code path
            // as seems to be needed to catch the correct overlaod for MappedType::setValue(MappedType&)
            return set(key, const_cast<const T&>(value));
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline MappedType& OrderedMap<KeyType, MappedType>::set(const KeyType& key, T&& value) {
            // Take care - any code change is likely to be done to the overload with 'const T& value' argument as well.
            MappedType* nodePtr = nullptr;
            map_iterator it = find(key);
            if (it != m_mapNodes.end()) {
                nodePtr = &(it->second);
            } else {
                nodePtr = &(m_mapNodes[/*hash*/ (key)]);
                nodePtr->setKey(key);
                m_listNodes.push_back(nodePtr);
            }
            nodePtr->setValue(std::forward<T>(value));
            return *nodePtr;
        }

        template <class KeyType, class MappedType>
        inline const MappedType& OrderedMap<KeyType, MappedType>::getNode(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second; //
        }

        template <class KeyType, class MappedType>
        inline MappedType& OrderedMap<KeyType, MappedType>::getNode(const KeyType& key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second; //
        }

        template <class KeyType, class MappedType>
        inline const std::any& OrderedMap<KeyType, MappedType>::getAny(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.getValueAsAny();
        }

        template <class KeyType, class MappedType>
        inline std::any& OrderedMap<KeyType, MappedType>::getAny(const KeyType& key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.getValueAsAny();
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline const T& OrderedMap<KeyType, MappedType>::get(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return get<T>(it); // return it->second.template value<T > (); //
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline T& OrderedMap<KeyType, MappedType>::get(const KeyType& key) {
            map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return get<T>(it);
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline void OrderedMap<KeyType, MappedType>::get(const KeyType& key, T& value) const {
            value = get<T>(key);
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline const T& OrderedMap<KeyType, MappedType>::get(const const_map_iterator& it) const {
            return it->second.template getValue<const T>();
        }

        template <class KeyType, class MappedType>
        template <class T>
        inline T& OrderedMap<KeyType, MappedType>::get(const map_iterator& it) {
            return it->second.template getValue<T>();
        }

        template <class KeyType, class MappedType>
        template <class ValueType>
        inline ValueType OrderedMap<KeyType, MappedType>::getAs(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAs<ValueType>();
        }

        template <class KeyType, class MappedType>
        template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
        inline Cont<T> OrderedMap<KeyType, MappedType>::getAs(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return it->second.template getValueAs<T, Cont>();
        }

        template <class KeyType, class MappedType>
        template <typename T>
        bool OrderedMap<KeyType, MappedType>::is(const KeyType& key) const {
            const_map_iterator it;
            if ((it = find(key)) == m_mapNodes.end()) {
                throw KARABO_PARAMETER_EXCEPTION("Key '" + key + "' does not exist");
            }
            return is<T>(it);
        }

        template <class KeyType, class MappedType>
        template <typename T>
        bool OrderedMap<KeyType, MappedType>::is(const const_map_iterator& it) const {
            return it->second.template is<T>();
        }

        template <class KeyType, class MappedType>
        bool OrderedMap<KeyType, MappedType>::is(const KeyType& key, const Types::ReferenceType& type) const {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("getTypeAsId(key) == type");
            return true;
        }
    } // namespace util
} // namespace karabo

#endif /* CONTAINER_HH */
