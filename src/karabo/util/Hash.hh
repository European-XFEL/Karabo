/*
 * $Id: Hash.hh 6954 2012-08-09 13:01:34Z heisenb $
 *
 * File:   Hash.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 13, 2010, 6:55 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef KARABO_UTIL_HASH_HH
#define KARABO_UTIL_HASH_HH

#include <map>
#include <string>
#include <set>
#include <stdio.h>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "Exception.hh"
#include "Types.hh"
#include "String.hh"
#include "utildll.hh"
#include "ClassInfo.hh"

// Forwarding python wrapper
namespace karabo {
    namespace pyexfel {
        class HashWrap;
    }
}

/*!
 *  Defines top level namespace for European XFEL software
 */
namespace karabo {

    namespace util {

        /**
         * Heterogeneous generic key/value container.
         * The Hash class can be regarded as a generic hash container
         * which associates a string key to any value type.  Any type means the Hash type itself.
         * It means that Hash container may keep values that are just other Hash containers forming
         * a tree-like hierarchy.  The key representing a hierarchy we are calling a path.  Default key separator
         * in a path is a "." (dot) symbol.  Correspondingly, Hash container supports "one-level-access" methods,
         * like <b>get</b>/<b>set</b>, but also "any-level-access" methods: <b>getFromPath</b>/<b>setFromPath</b>.
         *
         * <b>Example.</b>  Constructing an empty hash, setting and getting key->value pairs
         * @code
         * Hash hash;
         *
         * // ---- Inserting key->value pairs ----
         *
         * // This will associate the integer 4 to "myKey"
         * hash.set("myInt", 4);
         *
         * // If you want to be explicit about the value, you may type
         * hash.set<double>("myDouble", 3.14);
         *
         * // or equivalently:
         * hash.set("myDouble", double(3.14));
         *
         * // ---- Retrieving key->value pairs ----
         *
         * int anInt = hash.get<int>("myInt");
         *
         * // or equivalently:
         * int anotherInt;
         * hash.get("myInt", anotherInt);
         * 
         * // ---- Using hierarchical methods ----
         * hash.setFromPath("a.b.c.e", double(2.7182818284));
         * hash.setFromPath("a.b.c.s", std::string("e number"));
         * 
         * // ---- Retrieving these values ----
         * double e = hash.getFromPath<double>("a.b.c.e");
         * std::string s = hash.getFromPath<std::string>("a.b.c.s");
         * 
         * @endcode
         */
        class DECLSPEC_UTIL Hash : public std::map<std::string, boost::any> {
            friend class karabo::pyexfel::HashWrap;

        public:

            KARABO_CLASSINFO(Hash, "Hash", "1.0")

            /**
             * Use default constructor to create an empty hash
             * 
             * <b>Example:</b>
             * @code
             * Hash hash;
             * @endcode
             */
            Hash() {
            }

            virtual ~Hash() {
            }

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
            explicit Hash(const std::string& key) {
                setFromPath(key);
            }

            /**
             * Use this constructor to create a hash with one key/value pair
             * @param key A string hash key
             * @param value Any object as value
             * 
             * <b>Example:</b>
             * @code
             * Hash hash("myKey", 2);
             * Hash anotherDictionary("myKey", vector<double>(10,0.0));
             * @endcode
             */
            template<class T>
            Hash(const std::string& key, const T& value) {
                setFromPath(key, value);
            }

            /**
             * Use this constructor to create a hash with 2  key/value pairs
             * @param key1 A string hash key
             * @param value1 Any object as value
             * @param key2 A string hash key
             * @param value2 Any object as value
             * 
             * <b>Example:</b>
             * @code
             * Hash hash("myFirstKey", 2, "mySecondKey", "stringValue");
             * @endcode
             */
            template<class T, class U>
            Hash(const std::string& key1, const T& value1, const std::string& key2, const U& value2) {
                setFromPath(key1, value1);
                setFromPath(key2, value2);
            }

            /**
             * Use this constructor to create a hash with 3 key/value pairs
             * @param key1 A string hash key
             * @param value1 Any object as value
             * @param key2 A string hash key
             * @param value2 Any object as value
             * @param key3 A string hash key
             * @param value3 Any object as value
             * 
             * <b>Example:</b>
             * @code
             * Hash hash("myFirstKey", 2, "mySecondKey", "stringValue", "myThirdKey", (double)3.14);
             * @endcode
             */
            template<class T, class U, class V>
            Hash(const std::string& key1, const T& value1, const std::string& key2, const U& value2,
            const std::string& key3, const V& value3) {
                setFromPath(key1, value1);
                setFromPath(key2, value2);
                setFromPath(key3, value3);
            }

            /**
             * Use this constructor to create a hash with 4 key/value pairs
             * @param key1 A string hash key
             * @param value1 Any object as value
             * @param key2 A string hash key
             * @param value2 Any object as value
             * @param key3 A string hash key
             * @param value3 Any object as value
             * @param key4 A string hash key
             * @param value4 Any object as value
             * 
             * <b>Example:</b>
             * @code
             * Hash hash("firstKey", 1, "secondKey", 2, "thirdKey", 3, "fourthKey", 4);
             * @endcode
             */
            template<class T, class U, class V, class W>
            Hash(const std::string& key1, const T& value1, const std::string& key2, const U& value2,
            const std::string& key3, const V& value3, const std::string& key4, const W& value4) {
                setFromPath(key1, value1);
                setFromPath(key2, value2);
                setFromPath(key3, value3);
                setFromPath(key4, value4);
            }



            /**
             * Returns all keys of hash in a vector
             * @return std::vector<std::string> object
             */
            std::vector<std::string> getKeysAsVector() const;

            /**
             * Returns all keys of hash in a set
             * @return std::set<std::string> object
             */
            std::set<std::string> getKeysAsSet() const;



