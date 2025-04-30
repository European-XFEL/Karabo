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
 * File:   Hash.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on December 14, 2012, 11:19 AM
 */

#ifndef KARABO_DATA_TYPES_HASH_HH
#define KARABO_DATA_TYPES_HASH_HH

// clang-format off

// Build is dependent on order of inclusion of the headers. At least for now,
// protect the order by creating a clang-format free section.

#include <iostream>
#include <sstream>
#include <string>

#include <set>
#include <vector>
#include <list>

#include <tuple>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>

#include "ClassInfo.hh"
#include "StringTools.hh"
#include "Types.hh"
#include "Element.hh"
#include "OrderedMap.hh"
#include "Exception.hh"

#include "karaboDll.hh"

// clang-format on

namespace karabo {

    namespace data {

        /**
         * @class Hash
         * @brief A generic key value container that supports ordering and attributes.
         *
         * Hash container:
         * - The Hash is a heterogeneous generic key/value container that associates a string key to a value of any
         * type.<br>
         * - The Hash is a core data structure in Karabo software framework, and is widely used in the karabo
         * system.<br>
         * - For instance, exchanging data and configurations between two or more entities (devices, GUI),
         * database interface (store and retrieval ), meta-data handling, etc.<br>
         * - The Hash class is much like a XML-DOM container with the difference of
         * allowing only unique keys on a given tree-level.<br>
         * - Like and XML DOM object, the Hash provides a multi-level (recursive) key-value associative container,
         * where keys are strings and values can be of any C++ type.<br>
         *
         * Concept:
         * - Provide recursive key-value associative container (keys are strings and unique, values can be of any
         * type)<br>
         * - Preserve insertion order, while optimized for random key-based lookup. Different iterators are available
         * for each use case.<br>
         * - Like in XML, each hash key can have a list of (key-value) attributes (attribute keys are strings and
         * unique, attribute values can be of any type).<br>
         * - Seamless serialization to/from XML, Binary, HDF5, etc.<br>
         * - Usage: configuration, device-state cache, database interface (result-set), message protocol, meta-data
         * handling, etc.<br>
         * - Templated set, get for retrieving values from keys. Assumes recursursion on "." characters in key by
         * default. Seperator can be specified per function call. <br>
         * - Exposed iterators will a sequential iterator (insertion order) and a alpha-numeric order iterator.<br>
         * - Each iterator provides access to its key, value, and attributes in form of a Hash::Node and can thus be
         * used for recursive traversal.<br>
         * - Insertion of a non-existing key leads to new entry whilst insertion of an existing key will only update
         * (merge) the corresponding value/attributes.<br>
         * - Additional functionality include: list of paths, clear/erase, find, merge, comparison, etc.
         */
        class KARABO_DECLSPEC Hash {
           public:
            KARABO_CLASSINFO(Hash, "Hash", "2.0")

            typedef OrderedMap<std::string, Element<std::string> > Attributes;
            typedef Element<std::string, Attributes> Node;

            enum MergePolicy {

                MERGE_ATTRIBUTES,
                REPLACE_ATTRIBUTES
            };

           private:
            typedef OrderedMap<std::string, Node> Container;
            Container m_container;

           public:
            typedef Container::iterator iterator;
            typedef Container::const_iterator const_iterator;

            typedef Container::map_iterator map_iterator;
            typedef Container::const_map_iterator const_map_iterator;

            static const char k_defaultSep;

            /**
             * Insertion order iterator (i.e. list iterator)
             */
            const_iterator begin() const;
            iterator begin();

            const_iterator end() const;
            iterator end();

            /**
             * Alpha-numeric order iterator (i.e. map iterator)
             */
            const_map_iterator mbegin() const;
            map_iterator mbegin();

            const_map_iterator mend() const;
            map_iterator mend();

            /**
             * Lookup for the hash element identified by "path". If the node exists,
             * it returns a reference to it wrapped in boost::optional object.
             * Otherwise, uninitialized boost::optional object is returned.
             * @param path sequence of keys to the searched for element
             * @param separator key separation char
             * @return Hash::Node wrapped in boost::optional
             */
            boost::optional<const Hash::Node&> find(const std::string& path, const char separator = k_defaultSep) const;
            boost::optional<Hash::Node&> find(const std::string& path, const char separator = k_defaultSep);

            /**
             * Default constructor creates an empty hash
             *
             * <b>Example:</b>
             * @code
             * Hash hash;
             * @endcode
             */
            Hash();

            /**
             * Use this constructor to create a hash with one key/value pair where value is just an empty hash
             * @param key Name of empty child
             *
             * <b>Example:</b>
             * The code like this ...
             * @code
             * Hash hash(myKey);
             * @endcode
             * ... is equivalent to ...
             * @code
             * Hash hash(myKey, Hash());
             * @endcode
             */
            explicit Hash(const std::string& path);

