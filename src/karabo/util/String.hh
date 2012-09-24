/*
 * $Id: String.hh 5315 2012-03-01 13:33:13Z heisenb $
 *
 * File:   String.hh
 * Author: <your.email@xfel.eu>
 *
 * Created on July 26, 2010, 9:29 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_UTIL_STRING_HH
#define	EXFEL_UTIL_STRING_HH

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <locale>
#include <functional> 
#include <stdlib.h>

namespace exfel {
    namespace util {

        /**
         * A string utility class.
         */

        class String : public std::string {
        public:

            template <class T>
            static std::string toString(const T in, int width = -1, int precision = -1, char fillChar = ' ') {
                std::ostringstream s;
                if (width > -1) s.width(width);
                if (precision > -1) s.precision(precision);
                if (fillChar != ' ') s.fill(fillChar);
                s << in;
                return s.str();
            }

            static std::string toString(const float& in, int width = 0, int precision = 2) {
                std::ostringstream s;
                s.setf(std::ios::fixed);
                s.width(width + precision + 1);
                s.precision(precision);
                s << in;
                return s.str();
            }

            static std::string toString(const double& in, int width = 0, int precision = 2) {
                std::ostringstream s;
                s.setf(std::ios::fixed);
                s.width(width + precision + 1);
                s.precision(precision);
                s << in;
                return s.str();
            }

            static std::string toString(const wchar_t* in, size_t maxNBytes = 256) {
                char tmp[256];
                wcstombs(tmp, in, maxNBytes);
                return std::string(tmp);
            }

            static std::string toString(wchar_t* in, size_t maxNBytes = 256) {
                char tmp[256];
                wcstombs(tmp, in, maxNBytes);
                return std::string(tmp);
            }

            static std::string toString(const std::wstring& in) {
                return toString(in.c_str(), in.length());
            }

            template <class T>
            static std::string sequenceToString(const T & in) {
                std::ostringstream s;
                if (in.empty()) return "";
                typename T::const_iterator it = in.begin();
                s << (*it);
                it++;
                for (; it != in.end(); it++) {
                    s << "," << (*it);
                }
                return s.str();
            }
            
            // Currently specialized for vector container only
            // TODO Template the container type and the content type individually
            static std::string sequenceToString(const std::vector<unsigned char>& in) {
                std::ostringstream s;
                if (in.empty()) return "";
                std::vector<unsigned char>::const_iterator it = in.begin();
                s << static_cast<unsigned int>(*it);
                it++;
                for (; it != in.end(); it++) {
                    s << "," << static_cast<unsigned int>(*it);
                }
                return s.str();
            }
            
             // Currently specialized for vector container only
            // TODO Template the container type and the content type individually
            static std::string sequenceToString(const std::vector<signed char>& in) {
                std::ostringstream s;
                if (in.empty()) return "";
                std::vector<signed char>::const_iterator it = in.begin();
                s << static_cast<int>(*it);
                it++;
                for (; it != in.end(); it++) {
                    s << "," << static_cast<int>(*it);
                }
                return s.str();
            }
                       
            template <class T>
            static std::string mapKeyToString(const T & in) {
                std::ostringstream s;
                if (in.empty()) return "";
                typename T::const_iterator it = in.begin();
                s << it->first;
                it++;
                for (; it != in.end(); it++) {
                    s << "," << (it->first);
                }
                return s.str();
            }
        };

        template<> std::string String::toString(const char* in, int width, int precision, char fillChar);

        /**
         * This class is taken from http://www.c-plusplus.de/forum/168607
         */
        template<class E,
        class T = std::char_traits<E>,
        class A = std::allocator<E> >

        class Widen : public std::unary_function<
        const std::string&, std::basic_string<E, T, A> > {
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

            std::basic_string<E, T, A> operator() (const std::string& str) const {
                typename std::basic_string<E, T, A>::size_type srcLen =
                        str.length();
                const char* pSrcBeg = str.c_str();
                std::vector<E> tmp(srcLen);

                pCType_->widen(pSrcBeg, pSrcBeg + srcLen, &tmp[0]);
                return std::basic_string<E, T, A > (&tmp[0], srcLen);
            }
        };

    } // namespace util
} // namespace exfel

#endif	/* EXFEL_UTIL_STRING_HH */