            /**
             * Returns all leaves of hash in a vector
             * This function goes recursively through the whole hash and
             * should just be called for frequently lookup in log(n)
             * @return std::vector<std::string> object
             */
            std::vector<std::string> getLeavesAsVector(const std::string& sep = ".") const;

            /**
             * Returns all leaves of hash in a set
             * This function goes recursively through the whole hash and
             * should just be called for frequently lookup in log(n)
             * @return std::set<std::string> object
             */
            std::set<std::string> getLeavesAsSet(const std::string & sep = ".") const;

            /**
             * Allows checking if key is in hash
             * @param key A string hash key
             * @return <b>true</b> if key exists, <b>false</b> otherwise
             */
            inline bool has(const std::string& key) const {
                return find(key) != this->end();
            }

            /**
             * Set key/value pair in current container
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
            template<class T>
            inline void set(const std::string& key, const T& value) {
                ((*this)[key] = boost::any()) = value;
            }

            /** 
             * Overloaded <b>set</b> function for <i>const char*</i>.
             * This function makes a copy of the data stored under the address indicated  
             * by the <i>const char</i> pointer into a <i>std::string</i> and only then stores the string value in the hash.
             * In order to get the value from the hash one needs to use <i>std::string</i> as a type.
             * This function is implicitly called when a string literal is given as a parameter:
             * @code
             *  Hash hash;
             *  hash.set("a", "abcdefg");               
             *  std::string str = hash.get<std::string>("a");
             * @endcode
             *  
             * One can force calling the original, not overloaded function by explicitly indicating
             * template parameter type: 
             * @code
             *  Hash hash;
             *  const char* ch = "this is not converted to the std::string";
             *  hash.set<const char*>("a", ch);
             *  const char* cChar = hash.get<const char*>("a");
             * @endcode
             * In that case only the pointer is copied and the client code is responsible for making sure
             * that the memory is not deallocated (i.e.: by going out of the scope)
             * 
             * Note that by purpose the non-<i>const char*</i> version of this function is not overloaded
             * @param key A string key
             * @param value <i>const char</i> pointer as a value
             * @return void
             */
            void set(const std::string& key, const char* const& value) {
                (operator[](key) = boost::any()) = std::string(value);
            }

            /**
             * Overloaded <b>set</b> function to resolve <i>const wchar* / wstring</i> ambiguity in C++
             * @param key A std::string key
             * @param value const wchar* pointer as value
             * @return void
             */
            void set(const std::string& key, const wchar_t* const& value) {
                (operator[](key) = boost::any()) = std::wstring(value);
            }

            /**
             * Overloaded <b>set</b> function to resolve <i>wchar* / wstring</i> ambiguity in C++
             * @param key A string key
             * @param value <i>wchar*</i> pointer as value
             * @return void                                               
             */
            void set(const std::string& key, wchar_t* const& value) {
                (operator[](key) = boost::any()) = std::wstring(value);
            }

            /**
             * This <b>set</b> function gives exception intentionally because of broken support of <i>vector<bool></i> in C++.
             * @param key A string key
             * @param value <i>std::vector<bool></i> object
             * @return Gives NON_SUPPORTED_EXCEPTION 
             */
            void set(const std::string& key, const std::vector<bool>& value) {
                throw NOT_SUPPORTED_EXCEPTION("Datatype vector<bool> is a broken STL construct and will be deprecated soon, use deque<bool> instead");
            }

            /**
             * Overloaded <b>set</b> function for cases when the value is already of <i>boost::any</i> type.
             * @param key A string key
             * @param value <i>boost::any</i> object
             * @return void
             */
            void set(const std::string& key, const boost::any& value) {
                operator[](key) = value;
            }

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

            template <class T>
            T& bindReference(const std::string& key) {
                return boost::any_cast<T& > (((*this)[key] = boost::any(T())));
            }

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

            template <class T>
            T* bindPointer(const std::string& key) {
                return &(bindReference<T > (key));
            }

            /**
             * Retrieve a constant reference to the stored value of a given key
             * @param key A string key
             * @return The associated value
             */
            template<class T>
            inline const T& get(const std::string& key) const {
                const_iterator it = this->find(key);
                if (it == this->end()) {
                    throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                }
                return get<T > (it);
            }

            /**
             * Retrieve a (non-const) reference to the stored value of a given key
             * @param key A string key
             * @return The associated value
             */
            template<class T>
            inline T& get(const std::string& key) {
                iterator it = this->find(key);
                if (it == this->end()) {
                    throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                }
                return get<T > (it);
            }

            /**
             * Retrieve (non-const) reference to the stored value into second parameter
             * @param key A string key
             * @param value Any object that will be filled by reference
             * @return void
             */
            template<class T>
            inline void get(const std::string& key, T & value) const {
                try {
                    value = get<T > (key);
                } catch (...) {
                    RETHROW;
                }
            }

            /**
             *  Obtains the value associated with Hash::const_iterator <b>it</b>
             *
             * @param it <i>Hash::const</i>_iterator
             * @return The associated value
             */
            template<class T>
            inline const T & get(const const_iterator & it) const {
                const boost::any* value = &(it->second);
                // TODO See whether this is efficient enough in long term
                // Things got changed upon the MATLAB integration problems related to RTTI
                // Before the line below would read:
                // if (value->type() == typeid (T)) {
                if (strcmp(value->type().name(), typeid (T).name()) == 0) {
                    return *(boost::any_cast<T > (value));
                } else {
                    std::string msg = getCastFailureDetails(it->first, it->second.type(), typeid (T));
                    throw CAST_EXCEPTION(msg);
                }
            }