            /**
             * Constructor for different numbers of key-value pairs as arguments
             * Create and initialize a hash with multiple key-value pairs in a single call,
             * supporting also move semantics
             *
             * @param key1 A string hash key
             * @param value1 Any object as (r- or l-)value
             * @param key2 Optionally another key
             * @param value2 Optionally another object as (r- or l-)value
             * ...
             * <b>Example:</b>
             * @code
             * Hash hash("firstKey", 1, "secondKey", 2, "thirdKey", 3, "fourthKey", 4, "fifthKey", 5, "sixthKey", 6);
             * @endcode
             */
            template <typename V1, typename... Args>
            Hash(const std::string& path1, V1&& value1, Args&&... args);

            /**
             * Destructor
             */
            virtual ~Hash();

            /**
             * Copy constructor
             */
            Hash(const Hash& other) = default;

            /**
             * Assignment of "lvalue"
             */
            Hash& operator=(const Hash& other) = default;

            /**
             * Move constructor
             */
            Hash(Hash&& other) noexcept =
                  default; // noexcept: http://thbecker.net/articles/rvalue_references/section_09.html

            /**
             * Assignment of "rvalue"
             */
            Hash& operator=(Hash&& other) noexcept =
                  default; // noexcept: http://thbecker.net/articles/rvalue_references/section_09.html

            /**
             * Merge the current hash with another one
             * @param other Hash object
             */
            Hash& operator+=(const Hash& other);

            /**
             * Subtract another hash from the current one
             * @param other Hash object
             */
            Hash& operator-=(const Hash& other);

            /*******************************************************************
             * Provide same interface as STL
             *******************************************************************/
            /**
             * Return the number of key elements in the hash
             * @return size_t
             */
            size_t size() const;

            /**
             * Check if the hash contains any keys or not.
             * @return <b>true</b> if hash is empty, <b>false</b> otherwise
             */
            bool empty() const;

            /**
             * Remove all the keys from the hash
             * @return No
             */
            void clear();

            /**
             * Remove the element identified by 'path' if it exists.
             * Otherwise, do nothing.
             * If 'path' is a composite element, all its descendents are removed
             * as well. The path up to the last part of 'path' is not removed.
             * Example 1: erase ("a.b.c") will remove "c", but "a.b" should
             *            not be removed even if "c" is the only child of "a.b".
             * If 'path' refers to a Hash inside a vector<Hash>, that element of
             * the vector is erased, i.e. the vector shrinks.
             * Example 2: erase ("a.b[1]") will remove element 1 of "b". If "b"
             *            had a size above 2, the old element "b[2]" will now be
             *            referred to as "b[1]".
             * @return true if key exists, otherwise false
             */
            bool erase(const std::string& path, const char separator = k_defaultSep);

            /**
             * Remove element identified by map_iterator of this Hash.
             *
             * The iterator must not belong to a Hash nested in this one.
             *
             * @param it a valid iterator (invalid afterwards)
             *
             * @return a map_iterator pointing to the element after 'it' (or mend() if it was the last element)
             */
            map_iterator erase(map_iterator it);

            /**
             * Remove the element identified by 'path' if it exists.
             * If 'key' is composite (e.g. "a.b.c") and the last component ("c")
             * is the only child of its parent, the parent is removed as well.
             * This removal is recursively continued to the root.
             * Example 1: erasePath("a.b.c") will remove "c" and "b" will
             *            be removed as well if "c" is the only child of "a.b".
             *            If "a.b.c" is the only element in the Hash, then
             *            erasePath("a.b.c") will result in an empty hash.
             * If 'path' refers to a Hash inside a vector<Hash>, that element of
             * the vector is erased, i.e. the vector shrinks. If the element was
             * the only one in the vector, the vector will be removed as well
             * Example 2: erase ("a.b[0]") will remove element 0 of "b". If "b"
             *            had a size of 1, b will be removed completely (and 'a'
             *            as well in case of no other child).
             * @return No
             */
            void erasePath(const std::string& path, const char separator = k_defaultSep);

            /**
             * Returns all first level keys of the hash in the provided container (vector, list, ...)
             *
             * No recursion, i.e. keys in inner hashes are not included
             *
             * @param container that the keys are appended to (it is not cleared before)
             */
            template <template <class T, class All = std::allocator<T> > class container>
            void getKeys(container<std::string>& result) const;

            /**
             * Returns all first level keys of the hash in the provided set
             *
             * No recursion, i.e. keys in inner hashes are not included
             *
             * @param result that the keys are inserted into (it is not cleared before!))
             */
            void getKeys(std::set<std::string>& result) const;

            /**
             * Returns all first level keys of the hash as a vector<string>
             *
             * No recursion, i.e. keys in inner hashes are not included
             *
             * @return first level keys
             */
            std::vector<std::string> getKeys() const;

            /**
             * Returns all root-to-leaves paths in hash in the provided container (vector, list, ...)
             *
             * This function goes recursively through the whole hash and the returned paths are their keys
             * "glued" together via the given separator
             *
             * @param result container to which paths get appended (it is not cleared before)
             * @param separator to glue keys of the hierarchy levels
             */
            template <template <class T, class All = std::allocator<T> > class container>
            void getPaths(container<std::string>& result, const char separator = k_defaultSep) const;

            /**
             * Returns all root-to-leaves paths in hash in the provided set
             *
             * This function goes recursively through the whole hash and the returned paths are their keys
             * "glued" together via the given separator
             *
             * @param result set into which paths get inserted (it is not cleared before)
             * @param separator to glue keys of the hierarchy levels
             */
            void getPaths(std::set<std::string>& result, const char separator = k_defaultSep) const;

