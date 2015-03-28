/* 
 * File:   Hash.cc
 * Author: <burkhard.heisen@xfel.eu>
 * Author: <djelloul.boukhelef@xfel.eu>
 *
 * Created on December 14, 2012, 11:19 AM
 */

#include "Hash.hh"
#include "Schema.hh"
#include "ToLiteral.hh"

namespace karabo {

    namespace util {

        Hash::Hash() {
        }

        Hash::Hash(const std::string& path) {
            this->set(path, Hash());
        }

        Hash::~Hash() {
        }

        Hash::const_iterator Hash::begin() const {
            return m_container.begin();
        }

        Hash::iterator Hash::begin() {
            return m_container.begin();
        }

        Hash::const_iterator Hash::end() const {
            return m_container.end();
        }

        Hash::iterator Hash::end() {
            return m_container.end();
        }

        Hash::const_map_iterator Hash::mbegin() const {
            return m_container.mbegin();
        }

        Hash::map_iterator Hash::mbegin() {
            return m_container.mbegin();
        }

        Hash::const_map_iterator Hash::mend() const {
            return m_container.mend();
        }

        Hash::map_iterator Hash::mend() {
            return m_container.mend();
        }

        size_t Hash::size() const {
            return m_container.size();
        }

        bool Hash::empty() const {
            return m_container.empty();
        }

        void Hash::clear() {
            m_container.clear();
        }

        bool Hash::has(const std::string& path, const char separator) const {
            try {
                std::string key;
                const Hash& hash = getLastHash(path, key, separator);
                int index = karabo::util::getAndCropIndex(key);
                if (index == -1) {
                    return hash.m_container.has(key);
                } else {
                    return static_cast<int> (hash.m_container.get<vector<Hash> >(key).size()) > index;
                }
            } catch (...) {
            }
            return false;
        }

        Hash::Node& Hash::setNode(const Hash::Node& srcElement) {
            Node& destElement = set(srcElement.getKey(), srcElement.getValueAsAny());
            destElement.setAttributes(srcElement.getAttributes());
            return destElement;
        }

        Hash::Node& Hash::setNode(Hash::const_iterator srcIterator) {
            Node& el = set(srcIterator->getKey(), srcIterator->getValueAsAny());
            el.setAttributes(srcIterator->getAttributes());
            return el;
        }

        bool Hash::is(const std::string& path, const Types::ReferenceType & type, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                return hash.m_container.is(key, type);
            } else {
                return typeid (hash.m_container.get<vector<Hash> >(key)[index]) == typeid (Hash);
            }
        }

        //        std::string Hash::getString(const std::string & path, const char separator) const {
        //            return getNode(path, separator).getValueAsString();
        //        }