            /**
             *  Obtains the value associated with Hash::iterator <b>it</b>
             * @param it <i>Hash::const_iterator</i>
             * @return The associated value
             */
            template<class T>
            inline T & get(const iterator & it) {
                boost::any* value = &(it->second);
                // TODO See whether this is efficient enough in long term
                // Things got changed upon the MATLAB integration problems related to RTTI
                // Before the line below would read:
                // if (value->type() == typeid (T)) {
                if (strcmp(value->type().name(), typeid (T).name()) == 0) {
                    return *(boost::any_cast<T > (value));
                } else {
                    std::string msg = getCastFailureDetails(it->first, it->second.type(), typeid (T));
                    throw CAST_EXCEPTION(msg);
                }
            }

            /**
             * Obtains the value associated with Hash::const_iterator <b>it</b>.
             * The value is wrapped in the boost::any object. Iterator must be checked for validity before.
             * 
             * <b>Example:</b>
             * @code
             * Hash::const_iterator it = data.find(key);
             * if (it != this->end())  { return 1; }  
             * const boost::any& any = data.getAny(it); 
             * 
             * @endcode
             * @param it <i>Hash::const_iterator</i>
             * @return <i>boost::any</i> object
             */
            inline const boost::any& getAny(const const_iterator& it) const {
                return it->second;
            }

            /**
             * Non const version of the getAny function.
             * The value is wrapped in the <i>boost::any</i> object. Iterator must be checked for validity before.
             * @param it <i>Hash::iterator</i>
             * @return <i>boost::any</i> object
             */

            inline boost::any& getAny(iterator& it) {
                return it->second;
            }

            template <class T>
            inline std::pair<bool, const T&> tryToGet(const std::string& key) const {
                const_iterator it = this->find(key);
                if (it == this->end()) {
                    static T t;
                    return std::pair<bool, const T&>(false, t);
                } else {
                    return std::pair<bool, const T&>(true, get<T > (it));
                }
            }

            template <class T>
            inline std::pair<bool, T&> tryToGet(const std::string& key) {
                iterator it = this->find(key);
                if (it == this->end()) {
                    static T t;
                    return std::pair<bool, const T&>(false, t);
                } else {
                    return std::pair<bool, const T&>(true, get<T > (it));
                }
            }

            template <class T>
            inline bool tryToGet(const std::string& key, T& value) const {
                const_iterator it = this->find(key);
                if (it == this->end()) {
                    return false;
                } else {
                    value = get<T > (it);
                    return true;
                }
            }

            /**
             * Tries to safely cast and return a numeric type
             * @param key A string key
             * @return The associated value and convert it if a converter defined
             */
            template<class T>
            inline T getNumeric(const std::string & key) const {
                const_iterator it = this->find(key);
                if (it == this->end()) {
                    throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                }
                return getNumeric<T > (it);
            }

            /**
             * Tries to safely cast into given numeric type
             * @param key A string key
             * @param value Numeric datatype that will be filled by reference
             */
            template<class T>
            inline void getNumeric(const std::string& key, T & value) const {
                try {
                    value = getNumeric<T > (key);
                } catch (...) {
                    RETHROW;
                }
            }

            /**
             * Casts the value associated with iterator <b>it</b> to type T defined as template parameter
             * @param it Iterator of current Hash object
             * @return value casted to type T
             * 
             * <b>Example:</b>
             * @code
             * Hash hash;
             * // Fill hash with some key/value pairs where some values have double type
             * ...
             * // Assign iterator to some key/value pair with the value of type double
             * Hash::const_iterator it = ...;
             * double d = hash.getNumeric<double>(it);
             * @endcode
             */
            template<class T>
            inline T getNumeric(const const_iterator & it) const {
                boost::any value = it->second;
                const std::type_info& src = value.type();
                try {
                    if (src == typeid (double)) {
                        return boost::numeric_cast<T > (boost::any_cast<double > (value));
                    } else if (src == typeid (float)) {
                        return boost::numeric_cast<T > (boost::any_cast<float > (value));
                    } else if (src == typeid (long long)) {
                        return boost::numeric_cast<T > (boost::any_cast<long long > (value));
                    } else if (src == typeid (int)) {
                        return boost::numeric_cast<T > (boost::any_cast<int > (value));
                    } else if (src == typeid (short)) {
                        return boost::numeric_cast<T > (boost::any_cast<short > (value));
                    } else if (src == typeid (char)) {
                        return boost::numeric_cast<T > (boost::any_cast<char > (value));
                    } else if (src == typeid (signed char)) {
                        return boost::numeric_cast<T > (boost::any_cast<signed char > (value));
                    } else if (src == typeid (unsigned long long)) {
                        return boost::numeric_cast<T > (boost::any_cast<unsigned long long > (value));
                    } else if (src == typeid (unsigned int)) {
                        return boost::numeric_cast<T > (boost::any_cast<unsigned int > (value));
                    } else if (src == typeid (unsigned short)) {
                        return boost::numeric_cast<T > (boost::any_cast<unsigned short > (value));
                    } else if (src == typeid (unsigned char)) {
                        return boost::numeric_cast<T > (boost::any_cast<unsigned char > (value));
                    } else if (src == typeid (bool)) {
                        return boost::numeric_cast<T > (boost::any_cast<bool > (value));
                    } else {
                        throw NOT_SUPPORTED_EXCEPTION("Given value type is not supported");
                    }
                } catch (...) {
                    std::string msg = getCastFailureDetails(it->first, it->second.type(), typeid (T));
                    throw CAST_EXCEPTION(msg);
                }
            }

            /**
             * Converts the value corresponding the <b>key</b> into string representation if possible
             * @param key A string key
             * @return String representation of value
             */
            std::string getAsString(const std::string & key) const;

            /**
             * Converts the value corresponding iterator <b>it</b> into string representation if possible
             * @param it Valid Hash iterator
             * @return String representation of the value
             */
            std::string getAsString(const const_iterator & it) const;