            /**
             * Returns all root-to-leaves paths in hash as vector<string>
             *
             * This function goes recursively through the whole hash and the returned paths are their keys
             * "glued" together via the given separator
             *
             * @param separator to glue keys of the hierarchy levels
             * @return vector of all paths
             */
            std::vector<std::string> getPaths(const char separator = k_defaultSep) const;

            /**
             * Returns all root-to-leaves paths in hash in the provided container (vector, list, ...)
             *
             * In contrast to getPaths, inserted data of C++ objects with protected inheritance from Hash
             * (NDarray, ImageData) ar not treated as atomic, but as another Hash level
             *
             * @param result container to which paths get appended (it is not cleared before)
             * @param separator to glue keys of the hierarchy levels
             */
            template <template <class T, class All = std::allocator<T> > class container>
            void getDeepPaths(container<std::string>& result, const char separator = k_defaultSep) const;

            /**
             * Returns all root-to-leaves paths in hash in the provided set
             *
             * In contrast to getPaths, inserted data of C++ objects with protected inheritance from Hash
             * (NDarray, ImageData) ar not treated as atomic, but as another Hash level
             *
             * @param result container to which paths get appended (it is not cleared before)
             * @param separator to glue keys of the hierarchy levels
             */
            void getDeepPaths(std::set<std::string>& result, const char separator = k_defaultSep) const;

            /**
             * Returns all root-to-leaves paths in hash in the provided set
             *
             * In contrast to getPaths, inserted data of C++ objects with protected inheritance from Hash
             * (NDarray, ImageData) ar not treated as atomic, but as another Hash level
             *
             * @param separator to glue keys of the hierarchy levels
             * @return vector of all deep paths
             */
            std::vector<std::string> getDeepPaths(const char separator = k_defaultSep) const;

            static void getPaths(const Hash& hash, std::vector<std::string>& paths, std::string prefix,
                                 const char separator = k_defaultSep, const bool fullPaths = false);

            /**
             * Check if the key 'key' exist in hash
             * @param key A string hash key
             * @return <b>true</b> if key exists, <b>false</b> otherwise
             */
            bool has(const std::string& path, const char separator = k_defaultSep) const;


            // Looks like the 'const ValueType&' version has to stay to ensure that explicit type specifications like
            //    std::string value("v");
            //    aHash.set<std::string>(path, value);
            // still compile. Just
            //    aHash.set(path, value);
            // seems to work correctly even with only the 'ValueType&&' version.
            /**
             * @brief Insert key/value pair in current container.
             *
             * @param key A string key
             * @param value Any object as value
             * @return void
             *
             * @example
             * @code
             * Hash hash;
             * hash.set("myKey", "myValue");
             * hash.set("myFloat", float(12.44));
             * @endcode
             *
             * Optimization: to avoid double-copy, ie *value* into <i>std::any</i> object, and the later
             * into the map, we first insert an empty object of type <b>std::any</b> into the map, get a reference to
             * the <i>std::any</i> object associated with <b>key</b>, then copy the given value into it.
             */
            template <typename ValueType>
            inline Node& set(const std::string& path, const ValueType& value, const char separator = k_defaultSep);

            template <typename ValueType>
            inline Node& set(const std::string& path, ValueType&& value, const char separator = k_defaultSep);

            /**
             * Set an arbitray number of key/value pairs, internally using Hash::set(..) with the default separator
             */
            template <typename ValueType, typename... Args>
            inline void setMulti(const std::string& key, ValueType&& value, Args&&... args);

            /**
             * Clone the content (key, value, attributes) of another elements.
             * This function use the source element's key, NOT his full path.
             */
            Node& setNode(const Node& srcElement);

            Node& setNode(const_iterator srcIterator);

            /**
             * Bind a (newly created) object in the map into and external variable
             * Optimization: This is useful in order to avoid the later copy of the value into <b>std::any</b>, in the
             * hash::set(key, value). This function provides a reference to the object in the map where you can build
             * your data directly.
             * @param key A string hash key
             * @return A <b>reference</b> to the internal object
             *
             * @example
             * @code
             *  vector<string>& vec = set<vector<string> >("key");
             *  vec.resize(20);
             *  vec[1] = "a string";
             * @endcode
             */
            template <typename ValueType>
            ValueType& bindReference(const std::string& path, const char separator = k_defaultSep);

            /**
             * Bind a (newly created) object in the map into and external variable
             * Optimization: This is useful in order to avoid the later copy of the value into <b>std::any</b>, in the
             * hash::set(key, value). This function provides a pointer to the object in the map where you can build your
             * data directly.
             * @param key A string hash key
             * @return A <b>pointer</b> to the internal object
             * <b>Example:<b>
             * @code
             *  int* value = set<int>("key");
             *  *value = 10;
             * @endcode
             */
            template <typename ValueType>
            ValueType* bindPointer(const std::string& path, const char separator = k_defaultSep);

