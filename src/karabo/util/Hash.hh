/* 
 * File:   Hash.hh
 * Author: <burkhard.heisen@xfel.eu>
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on December 14, 2012, 11:19 AM
 */

#ifndef KARABO_UTIL_HASH_HH
#define	KARABO_UTIL_HASH_HH

#include <iostream>
#include <sstream>
#include <string>

#include <set>
#include <vector>
#include <list>

#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include "ClassInfo.hh"
#include "StringTools.hh"
#include "Types.hh"
#include "Element.hh"
#include "OrderedMap.hh"
#include "Exception.hh"

#include "karaboDll.hh"

namespace karabo {

    namespace util {

        /**
         * Hash container:
         * - The Hash is a heterogeneous generic key/value container that associates a string key to a value of any type.<br>
         * - The Hash is a core data structure in Karabo software framework, and is widly used in the karabo system.<br>
         * - For instance, exchanging data and configurations between two or more entities (devices, GUI), 
         * database interface (store and retrival ), meta-data handling, etc.<br>
         * - The Hash class is much like a XML-DOM container with the difference of 
         * allowing only unique keys on a given tree-level.<br>
         * - Like and XML DOM object, the Hash provides a multi-level (recursive) key-value associative container, 
         * where keys are strings and values can be of any C++ type.<br>
         * 
         * Concept:
         * - Provide recursive key-value associative container (keys are strings and unique, values can be of any type)<br>
         * - Preserve insertion order, while optimized for random key-based lookup. Different iterators are available for each use case.<br>
         * - Like in XML, each hash key can have a list of (key-value) attributes (attribute keys are strings and unique, attribute values can be of any type).<br>
         * - Seamless serialization to/from XML, Binary, HDF5, etc.<br>
         * - Usage: configuration, device-state cache, database interface (result-set), message protocol, meta-data handling, etc.<br>
         * - Templated set, get for retrieving values from keys. Assumes recursursion on "." characters in key by default. 
         *   Seperator can be specified per function call. <br>
         * - Exposed iterators will a sequential iterator (insertion order) and a alpha-numeric order iterator.<br>
         * - Each iterator provides access to its key, value, and attributes in form of a Hash::Node and can thus be used for recursive traversal.<br>
         * - Insertion of a non-existing key leads to new entry whilst insertion of an existing key will only update (merge) the corresponding value/attributes.<br>
         * - Additional functionality include: list of paths, clear/erase, find, merge, comparison, etc.
         */

        class KARABO_DECLSPEC Hash {

        public:

            KARABO_CLASSINFO(Hash, "Hash", "2.0")

            typedef OrderedMap<std::string, Element<std::string> > Attributes;
            typedef Element<std::string, Attributes > Node;

            enum MergePolicy {

                MERGE_ATTRIBUTES,
                REPLACE_ATTRIBUTES
            };

        private:

            typedef OrderedMap<std::string, Node > Container;
            Container m_container;

        public:

            typedef Container::iterator iterator;
            typedef Container::const_iterator const_iterator;

            typedef Container::map_iterator map_iterator;
            typedef Container::const_map_iterator const_map_iterator;

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
            boost::optional<const Hash::Node&> find(const std::string& path, const char separator = '.') const;
            boost::optional<Hash::Node&> find(const std::string& path, const char separator = '.');

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
             * Constructors with different number of key-value pairs as arguments
             * Create and initialize a hash with multiple key-value pairs in a single call
             * @param key1 A string hash key
             * @param value1 Any object as value
             * @param key2 A string hash key
             * @param value2 Any object as value
             * @param key3 A string hash key
             * @param value3 Any object as value
             * @param key4 A string hash key
             * @param value4 Any object as value
             * @param key5 A string hash key
             * @param value5 Any object as value
             * @param key6 A string hash key
             * @param value6 Any object as value
             * 
             * <b>Example:</b>
             * @code
             * Hash hash("firstKey", 1, "secondKey", 2, "thirdKey", 3, "fourthKey", 4, "fifthKey", 5, "sixthKey", 6);
             * @endcode
             */
            template <typename V1>
            Hash(const std::string& path1, const V1& value1);

            template <typename V1, typename V2>
            Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2);

            template <typename V1, typename V2, typename V3>
            Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                 const std::string& path3, const V3& value3);

