/*
 * $Id: Schema.hh 4626 2011-11-01 13:01:01Z heisenb@DESY.DE $
 *
 * File:   Schema.hh
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


#ifndef KARABO_UTIL_SCHEMA_HH
#define KARABO_UTIL_SCHEMA_HH

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <set>
#include <tuple>

#include "AlarmConditions.hh"
#include "Hash.hh"
#include "State.hh"
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
            return AccessType(static_cast<int>(__a) | static_cast<int>(__b));
        }

        inline AccessType& operator|=(AccessType& __a, AccessType __b) {
            return __a = __a | __b;
        }

        inline AccessType operator&(AccessType __a, AccessType __b) {
            return AccessType(static_cast<int>(__a) & static_cast<int>(__b));
        }

        inline AccessType& operator&=(AccessType& __a, AccessType __b) {
            return __a = __a & __b;
        }

        enum DaqDataType {
            PULSE = 0,
            TRAIN = 1,
            PULSEMASTER = 10,
            TRAINMASTER = 11,
        };

        /**
         * An enum specifying the DAQ storage policy
         */
        enum DAQPolicy {
            UNSPECIFIED = -1,
            OMIT = 0,
            SAVE = 1,
        };

        /**
         * The Schema class correlates to the Hash class like an XML Schema document correlates to an XML document.
         * The Schema object is a description of type of Hash objects, expressed in terms of constraints
         * on the structure and content of Hash objects of that type. Because generally the Hash object is
         * a collection of key/value pairs of quite a common nature (std::string, std::any), the constraints
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
#define KARABO_SCHEMA_ROW_SCHEMA "rowSchema"
#define KARABO_SCHEMA_SKIP_VALIDATION "skipValidation"

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

#define KARABO_SCHEMA_RELATIVE_ERROR "relativeError"
#define KARABO_SCHEMA_ABSOLUTE_ERROR "absoluteError"

#define KARABO_SCHEMA_MIN_SIZE "minSize"
#define KARABO_SCHEMA_MAX_SIZE "maxSize"


#define KARABO_SCHEMA_ARCHIVE_POLICY "archivePolicy"

#define KARABO_SCHEMA_MIN "min"
#define KARABO_SCHEMA_MAX "max"

#define KARABO_SCHEMA_OVERWRITE "overwrite"

#define KARABO_SCHEMA_ALARM_ACK "alarmNeedsAck"
#define KARABO_SCHEMA_ALARM_INFO "alarmInfo"

#define KARABO_RUNTIME_SCHEMA_UPDATE "runtimeSchemaUpdates"

#define KARABO_SCHEMA_DAQ_DATA_TYPE "daqDataType"

#define KARABO_SCHEMA_DAQ_POLICY "daqPolicy"

#define KARABO_SCHEMA_DISPLAY_TYPE_BIN "bin"
#define KARABO_SCHEMA_DISPLAY_TYPE_OCT "oct"
#define KARABO_SCHEMA_DISPLAY_TYPE_HEX "hex"
#define KARABO_SCHEMA_DISPLAY_TYPE_BITSET "Bitset"
#define KARABO_SCHEMA_DISPLAY_TYPE_FILEOUT "fileOut"
#define KARABO_SCHEMA_DISPLAY_TYPE_FILEIN "fileIn"
#define KARABO_SCHEMA_DISPLAY_TYPE_IMAGEDATA "ImageData"
#define KARABO_SCHEMA_DISPLAY_TYPE_CURVE "Curve"
#define KARABO_SCHEMA_DISPLAY_TYPE_SCENES "Scenes"
#define KARABO_SCHEMA_DISPLAY_TYPE_RUNCONFIGURATOR "RunConfigurator"
#define KARABO_SCHEMA_DISPLAY_TYPE_DIRECTORY "directory"
#define KARABO_SCHEMA_DISPLAY_TYPE_OUTPUT_CHANNEL "OutputChannel"
#define KARABO_SCHEMA_DISPLAY_TYPE_INPUT_CHANNEL "InputChannel"
#define KARABO_SCHEMA_DISPLAY_TYPE_STATE "State"

#define KARABO_SCHEMA_ALLOWED_ACTIONS "allowedActions"

            // Grant friendship to the GenericElement
            // GenericElement is the base class for all schema build-up helper classes
            // It will use the private addElement function
            template <class T>
            friend class GenericElement;

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

            DAQPolicy m_defaultDAQPolicy;

           public:
            KARABO_CLASSINFO(Schema, "Schema", "1.0")

            /**
             * The AssemblyRules specify how the karabo::util::Configurator
             * assembles the configuration
             */
            struct AssemblyRules {
                AccessType m_accessMode;
                std::string m_state;
                int m_accessLevel;

                AssemblyRules(const AccessType& accessMode = INIT | WRITE | READ, const std::string& state = "",
                              const int accessLevel = -1)
                    : m_accessMode(accessMode), m_state(state), m_accessLevel(accessLevel) {}
            };

            /**
             * An enum specifying the type of Node-like elements (NodeElement,
             * ChoiceElement, ListElement)
             */
            enum NodeType {

                LEAF,
                NODE,
                CHOICE_OF_NODES,
                LIST_OF_NODES
            };

            /**
             * An enum specifying the type of Leaf-like elements
             */
            enum LeafType {

                PROPERTY,
                COMMAND,
                STATE,
                ALARM_CONDITION
            };

            /**
             * An enum specifying assignment restrictions for elements
             */
            enum AssignmentType {

                OPTIONAL_PARAM,
                MANDATORY_PARAM,
                INTERNAL_PARAM
            };

            /**
             * An enum specifying the archiving interval for updates to
             * elements
             */
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

            /**
             * An enum specifying the access-level needed to view or alter elements.
             */
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
             * @param rules Assembly rules if the schema is assembled from a class configurations (filters access modes,
             *              states and access rights)
             */
            Schema(const std::string& classId = "", const Schema::AssemblyRules& rules = Schema::AssemblyRules());

            virtual ~Schema(){

            };

            /**
             * Set the assembly rules for this schema
             * @param rules
             */
            void setAssemblyRules(const Schema::AssemblyRules& rules);

            /**
             * Get the assembly rules of this schema
             * @return
             */
            Schema::AssemblyRules getAssemblyRules() const;

            /**
             * Get the schema's root element's name
             * @return
             */
            const std::string& getRootName() const;

            /**
             * St the schema's root element's name
             * @param rootName
             */
            void setRootName(const std::string& rootName);

            /**
             * Get a Hash representation of this Schema. It will have the same
             * hierarchical structure as the schema. Each keys attributes map
             * to the attributes/modifiers of the respective schema element. The
             * value types to not match up with the schema but are all of type
             * INT32. If you need a Hash holding empty values of correct data
             * types use the karabo::DeviceClient::getDataSchema method instead
             * @return
             */
            const karabo::util::Hash& getParameterHash() const;

            karabo::util::Hash& getParameterHash();

            /**
             * Set the parameter hash of the schema. Implicitly alters the schema
             * as this Hash is its underlying description.
             * @param parameterDescription - must match the structure of the Schema internal Hash
             *
             */
            void setParameterHash(const karabo::util::Hash& parameterDescription);

            /**
             * Set (with move semantics) the parameter hash of the schema. Implicitly alters the schema
             * as this Hash is its underlying description.
             * @param parameterDescription - must match the structure of the Schema internal Hash
             */
            void setParameterHash(karabo::util::Hash&& parameterDescription);

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
            std::vector<std::string> getDeepPaths() const;

            //**********************************************
            //          General functions on Schema        *
            //**********************************************

            /**
             * Check if a given path exists in the Schema
             * @param path
             * @return
             */
            bool has(const std::string& path) const;

            /**
             * Merges another schema into the current one
             * @param schema Another schema to be merged
             */
            void merge(const Schema& schema);

            /**
             * Check if the Schema is empty
             * @return
             */
            bool empty() const;

            //**********************************************
            //              Node property                  *
            //**********************************************

            /**
             * Check if the element at path is a command
             * @param path
             * @return
             */
            bool isCommand(const std::string& path) const;

            /**
             * Check if the element at path is a property
             * @param path
             * @return
             */
            bool isProperty(const std::string& path) const;

            /**
             * Check if the element at path is of Leaf-type
             * @param path
             * @return
             */
            bool isLeaf(const std::string& path) const;

            /**
             * Check if the element at path is of Node-Element-type
             * @param path
             * @return
             */
            bool isNode(const std::string& path) const;

            /**
             * Check if the element at path is of Choice-of-Nodes type
             * @param path
             * @return
             */
            bool isChoiceOfNodes(const std::string& path) const;

            /**
             * Check if the element at path is of List-of-Nodes type
             * @param path
             * @return
             */
            bool isListOfNodes(const std::string& path) const;

            /**
             * Check if the element at path has one of the node element types
             * (NodeElement, ListElement, ChoiceElement)
             * @param path
             * @return
             */
            bool hasNodeType(const std::string& path) const;

            /**
             * Get the node type of the element identified by path. You should
             * check if it is of Node-type first with Schema::hasNodeType()
             * @param path
             * @return a value mapping to the Schema::NodeType enum
             */
            int getNodeType(const std::string& path) const;

            //**********************************************
            //                Value Type                  *
            //**********************************************

            /**
             * Get the valuetype of the element at path. You should check that
             * it is of LeafType first using Schema::isLeaf
             * @param path
             * @return
             */
            Types::ReferenceType getValueType(const std::string& path) const;

            //**********************************************
            //                Access Mode                  *
            //**********************************************

            /**
             * Set the access mode for the element identified by path
             * @param path
             * @param value
             */
            void setAccessMode(const std::string& path, const AccessType& value);

            /**
             * Check if the element identified by path has an access mode set
             * @param path
             * @return
             */
            bool hasAccessMode(const std::string& path) const;

            /**
             * Check if the element identified by path is init-only
             * @param path
             * @return
             */
            bool isAccessInitOnly(const std::string& path) const;

            /**
             * Check if the element identified by path is read-only
             * @param path
             * @return
             */
            bool isAccessReadOnly(const std::string& path) const;

            /**
             * Check if the element identified by path is reconfigurable
             * @param path
             * @return
             */
            bool isAccessReconfigurable(const std::string& path) const;

            /**
             * Get the access mode for the element identified by path
             * @param path
             * @return maps to the Schema::AccessMode enum
             */
            int getAccessMode(const std::string& path) const;

            //**********************************************
            //             DisplayedName                   *
            //**********************************************

            /**
             * Set the displayed name for the element identified by path
             * @param path
             * @param value
             */
            void setDisplayedName(const std::string& path, const std::string& value);

            /**
             * Check if the element identified by path has a displayed name set
             * @param path
             * @return
             */
            bool hasDisplayedName(const std::string& path) const;

            /**
             * Set the displayed name for the element identified by path
             * @param path
             * @return
             */
            const std::string& getDisplayedName(const std::string& path) const;

            //**********************************************
            //               Description                   *
            //**********************************************

            /**
             * Set the description for the element identified by path
             * @param path
             * @param value
             */
            void setDescription(const std::string& path, const std::string& value);

            /**
             * Check if the element identified by path has a description set
             * @param path
             * @return
             */
            bool hasDescription(const std::string& path) const;

            /**
             * Get the description for the element identified by path
             * @param path
             * @return
             */
            const std::string& getDescription(const std::string& path) const;

            //**********************************************
            //                   Tags                      *
            //**********************************************

            /**
             * Set the tags for the element identified by path
             * @param path
             * @param value
             */
            void setTags(const std::string& path, const std::string& value, const std::string& sep = " ,;");

            /**
             * Check if the element identified by path has  tags set
             * @param path
             * @return
             */
            bool hasTags(const std::string& path) const;

            /**
             * Get the tags for the element identified by path
             * @param path
             * @return
             */
            const std::vector<std::string>& getTags(const std::string& path) const;

            //**********************************************
            //               classId                       *
            //**********************************************

            /**
             * Check if the element identified by path has a classId
             * @param path
             * @return
             */
            bool hasClassId(const std::string& path) const;

            /**
             * Get the classId for the element identified by path
             * @param path
             * @return
             */
            const std::string& getClassId(const std::string& path) const;

            //**********************************************
            //               DisplayType                   *
            //**********************************************

            /**
             * Set the display type for the element identified by path
             * @param path
             * @param value
             */
            void setDisplayType(const std::string& path, const std::string& value);

            /**
             * Check if the element identified by path has a display type set
             * @param path
             * @return
             */
            bool hasDisplayType(const std::string& path) const;

            /**
             * Get the display type for the element identified by path
             * @param path
             * @return
             */
            const std::string& getDisplayType(const std::string& path) const;

            //**********************************************
            //               Assignment                    *
            //**********************************************

            /**
             * Set the assignment type for the element identified by path
             * @param path
             * @param value
             */
            void setAssignment(const std::string& path, const AssignmentType& value);

            /**
             * Check if the element identified by path has a assignment type set
             * @param path
             * @return
             */
            bool hasAssignment(const std::string& path) const;

            /**
             * Check if the element identified by path is assignment mandatory
             * @param path
             * @return
             */
            bool isAssignmentMandatory(const std::string& path) const;

            /**
             * Check if the element identified by path is assignment optional
             * @param path
             * @return
             */
            bool isAssignmentOptional(const std::string& path) const;

            /**
             * Check if the element identified by path is assignment internal
             * @param path
             * @return
             */
            bool isAssignmentInternal(const std::string& path) const;

            /**
             * Get the assignment type for the element identified by path
             * @param path
             * @return
             */
            const int getAssignment(const std::string& path) const;

            //**********************************************
            //               Skip Validation               *
            //**********************************************

            /**
             * Set the element identified by path to skip validation
             * @param path
             * @param value
             */
            void setSkipValidation(const std::string& path, const bool value);

            /**
             * Check if the element identified by path is set to skip validation
             * @param path
             * @return
             */
            bool getSkipValidation(const std::string& path);

            //**********************************************
            //                  Options                    *
            //**********************************************

            /**
             * Set options for the element identified by path.
             * @param path
             * @param value a stringified list of options, separated by sep
             * @param sep
             */
            void setOptions(const std::string& path, const std::string& value, const std::string& sep);

            /**
             * Check if the element identified by path has options set
             * @param path
             * @return
             */
            bool hasOptions(const std::string& path) const;

            /**
             * Get the options set for the element identified by path
             * @param path
             * @return
             */
            template <class T>
            const std::vector<T>& getOptions(const std::string& path) const {
                return m_hash.getAttribute<std::vector<T> >(path, KARABO_SCHEMA_OPTIONS);
            }

            //**********************************************
            //                AllowedStates                *
            //**********************************************

            // overloads for up to six states
            /**
             * Set new allowed states for the element identified by path
             * @param path
             * @param s1
             */
            void setAllowedStates(const std::string& path, const karabo::util::State& s1);
            void setAllowedStates(const std::string& path, const karabo::util::State& s1,
                                  const karabo::util::State& s2);
            void setAllowedStates(const std::string& path, const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3);
            void setAllowedStates(const std::string& path, const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4);
            void setAllowedStates(const std::string& path, const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5);
            void setAllowedStates(const std::string& path, const karabo::util::State& s1, const karabo::util::State& s2,
                                  const karabo::util::State& s3, const karabo::util::State& s4,
                                  const karabo::util::State& s5, const karabo::util::State& s6);


            // generic interface
            void setAllowedStates(const std::string& path, const std::vector<karabo::util::State>& value);


            /**
             * Check if the element identified by path has allowed states set
             * @param path
             * @return
             */
            bool hasAllowedStates(const std::string& path) const;

            /**
             * Get the allowed states set for the element identified by path
             * @param path
             * @return
             */
            const std::vector<karabo::util::State> getAllowedStates(const std::string& path) const;


            //**********************************************
            //                  RequiredAccessLevel                *
            //**********************************************

            /**
             * Set the required access level for the element identified by path
             * @param path
             * @param value
             */
            void setRequiredAccessLevel(const std::string& path, const AccessLevel& value);

            /**
             * Get the required access level for the element identified by path
             * @param path
             * @return maps to the Schema::AccessLevel enum
             */
            const int getRequiredAccessLevel(const std::string& path) const;

            //**********************************************
            //                DefaultValue                 *
            //**********************************************

            /**
             * Set the new default value for the element identified by path
             * @param path
             * @param value
             */
            template <class ValueType>
            void setDefaultValue(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_DEFAULT_VALUE, value);
            }

            /**
             * Check if the element identified by path has a default value set
             * @param
             * @return
             */
            bool hasDefaultValue(const std::string&) const;

            /**
             * Get the default value set for the element identified by path.
             * Use Schema::hasDefaultValue first to check if one is set. Will
             * throw an Exception if ValueType does not match the type of the
             * default value set (it will be of the Element's value type)
             * @param path
             * @return
             */
            template <class ValueType>
            const ValueType& getDefaultValue(const std::string& path) const {
                return m_hash.getAttribute<ValueType>(path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            /**
             * Get the default value set for the element identified by path.
             * Use Schema::hasDefaultValue first to check if one is set. Will
             * throw an Exception if the type of the default value set
             * (it will be of the Element's value type) cannot be casted to T
             * @param path
             * @return
             */
            template <class T>
            T getDefaultValueAs(const std::string& path) const {
                return m_hash.getAttributeAs<T>(path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            /**
             * Get the default value set for the element identified by path.
             * Use Schema::hasDefaultValue first to check if one is set. Will
             * throw an Exception if the type of the default value set
             * (it will be of the Element's value type) cannot be casted to T.
             * Overload for sequence types.
             * @param path
             * @return
             */
            template <typename T, template <typename Elem, typename = std::allocator<Elem> > class Cont>
            Cont<T> getDefaultValueAs(const std::string& path) const {
                return m_hash.getAttributeAs<T, Cont>(path, KARABO_SCHEMA_DEFAULT_VALUE);
            }

            //**********************************************
            //                  Alias                      *
            //**********************************************

            /**
             * Check if an alias has been set for path
             * @param path
             * @return
             */
            bool keyHasAlias(const std::string& path) const;

            /**
             * Check if a key with alias exists
             * @param alias
             * @return
             */
            template <class AliasType>
            bool aliasHasKey(const AliasType& alias) const {
                return (m_aliasToKey.find(karabo::util::toString(alias)) != m_aliasToKey.end());
            }

            /**
             * Get the alias mapping to the key at path. Check if an alias has
             * been set first by using Schema::keyHasAlias
             * @param path
             * @return
             */
            template <class AliasType>
            const AliasType& getAliasFromKey(const std::string& path) const {
                return m_hash.getAttribute<AliasType>(path, KARABO_SCHEMA_ALIAS);
            }

            /**
             * Get the key the alias maps to. Check if the alias maps to a key
             * first by using Schema::aliasHasKey
             * @param path
             * @return
             */
            template <class AliasType>
            const std::string& getKeyFromAlias(const AliasType& alias) const {
                return (m_aliasToKey.find(karabo::util::toString(alias)))->second;
            }

            /**
             * Get the alias for path as string. Check if an alias has
             * been set first by using Schema::keyHasAlias
             * @param path
             * @return
             */
            std::string getAliasAsString(const std::string& path) const;

            /**
             * Set an alias for path
             * @param path
             * @param value
             */
            template <class AliasType>
            void setAlias(const std::string& path, const AliasType& value) {
                m_aliasToKey[karabo::util::toString(value)] = path;
                m_hash.setAttribute<AliasType>(path, KARABO_SCHEMA_ALIAS, value);
            }

            //**********************************************
            //                  Unit                       *
            //**********************************************

            /**
             * Set the unit for the element identified by path
             * @param path
             * @param value
             */
            void setUnit(const std::string& path, const UnitType& value);

            /**
             * Check if the element identified by path has an unit set
             * @param path
             * @return
             */
            bool hasUnit(const std::string& path) const;

            /**
             * Get the unit set for the element identified by path. Check if the
             * element has an unit set first using Schema::hasUnit()
             * @param path
             * @return maps to a karabo::util::UnitType enum
             */
            const int getUnit(const std::string& path) const;

            /**
             * Get the unit name for the element identified by path. Check if the
             * element has an unit set first using Schema::hasUnit()
             * @param path
             * @return
             */
            const std::string& getUnitName(const std::string& path) const;

            /**
             * Get the unit symbol for the element identified by path. Check if the
             * element has an unit set first using Schema::hasUnit()
             * @param path
             * @return
             */
            const std::string& getUnitSymbol(const std::string& path) const;

            //**********************************************
            //                  UnitMetricPrefix           *
            //**********************************************

            /**
             * Set the metric prefix for the element identified by path
             * @param path
             * @param value
             */
            void setMetricPrefix(const std::string& path, const MetricPrefixType& value);

            /**
             * Check if the element identified by path has an metric prefix set
             * @param path
             * @return
             */
            bool hasMetricPrefix(const std::string& path) const;

            /**
             * Get the metric prefix set for the element identified by path. Check if the
             * element has an metric prefix set first using Schema::hasMetricPrefix()
             * @param path
             * @return maps to a karabo::util::UnitType enum
             */
            const int getMetricPrefix(const std::string& path) const;

            /**
             * Get the metric prefix name for the element identified by path. Check if the
             * element has an metric prefix set first using Schema::hasMetricPrefix()
             * @param path
             * @return
             */
            const std::string& getMetricPrefixName(const std::string& path) const;

            /**
             * Get the metric prefix symbol for the element identified by path. Check if the
             * element has an metric prefix set first using Schema::hasMetricPrefix()
             * @param path
             * @return
             */
            const std::string& getMetricPrefixSymbol(const std::string& path) const;

            //******************************************************************
            //    Specific functions for LEAF node (which is not a vector) :   *
            //    Minimum Inclusive value                                      *
            //******************************************************************

            /**
             * Set the minimum inclusive restriction for setting values to the
             * element identified by path
             * @param path
             * @param value
             */
            template <class ValueType>
            void setMinInc(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MIN_INC, value);
            }

            /**
             * Get the minimum inclusive restriction for setting values to the
             * element identified by path. Will throw an exception if ValueType
             * is not of the type of the bound (usually the Element's data type)
             * @param path
             */
            template <class ValueType>
            const ValueType& getMinInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType>(path, KARABO_SCHEMA_MIN_INC);
            }

            /**
             * Get the minimum inclusive restriction for setting values to the
             * element identified by path. Will throw an exception if T
             * is not of the type of the bound (usually the Element's data type)
             * or cannot be casted to T.
             * @param path
             */
            template <class T>
            T getMinIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T>(path, KARABO_SCHEMA_MIN_INC);
            }

            /**
             * Check if a minimum inclusive restriction for setting values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMinInc(const std::string& path) const;

            //******************************************************************
            //    Maximum Inclusive value                                      *
            //******************************************************************

            /**
             * Set the maximum inclusive restriction for setting values to the
             * element identified by path
             * @param path
             * @param value
             */
            template <class ValueType>
            void setMaxInc(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MAX_INC, value);
            }

            /**
             * Get the maximum inclusive restriction for setting values to the
             * element identified by path. Will throw an exception if ValueType
             * is not of the type of the bound (usually the Element's data type)
             * @param path
             */
            template <class ValueType>
            const ValueType& getMaxInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType>(path, KARABO_SCHEMA_MAX_INC);
            }

            /**
             * Get the maximum inclusive restriction for setting values to the
             * element identified by path. Will throw an exception if T
             * is not of the type of the bound (usually the Element's data type)
             * or cannot be casted to T.
             * @param path
             */
            template <class T>
            T getMaxIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T>(path, KARABO_SCHEMA_MAX_INC);
            }

            /**
             * Check if a maximum inclusive restriction for setting values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMaxInc(const std::string& path) const;

            //******************************************************************
            //    Minimum Exclusive value                                      *
            //******************************************************************

            /**
             * Set the minimum exclusive restriction for setting values to the
             * element identified by path
             * @param path
             * @param value
             */
            template <class ValueType>
            void setMinExc(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MIN_EXC, value);
            }

            /**
             * Get the minimum exclusive restriction for setting values to the
             * element identified by path. Will throw an exception if ValueType
             * is not of the type of the bound (usually the Element's data type)
             * @param path
             */
            template <class ValueType>
            const ValueType& getMinExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType>(path, KARABO_SCHEMA_MIN_EXC);
            }

            /**
             * Get the minimum exclusive restriction for setting values to the
             * element identified by path. Will throw an exception if T
             * is not of the type of the bound (usually the Element's data type)
             * or cannot be casted to T.
             * @param path
             */
            template <class T>
            T getMinExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T>(path, KARABO_SCHEMA_MIN_EXC);
            }

            /**
             * Check if a minimum exclusive restriction for setting values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMinExc(const std::string& path) const;

            //******************************************************************
            //    Maximum Exclusive value                                      *
            //******************************************************************

            /**
             * Set the maximum exclusive restriction for setting values to the
             * element identified by path
             * @param path
             * @param value
             */
            template <class ValueType>
            void setMaxExc(const std::string& path, const ValueType& value) {
                m_hash.setAttribute(path, KARABO_SCHEMA_MAX_EXC, value);
            }

            /**
             * Get the maximum exclusive restriction for setting values to the
             * element identified by path. Will throw an exception if ValueType
             * is not of the type of the bound (usually the Element's data type)
             * @param path
             */
            template <class ValueType>
            const ValueType& getMaxExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType>(path, KARABO_SCHEMA_MAX_EXC);
            }

            /**
             * Get the maximum exclusive restriction for setting values to the
             * element identified by path. Will throw an exception if T
             * is not of the type of the bound (usually the Element's data type)
             * or cannot be casted to T.
             * @param path
             */
            template <class T>
            T getMaxExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T>(path, KARABO_SCHEMA_MAX_EXC);
            }

            /**
             * Check if a maximum exclusive restriction for setting values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMaxExc(const std::string& path) const;

            //******************************************************
            //  Specific functions for LEAF node (which is vector):*
            //  Minimum Size of the vector                         *
            //******************************************************

            /**
             * Set the minimum size restriction for setting sequence values to the
             * element identified by path
             * @param path
             * @param value
             */
            void setMinSize(const std::string& path, const unsigned int& value);

            /**
             * Check if a minimum size restriction for setting sequence values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMinSize(const std::string& path) const;

            /**
             * Get the minimum size restriction for setting sequence values to the
             * element identified by path.
             * @param path
             */
            const unsigned int& getMinSize(const std::string& path) const;

            //******************************************************
            //  Specific functions for LEAF node (which is vector):*
            //  Maximum Size of the vector                         *
            //******************************************************

            /**
             * Set the maximum size restriction for setting sequence values to the
             * element identified by path
             * @param path
             * @param value
             */
            void setMaxSize(const std::string& path, const unsigned int& value);

            /**
             * Check if a maximum size restriction for setting sequence values to the
             * element identified by path has been set
             * @param path
             * @return
             */
            bool hasMaxSize(const std::string& path) const;

            /**
             * Get the maximum size restriction for setting sequence values to the
             * element identified by path.
             * @param path
             */
            const unsigned int& getMaxSize(const std::string& path) const;

            //**********************************************
            //               archivePolicy                 *
            //**********************************************

            /**
             * Set the archive policy for the element identified by path
             * @param path
             * @param value
             */
            void setArchivePolicy(const std::string& path, const ArchivePolicy& value);

            /**
             * Check if the element identified by path has an archive policy set
             * @param path
             * @return
             */
            bool hasArchivePolicy(const std::string& path) const;

            /**
             * Get the archive policy for the element identified by path.
             * @param path
             * @return maps to the Schema::ArchivePolicy enum
             */
            const int& getArchivePolicy(const std::string& path) const;

            //******************************************************
            //      min/max for number of nodes in ListElement     *
            //******************************************************

            /**
             * Set the minimum number of nodes needed in a list element
             * @param path
             * @param value
             */
            void setMin(const std::string& path, const int& value);

            /**
             * Check if the element identified by path has a minimum number of Nodes set
             * @param path
             * @return
             */
            bool hasMin(const std::string& path) const;

            /**
             * Get the minimum number of nodes needed for the element identified by path
             * @param path
             * @return
             */
            const int& getMin(const std::string& path) const;

            /**
             * Set the maximum number of nodes needed in a list element
             * @param path
             * @param value
             */
            void setMax(const std::string& path, const int& value);

            /**
             * Check if the element identified by path has a maximum number of Nodes set
             * @param path
             * @return
             */
            bool hasMax(const std::string& path) const;

            /**
             * Set the maximum number of nodes needed in a list element
             * @param path
             * @param value
             */
            const int& getMax(const std::string& path) const;

            // TODO: check if this is needed. There seems to be no implementation
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
            KARABO_DECLSPEC friend std::ostream& operator<<(std::ostream& os, const Schema& schema);

            /**
             * Retrieve a sub-schema of the schema
             * @param subNodePath path of the node element to retrieve sub schema from
             * @param filterTags optional tags to filter sub schema by
             * @return a sub-schema of this schema.
             */
            Schema subSchema(const std::string& subNodePath, const std::string& filterTags = std::string()) const;

            /**
             * Retrieve a sub-schema of those paths that fulfill given rules
             * @param rules assemby rules: - if their state field is emtpy, accept all states
             *                             - similarly, default accessLevel -1 means do not care
             *
             */
            Schema subSchemaByRules(const AssemblyRules& rules) const;

            /**
             * Retrieves a sub-schema with given paths
             *
             * A path to a node means that all further nested paths are kept
             *
             * @param paths in the schema whose information should be copied to the resulting sub schema
             * @return reduced schema
             */
            Schema subSchemaByPaths(const std::set<std::string>& paths) const;

            /**
             * Set the DAQ data type for a given node element.
             *
             * Data types may only be set for node elements
             * @param path at which the node is located
             * @param dataType of this node
             */
            void setDaqDataType(const std::string& path, const DaqDataType& dataType);

            /**
             * Get the DAQ data type for a given node element
             * @param path
             */
            DaqDataType getDaqDataType(const std::string& path) const;

            /**
             * Check if a DAQ data type has been set for a given element.
             * @param path
             */
            bool hasDaqDataType(const std::string& path) const;

            /**
             * Check if given element is a custom node like IMAGEDATA or NDARRAY_ELEMENT.
             * @param path
             */
            bool isCustomNode(const std::string& path) const;

            /**
             * Get the class name for a given custom node element
             * Result is undefined if path does not point to a custom node element (may e.g. throw or not), so better
             * check isCustomNode before.
             * @param path of custom node element
             */
            const std::string& getCustomNodeClass(const std::string& path) const;

            //**********************************************
            //            allowed actions                  *
            //**********************************************

            /**
             * Specify one or more actions that are allowed on the element.
             *
             * If a Karabo device specifies allowed actions, that means that it offers a specific slot
             * interface to operate on this element.
             * Which allowed actions require which interface will be defined elsewhere.
             * @param path to element
             * @param actions vector of strings of allowed actions
             */
            void setAllowedActions(const std::string& path, const std::vector<std::string>& actions);

            /**
             * Check if given element has allowed actions.
             * @param path to element
             */
            bool hasAllowedActions(const std::string& path) const;

            /**
             * Get allowed actions of given element if available.
             * Pre-requisite: hasAllowedActions(path) has to return true.
             * @param path to element
             * @return specified allowed actions
             */
            const std::vector<std::string>& getAllowedActions(const std::string& path) const;

            //**********************************************
            //               daqPolicy                     *
            //**********************************************

            /**
             * Set the DAQ policy for the element identified by path
             * @param path
             * @param value
             */
            void setDAQPolicy(const std::string& path, const DAQPolicy& value);

            /**
             * Check if the element identified by path has an DAQ policy set
             * @param path
             * @return
             */
            bool hasDAQPolicy(const std::string& path) const;

            /**
             * Get the DAQ policy for the element identified by path.
             * @param path
             * @return maps to the Schema::DAQPolicy enum
             */
            DAQPolicy getDAQPolicy(const std::string& path) const;

            /**
             * Set the default DAQ policy to use if not specified per element.
             * Needs to be called at the very beginning of the expected parameter
             * section.
             *
             * @param policy
             */
            void setDefaultDAQPolicy(const DAQPolicy& value);

            /**
             * Get the default DAQ policy to use if not specified per element
             */
            DAQPolicy getDefaultDAQPolicy() const;


           private: // functions
            void addElement(Hash::Node& node);

            void overwriteAttributes(const Hash::Node& node);

            void ensureParameterDescriptionIsComplete(Hash::Node& node) const;

            bool isAllowedInCurrentAccessMode(const Hash::Node& node) const;

            bool isAllowedInCurrentAccessLevel(const Hash::Node& node) const;

            bool isAllowedInCurrentState(const Hash::Node& node) const;

            /// True if node's key points to a subnode whose parent does not
            /// exist or is of inappropriate node type.
            bool isOrphaned(const Hash::Node& node) const;

            void processingLeaf(const std::string& key, std::ostringstream& stream);

            void processingNode(const std::string& key, std::ostringstream& stream);

            void processingChoiceOfNodes(const std::string& key, std::ostringstream& stream);

            void processingListOfNodes(const std::string& key, std::ostringstream& stream);

            void processingStandardAttributes(const std::string& key, std::ostringstream& stream);

            std::string extractKey(const std::string& key);

            bool checkRequiredAccessLevel(const std::string& path, const Schema::AccessLevel& accessLevel) const;

            void r_updateAliasMap(const std::vector<std::string> keys, const std::string oldPath = "");

            void setAllowedStates(const std::string& path, const std::string& value);
        };

        bool similar(const Schema& left, const Schema& right);
    } // namespace util
} // namespace karabo

#endif /* KARABO_UTIL_MASTERCONFIG_HH */