            /**
             * Retrieve a constant reference to the value of element identified by 'key'
             * @param key A string key
             * @return The associated value
             *
             * @note Differently from Hash::getAs, Hash::get (and its overloads) doesn't perform any
             * conversion while retrieving a node value. If the node type specified on the get call
             * doesn't match the actual node type, a cast exception will be thrown.
             */
            template <typename ValueType>
            inline const ValueType& get(const std::string& path, const char separator = k_defaultSep) const;

            /**
             * Retrieve a (non-const) reference to the stored value of a given key
             * @param key A string key
             * @return The associated value
             */
            template <typename ValueType>
            inline ValueType& get(const std::string& key, const char separator = k_defaultSep);

            /**
             * Retrieve (non-const) reference to the stored value into second parameter
             * @param key A string key
             * @param value Any object that will be filled by reference
             * @return void
             */
            template <typename ValueType>
            inline void get(const std::string& path, ValueType& value, const char separator = k_defaultSep) const;

            /**
             * Casts the the value of element identified by "path" from its original type
             * to another different target type.
             * Throws CastException if casting fails, i.e. not possible or unsafe
             * @param path
             * @param separator
             * @return value
             *
             * @note Hash::getAs (and its overload for vector elements) converts the node values to the
             * target type specified by the caller. As an example, retrieval of unsigned integer values
             * from nodes that are signed integers will perform the same implicit conversion performed
             * by C/C++ (e.g., -1 assigned to an INT32 key will be retrieved as 4.294.967.295 when
             * requested as an UINT32).
             */
            template <typename ValueType>
            inline ValueType getAs(const std::string& path, const char separator = k_defaultSep) const;

            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            inline Cont<T> getAs(const std::string& key, const char separator = k_defaultSep) const;

            /**
             * Return the internal Hash node element designated by "path"
             * @param path
             * @param separator
             * @return Hash::Node element
             */
            const Node& getNode(const std::string& path, const char separator = k_defaultSep) const;

            Node& getNode(const std::string& path, const char separator = k_defaultSep);

            /**
             * Predicate function calculating if the type of the value associated with the <b>key</b> is
             * of a specific type in template parameter
             * @param key The key having associated value and the type of this value we want to test against template
             * parameter
             * @return <b>true</b> or <b>false</b>
             */
            template <typename ValueType>
            bool is(const std::string& path, const char separator = k_defaultSep) const;

            /**
             * Predicate function calculating if the value associated with <b>key</b> is of type <b>type</b>.
             * @param key Some string
             * @param type Some type from Types::Type enumeration
             * @return <b>true</b> or <b>false</b>
             */
            bool is(const std::string& path, const Types::ReferenceType& type,
                    const char separator = k_defaultSep) const;

            /**
             * Function to obtain value type information
             * @param key The key to the value of which type information should be returned
             * @return The typeId as defined in @see Types.hh
             */
            Types::ReferenceType getType(const std::string& path, const char separator = k_defaultSep) const;

            /** Merges another hash into this one
             * Creates new nodes, if they do not already exists. Creates new leaves, if they do not already exist.
             * Existing leaves will be replaced by the new hash.
             * @param hash Another hash to be merged into current hash
             * @param policy Whether to replace attributes by those merged in or to merge them
             * @param selectedPaths If not empty, only merge these paths
             * @param separator The separator for nested keys in selectedPaths
             */
            void merge(const Hash& other, const MergePolicy policy = REPLACE_ATTRIBUTES,
                       const std::set<std::string>& selectedPaths = std::set<std::string>(),
                       char separator = k_defaultSep);

            /**
             * Subtracts from current hash all nodes that can be found in other hash given as argument.
             * @param other hash used for defining the candidates for subtraction
             * @param separator.  The default separator is k_defaultSep
             * @return the current hash is shrinked in place, the other hash is untouched
             */
            void subtract(const Hash& other, const char separator = k_defaultSep);

            /**
             * Flattens a hierarchical Hash into "one-level", <i>flat</i> Hash object
             * @return A Hash having keys that are all leaves
             */
            void flatten(Hash& flat, const char separator = k_defaultSep) const;

            static void flatten(const Hash& hash, Hash& flat, std::string prefix = "",
                                const char separator = k_defaultSep);

            /**
             * Arranges <i>flat</i> Hash object in a hierarchy using
             * separator symbol (default: ".") during parsing the keys to recognize a hierarchy
             * @param sep An optional separator symbol (default: ".")
             * @return A Hash object containing keys with no separator symbols
             */
            void unflatten(Hash& tree, const char separator = k_defaultSep) const;

            /*******************************************************************
             * Attributes manipulation
             *******************************************************************/

            /**
             * Check if the element identified by "path" has an attribute called "attribute"
             * @param path
             * @param attribute
             * @param separator
             * @return bool
             */
            bool hasAttribute(const std::string& path, const std::string& attribute,
                              const char separator = k_defaultSep) const;

            /**
             * Return the value of the attribute called "attribute" of the element identified by "path"
             * @param path
             * @param attribute
             * @param separator
             * @return bool
             */
            template <typename ValueType>
            const ValueType& getAttribute(const std::string& path, const std::string& attribute,
                                          const char separator = k_defaultSep) const;

            template <typename ValueType>
            ValueType& getAttribute(const std::string& path, const std::string& attribute,
                                    const char separator = k_defaultSep);