            template <typename V1, typename V2, typename V3, typename V4>
            Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                 const std::string& path3, const V3& value3, const std::string& path4, const V4& value4);

            template <typename V1, typename V2, typename V3, typename V4, typename V5>
            Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                 const std::string& path3, const V3& value3, const std::string& path4, const V4& value4,
                 const std::string& path5, const V5& value5);

            template <typename V1, typename V2, typename V3, typename V4, typename V5, typename V6>
            Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                 const std::string& path3, const V3& value3, const std::string& path4, const V4& value4,
                 const std::string& path5, const V5& value5, const std::string& path6, const V6& value6);

            /**
             * Destructor
             */
            virtual ~Hash();

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

            /**
             * Merge two Hases and assign the result to a third one.
             * @param other Hash object
             */
            friend
            Hash& operator+(const Hash& hash1, const Hash& hash2);

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
             * Remove the element identified by 'key' if it exists. 
             * Otherwise, it return silently.
             * If 'key' is a composite element, all its descendents are removed. 
             * The path to 'key' is, however, not removed in this case.
             * Example: erase ("a.b.c") will remove "c", but "a" and "b" should 
             * not be removed even when "c" is the only child of "b".
             * @return No
             */
            void erase(const std::string& path, const char separator = '.');

            /**
             * Remove the element identified by 'key' if it exists. 
             * Otherwise, do nothing.
             * If 'key' is a composite element, all its descendents are removed. 
             * The path to 'key' is, however, not removed in this case.
             * Example: erase ("a.b.c") will remove "c", but "a" and "b" should 
             * not be removed even when "c" is the only child of "b".
             * @return true if key exists, otherwise false
             */
            bool eraseFound(const std::string& path, const char separator = '.');
            
            /**
             * Remove the element identified by 'key' if it exists. 
             * Otherwise, it return silently.
             * If 'key' is a composite element, all its descendents are removed. 
             * The path to 'key' is removed as well if parent containers are getting empty.
             * Example: eraseKey ("a.b.c") will remove "c", but "a" and "b" should 
             * be removed as well if "c" is the only child of "b" and this will recursively
             * be continued to the root.  If "a.b.c" the only element in Hash then
             * eraseKey("a.b.c") will result in empty hash.
             * @return No
             */
            void erasePath(const std::string& path, const char separator = '.');

            /**
             * Returns all the keys in the hash in the provided container (vector, list, set, ...)
             * Keys in inner-hashes are not included
             * @param container
             */
            template<template<class T, class All = std::allocator<T> > class container >
            void getKeys(container<std::string>& result) const;

            void getKeys(std::set<std::string>& result) const;

            /**
             * Returns all root-to-leaves paths in hash in the provided container (vector, list, set, ...)
             * This function goes recursively through the whole hash and
             * should just be called for frequently lookup in log(n) ????????????????????
             * @param container
             * @return std::vector<std::string> object
             */

            template<template<class T, class All = std::allocator<T> > class container >
            void getPaths(container<std::string>& result, const char separator = '.') const;

            void getPaths(std::set<std::string>& result, const char separator = '.') const;

            static void getPaths(const Hash& hash, std::vector<std::string>& paths, std::string prefix, const char separator = '.');

            /**
             * Check if the key 'key' exist in hash
             * @param key A string hash key
             * @return <b>true</b> if key exists, <b>false</b> otherwise
             */
            bool has(const std::string& path, const char separator = '.') const;

            /**
             * Insert key/value pair in current container
             * Optimization: to avoid double-copy, ie <i>value</i> into <i>boost::any</i> object, and the later into the map;
             * we insert first insert an empty object of type <b>boost::any</b> into the map, 
             * get a reference to the <i>boost::any</i> object associated with <b>key</b>, then copy the given value into it.
             * @param key A string key
             * @param value Any object as value
             * @return void
             * 
             * <b>Example:</b>
             * @code
             * Hash hash;
             * hash.set("myKey", "myValue");
             * hash.set("myFloat", float(12.44));
             * @endcode
             */
            template<typename ValueType>
            inline Node& set(const std::string& path, const ValueType& value, const char separator = '.'); /**/

            /**
             * Clone the content (key, value, attributes) of another elements.
             * This function use the source element's key, NOT his full path.
             */
            Node& setNode(const Node& srcElement);

            Node& setNode(const_iterator srcIterator);

            /**
             * Bind a (newly created) object in the map into and external variable
             * Optimization: This is useful in order to avoid the later copy of the value into <b>boost::any</b>, in the hash::set(key, value).
             * This function provides a reference to the object in the map where you can build your data directly.
             * @param key A string hash key
             * @return A <b>reference</b> to the internal object
             * <b>Example:<b>
             * @code
             *  vector<string>& vec = set<vector<string> >("key");
             *  vec.resize(20);   
             *  vec[1] = "a string";
             * @endcode
             */
            template <typename ValueType>
            ValueType& bindReference(const std::string& path, const char separator = '.');

            /**
             * Bind a (newly created) object in the map into and external variable
             * Optimization: This is useful in order to avoid the later copy of the value into <b>boost::any</b>, in the hash::set(key, value).
             * This function provides a pointer to the object in the map where you can build your data directly.
             * @param key A string hash key
             * @return A <b>pointer</b> to the internal object
             * <b>Example:<b>
             * @code
             *  int* value = set<int>("key");
             *  *value = 10;
             * @endcode
             */
            template <typename ValueType>
            ValueType* bindPointer(const std::string& path, const char separator = '.');

            /**
             * Retrieve a constant reference to the value of element identified by 'key'
             * @param key A string key
             * @return The associated value
             */
            template<typename ValueType>
            inline const ValueType& get(const std::string& path, const char separator = '.') const;

            /**
             * Retrieve a (non-const) reference to the stored value of a given key
             * @param key A string key
             * @return The associated value
             */
            template<typename ValueType>
            inline ValueType& get(const std::string& key, const char separator = '.');

            /**
             * Retrieve (non-const) reference to the stored value into second parameter
             * @param key A string key
             * @param value Any object that will be filled by reference
             * @return void
             */
            template <typename ValueType>
            inline void get(const std::string& path, ValueType & value, const char separator = '.') const;

            /**
             * Casts the the value of element identified by "path" from its original type 
             * to another different target type.
             * Throws CastException if casting fails, i.e. not posible or unsafe
             * @param path
             * @param separator
             * @return value
             */
            template <typename ValueType>
            inline ValueType getAs(const std::string& path, const char separator = '.') const;

            template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
            inline Cont<T> getAs(const std::string& key, const char separator = '.') const;

            /**
             * Return the internal Hash node element designated by "path"
             * @param path
             * @param separator
             * @return Hash::Node element
             */
            const Node& getNode(const std::string& path, const char separator = '.') const;

            Node& getNode(const std::string& path, const char separator = '.');

            /**
             * Predicate function calculating if the type of the value associated with the <b>key</b> is
             * of a specific type in template parameter
             * @param key The key having associated value and the type of this value we want to test against template parameter
             * @return <b>true</b> or <b>false</b>
             */
            template <typename ValueType> bool is(const std::string & path, const char separator = '.') const;

            /**
             * Predicate function calculating if the value associated with <b>key</b> is of type <b>type</b>.
             * @param key Some string
             * @param type Some type from Types::Type enumeration
             * @return <b>true</b> or <b>false</b>
             */
            bool is(const std::string& path, const Types::ReferenceType & type, const char separator = '.') const;

            /**
             * Function to obtain value type information
             * @param key The key to the value of which type information should be returned
             * @return The typeId as defined in @see Types.hh
             */
            Types::ReferenceType getType(const std::string& path, const char separator = '.') const; /**/

            /** Merges another hash into this one
             * Creates new nodes, if they do not already exists. Creates new leaves, if they do not already exist.
             * Existing leaves will be replaced by the new hash.
             * @param hash Another hash to be merged into current hash
             * @return A self-reference after the appending process (allows object chaining)
             */
            void merge(const Hash& other, const MergePolicy policy = REPLACE_ATTRIBUTES);

            /**
             * Subtracts from current hash all nodes that can be found in other hash given as argument.
             * @param other hash used for defining the candidates for subtraction
             * @param separator.  The default separator is '.'
             * @return the current hash is shrinked in place, the other hash is untouched
             */
            void subtract(const Hash& other, const char separator = '.');
            
            /**
             * Flattens a hierarchical Hash into "one-level", <i>flat</i> Hash object 
             * @return A Hash having keys that are all leaves
             */
            void flatten(Hash& flat, const char separator = '.') const;

            static void flatten(const Hash& hash, Hash& flat, std::string prefix = "", const char separator = '.');

            /**
             * Arranges <i>flat</i> Hash object in a hierarchy using
             * separator symbol (default: ".") during parsing the keys to recognize a hierarchy
             * @param sep An optional separator symbol (default: ".")
             * @return A Hash object containing keys with no separator symbols
             */
            void unflatten(Hash& tree, const char separator = '.') const;

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
            bool hasAttribute(const std::string& path, const std::string& attribute, const char separator = '.') const;

            /**
             * Return the value of the attribute called "attribute" of the element identified by "path"
             * @param path
             * @param attribute
             * @param separator
             * @return bool
             */
            template <typename ValueType>
            const ValueType& getAttribute(const std::string& path, const std::string& attribute, const char separator = '.') const;

            template <typename ValueType>
            ValueType& getAttribute(const std::string& path, const std::string& attribute, const char separator = '.');

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
            T getAttributeAs(const std::string& path, const std::string& attribute, const char separator = '.') const;

            template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
            Cont<T> getAttributeAs(const std::string& path, const std::string& attribute, const char separator = '.') const;

            /**
             * Return the value of the attribute called "attribute" of the element identified as boost::any.
             * @param path
             * @param attribute
             * @param separator
             * @return boost::any
             */
            boost::any& getAttributeAsAny(const std::string& path, const std::string& attribute, const char separator = '.');

            const boost::any& getAttributeAsAny(const std::string& path, const std::string& attribute, const char separator = '.') const;

            /**
             * Return the list of attributes of the element identified by "path"
             * @param path
             * @param separator
             * @return Hash::Attributes
             */
            const Attributes& getAttributes(const std::string& path, const char separator = '.') const;
            Attributes& getAttributes(const std::string& path, const char separator = '.');

            /**
             * Set the value of an attribute called "attribute" of the element identified by "path"
             * @param path
             * @param attribute
             * @param value
             * @param separator
             */
            template <typename ValueType>
            void setAttribute(const std::string& path, const std::string& attribute, const ValueType& value, const char separator = '.');
            void setAttribute(const std::string& path, const std::string& attribute, const char* value, const char separator = '.');

            /**
             * Assign of list of attributes (i.e. Hash::Attributes container) to the element identified by "path"
             * @param path
             * @param attribute
             * @param value
             * @param separator
             */
            void setAttributes(const std::string& path, const Attributes& attributes, const char separator = '.');

            /**
             * Serialize a hash to standard std::ostream object
             * @param visitor
             */
            friend std::ostream& operator<<(std::ostream& os, const Hash& hash);

            bool operator==(const Hash& other);
            bool operator!=(const Hash& other);
            
            /**
             * Implement the visitor pattern
             * @param visitor
             * @return bool
             */
            template<class Visitor>
            bool visit(Visitor& visitor);

            template<class Visitor>
            bool visit2(Visitor& visitor);
        private:
            template<class Visitor>
            static bool visit(karabo::util::Hash& hash, Visitor& visitor);

            template<class Visitor>
            static bool visit(karabo::util::Hash::Node& node, Visitor& visitor);

            template<class Visitor>
            static bool visit2(karabo::util::Hash& hash, Visitor& visitor);

            template<class Visitor>
            static bool visit2(karabo::util::Hash::Node& node, Visitor& visitor);
        private:

            void mergeAndMergeAttributes(const Hash& other);

            void mergeAndReplaceAttributes(const Hash& other);

            Hash* setNodesAsNeeded(const std::vector<std::string>& tokens, char seperator);

            Hash& getLastHash(const std::string& path, std::string& last_key, const char separator = '.');
            const Hash& getLastHash(const std::string& path, std::string& last_key, const char separator = '.') const;

            const Hash & thisAsConst() const {
                return const_cast<const Hash &> (*this);
            }

            void toStream(std::ostream& os, const Hash& hash, int depth) const;

        };
    }
}