        void Hash::erase(const std::string& path, const char separator) {
            std::string key;
            Hash& hash = getLastHash(path, key, separator);
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                hash.m_container.erase(key);
            } else {
                std::vector<Hash>& vect = hash.m_container.get<vector<Hash> >(key);
                vect.erase(vect.begin() + index);
            }
        }

        bool Hash::eraseFound(const std::string& path, const char separator) {
            std::string key;
            Hash& hash = getLastHash(path, key, separator);
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                return hash.m_container.eraseFound(key);
            } else {
                std::vector<Hash>& vect = hash.m_container.get<vector<Hash> >(key);
                vect.erase(vect.begin() + index);
                return true;
            }
        }

        static std::string concat(const std::vector<string>& vec, size_t len, const std::string& sep) {
            std::string result;
            for (size_t i = 0; i < len; i++) {
                result += vec[i];
                if (i < (len - 1)) result += sep;
            }
            return result;
        }

        void Hash::erasePath(const std::string& path, const char separator) {
            string sep(1, separator);
            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);
            size_t length = tokens.size();
            string thePath = path;
            try {
                while (length > 0 && !thePath.empty()) {
                    std::string key;
                    Hash& hash = getLastHash(thePath, key, separator);
                    int index = karabo::util::getAndCropIndex(key); // key cleared from possible []
                    if (index == -1) {
                        hash.m_container.erase(key);
                        thePath = concat(tokens, --length, sep);
                        if (thePath.empty())
                            break;
                    } else {
                        std::vector<Hash>& vect = hash.m_container.get<vector<Hash> >(key);
                        vect.erase(vect.begin() + index);
                        if (!vect.empty()) break;
                        thePath = concat(tokens, --length, sep);
                        if (thePath.empty()) {
                            erase(key, separator);
                            break;
                        } else {
                            erase(thePath + sep + key, separator);
                        }
                    }
                    if ((this->is<Hash>(thePath, separator) && !this->get<Hash>(thePath, separator).empty()) ||
                            (this->is<vector<Hash> >(thePath, separator) && !this->get<vector<Hash> >(thePath, separator).empty()))
                        break;
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst erasing path (" + path + ") from Hash"));
            }
        }

        const Hash& Hash::getLastHash(const std::string& path, std::string& lastKey, const char separator) const {
            if (path.empty()) throw KARABO_PARAMETER_EXCEPTION("Illegal call to get with empty path");
            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);

            const Hash* tmp = this;
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                int index = karabo::util::getAndCropIndex(tokens[i]);
                if (index == -1) {
                    tmp = &tmp->m_container.getNode(tokens[i]).getValue<Hash > ();
                } else {
                    tmp = &tmp->m_container.getNode(tokens[i]).getValue<vector<Hash> > ()[index];
                }
            }
            lastKey.swap(tokens.back());
            return *tmp;
        }

        Hash & Hash::getLastHash(const std::string& path, std::string& last_key, const char separator) {
            return const_cast<Hash&> (thisAsConst().getLastHash(path, last_key, separator));
        }

        const Hash::Node& Hash::getNode(const std::string& path, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            if (karabo::util::getAndCropIndex(key) == -1) {
                return hash.m_container.getNode(key);
            }
            throw KARABO_LOGIC_EXCEPTION("Array syntax on a leaf is not possible (would be a Hash and not a Node)");
        }

        Hash::Node& Hash::getNode(const std::string& path, const char separator) {
            return const_cast<Hash::Node&> (thisAsConst().getNode(path, separator));
        }

        boost::optional<const Hash::Node&> Hash::find(const std::string& path, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            if (karabo::util::getAndCropIndex(key) == -1) {
                const_map_iterator it = hash.m_container.find(key);
                if (it != this->mend()) return it->second;
                else return boost::optional<const Hash::Node&>();
            } else {
                throw KARABO_LOGIC_EXCEPTION("Array syntax on a leaf is not possible (would be a Hash and not a Node)");
            }
        }

        boost::optional<Hash::Node&> Hash::find(const std::string& path, const char separator) {
            try {
                return getNode(path, separator);
            } catch (...) {
                // Exception must be ignored here!
                // TODO Construction and catching of an exception is expensive here.
                Exception::clearTrace();
            }
            return boost::optional<Hash::Node&>();
        }

        Types::ReferenceType Hash::getType(const std::string& path, const char separator) const {
            std::string tmp(path);
            int index = karabo::util::getAndCropIndex(tmp);
            if (index == -1) {
                return getNode(tmp, separator).getType();
            } else {
                return Types::HASH;
            }
        }

        /*-------------------------------------------------------------------------
         * Some useful functions
         *-------------------------------------------------------------------------*/

        void Hash::getKeys(std::set<std::string>& result) const {
            for (const_iterator iter = m_container.begin(); iter != m_container.end(); ++iter) {
                result.insert(iter->getKey());
            }
        }

        void Hash::getPaths(std::set<std::string>& result, const char separator) const {
            if (this->empty()) return;
            vector<std::string> vect;
            Hash::getPaths(*this, vect, "", separator);

            for (size_t i = 0; i < vect.size(); ++i) {
                result.insert(vect[i]);
            }
        }

        void Hash::getPaths(const Hash& hash, std::vector<std::string>& result, std::string prefix, const char separator) {
            if (hash.empty()) {
                result.push_back(prefix);
                return;
            }
            for (const_iterator it = hash.begin(); it != hash.end(); ++it) {
                std::string currentKey = it->getKey();

                if (!prefix.empty()) {
                    char separators[] = {separator, 0};
                    currentKey = prefix + separators + currentKey;
                }
                if (it->is<Hash > ()) { // Recursion
                    getPaths(it->getValue<Hash > (), result, currentKey, separator);
                } else {
                    if (it->is<std::vector<Hash> > ()) { // Recursion for vector
                        for (size_t i = 0; i < it->getValue<std::vector<Hash> > ().size(); ++i) {
                            std::ostringstream os;
                            os << currentKey << "[" << i << "]";
                            getPaths(it->getValue<std::vector<Hash> > ().at(i), result, os.str(), separator);
                        }
                    } else {
                        result.push_back(currentKey);
                    }
                }
            }
        }

        void Hash::flatten(const Hash& hash, Hash& flat, std::string prefix, const char separator) {
            if (!hash.empty()) {
                for (const_iterator it = hash.begin(); it != hash.end(); ++it) {
                    std::string currentKey = it->getKey();

                    if (!prefix.empty()) {
                        char separators[] = {separator, 0};
                        currentKey = prefix + separators + currentKey;
                    }

                    if (it->is<Hash > ()) { // Recursion
                        flatten(it->getValue<Hash > (), flat, currentKey, separator);
                    } else {
                        if (it->is<std::vector<Hash> > ()) { // Recursion for vector
                            for (size_t i = 0; i < it->getValue<std::vector<Hash> > ().size(); ++i) {
                                std::ostringstream os;
                                os << currentKey << "[" << i << "]";
                                flatten(it->getValue<std::vector<Hash> > ().at(i), flat, os.str(), separator);
                            }
                        } else {
                            flat.set(currentKey, it->getValueAsAny(), 0);
                            flat.setAttributes(currentKey, it->getAttributes(), 0);
                        }
                    }
                }
            }
        }

        void Hash::flatten(Hash& flat, const char separator) const {
            flatten(*this, flat, "", separator);
        }

        void Hash::unflatten(Hash& tree, const char separator) const {
            for (const_iterator it = begin(); it != end(); ++it) {
                tree.set(it->getKey(), it->getValueAsAny(), separator);
                tree.setAttributes(it->getKey(), it->getAttributes(), separator);
            }
        }

        void Hash::merge(const Hash& other, const Hash::MergePolicy policy) {
            if (policy == MERGE_ATTRIBUTES) mergeAndMergeAttributes(other);
            else if (policy == REPLACE_ATTRIBUTES) mergeAndReplaceAttributes(other);
        }

        void Hash::mergeAndMergeAttributes(const Hash& other) {
            if (this->empty() && !other.empty()) {
                *this = other;
                return;
            }
            for (Hash::const_iterator it = other.begin(); it != other.end(); ++it) {
                const Hash::Node& otherNode = *it;
                std::string key = otherNode.getKey();
                boost::optional<Hash::Node&> thisNode = this->find(key);
                if (thisNode) { // Key already exists
                    // Always merge attributes
                    const Hash::Attributes& attrs = otherNode.getAttributes();
                    for (Hash::Attributes::const_iterator jt = attrs.begin(); jt != attrs.end(); ++jt) {
                        thisNode->setAttribute(jt->getKey(), jt->getValueAsAny());
                    }

                    // Both nodes are Hash
                    if (thisNode->is<Hash > () && otherNode.is<Hash > ()) {
                        thisNode->getValue<Hash > ().mergeAndMergeAttributes(otherNode.getValue<Hash > ());
                        continue;
                    }

                    // Both nodes are vector<Hash>
                    if (thisNode->is<vector<Hash> > () && otherNode.is<vector<Hash> > ()) {
                        vector<Hash>& this_vec = thisNode->getValue<vector<Hash> > ();
                        const vector<Hash>& other_vec = otherNode.getValue<vector<Hash> > ();
                        this_vec.insert(this_vec.end(), other_vec.begin(), other_vec.end());
                        continue;
                    }
                    thisNode->setValue(otherNode.getValueAsAny());

                } else { // Key does not exist
                    this->setNode(otherNode);
                }
            }
        }

        void Hash::mergeAndReplaceAttributes(const Hash& other) {
            if (this->empty() && !other.empty()) {
                *this = other;
                return;
            }
            for (Hash::const_iterator it = other.begin(); it != other.end(); ++it) {
                const Hash::Node& otherNode = *it;
                std::string key = otherNode.getKey();
                boost::optional<Hash::Node&> thisNode = this->find(key);
                if (thisNode) { // Key already exists
                    // Always replace attributes
                    const Hash::Attributes& attrs = otherNode.getAttributes();
                    thisNode->setAttributes(attrs);
                    // Both nodes are Hash
                    if (thisNode->is<Hash > () && otherNode.is<Hash > ()) {
                        thisNode->getValue<Hash > ().mergeAndReplaceAttributes(otherNode.getValue<Hash > ());
                        continue;
                    }

                    // Both nodes are vector<Hash>
                    if (thisNode->is<vector<Hash> > () && otherNode.is<vector<Hash> > ()) {
                        vector<Hash>& this_vec = thisNode->getValue<vector<Hash> > ();
                        const vector<Hash>& other_vec = otherNode.getValue<vector<Hash> > ();
                        this_vec.insert(this_vec.end(), other_vec.begin(), other_vec.end());
                        continue;
                    }
                    thisNode->setValue(otherNode.getValueAsAny());

                } else { // Key does not exist
                    this->setNode(otherNode);
                }
            }
        }

        Hash& Hash::operator+=(const Hash& other) {
            this->merge(other);
            return *this;
        }

        void Hash::subtract(const Hash& other, const char separator) {
            if (this->empty() || other.empty()) return;
            vector<string> candidates;
            getPaths(other, candidates, "", separator); // may be optimized to avoid list creation 
            if (candidates.empty()) return;
            for (vector<string>::const_iterator it = candidates.begin(); it != candidates.end(); ++it) {
                this->eraseFound(*it, separator);
            }
        }

        Hash& Hash::operator-=(const Hash& other) {
            this->subtract(other);
            return *this;
        }

        bool Hash::operator==(const Hash& other) {
            return similar(*this, other);
        }

        bool Hash::operator!=(const Hash& other) {
            return !similar(*this, other);
        }

        /*******************************************************************
         * Attributes manipulation
         *******************************************************************/

        boost::any& Hash::getAttributeAsAny(const std::string& path, const std::string& attribute, const char separator) {
            return getNode(path, separator).getAttributeAsAny(attribute);
        }

        const boost::any& Hash::getAttributeAsAny(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).getAttributeAsAny(attribute);
        }

        bool Hash::hasAttribute(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).hasAttribute(attribute);
        }

        const Hash::Attributes & Hash::getAttributes(const std::string& path, const char separator) const {
            return getNode(path, separator).getAttributes();
        }

        Hash::Attributes & Hash::getAttributes(const std::string& path, const char separator) {
            return getNode(path, separator).getAttributes();
        }

        void Hash::setAttribute(const std::string& path, const std::string& attribute, const char* value, const char separator) {
            getNode(path, separator).setAttribute(attribute, value);
        }

        void Hash::setAttributes(const std::string& path, const Hash::Attributes& attributes, const char separator) {
            return getNode(path, separator).setAttributes(attributes);
        }

        Hash* Hash::setNodesAsNeeded(const std::vector<std::string>& tokens, char separator) {
            // Loop all but last token
            Hash* tmp = this;
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                std::string token = tokens[i];
                int index = karabo::util::getAndCropIndex(token);
                if (index == -1) { // Just Hash
                    if (tmp->m_container.has(token)) { // Node exists
                        Hash::Node* node = &(tmp->m_container.getNode(token));
                        if (!node->is<Hash > ()) { // Node is not Hash
                            node->setValue(Hash()); // Force it to be one
                        }
                        tmp = &(node->getValue<Hash > ());
                    } else { // Node does not exist
                        Hash::Node* node = &(tmp->m_container.set(token, Hash()));
                        tmp = &(node->getValue<Hash > ());
                    }
                } else { // vector of Hash
                    if (tmp->m_container.has(token)) { // Node exists
                        Hash::Node* node = &(tmp->m_container.getNode(token));
                        if (!node->is<std::vector<Hash> >()) { // Node is not std::vector<Hash>
                            node->setValue(std::vector<Hash > (index + 1)); // Force it to be one
                        }
                        std::vector<Hash>& hashes = node->getValue<std::vector<Hash> >();
                        if (static_cast<int> (hashes.size()) <= index) hashes.resize(index + 1);
                        tmp = &hashes[index];
                    } else { // Node does not exist
                        Hash::Node* node = &(tmp->m_container.set(token, std::vector<Hash > (index + 1)));
                        tmp = &(node->getValue<std::vector<Hash> >()[index]);
                    }
                }
            }
            return tmp;
        }

        std::ostream& operator<<(std::ostream& os, const Hash& hash) {
            try {
                hash.toStream(os, hash, 0);
            } catch (...) {
                KARABO_RETHROW;
            }
            return os;
        }

        void Hash::toStream(std::ostream& os, const Hash& hash, int depth) const {

            std::string fill(depth * 2, ' ');

            for (Hash::const_iterator hit = hash.begin(); hit != hash.end(); hit++) {
                os << fill << hit->getKey();

                const Hash::Attributes& attrs = hit->getAttributes();
                if (attrs.size() > 0) {
                    for (Hash::Attributes::const_iterator ait = attrs.begin(); ait != attrs.end(); ++ait) {
                        os << " " << ait->getKey() << "=\"" << ait->getValueAs<string>() /*<< " " << Types::to<ToLiteral>(ait->getType())*/ << "\"";
                    }
                }

                Types::ReferenceType type = hit->getType();
                if (type == Types::HASH) {
                    os << " +" << std::endl;
                    toStream(os, hit->getValue<Hash>(), depth + 1);
                } else if (type == Types::HASH_POINTER) {
                    os << " + (Pointer)" << std::endl;
                    toStream(os, *(hit->getValue<Hash::Pointer>()), depth + 1);
                } else if (type == Types::VECTOR_HASH) {
                    const vector<Hash>& hashes = hit->getValue<vector<Hash> >();
                    os << " @" << endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "[" << i << "]" << std::endl;
                        toStream(os, hashes[i], depth + 1);
                    }
                } else if (type == Types::VECTOR_HASH_POINTER) {
                    const vector<Hash::Pointer>& hashes = hit->getValue<vector<Hash::Pointer> >();
                    os << " @ (Pointer)" << endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "[" << i << "]" << std::endl;
                        toStream(os, *(hashes[i]), depth + 1);
                    }
                } else if (type == Types::SCHEMA) {
                    os << " => " << hit->getValue<Schema>() << std::endl;
                } else if (Types::isPointer(type)) {// TODO Add pointer types
                    os << " => xxx " << Types::to<ToLiteral>(type) << std::endl;
                } else {
                    os << " => " << hit->getValueAs<string>() << " " << Types::to<ToLiteral>(type) << std::endl;
                }
            }
        }

        bool similar(const Hash& left, const Hash& right) {
            if (left.size() != right.size())
                return false;

            for (Hash::const_iterator itl = left.begin(), itr = right.begin(); itl != left.end() && itr != right.end(); ++itl, itr++) {
                if (!similar(*itl, *itr)) return false;
            }
            return true;
        }

        bool similar(const std::vector<Hash>& left, const std::vector<Hash>& right) {
            if (left.size() != right.size())
                return false;

            for (size_t i = 0; i < left.size(); ++i) {
                if (!similar(left[i], right[i])) return false;
            }
            return true;
        }

        bool similar(const Hash::Node& left, const Hash::Node& right) {
            if (left.getType() != right.getType())
                return false;

            if (left.getType() == Types::HASH) {
                return similar(left.getValue<Hash > (), right.getValue<Hash > ());
            }
            if (left.getType() == Types::VECTOR_HASH) {
                return similar(left.getValue<std::vector<Hash> >(), right.getValue<std::vector<Hash> >());
            }
            return true;
        }

        size_t counter(const Hash& hash) {
            size_t partial_count = 0;
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                const Hash::Node& ele = *iter;
                partial_count++;

                if (ele.is<Hash > ()) {
                    partial_count += counter(ele.getValue<Hash > ());
                } else {
                    if (ele.is<std::vector<Hash> >()) {
                        const std::vector<Hash>& vect = ele.getValue<std::vector<Hash> >();
                        partial_count += vect.size();
                        for (size_t i = 0; i < vect.size(); ++i) {
                            partial_count += counter(vect[i]);
                        }
                    } else {//?????
                        partial_count += counter(ele);
                    }
                }
            }
            return partial_count;
        }

        size_t counter(const Hash& hash, Types::ReferenceType type) {
            size_t partial_count = 0;
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                const Hash::Node& ele = *iter;
                partial_count += ((ele.getType() == type) ? 1 : 0);

                if (ele.is<Hash > ()) {
                    partial_count += counter(ele.getValue<Hash > (), type);
                } else {
                    if (ele.is<std::vector<Hash> >()) {
                        const std::vector<Hash>& vect = ele.getValue<std::vector<Hash> >();
                        partial_count += ((type == Types::HASH) ? vect.size() : 0);
                        for (size_t i = 0; i < vect.size(); ++i) {
                            partial_count += counter(vect[i], type);
                        }
                    } else {
                        if (Types::category(ele.getType()) == Types::SEQUENCE) {
                            if (type == (ele.getType() - 1)) {
                                partial_count += counter(ele);
                            }
                        }
                    }
                }
            }
            return partial_count;
        }

        #define COUNTER(ReferenceType, CppType) ReferenceType: return element.getValue < vector <CppType> >().size();

        size_t counter(const Hash::Node& element) {
            switch (element.getType()) {
                case COUNTER(Types::VECTOR_BOOL, bool);
                case COUNTER(Types::VECTOR_STRING, std::string);
                case COUNTER(Types::VECTOR_CHAR, char);
                case COUNTER(Types::VECTOR_INT8, signed char);
                case COUNTER(Types::VECTOR_INT16, short);
                case COUNTER(Types::VECTOR_INT32, int);
                case COUNTER(Types::VECTOR_INT64, long long);
                case COUNTER(Types::VECTOR_UINT8, unsigned char);
                case COUNTER(Types::VECTOR_UINT16, unsigned short);
                case COUNTER(Types::VECTOR_UINT32, unsigned int);
                case COUNTER(Types::VECTOR_UINT64, unsigned long long);
                case COUNTER(Types::VECTOR_FLOAT, float);
                case COUNTER(Types::VECTOR_DOUBLE, double);
                case COUNTER(Types::VECTOR_COMPLEX_FLOAT, std::complex<float>);
                case COUNTER(Types::VECTOR_COMPLEX_DOUBLE, std::complex<double>);
                case COUNTER(Types::VECTOR_HASH, Hash);
                default: return 0;
            }
        }
    }
}