            /**
             * Casts the value of the attribute called "attribute" of the element identified
             * by "path" from its original type to another different target type.
             * Throws CastException if casting fails, i.e. not posible or unsafe
             * @param path
             * @param attribute
             * @param separator
             * @return value
             */
            template <typename T>
            T getAttributeAs(const std::string& path, const std::string& attribute,
                             const char separator = k_defaultSep) const;

            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            Cont<T> getAttributeAs(const std::string& path, const std::string& attribute,
                                   const char separator = k_defaultSep) const;

            /**
             * Return the value of the attribute called "attribute" of the element identified as std::any.
             * @param path
             * @param attribute
             * @param separator
             * @return std::any
             */
            std::any& getAttributeAsAny(const std::string& path, const std::string& attribute,
                                        const char separator = k_defaultSep);

            const std::any& getAttributeAsAny(const std::string& path, const std::string& attribute,
                                              const char separator = k_defaultSep) const;

            /**
             * Return the list of attributes of the element identified by "path"
             * @param path
             * @param separator
             * @return Hash::Attributes
             */
            const Attributes& getAttributes(const std::string& path, const char separator = k_defaultSep) const;
            Attributes& getAttributes(const std::string& path, const char separator = k_defaultSep);

            /**
             * Set the value of an attribute called "attribute" of the element identified by "path"
             * @param path
             * @param attribute
             * @param value
             * @param separator
             */
            template <typename ValueType>
            void setAttribute(const std::string& path, const std::string& attribute, const ValueType& value,
                              const char separator = k_defaultSep);

            /**
             * Set the value of an attribute called "attribute" of the element identified by "path"
             * @param path
             * @param attribute
             * @param value
             * @param separator
             */
            template <typename ValueType>
            void setAttribute(const std::string& path, const std::string& attribute, ValueType&& value,
                              const char separator = k_defaultSep);

            /**
             * Assign of list of attributes (i.e. Hash::Attributes container) to the element identified by "path"
             * @param path
             * @param attributes
             * @param separator
             */
            void setAttributes(const std::string& path, const Attributes& attributes,
                               const char separator = k_defaultSep);

            /**
             * Assign of list of attributes (i.e. Hash::Attributes container) to the element identified by "path"
             * @param path
             * @param attributes to move from
             * @param separator
             */
            void setAttributes(const std::string& path, Attributes&& attributes, const char separator = k_defaultSep);

            /**
             * Serialize a hash to standard std::ostream object
             * @param visitor
             */
            friend std::ostream& operator<<(std::ostream& os, const Hash& hash);

            /**
             * Checks if this Hash is similar to other.
             *
             * A Hash is considered similar to another if both have
             * the same number of elements, of the same type and in
             * the same order.
             *
             * @param other the Hash this Hash will be compared to.
             * @return true if both hashes are similar.
             *
             * @note: this is not the full equality operator as the
             * values of the elements are not considered.
             */
            bool operator==(const Hash& other) const;

            /**
             * Checks if this Hash is equal to other.
             *
             * A Hash is considered fully equal to another if both
             * are similar and their corresponding elements have
             * the same keys, values and attributes.
             * @param other Hash to compare with
             * @param orderMatters if true (default) order of keys is relevant as well
             * @return bool
             */

            bool fullyEquals(const Hash& other, bool orderMatters = true) const;

            bool operator!=(const Hash& other) const;

            /**
             * Implement the visitor pattern
             * @param visitor
             * @return bool
             */
            template <class Visitor>
            bool visit(Visitor& visitor);

            template <class Visitor>
            bool visit2(Visitor& visitor);

           private:
            template <class Visitor>
            static bool visit(karabo::data::Hash& hash, Visitor& visitor);

            template <class Visitor>
            static bool visit(karabo::data::Hash::Node& node, Visitor& visitor);

            template <class Visitor>
            static bool visit2(karabo::data::Hash& hash, Visitor& visitor);

            template <class Visitor>
            static bool visit2(karabo::data::Hash::Node& node, Visitor& visitor);

            /**
             * Internal helper to avoid code duplication for template specialisations of set(path, hashValue,
             * separator).
             *
             * HashType shall be const reference or lvalue reference of Hash or Hash derived classes: 'const Hash&',
             * 'Hash&&', 'const NDArray&', etc.
             */
            template <typename HashType>
            inline Node& setHash(const std::string& path, HashType value, const char separator = k_defaultSep);

            /**
             * End point for setMulti(..) with variadic templates
             */
            inline void setMulti() const {}

            /**
             *  Out of 'paths' select those that belong to child with 'childKey',
             * e.g. out of ["a", "b.c", "b.d.e"] return ["c", "d.e"] if childKey == "b" and separator == k_defaultSep.
             */
            static std::set<std::string> selectChildPaths(const std::set<std::string>& paths,
                                                          const std::string& childKey, char separator);

            /**
             * True if the first key (separated by 'separator') of any of 'paths' matches 'key'.
             * A first key that contains an index also matches (indirectly) 'key' without index if index < 'size',
             * i.e. path "a[0].g" matches key "a" if 'size' >= 1, but not if 'size' == 0.
             */
            static bool keyIsPrefixOfAnyPath(const std::set<std::string>& paths, const std::string& key, char separator,
                                             unsigned int size);