            /**
             * Predicate function calculating if the type of the value associated with the <b>key</b> is
             * of a specific type in template parameter
             * @param key The key having associated value and the type of this value we want to test against template parameter
             * @return <b>true</b> or <b>false</b>
             */
            template <typename T> bool is(const std::string & key) const {
                const const_iterator it = find(key);
                if (it == this->end()) {
                    throw PARAMETER_EXCEPTION("Key \"" + key + "\" does not exist");
                }
                return is<T > (it);
            }

            /**
             * Predicate function calculating if the type of the value associated with iterator <b>it</b>
             * is of type of template parameter
             * @param it Iterator pointing to key/value pair
             * @return <b>true</b> or <b>false</b>
             */
            template <typename T> bool is(const const_iterator & it) const {
                return it->second.type() == typeid (T);
            }

            /**
             * Predicate function calculating if the value associated with <b>key</b> is of type <b>type</b>.
             * @param key Some string
             * @param type Some type from Types::Type enumeration
             * @return <b>true</b> or <b>false</b>
             */
            bool is(const std::string& key, const Types::Type & type) const {
                return getTypeAsId(key) == type;
            }

            /**
             * Predicate function checks that given key/value pair is identical (equal content, size, etc) to
             * some pair in current Hash
             * @param sit iterator of Hash container
             * @return true if current Hash contains such a pair
             */
            bool identical(const const_iterator& sit) const;

            /**
             * Function to obtain value type information
             * @param key The key to the value of which type information should be returned
             * @return A human readable string reflecting type information
             */
            std::string getTypeAsString(const std::string & key) const;

            /**
             * Function to obtain value type information
             * @param it A valid iterator on the hash
             * @return A human readable string reflecting type information
             */
            std::string getTypeAsString(const const_iterator & it) const;

            /**
             * Function to obtain value type information
             * @param key The key to the value of which type information should be returned
             * @return The typeId as defined in @see Types.hh
             */
            Types::Type getTypeAsId(const std::string & key) const;

            /**
             * Function to obtain value type information
             * @param it A valid iterator on the hash
             * @return The typeId as defined in @see Types.hh
             */
            Types::Type getTypeAsId(const const_iterator & it) const;

            /** Appends another hash to this one
             * @n
             * <b>PLEASE NOTE:</b> In case of duplicate keys, the value of the appended key overwrites the original one
             * @param hash Another hash to be appended to current hash
             * @return A self-reference after the appending process (allows object chaining)
             */
            const Hash& append(const Hash& hash);

            /** Converts the value into user specified datatype or does nothing
             * if the value is not of type std::string.
             * @param key The key of the value to be converted
             * @param type The datatype to which the string value should be converted
             * @return void
             */
            void convertFromString(const std::string& key, const Types::Type & type);


            /** Updates the current Hash container in place using container given in parameter.
             * The intersected parts will be updated, new parts will be copied
             * @param hash Input hash used for updating
             * @return void
             */
            void update(const Hash& hash);

            /**
             * Flattens a hierarchical Hash into "one-level", <i>flat</i> Hash object 
             * @return A Hash having keys that are all leaves
             */
            Hash flatten(const std::string& sep = ".") const;

            /**
             * Arranges <i>flat</i> Hash object in a hierarchy using
             * separator symbol (default: ".") during parsing the keys to recognize a hierarchy
             * @param sep An optional separator symbol (default: ".")
             * @return A Hash object containing keys with no separator symbols
             */
            Hash unflatten(const std::string& sep = ".") const;


            //      bool hasFromPath(const string& path, string sep = ".") const {
            //        
            //      }

            /**
             * Retrieves value given a complete path. Works in non-const context.
             * @param path A string containing separator symbols using to parse a path to recognize a hierarchy
             * @param sep  An optional separator symbol (default: ".")
             * @return A reference to object of type <i>T</i> 
             */
            template <class T >
            T & getFromPath(const std::string& path, std::string sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    throw LOGIC_EXCEPTION("No path (nested key value given)");
                } else if (nElements == 1) {
                    std::string key(v[0]);
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(key);
                    if (arrayType.get < 0 > () == true) {
                        return getArrayElement(arrayType, T());
                    } else {
                        return get<T > (key);
                    }
                } else {
                    std::string shorterPath = path.substr(0, path.find_last_of(sep));
                    std::string last = *(v.rbegin());
                    return r_get<Hash > (shorterPath, sep).getFromPath<T > (last);
                }
            }

