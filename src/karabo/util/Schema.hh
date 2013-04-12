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
    
    namespace io {
        class SchemaXmlSerializer;
    }

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
            #define KARABO_SCHEMA_ADVANCED "advanced"
            #define KARABO_SCHEMA_ALLOWED_STATES "allowedStates"
            #define KARABO_SCHEMA_ALLOWED_ROLES "allowedRoles"
            #define KARABO_SCHEMA_ASSIGNMENT "assignment"
            #define KARABO_SCHEMA_TAGS "tags"

            #define KARABO_SCHEMA_OPTIONS "options"
            #define KARABO_SCHEMA_EXPERT_LEVEL "expertLevel"

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

            #define KARABO_SCHEMA_MIN "min"
            #define KARABO_SCHEMA_MAX "max"

            #define KARABO_SCHEMA_OVERWRITE "overwrite"


            template <class T> friend class GenericElement;
            template< class T> friend class SimpleElement;
            friend class OverwriteElement;
            template< typename T, template <typename, typename> class CONT> friend class VectorElement;
            friend class ChoiceElement;
            friend class ListElement;
            friend class NodeElement;
            friend class karabo::io::SchemaXmlSerializer;
            
            // Container
            Hash m_hash;

            // Filter
            AccessType m_currentAccessMode;
            std::string m_currentState;
            std::string m_currentAccessRole;

            // Root name
            std::string m_rootName;

            // Indices
            std::map<std::string, std::string> m_aliasToKey;

        public:

            KARABO_CLASSINFO(Schema, "Schema", "1.0")

            struct AssemblyRules {

                AccessType m_accessMode;
                std::string m_state;
                std::string m_accessRole;

                AssemblyRules(const AccessType& accessMode = INIT | WRITE, const std::string& state = "", const std::string& accessRole = "") :
                m_accessMode(accessMode), m_state(state), m_accessRole(accessRole) {
                }
            };

            enum ExpertLevelType {

                SIMPLE,
                MEDIUM,
                ADVANCED
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

        public:

            // Constructs empty anonymous schema
            Schema();

            // Constructs empty schema for given classId
            Schema(const std::string& classId, const Schema::AssemblyRules& rules = Schema::AssemblyRules());

            void setAssemblyRules(const Schema::AssemblyRules& rules);

            Schema::AssemblyRules getAssemblyRules() const;

            const std::string& getRootName() const;
            
            const karabo::util::Hash& getParameterHash() const;
            
            std::vector<std::string> getParameters(const std::string& path = "") const;
            
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
            //                  ExpertLevel                *
            //**********************************************

            void setExpertLevel(const std::string& path, const ExpertLevelType& value);

            bool hasExpertLevel(const std::string& path) const;

            bool isExpertLevelAdvanced(const std::string& path) const;

            bool isExpertLevelMedium(const std::string& path) const;

            bool isExpertLevelSimple(const std::string& path) const;

            const int getExpertLevel(const std::string& path) const;

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

            void setUnit(const std::string& path, const Units::Unit& value);

            bool hasUnit(const std::string& path) const;

            const int getUnit(const std::string& path) const;

            const std::string& getUnitName(const std::string& path) const;

            const std::string& getUnitSymbol(const std::string& path) const;

            //**********************************************
            //                  UnitMetricPrefix           *
            //**********************************************

            void setMetricPrefix(const std::string& path, const Units::MetricPrefix& value);

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
             *  Help function to show all parameters on console
             */
            void help(const std::string& classId = "");

            /**
             * Default output using help function
             * @param key
             * @return 
             */
            KARABO_DECLSPEC friend std::ostream & operator<<(std::ostream& os, const Schema& schema);


            //            Hash mergeUserInput(const std::vector<Hash>& userConfigurations);
            //
            //            Hash injectRootedDefaults(const Hash& user = Hash()) const;
            //
            //            Hash injectUnrootedDefaults(const Hash& user = Hash()) const;

            //            /**
            //             * The validate function validates any Hash against this Schema. Several flags control the detailed behavior of the validation.
            //             * @param user
            //             * @param forceRootedConfiguration
            //             * @param allowAdditionalKeys
            //             * @param allowMissingKeys
            //             * @param injectDefaults
            //             * @return 
            //             */
            //            Hash validate(const Hash& user, bool injectDefaults = true, bool allowUnrootedConfiguration = false, bool allowAdditionalKeys = false, bool allowMissingKeys = false);
            //
            //            /**
            //             * Add schema descriptions defined externally to current schema definitions
            //             * @param params user-defined schema container
            //             * @return current schema 
            //             */
            //            Schema& addExternalSchema(const Schema& params);


        private: // functions
            
            void setParameterHash(const karabo::util::Hash& parameterDescription);
            
            void setRootName(const std::string& rootName);

            karabo::util::Hash& getParameterHash();

            void addElement(Hash::Node& node);

            void overwriteAttributes(const Hash::Node& node);

            void ensureParameterDescriptionIsComplete(Hash::Node& node) const;

            bool isAllowedInCurrentAccessMode(const Hash::Node& node) const;

            bool isAllowedInCurrentAccessRole(const Hash::Node& node) const;

            bool isAllowedInCurrentState(const Hash::Node& node) const;

            void processingLeaf(const std::string& key, ostringstream& stream);

            void processingNode(const std::string& key, ostringstream& stream);

            void processingChoiceOfNodes(const std::string& key, ostringstream& stream);

            void processingListOfNodes(const std::string& key, ostringstream& stream);

            void processingStandardAttributes(const std::string& key, ostringstream & stream);

            std::string extractKey(const std::string& key);

            //private: // functions

            //            static void processingDescription(const Schema& desc, std::ostringstream& stream);
            //
            //            static void processingExpectParams(const Schema& expected, const std::string& classId, std::ostringstream& stream);
            //
            //            static void processingComplexType(const Schema& elementsComplexType, std::ostringstream& stream);
            //
            //            static void r_processingPathToElem(const Schema& expected, std::vector<std::string>& tokens, std::ostringstream& stream);
            //
            //            static void showSingleElement(const Schema& desc, std::ostringstream& stream);
            //
            //            Schema::Node& addElement(Schema::Node& element);
            //
            //
            //            void setNodeTypeLeaf(const std::string& path);
            //            
            //            void setNodeTypeRoot(const std::string& path);
            //            
            //            void setNodeTypeChoice(const std::string& path);
            //            
            //            void setNodeTypeList(const std::string& path);
            //            
            //            void setNodeTypeNonEmptyList(const std::string& path);
            //            
            //            void setNodeTypeSequence(const std::string& path);
            //            
            //
            //            void ensureValidCache();
            //
            //
            //
            //            void r_generateAliasMaps(Schema& config, std::string path);
            //
            //            void complexType(const Schema & type);
            //
            //            void occurance(OccuranceType occ);
            //
            //            void overwriteIfDuplicated(const std::string& key);
            //
            //            // TODO Implement support for SingleElement type Schemas
            //            void overwriteNestedDefaults(Schema& item);
            //
            //            void reportWrongComplexTypeFormat(const std::string & scope) const;
            //
            //            void applyDefault(const std::string& key, const Schema& desc, Hash& uParam, Hash & wParam) const;
            //
            //            void reportNotNeededInformation(const std::string& scope, const std::set<std::string>& sufficientParameters) const;
            //
            //
            //
            //            void r_injectDefaults(const Schema& schema, Hash& user) const;
            //
            //            void r_validate(const Schema& master, Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            //
            //            void assertSimpleType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, std::string & scope) const;
            //
            //            void assertComplexType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, std::string & scope) const;
            //
            //            template <class T>
            //            void checkOptions(const std::string& key, const T& value, const Schema& desc, std::ostringstream & report) const {
            //                const std::vector<std::string>& options = desc.get<std::vector<std::string> >("options");
            //                for (size_t i = 0; i < options.size(); ++i) {
            //                    T option = boost::lexical_cast<T > (options[i]);
            //                    if (option == value) return;
            //                }
            //                report << "Value " << value << " for parameter \"" << key << "\" is not one of the valid options: " << karabo::util::toString(options) << std::endl;
            //            }
            //
            //            template <class T>
            //            void checkMinInc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
            //                if (iValue < eValue) {
            //                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of lower bound: " << eValue << std::endl;
            //                }
            //            }
            //
            //            template <class T>
            //            void checkMinExc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
            //                if (iValue <= eValue) {
            //                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of lower bound: " << eValue << std::endl;
            //                }
            //            }
            //
            //            template <class T>
            //            void checkMaxInc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
            //                if (iValue > eValue) {
            //                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of upper bound: " << eValue << std::endl;
            //                }
            //            }
            //
            //            template <class T>
            //            void checkMaxExc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
            //                if (iValue >= eValue) {
            //                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of upper bound: " << eValue << std::endl;
            //                }
            //            }
            //
            //            void checkSizeOfVector(const Schema& desc, const std::string& scope, const std::string& key, unsigned int iValue, std::ostringstream & report) const;
            //
            //            template <class T>
            //            void checkRangeOfVectorElements(const Schema& desc, const std::string& scope, const std::string& key, unsigned int iValue, const std::vector<T>& iValueVect, std::ostringstream & report) const {
            //                if (desc.has("minInc")) {
            //                    double eValue = desc.getAs<double>("minInc");
            //                    for (size_t i = 0; i < iValue; i++) {
            //                        if (iValueVect[i] < eValue) {
            //                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of min Inclusive boundary: " << eValue << std::endl;
            //                        }
            //                    }
            //                } else if (desc.has("minExc")) {
            //                    double eValue = desc.getAs<double>("minExc");
            //                    for (size_t i = 0; i < iValue; i++) {
            //                        if (iValueVect[i] <= eValue) {
            //                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of min Exclusive boundary: " << eValue << std::endl;
            //                        }
            //                    }
            //                }
            //
            //                if (desc.has("maxInc")) {
            //                    double eValue = desc.getAs<double>("maxInc");
            //                    for (size_t i = 0; i < iValue; i++) {
            //                        if (iValueVect[i] > eValue) {
            //                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of max Inclusive boundary: " << eValue << std::endl;
            //                        }
            //                    }
            //                } else if (desc.has("maxExc")) {
            //                    double eValue = desc.getAs<double>("maxExc");
            //                    for (size_t i = 0; i < iValue; i++) {
            //                        if (iValueVect[i] >= eValue) {
            //                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of max Exclusive boundary: " << eValue << std::endl;
            //                        }
            //                    }
            //                }
            //            }
            //
            //            void assertOccuranceEitherOr(const Schema& mComplex, Hash& uComplex, Hash& wParam, std::ostringstream& report, const std::string & scope) const;
            //
            //            void assertOccuranceZeroOrMore(const Schema& mComplex, std::vector<Hash>& uComplex, std::vector<Hash>& wComplex, std::ostringstream& report, const std::string & scope) const;
            //
            //            void assertOccuranceOneOrMore(const Schema& mComplex, std::vector<Hash>& uComplex, std::vector<Hash>& wComplex, std::ostringstream& report, const std::string & scope) const;
            //

        };

    } // namespace util
} // namespace karabo

#endif	/* KARABO_UTIL_MASTERCONFIG_HH */
