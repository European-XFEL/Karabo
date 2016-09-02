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
#include "NDArray.hh"

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


        std::string toString(const karabo::util::NDArray& value) {
            switch (value.getType()) {
                case Types::BOOL: {
                    const std::pair<const bool*, size_t> data(value.getData<bool>(), value.size());
                    return toString<bool>(data);
                }
                case Types::INT8: {
                    const std::pair<const signed char*, size_t> data(value.getData<signed char>(), value.size());
                    return toString<signed char>(data);
                }
                case Types::INT16: {
                    const std::pair<const signed short*, size_t> data(value.getData<signed short>(), value.size());
                    return toString<signed short>(data);
                }
                case Types::INT32: {
                    const std::pair<const int*, size_t> data(value.getData<int>(), value.size());
                    return toString<int>(data);
                }
                case Types::INT64: {
                    const std::pair<const long long*, size_t> data(value.getData<long long>(), value.size());
                    return toString<long long>(data);
                }
                case Types::UINT8: {
                    const std::pair<const unsigned char*, size_t> data(value.getData<unsigned char>(), value.size());
                    return toString<unsigned char>(data);
                }
                case Types::UINT16: {
                    const std::pair<const unsigned short*, size_t> data(value.getData<unsigned short>(), value.size());
                    return toString<unsigned short>(data);
                }
                case Types::UINT32: {
                    const std::pair<const unsigned int*, size_t> data(value.getData<unsigned int>(), value.size());
                    return toString<unsigned int>(data);
                }
                case Types::UINT64: {
                    const std::pair<const unsigned long long*, size_t> data(value.getData<unsigned long long>(), value.size());
                    return toString<unsigned long long>(data);
                }
                case Types::FLOAT: {
                    const std::pair<const float*, size_t> data(value.getData<float>(), value.size());
                    return toString<float>(data);
                }
                case Types::DOUBLE: {
                    const std::pair<const double*, size_t> data(value.getData<double>(), value.size());
                    return toString<double>(data);
                }
                default:
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Conversion to string not implemented for this NDArray type");
            }
        }
    }
}
