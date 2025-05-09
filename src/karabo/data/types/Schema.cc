/*
 * $Id: Schema.cc 4587 2011-10-21 10:52:13Z heisenb@DESY.DE $
 *
 * File:   Schema.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 11, 2010, 3:44 PM
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
#include "Schema.hh"

#include <deque>
#include <iosfwd>
#include <iostream>
#include <set>

#include "Exception.hh"
#include "FromLiteral.hh"
#include "Hash.hh"
#include "HashFilter.hh"
#include "StringTools.hh"

namespace karabo {
    namespace data {

        using namespace std;


        Schema::Schema(const std::string& classId, const Schema::AssemblyRules& rules)
            : m_currentAccessMode(rules.m_accessMode),
              m_currentState(rules.m_state),
              m_currentAccessLevel(rules.m_accessLevel),
              m_rootName(classId) {}


        void Schema::setRootName(const std::string& rootName) {
            m_rootName = rootName;
        }


        karabo::data::Hash& Schema::getParameterHash() {
            return m_hash;
        }


        void Schema::setParameterHash(const karabo::data::Hash& parameterDescription) {
            m_hash = parameterDescription;
        }


        void Schema::setParameterHash(karabo::data::Hash&& parameterDescription) {
            m_hash = std::move(parameterDescription);
        }


        const karabo::data::Hash& Schema::getParameterHash() const {
            return m_hash;
        }


        std::vector<std::string> Schema::getKeys(const std::string& path) const {
            std::vector<std::string> tmp;
            if (path.empty()) m_hash.getKeys(tmp);
            else if (m_hash.is<Hash>(path)) m_hash.get<Hash>(path).getKeys(tmp);

            return tmp;
        }


        std::vector<std::string> Schema::getPaths() const {
            std::vector<std::string> tmp;
            m_hash.getPaths(tmp);
            return tmp;
        }

        std::vector<std::string> Schema::getDeepPaths() const {
            std::vector<std::string> tmp;
            m_hash.getDeepPaths(tmp);
            return tmp;
        }


        void Schema::setAssemblyRules(const Schema::AssemblyRules& rules) {
            m_currentAccessMode = rules.m_accessMode;
            m_currentState = rules.m_state;
            m_currentAccessLevel = rules.m_accessLevel;
        }


        Schema::AssemblyRules Schema::getAssemblyRules() const {
            Schema::AssemblyRules rules;
            rules.m_accessMode = m_currentAccessMode;
            rules.m_accessLevel = m_currentAccessLevel;
            rules.m_state = m_currentState;
            return rules;
        }


        const std::string& Schema::getRootName() const {
            return m_rootName;
        }


        bool Schema::has(const std::string& path) const {
            return m_hash.has(path);
        }


        // TODO Implement it such that the current assembly rules are respected


        void Schema::merge(const Schema& schema) {
            this->getParameterHash().merge(schema.getParameterHash());
            updateAliasMap();
        }


        bool Schema::empty() const {
            return m_hash.empty();
        }

        //**********************************************
        //              Node property                  *
        //**********************************************


        bool Schema::isLeaf(const std::string& path) const {
            return NodeType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE)) == LEAF;
        }


        bool Schema::isNode(const std::string& path) const {
            return NodeType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE)) == NODE;
        }


        bool Schema::isChoiceOfNodes(const std::string& path) const {
            return NodeType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE)) == CHOICE_OF_NODES;
        }


        bool Schema::hasNodeType(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_NODE_TYPE);
        }


        Schema::NodeType Schema::getNodeType(const std::string& path) const {
            return NodeType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE));
        }


        bool Schema::isCommand(const std::string& path) const {
            if (this->isNode(path)) {
                return m_hash.hasAttribute(path, KARABO_SCHEMA_CLASS_ID) &&
                       m_hash.getAttribute<string>(path, KARABO_SCHEMA_CLASS_ID) == "Slot";
            }
            return false;
        }


        bool Schema::isProperty(const std::string& path) const {
            // Commands (Slots) are not leafs
            return this->isLeaf(path);
        }

        //**********************************************
        //                Value Type                   *
        //**********************************************


        Types::ReferenceType Schema::getValueType(const std::string& path) const {
            return Types::from<FromLiteral>(m_hash.getAttribute<string>(path, KARABO_SCHEMA_VALUE_TYPE));
        }

        //**********************************************
        //                Access Mode                  *
        //**********************************************


        void Schema::setAccessMode(const std::string& path, const AccessType& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE, value);
        }


        bool Schema::hasAccessMode(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ACCESS_MODE);
        }


        bool Schema::isAccessInitOnly(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE) == INIT;
        }


        bool Schema::isAccessReadOnly(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE) == READ;
        }


        bool Schema::isAccessReconfigurable(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE) == WRITE;
        }


        int Schema::getAccessMode(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ACCESS_MODE);
        }

        //**********************************************
        //                DisplayedName                *
        //**********************************************


        void Schema::setDisplayedName(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_DISPLAYED_NAME, value);
        }


        bool Schema::hasDisplayedName(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DISPLAYED_NAME);
        }


        const std::string& Schema::getDisplayedName(const std::string& path) const {
            return m_hash.getAttribute<std::string>(path, KARABO_SCHEMA_DISPLAYED_NAME);
        }

        //**********************************************
        //                Description                  *
        //**********************************************


        void Schema::setDescription(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_DESCRIPTION, value);
        }


        bool Schema::hasDescription(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DESCRIPTION);
        }


        const std::string& Schema::getDescription(const std::string& path) const {
            return m_hash.getAttribute<std::string>(path, KARABO_SCHEMA_DESCRIPTION);
        }

        //**********************************************
        //                DefaultValue                 *
        //**********************************************


        bool Schema::hasDefaultValue(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE);
        }

        //**********************************************
        //                Assignment                   *
        //**********************************************


        void Schema::setAssignment(const std::string& path, const Schema::AssignmentType& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT, static_cast<int>(value));
        }


        bool Schema::hasAssignment(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ASSIGNMENT);
        }


        bool Schema::isAssignmentMandatory(const std::string& path) const {
            return Schema::AssignmentType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT)) == MANDATORY_PARAM;
        }


        bool Schema::isAssignmentOptional(const std::string& path) const {
            return Schema::AssignmentType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT)) == OPTIONAL_PARAM;
        }


        bool Schema::isAssignmentInternal(const std::string& path) const {
            return Schema::AssignmentType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT)) == INTERNAL_PARAM;
        }


        const Schema::AssignmentType Schema::getAssignment(const std::string& path) const {
            return Schema::AssignmentType(m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT));
        }

        //**********************************************
        //                   Tags                      *
        //**********************************************


        void Schema::setTags(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, KARABO_SCHEMA_TAGS,
                                karabo::data::fromString<std::string, std::vector>(value, sep));
        }


        bool Schema::hasTags(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_TAGS);
        }


        const std::vector<std::string>& Schema::getTags(const std::string& path) const {
            return m_hash.getAttribute<std::vector<std::string> >(path, KARABO_SCHEMA_TAGS);
        }

        //**********************************************
        //                  classId                    *
        //**********************************************

        bool Schema::hasClassId(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_CLASS_ID);
        }

        const string& Schema::getClassId(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_CLASS_ID);
        }

        //**********************************************
        //                  DisplayType                *
        //**********************************************


        void Schema::setDisplayType(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE, value);
        }


        bool Schema::hasDisplayType(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE);
        }


        const string& Schema::getDisplayType(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_DISPLAY_TYPE);
        }

        //**********************************************
        //                  Alias                      *
        //**********************************************


        bool Schema::keyHasAlias(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALIAS);
        }


        string Schema::getAliasAsString(const std::string& path) const {
            return m_hash.getAttributeAs<string>(path, KARABO_SCHEMA_ALIAS);
        }


        //**********************************************
        //               Skip Validation               *
        //**********************************************


        void Schema::setSkipValidation(const std::string& path, const bool value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_SKIP_VALIDATION, value);
        }


        bool Schema::getSkipValidation(const std::string& path) {
            if (m_hash.hasAttribute(path, KARABO_SCHEMA_SKIP_VALIDATION)) {
                return m_hash.getAttributeAs<bool>(path, KARABO_SCHEMA_SKIP_VALIDATION);
            }
            return false;
        }


        //**********************************************
        //                  Options             *
        //**********************************************

        struct SetOptions {
            inline SetOptions(Hash& hash, const string& path, const string& value, const string& sep)
                : m_hash(hash), m_path(path), m_value(value), m_sep(sep) {}

            template <class T>
            inline void operator()(T*) {
                m_hash.setAttribute(m_path, KARABO_SCHEMA_OPTIONS,
                                    karabo::data::fromStringForSchemaOptions<T>(m_value, m_sep));
            }

            void error() {
                throw KARABO_PARAMETER_EXCEPTION("vectors have no options");
            }

            template <class T>
            inline void operator()(vector<T>*) {
                error();
            }

            Hash& m_hash;
            const string& m_path;
            const string& m_value;
            const string& m_sep;
        };


        void Schema::setOptions(const std::string& path, const std::string& value, const std::string& sep) {
            SetOptions setOptions(m_hash, path, value, sep);
            templatize(getValueType(path), setOptions);
        }


        bool Schema::hasOptions(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_OPTIONS);
        }


        //**********************************************
        //                AllowedStates                *
        //**********************************************


        void Schema::setAllowedStates(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_ALLOWED_STATES,
                                karabo::data::fromString<std::string, std::vector>(value, ","));
        }


        void Schema::setAllowedStates(const std::string& path, const std::vector<State>& value) {
            setAllowedStates(path, karabo::data::toString(value));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1) {
            const State arr[] = {s1};
            setAllowedStates(path, std::vector<State>(arr, arr + 1));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2) {
            const State arr[] = {s1, s2};
            setAllowedStates(path, std::vector<State>(arr, arr + 2));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3) {
            const State arr[] = {s1, s2, s3};
            setAllowedStates(path, std::vector<State>(arr, arr + 3));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3,
                                      const State& s4) {
            const State arr[] = {s1, s2, s3, s4};
            setAllowedStates(path, std::vector<State>(arr, arr + 4));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3,
                                      const State& s4, const State& s5) {
            const State arr[] = {s1, s2, s3, s4, s5};
            setAllowedStates(path, std::vector<State>(arr, arr + 5));
        }


        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3,
                                      const State& s4, const State& s5, const State& s6) {
            const State arr[] = {s1, s2, s3, s4, s5, s6};
            setAllowedStates(path, std::vector<State>(arr, arr + 6));
        }


        bool Schema::hasAllowedStates(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALLOWED_STATES);
        }


        const vector<State> Schema::getAllowedStates(const std::string& path) const {
            std::vector<string> stateList = m_hash.getAttribute<vector<string> >(path, KARABO_SCHEMA_ALLOWED_STATES);
            std::vector<State> ret;
            for (unsigned int i = 0; i != stateList.size(); ++i) {
                ret.push_back(State::fromString(stateList[i]));
            }
            return ret;
        }


        //**********************************************
        //                  RequiredAccessLevel                *
        //**********************************************


        void Schema::setRequiredAccessLevel(const std::string& path, const Schema::AccessLevel& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, static_cast<int>(value));
        }


        const int Schema::getRequiredAccessLevel(const std::string& path) const {
            std::vector<std::string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));

            std::string partialPath;
            int highestLevel = static_cast<int>(Schema::OBSERVER);


            for (const std::string& token : tokens) {
                if (partialPath.empty()) partialPath = token;
                else partialPath += "." + token;
                if (m_hash.hasAttribute(partialPath, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) {
                    int currentLevel = m_hash.getAttribute<int>(partialPath, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL);
                    if (currentLevel > highestLevel) highestLevel = currentLevel;
                }
            }
            return highestLevel;
        }


        //**********************************************
        //                  Unit                       *
        //**********************************************


        void Schema::setUnit(const std::string& path, const Unit& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_UNIT_ENUM, static_cast<int>(value));
            pair<string, string> names = karabo::data::getUnit(value);
            m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_NAME, names.first);
            m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_SYMBOL, names.second);
        }


        bool Schema::hasUnit(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_UNIT_ENUM);
        }


        const Unit Schema::getUnit(const std::string& path) const {
            return Unit(m_hash.getAttribute<int>(path, KARABO_SCHEMA_UNIT_ENUM));
        }


        const std::string& Schema::getUnitName(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_UNIT_NAME);
        }


        const std::string& Schema::getUnitSymbol(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_UNIT_SYMBOL);
        }


        //**********************************************
        //                  MetricPrefix               *
        //**********************************************


        void Schema::setMetricPrefix(const std::string& path, const MetricPrefix& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM, static_cast<int>(value));
            pair<string, string> names = karabo::data::getMetricPrefix(value);
            m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
            m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
        }


        bool Schema::hasMetricPrefix(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM);
        }


        const MetricPrefix Schema::getMetricPrefix(const std::string& path) const {
            return MetricPrefix(m_hash.getAttribute<int>(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM));
        }


        const std::string& Schema::getMetricPrefixName(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_METRIC_PREFIX_NAME);
        }


        const std::string& Schema::getMetricPrefixSymbol(const std::string& path) const {
            return m_hash.getAttribute<string>(path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL);
        }

        //**********************************************
        //    Minimum Inclusive value                  *
        //**********************************************


        bool Schema::hasMinInc(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_INC);
        }

        //**********************************************
        //    Maximum Inclusive value                  *                   *
        //**********************************************


        bool Schema::hasMaxInc(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_INC);
        }


        //**********************************************
        //    Minimum Exclusive value                  *                   *
        //**********************************************


        bool Schema::hasMinExc(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_EXC);
        }


        //**********************************************
        //    Maximum Exclusive value                  *                   *
        //**********************************************


        bool Schema::hasMaxExc(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_EXC);
        }

        //**********************************************************
        //       Specific functions for LEAF node which is vector  *
        //       Minimum Size of the vector                        *
        //**********************************************************


        void Schema::setMinSize(const std::string& path, const unsigned int& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_MIN_SIZE, value);
        }


        bool Schema::hasMinSize(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN_SIZE);
        }


        const unsigned int& Schema::getMinSize(const std::string& path) const {
            return m_hash.getAttribute<unsigned int>(path, KARABO_SCHEMA_MIN_SIZE);
        }


        //******************************************************
        //  Specific functions for LEAF node (which is vector):*
        //  Maximum Size of the vector                         *
        //******************************************************


        void Schema::setMaxSize(const std::string& path, const unsigned int& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_MAX_SIZE, value);
        }


        bool Schema::hasMaxSize(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX_SIZE);
        }


        const unsigned int& Schema::getMaxSize(const std::string& path) const {
            return m_hash.getAttribute<unsigned int>(path, KARABO_SCHEMA_MAX_SIZE);
        }

        //**********************************************
        //               archivePolicy                 *
        //**********************************************


        void Schema::setArchivePolicy(const std::string& path, const Schema::ArchivePolicy& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_ARCHIVE_POLICY, static_cast<int>(value));
        }


        bool Schema::hasArchivePolicy(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ARCHIVE_POLICY);
        }


        const Schema::ArchivePolicy Schema::getArchivePolicy(const std::string& path) const {
            return Schema::ArchivePolicy(m_hash.getAttribute<int>(path, KARABO_SCHEMA_ARCHIVE_POLICY));
        }

        //******************************************************
        //      min/max for number of nodes in TableElement     *                     *
        //******************************************************


        void Schema::setMin(const std::string& path, const int& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_MIN, value);
        }


        bool Schema::hasMin(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MIN);
        }


        const int& Schema::getMin(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_MIN);
        }


        void Schema::setMax(const std::string& path, const int& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_MAX, value);
        }


        bool Schema::hasMax(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_MAX);
        }


        const int& Schema::getMax(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_MAX);
        }


        void Schema::addElement(Hash::Node& node) {
            // TODO It seems the condition below can never be true
            // TODO If so, get rid of this code entirely
            if (node.hasAttribute(KARABO_SCHEMA_OVERWRITE)) {
                this->overwriteAttributes(node);
                return;
            }

            // Ensure completeness of node parameter description
            ensureParameterDescriptionIsComplete(node); // Will throw in case of error

            // Check whether node is allowed to be added
            bool accessModeOk = isAllowedInCurrentAccessMode(node);
            bool accessRoleOk = isAllowedInCurrentAccessLevel(node);
            bool stateOk = isAllowedInCurrentState(node);

            if (!(accessModeOk && accessRoleOk && stateOk)) return;

            if (this->isOrphaned(node)) {
                throw KARABO_LOGIC_EXCEPTION(
                      "Cannot add element with key '" + node.getKey() +
                      "' since parent node does not exist, is a leaf element or is a list/choice of nodes, but '" +
                      node.getKey() + "' is not a node.");
            } else {
                this->getParameterHash().setNode(node);
            }
        }


        void Schema::overwriteAttributes(const Hash::Node& node) {
            boost::optional<Hash::Node&> thisNodeOpt = m_hash.find(node.getKey());
            if (thisNodeOpt) {
                const Hash::Attributes& attrs = node.getAttributes();
                for (Hash::Attributes::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
                    const std::string& attributeKey = it->getKey();
                    if (thisNodeOpt->hasAttribute(attributeKey)) {
                        thisNodeOpt->setAttribute(attributeKey, it->getValueAsAny());
                    }
                }
            }
        }


        void Schema::ensureParameterDescriptionIsComplete(Hash::Node& node) const {
            std::string error;
            if (node.hasAttribute(KARABO_SCHEMA_NODE_TYPE)) {
                auto type = NodeType(node.getAttribute<int>(KARABO_SCHEMA_NODE_TYPE));
                if (type == LEAF || type == CHOICE_OF_NODES) {
                    if (!node.hasAttribute(KARABO_SCHEMA_ASSIGNMENT))
                        error = "Missing assignment, i.e. assignmentMandatory() / assignmentOptional(). ";
                }
            } else {
                error = "Missing nodeType attribute. ";
            }
            if (!node.hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) error = "Missing accessMode attribute. ";


            if (!error.empty())
                throw KARABO_PARAMETER_EXCEPTION("Bad description for parameter \"" + node.getKey() + "\": " + error);
        }


        bool Schema::isAllowedInCurrentAccessMode(const Hash::Node& node) const {
            return (static_cast<int>(m_currentAccessMode) & node.getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE));
        }


        bool Schema::isAllowedInCurrentAccessLevel(const Hash::Node& node) const {
            if (node.hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL) && (m_currentAccessLevel != -1)) {
                return m_currentAccessLevel >= node.getAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL);
            } else {
                return true;
            }
        }


        bool Schema::isAllowedInCurrentState(const Hash::Node& node) const {
            if (node.hasAttribute(KARABO_SCHEMA_ALLOWED_STATES) && !m_currentState.empty()) {
                const vector<string>& allowedStates = node.getAttribute<vector<string> >(KARABO_SCHEMA_ALLOWED_STATES);
                return (std::find(allowedStates.begin(), allowedStates.end(), m_currentState) != allowedStates.end());
            } else { // If no states are assigned, access is always possible
                return true;
            }
        }


        bool Schema::isOrphaned(const Hash::Node& node) const {
            const std::string& key = node.getKey();
            const size_t lastSep = key.find_last_of(Hash::k_defaultSep);
            if (lastSep == std::string::npos) {
                // first level key is not an orphan
                return false;
            }
            const std::string parentKey(key.substr(0, lastSep));
            if (!this->has(parentKey)) {
                // e.g. key is a.b.c, but a.b is not part of the schema
                return true;
            } else {
                switch (this->getNodeType(parentKey)) {
                    case LEAF: // leaves cannot be parents
                        return true;
                    case NODE:
                        return false;
                    case CHOICE_OF_NODES:
                        // Only nodes can be members (i.e. children) of lists and choices:
                        return (NodeType(node.getAttribute<int>(KARABO_SCHEMA_NODE_TYPE)) != NODE);
                    default: // If getNodeType would return Schema::NodeType and not int, default would not be needed:
                        throw KARABO_LOGIC_EXCEPTION("getNodeType returns unknown value '" +
                                                           data::toString(int(this->getNodeType(parentKey))) +=
                                                     "' for key '" + parentKey + "'");
                        return true;
                }
            }
        }


        ostream& operator<<(ostream& os, const Schema& schema) {
            os << "Schema for: " << schema.getRootName() << endl;
            os << schema.m_hash;
            return os;
        }


        void Schema::help(const string& classId, ostream& os) {
            ostringstream stream;
            if (classId.empty() || classId == getRootName()) {
                vector<string> keys = getKeys();


                for (const string& key : keys) {
                    int nodeType;
                    try {
                        nodeType = getNodeType(key);
                    } catch (...) {
                        nodeType = -1;
                    }
                    if (nodeType == LEAF) {
                        processingLeaf(key, stream);
                    } else if (nodeType == NODE) {
                        processingNode(key, stream);
                    } else if (nodeType == CHOICE_OF_NODES) {
                        processingChoiceOfNodes(key, stream);
                    }
                }
            } else {
                int nodeTypeClassId;
                try {
                    nodeTypeClassId = getNodeType(classId);
                } catch (...) {
                    nodeTypeClassId = -1;
                }

                if (nodeTypeClassId == LEAF) {
                    processingLeaf(classId, stream);
                }

                if (nodeTypeClassId == NODE) {
                    vector<string> keys = getKeys(classId);
                    if (!keys.empty()) {
                        for (const string& key : keys) {
                            string path = classId + "." + key;
                            int nodeType;
                            try {
                                nodeType = getNodeType(path);
                            } catch (...) {
                                nodeType = -1;
                            }

                            if (nodeType == LEAF) {
                                processingLeaf(path, stream);
                            } else if (nodeType == NODE) {
                                processingNode(path, stream);
                            } else if (nodeType == CHOICE_OF_NODES) {
                                processingChoiceOfNodes(path, stream);
                            }
                        }
                    } else {
                        processingNode(classId, stream);
                    }
                }

                if (nodeTypeClassId == CHOICE_OF_NODES) {
                    vector<string> keys = getKeys(classId);


                    for (const string& key : keys) {
                        string path = classId + "." + key;
                        processingNode(path, stream);
                    }
                }
            }

            // show results:
            os << "\n" << stream.str();
        }


        void Schema::processingLeaf(const std::string& key, ostringstream& stream) {
            string showKey = extractKey(key);

            string valueType = Types::to<ToLiteral>(getValueType(key));

            stream << "\n  " << showKey << " (" << valueType << ")" << endl;

            processingStandardAttributes(key, stream);

            if (getAccessMode(key) == INIT) stream << "     Access mode    : initialization" << endl;
            else if (getAccessMode(key) == READ) stream << "     Access mode    : read only" << endl;
            else if (getAccessMode(key) == WRITE) stream << "     Access mode    : reconfigurable" << endl;

            if (hasAllowedStates(key)) {
                const vector<State> states = getAllowedStates(key);
                stream << "     Allowed states : " << karabo::data::toString(states) << endl;
            }
        }


        void Schema::processingNode(const std::string& key, ostringstream& stream) {
            string showKey = extractKey(key);
            stream << "\n  " << showKey << " (NODE)" << endl;


            if (hasDescription(key)) stream << "     Description    : " << getDescription(key) << endl;
        }


        void Schema::processingChoiceOfNodes(const std::string& key, ostringstream& stream) {
            string showKey = extractKey(key);
            stream << "\n  " << showKey << " (CHOICE_OF_NODES)" << endl;
            processingStandardAttributes(key, stream);
        }


        void Schema::processingStandardAttributes(const std::string& key, ostringstream& stream) {
            if (getAssignment(key) == OPTIONAL_PARAM) stream << "     Assignment     : OPTIONAL" << endl;
            else if (getAssignment(key) == MANDATORY_PARAM) stream << "     Assignment     : MANDATORY" << endl;
            else if (getAssignment(key) == INTERNAL_PARAM) stream << "     Assignment     : INTERNAL" << endl;

            if (hasDefaultValue(key)) stream << "     Default value  : " << getDefaultValueAs<string>(key) << endl;


            if (hasDescription(key)) stream << "     Description    : " << getDescription(key) << endl;
        }


        string Schema::extractKey(const std::string& key) {
            string newKey;
            if (key.rfind(".") != string::npos) {
                vector<string> tokens;
                boost::split(tokens, key, boost::is_any_of("."));
                int i = tokens.size() - 1;
                newKey = tokens[i];
            } else {
                newKey = key;
            }
            return newKey;
        }


        void Schema::updateAliasMap() {
            m_aliasToKey.clear();
            r_updateAliasMap(getKeys());
        }


        void Schema::r_updateAliasMap(const vector<string> keys, const std::string oldPath) {
            for (const string& key : keys) {
                string newPath = key;
                if (!oldPath.empty()) newPath = oldPath + "." + key;
                if (keyHasAlias(newPath)) m_aliasToKey[getAliasAsString(newPath)] = newPath;
                // getNodeType(...)  may throw exception: Key 'nodeType' is not found
                int nodeType;

                try {
                    nodeType = getNodeType(newPath);
                } catch (...) {
                    nodeType = -1;
                }

                if (nodeType == NODE) {
                    r_updateAliasMap(getKeys(newPath), newPath);
                } else if (nodeType == CHOICE_OF_NODES) {
                    r_updateAliasMap(getKeys(newPath), newPath);
                }
            }
        }


        bool similar(const Schema& left, const Schema& right) {
            return similar(left.getParameterHash(), right.getParameterHash());
        }

        Schema Schema::subSchema(const std::string& subNodePath, const std::string& filterTags) const {
            Schema sub;
            const karabo::data::Hash& subHash = m_hash.get<Hash>(subNodePath);
            sub.setParameterHash(subHash);
            if (!filterTags.empty()) {
                karabo::data::Hash filteredHash;
                HashFilter::byTag(sub, subHash, filteredHash, filterTags);
                sub.setParameterHash(std::move(filteredHash));
            }

            sub.updateAliasMap();
            return sub;
        }


        Schema Schema::subSchemaByRules(const AssemblyRules& rules) const {
            std::set<std::string> selectedPaths;
            for (const std::string& path : getPaths()) { // getDeepPaths would e.g. dig into NDArrayElement details

                // Check that it belongs to selected m_accessMode - which is an OR of possible enum AccessType values
                if (!(getAccessMode(path) & rules.m_accessMode)) {
                    continue;
                }

                // Check that, in case allowed state(s) requested on both sides (rules or Schema item at path),
                // they match:
                const std::vector<State> states(hasAllowedStates(path) ? getAllowedStates(path) : std::vector<State>());
                if (!rules.m_state.empty() // empty: rules do not care about state
                    && !states.empty()     // empty: states not restricted
                    && std::find(states.begin(), states.end(), State::fromString(rules.m_state)) == states.end()) {
                    continue;
                }

                // Last check: access level
                if (rules.m_accessLevel != -1 // rules do not care about access level
                    && rules.m_accessLevel < getRequiredAccessLevel(path)) {
                    continue;
                }

                selectedPaths.insert(path);
            }

            // Finally assemble Schema out of surviving paths
            Schema result = subSchemaByPaths(selectedPaths);
            result.setAssemblyRules(rules);

            return result;
        }


        Schema Schema::subSchemaByPaths(const std::set<std::string>& paths) const {
            Schema result;
            if (!paths.empty()) {
                Hash resultHash;
                // Note: 1) Merge policy does not matter since resultHash is empty.
                //       2) paths.empty() indicates to ignore this selection and take all!
                resultHash.merge(getParameterHash(), Hash::MergePolicy::REPLACE_ATTRIBUTES, paths);
                result.setParameterHash(std::move(resultHash));
            }

            result.updateAliasMap();
            result.setRootName(getRootName());
            return result;
        }

        void Schema::setDaqDataType(const std::string& path, const DaqDataType& dataType) {
            if (!isNode(path)) {
                throw KARABO_PARAMETER_EXCEPTION("DAQ data types may only be set for node elements. Element at " +
                                                 path + " is not a node element!");
            }
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_DAQ_DATA_TYPE, static_cast<int>(dataType));
        }

        DaqDataType Schema::getDaqDataType(const std::string& path) const {
            return static_cast<DaqDataType>(m_hash.getAttribute<int>(path, KARABO_SCHEMA_DAQ_DATA_TYPE));
        }

        bool Schema::hasDaqDataType(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DAQ_DATA_TYPE);
        }


        bool Schema::isCustomNode(const std::string& path) const {
            if (isNode(path)) {
                const Hash::Attributes& attrs = m_hash.getAttributes(path);
                if (!attrs.has(KARABO_SCHEMA_CLASS_ID)) {
                    return false;
                }
                // HACK: Some are not custom nodes!
                const std::string& schemaClass = attrs.get<std::string>(KARABO_SCHEMA_CLASS_ID);
                if (schemaClass == "Slot") {
                    return false;
                }
                // Treat choices of a choice of nodes and entries in list of nodes, i.e. check whether
                // there is a mother path - if yes, check whether it points to a CHOICE_OF_NODES!
                const size_t lastDot = path.rfind(data::Hash::k_defaultSep);
                if (lastDot != std::string::npos) {
                    const std::string motherPath(path.substr(0, lastDot));
                    if (isChoiceOfNodes(motherPath)) {
                        return false;
                    }
                }
                // HACK end
                return true;
            }
            return false;
        }


        const std::string& Schema::getCustomNodeClass(const std::string& path) const {
            return m_hash.getAttribute<std::string>(path, KARABO_SCHEMA_CLASS_ID);
        }

    } // namespace data
} // namespace karabo
