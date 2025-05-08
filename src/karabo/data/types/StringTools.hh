/*
 * $Id: String.hh 5315 2012-03-01 13:33:13Z heisenb $
 *
 * File:   String.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 26, 2010, 9:29 AM
 *
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


#ifndef KARABO_DATA_TYPES_STRINGTOOLS_HH
#define KARABO_DATA_TYPES_STRINGTOOLS_HH

#include <bitset>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <complex>
#include <cstdlib>
#include <deque>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "Base64.hh"
#include "Exception.hh"
#include "State.hh"
#include "Types.hh"

namespace {
    // A string that does not have any of these characters
    // is not a floating point number. ',' is added in case
    // German locales are used
    const char floating_point_chars[] = "eE.,";
} // namespace

namespace karabo {
    namespace data {

        class NDArray;

        /**
         * Create a cast failure message for a Hash key when trying to cast to a different value type
         * @param key the message pertains to
         * @param src typeinfo of source type, i.e. type of the value in the Hash
         * @param tgt typeinfo of target type, i.e. type to which the cast failed
         * @return the failure message
         */
        std::string createCastFailureMessage(const std::string& key, const std::type_info& src,
                                             const std::type_info& tgt);
        std::string createCastFailureMessage(const std::string& key, const Types::ReferenceType& src,
                                             const Types::ReferenceType& tgt);

        /**
         * @brief Creates a type mismatch message for an attempt to get a Hash node value with a type different from the
         * current type of the node.
         *
         * @param key the key of the node whose value has been requested.
         * @param srcType the current type of the node whose value should have been retrieved.
         * @param tgtType the expected type of the node whose value should have been retrieved
         * @return std::string the failure message.
         */
        std::string createTypeMismatchMessage(const std::string& key, const std::type_info& srcType,
                                              const std::type_info& tgtType);

        // inplace convert to lowercase
        inline std::string& toLower(std::string& str) {
            std::transform(str.begin(), str.end(), str.begin(),
                           [](char ch) { return static_cast<char>((ch >= 'A' && ch <= 'Z') ? ch + ('a' - 'A') : ch); });
            return str;
        }

        // inplace convert to uppercase
        inline std::string& toUpper(std::string& str) {
            std::transform(str.begin(), str.end(), str.begin(),
                           [](char ch) { return static_cast<char>((ch >= 'a' && ch <= 'z') ? ch - ('a' - 'A') : ch); });
            return str;
        }

        // inplace trim spaces
        inline std::string& trim(std::string& str) {
            const char* spaces = " \n\r\t";
            str.erase(str.find_last_not_of(spaces) + 1);
            str.erase(0, str.find_first_not_of(spaces));
            return str;
        }


        /**
         * Split input string using delimiter (default = space) into vector of strings.
         * @param str input string to split
         * @param dl delimiter character used for splitting (default = " ")
         * @param maxsplit max size of resulting vector ... (default = 0: unlimited)
         * @return vector of splitted parts of input string
         */
        std::vector<std::string> split(std::string& str, const char* dl = " ", std::size_t maxsplit = 0);

        /**
         * Return a string representation of a value of type T. Overloads for
         * common value types exist. In general std::ostream is used for output
         * so it will work for any type supporting the "<<" operator or supported by
         * std::ostream
         *
         * @param value
         * @return
         */
        template <class T>
        inline std::string toString(const T& value) {
            std::ostringstream s;
            s << std::fixed << value;
            return s.str();
        }

        /**
         * Floats are output to their maximum precision of 7 digits
         * @param value
         * @return
         */
        inline std::string toString(const float& value) {
            std::ostringstream s;
            s << boost::format("%.7g") % value;
            return s.str();
        }

        /**
         * Doubles are output to their maximum precision of 15 digits
         * @param value
         * @return
         */
        inline std::string toString(const double& value) {
            std::ostringstream s;
            s << boost::format("%.15g") % value;
            return s.str();
        }

        /**
         * Complex floats are output to their maximum precision of 7 digits
         * @param value
         * @return
         */
        inline std::string toString(const std::complex<float>& value) {
            std::ostringstream s;
            s << boost::format("%.7g") % value;
            return s.str();
        }

        /**
         * Complex doubles are output to their maximum precision of 15 digits
         * @param value
         * @return
         */
        inline std::string toString(const std::complex<double>& value) {
            std::ostringstream s;
            s << boost::format("%.15g") % value;
            return s.str();
        }

        inline std::string toString(const std::string& value) {
            return value;
        }

        inline std::string toString(const char* const& value) {
            return std::string(value ? value : "");
        }

        inline std::string toString(const unsigned char value) {
            std::ostringstream s;
            s << static_cast<unsigned int>(value);
            return s.str();
        }

        inline std::string toString(const signed char value) {
            std::ostringstream s;
            s << static_cast<int>(value);
            return s.str();
        }

        inline std::string toString(const wchar_t* const& value) {
            char tmp[4096];
            wcstombs(tmp, value, 4096);
            return std::string(tmp);
        }

        inline std::string toString(const std::wstring& value) {
            return toString(value.c_str());
        }

        inline std::string toString(const karabo::data::CppNone& value) {
            return std::string("None");
        }

        /**
         * Vector values are output as a comma separated list, where the StringTools::toString
         * method for their value type T defines the representation of each element
         * @param value is the vector to be converted
         * @param maxElementsShown is the maximum number of vector elements treated. If value.size() is larger,
         *                         skip elements in the middle. Default is 0 which means _no_ skipping of elements.
         * @return
         */
        template <typename T>
        inline std::string toString(const std::vector<T>& value, size_t maxElementsShown = 0) {
            if (value.empty()) return "";

            std::ostringstream s;
            const size_t size = value.size();
            s << toString(value[0]);
            if (maxElementsShown == 0) {
                maxElementsShown = std::numeric_limits<size_t>::max();
            }
            // If size > maxElementsShown, show only a few less than first and last (maxElementsShown / 2) values.
            // Otherwise string for (maxElementsShown - 1) elements is longer than for maxElementsShown elements,
            // due to adding how many elements are skipped.
            const size_t numElementsBeginEnd = std::max(maxElementsShown / 2, static_cast<size_t>(6)) - 5;
            size_t index = 1;
            for (; index < size; ++index) {
                // If vector is too long, jump to last elements, but state how many we skip:
                if (size > maxElementsShown && index == numElementsBeginEnd) {
                    s << ",...(skip " << size - 2 * numElementsBeginEnd << " values)...";
                    index = size - numElementsBeginEnd;
                }
                s << "," << toString(value[index]);
            }
            return s.str();
        }

        /**
         * Byte array is output as a hexadecimal string (short)
         * @param value   ByteArray object
         * @param maxBytesShown  max. number of bytes to be presented
         * @return string representation of ByteArray
         */
        std::string toString(const karabo::data::ByteArray& value, size_t maxBytesShown = 0);

        /**
         * String vector values are output as a comma separated list. The individual strings in the list
         * may not contain comma (,) separators, e.g. an element "Hello, World" is not allowed as it would
         * yield a representation ambiguous with two elements "Hello" and "World" -> "Hello, World".
         * @param value
         * @return
         */
        inline std::string toString(const std::vector<std::string>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            std::vector<std::string>::const_iterator it = value.begin();
            if (it->find_first_of(',') != std::string::npos)
                throw KARABO_NOT_SUPPORTED_EXCEPTION(
                      "Comma separator within in string array element is not supported during casting");
            s << *it;
            ++it;
            for (; it != value.end(); ++it) {
                s << "," << *it;
            }
            return s.str();
        }

        /**
         * Convert vector<char> to string
         *
         * As this is raw data, it is base64 encoded
         */
        inline std::string toString(const std::vector<char>& value) {
            return karabo::data::base64Encode(reinterpret_cast<const unsigned char*>(&value[0]), value.size());
        }

        /**
         * Pointers with size information given as a std::pair of pointer and size are output like
         * vectors (see StringTools::toString(const std::vector<T>&) )
         * @param value
         * @return
         */
        template <typename T>
        inline std::string toString(const std::pair<const T*, size_t>& value) {
            if (value.second == 0) return "";
            const T* ptr = value.first;
            std::ostringstream s;
            s << toString(ptr[0]);
            for (size_t i = 1; i < value.second; ++i) {
                s << "," << toString(ptr[i]);
            }
            return s.str();
        }

        inline std::string toString(const std::pair<const char*, size_t>& value) {
            if (value.second == 0) return "";
            return karabo::data::base64Encode(reinterpret_cast<const unsigned char*>(value.first), value.second);
        }

        /**
         * An NDArray is output in a flattened representation using StringTools::toString(const std::pair<const T*,
         * size_t>&)
         * @param value
         * @return
         */
        std::string toString(const karabo::data::NDArray& value);

        /**
         * A std::set is output as a list of comma (,) separated values
         * @param value
         * @return
         */
        template <typename T>
        inline std::string toString(const std::set<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::set<T>::const_iterator it = value.begin();
            s << toString(*it);
            ++it;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        /**
         * A std::unordered_set is output as a list of comma (,) separated values
         * @param value
         * @return
         */
        template <typename T>
        inline std::string toString(const std::unordered_set<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::unordered_set<T>::const_iterator it = value.begin();
            s << toString(*it);
            ++it;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        /**
         * A std::deque is output as a list of comman (,) separated values
         * @param value
         * @return
         */
        template <typename T>
        inline std::string toString(const std::deque<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::deque<T>::const_iterator it = value.begin();
            s << toString(*it);
            ++it;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        /**
         * A std::map is output in the form { key1 : value1, key2: value2, ... }
         * @param value
         * @return
         */
        template <typename KeyType, typename ValueType>
        inline std::string toString(const std::map<KeyType, ValueType>& value) {
            if (value.empty()) return "{}";
            std::ostringstream s;
            typename std::map<KeyType, ValueType>::const_iterator it = value.begin();
            s << "{" << toString(it->first) << ":" << toString(it->second);
            ++it;
            for (; it != value.end(); ++it) {
                s << "," << toString(it->first) << ":" << toString(it->second);
            }
            s << "}";
            return s.str();
        }

        /**
         * States are output using their stringified name
         * @param value
         * @return
         */
        inline std::string toString(const karabo::data::State& value) {
            return value.name();
        }

        /**
         * The generic fromString method tries to obtain a value of type T using a
         * boost::lexical_cast of the passed value.
         * @param value
         * @return
         */
        template <class T>
        inline T fromString(const std::string& value) {
            return boost::lexical_cast<T>(value);
        }

        /**
         * For integer return values the lowever overhead method strtol is used
         * @param value
         * @return
         */
        template <>
        inline int fromString(const std::string& value) {
            // Calling stoi directly would fail for numbers in scientific notation
            // (it stops after the '.' for 1.6e-1)
            if (value.find_first_of(floating_point_chars) != std::string::npos) {
                return static_cast<int>(std::stod(value, nullptr));
            }

            return std::stoi(value, nullptr, 0);
        }

        template <>
        inline unsigned int fromString(const std::string& value) {
            // Calling stoul directly would fail for numbers in scientific notation
            // (it stops after the '.' for 1.6e-1)
            if (value.find_first_of(floating_point_chars) != std::string::npos) {
                return static_cast<unsigned int>(std::stod(value, nullptr));
            }

            return std::stoul(value, nullptr, 0);
        }

        template <>
        inline long long fromString(const std::string& value) {
            // Calling stoll directly would fail for numbers in scientific notation
            // (it stops after the '.' for 1.6e-1)
            if (value.find_first_of(floating_point_chars) != std::string::npos) {
                return static_cast<long long>(std::stod(value, nullptr));
            }

            return std::stoll(value, nullptr, 0);
        }

        template <>
        inline unsigned long long fromString(const std::string& value) {
            // Calling stoull directly would fail for numbers in scientific notation
            // (it stops after the '.' for 1.6e-1)
            if (value.find_first_of(floating_point_chars) != std::string::npos) {
                return static_cast<unsigned long long>(std::stod(value, nullptr));
            }

            return std::stoull(value, nullptr, 0);
        }

        /**
         * A string "None" can be cast to karabo::data::CppNone. Any other
         * string representation may not!
         * @param value
         * @return
         */
        template <>
        inline karabo::data::CppNone fromString(const std::string& value) {
            std::string tmp(value);
            boost::trim(tmp);
            if (tmp != "None") throw KARABO_CAST_EXCEPTION("Cannot interprete \"" + value + "\" as None.");
            return karabo::data::CppNone();
        }

        /**
         * Bytearrays can be constructed from strings where each character in the string represents
         * a byte (char) in the array
         * @param value
         * @return
         */
        template <>
        inline karabo::data::ByteArray fromString(const std::string& value) {
            std::vector<unsigned char> array;
            karabo::data::base64Decode(value, array);

            const size_t byteSize = array.size();
            std::shared_ptr<char> data(new char[byteSize], std::default_delete<char[]>());
            std::memcpy(data.get(), reinterpret_cast<char*>(&array[0]), byteSize);
            return karabo::data::ByteArray(data, byteSize);
        }

        /**
         * Sequence type elements can be constructed from strings of the form
         *
         *  [ value1, value2, ..., valueN ]
         *
         * where the enclosing brackets ([]) are optional and other separators may be specified.
         * The sequence elements must have a StringTools:fromString method for their type T
         * and each element must be castable to T using this method.
         * @param value
         * @param separator if separator other than the comma (,) is used
         * @return
         */
        template <typename T,
                  template <typename ELEM, typename = std::allocator<ELEM>> class CONT> // e.g. for vector container
        inline CONT<T> fromString(const std::string& value, const std::string& separator = ",") {
            try {
                if (value.empty()) return CONT<T>();
                CONT<std::string, std::allocator<std::string>> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                size_t size = elements.size();
                CONT<T> resultArray(size);
                for (size_t i = 0; i < size; ++i) {
                    std::string& element(elements[i]);
                    boost::trim(element);
                    resultArray[i] = data::fromString<T>(element);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return CONT<T>(); // Make the compiler happy
            }
        }

        /**
         * XXX: This function is working around the surprising behavior of fromstring<unsigned char>(value, sep) seen
         above. The long-term solution should be to remove the base64 encoding/decoding in toString/fromString. However,
         we need to discover which code is expecting that behavior before making such a change.
         NOTE: Karabo 3 should have fixed that issue - probably except for T=char

           In the meantime, we can use this simple version for schema options parsing.
         */
        template <class T>
        inline std::vector<T> fromStringForSchemaOptions(const std::string& value, const std::string& sep) {
            std::vector<T> converted;
            if (!value.empty()) {
                const std::vector<std::string> items = karabo::data::fromString<std::string, std::vector>(value, sep);
                converted.reserve(items.size());
                for (const std::string& item : items) {
                    converted.push_back(karabo::data::fromString<T>(item));
                }
            }
            return converted;
        }

        /**
         * Convert a string to vector<char>
         *
         * Since vector<char> is raw data, conversion to string does base64 encoding
         * which is reverted in this function.
         *
         * @param value the (base64 encoded) string to be converted
         * @param second argument is ignored
         * @return
         */
        template <>
        inline std::vector<char> fromString(const std::string& value, const std::string&) {
            std::vector<char> tmp;
            std::vector<unsigned char>* casted = reinterpret_cast<std::vector<unsigned char>*>(&tmp);
            karabo::data::base64Decode(value, *casted);
            return tmp;
        }

        /**
         * Convert a string to vector<unsigned char>
         *
         * Since before Karabo 3 vector<unsigned char> was erroneously base64 encoded,
         * a simple detection of such strings is done and then base64 decoding applied.
         *
         * @param value the string to be converted, i.e. numbers separated by 'sep'
         * @param sep a string containing any character separating two numbers
         * @return
         */
        template <>
        inline std::vector<unsigned char> fromString(const std::string& value, const std::string& sep) {
            std::vector<unsigned char> result;
            if (value.find_first_of(sep, 1) == std::string::npos && value.size() > 3ul) {
                if (sep.find_first_of(data::b64_char) != std::string::npos) {
                    throw KARABO_CAST_EXCEPTION("Separator (" + sep + ") contains a base64 encoding character");
                }
                // Old, data, i.e. vector<unsigned char> stringified by Karabo 2
                // since no 'sep' in it and longer than 3 characters, the max amount of digits of single unsigned char
                karabo::data::base64Decode(value, result);
            } else {
                try {
                    // Avoiding copy of data by re-interpret.
                    // Does not work for values in string larger than the signed char max (std::bad_cast)
                    std::vector<signed char> tmp = fromString<signed char, std::vector>(value, sep);
                    std::vector<unsigned char>& castedTmp = *reinterpret_cast<std::vector<unsigned char>*>(&tmp);
                    result.swap(castedTmp);
                } catch (const karabo::data::PropagatedException& e) {
                    if (e.userFriendlyMsg(false).find("bad numeric conversion") == std::string::npos) {
                        // If a propagated exception, we expect it to be caused by a bad lexical cast
                        KARABO_RETHROW_MSG("Expected an std::bad_cast to be the reason, but is " +
                                           e.userFriendlyMsg(false));
                    }
                    // Let's go via an inefficient copy:
                    std::vector<unsigned short> tmp = fromString<unsigned short, std::vector>(value, sep);
                    result.reserve(tmp.size());
                    for (unsigned short shortVal : tmp) {
                        result.push_back(static_cast<unsigned char>(shortVal));
                    }
                }
            }
            return result;
        }

        //
        // The method below has been renamed as part of the migration from C++14 to C++17.
        // In C++17, matching of template template arguments started taking default template arguments
        // into consideration. Due to this change, this template overload started clashing with the
        // one on line 458. Before C++ 17 this overload would match sorted containers like sets and the
        // one on line 458 would match containers like vectors. With C++ 17, some existing uses became
        // ambiguous (e.g. "std::set<std::string> tagSet = fromString<std::string, std::set>(tags, sep);"
        // from line 42 of file HashFilter.cc).
        //
        // Details about this change of behavior in C++ 17 can be seem
        // https://developers.redhat.com/articles/2021/08/06/porting-your-code-c17-gcc-11#new_template_template_parameter_matching
        //
        template <typename T, template <typename ELEM, typename = std::less<ELEM>,
                                        typename = std::allocator<ELEM>>
                              class CONT> // e.g. for set
        inline CONT<T> fromStringToSortedCont(const std::string& value, const std::string& separator = ",") {
            try {
                if (value.empty()) return CONT<T>();
                CONT<std::string, std::less<std::string>> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                CONT<T> resultArray;
                for (auto it = elements.begin(); it != elements.end(); ++it) {
                    std::string element(*it);
                    boost::trim(element);
                    resultArray.insert(data::fromString<T>(element));
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return CONT<T>(); // Make the compiler happy
            }
        }

        template <>
        inline std::vector<int> fromString(const std::string& value, const std::string& separator) {
            try {
                if (value.empty()) return std::vector<int>();
                std::vector<std::string> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                std::vector<int> resultArray;
                resultArray.reserve(elements.size());
                for (std::vector<std::string>::iterator it = elements.begin(); it != elements.end(); ++it) {
                    std::string& element(*it);
                    boost::trim(element);
                    int val = strtol(element.c_str(), NULL, 0);
                    resultArray.push_back(val);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return std::vector<int>(); // Make the compiler happy
            }
        }

        template <>
        inline std::vector<unsigned int> fromString(const std::string& value, const std::string& separator) {
            try {
                if (value.empty()) return std::vector<unsigned int>();
                std::vector<std::string> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                std::vector<unsigned int> resultArray;
                resultArray.reserve(elements.size());
                for (std::vector<std::string>::iterator it = elements.begin(); it != elements.end(); ++it) {
                    std::string& element(*it);
                    boost::trim(element);
                    unsigned int val = strtoul(element.c_str(), NULL, 0);
                    resultArray.push_back(val);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return std::vector<unsigned int>(); // Make the compiler happy
            }
        }

        template <>
        inline std::vector<long long> fromString(const std::string& value, const std::string& separator) {
            try {
                if (value.empty()) return std::vector<long long>();
                std::vector<std::string> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                std::vector<long long> resultArray;
                resultArray.reserve(elements.size());
                for (std::vector<std::string>::iterator it = elements.begin(); it != elements.end(); ++it) {
                    std::string& element(*it);
                    boost::trim(element);
                    long long val = strtoll(element.c_str(), NULL, 0);
                    resultArray.push_back(val);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return std::vector<long long>(); // Make the compiler happy
            }
        }

        template <>
        inline std::vector<unsigned long long> fromString(const std::string& value, const std::string& separator) {
            try {
                if (value.empty()) return std::vector<unsigned long long>();
                std::vector<std::string> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                std::vector<unsigned long long> resultArray;
                resultArray.reserve(elements.size());
                for (std::vector<std::string>::iterator it = elements.begin(); it != elements.end(); ++it) {
                    std::string& element(*it);
                    boost::trim(element);
                    unsigned long long val = strtoull(element.c_str(), NULL, 0);
                    resultArray.push_back(val);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return std::vector<unsigned long long>(); // Make the compiler happy
            }
        }

        template <>
        inline unsigned char fromString<unsigned char>(const std::string& value) {
            return boost::numeric_cast<unsigned char>(boost::lexical_cast<int>(value));
        }

        template <>
        inline signed char fromString<signed char>(const std::string& value) {
            return boost::numeric_cast<signed char>(boost::lexical_cast<int>(value));
        }

        template <>
        inline float fromString<float>(const std::string& value) {
            if (value == "nan" || value == "-nan") return std::numeric_limits<float>::quiet_NaN();
            else return boost::lexical_cast<float>(value);
        }

        template <>
        inline double fromString<double>(const std::string& value) {
            if (value == "nan" || value == "-nan") return std::numeric_limits<double>::quiet_NaN();
            else return boost::lexical_cast<double>(value);
        }

        template <>
        inline bool fromString<bool>(const std::string& value) {
            std::string val = boost::to_lower_copy(value);
            bool boolVal;
            if (val == "n" || val == "no" || val == "false" || val == "0") {
                boolVal = false;
            } else if (val == "y" || val == "yes" || val == "1" || val == "true") {
                boolVal = true;
            } else {
                throw KARABO_CAST_EXCEPTION("Cannot interprete \"" + val + "\" as boolean.");
            }
            return boolVal;
        }

        /**
         * This class is taken from http://www.c-plusplus.de/forum/168607
         */
        template <class E, class T = std::char_traits<E>, class Sub1 = std::allocator<E>>

        class Widen {
            std::locale loc_;
            const std::ctype<E>* pCType_;

            // No copy-constructor, no assignment operator...
            Widen(const Widen&);
            Widen& operator=(const Widen&);

           public:
            // Constructor...

            Widen(const std::locale& loc = std::locale()) : loc_(loc) {
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6.0...
                using namespace std;
                pCType_ = &_USE(loc, ctype<E>);
#else
                pCType_ = &std::use_facet<std::ctype<E>>(loc);
#endif
            }

            // Conversion...

            std::basic_string<E, T, Sub1> operator()(const std::string& str) const {
                typename std::basic_string<E, T, Sub1>::size_type srcLen = str.length();
                const char* pSrcBeg = str.c_str();
                std::vector<E> tmp(srcLen);

                pCType_->widen(pSrcBeg, pSrcBeg + srcLen, &tmp[0]);
                return std::basic_string<E, T, Sub1>(&tmp[0], srcLen);
            }
        };


        int getAndCropIndex(std::string& str);


        /**
         * Split a string into its components separated by any of the given delimiters
         *
         * @param inputString
         * @param tokens output container - will never be empty
         * @param delimiters each char in this C-style string indicates the end of a substring
         */
        template <typename container>
        inline void tokenize(const std::string& inputString, container& tokens, char const* delimiters) {
            //-----------------------------------------------------------------------------------------------
            //
            // http://stackoverflow.com/questions/5505965/fast-string-splitting-with-multiple-delimiters
            //
            // (but simplified to avoid dropping of empty tokens)
            //-----------------------------------------------------------------------------------------------
            container output;

            std::bitset<255> delims;
            while (*delimiters) {
                unsigned char code = *delimiters++;
                delims[code] = true;
            }
            using iter = std::string::const_iterator;
            iter beg = inputString.begin();
            for (iter it = beg, end = inputString.end(); it != end;) {
                if (delims[*it & 0xff]) { // & 0xff ensures to be within length of delims
                    output.push_back(typename container::value_type(beg, it));
                    beg = ++it;
                } else {
                    ++it;
                }
            }

            output.push_back(typename container::value_type(beg, inputString.end()));

            output.swap(tokens);
        }

        /**
         * Split a string into its components separated by the given delimiter
         *
         * @param inputString
         * @param tokens output container - will never be empty
         * @param delimiter
         */
        template <typename container>
        inline void tokenize(const std::string& inputString, container& tokens, const char delimiter) {
            const char delims[] = {delimiter, 0};
            tokenize(inputString, tokens, delims);
        }


        // TODO Move to other file

        inline bool isBigEndian() {
            union {
                unsigned int i;
                char c[4];
            } bint = {0x01020304};
            return bint.c[0] == 1;
        }
    } // namespace data
} // namespace karabo

#endif /* KARABO_DATA_TYPES_STRINGTOOLS_HH */