            /**
             * For all 'paths', check whether their first key matches 'key' (as in keyIsPrefixOfAnyPath).
             * If it does indirectly (see keyIsPrefixOfAnyPath), append the index specified behind it to the result,
             * except if there is also a direct match - then the result is empty:
             * Paths = {a[0], b, a[2]} and key = a ==> return [0,2]
             * Paths = {a[0], b, a}    and key = a ==> return []
             * Indices >= 'targetSize' are ignored.
             */
            static std::set<unsigned int> selectIndicesOfKey(unsigned int targetSize,
                                                             const std::set<std::string>& paths, const std::string& key,
                                                             char separator);

            /**
             *  Merge 'attrs' to 'targetNode' according to merge 'policy'.
             */
            static void mergeAttributes(Hash::Node& targetNode, const Hash::Attributes& attrs,
                                        Hash::MergePolicy policy);

            /**
             * Merge two vector<Hash> nodes that represent table elements, i.e. the content of 'source' replaces
             * the content of 'target'. The 'selectedPaths' with their 'separator' are respected.
             * Note that the 'selectedPaths' are those that selected 'source' for merging, i.e. begin with
             * the key of 'source', possibly suffixed by indices.
             */
            static void mergeTableElement(const Hash::Node& source, Hash::Node& target,
                                          const std::set<std::string>& selectedPaths, char separator);

            Hash* setNodesAsNeeded(const std::vector<std::string>& tokens, char seperator);

            Hash& getLastHash(const std::string& path, std::string& last_key, const char separator = k_defaultSep);
            const Hash& getLastHash(const std::string& path, std::string& last_key,
                                    const char separator = k_defaultSep) const;

            Hash* getLastHashPtr(const std::string& path, std::string& last_key, const char separator = k_defaultSep);
            const Hash* getLastHashPtr(const std::string& path, std::string& last_key,
                                       const char separator = k_defaultSep) const;

            const Hash& thisAsConst() const {
                return const_cast<const Hash&>(*this);
            }

            void toStream(std::ostream& os, const Hash& hash, int depth) const;
        };
    } // namespace data
} // namespace karabo

namespace karabo {

    namespace data {

        template <typename V1, typename... Args>
        Hash::Hash(const std::string& path1, V1&& value1, Args&&... args) : Hash() {
            set(path1, std::forward<V1>(value1));
            setMulti(std::forward<Args>(args)...);
        }

        template <>
        inline const std::any& Hash::get(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAsAny();
        }

        template <>
        inline const Hash& Hash::get(const std::string& path, const char separator) const {
            // TODO: To reduce code in header, move implementation to getHash or sth. like that...
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            int index = karabo::data::getAndCropIndex(key);
            if (index == -1) {
                return hash.m_container.get<Hash>(key);
            } else {
                const std::vector<Hash>& hashVec = hash.m_container.get<std::vector<Hash> >(key);
                if (static_cast<unsigned int>(index) >= hashVec.size()) {
                    throw KARABO_PARAMETER_EXCEPTION("Index " + toString(index) + " out of range in '" + path + "'.");
                }
                return hashVec[index];
            }
        }

        template <>
        inline Hash& Hash::get(const std::string& path, const char separator) {
            return const_cast<Hash&>(thisAsConst().get<Hash>(path, separator));
        }

        template <>
        inline std::any& Hash::get(const std::string& path, const char separator) {
            return getNode(path, separator).getValueAsAny();
        }

        template <typename ValueType>
        inline Hash::Node& Hash::set(const std::string& path, const ValueType& value, const char separator) {
            // Be aware of code duplication with 'ValueType&& value' overload...
            std::vector<std::string> tokens;
            karabo::data::tokenize(path, tokens, separator);

            Hash* leaf = this->setNodesAsNeeded(tokens, separator);

            // Set last token
            std::string& token = tokens.back();
            int index = karabo::data::getAndCropIndex(token);
            if (index == -1) // No vector
                return leaf->m_container.set(token, value);
            else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Only Hash objects may be assigned to a leaf node of array type");
            }
        }

