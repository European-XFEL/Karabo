/*
 * $Id: String.cc 5321 2012-03-01 13:49:34Z heisenb $
 *
 * File:   String.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August 19, 2010, 8:14 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include "StringTools.hh"
#include "FromTypeInfo.hh"
#include "ToCppString.hh"

using namespace std;

namespace karabo {
    namespace util {


        std::string createCastFailureMessage(const std::string& key, const std::type_info& src, const std::type_info& tgt) {
            std::string srcType = Types::convert<FromTypeInfo, ToCppString > (src);
            std::string tgtType = Types::convert<FromTypeInfo, ToCppString > (tgt);
            return "Failed conversion from \"" + srcType + "\" into \"" + tgtType + "\" on key \"" + key + "\"";
        }


        std::string createCastFailureMessage(const std::string& key, const Types::ReferenceType& src, const Types::ReferenceType& tgt) {
            std::string srcType = ToType<ToCppString>::to(src);
            std::string tgtType = ToType<ToCppString>::to(tgt);
            return "Failed conversion from \"" + srcType + "\" into \"" + tgtType + "\" on key \"" + key + "\"";
        }


        int getAndCropIndex(std::string& str) {
            if (str.empty()) return -1;
            int len = str.length() - 1;
            if (str[len] == ']') {
                str[len] = 0;
                int pos = str.rfind('[');
                str[pos] = 0;
                len = atoi(str.c_str() + pos + 1);
                str.erase(pos);
                return len;
            }
            return -1;
        }
    }
}