namespace karabo {

    namespace util {

        template <typename V1>
        Hash::Hash(const std::string& path1, const V1& value1) {
            this->set(path1, value1);
        }

        template <typename V1, typename V2>
        Hash::Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2) {
            this->set(path1, value1);
            this->set(path2, value2);
        }

        template <typename V1, typename V2, typename V3>
        Hash::Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                   const std::string& path3, const V3& value3) {
            this->set(path1, value1);
            this->set(path2, value2);
            this->set(path3, value3);
        }

        template <typename V1, typename V2, typename V3, typename V4>
        Hash::Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                   const std::string& path3, const V3& value3, const std::string& path4, const V4& value4) {
            this->set(path1, value1);
            this->set(path2, value2);
            this->set(path3, value3);
            this->set(path4, value4);
        }

        template <typename V1, typename V2, typename V3, typename V4, typename V5>
        Hash::Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                   const std::string& path3, const V3& value3, const std::string& path4, const V4& value4,
                   const std::string& path5, const V5& value5) {
            this->set(path1, value1);
            this->set(path2, value2);
            this->set(path3, value3);
            this->set(path4, value4);
            this->set(path5, value5);
        }

        template <typename V1, typename V2, typename V3, typename V4, typename V5, typename V6>
        Hash::Hash(const std::string& path1, const V1& value1, const std::string& path2, const V2& value2,
                   const std::string& path3, const V3& value3, const std::string& path4, const V4& value4,
                   const std::string& path5, const V5& value5, const std::string& path6, const V6& value6) {
            this->set(path1, value1);
            this->set(path2, value2);
            this->set(path3, value3);
            this->set(path4, value4);
            this->set(path5, value5);
            this->set(path6, value6);
        }

        template<> inline const boost::any& Hash::get(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAsAny();
        }

        template<> inline const Hash& Hash::get(const std::string& path, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                return hash.m_container.get<Hash > (key);
            } else {
                return hash.m_container.get<vector<Hash> >(key)[index];
            }
        }

        template<> inline Hash& Hash::get(const std::string& path, const char separator) {
            return const_cast<Hash&> (thisAsConst().get<Hash > (path, separator));
        }

        template<> inline boost::any& Hash::get(const std::string& path, const char separator) {
            return getNode(path, separator).getValueAsAny();
        }

        template<typename ValueType>
        inline Hash::Node& Hash::set(const std::string& path, const ValueType& value, const char separator) {

            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);

            Hash* leaf = this->setNodesAsNeeded(tokens, separator);

            // Set last token
            std::string token = tokens.back();
            int index = karabo::util::getAndCropIndex(token);
            if (index == -1) // No vector
                return leaf->m_container.set(token, value);
            else {
                throw KARABO_NOT_SUPPORTED_EXCEPTION("Only Hash objects may be assigned to a leaf node of array type");
            }
        }

        template<>
        inline Hash::Node& Hash::set(const std::string& path, const Hash& value, const char separator) {

            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);

            Hash* leaf = this->setNodesAsNeeded(tokens, separator);

            std::string token = tokens.back();
            int index = karabo::util::getAndCropIndex(token);
            if (index == -1) // No vector of hashes
                return leaf->m_container.set(token, value);
            else { // vector of hashes
                if (leaf->m_container.has(token)) { // node exists
                    Hash::Node* node = &(leaf->m_container.getNode(token));
                    if (!node->is<std::vector<Hash> >()) { // Node is not std::vector<Hash>
                        std::vector<Hash> hashes(index + 1);
                        hashes[index] = value;
                        node->setValue(std::vector<Hash > (hashes)); // Force it to be one
                    } else {
                        std::vector<Hash>& hashes = node->getValue<std::vector<Hash> >();
                        if (index >= static_cast<int> (hashes.size())) hashes.resize(index + 1);
                        hashes[index] = value;
                    }
                    return *node;
                } else { // node does not exist
                    std::vector<Hash> hashes(index + 1);
                    hashes[index] = value;
                    return leaf->m_container.set(token, hashes);
                }
            }
        }

        template <typename ValueType >
        ValueType & Hash::bindReference(const std::string& path, const char separator) {
            return set(path, ValueType(), separator).template getValue<ValueType > ();
        }

        template <typename ValueType >
        ValueType * Hash::bindPointer(const std::string& path, const char separator) {
            return &set(path, ValueType(), separator).template getValue<ValueType > ();
        }

        template <typename ValueType>
        inline const ValueType & Hash::get(const std::string& path, const char separator) const {
            return getNode(path, separator).getValue<const ValueType > ();
        }

        template <typename ValueType>
        inline ValueType & Hash::get(const std::string& path, const char separator) {
            return getNode(path, separator).getValue<ValueType > ();
        }

        template <typename ValueType>
        inline void Hash::get(const std::string& path, ValueType & value, const char separator) const {
            value = getNode(path, separator).getValue<ValueType > ();
        }

        template <typename ValueType>
        inline ValueType Hash::getAs(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAs<ValueType>();
        }

        template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
        inline Cont<T> Hash::getAs(const std::string& path, const char separator) const {
            return getNode(path, separator).getValueAs<T, Cont>();
        }

        template <typename ValueType> bool Hash::is(const std::string & path, const char separator) const {
            std::string tmp(path);
            int index = karabo::util::getAndCropIndex(tmp);
            if (index == -1) {
                return getNode(tmp, separator).is<ValueType > ();
            } else {
                return typeid (getNode(tmp, separator).getValue<vector<Hash> >()[index]) == typeid (ValueType);
            }
        }

        template<template<class ValueType, class All = std::allocator<ValueType> > class container >
        void Hash::getKeys(container<std::string>& result) const {
            for (const_iterator iter = m_container.begin(); iter != m_container.end(); ++iter) {
                result.push_back(iter->getKey());
            }
        }

        template<template<class ValueType, class All = std::allocator<ValueType> > class container >
        void Hash::getPaths(container<std::string>& result, const char separator) const {
            if (this->empty()) return;
            getPaths(*this, result, "", separator);
        }

        /*******************************************************************
         * Attributes manipulation
         *******************************************************************/

        template <typename ValueType>
        const ValueType & Hash::getAttribute(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).getAttribute<ValueType > (attribute);
        }

        template <typename ValueType >
        ValueType & Hash::getAttribute(const std::string& path, const std::string& attribute, const char separator) {
            return getNode(path, separator).getAttribute<ValueType > (attribute);
        }

        template <typename T>
        T Hash::getAttributeAs(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).getAttributeAs<T >(attribute);
        }

        template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
        Cont<T> Hash::getAttributeAs(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).getAttributeAs<T, Cont>(attribute);
        }

        template <typename ValueType>
        void Hash::setAttribute(const std::string& path, const std::string& attribute, const ValueType& value, const char separator) {
            getNode(path, separator).setAttribute(attribute, value);
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

        template<typename ValueType>
        size_t counter(const Hash& hash) {
            size_t partial_count = 0;

            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                const Hash::Node& ele = *iter;

                partial_count += (ele.is<ValueType > () ? 1 : 0);

                if (ele.is<Hash > ()) {
                    partial_count += counter<ValueType > (ele.getValue<Hash > ());
                } else {
                    if (ele.is<std::vector<Hash> >()) {
                        const std::vector<Hash>& vect = ele.template getValue<std::vector<Hash> >();
                        partial_count += (typeid (ValueType) == typeid (Hash)) ? vect.size() : 0;
                        for (size_t i = 0; i < vect.size(); ++i) {
                            partial_count += counter<ValueType > (vect[i]);
                        }
                    } else {
                        if (Types::category(ele.getType()) == Types::SEQUENCE) {
                            if (typeid (std::vector<ValueType>) == ele.getValueAsAny().type()) {
                                partial_count += counter(ele);
                            }
                        }
                    }
                }
            }
            return partial_count;
        }

        template<class Visitor>
        bool karabo::util::Hash::visit(Visitor& visitor) {
            return karabo::util::Hash::visit(*this, visitor);
        }

        template<class Visitor>
        bool karabo::util::Hash::visit(karabo::util::Hash& hash, Visitor& visitor) {
            for (Hash::iterator it = hash.begin(), end = hash.end(); it != end; ++it) {
                if (!visit(*it, visitor)) return false;
            }
            return true;
        }

        template<class Visitor>
        bool karabo::util::Hash::visit(karabo::util::Hash::Node& node, Visitor& visitor) {
            if (!visitor(node)) return false;

            switch (node.getType()) {
                case Types::HASH:
                    return node.getValue<Hash > ().visit(visitor);
                    break;
                case Types::VECTOR_HASH:
                {
                    std::vector<karabo::util::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!vect[i].visit(visitor)) return false;
                    }
                }
                    break;
                default:
                    break;
            }

            return true;
        }

        template<class Visitor>
        bool karabo::util::Hash::visit2(Visitor& visitor) {
            return karabo::util::Hash::visit2(*this, visitor);
        }

        template<class Visitor>
        bool karabo::util::Hash::visit2(karabo::util::Hash& hash, Visitor& visitor) {
            for (Hash::iterator it = hash.begin(), end = hash.end(); it != end; ++it) {
                if (!visit2(*it, visitor)) {
                    return false;
                }
            }
            return true;
        }

        template<class Visitor>
        bool karabo::util::Hash::visit2(karabo::util::Hash::Node& node, Visitor& visitor) {
            visitor.pre(node);
            bool res = visitor(node);

            switch (node.getType()) {
                case Types::HASH:
                    res = node.getValue<Hash > ().visit2(visitor);
                    break;
                case Types::VECTOR_HASH:
                {
                    std::vector<karabo::util::Hash>& vect = node.getValue<std::vector<Hash> >();
                    for (size_t i = 0, size = vect.size(); i < size; ++i) {
                        if (!(res = vect[i].visit2(visitor))) break;
                    }
                }
                    break;
                default:
                    break;
            }

            visitor.post(node);
            return res;
        }
    }
}
#endif