        template <typename ValueType>
        inline Hash::Node& Hash::set(const std::string& path, ValueType&& value, const char separator) {
            // Be aware of code duplication with 'const ValueType& value' overload...
            std::vector<std::string> tokens;
            karabo::data::tokenize(path, tokens, separator);

            Hash* leaf = this->setNodesAsNeeded(tokens, separator);

            // Set last token
            std::string& token = tokens.back();
            int index = karabo::data::getAndCropIndex(token);
            if (index == -1) // No vector
                return leaf->m_container.set(token, std::forward<ValueType>(value));
            else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Only Hash objects may be assigned to a leaf node of array type");
            }
        }

        template <>
        inline Hash::Node& Hash::set<Hash>(const std::string& path, const Hash& value, const char separator) {
            return setHash<const Hash&>(path, value, separator);
        }

        template <>
        inline Hash::Node& Hash::set(const std::string& path, Hash& value, const char separator) {
            // Special overload for 'Hash&' to avoid that the 'ValueType&&' code path is taken for a non-const Hash
            return set(path, const_cast<const Hash&>(value), separator);
        }

        template <>
        inline Hash::Node& Hash::set(const std::string& path, Hash&& value, const char separator) {
            return setHash(path, std::forward<Hash>(value), separator);
        }

        template <typename HashType>
        inline Hash::Node& Hash::setHash(const std::string& path, HashType hashValue, const char separator) {
            // HashType should be 'const Hash&' or 'Hash&&'
            std::vector<std::string> tokens;
            karabo::data::tokenize(path, tokens, separator);

            Hash* leaf = this->setNodesAsNeeded(tokens, separator);

            std::string& token = tokens.back();
            int index = karabo::data::getAndCropIndex(token);
            if (index == -1) // No vector of hashes
                return leaf->m_container.set(token, std::forward<HashType>(hashValue));
            else {                                  // vector of hashes
                if (leaf->m_container.has(token)) { // node exists
                    Hash::Node* node = &(leaf->m_container.getNode(token));
                    if (!node->is<std::vector<Hash> >()) { // Node is not std::vector<Hash>
                        std::vector<Hash> hashes(index + 1);
                        hashes[index] = std::forward<HashType>(hashValue);
                        node->setValue(std::move(hashes)); // Force it to be one
                    } else {
                        std::vector<Hash>& hashes = node->getValue<std::vector<Hash> >();
                        if (index >= static_cast<int>(hashes.size())) hashes.resize(index + 1);
                        hashes[index] = std::forward<HashType>(hashValue);
                    }
                    return *node;
                } else { // node does not exist
                    std::vector<Hash> hashes(index + 1);
                    hashes[index] = std::forward<HashType>(hashValue);
                    return leaf->m_container.set(token, std::move(hashes));
                }
            }
        }

        template <typename ValueType, typename... Args>
        inline void Hash::setMulti(const std::string& key, ValueType&& value, Args&&... args) {
            // The first...
            set(key, std::forward<ValueType>(value));
            // ...and the rest if any
            setMulti(std::forward<Args>(args)...); // does not compile if the args pack has an odd number of elements
        }

        template <typename ValueType>
        ValueType& Hash::bindReference(const std::string& path, const char separator) {
            return set(path, ValueType(), separator).template getValue<ValueType>();
        }

        template <typename ValueType>
        ValueType* Hash::bindPointer(const std::string& path, const char separator) {
            return &set(path, ValueType(), separator).template getValue<ValueType>();
        }

        template <typename ValueType>
        inline const ValueType& Hash::get(const std::string& path, const char separator) const {
            return getNode(path, separator).getValue<const ValueType>();
        }

        template <typename ValueType>
        inline ValueType& Hash::get(const std::string& path, const char separator) {
            return getNode(path, separator).getValue<ValueType>();
        }

        template <typename ValueType>
        inline void Hash::get(const std::string& path, ValueType& value, const char separator) const {
            value = getNode(path, separator).getValue<ValueType>();
        }

        template <typename ValueType>
        inline ValueType Hash::getAs(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAs<ValueType>();
        }

        template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
        inline Cont<T> Hash::getAs(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAs<T, Cont>();
        }

        template <typename ValueType>
        bool Hash::is(const std::string& path, const char separator) const {
            // TODO: remove detailed implementation from header by calling something like
            //       return this->is(typeid (ValueType), path, separator);
            std::string tmp(path);
            int index = karabo::data::getAndCropIndex(tmp);
            if (index == -1) {
                return getNode(tmp, separator).is<ValueType>();
            } else {
                const std::vector<Hash>& hashVec = getNode(tmp, separator).getValue<std::vector<Hash> >();
                if (size_t(index) >= hashVec.size()) {
                    throw KARABO_PARAMETER_EXCEPTION("Index " + toString(index) + " out of range in '" + path + "'.");
                }
                return typeid(hashVec[index]) == typeid(ValueType);
            }
        }

        template <template <class ValueType, class All = std::allocator<ValueType> > class container>
        void Hash::getKeys(container<std::string>& result) const {
            for (const_iterator iter = m_container.begin(); iter != m_container.end(); ++iter) {
                result.push_back(iter->getKey());
            }
        }

        template <template <class ValueType, class All = std::allocator<ValueType> > class container>
        void Hash::getPaths(container<std::string>& result, const char separator) const {
            if (this->empty()) return;
            getPaths(*this, result, "", separator, false);
        }

        template <template <class ValueType, class All = std::allocator<ValueType> > class container>
        void Hash::getDeepPaths(container<std::string>& result, const char separator) const {
            if (this->empty()) return;
            getPaths(*this, result, "", separator, true);
        }

        /*******************************************************************
         * Attributes manipulation
         *******************************************************************/

        template <typename ValueType>
        const ValueType& Hash::getAttribute(const std::string& path, const std::string& attribute,
                                            const char separator) const {
            return getNode(path, separator).getAttribute<ValueType>(attribute);
        }

        template <typename ValueType>
        ValueType& Hash::getAttribute(const std::string& path, const std::string& attribute, const char separator) {
            return getNode(path, separator).getAttribute<ValueType>(attribute);
        }

        template <typename T>
        T Hash::getAttributeAs(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).getAttributeAs<T>(attribute);
        }

        template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
        Cont<T> Hash::getAttributeAs(const std::string& path, const std::string& attribute,
                                     const char separator) const {
            return getNode(path, separator).getAttributeAs<T, Cont>(attribute);
        }

        template <typename ValueType>
        void Hash::setAttribute(const std::string& path, const std::string& attribute, const ValueType& value,
                                const char separator) {
            getNode(path, separator).setAttribute(attribute, value);
        }

        template <typename ValueType>
        void Hash::setAttribute(const std::string& path, const std::string& attribute, ValueType&& value,
                                const char separator) {
            getNode(path, separator).setAttribute(attribute, std::forward<ValueType>(value));
        }

        /*
         * Check the similarity between two objects.
         * Hash: Same number, and same order of similar elements
         * vector<Hash>: Same size, and same order of similar element
         * Element: Same key and same data type
         */

        bool similar(const Hash& left, const Hash& right);
        bool similar(const std::vector<Hash>& left, const std::vector<Hash>& right);
        bool similar(const Hash::Node& left, const Hash::Node& right);

        /*
         * Count the total number of nodes/leaves in Hash/Element
         */
        size_t counter(const Hash& hash);
        size_t counter(const Hash::Node& element);

        /*
         * Count the number of nodes/leaves in Hash of type 'type'/ValueType
         */
        size_t counter(const Hash& hash, Types::ReferenceType);

        template <typename ValueType>
        size_t counter(const Hash& hash) {
            size_t partial_count = 0;

            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                const Hash::Node& ele = *iter;

                partial_count += (ele.is<ValueType>() ? 1 : 0);

                if (ele.is<Hash>()) {
                    partial_count += counter<ValueType>(ele.getValue<Hash>());
                } else {
                    if (ele.is<std::vector<Hash> >()) {
                        const std::vector<Hash>& vect = ele.template getValue<std::vector<Hash> >();
                        partial_count += (typeid(ValueType) == typeid(Hash)) ? vect.size() : 0;
                        for (size_t i = 0; i < vect.size(); ++i) {
                            partial_count += counter<ValueType>(vect[i]);
                        }
                    } else {
                        if (Types::category(ele.getType()) == Types::SEQUENCE) {
                            if (typeid(std::vector<ValueType>) == ele.getValueAsAny().type()) {
                                partial_count += counter(ele);
                            }
                        }
                    }
                }
            }
            return partial_count;
        }

        template <class Visitor>
        bool karabo::data::Hash::visit(Visitor& visitor) {
            return karabo::data::Hash::visit(*this, visitor);
        }

        template <class Visitor>
        bool karabo::data::Hash::visit(karabo::data::Hash& hash, Visitor& visitor) {
            for (Hash::iterator it = hash.begin(), end = hash.end(); it != end; ++it) {
                if (!visit(*it, visitor)) return false;
            }
            return true;
        }

        template <class Visitor>
        bool karabo::data::Hash::visit(karabo::data::Hash::Node& node, Visitor& visitor) {
            if (!visitor(node)) return false;

            switch (node.getType()) {
#if __GNUC__ >= 12
                case Types::HASH:
                    return node.getValue<Hash>().visit(visitor);
                    break;
                case Types::VECTOR_HASH: {
                    std::vector<karabo::data::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!vect[i].visit(visitor)) return false;
                    }
                } break;
