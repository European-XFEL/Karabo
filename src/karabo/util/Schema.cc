/*
 * $Id: Schema.cc 4587 2011-10-21 10:52:13Z heisenb@DESY.DE $
 *
 * File:   Schema.cc
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 11, 2010, 3:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */
#include <iostream>
#include <iosfwd>
#include <deque>
#include <set>

#include "Exception.hh"
#include "Schema.hh"
#include "Hash.hh"
#include "FromLiteral.hh"
#include "StringTools.hh"
#include "karabo/webAuth/Authenticator.hh"
#include "karabo/log/Logger.hh"

namespace karabo {
    namespace util {

        using namespace std;
        using namespace karabo::core;


        //        Schema::Schema() : m_rootName("") {
        //        };

        Schema::Schema(const std::string& classId, const Schema::AssemblyRules& rules) :
        m_currentAccessMode(rules.m_accessMode), m_currentState(rules.m_state), m_currentAccessLevel(rules.m_accessLevel), m_rootName(classId) {
        }

        void Schema::setRootName(const std::string& rootName) {
            m_rootName = rootName;
        }

        karabo::util::Hash& Schema::getParameterHash() {
            return m_hash;
        }

        void Schema::setParameterHash(const karabo::util::Hash& parameterDescription) {
            m_hash = parameterDescription;
        }

        const karabo::util::Hash& Schema::getParameterHash() const {
            return m_hash;
        }


        //        const karabo::util::Hash& Schema::getParameterHash1() const {
        //            return m_hash;
        //        }

        std::vector<std::string> Schema::getKeys(const std::string& path) const {
            std::vector<std::string> tmp;
            if (path.empty())
                m_hash.getKeys(tmp);
            else if (m_hash.is<Hash > (path))
                m_hash.get<Hash > (path).getKeys(tmp);

            return tmp;
        }

        std::vector<std::string> Schema::getPaths() const {
            std::vector<std::string> tmp;
            m_hash.getPaths(tmp);
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
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE) == LEAF;
        }

