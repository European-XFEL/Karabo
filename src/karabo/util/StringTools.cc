/*
 * $Id: String.cc 5321 2012-03-01 13:49:34Z heisenb $
 *
 * File:   String.cc
 * Author: <your.email@xfel.eu>
 *
 * Created on August 19, 2010, 8:14 PM
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

#include "StringTools.hh"

#include "FromTypeInfo.hh"
#include "NDArray.hh"
#include "ToCppString.hh"

using namespace std;

namespace karabo {
    namespace util {

        static std::string typeNameFromTypeInfo(const std::type_info& typeInf) {
            std::string typeName = Types::convert<FromTypeInfo, ToCppString>(typeInf);
            if (Types::from<FromTypeInfo>(typeInf) == Types::ReferenceType::UNKNOWN) {
                // type_info::name() is implementation dependent - but is at least a hint
                ((typeName += " (std::type_info: ") += typeInf.name()) += ")";
            }
            return typeName;
        }

        std::string createCastFailureMessage(const std::string& key, const std::type_info& src,
                                             const std::type_info& tgt) {
            const std::string srcType = typeNameFromTypeInfo(src);
            const std::string tgtType = typeNameFromTypeInfo(tgt);
            return "Failed conversion from \"" + srcType + "\" into \"" + tgtType + "\" on key \"" + key + "\"";
        }


        std::string createCastFailureMessage(const std::string& key, const Types::ReferenceType& src,
                                             const Types::ReferenceType& tgt) {
            std::string srcType = ToType<ToCppString>::to(src);
            std::string tgtType = ToType<ToCppString>::to(tgt);
            return "Failed conversion from \"" + srcType + "\" into \"" + tgtType + "\" on key \"" + key + "\"";
        }

        std::string createTypeMismatchMessage(const std::string& key, const std::type_info& srcType,
                                              const std::type_info& tgtType) {
            const std::string srcTypeName = typeNameFromTypeInfo(srcType);
            const std::string tgtTypeName = typeNameFromTypeInfo(tgtType);
            return "Value for key \"" + key + "\" has type \"" + srcTypeName +
                   "\". It can't be read as being of type \"" + tgtTypeName + "\".";
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


        std::vector<std::string> split(std::string& str, const char* dl, std::size_t maxsplit) {
            std::size_t start, end;
            std::vector<std::string> v;
            start = end = 0;
            int steps = maxsplit;
            while ((start = str.find_first_not_of(dl, end)) != std::string::npos) {
                if (steps == 1) {
                    v.push_back(str.substr(start));
                    break;
                }
                end = str.find(dl, start);
                v.push_back(str.substr(start, end - start));
                if (steps > 1) --steps;
            }
            return v;
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
                    const std::pair<const unsigned long long*, size_t> data(value.getData<unsigned long long>(),
                                                                            value.size());
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
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION(
                          "Conversion to string not implemented for this NDArray type");
            }
        }


        std::string toString(const karabo::util::ByteArray& value, size_t maxBytesShown) {
            if (!value.first || value.second == 0) return "";

            std::ostringstream os;
            const size_t size = value.second;
            const std::shared_ptr<char>& ptr = value.first;
            if (maxBytesShown == 0) {
                maxBytesShown = std::numeric_limits<size_t>::max();
            }
            const std::size_t nBytes = maxBytesShown / 2;
            os << "0x" << std::hex;
            for (std::size_t i = 0; i < size;) {
                if (i < nBytes || i >= (size - nBytes)) {
                    os << std::setw(2) << std::setfill('0') << int(ptr.get()[i]);
                    ++i;
                } else {
                    os << "...(skip " << std::dec << (size - 2 * nBytes) << " bytes)..." << std::hex;
                    i = size - nBytes;
                }
            }
            os << std::dec;
            return os.str();
        }
    } // namespace util
} // namespace karabo