#else
                case Types::ReferenceType::HASH:
                    return node.getValue<Hash>().visit(visitor);
                    break;
                case Types::ReferenceType::VECTOR_HASH: {
                    std::vector<karabo::data::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!vect[i].visit(visitor)) return false;
                    }
                } break;
#endif
                default:
                    break;
            }

            return true;
        }

        template <class Visitor>
        bool karabo::data::Hash::visit2(Visitor& visitor) {
            return karabo::data::Hash::visit2(*this, visitor);
        }

        template <class Visitor>
        bool karabo::data::Hash::visit2(karabo::data::Hash& hash, Visitor& visitor) {
            for (Hash::iterator it = hash.begin(), end = hash.end(); it != end; ++it) {
                if (!visit2(*it, visitor)) {
                    return false;
                }
            }
            return true;
        }

        template <class Visitor>
        bool karabo::data::Hash::visit2(karabo::data::Hash::Node& node, Visitor& visitor) {
            visitor.pre(node);
            bool res = visitor(node);

            switch (node.getType()) {
#if __GNUC__ >= 12
                case Types::HASH:
                    res = node.getValue<Hash>().visit2(visitor);
                    break;
                case Types::VECTOR_HASH: {
                    std::vector<karabo::data::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!(res = vect[i].visit2(visitor))) break;
                    }
                } break;
#else
                case Types::ReferenceType::HASH:
                    res = node.getValue<Hash>().visit2(visitor);
                    break;
                case Types::ReferenceType::VECTOR_HASH: {
                    std::vector<karabo::data::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!(res = vect[i].visit2(visitor))) break;
                    }
                } break;
#endif
                default:
                    break;
            }

            visitor.post(node);
            return res;
        }
    } // namespace data
} // namespace karabo
#endif
