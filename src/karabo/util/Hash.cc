/*
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

        const char Hash::k_defaultSep = '.'; /// The default path separator for nested Hash

        Hash::Hash() {}


        Hash::Hash(const std::string& path) {
            this->set(path, Hash());
        }


        Hash::~Hash() {}


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
            std::string key;
            const Hash* hash = getLastHashPtr(path, key, separator);
            if (!hash) {
                return false; // invalid path (empty, wrong sub-key or bad index)
            }
            int index = karabo::util::getAndCropIndex(key);
            if (!hash->m_container.has(key)) {
                return false; // e.g. asking for 'key[1]', but there is no 'key'
            } else if (index == -1) {
                return true;
            } else {
                return hash->m_container.get<std::vector<Hash>>(key).size() > static_cast<unsigned int>(index);
            }
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


        bool Hash::is(const std::string& path, const Types::ReferenceType& type, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                return hash.m_container.is(key, type);
            } else {
                const std::vector<Hash>& hashVec = hash.m_container.get<std::vector<Hash>>(key);
                if (static_cast<unsigned int>(index) >= hashVec.size()) {
                    throw KARABO_PARAMETER_EXCEPTION("Index " + toString(index) + " out of range in '" + path + "'.");
                }
                return true; // by definition of hashVec (or already an exception)!
            }
        }


        bool Hash::erase(const std::string& path, const char separator) {
            std::string key;
            Hash* hash = getLastHashPtr(path, key, separator);
            if (!hash) {
                return false;
            }
            int index = karabo::util::getAndCropIndex(key);
            if (index == -1) {
                return (hash->m_container.erase(key) != 0);
            } else {
                Container::map_iterator it = hash->m_container.find(key);
                if (it == hash->m_container.mend()) {
                    // Could be 'erase("a[2]")', but there is no "a" at all!
                    return false;
                }
                std::vector<Hash>& vect = hash->m_container.get<std::vector<Hash>>(it);
                if (static_cast<unsigned int>(index) >= vect.size()) {
                    return false;
                } else {
                    vect.erase(vect.begin() + index);
                    return true;
                }
            }
        }


        static std::string concat(const std::vector<std::string>& vec, size_t len, const std::string& sep) {
            std::string result;
            for (size_t i = 0; i < len; i++) {
                result += vec[i];
                if (i < (len - 1)) result += sep;
            }
            return result;
        }


        void Hash::erasePath(const std::string& path, const char separator) {
            const std::string sep(1, separator);
            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);
            size_t length = tokens.size();
            std::string thePath = path;
            try {
                while (length > 0) {
                    std::string key;
                    Hash* hash = getLastHashPtr(thePath, key, separator);
                    if (!hash) {
                        break; // probably wrong sub-key or bad index
                    }
                    int index = karabo::util::getAndCropIndex(key); // key cleared from possible []
                    if (index == -1) {
                        hash->m_container.erase(key);
                        thePath = concat(tokens, --length, sep);
                        if (length == 0) break; // Done!
                    } else {
                        Container::map_iterator it = hash->m_container.find(key);
                        if (it == hash->m_container.mend()) {
                            // Could be 'erasePath("a[2]")', but there is no "a" at all.
                            break;
                        }
                        std::vector<Hash>& vect = hash->m_container.get<std::vector<Hash>>(it);
                        if (static_cast<unsigned int>(index) < vect.size()) {
                            vect.erase(vect.begin() + index);
                        }
                        if (!vect.empty()) break;
                        thePath = concat(tokens, --length, sep);
                        if (length == 0) {
                            erase(key, separator);
                            break;
                        } else {
                            erase(thePath + sep + key, separator);
                        }
                    }
                    if ((this->is<Hash>(thePath, separator) && !this->get<Hash>(thePath, separator).empty()) ||
                        (this->is<std::vector<Hash>>(thePath, separator) &&
                         !this->get<std::vector<Hash>>(thePath, separator).empty()))
                        break;
                }
            } catch (const karabo::util::Exception& e) {
                KARABO_RETHROW_AS(KARABO_PARAMETER_EXCEPTION("Error whilst erasing path '" + path + "' from Hash"));
            }
        }


        const Hash& Hash::getLastHash(const std::string& path, std::string& lastKey, const char separator) const {
            const Hash* hash = getLastHashPtr(path, lastKey, separator);
            if (!hash) {
                // If getLastHashPtr would provide an error code, we could be more specific...
                throw KARABO_PARAMETER_EXCEPTION("non-existing key, wrong type or index out of range in '" + path +
                                                 "'.");
            }
            return *hash;
        }


        Hash& Hash::getLastHash(const std::string& path, std::string& last_key, const char separator) {
            return const_cast<Hash&>(thisAsConst().getLastHash(path, last_key, separator));
        }


        const Hash* Hash::getLastHashPtr(const std::string& path, std::string& lastKey, const char separator) const {
            // TODO: We should add an error code to be returned as argument by value.
            std::vector<std::string> tokens;
            karabo::util::tokenize(path, tokens, separator);

            const Hash* tmp = this;
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                int index = karabo::util::getAndCropIndex(tokens[i]);
                Container::const_map_iterator it = tmp->m_container.find(tokens[i]);
                if (it == tmp->m_container.mend()) return 0;
                const Hash::Node& node = it->second;
                if (index == -1) {
                    if (!node.is<Hash>()) return 0;
                    tmp = &node.getValue<Hash>();
                } else {
                    if (!node.is<std::vector<Hash>>()) return 0;
                    const std::vector<Hash>& hashVec = node.getValue<std::vector<Hash>>();
                    if (static_cast<unsigned int>(index) >= hashVec.size()) {
                        return 0;
                    }
                    tmp = &(hashVec[index]);
                }
            }
            lastKey.swap(tokens.back());
            return tmp;
        }


        Hash* Hash::getLastHashPtr(const std::string& path, std::string& last_key, const char separator) {
            return const_cast<Hash*>(thisAsConst().getLastHashPtr(path, last_key, separator));
        }


        const Hash::Node& Hash::getNode(const std::string& path, const char separator) const {
            std::string key;
            const Hash& hash = getLastHash(path, key, separator);
            if (karabo::util::getAndCropIndex(key) == -1) {
                return hash.m_container.getNode(key);
            }
            throw KARABO_LOGIC_EXCEPTION("Array syntax on leaf '" + path + "' is not possible");
        }


        Hash::Node& Hash::getNode(const std::string& path, const char separator) {
            return const_cast<Hash::Node&>(thisAsConst().getNode(path, separator));
        }


        boost::optional<const Hash::Node&> Hash::find(const std::string& path, const char separator) const {
            std::string key;
            const Hash* hash = getLastHashPtr(path, key, separator);
            if (hash) {
                if (karabo::util::getAndCropIndex(key) == -1) {
                    const_map_iterator it = hash->m_container.find(key);
                    if (it != hash->m_container.mend()) {
                        return it->second;
                    } // else ...
                    // ... we have array syntax that would get a Hash (within an std::vector) and not a Node
                }
            }
            return boost::optional<const Hash::Node&>();
        }


        boost::optional<Hash::Node&> Hash::find(const std::string& path, const char separator) {
            // Use const version and just cast const away from result if successful.
            boost::optional<const Hash::Node&> constResult = thisAsConst().find(path, separator);
            if (constResult) {
                return boost::optional<Hash::Node&>(const_cast<Hash::Node&>(*constResult));
            } else {
                return boost::optional<Hash::Node&>();
            }
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
            std::vector<std::string> vect;
            Hash::getPaths(*this, vect, "", separator, false);

            for (size_t i = 0; i < vect.size(); ++i) {
                result.insert(vect[i]);
            }
        }

        void Hash::getDeepPaths(std::set<std::string>& result, const char separator) const {
            if (this->empty()) return;
            std::vector<std::string> vect;
            Hash::getPaths(*this, vect, "", separator, true);

            for (size_t i = 0; i < vect.size(); ++i) {
                result.insert(vect[i]);
            }
        }


        void Hash::getPaths(const Hash& hash, std::vector<std::string>& result, std::string prefix,
                            const char separator, const bool fullPaths) {
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
                if (it->is<Hash>() &&
                    (fullPaths || !it->hasAttribute(KARABO_HASH_CLASS_ID))) { // Recursion, but no hash sub classes
                    getPaths(it->getValue<Hash>(), result, currentKey, separator, fullPaths);
                } else {
                    if (it->is<std::vector<Hash>>() &&
                        it->getValue<std::vector<Hash>>().size() > 0) { // Recursion for vector
                        for (size_t i = 0; i < it->getValue<std::vector<Hash>>().size(); ++i) {
                            std::ostringstream os;
                            os << currentKey << "[" << i << "]";
                            getPaths(it->getValue<std::vector<Hash>>().at(i), result, os.str(), separator, fullPaths);
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

                    if (it->is<Hash>() &&
                        !it->hasAttribute(KARABO_HASH_CLASS_ID)) { // Recursion, but no hash sub classes
                        flatten(it->getValue<Hash>(), flat, currentKey, separator);
                    } else {
                        if (it->is<std::vector<Hash>>()) { // Recursion for vector
                            for (size_t i = 0; i < it->getValue<std::vector<Hash>>().size(); ++i) {
                                std::ostringstream os;
                                os << currentKey << "[" << i << "]";
                                flatten(it->getValue<std::vector<Hash>>().at(i), flat, os.str(), separator);
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


        void Hash::merge(const Hash& other, const Hash::MergePolicy policy, const std::set<std::string>& selectedPaths,
                         char sep) {
            if (selectedPaths.empty() && this->empty() && !other.empty()) {
                *this = other;
                return;
            }
            for (Hash::const_iterator it = other.begin(); it != other.end(); ++it) {
                const Hash::Node& otherNode = *it;
                const std::string& key = otherNode.getKey();

                // If we have selected paths, check whether to go on
                if (!selectedPaths.empty()) {
                    const unsigned int size =
                          (otherNode.is<std::vector<Hash>>() ? otherNode.getValue<std::vector<Hash>>().size() : 0u);
                    if (!Hash::keyIsPrefixOfAnyPath(selectedPaths, key, sep, size)) {
                        continue;
                    }
                }

                boost::optional<Hash::Node&> thisNode = this->find(key);
                if (!thisNode) {
                    // No node yet - create one with appropriate type or simply copy over and go on with next key.
                    if (otherNode.is<Hash>() && !otherNode.hasAttribute(KARABO_HASH_CLASS_ID)) {
                        thisNode = this->set(key, Hash());
                    } else if (otherNode.is<std::vector<Hash>>()) {
                        thisNode = this->set(key, std::vector<Hash>());
                    } else {
                        // Other merge policies than REPLACE or MERGE might have to be treated here.
                        this->setNode(otherNode);
                        continue;
                    }
                } else {
                    // Take care that node has proper type if it requires further treatment
                    // or just take over and go on with next key if not.
                    if (otherNode.is<Hash>() && !otherNode.hasAttribute(KARABO_HASH_CLASS_ID)) {
                        if (!thisNode->is<Hash>()) {
                            thisNode->setValue(Hash());
                        }
                    } else if (otherNode.is<std::vector<Hash>>()) {
                        if (!thisNode->is<std::vector<Hash>>()) {
                            thisNode->setValue(std::vector<Hash>());
                        }
                    } else {
                        Hash::mergeAttributes(*thisNode, otherNode.getAttributes(), policy);
                        thisNode->setValue(otherNode.getValueAsAny());
                        continue;
                    }
                }
                // We are done except of merging Hash or vector<Hash> - but both nodes are already of same type.
                // First treat the attributes:
                Hash::mergeAttributes(*thisNode, otherNode.getAttributes(), policy);
                // Now merge content:
                if (otherNode.is<Hash>()) { // Both (!) nodes are Hash
                    const std::set<std::string>& subPaths =
                          (selectedPaths.empty() ? selectedPaths : Hash::selectChildPaths(selectedPaths, key, sep));
                    thisNode->getValue<Hash>().merge(otherNode.getValue<Hash>(), policy, subPaths, sep);

                } else { // Both nodes are vector<Hash>
                    // Note that thisNode's attributes are already copied from otherNode!
                    if (thisNode->hasAttribute(KARABO_SCHEMA_ROW_SCHEMA)) {
                        Hash::mergeTableElement(otherNode, *thisNode, selectedPaths, sep);
                    } else {
                        Hash::mergeVectorHashNodes(otherNode, *thisNode, policy, selectedPaths, sep);
                    }
                }

            } // loop on other
        }


        std::set<std::string> Hash::selectChildPaths(const std::set<std::string>& paths, const std::string& key,
                                                     char separator) {
            std::set<std::string> result;


            for (const std::string& path : paths) {
                const size_t sepPos = path.find_first_of(separator);
                // Add what is left after first separator - if that is not empty and if that before separator matches
                // key:
                if (sepPos != std::string::npos                   // Found a separator,
                    && path.size() != sepPos + 1                  // there is something behind and
                    && path.compare(0, sepPos, key) == 0) {       // before the separator we have 'key'.
                    result.insert(std::string(path, sepPos + 1)); // Cut away key and separator.
                }
            }
            return result;
        }


        bool Hash::keyIsPrefixOfAnyPath(const std::set<std::string>& paths, const std::string& key, char separator,
                                        unsigned int size) {
            for (const std::string& path : paths) {
                const size_t sepPos = path.find_first_of(separator);
                const std::string& firstKeyOfPath = (sepPos == std::string::npos ? path : std::string(path, 0, sepPos));
                if (firstKeyOfPath.compare(0, key.size(), key) == 0) {
                    // firstKeyOfPath begins with key - why is there no simple string::beginsWith(..)?!
                    if (firstKeyOfPath.size() == key.size()) {
                        // In fact, they are the same:
                        return true;
                    } else {
                        // Check whether after key there is a valid (i.e. < size) [<index>]:
                        std::string croppedFirstKey(firstKeyOfPath);
                        const int index = karabo::util::getAndCropIndex(croppedFirstKey);
                        if (index != -1 && static_cast<unsigned int>(index) < size && croppedFirstKey == key) {
                            return true;
                        }
                    }
                }
            }

            return false;
        }


        std::set<unsigned int> Hash::selectIndicesOfKey(const unsigned int targetSize,
                                                        const std::set<std::string>& paths, const std::string& key,
                                                        char separator) {
            std::set<unsigned int> result;


            for (const std::string& path : paths) {
                const size_t sepPos = path.find_first_of(separator);
                const std::string& firstKeyOfPath = (sepPos == std::string::npos ? path : std::string(path, 0, sepPos));
                if (firstKeyOfPath.compare(0, key.size(), key, 0, key.size()) == 0) {
                    // firstKeyOfPath begins with key - why is there no simple string::beginsWith(..)?!
                    if (firstKeyOfPath.size() == key.size()) {
                        // In fact, they are the same:
                        // For a direct match of any path, indices in other paths are irrelevant
                        result.clear();
                        break; // no need to look for further indices
                    } else {
                        // Check whether after key there is an [<index>]:
                        std::string croppedFirstKey(firstKeyOfPath);
                        const int index = karabo::util::getAndCropIndex(croppedFirstKey);
                        if (index != -1 && index < int(targetSize) && croppedFirstKey == key) {
                            result.insert(static_cast<unsigned int>(index));
                        }
                    }
                }
            }

            return result;
        }


        void Hash::mergeAttributes(Hash::Node& targetNode, const Hash::Attributes& attrs, Hash::MergePolicy policy) {
            switch (policy) {
                case MERGE_ATTRIBUTES:
                    for (Hash::Attributes::const_iterator jt = attrs.begin(); jt != attrs.end(); ++jt) {
                        targetNode.setAttribute(jt->getKey(), jt->getValueAsAny());
                    }
                    break;
                case REPLACE_ATTRIBUTES:
                    targetNode.setAttributes(attrs);
                    // No 'default:' to get compiler warning if we add a further MergePolicy like IGNORE_ATTRIBUTES?
            }
        }


        void Hash::mergeTableElement(const Hash::Node& source, Hash::Node& target,
                                     const std::set<std::string>& selectedPaths, char separator) {
            // A table is an atomic entity, so merging two tables means replacing the target by source
            // (except that we might use a selected subset of rows).
            std::vector<Hash>& targetVec = target.getValue<std::vector<Hash>>();
            const std::vector<Hash>& sourceVec = source.getValue<std::vector<Hash>>();
            const std::set<unsigned int> selectedIndices(
                  Hash::selectIndicesOfKey(sourceVec.size(), selectedPaths, source.getKey(), separator));

            std::vector<Hash> selectedSourceVec;
            unsigned int rowCounter = 0;
            for (std::vector<Hash>::const_iterator it = sourceVec.begin(); it != sourceVec.end(); ++it, ++rowCounter) {
                if (!selectedIndices.empty() && selectedIndices.find(rowCounter) == selectedIndices.end()) {
                    // Skip non-selected indices
                    continue;
                }
                selectedSourceVec.push_back(*it);
            }
            targetVec.swap(selectedSourceVec);
        }


        void Hash::mergeVectorHashNodes(const Hash::Node& source, Hash::Node& target, Hash::MergePolicy policy,
                                        const std::set<std::string>& selectedPaths, char separator) {
            std::vector<Hash>& targetVec = target.getValue<std::vector<Hash>>();
            const std::vector<Hash>& sourceVec = source.getValue<std::vector<Hash>>();
            std::set<unsigned int> selectedIndices(
                  Hash::selectIndicesOfKey(sourceVec.size(), selectedPaths, source.getKey(), separator));

            // Merge Hashes for ordinary vector<Hash>
            std::set<unsigned int>::const_iterator it = selectedIndices.begin();
            for (size_t i = 0; i < sourceVec.size(); ++i) {
                if (!selectedIndices.empty()) {
                    // Check if the left indices are out of range ...
                    if (it == selectedIndices.end() || *it >= sourceVec.size()) break;
                }
                if (i == targetVec.size()) {
                    // Align the target vector size with the source one
                    targetVec.push_back(Hash());
                }
                if (selectedIndices.empty()) {
                    // There cannot be sub-path selections here.
                    targetVec[i].merge(sourceVec[i], policy, std::set<std::string>(), separator);
                } else if (selectedIndices.find(i) == selectedIndices.end()) {
                    // this is not our index -> don't merge!
                    continue;
                } else {
                    // sub-path selection
                    const std::string indexedKey((source.getKey() + '[') += util::toString(i) += ']');
                    const std::set<std::string> paths(Hash::selectChildPaths(selectedPaths, indexedKey, separator));
                    targetVec[i].merge(sourceVec[i], policy, paths, separator);
                    ++it;
                }
            }
        }


        Hash& Hash::operator+=(const Hash& other) {
            this->merge(other);
            return *this;
        }


        void Hash::subtract(const Hash& other, const char separator) {
            if (this->empty() || other.empty()) return;
            std::vector<std::string> candidates;
            getPaths(other, candidates, "", separator); // may be optimized to avoid list creation
            if (candidates.empty()) return;
            std::set<std::string> myset;
            getPaths(myset, separator);
            // Go through the list in reverse order to avoid
            for (std::vector<std::string>::const_reverse_iterator it = candidates.rbegin(); it != candidates.rend();
                 ++it) {
                // keep unrelated entries or empty "vector<Hash>" items
                if (myset.find(*it) == myset.end() || (*it).back() == ']') continue;
                this->erase(*it, separator);
            }

            // Remove possible vector<Hash>'s empty tails if we need this functionality
            myset.clear();
            getPaths(myset, separator);
            std::string lastPath;
            for (std::set<std::string>::const_reverse_iterator it = myset.rbegin(); it != myset.rend(); ++it) {
                size_t pos = (*it).rfind("[");
                if (pos == std::string::npos) continue;             // skip "not-an-array" paths
                const std::string& path = (*it).substr(0, pos);     // path to vector<Hash>
                if ((*it).back() == ']') {                          // vector<Hash>[...] is empty
                    if (lastPath.find(path) == std::string::npos) { // select empty tails only
                        this->erase(*it, separator);
                    }
                } else {
                    lastPath = path; // "not empty" entry encountered
                }
            }
        }


        Hash& Hash::operator-=(const Hash& other) {
            this->subtract(other);
            return *this;
        }


        bool Hash::operator==(const Hash& other) const {
            return similar(*this, other);
        }


        bool Hash::fullyEquals(const Hash& other, bool orderMatters) const {
            // Local lambda that verifies if two vectors of hashes are fully equal.
            auto vectorsHashesEqual = [](const std::vector<Hash>& lvh, const std::vector<Hash>& rvh,
                                         bool orderMatters) -> bool {
                if (lvh.size() != rvh.size()) {
                    return false;
                }
                for (size_t i = 0; i < lvh.size(); i++) {
                    if (!lvh[i].fullyEquals(rvh[i], orderMatters)) {
                        return false;
                    }
                }
                return true;
            };

            if (this->size() != other.size()) {
                return false;
            }

            // Visits all the corresponding nodes of both hashes checking for equality of keys, values and
            // attributes.
            for (Hash::const_iterator itl = this->begin(), itr = other.begin();
                 itl != this->end() && itr != other.end(); ++itl, ++itr) {
                // Figure out the 'other's (i.e. right hand) Hash::Node to compare with:
                // If orderMatters, it must be *itr. Otherwise we find the matching key.
                const Hash::Node* otherNodePtr = &(*itr);
                if ((*itl).getKey() != (*itr).getKey()) {
                    if (orderMatters) return false;
                    boost::optional<const Hash::Node&> otherNode = other.find((*itl).getKey());
                    if (!otherNode) return false;
                    otherNodePtr = &(*otherNode);
                }

                if ((*itl).getType() != (*otherNodePtr).getType()) {
                    return false;
                }

                // Checks the hash node attributes.
                const Attributes& leftNodeAttrs = (*itl).getAttributes();
                const Attributes& rightNodeAttrs = (*otherNodePtr).getAttributes();
                if (leftNodeAttrs.size() != rightNodeAttrs.size()) {
                    return false;
                }
                for (Hash::Attributes::const_iterator lAttrIt = leftNodeAttrs.begin(), rAttrIt = rightNodeAttrs.begin();
                     lAttrIt != leftNodeAttrs.end(); ++lAttrIt, ++rAttrIt) {
                    const Element<std::string>* otherAttrNodePtr = &(*rAttrIt);
                    if (lAttrIt->getKey() != rAttrIt->getKey()) {
                        if (orderMatters) return false;
                        auto otherAttrNodeIt = rightNodeAttrs.find(lAttrIt->getKey());
                        if (otherAttrNodeIt == rightNodeAttrs.mend()) return false;
                        otherAttrNodePtr =
                              &(otherAttrNodeIt->second); // otherAttrNodeIt is a map_iterator, i.e. points to a pair
                    }
                    if (lAttrIt->getType() != otherAttrNodePtr->getType()) {
                        return false;
                    }
                    if (lAttrIt->getType() == Types::HASH) {
                        if (!lAttrIt->getValue<Hash>().fullyEquals(otherAttrNodePtr->getValue<Hash>(), orderMatters)) {
                            return false;
                        }
                    } else if (lAttrIt->getType() == Types::VECTOR_HASH) {
                        if (!vectorsHashesEqual(lAttrIt->getValue<std::vector<Hash>>(),
                                                otherAttrNodePtr->getValue<std::vector<Hash>>(), orderMatters)) {
                            return false;
                        }
                    } else if (lAttrIt->getType() == Types::VECTOR_STRING) {
                        // For now treat VECTOR_STRING separately:
                        // The generic getValueAs<std::string>() below has trouble with commas in an element.
                        if (lAttrIt->getValue<std::vector<std::string>>() !=
                            otherAttrNodePtr->getValue<std::vector<std::string>>()) {
                            return false;
                        }
                    } else if (lAttrIt->getType() == Types::VECTOR_HASH_POINTER) {
                        throw KARABO_NOT_IMPLEMENTED_EXCEPTION(
                              "Hash::fullyEquals does not support VECTOR_HASH_POINTER");
                    } else if (lAttrIt->getValueAs<std::string>() != otherAttrNodePtr->getValueAs<std::string>()) {
                        return false;
                    }
                }

                // Checks the hash node values.
                if ((*itl).getType() == Types::HASH) {
                    if (!(*itl).getValue<Hash>().fullyEquals((*otherNodePtr).getValue<Hash>(), orderMatters)) {
                        return false;
                    }
                } else if ((*itl).getType() == Types::VECTOR_HASH) {
                    if (!vectorsHashesEqual((*itl).getValue<std::vector<Hash>>(),
                                            (*otherNodePtr).getValue<std::vector<Hash>>(), orderMatters)) {
                        return false;
                    }
                } else if ((*itl).getType() == Types::VECTOR_HASH_POINTER) {
                    throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Hash::fullyEquals does not support VECTOR_HASH_POINTER");
                } else if ((*itl).getType() == Types::VECTOR_STRING) {
                    // For now treat VECTOR_STRING separately:
                    // The generic getValueAs<std::string>() below has trouble with commas in an element.
                    if ((*itl).getValue<std::vector<std::string>>() !=
                        (*otherNodePtr).getValue<std::vector<std::string>>()) {
                        return false;
                    }
                } else if ((*itl).getValueAs<std::string>() != (*otherNodePtr).getValueAs<std::string>()) {
                    return false;
                }
            }

            // If this point has been reached, all checks have been made and the input hashes are fully equal.
            return true;
        }


        bool Hash::operator!=(const Hash& other) const {
            return !similar(*this, other);
        }


        /*******************************************************************
         * Attributes manipulation
         *******************************************************************/

        std::any& Hash::getAttributeAsAny(const std::string& path, const std::string& attribute, const char separator) {
            return getNode(path, separator).getAttributeAsAny(attribute);
        }


        const std::any& Hash::getAttributeAsAny(const std::string& path, const std::string& attribute,
                                                const char separator) const {
            return getNode(path, separator).getAttributeAsAny(attribute);
        }


        bool Hash::hasAttribute(const std::string& path, const std::string& attribute, const char separator) const {
            return getNode(path, separator).hasAttribute(attribute);
        }


        const Hash::Attributes& Hash::getAttributes(const std::string& path, const char separator) const {
            return getNode(path, separator).getAttributes();
        }


        Hash::Attributes& Hash::getAttributes(const std::string& path, const char separator) {
            return getNode(path, separator).getAttributes();
        }


        void Hash::setAttributes(const std::string& path, const Hash::Attributes& attributes, const char separator) {
            return getNode(path, separator).setAttributes(attributes);
        }


        void Hash::setAttributes(const std::string& path, Hash::Attributes&& attributes, const char separator) {
            return getNode(path, separator).setAttributes(std::move(attributes));
        }


        Hash* Hash::setNodesAsNeeded(const std::vector<std::string>& tokens, char separator) {
            // Loop all but last token
            Hash* tmp = this;
            for (size_t i = 0; i < tokens.size() - 1; ++i) {
                std::string token = tokens[i];
                int index = karabo::util::getAndCropIndex(token);
                if (index == -1) {                     // Just Hash
                    if (tmp->m_container.has(token)) { // Node exists
                        Hash::Node* node = &(tmp->m_container.getNode(token));
                        if (!node->is<Hash>()) {    // Node is not Hash
                            node->setValue(Hash()); // Force it to be one
                        }
                        tmp = &(node->getValue<Hash>());
                    } else { // Node does not exist
                        Hash::Node* node = &(tmp->m_container.set(token, Hash()));
                        tmp = &(node->getValue<Hash>());
                    }
                } else {                               // vector of Hash
                    if (tmp->m_container.has(token)) { // Node exists
                        Hash::Node* node = &(tmp->m_container.getNode(token));
                        if (!node->is<std::vector<Hash>>()) {             // Node is not std::vector<Hash>
                            node->setValue(std::vector<Hash>(index + 1)); // Force it to be one
                        }
                        std::vector<Hash>& hashes = node->getValue<std::vector<Hash>>();
                        if (static_cast<int>(hashes.size()) <= index) hashes.resize(index + 1);
                        tmp = &hashes[index];
                    } else { // Node does not exist
                        Hash::Node* node = &(tmp->m_container.set(token, std::vector<Hash>(index + 1)));
                        tmp = &(node->getValue<std::vector<Hash>>()[index]);
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

            for (Hash::const_iterator hit = hash.begin(); hit != hash.end(); ++hit) {
                os << fill << "'" << hit->getKey() << "'";

                const Hash::Attributes& attrs = hit->getAttributes();
                if (attrs.size() > 0) {
                    for (Hash::Attributes::const_iterator ait = attrs.begin(); ait != attrs.end(); ++ait) {
                        os << " " << ait->getKey() << "=\""
                           << ait->getValueAsShortString(30) /*<< " " << Types::to<ToLiteral>(ait->getType())*/ << "\"";
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
                    const std::vector<Hash>& hashes = hit->getValue<std::vector<Hash>>();
                    os << " @" << std::endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "[" << i << "]" << std::endl;
                        toStream(os, hashes[i], depth + 1);
                    }
                } else if (type == Types::VECTOR_HASH_POINTER) {
                    const std::vector<Hash::Pointer>& hashes = hit->getValue<std::vector<Hash::Pointer>>();
                    os << " @ (Pointer)" << std::endl;
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        os << fill << "[" << i << "]" << std::endl;
                        toStream(os, *(hashes[i]), depth + 1);
                    }
                } else if (type == Types::SCHEMA) {
                    os << " => " << hit->getValue<Schema>() << std::endl;
                } else if (Types::isPointer(type)) { // TODO Add pointer types
                    os << " => xxx " << Types::to<ToLiteral>(type) << std::endl;
                } else if (type == Types::UNKNOWN) {
                    os << " => " << hit->type().name() << " " << Types::to<ToLiteral>(type) << std::endl;
                } else {
                    os << " => " << hit->getValueAsShortString(100) << " " << Types::to<ToLiteral>(type) << std::endl;
                }
            }
        }


        bool similar(const Hash& left, const Hash& right) {
            if (left.size() != right.size()) return false;

            for (Hash::const_iterator itl = left.begin(), itr = right.begin(); itl != left.end() && itr != right.end();
                 ++itl, itr++) {
                if (!similar(*itl, *itr)) return false;
            }
            return true;
        }


        bool similar(const std::vector<Hash>& left, const std::vector<Hash>& right) {
            if (left.size() != right.size()) return false;

            for (size_t i = 0; i < left.size(); ++i) {
                if (!similar(left[i], right[i])) return false;
            }
            return true;
        }


        bool similar(const Hash::Node& left, const Hash::Node& right) {
            if (left.getType() != right.getType()) return false;

            if (left.getType() == Types::HASH) {
                return similar(left.getValue<Hash>(), right.getValue<Hash>());
            }
            if (left.getType() == Types::VECTOR_HASH) {
                return similar(left.getValue<std::vector<Hash>>(), right.getValue<std::vector<Hash>>());
            }
            return true;
        }


        size_t counter(const Hash& hash) {
            size_t partial_count = 0;
            for (Hash::const_iterator iter = hash.begin(); iter != hash.end(); ++iter) {
                const Hash::Node& ele = *iter;
                partial_count++;

                if (ele.is<Hash>() && !ele.hasAttribute(KARABO_HASH_CLASS_ID)) { // Recursion, but no hash sub classes
                    partial_count += counter(ele.getValue<Hash>());
                } else {
                    if (ele.is<std::vector<Hash>>()) {
                        const std::vector<Hash>& vect = ele.getValue<std::vector<Hash>>();
                        partial_count += vect.size();
                        for (size_t i = 0; i < vect.size(); ++i) {
                            partial_count += counter(vect[i]);
                        }
                    } else { //?????
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

                if (ele.is<Hash>() && !ele.hasAttribute(KARABO_HASH_CLASS_ID)) { // Recursion, but no hash sub classes
                    partial_count += counter(ele.getValue<Hash>(), type);
                } else {
                    if (ele.is<std::vector<Hash>>()) {
                        const std::vector<Hash>& vect = ele.getValue<std::vector<Hash>>();
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

#define COUNTER(ReferenceType, CppType) ReferenceType : return element.getValue<std::vector<CppType>>().size();


        size_t counter(const Hash::Node& element) {
            switch (element.getType()) {
                case COUNTER(Types::VECTOR_BOOL, bool); case COUNTER(Types::VECTOR_STRING, std::string);
                      case COUNTER(Types::VECTOR_CHAR, char); case COUNTER(Types::VECTOR_INT8, signed char);
                      case COUNTER(Types::VECTOR_INT16, short); case COUNTER(Types::VECTOR_INT32, int);
                      case COUNTER(Types::VECTOR_INT64, long long); case COUNTER(Types::VECTOR_UINT8, unsigned char);
                      case COUNTER(Types::VECTOR_UINT16, unsigned short);
                      case COUNTER(Types::VECTOR_UINT32, unsigned int);
                      case COUNTER(Types::VECTOR_UINT64, unsigned long long); case COUNTER(Types::VECTOR_FLOAT, float);
                      case COUNTER(Types::VECTOR_DOUBLE, double);
                      case COUNTER(Types::VECTOR_COMPLEX_FLOAT, std::complex<float>);
                      case COUNTER(Types::VECTOR_COMPLEX_DOUBLE, std::complex<double>);
                      case COUNTER(Types::VECTOR_HASH, Hash); default:
                    return 0;
            }
        }
    } // namespace util
} // namespace karabo
