/*
 * $Id: Schema.hh 4626 2011-11-01 13:01:01Z heisenb@DESY.DE $
 *
 * File:   Schema.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on August 11, 2010, 3:44 PM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_SCHEMA_HH
#define	KARABO_UTIL_SCHEMA_HH

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include "Hash.hh"
#include "StringTools.hh"
#include "ToLiteral.hh"
#include "Units.hh"

#include "karaboDll.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package util
     */
    namespace util {

        enum AccessType {

            INIT = 1 << 0,
            READ = 1 << 1,
            WRITE = 1 << 2,
        };

        inline AccessType operator|(AccessType __a, AccessType __b) {
            return AccessType(static_cast<int> (__a) | static_cast<int> (__b));
        }

        inline AccessType & operator|=(AccessType& __a, AccessType __b) {
            return __a = __a | __b;
        }

        inline AccessType operator&(AccessType __a, AccessType __b) {
            return AccessType(static_cast<int> (__a) & static_cast<int> (__b));
        }

        inline AccessType & operator&=(AccessType& __a, AccessType __b) {
            return __a = __a & __b;
        }

        /**
         * The Schema class correlates to the Hash class like an XML Schema document correlates to an XML document.
         * The Schema object is a description of type of Hash objects, expressed in terms of constraints
         * on the structure and content of Hash objects of that type. Because generally the Hash object is
         * a collection of key/value pairs of quite a common nature (std::string, boost::any), the constraints
         * applied by Schema object may define which keys has to be in Hash, some rules for the value range,
         * access type, assignment type, existence of additional attributes associated with some particular key
         * like description, visual representation, aliases, default value and so on. The Schema object is the
         * place where one can enter all these informations.
         * 
         */
        class KARABO_DECLSPEC Schema {

            #define KARABO_SCHEMA_NODE_TYPE "nodeType"
            #define KARABO_SCHEMA_LEAF_TYPE "leafType"
            #define KARABO_SCHEMA_VALUE_TYPE "valueType"
            #define KARABO_SCHEMA_CLASS_ID "classId"

            #define KARABO_SCHEMA_DISPLAYED_NAME "displayedName"
            #define KARABO_SCHEMA_DESCRIPTION "description"
            #define KARABO_SCHEMA_DEFAULT_VALUE "defaultValue"
            #define KARABO_SCHEMA_DISPLAY_TYPE "displayType"

            #define KARABO_SCHEMA_ACCESS_MODE "accessMode"
            #define KARABO_SCHEMA_ALIAS "alias"
            #define KARABO_SCHEMA_ALLOWED_STATES "allowedStates"
            #define KARABO_SCHEMA_ASSIGNMENT "assignment"
            #define KARABO_SCHEMA_TAGS "tags"

            #define KARABO_SCHEMA_OPTIONS "options"
            #define KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL "requiredAccessLevel"

            #define KARABO_SCHEMA_UNIT_ENUM "unitEnum"
            #define KARABO_SCHEMA_UNIT_NAME "unitName"
            #define KARABO_SCHEMA_UNIT_SYMBOL "unitSymbol"

            #define KARABO_SCHEMA_METRIC_PREFIX_ENUM "metricPrefixEnum"
            #define KARABO_SCHEMA_METRIC_PREFIX_NAME "metricPrefixName"
            #define KARABO_SCHEMA_METRIC_PREFIX_SYMBOL "metricPrefixSymbol"

            #define KARABO_SCHEMA_MIN_INC "minInc"
            #define KARABO_SCHEMA_MAX_INC "maxInc"
            #define KARABO_SCHEMA_MIN_EXC "minExc"
            #define KARABO_SCHEMA_MAX_EXC "maxExc"

            #define KARABO_SCHEMA_MIN_SIZE "minSize"
            #define KARABO_SCHEMA_MAX_SIZE "maxSize"

            #define KARABO_SCHEMA_WARN_LOW "warnLow"
            #define KARABO_SCHEMA_WARN_HIGH "warnHigh"

            #define KARABO_SCHEMA_ALARM_LOW "alarmLow"
            #define KARABO_SCHEMA_ALARM_HIGH "alarmHigh"

            #define KARABO_SCHEMA_ARCHIVE_POLICY "archivePolicy"

            #define KARABO_SCHEMA_MIN "min"
            #define KARABO_SCHEMA_MAX "max"

            #define KARABO_SCHEMA_OVERWRITE "overwrite"

            // Grant friendship to the GenericElement
            // GenericElement is the base class for all schema build-up helper classes
            // It will use the private addElement function
            template <class T> friend class GenericElement;

            // Container
            Hash m_hash;

            // Filter
            AccessType m_currentAccessMode;
            std::string m_currentState;
            int m_currentAccessLevel;

            // Root name
            std::string m_rootName;

            // Indices
            std::map<std::string, std::string> m_aliasToKey;

        public:

            KARABO_CLASSINFO(Schema, "Schema", "1.0")

            struct AssemblyRules {

                AccessType m_accessMode;
                std::string m_state;
                int m_accessLevel;

                AssemblyRules(const AccessType& accessMode = INIT | WRITE | READ, const std::string& state = "", const int accessLevel = -1) :
                m_accessMode(accessMode), m_state(state), m_accessLevel(accessLevel) {
                }
            };

            enum NodeType {

                LEAF,
                NODE,
                CHOICE_OF_NODES,
                LIST_OF_NODES
            };

            enum LeafType {

                PROPERTY,
                COMMAND
            };

            enum AssignmentType {

                OPTIONAL_PARAM,
                MANDATORY_PARAM,
                INTERNAL_PARAM
            };

            enum ArchivePolicy {

                EVERY_EVENT,
                EVERY_100MS,
                EVERY_1S,
                EVERY_5S,
                EVERY_10S,
                EVERY_1MIN,
                EVERY_10MIN,
                NO_ARCHIVING
            };

            enum AccessLevel {

                OBSERVER = 0,
                USER,
                OPERATOR,
                EXPERT,
                ADMIN
            };

        public:

            /**
             * Constructs empty schema for given classId
             * @param classId The factory key of the configurable class (will be stored outside the inner hash)
             * @param rules Assembly rules if the schema is assembled from a class configurations (filters access modes, states and access rights)
             */
            Schema(const std::string& classId = "", const Schema::AssemblyRules& rules = Schema::AssemblyRules());

            void setAssemblyRules(const Schema::AssemblyRules& rules);

            Schema::AssemblyRules getAssemblyRules() const;

            const std::string& getRootName() const;

            void setRootName(const std::string& rootName);

            const karabo::util::Hash& getParameterHash() const;

            karabo::util::Hash& getParameterHash();

            void setParameterHash(const karabo::util::Hash& parameterDescription);

            /**
             * This function updates the internal mapping between keys and their aliases
             * The function must be called after de-serialization in order to construct the proper inner structure
             */
            void updateAliasMap();

            /**
             * Returns all keys in the schema (no recursion)
             * Keys in inner-structures are not provided
             * @param level from which to start the traversal
             * @return array of strings
             */
            std::vector<std::string> getKeys(const std::string& path = "") const;

            /**
             * Returns all root-to-leaves paths of the schema
             * @return array of strings
             */
            std::vector<std::string> getPaths() const;

            //**********************************************
            //          General functions on Schema        *
            //**********************************************

            bool has(const std::string& path) const;

            /**
             * Merges another schema into the current one
             * @param schema Another schema to be merged
             */
            void merge(const Schema& schema);

            bool empty() const;

            //**********************************************
            //              Node property                  *
            //**********************************************

            bool isCommand(const std::string& path) const;

            bool isProperty(const std::string& path) const;

            bool isLeaf(const std::string& path) const;

            bool isNode(const std::string& path) const;

            bool isChoiceOfNodes(const std::string& path) const;

            bool isListOfNodes(const std::string& path) const;

            bool hasNodeType(const std::string& path) const;

            int getNodeType(const std::string& path) const;

            //**********************************************
            //                Value Type                  *
            //**********************************************

            //const string& getValueType(const std::string& path) const;

            Types::ReferenceType getValueType(const std::string& path) const;

            //**********************************************
            //                Access Mode                  *
            //**********************************************

            void setAccessMode(const std::string& path, const AccessType& value);

            bool hasAccessMode(const std::string& path) const;

            bool isAccessInitOnly(const std::string& path) const;

            bool isAccessReadOnly(const std::string& path) const;

            bool isAccessReconfigurable(const std::string& path) const;

            int getAccessMode(const std::string& path) const;

            //**********************************************
            //             DisplayedName                   *
            //**********************************************

            void setDisplayedName(const std::string& path, const std::string& value);

            bool hasDisplayedName(const std::string& path) const;

            const std::string& getDisplayedName(const std::string& path) const;

            //**********************************************
            //               Description                   *
            //**********************************************

            void setDescription(const std::string& path, const std::string& value);

            bool hasDescription(const std::string& path) const;

            const std::string& getDescription(const std::string& path) const;

            //**********************************************
            //                   Tags                      *
            //**********************************************

            void setTags(const std::string& path, const std::string& value, const std::string& sep = " ,;");

            bool hasTags(const std::string& path) const;

            const std::vector<std::string>& getTags(const std::string& path) const;

            //**********************************************
            //               DisplayType                   *
            //**********************************************

            void setDisplayType(const std::string& path, const std::string& value);

            bool hasDisplayType(const std::string& path) const;

            const std::string& getDisplayType(const std::string& path) const;

            //**********************************************
            //               Assignment                    *
            //**********************************************

            void setAssignment(const std::string& path, const AssignmentType& value);

            bool hasAssignment(const std::string& path) const;

            bool isAssignmentMandatory(const std::string& path) const;

            bool isAssignmentOptional(const std::string& path) const;

            bool isAssignmentInternal(const std::string& path) const;

            const int getAssignment(const std::string& path) const;

            //**********************************************
            //                  Options                    *
            //**********************************************

            void setOptions(const std::string& path, const std::string& value, const std::string& sep);

            bool hasOptions(const std::string& path) const;

            const std::vector<std::string>& getOptions(const std::string& path) const;

            //**********************************************
            //                AllowedStates                *
            //**********************************************

            void setAllowedStates(const std::string& path, const std::string& value, const std::string& sep);

            bool hasAllowedStates(const std::string& path) const;

            const std::vector<std::string>& getAllowedStates(const std::string& path) const;


            //**********************************************
            //                  RequiredAccessLevel                *
            //**********************************************

            void setRequiredAccessLevel(const std::string& path, const AccessLevel& value);

            const int getRequiredAccessLevel(const std::string& path) const;

            //**********************************************
            //                DefaultValue                 *
            //**********************************************

            template <class ValueType>
            void setDefaultValue(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE, value);
            }

            bool hasDefaultValue(const std::string&) const;

            template <class ValueType>
            const ValueType& getDefaultValue(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            template <class T>
            T getDefaultValueAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            template<typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont >
            Cont<T> getDefaultValueAs(const std::string& path) const {
                return m_hash.getAttributeAs<T, Cont>(path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            //**********************************************
            //                  Alias                      *
            //**********************************************

            bool keyHasAlias(const std::string& path) const;

            template <class AliasType>
            bool aliasHasKey(const AliasType& alias) const {
                return (m_aliasToKey.find(karabo::util::toString(alias)) != m_aliasToKey.end());
            }

            template <class AliasType>
            const AliasType& getAliasFromKey(const std::string& path) const {
                return m_hash.getAttribute < AliasType > (path, KARABO_SCHEMA_ALIAS);
            }

            template <class AliasType>
            const std::string& getKeyFromAlias(const AliasType& alias) const {
                return (m_aliasToKey.find(karabo::util::toString(alias)))->second;
            }

            std::string getAliasAsString(const std::string& path) const;

            template <class AliasType>
            void setAlias(const std::string& path, const AliasType& value) {
                m_aliasToKey[karabo::util::toString(value)] = path;
                m_hash.setAttribute<AliasType > (path, KARABO_SCHEMA_ALIAS, value);
            }

            //**********************************************
            //                  Unit                       *
            //**********************************************

            void setUnit(const std::string& path, const UnitType& value);

            bool hasUnit(const std::string& path) const;

            const int getUnit(const std::string& path) const;

            const std::string& getUnitName(const std::string& path) const;

            const std::string& getUnitSymbol(const std::string& path) const;

            //**********************************************
            //                  UnitMetricPrefix           *
            //**********************************************

            void setMetricPrefix(const std::string& path, const MetricPrefixType& value);

            bool hasMetricPrefix(const std::string& path) const;

            const int getMetricPrefix(const std::string& path) const;

            const std::string& getMetricPrefixName(const std::string& path) const;

            const std::string& getMetricPrefixSymbol(const std::string& path) const;

            //******************************************************************
            //    Specific functions for LEAF node (which is not a vector) :   *
            //    Minimum Inclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMinInc(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MIN_INC, value);
            }

            template <class ValueType>
            const ValueType& getMinInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_MIN_INC);
            }

            template <class T>
            T getMinIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_MIN_INC);
            }

            bool hasMinInc(const std::string& path) const;

            //******************************************************************
            //    Maximum Inclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMaxInc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MAX_INC, value);
            }

            template <class ValueType>
            const ValueType& getMaxInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_MAX_INC);
            }

            template <class T>
            T getMaxIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_MAX_INC);
            }

            bool hasMaxInc(const std::string& path) const;

            //******************************************************************
            //    Minimum Exclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMinExc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MIN_EXC, value);
            }

            template <class ValueType>
            const ValueType& getMinExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_MIN_EXC);
            }

            template <class T>
            T getMinExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_MIN_EXC);
            }

            bool hasMinExc(const std::string& path) const;

            //******************************************************************
            //    Maximum Exclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMaxExc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MAX_EXC, value);
            }

            template <class ValueType>
            const ValueType& getMaxExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_MAX_EXC);
            }

            template <class T>
            T getMaxExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_MAX_EXC);
            }

            bool hasMaxExc(const std::string& path) const;

            //******************************************************
            //  Specific functions for LEAF node (which is vector):*
            //  Minimum Size of the vector                         *
            //******************************************************

            void setMinSize(const std::string& path, const unsigned int& value);

            bool hasMinSize(const std::string& path) const;

            const unsigned int& getMinSize(const std::string& path) const;

            //******************************************************
            //  Specific functions for LEAF node (which is vector):*
            //  Maximum Size of the vector                         *  
            //******************************************************

            void setMaxSize(const std::string& path, const unsigned int& value);

            bool hasMaxSize(const std::string& path) const;

            const unsigned int& getMaxSize(const std::string& path) const;

            //******************************************************
            //                   WarnLow                          *  
            //******************************************************

            template <class ValueType>
            void setWarnLow(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_WARN_LOW, value);
            }

            template <class ValueType>
            const ValueType& getWarnLow(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_WARN_LOW);
            }

            template <class T>
            T getWarnLowAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_WARN_LOW);
            }

            bool hasWarnLow(const std::string& path) const;


            //******************************************************
            //                   WarnHigh                         *  
            //******************************************************

            template <class ValueType>
            void setWarnHigh(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_WARN_HIGH, value);
            }

            template <class ValueType>
            const ValueType& getWarnHigh(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_WARN_HIGH);
            }

            template <class T>
            T getWarnHighAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_WARN_HIGH);
            }

            bool hasWarnHigh(const std::string& path) const;

            //******************************************************
            //                   AlarmLow                          *  
            //******************************************************

            template <class ValueType>
            void setAlarmLow(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_ALARM_LOW, value);
            }

            template <class ValueType>
            const ValueType& getAlarmLow(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_ALARM_LOW);
            }

            template <class T>
            T getAlarmLowAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_ALARM_LOW);
            }

            bool hasAlarmLow(const std::string& path) const;

            //******************************************************
            //                   AlarmHigh                          *  
            //******************************************************

            template <class ValueType>
            void setAlarmHigh(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_ALARM_HIGH, value);
            }

            template <class ValueType>
            const ValueType& getAlarmHigh(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, KARABO_SCHEMA_ALARM_HIGH);
            }

            template <class T>
            T getAlarmHighAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, KARABO_SCHEMA_ALARM_HIGH);
            }

            bool hasAlarmHigh(const std::string& path) const;

            //**********************************************
            //               archivePolicy                 *
            //**********************************************

            void setArchivePolicy(const std::string& path, const ArchivePolicy& value);

            bool hasArchivePolicy(const std::string& path) const;

            const int& getArchivePolicy(const std::string& path) const;

            //******************************************************
            //      min/max for number of nodes in ListElement     *                       
            //******************************************************

            void setMin(const std::string& path, const int& value);

            bool hasMin(const std::string& path) const;

            const int& getMin(const std::string& path) const;

            void setMax(const std::string& path, const int& value);

            bool hasMax(const std::string& path) const;

            const int& getMax(const std::string& path) const;


            template <class T>
            void overwrite(const T& defaultValue);


            /**
             *  Help function to show all parameters
             */
            void help(const std::string& classId = "", std::ostream& os = std::cout);

            /**
             * Default output using help function
             * @param key
             * @return 
             */
            KARABO_DECLSPEC friend std::ostream & operator<<(std::ostream& os, const Schema& schema);


        private: // functions            

            void addElement(Hash::Node& node);

            void overwriteAttributes(const Hash::Node& node);

            void ensureParameterDescriptionIsComplete(Hash::Node& node) const;

            bool isAllowedInCurrentAccessMode(const Hash::Node& node) const;

            bool isAllowedInCurrentAccessLevel(const Hash::Node& node) const;

            bool isAllowedInCurrentState(const Hash::Node& node) const;

            void processingLeaf(const std::string& key, ostringstream& stream);

            void processingNode(const std::string& key, ostringstream& stream);

            void processingChoiceOfNodes(const std::string& key, ostringstream& stream);

            void processingListOfNodes(const std::string& key, ostringstream& stream);

            void processingStandardAttributes(const std::string& key, ostringstream & stream);

            std::string extractKey(const std::string& key);

            bool checkRequiredAccessLevel(const std::string& path, const Schema::AccessLevel& accessLevel) const;

            void r_updateAliasMap(const std::vector<std::string> keys, const std::string oldPath = "");

        };

        bool similar(const Schema& left, const Schema& right);
    } // namespace util
} // namespace karabo

#endif	/* KARABO_UTIL_MASTERCONFIG_HH */