            template <class C >
            C & r_get(const std::string& path, std::string sep) {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    return *(static_cast<C*> (this));
                } else if (nElements == 1) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[0]);
                    if (arrayType.get < 0 > () == true) {
                        return getArrayElement<C > (arrayType);
                    } else {
                        return get<C > (v[0]);
                    }
                } else {
                    for (size_t i = 0; i < nElements; ++i) {
                        std::string shorterPath = path.substr(0, path.find_last_of(sep));
                        std::string last = *(v.rbegin());
                        boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(last);
                        if (arrayType.get < 0 > () == true) {
                            return r_get<C > (shorterPath, sep).getArrayElement<C > (arrayType);
                        } else {
                            return r_get<C > (shorterPath, sep).get<C > (last);
                        }
                    }
                    // This should never happen
                    throw LOGIC_EXCEPTION("Bad internal recursion error, ask BH");
                }
            }

            /**
             * Retrieves value given a complete path. Works in <i>const</i> context.
             * @param path A string containing separator symbols using to parse a path to recognize a hierarchy
             * @param sep  An optional separator symbol (default: ".")
             * @return A constant reference to object of type <i>T</i> 
             */
            template <class T>
            const T & getFromPath(const std::string& path, std::string sep = ".") const {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    throw LOGIC_EXCEPTION("No path (nested key value given)");
                } else if (nElements == 1) {
                    std::string key(v[0]);
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(key);
                    if (arrayType.get < 0 > () == true) {
                        return getArrayElement(arrayType, T());
                    } else {
                        return get<T > (key);
                    }
                } else {
                    std::string shorterPath = path.substr(0, path.find_last_of(sep));
                    std::string last = *(v.rbegin());
                    return r_get<Hash > (shorterPath, sep).getFromPath<T > (last);
                }
            }

            template <class C>
            const C & r_get(const std::string& path, std::string sep) const {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    return *(static_cast<const C*> (this));
                } else if (nElements == 1) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[0]);
                    if (arrayType.get < 0 > () == true) {
                        return getArrayElement<C > (arrayType);
                    } else {
                        return get<C > (v[0]);
                    }
                } else {
                    for (size_t i = 0; i < nElements; ++i) {
                        std::string shorterPath = path.substr(0, path.find_last_of(sep));
                        std::string last = *(v.rbegin());
                        boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(last);
                        if (arrayType.get < 0 > () == true) {
                            return r_get<C > (shorterPath, sep).getArrayElement<C > (arrayType);
                        } else {
                            return r_get<C > (shorterPath, sep).get<C > (last);
                        }
                    }
                    // This should never happen
                    throw LOGIC_EXCEPTION("Bad internal recursion error, ask BH");
                }
            }

            template <class T>
            bool tryToGetFromPath(const std::string& path, T & value) const {
                try {
                    value = getFromPath<T > (path);
                    return true;
                } catch (const ParameterException&) {
                } catch (const CastException&) {
                    RETHROW_AS(CAST_EXCEPTION("Could not cast value"))
                } catch (...) {
                    RETHROW
                }
                return false;
            }

            /**
             * Set key/value pair into current container using path, value and separator
             * @param path  A path used for hierarchy recognition, and finding a key
             * @param value A value that will be put into key/value pair
             * @param sep An optional separator symbol (default: ".")
             * 
             * <b>Example:</b>
             * @code
             * Hash hash;
             * hash.setFromPath("a.b.c.d", float(3.1415));  // key is 'd', value is 3.1415
             * hash.setFromPath("a.b.c1.d", 42);            // key is 'd', value is 42
             * @endcode
             */
            template <class T>
            void setFromPath(const std::string& path, const T& value, std::string sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) {
                    throw LOGIC_EXCEPTION("No path (nested key value given)");
                } else if (nElements == 1) {
                    std::string key = v[0];
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(key);
                    if (arrayType.get < 0 > () == true) {
                        setArrayElement(arrayType, value);
                    } else {
                        set<T > (key, value);
                    }
                } else {
                    std::string shorterPath = path.substr(path.find_first_of(sep) + 1);
                    std::string first = *(v.begin());
                    // Check for array type
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(first);
                    if (arrayType.get < 0 > () == true) {
                        Hash& tmp = setArrayElement<Hash > (arrayType);
                        tmp.setFromPath<T > (shorterPath, value, sep);
                    } else {
                        if (!this->has(first)) {
                            set(first, Hash());
                        }
                        if (!this->is<Hash > (first)) { // This overrides anything that was not Hash before
                            set(first, Hash());
                        }
                        get<Hash > (first).setFromPath<T > (shorterPath, value, sep);
                    }
                }
            }

            /**
             * Overloaded <b>setFromPath</b> with path, <i>const char*</i> value and, optionally, a separator.
             * @param path  A path used for hierarchy recognition, and finding a key
             * @param value A value that will be put into key/value pair
             * @param sep   An optional separator symbol (default: ".")
             * @return void
             * 
             * <b>Example:</b>
             * @code
             * Hash hash;
             * hash.setFromPath<const char*>("a.b.c.d", "some c-style string");
             * @endcode
             */
            void setFromPath(const std::string& path, const char* value, std::string sep = ".") {
                setFromPath(path, std::string(value), sep);
            }

            /**
             * Constructs an empty Hash object at given path
             * @param path
             * @return void
             */
            void setFromPath(const std::string & path) {
                setFromPath(path, Hash());
            }
            
            /**
             * The function checks if the path is associated with some object in hierarchical Hash
             * @param path  Path to the queried object
             * @param sep   The separator used for representing a hierarchy
             * @return   boolean value
             */
            bool hasFromPath(const std::string& path, std::string sep = ".") const {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) return false;
                const Hash* ctx = this;
                for (size_t i = 0; i < nElements; i++) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[i]);
                    if (arrayType.get < 0 > ()) {
                        std::string key = arrayType.get < 1 > (); // key
                        if (!ctx->has(key)) return false;
                        if (arrayType.get < 2 > () >= 0) {
                            size_t index = arrayType.get < 2 > (); // index
                            if (ctx->is<std::vector<Hash> >(key)) {
                                const std::vector<Hash>& tmp = ctx->get<std::vector<Hash> >(key);
                                if (index >= tmp.size()) return false;
                                ctx = &(ctx->get<std::vector<Hash> >(key)[index]);
                                continue;
                            } else
                                // this is vector but not a vector<Hash>!
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else if (arrayType.get < 2 > () == -1) { // Use last element
                            if (ctx->is<std::vector<Hash> >(key)) {
                                ctx = &(*(ctx->get<std::vector<Hash> > (key).rbegin()));
                                continue;
                            } else
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else {
                            return false;
                        }
                    } else {
                        if (!ctx->has(v[i])) return false;
                        if (ctx->is<Hash > (v[i])) {
                            ctx = &(ctx->get<Hash > (v[i]));
                            continue;
                        }
                    }
                    if (i == nElements - 1) return true;
                    return false;
                }
                return true;
            }

            /**
             * This function queries the Hash if the path is associated with the object of type <b>'type'</b>
             * @param path  Path to the queried object
             * @param type  Expected object type
             * @param sep   Separator used for representing a hierarchy in the Hash
             * @return true or false
             */
            bool isFromPath(const std::string& path, Types::Type type, const std::string& sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) return false;
                Hash* ctx = this;

                for (size_t i = 0; i < nElements; i++) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[i]);
                    if (arrayType.get < 0 > ()) {
                        std::string key = arrayType.get < 1 > (); // key
                        if (!ctx->has(key)) return false;
                        if (arrayType.get < 2 > () >= 0) {
                            size_t index = arrayType.get < 2 > (); // index
                            if (ctx->is(key, Types::VECTOR_HASH)) {
                                const std::vector<Hash>& tmp = ctx->get<std::vector<Hash> >(key);
                                if (index >= tmp.size()) return false;
                                if (i == nElements - 1 && type == Types::HASH) return true;
                                ctx = &(ctx->get<std::vector<Hash> >(key)[index]);
                                continue;
                            } else
                                // this is vector but not a vector<Hash>!
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else if (arrayType.get < 2 > () == -1) { // Use last element
                            if (ctx->is(key, Types::VECTOR_HASH)) {
                                if (i == nElements - 1 && type == Types::HASH) return true;
                                ctx = &(*(ctx->get<std::vector<Hash> > (key).rbegin()));
                                continue;
                            } else
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else {
                            return false;
                        }
                    } else {
                        if (!ctx->has(v[i])) return false;
                        if (ctx->is<Hash > (v[i])) {
                            if (i == nElements - 1 && type == Types::HASH) return true;
                            ctx = &(ctx->get<Hash > (v[i]));
                            continue;
                        }
                    }
                    if (i == nElements - 1) return ctx->is(v[i], type);
                }
                return false;
            }

            /**
             * This function queries the Hash if the path is associated with the object of type equal template parameter
             * @param path  Path to the queried object
             * @param sep   Separator used for representing a hierarchy in the Hash
             * @return true or false
             */
            template<typename T>
            bool isFromPath(const std::string& path, const std::string& sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) return false;
                Hash* ctx = this;
                for (size_t i = 0; i < nElements; i++) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[i]);
                    if (arrayType.get < 0 > ()) {
                        std::string key = arrayType.get < 1 > (); // key
                        if (!ctx->has(key)) return false;
                        if (arrayType.get < 2 > () >= 0) {
                            size_t index = arrayType.get < 2 > (); // index
                            if (ctx->is<std::vector<Hash> >(key)) {
                                const std::vector<Hash>& tmp = ctx->get<std::vector<Hash> >(key);
                                if (index >= tmp.size()) return false;
                                if (i == nElements - 1) return true; // last element
                                ctx = &(ctx->get<std::vector<Hash> >(key)[index]);
                                continue;
                            } else if (i == nElements - 1 && ctx->is<std::vector<T> >(key)) {
                                const std::vector<T>& tmp = ctx->get<std::vector<T> >(key);
                                if (index >= tmp.size()) return false;
                                return true;
                            } else
                                return false;
                        } else if (arrayType.get < 2 > () == -1) { // Use last element
                            if (ctx->is<std::vector<Hash> >(key)) {
                                if (i == nElements - 1) return true;
                                ctx = &(*(ctx->get<std::vector<Hash> > (key).rbegin()));
                                continue;
                            } else if (i == nElements - 1)
                                return ctx->is<std::vector<T> >(key);
                            else
                                return false;
                        } else {
                            return false;
                        }
                    } else {
                        if (!ctx->has(v[i])) return false;
                        if (i == nElements - 1) return ctx->is<T > (v[i]);
                        ctx = &(ctx->get<Hash > (v[i]));
                        continue;
                    }
                }
                return false;
            }

            /**
             * Erase leaf of Hash tree represented by path
             * @param path Path to the queried object
             * @param sep Separator used for representing a tree in the Hash
             * @return 0 if no erase and 1 if the leaf was erased
             */
            std::size_t eraseFromPath(const std::string& path, const std::string& sep = ".") {
                std::string p(path);
                std::vector<std::string> v;
                boost::trim(p);
                boost::split(v, p, boost::is_any_of(sep));
                size_t nElements = v.size();
                if (nElements == 0) return 0;
                Hash* ctx = this;
                for (size_t i = 0; i < nElements; i++) {
                    boost::tuple<bool, std::string, int> arrayType = checkKeyForArrayType(v[i]);
                    if (arrayType.get < 0 > ()) {
                        std::string key = arrayType.get < 1 > (); // key
                        if (!ctx->has(key)) return 0;
                        if (arrayType.get < 2 > () >= 0) {
                            size_t index = arrayType.get < 2 > (); // index
                            if (ctx->is<std::vector<Hash> >(key)) {
                                std::vector<Hash>& tmp = ctx->get<std::vector<Hash> >(key);
                                if (index >= tmp.size()) return 0;
                                if (i == nElements - 1) { // last element in path
                                    tmp.erase(tmp.begin() + index);
                                    return 1;
                                }
                                ctx = &(ctx->get<std::vector<Hash> >(key)[index]);
                                continue;
                            } else
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else if (arrayType.get < 2 > () == -1) { // Use 'last' element
                            if (ctx->is<std::vector<Hash> >(key)) {
                                if (i == nElements - 1) {
                                    std::vector<Hash>& tmp = ctx->get<std::vector<Hash> >(key);
                                    tmp.erase(tmp.begin() + tmp.size()-1);
                                    return 1;
                                }
                                ctx = &(*(ctx->get<std::vector<Hash> > (key).rbegin()));
                                continue;
                            } else
                                throw NOT_SUPPORTED_EXCEPTION("The square bracket syntax may be used only for vector<Hash>!");
                        } else {
                            return 0;
                        }
                    } else {
                        if (!ctx->has(v[i])) return 0;
                        if (i == nElements - 1) return ctx->erase(v[i]);
                        ctx = &(ctx->get<Hash > (v[i]));
                        continue;
                    }
                }
                return 0;
            }
            
            /**
             * Convenient output operator to console
             */
            DECLSPEC_UTIL friend std::ostream & operator<<(std::ostream& os, const Hash & param);


        protected:

            template <class T>
            static void r_leaves(const T& hash, std::vector<std::string>& leaves, std::string keyPath, const std::string & sep) {
                if (hash.empty()) {
                    leaves.push_back(keyPath);
                    return;
                }
                for (const_iterator it = hash.begin(); it != hash.end(); ++it) {
                    std::string currentKey;
                    if (keyPath.empty()) currentKey = it->first;
                    else currentKey = keyPath + sep + it->first;
                    if (hash.is<T > (it)) { // Recursion
                        r_leaves(hash.get<T > (it), leaves, currentKey, sep);
                    } else if (hash.is<std::vector<T> > (it)) { // Recursion for vector
                        for (size_t i = 0; i < hash.get<std::vector<T> > (it).size(); ++i) {
                            std::ostringstream os;
                            os << currentKey << "[" << i << "]";
                            r_leaves(hash.get<std::vector<T> > (it).at(i), leaves, os.str(), sep);
                        }
                    } else {
                        leaves.push_back(currentKey);
                    }
                }
            }


            virtual void r_toStream(std::ostream& os, const Hash& hash, int depth) const;

            virtual bool handleStandardTypes(std::ostream& os, const Hash& hash, const Hash::const_iterator& it,
                    const std::string & fill) const;

            template <class T >
            T & setArrayElement(const boost::tuple<bool, std::string, int>& arrayType) {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        set(key, std::vector<T > (index + 1, T()));
                    } else {
                        std::vector<T>& tmp = get<std::vector<T> >(key);
                        if (index >= tmp.size()) {
                            tmp.resize(index + 1, T());
                        }
                    }
                    return get<std::vector<T> > (key)[index];
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        set(key, std::vector<T > (1, T()));
                    }
                    return get<std::vector<T> > (key).back();
                } else if (arrayType.get < 2 > () == -2) { // Add new element
                    if (!this->has(key)) {
                        set(key, std::vector<T > (1, T()));
                        return get<std::vector<T> >(key).back();
                    } else {
                        std::vector<T>& tmpVector = get<std::vector<T> >(key);
                        tmpVector.push_back(T());
                        return tmpVector.back();
                    }
                } else {
                    throw LOGIC_EXCEPTION("Internal error whilst interpreting array operator, ask BH");
                }
            }

            template <class T>
            void setArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const T & value) {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        set(key, std::vector<T > (index + 1, T()));
                    } else {
                        std::vector<T>& tmp = get<std::vector<T> >(key);
                        if (index >= tmp.size()) {
                            tmp.resize(index + 1, T());
                        }
                    }
                    get<std::vector<T> > (key)[index] = value;
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        set(key, std::vector<T > (1, T()));
                    }
                    std::vector<T>& tmp = get<std::vector<T> >(key);
                    tmp[tmp.size() - 1] = value;
                } else if (arrayType.get < 2 > () == -2) { // Add new element
                    if (!this->has(key)) {
                        set(key, std::vector<T > (1, value));
                    } else {
                        std::vector<T>& tmpVector = get<std::vector<T> >(key);
                        tmpVector.push_back(value);
                    }
                }
            }

            /**
             * Necessary substitution in order to work around broken vector<bool>
             */
            void setArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const bool& value) {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        set(key, std::deque<bool > (index + 1, false));
                    } else {
                        std::deque<bool>& tmp = get < std::deque<bool> >(key);
                        if (index >= tmp.size()) {
                            tmp.resize(index + 1, false);
                        }
                    }
                    get < std::deque<bool> > (key)[index] = value;
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        set(key, std::deque<bool > (1, false));
                    }
                    std::deque<bool>& tmp = get < std::deque<bool> >(key);
                    tmp[tmp.size() - 1] = value;
                } else if (arrayType.get < 2 > () == -2) { // Add new element
                    if (!this->has(key)) {
                        set(key, std::deque<bool > (1, false));
                    } else {
                        std::deque<bool>& tmpVector = get < std::deque<bool> >(key);
                        tmpVector.push_back(value);
                    }
                }
            }

            /*
             * Retrieves a non-const reference to an vector<T> element.
             *
             * THIS APPROACH WILL NOT COMPILE if not placed into a specific context.
             *
             * This is because std::vector<bool> is broken in a sense that:
             *
             * vector<bool> vec(1,false);
             * bool& ref = vec.front();
             *
             * will not compile. The std::vector<bool> simply DOES NOT play the standard stl rules.
             * See: http://www.cplusplus.com/reference/stl/vector/
             *
             * The only way to fix this problem is to use a different container type (e.g. deque) for boolean values.
             * Consequently this (member-)function has to be explicitely specialized. This however is technically not possible
             * in C++. The way to do, is to give the compiler a signature of the boolean version of this function which
             * than gets used and the instantiation of the templated version is skipped
             * (i.e. the bogus vector<bool> version gets never generated and compiled).
             *
             * In order to the use the differently instantiated functions, a direct client call involving the syntax of explicitely
             * naming the templated type (i.e. function<bool>(doSomething);) is not possible. As the templated type is part of
             * the return type only, this is however technically not possible in C++. The final solution to that is to have
             * a dummy argument for automatic type deduction. This is what is implemented here.
             *
             * We end up having put to 4 functions (const and non-const context, explicit bool version) serving the same purpose.
             * This is not dirty code BUT IS ESSENTIAL AND CRITICAL FOR FUNCTION!
             *
             */
            template <class T >
            T & getArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const T& dummy = T()) {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Addresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    } else {
                        const std::vector<T>& tmp = get<std::vector<T> >(key);
                        if (index >= tmp.size()) {
                            throw LOGIC_EXCEPTION("Array index " + String::toString(index) + " for array \"" + key + "\" is out of bounds.");
                        }
                    }
                    return get<std::vector<T> > (key)[index];
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    }
                    return *(get<std::vector<T> > (key).rbegin());
                } else {
                    throw LOGIC_EXCEPTION("Illegal trial to add new element in read-only context");
                }
            }

            template <class T>
            const T & getArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const T& dummy = T()) const {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    } else {
                        const std::vector<T>& tmp = get<std::vector<T> >(key);
                        if (index >= tmp.size()) {
                            throw LOGIC_EXCEPTION("Array index " + String::toString(index) + " for array \"" + key + "\" is out of bounds.");
                        }
                    }
                    return get<std::vector<T> > (key)[index];
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    }
                    return *(get<std::vector<T> > (key).rbegin());
                } else {
                    throw LOGIC_EXCEPTION("Illegal trial to add new element in read-only context");
                }
            }

            /**
             *  Necessary substitution in order to work around broken std::vector<bool>
             */
            const bool& getArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const bool& dummy) const {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    } else {
                        const std::deque<bool>& tmp = get < std::deque<bool> >(key);
                        if (index >= tmp.size()) {
                            throw LOGIC_EXCEPTION("Array index " + String::toString(index) + " for array \"" + key + "\" is out of bounds.");
                        }
                    }
                    return get < std::deque<bool> > (key)[index];
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    }
                    return *(get < std::deque<bool> > (key).rbegin());
                } else {
                    throw LOGIC_EXCEPTION("Illegal trial to add new element in read-only context");
                }
            }

            /**
             * Necessary substitution in order to work around broken vector<bool>
             */
            bool& getArrayElement(const boost::tuple<bool, std::string, int>& arrayType, const bool& dummy) {
                std::string key = arrayType.get < 1 > ();
                if (arrayType.get < 2 > () >= 0) { // Adresses an index
                    size_t index = arrayType.get < 2 > ();
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    } else {
                        std::deque<bool>& tmp = get < std::deque<bool> >(key);
                        if (index >= tmp.size()) {
                            throw LOGIC_EXCEPTION("Array index " + String::toString(index) + " for array \"" + key + "\" is out of bounds.");
                        }
                    }
                    return get < std::deque<bool> > (key)[index];
                } else if (arrayType.get < 2 > () == -1) { // Use last element
                    if (!this->has(key)) {
                        throw LOGIC_EXCEPTION("Array of name \"" + key + "\" does not exist.");
                    }
                    return *(get < std::deque<bool> > (key).rbegin());
                } else {
                    throw LOGIC_EXCEPTION("Illegal trial to add new element in read-only context");
                }
            }

            boost::tuple<bool, std::string, int> checkKeyForArrayType(const std::string & key) const;

            template <class T>
            static void r_flatten(const T& hash, T& flat, std::string keyPath, const std::string & sep) {
                if (hash.empty() && keyPath != "") {
                    flat[keyPath] = hash;
                } else {
                    for (typename T::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                        std::string currentKey;
                        if (keyPath.empty()) currentKey = it->first;
                        else currentKey = keyPath + sep + it->first;
                        if (hash.is<T > (it)) { // Recursion
                            r_flatten(hash.get<T > (it), flat, currentKey, sep);
                        } else {
                            flat[currentKey] = it->second;
                        }
                    }
                }
            }

        private: // functions

            std::string getCastFailureDetails(const std::string& key, const std::type_info& src, const std::type_info & tgt) const {
                const Types& types = Types::getInstance();
                std::string srcType = types.getTypeAsString(src);
                std::string tgtType = types.getTypeAsString(tgt);
                std::ostringstream str;
                str << "Failed conversion from \"" << srcType << "\" into \"";
                str << tgtType << "\" on key \"" << key << "\"";
                return str.str();
            }

            template<class T>
            void convertToVector(const std::string& key, const std::string & stringArray) {
                try {
                    std::vector<std::string> elements;
                    std::string tmp(stringArray);
                    boost::trim(tmp);
                    boost::split(elements, tmp, boost::is_any_of(","));
                    size_t size = elements.size();
                    std::vector<T> resultArray(size);
                    for (size_t i = 0; i < size; ++i) {
                        std::string element(elements[i]);
                        boost::trim(element);
                        resultArray[i] = boost::lexical_cast<T > (element);
                    }
                    this->set(key, resultArray);
                } catch (...) {
                    RETHROW;
                }
            }

            //special case for handling vectors of data types CHAR (char), INT8 (signed char), UINT8 (unsigned char)

            template<class T>
            void convertToVectorChar(const std::string& key, const std::string & stringArray) {
                try {
                    std::vector<std::string> elements;
                    std::string tmp(stringArray);
                    boost::trim(tmp);
                    boost::split(elements, tmp, boost::is_any_of(","));
                    size_t size = elements.size();
                    std::vector<T> resultArray(size);
                    for (size_t i = 0; i < size; ++i) {
                        std::string element(elements[i]);
                        boost::trim(element);
                        resultArray[i] = boost::numeric_cast<T > (boost::lexical_cast<int>(element));
                    }
                    this->set(key, resultArray);
                } catch (...) {
                    RETHROW;
                }
            }

            void convertToVectorBool(const std::string& key, const std::string & stringArray);

            // TODO Remove this later or make it available everywhere

            template<class T>
            void tryHarderToCastThis(const std::string& key, const std::string& value, Types::Type type) {
                T casted = boost::numeric_cast<T > (boost::lexical_cast<double>(value));
                set(key, casted);
                std::string typeString(Types::convert(type));
                std::cout << "### WARNING ###: Casted intermediate DOUBLE = " << value << " to final type "
                        << typeString << " = " << casted << " on key \"" << key << "\"" << std::endl;
            }

            bool castStringToBool(const std::string & s);
        };
    }
}



#endif
