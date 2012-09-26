/*
 * $Id: Hash.cc 6763 2012-07-18 09:28:27Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on July 13, 2010, 6:55 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "Hash.hh"
#include "String.hh"
#include "ConfigConstants.hh"
#include "Schema.hh"

using namespace std;
using namespace boost;

namespace karabo {
    namespace util {

        string Hash::getAsString(const const_iterator& it) const {
            Types::Type type = getTypeAsId(it);
            ostringstream os;
            os << fixed;
            switch (type) {
                case Types::CHAR:
                    os << getNumeric<char>(it);
                    break;
                case Types::INT8:
                    os << static_cast<int> (getNumeric<signed char>(it));
                    break;
                case Types::INT16:
                    os << getNumeric<short>(it);
                    break;
                case Types::INT32:
                    os << getNumeric<int>(it);
                    break;
                case Types::INT64:
                    os << getNumeric<long long>(it);
                    break;
                case Types::UINT8:
                    os << static_cast<unsigned int> (getNumeric<unsigned char>(it));
                    break;
                case Types::UINT16:
                    os << getNumeric<unsigned short>(it);
                    break;
                case Types::UINT32:
                    os << getNumeric<unsigned int>(it);
                    break;
                case Types::UINT64:
                    os << getNumeric<unsigned long long>(it);
                    break;
                case Types::FLOAT:
                    os << getNumeric<float>(it);
                    break;
                case Types::DOUBLE:
                    os << getNumeric<double>(it);
                    break;
                case Types::BOOL:
                {
                    if (get<bool>(it)) os << "true";
                    else os << "false";
                }
                    break;
                case Types::STRING:
                    os << get<string > (it);
                    break;
                case Types::PATH:
                    os << get<boost::filesystem::path > (it).string();
                    break;
                case Types::CONST_CHAR_PTR:
                    os << get<const char*>(it);
                    break;
                case Types::COMPLEX_FLOAT:
                    os << get<complex<float> > (it);
                    break;
                case Types::COMPLEX_DOUBLE:
                    os << get<complex<double> > (it);
                    break;
                case Types::VECTOR_STRING:
                    os << String::sequenceToString(get<vector<string> >(it));
                    break;
                case Types::VECTOR_CHAR:
                    os << String::sequenceToString(get < vector<char> >(it));
                    break;
                case Types::VECTOR_INT8:
                    os << String::sequenceToString(get < vector<signed char> >(it));
                    break;
                case Types::VECTOR_INT16:
                    os << String::sequenceToString(get < vector<signed short> >(it));
                    break;
                case Types::VECTOR_INT32:
                    os << String::sequenceToString(get<vector<int> >(it));
                    break;
                case Types::VECTOR_INT64:
                    os << String::sequenceToString(get < vector<signed long long> >(it));
                    break;
                case Types::VECTOR_UINT8:
                    os << String::sequenceToString(get<vector<unsigned char> >(it));
                    break;
                case Types::VECTOR_UINT16:
                    os << String::sequenceToString(get<vector<unsigned short> >(it));
                    break;
                case Types::VECTOR_UINT32:
                    os << String::sequenceToString(get<vector<unsigned int> >(it));
                    break;
                case Types::VECTOR_UINT64:
                    os << String::sequenceToString(get<vector<unsigned long long> >(it));
                    break;
                case Types::VECTOR_DOUBLE:
                    os << String::sequenceToString(get<vector<double> >(it));
                    break;
                case Types::VECTOR_FLOAT:
                    os << String::sequenceToString(get<vector<float> >(it));
                    break;
                case Types::VECTOR_BOOL:
                    os << String::sequenceToString(get < deque<bool> >(it));
                    break;
                case Types::VECTOR_PATH:
                    os << String::sequenceToString(get <vector<boost::filesystem::path> >(it));
                    break;
                case Types::DATA_TYPE:
                    os << Types::convert(get<Types::Type > (it));
                    break;
                case Types::HASH:
                    os << get<Hash > (it);
                    break;
                case Types::ACCESS_TYPE:
                    os << get<AccessType>(it);
                    break;
                case Types::OCCURANCE_TYPE:
                    os << get<Schema::OccuranceType>(it);
                    break;
                case Types::ASSIGNMENT_TYPE:
                    os << get<Schema::AssignmentType>(it);
                    break;
                case Types::UNKNOWN: // Has to be the last case
                {
                    // Add any internally printable types here
                    if (it->second.type() == typeid (std::set<string>)) {
                        os << String::sequenceToString(get<std::set<string> >(it));
                        break;
                    } else if (this->is<wstring > (it)) {
                        os << String::toString(get<wstring > (it));
                        break;
                    }
                }
                default:
                    throw CAST_EXCEPTION("Could not convert value of key \"" + it->first + "\" to string");
            }
            return os.str();
        }

        string Hash::getAsString(const string& memberName) const {
            const_iterator it = this->find(memberName);
            if (it == this->end()) {
                throw PARAMETER_EXCEPTION("Key \"" + memberName + "\" does not exist");
            }
            return getAsString(it);
        }

        void Hash::convertFromString(const string& memberName, const Types::Type& type) {
            const_iterator it = this->find(memberName);
            if (it == this->end()) {
                throw PARAMETER_EXCEPTION("Key \"" + memberName + "\" does not exist");
            }
            Types::Type srcType = getTypeAsId(it);
            if (srcType != Types::STRING) return;
            if (type == Types::STRING) return;
            string key = it->first;
            string value = get<string > (it);
            switch (type) {
                case Types::CHAR:
                    set(key, boost::numeric_cast<char>(boost::lexical_cast<int>(value)));
                    break;
                case Types::INT8:
                    set(key, boost::numeric_cast<signed char>(boost::lexical_cast<int>(value)));
                    //set(key, boost::lexical_cast<char>(value));
                    break;
                case Types::INT16:
                    set(key, boost::lexical_cast<short>(value));
                    break;
                case Types::INT32:
                    set(key, boost::lexical_cast<int>(value));
                    break;
                case Types::INT64:
                    set(key, boost::lexical_cast<long long>(value));
                    break;
                case Types::UINT8:
                    set(key, boost::numeric_cast<unsigned char>(boost::lexical_cast<unsigned int>(value)));
                    break;
                case Types::UINT16:
                    set(key, boost::lexical_cast<unsigned short>(value));
                    break;
                case Types::UINT32:
                    set(key, boost::numeric_cast<unsigned int>(boost::lexical_cast<double>(value)));
                    break;
                case Types::UINT64:
                    try {
                        set(key, boost::lexical_cast<unsigned long long>(value));
                    } catch (const boost::bad_lexical_cast&) {
                        tryHarderToCastThis<unsigned long long>(key, value, type);
                    }
                    break;
                case Types::FLOAT:
                {
                    if (value == "nan" || value == "-nan") set(key, std::numeric_limits<float>::quiet_NaN());
                    else set(key, boost::lexical_cast<float>(value));
                    break;
                }
                case Types::DOUBLE:
                {
                    if (value == "nan" || value == "-nan") set(key, std::numeric_limits<double>::quiet_NaN());
                    else set(key, boost::lexical_cast<double>(value));
                    break;
                }
                case Types::PATH:
                    set(key, boost::filesystem::path(value));
                    break;
                case Types::BOOL:
                    boost::to_lower(value);
                    bool boolVal;
                    if (value == "n" || value == "no" || value == "false" ||
                            value == "0") {
                        boolVal = false;
                    } else if (value == "y" || value == "yes" || value == "1" ||
                            value == "true") {
                        boolVal = true;
                    } else {
                        throw CAST_EXCEPTION("Cannot interprete \"" + value + "\" as boolean.");
                    }
                    set(key, boolVal);
                    break;
                case Types::CONST_CHAR_PTR:
                    set(key, boost::lexical_cast<string > (value));
                    break;
                case Types::COMPLEX_FLOAT:
                    set(key, boost::lexical_cast<complex<float> >(value));
                    break;
                case Types::COMPLEX_DOUBLE:
                    set(key, boost::lexical_cast<complex<double> >(value));
                    break;
                case Types::VECTOR_STRING:
                    convertToVector<string > (key, value);
                    break;
                case Types::VECTOR_CHAR:
                    convertToVectorChar<char>(key, value);
                    break;
                case Types::VECTOR_INT8:
                    convertToVectorChar<signed char > (key, value);
                    break;
                case Types::VECTOR_INT16:
                    convertToVector<signed short > (key, value);
                    break;
                case Types::VECTOR_INT32:
                    convertToVector<signed int > (key, value);
                    break;
                case Types::VECTOR_INT64:
                    convertToVector<signed long long > (key, value);
                    break;
                case Types::VECTOR_UINT8:
                    convertToVectorChar<unsigned char > (key, value);
                    break;
                case Types::VECTOR_UINT16:
                    convertToVector<unsigned short > (key, value);
                    break;
                case Types::VECTOR_UINT32:
                    convertToVector<unsigned int > (key, value);
                    break;
                case Types::VECTOR_UINT64:
                    convertToVector<unsigned long long > (key, value);
                    break;
                case Types::VECTOR_BOOL:
                    convertToVectorBool(key, value);
                    break;
                case Types::VECTOR_DOUBLE:
                    convertToVector<double > (key, value);
                    break;
                case Types::VECTOR_FLOAT:
                    convertToVector<float > (key, value);
                    break;
                case Types::VECTOR_PATH:
                    convertToVector<boost::filesystem::path > (key, value);
                    break;
                case Types::ANY:
                    // Leave everything as it is
                    break;
                case Types::DATA_TYPE:
                    set(key, Types::convert(value));
                    break;
                case Types::ACCESS_TYPE:
                    set(key, static_cast<AccessType>(boost::lexical_cast<int>(value)));
                    break;
                case Types::OCCURANCE_TYPE:
                    set(key, static_cast<Schema::OccuranceType>(boost::lexical_cast<int>(value)));
                    break;
                case Types::ASSIGNMENT_TYPE:
                    set(key, static_cast<Schema::AssignmentType>(boost::lexical_cast<int>(value)));
                    break;
                default:
                    throw CAST_EXCEPTION("Cannot cast \"" + value + "\" into desired type");
            }
        }

        void Hash::convertToVectorBool(const std::string& key, const std::string& stringArray) {
            try {
                std::vector<std::string> elements;
                std::string tmp(stringArray);
                boost::trim(tmp);
                boost::split(elements, tmp, boost::is_any_of(","));
                size_t size = elements.size();
                std::deque<bool> resultArray(size);
                for (size_t i = 0; i < size; ++i) {
                    std::string element(elements[i]);
                    boost::trim(element);
                    resultArray[i] = boost::lexical_cast<bool> (element);
                }
                set(key, resultArray);
            } catch (...) {
                RETHROW;
            }
        }

        string Hash::getTypeAsString(const string& memberName) const {
            Hash::const_iterator it = this->find(memberName);
            if (it == this->end()) {
                throw PARAMETER_EXCEPTION("Invalid iterator as argument");
            }
            return Hash::getTypeAsString(it);
        }

        string Hash::getTypeAsString(const const_iterator& it) const {
            const Types& types = Types::getInstance();
            return types.getTypeAsString(it->second.type());
        }

        Types::Type Hash::getTypeAsId(const string& memberName) const {
            Hash::const_iterator it = this->find(memberName);
            if (it == this->end()) {
                throw PARAMETER_EXCEPTION("Invalid iterator as argument");
            }
            return Hash::getTypeAsId(it);
        }

        Types::Type Hash::getTypeAsId(const const_iterator& it) const {
            const Types& types = Types::getInstance();
            return types.getTypeAsId(it->second.type());
        }

        const Hash& Hash::append(const Hash& hash) {
            for (const_iterator i = hash.begin(); i != hash.end(); i++) {
                operator[](i->first) = i->second;
            }
            return *this;
        }
        
       vector<string> Hash::getKeysAsVector() const {
            vector<string> keys(this->size() );
            size_t i=0;
            for (const_iterator it = this->begin(); it != this->end(); it++, i++) {
                keys[i] = it->first;       
            }
            return keys;
        }

        set<string> Hash::getKeysAsSet() const {
            std::set<std::string> keys;
            for (const_iterator it = this->begin(); it != this->end(); it++) {
                keys.insert(it->first);
            }
            return keys;
        }
        
        vector<string> Hash::getLeavesAsVector(const string & sep) const {
            vector<string> ret;
            Hash::r_leaves<Hash > (*this, ret, "", sep);
            return ret;
        }

        set<string> Hash::getLeavesAsSet(const string & sep) const {
            vector<string> v = Hash::getLeavesAsVector();
            std::set<string> s;
            copy(v.begin(), v.end(), inserter(s, s.end()));
            return s;
        }
        
        void Hash::update(const Hash& hash) {
            // TODO Improve performance by not using "has" here
            for (const_iterator it = hash.begin(); it != hash.end(); ++it) {
                const string& key = it->first;
                int type = hash.getTypeAsId(it);
                if (type == Types::HASH) {
                    if (!this->has(key)) {
                        this->set(key, Hash());
                        this->get<Hash > (key).update(hash.get<Hash > (it));
                    } else {
                        if (this->getTypeAsId(key) == Types::HASH) {
                            this->get<Hash > (key).update(hash.get<Hash > (it));
                        }
                    }
                } else if (type == Types::VECTOR_HASH) {
                    if (!this->has(key)) { // Add the complete array (deep copy)
                        this->operator[](key) = it->second;
                    } else { // Push back each element
                        if (this->getTypeAsId(key) == Types::VECTOR_HASH) {
                            const vector<Hash>& tgt = hash.get<vector<Hash> >(it);
                            vector<Hash>& src = this->get<vector<Hash> >(key);
                            for (size_t j = 0; j < tgt.size(); ++j) {
                                src.push_back(tgt[j]);
                            }
                        }
                    }
                } else {
                    (*this)[it->first] = it->second;
                }
            }
        }

        Hash Hash::flatten(const std::string& sep) const {
            Hash flat;
            r_flatten(*this, flat, "", sep);
            return flat;
        }

        Hash Hash::unflatten(const std::string& sep) const {
            Hash tree;
            for (const_iterator i = this->begin(); i != this->end(); i++) {
                tree.setFromPath(i->first, i->second, sep);
            }
            return tree;
        }

        boost::tuple<bool, string, int> Hash::checkKeyForArrayType(const string& key) const {
            boost::tuple<bool, string, size_t > ret(false, "", 0);
            size_t pos = key.find_first_of("[");
            if (pos != string::npos) {
                vector<string> indexSplit;
                boost::split(indexSplit, key, boost::is_any_of("[]"));
                if (indexSplit.size() == 3) {
                    ret.get < 0 > () = true;
                    ret.get < 1 > () = key.substr(0, pos);
                    boost::trim(indexSplit[1]);
                    boost::to_lower(indexSplit[1]);
                    if (!indexSplit[1].empty()) {
                        if (indexSplit[1] == "last") {
                            ret.get < 2 > () = -1;
                        } else if (indexSplit[1] == "next") {
                            ret.get < 2 > () = -2;
                        } else {
                            ret.get < 2 > () = boost::lexical_cast<size_t > (indexSplit[1]);
                        }
                    } else {
                        ret.get < 0 > () = true;
                        ret.get < 1 > () = key.substr(0, pos);
                        ret.get < 2 > () = -1;
                    }
                } else {
                    ret.get < 0 > () = true;
                    ret.get < 1 > () = key.substr(0, pos);
                    ret.get < 2 > () = -1;
                }
            }
            return ret;
        }

        ostream & operator<<(std::ostream& os, const Hash& hash) {
            hash.r_toStream(os, hash, 0);
            return os;
        }

        void Hash::r_toStream(std::ostream& os, const Hash& hash, int depth) const {

            string fill(depth * 2, ' ');
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); it++) {
                bool isHandled = handleStandardTypes(os, hash, it, fill);
                if (!isHandled) {
                    Types::Type type = hash.getTypeAsId(it);
                    string typeString = hash.getTypeAsString(it);
                    switch (type) {
                        case Types::HASH:
                        {
                            os << fill << it->first << " => " << "Hash" << " (" << typeString << ") " << endl;
                            r_toStream(os, hash.get<Hash > (it), depth + 1);
                        }

                            break;
                        case Types::VECTOR_HASH:
                        {
                            os << fill << it->first << " => " << "Hash[]" << " (" << typeString << ") " << endl;
                            const vector<Hash>& tmp = hash.get<vector<Hash> >(it);
                            for (size_t i = 0; i < tmp.size(); ++i) {
                                os << fill << "[" << i << "]" << endl;
                                r_toStream(os, tmp[i], depth + 1);
                            }
                        }
                            break;
                        default:
                            os << fill << it->first << " => " << "UNKNOWN" << " (" << "UNKNOWN" << ") " << endl;
                    }
                }
            }
        }

        bool Hash::handleStandardTypes(ostream& os, const Hash& hash, const Hash::const_iterator& it,
                const string& fill) const {
            Types::Type type = hash.getTypeAsId(it);
            string typeString = hash.getTypeAsString(it);
            // Do not go into recursion here
            if (type == Types::HASH) {
                return false;
            } else {
                string value;
                try {
                    value = hash.getAsString(it);
                } catch (const Exception&) {
                    return false;
                }
                os << fill << it->first << " => ";
                os << value;
                os << " (" << typeString << ") " << endl;
                return true;
            }
        }

        bool Hash::castStringToBool(const std::string& value) {
            bool boolVal;
            if (value == "n" || value == "no" || value == "false" ||
                    value == "0") {
                boolVal = false;
            } else if (value == "y" || value == "yes" || value == "1" ||
                    value == "true") {
                boolVal = true;
            } else {
                throw CAST_EXCEPTION("Cannot interprete \"" + value + "\" as boolean.");
            }
            return boolVal;
        }

        bool Hash::identical(const const_iterator& source) const {
            Types::Type sourceType = getTypeAsId(source);
            const_iterator it = this->find(source->first);
            // check for existence
            if (it == this->end()) 
                return false;
            Types::Type type = getTypeAsId(it);
            // check the types ate identical
            if (type != sourceType)
                return false;
           
            switch (type) {
                case Types::CHAR:
                    return getNumeric<char>(it) == getNumeric<char>(source);
                case Types::INT8:
                    return static_cast<int> (getNumeric<signed char>(it)) == static_cast<int> (getNumeric<signed char>(source));
                case Types::INT16:
                    return getNumeric<short>(it) == getNumeric<short>(source);
                case Types::INT32:
                    return getNumeric<int>(it) == getNumeric<int>(source);
                case Types::INT64:
                    return getNumeric<long long>(it) == getNumeric<long long>(source);
                case Types::UINT8:
                    return static_cast<unsigned int> (getNumeric<unsigned char>(it)) == static_cast<unsigned int> (getNumeric<unsigned char>(source));
                case Types::UINT16:
                    return getNumeric<unsigned short>(it) == getNumeric<unsigned short>(source);
                case Types::UINT32:
                    return getNumeric<unsigned int>(it) == getNumeric<unsigned int>(source);
                case Types::UINT64:
                    return getNumeric<unsigned long long>(it) == getNumeric<unsigned long long>(source);
                case Types::FLOAT:
                    return getNumeric<float>(it) == getNumeric<float>(source);
                case Types::DOUBLE:
                    return getNumeric<double>(it) == getNumeric<double>(source);
                case Types::BOOL:
                    return get<bool>(it) == get<bool>(source);
                case Types::STRING:
                    return get<string > (it) == get<string > (source);
                case Types::PATH:
                    return get<boost::filesystem::path > (it).string() == get<boost::filesystem::path > (source).string();
                case Types::CONST_CHAR_PTR:
                    return string(get<const char*>(it)) == string(get<const char*>(source));
                case Types::COMPLEX_FLOAT:
                    return get<complex<float> > (it) == get<complex<float> > (source);
                case Types::COMPLEX_DOUBLE:
                    return get<complex<double> > (it) == get<complex<double> > (source);
                case Types::VECTOR_STRING:
                    return get<vector<string> >(it) == get<vector<string> >(source);
                case Types::VECTOR_CHAR:
                    return get < vector<char> >(it) == get<vector<char> >(source);
                case Types::VECTOR_INT8:
                    return get < vector<signed char> >(it) == get < vector<signed char> >(source);
                case Types::VECTOR_INT16:
                    return get < vector<signed short> >(it) == get < vector<signed short> >(source);
                case Types::VECTOR_INT32:
                    return get<vector<int> >(it) == get<vector<int> >(source);
                case Types::VECTOR_INT64:
                    return get < vector<signed long long> >(it) == get < vector<signed long long> >(source);
                case Types::VECTOR_UINT8:
                    return get<vector<unsigned char> >(it) == get<vector<unsigned char> >(source);
                case Types::VECTOR_UINT16:
                    return get<vector<unsigned short> >(it) == get<vector<unsigned short> >(source);
                case Types::VECTOR_UINT32:
                    return get<vector<unsigned int> >(it) == get<vector<unsigned int> >(source);
                case Types::VECTOR_UINT64:
                    return get<vector<unsigned long long> >(it) == get<vector<unsigned long long> >(it);
                case Types::VECTOR_DOUBLE:
                    return get<vector<double> >(it) == get<vector<double> >(it);
                case Types::VECTOR_FLOAT:
                    return get<vector<float> >(it) == get<vector<float> >(source);
                case Types::VECTOR_BOOL:
                    return get < deque<bool> >(it) == get < deque<bool> >(source);
                case Types::VECTOR_PATH:
                    return get <vector<boost::filesystem::path> >(it) == get <vector<boost::filesystem::path> >(source);
                case Types::HASH:
                    return true;
                default:
                    break;
            }
            return false;
        }

    } // namespace util
} // namespace karabo


