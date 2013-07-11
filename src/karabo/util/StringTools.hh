/*
 * $Id: String.hh 5315 2012-03-01 13:33:13Z heisenb $
 *
 * File:   String.hh
 * Author: <wp76@xfel.eu>
 *
 * Created on July 26, 2010, 9:29 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_STRING_HH
#define	KARABO_UTIL_STRING_HH

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <complex>
#include <cstdlib>
#include <bitset>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "Types.hh"
#include "Exception.hh"

namespace karabo {
    namespace util {

        std::string createCastFailureMessage(const std::string& key, const std::type_info& src, const std::type_info& tgt);
        std::string createCastFailureMessage(const std::string& key, const Types::ReferenceType& src, const Types::ReferenceType& tgt);

        template <class T>
        inline std::string toString(const T& value) {
            std::ostringstream s;
            s << std::fixed << value;
            return s.str();
        }

        inline std::string toString(const float& value) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(7) << value;
            return s.str();
        }

        inline std::string toString(const double& value) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(15) << value;
            return s.str();
        }

        inline std::string toString(const std::complex<float>& value) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(7) << value;
            return s.str();
        }

        inline std::string toString(const std::complex<double>& value) {
            std::ostringstream s;
            s << std::fixed << std::setprecision(15) << value;
            return s.str();
        }

        inline std::string toString(const std::string& value) {
            return value;
        }

        inline std::string toString(const char* const& value) {
            return std::string(value);
        }

        inline std::string toString(const unsigned char value) {
            std::ostringstream s;
            s << static_cast<unsigned int> (value);
            return s.str();
        }

        inline std::string toString(const signed char value) {
            std::ostringstream s;
            s << static_cast<int> (value);
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

        template <typename T>
        inline std::string toString(const std::vector<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::vector<T>::const_iterator it = value.begin();
            s << toString(*it);
            it++;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        inline std::string toString(const std::vector<std::string> value) {
            if (value.empty()) return "";
            std::ostringstream s;
            std::vector<std::string>::const_iterator it = value.begin();
            if (it->find_first_of(',') != std::string::npos) throw KARABO_NOT_SUPPORTED_EXCEPTION("Comma separator within in string array element is not supported during casting");
            s << *it;
            it++;
            for (; it != value.end(); ++it) {
                s << "," << *it;
            }
            return s.str();
        }

        template <typename T>
        inline std::string toString(const std::set<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::set<T>::const_iterator it = value.begin();
            s << toString(*it);
            it++;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        template <typename T>
        inline std::string toString(const std::deque<T>& value) {
            if (value.empty()) return "";
            std::ostringstream s;
            typename std::deque<T>::const_iterator it = value.begin();
            s << toString(*it);
            it++;
            for (; it != value.end(); ++it) {
                s << "," << toString(*it);
            }
            return s.str();
        }

        template <typename KeyType, typename ValueType>
        inline std::string toString(const std::map<KeyType, ValueType>& value) {
            if (value.empty()) return "{}";
            std::ostringstream s;
            typename std::map<KeyType, ValueType>::const_iterator it = value.begin();
            s << "{" << toString(it->first) << ":" << toString(it->second);
            it++;
            for (; it != value.end(); ++it) {
                s << "," << toString(it->first) << ":" << toString(it->second);
            }
            s << "}";
            return s.str();
        }

        template <class T>
        inline T fromString(const std::string& value) {
            return boost::lexical_cast<T > (value);
        }

        template<typename T,
        template <typename ELEM, typename = std::allocator<ELEM> > class CONT>
        inline CONT<T> fromString(const std::string& value, const std::string& separator = ",") {
            try {
                CONT<std::string> elements;
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
                    std::string element(elements[i]);
                    boost::trim(element);
                    resultArray[i] = util::fromString<T > (element);
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return CONT<T>(); // Make the compiler happy
            }
        }

        template<typename T,
        template <typename ELEM, typename = std::less<ELEM>, typename = std::allocator<ELEM> > class CONT>
        inline CONT<T> fromString(const std::string& value, const std::string& separator = ",") {
            try {
                CONT<std::string> elements;
                std::string tmp(value);
                boost::trim(tmp);
                if (tmp[0] == '[' && tmp[tmp.size() - 1] == ']') {
                    tmp = tmp.substr(1);
                    tmp.erase(tmp.size() - 1);
                }
                boost::split(elements, tmp, boost::is_any_of(separator), boost::token_compress_on);
                CONT<T> resultArray;
                for (typename CONT<T>::iterator it = elements.begin(); it != elements.end(); ++it) {
                    std::string element(*it);
                    boost::trim(element);
                    resultArray.insert(util::fromString<T > (element));
                }
                return resultArray;
            } catch (...) {
                KARABO_RETHROW;
                return CONT<T>(); // Make the compiler happy
            }
        }

        template<> inline char fromString<char>(const std::string& value) {
            return boost::numeric_cast<char>(boost::lexical_cast<int>(value));
        }

        template<> inline signed char fromString<signed char>(const std::string& value) {
            return boost::numeric_cast<signed char>(boost::lexical_cast<int>(value));
        }

        template<> inline float fromString<float>(const std::string& value) {
            if (value == "nan" || value == "-nan") return std::numeric_limits<float>::quiet_NaN();
            else return boost::lexical_cast<float>(value);
        }

        template<> inline double fromString<double>(const std::string& value) {
            if (value == "nan" || value == "-nan") return std::numeric_limits<double>::quiet_NaN();
            else return boost::lexical_cast<double>(value);
        }

        template<> inline bool fromString<bool>(const std::string& value) {
            std::string val = boost::to_lower_copy(value);
            bool boolVal;
            if (val == "n" || val == "no" || val == "false" ||
                    val == "0") {
                boolVal = false;
            } else if (val == "y" || val == "yes" || val == "1" ||
                    val == "true") {
                boolVal = true;
            } else {
                throw KARABO_CAST_EXCEPTION("Cannot interprete \"" + val + "\" as boolean.");
            }
            return boolVal;
        }

        /**
         * This class is taken from http://www.c-plusplus.de/forum/168607
         */
        template<class E,
        class T = std::char_traits<E>,
        class Sub1 = std::allocator<E> >

        class Widen : public std::unary_function<
        const std::string&, std::basic_string<E, T, Sub1> > {

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
                pCType_ = &std::use_facet<std::ctype<E> >(loc);
                #endif
            }

            // Conversion...

            std::basic_string<E, T, Sub1> operator() (const std::string& str) const {
                typename std::basic_string<E, T, Sub1>::size_type srcLen =
                        str.length();
                const char* pSrcBeg = str.c_str();
                std::vector<E> tmp(srcLen);

                pCType_->widen(pSrcBeg, pSrcBeg + srcLen, &tmp[0]);
                return std::basic_string<E, T, Sub1 > (&tmp[0], srcLen);
            }
        };


        int getAndCropIndex(std::string& str);

        //-----------------------------------------------------------------------------------------------
        //
        // http://stackoverflow.com/questions/5505965/fast-string-splitting-with-multiple-delimiters
        //
        //-----------------------------------------------------------------------------------------------

        template<typename container>
        inline void tokenize(const std::string& inputString, container& tokens, char const* delimiters) {
            container output;

            std::bitset < 255 > delims;
            while (*delimiters) {
                unsigned char code = *delimiters++;
                delims[code] = true;
            }
            typedef std::string::const_iterator iter;
            iter beg;
            bool in_token = false;
            for (std::string::const_iterator it = inputString.begin(), end = inputString.end();
                    it != end; ++it) {
                if (delims[*it]) {
                    if (in_token) {
                        output.push_back(typename container::value_type(beg, it));
                        in_token = false;
                    }
                } else if (!in_token) {
                    beg = it;
                    in_token = true;
                }
            }
            if (in_token) {
                output.push_back(typename container::value_type(beg, inputString.end()));
            }
            output.swap(tokens);
        }

        template <typename container>
        inline void tokenize(const std::string& inputString, container& tokens, const char delimiter) {
            const char delims[] = {delimiter, 0};
            tokenize(inputString, tokens, delims);
        }


        // TODO Move to other file

        inline bool isBigEndian() {

            union {

                uint32_t i;
                char c[4];
            } bint = {0x01020304};
            return bint.c[0] == 1;
        }
    }
}

#endif	/* KARABO_UTIL_STRING_HH */