        bool Schema::isNode(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE) == NODE;
        }

        bool Schema::isChoiceOfNodes(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE) == CHOICE_OF_NODES;
        }

        bool Schema::isListOfNodes(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE) == LIST_OF_NODES;
        }

        bool Schema::hasNodeType(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_NODE_TYPE);
        }

        int Schema::getNodeType(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_NODE_TYPE);
        }

        bool Schema::isCommand(const std::string& path) const {
            if (this->isNode(path)) {
                if (this->hasDisplayType(path)) {
                    // TODO Bad hack, clean this up later !!!!
                    if (m_hash.getAttribute<string>(path, KARABO_SCHEMA_DISPLAY_TYPE) == "Slot") return true;
                }
            }
            return false;
        }

        bool Schema::isProperty(const std::string& path) const {
            if (this->isLeaf(path) && m_hash.getAttribute<int>(path, KARABO_SCHEMA_LEAF_TYPE) == PROPERTY) return true;


            else return false;
        }

        //**********************************************
        //                Value Type                   *
        //**********************************************


        //const string& Schema::getValueType(const std::string& path) const {
        //   return m_hash.getAttribute<string > (path, KARABO_SCHEMA_VALUE_TYPE);
        // }

        Types::ReferenceType Schema::getValueType(const std::string& path) const {
            return Types::from<FromLiteral>(m_hash.getAttribute<string > (path, KARABO_SCHEMA_VALUE_TYPE));
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
            return m_hash.getAttribute<std::string > (path, KARABO_SCHEMA_DISPLAYED_NAME);
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
            return m_hash.getAttribute<std::string > (path, KARABO_SCHEMA_DESCRIPTION);
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
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT, value);
        }

        bool Schema::hasAssignment(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ASSIGNMENT);
        }

        bool Schema::isAssignmentMandatory(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT) == Schema::MANDATORY_PARAM;
        }

        bool Schema::isAssignmentOptional(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT) == Schema::OPTIONAL_PARAM;
        }

        bool Schema::isAssignmentInternal(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_ASSIGNMENT) == Schema::INTERNAL_PARAM;
        }

        const int Schema::getAssignment(const std::string& path) const {
            return m_hash.getAttribute<int > (path, KARABO_SCHEMA_ASSIGNMENT);
        }

        //**********************************************
        //                   Tags                      *
        //**********************************************

        void Schema::setTags(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, KARABO_SCHEMA_TAGS, karabo::util::fromString<std::string, std::vector > (value, sep));
        }

        bool Schema::hasTags(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_TAGS);
        }

        const std::vector<std::string>& Schema::getTags(const std::string& path) const {
            return m_hash.getAttribute<std::vector<std::string> >(path, KARABO_SCHEMA_TAGS);
        }


        //**********************************************
        //                  DisplayType                      *
        //**********************************************

        void Schema::setDisplayType(const std::string& path, const std::string& value) {
            m_hash.setAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE, value);
        }

        bool Schema::hasDisplayType(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_DISPLAY_TYPE);
        }

        const string& Schema::getDisplayType(const std::string& path) const {
            return m_hash.getAttribute<string > (path, KARABO_SCHEMA_DISPLAY_TYPE);
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
        //                  Options             *
        //**********************************************

        void Schema::setOptions(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (value, sep));
        }

        bool Schema::hasOptions(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_OPTIONS);
        }

        const std::vector<std::string>& Schema::getOptions(const std::string& path) const {
            return m_hash.getAttribute<std::vector<std::string> >(path, KARABO_SCHEMA_OPTIONS);
        }


        //**********************************************
        //                AllowedStates                *
        //**********************************************

        void Schema::setAllowedStates(const std::string& path, const std::string& value, const std::string& sep) {
            m_hash.setAttribute(path, KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector > (value, sep));
        }
        
        void Schema::setAllowedStates(const std::string& path, const std::vector<State>& value, const std::string& sep) {
            setAllowedStates(path, karabo::util::toString(value), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const std::string& sep) {
            const State arr[] = {s1};
            setAllowedStates(path, std::vector<State>(arr, arr+1), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const std::string& sep) {
            const State arr[] = {s1, s2};
            setAllowedStates(path, std::vector<State>(arr, arr+2), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3, const std::string& sep) {
            const State arr[] = {s1, s2, s3};
            setAllowedStates(path, std::vector<State>(arr, arr+3), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3, const State& s4, const std::string& sep) {
            const State arr[] = {s1, s2, s3, s4};
            setAllowedStates(path, std::vector<State>(arr, arr+4), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3, const State& s4, const State& s5, const std::string& sep) {
            const State arr[] = {s1, s2, s3, s4, s5};
            setAllowedStates(path, std::vector<State>(arr, arr+5), sep);
        }
        
        void Schema::setAllowedStates(const std::string& path, const State& s1, const State& s2, const State& s3, const State& s4, const State& s5, const State& s6, const std::string& sep) {
            const State arr[] = {s1, s2, s3, s4, s5, s6};
            setAllowedStates(path, std::vector<State>(arr, arr+6), sep);
        }
        
        

        bool Schema::hasAllowedStates(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALLOWED_STATES);
        }

        const vector<State> Schema::getAllowedStates(const std::string& path) const {
            std::vector<string> stateList =  m_hash.getAttribute<vector<string> >(path, KARABO_SCHEMA_ALLOWED_STATES);
            std::vector<State> ret;
            for(unsigned int i = 0; i != stateList.size(); ++i){
                ret.push_back(State::fromString(stateList[i]));
            }
            return ret;
        }


        //**********************************************
        //                  RequiredAccessLevel                *
        //**********************************************

        void Schema::setRequiredAccessLevel(const std::string& path, const Schema::AccessLevel& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, value);
        }

        const int Schema::getRequiredAccessLevel(const std::string& path) const {
            std::vector<std::string> tokens;
            boost::split(tokens, path, boost::is_any_of("."));

            std::string partialPath;
            int highestLevel = Schema::OBSERVER;

            BOOST_FOREACH(std::string token, tokens) {
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

        void Schema::setUnit(const std::string& path, const UnitType& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_UNIT_ENUM, value);
            pair<string, string> names = karabo::util::getUnit(value);
            m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_NAME, names.first);
            m_hash.setAttribute(path, KARABO_SCHEMA_UNIT_SYMBOL, names.second);
        }

        bool Schema::hasUnit(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_UNIT_ENUM);
        }

        const int Schema::getUnit(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_UNIT_ENUM);
        }

        const std::string& Schema::getUnitName(const std::string& path) const {
            return m_hash.getAttribute<string > (path, KARABO_SCHEMA_UNIT_NAME);
        }

        const std::string& Schema::getUnitSymbol(const std::string& path) const {
            return m_hash.getAttribute<string > (path, KARABO_SCHEMA_UNIT_SYMBOL);
        }


        //**********************************************
        //                  MetricPrefix               *
        //**********************************************

        void Schema::setMetricPrefix(const std::string& path, const MetricPrefixType& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM, value);
            pair<string, string> names = karabo::util::getMetricPrefix(value);
            m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_NAME, names.first);
            m_hash.setAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL, names.second);
        }

        bool Schema::hasMetricPrefix(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM);
        }

        const int Schema::getMetricPrefix(const std::string& path) const {
            return m_hash.getAttribute<int>(path, KARABO_SCHEMA_METRIC_PREFIX_ENUM);
        }

        const std::string& Schema::getMetricPrefixName(const std::string& path) const {
            return m_hash.getAttribute<string > (path, KARABO_SCHEMA_METRIC_PREFIX_NAME);
        }

        const std::string& Schema::getMetricPrefixSymbol(const std::string& path) const {
            return m_hash.getAttribute<string > (path, KARABO_SCHEMA_METRIC_PREFIX_SYMBOL);
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

        //******************************************************
        //    has/ WarnLow, WarnHigh, AlarmLow, AlarmHigh      *
        //    also for Variance                                *
        //******************************************************

        bool Schema::hasWarnLow(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_LOW);
        }

        bool Schema::hasWarnHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_HIGH);
        }

        bool Schema::hasAlarmLow(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_LOW);
        }

        bool Schema::hasAlarmHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_HIGH);
        }
        
        bool Schema::hasWarnVarianceLow(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_VARIANCE_LOW);
        }

        bool Schema::hasWarnVarianceHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_WARN_VARIANCE_HIGH);
        }

        bool Schema::hasAlarmVarianceLow(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_VARIANCE_LOW);
        }

        bool Schema::hasAlarmVarianceHigh(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ALARM_VARIANCE_HIGH);
        }
        
        bool Schema::hasRollingStatistics(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ENABLE_ROLLING_STATS);
        }
        
        unsigned int Schema::getRollingStatsEvalInterval(const std::string& path) const {
            if (!m_hash.hasAttribute(path, KARABO_SCHEMA_ENABLE_ROLLING_STATS)){
                 KARABO_LOGIC_EXCEPTION("Rolling statistics have not been enabled for '"+path+"'!");                              
            }
            return m_hash.getAttribute<unsigned int>(path, KARABO_SCHEMA_ROLLING_STATS_EVAL);
        }
        

        //**********************************************
        //               archivePolicy                 *
        //**********************************************

        void Schema::setArchivePolicy(const std::string& path, const ArchivePolicy& value) {
            m_hash.setAttribute<int>(path, KARABO_SCHEMA_ARCHIVE_POLICY, value);
        }

        bool Schema::hasArchivePolicy(const std::string& path) const {
            return m_hash.hasAttribute(path, KARABO_SCHEMA_ARCHIVE_POLICY);
        }

        const int& Schema::getArchivePolicy(const std::string& path) const {
            return m_hash.getAttribute<int> (path, KARABO_SCHEMA_ARCHIVE_POLICY);
        }

        //******************************************************
        //      min/max for number of nodes in ListElement     *                     *  
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
                KARABO_LOG_FRAMEWORK_ERROR << "Cannot add element with key '" << node.getKey()
                        << "' since parent node does not exist, is a leaf element or is a list/choice "
                        << "of nodes, but '" << node.getKey() << "' is not a node.";
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
                int type = node.getAttribute<int>(KARABO_SCHEMA_NODE_TYPE);
                if (type == Schema::LEAF || type == Schema::CHOICE_OF_NODES || type == Schema::LIST_OF_NODES) {
                    if (!node.hasAttribute(KARABO_SCHEMA_ASSIGNMENT)) error = "Missing assignment, i.e. assignmentMandatory() / assignmentOptional(). ";
                }
            } else {
                error = "Missing nodeType attribute. ";
            }
            if (!node.hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) error = "Missing accessMode attribute. ";


            if (!error.empty()) throw KARABO_PARAMETER_EXCEPTION("Bad description for parameter \"" + node.getKey() + "\": " + error);
        }

        bool Schema::isAllowedInCurrentAccessMode(const Hash::Node& node) const {
            return (m_currentAccessMode & node.getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE));
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
            } else { // If no states are assigned, access/visibility is always possible
                return true;
            }
        }

        bool Schema::isOrphaned(const Hash::Node& node) const {
            const std::string& key = node.getKey();
            const size_t lastSep = key.find_last_of('.'); // can't get default separator from Hash :-(
            if (lastSep == std::string::npos) {
                // first level key is not an orphan
                return false;
            }
            const std::string parentKey(key.substr(0, lastSep));
            if (!this->has(parentKey)) {
                // e.g. key is a.b.c, but a.b is not part of the schema
                return true;
            } else {
                switch(this->getNodeType(parentKey)) {
                    case Schema::LEAF: // leaves cannot be parents
                        return true;
                    case Schema::NODE:
                        return false;
                    case Schema::CHOICE_OF_NODES:
                    case Schema::LIST_OF_NODES:
                        // Only nodes can be members (i.e. children) of lists and choices:
                        return (node.getAttribute<int>(KARABO_SCHEMA_NODE_TYPE) != Schema::NODE);
                    default: // If getNodeType would return Schema::NodeType and not int, default would not be needed:
                        throw KARABO_LOGIC_EXCEPTION("getNodeType returns unknown value '" +
                                util::toString(this->getNodeType(parentKey)) += "' for key '" + parentKey + "'");
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
            stream << "----- HELP -----" << endl;
            if (classId.empty() || classId == getRootName()) {


                stream << "Schema: " << getRootName() << endl;
                vector<string> keys = getKeys();

                BOOST_FOREACH(string key, keys) {
                    int nodeType;
                    try {
                        nodeType = getNodeType(key);
                    } catch (...) {
                        nodeType = -1;
                    }
                    if (nodeType == Schema::LEAF) {
                        processingLeaf(key, stream);
                    } else if (nodeType == Schema::NODE) {
                        processingNode(key, stream);
                    } else if (nodeType == Schema::CHOICE_OF_NODES) {
                        processingChoiceOfNodes(key, stream);
                    } else if (nodeType == Schema::LIST_OF_NODES) {
                        processingListOfNodes(key, stream);
                    }
                }
            } else {
                stream << "Schema: " << getRootName() << ", key: " << classId << endl;
                
                int nodeTypeClassId;
                try {
                    nodeTypeClassId = getNodeType(classId);
                } catch (...) {
                    nodeTypeClassId = -1;
                }
                
                if (nodeTypeClassId == Schema::LEAF) {
                    stream << "LEAF element" << endl;
                    processingLeaf(classId, stream);
                }

                if (nodeTypeClassId == Schema::NODE) {

                    vector<string> keys = getKeys(classId);
                    if (!keys.empty()) {


                        stream << "NODE element" << endl;

                        BOOST_FOREACH(string key, keys) {
                            string path = classId + "." + key;
                            int nodeType = -1;
                            try {
                                nodeType = getNodeType(path);
                            } catch (...) {
                                nodeType = -1;
                            }
                            
                            if (nodeType == Schema::LEAF) {
                                processingLeaf(path, stream);
                            } else if (nodeType == Schema::NODE) {
                                processingNode(path, stream);
                            } else if (nodeType == Schema::CHOICE_OF_NODES) {
                                processingChoiceOfNodes(path, stream);
                            } else if (nodeType == Schema::LIST_OF_NODES) {
                                processingListOfNodes(path, stream);
                            }
                        }
                    } else {
                        stream << "NODE element (contains no other elements)" << endl;
                        processingNode(classId, stream);
                    }
                }

                if (nodeTypeClassId == Schema::CHOICE_OF_NODES) {


                    stream << "CHOICE element" << endl;
                    vector<string> keys = getKeys(classId);

                    BOOST_FOREACH(string key, keys) {
                        string path = classId + "." + key;
                        processingNode(path, stream);
                    }
                }

                if (nodeTypeClassId == Schema::LIST_OF_NODES) {


                    stream << "LIST element" << endl;
                    vector<string> keys = getKeys(classId);

                    BOOST_FOREACH(string key, keys) {


                        string path = classId + "." + key;
                        processingNode(path, stream);
                    }
                }
            }

            //show results:
            os << "\n" << stream.str();
        }

        void Schema::processingLeaf(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);

            string valueType = Types::to<ToLiteral>(getValueType(key));

            stream << "\n  ." << showKey << " (" << valueType << ")" << endl;

            processingStandardAttributes(key, stream);

            if (getAccessMode(key) == INIT)
                stream << "     " << "Access mode    : initialization" << endl;
            else if (getAccessMode(key) == READ)
                stream << "     " << "Access mode    : read only" << endl;
            else if (getAccessMode(key) == WRITE)
                stream << "     " << "Access mode    : reconfigurable" << endl;

            if (hasAllowedStates(key)) {
                const vector<State> states = getAllowedStates(key);
                stream << "     " << "Allowed states : " << karabo::util::toString(states) << endl;
            }

        }

        void Schema::processingNode(const std::string& key, ostringstream & stream) {
            string showKey = extractKey(key);
            stream << "\n  ." << showKey << " (NODE)" << endl;


            if (hasDescription(key))
                stream << "     " << "Description    : " << getDescription(key) << endl;

        }

        void Schema::processingChoiceOfNodes(const std::string& key, ostringstream & stream) {


            string showKey = extractKey(key);
            stream << "\n  ." << showKey << " (CHOICE_OF_NODES)" << endl;
            processingStandardAttributes(key, stream);
        }

        void Schema::processingListOfNodes(const std::string& key, ostringstream & stream) {


            string showKey = extractKey(key);
            stream << "\n  ." << showKey << " (LIST_OF_NODES)" << endl;
            processingStandardAttributes(key, stream);
        }

        void Schema::processingStandardAttributes(const std::string& key, ostringstream & stream) {
            if (getAssignment(key) == OPTIONAL_PARAM)
                stream << "     " << "Assignment     : OPTIONAL" << endl;
            else if (getAssignment(key) == MANDATORY_PARAM)
                stream << "     " << "Assignment     : MANDATORY" << endl;
            else if (getAssignment(key) == INTERNAL_PARAM)
                stream << "     " << "Assignment     : INTERNAL" << endl;

            if (hasDefaultValue(key))
                stream << "     " << "Default value  : " << getDefaultValueAs<string > (key) << endl;


            if (hasDescription(key))
                stream << "     " << "Description    : " << getDescription(key) << endl;
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
            r_updateAliasMap(getKeys());
        }

        void Schema::r_updateAliasMap(const vector<string> keys, const std::string oldPath) {

            BOOST_FOREACH(string key, keys) {
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
                    
                if (nodeType == Schema::NODE) {
                    r_updateAliasMap(getKeys(newPath), newPath);
                } else if (nodeType == Schema::CHOICE_OF_NODES) {
                    r_updateAliasMap(getKeys(newPath), newPath);
                } else if (nodeType == Schema::LIST_OF_NODES) {
                    r_updateAliasMap(getKeys(newPath), newPath);
                }
            }
        }

        bool similar(const Schema& left, const Schema& right) {
            return similar(left.getParameterHash(), right.getParameterHash());
        }
    }
}
