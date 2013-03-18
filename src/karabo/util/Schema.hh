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

            template <class T> friend class GenericElement;
            template< class T> friend class SimpleElement;
            friend class OverwriteElement;
            template< typename T, template <typename, typename> class CONT> friend class VectorElement;
            friend class ChoiceElement;
            friend class ListElement;
            friend class NodeElement;

            // Container
            Hash m_hash;

            // Filter
            AccessType m_currentAccessMode;
            std::string m_currentState;
            std::string m_currentAccessRole;

            // Root name
            std::string m_rootName;

            // Indices
            std::map<std::string, std::string> m_keyToAlias;
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

            template <class AliasType>
            boost::optional<const std::string&> aliasToKey(const AliasType& alias) const;

            template <class AliasType>
            boost::optional<AliasType> keyToAlias(const std::string& key) const;

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
            const string& getValueType(const std::string& path) const;


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
                m_hash.setAttribute(path, "defaultValue", value);
            }

            bool hasDefaultValue(const std::string&) const;

            template <class ValueType>
            const ValueType& getDefaultValue(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "defaultValue");
            }

            template <class T>
            T getDefaultValueAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "defaultValue");
            }

            //**********************************************
            //                  Alias                      *
            //**********************************************

            template <class AliasType>
            void setAlias(const std::string& path, const AliasType& value) {
                m_hash.setAttribute<AliasType > (path, "alias", value);
            }

            bool hasAlias(const std::string& path) const;

            template <class AliasType>
            const AliasType& getAlias(const std::string& path) const {
                return m_hash.getAttribute < AliasType > (path, "alias");
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
                m_hash.setAttribute(path, "minInc", value);
            }

            template <class ValueType>
            const ValueType& getMinInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "minInc");
            }

            template <class T>
            T getMinIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "minInc");
            }

            bool hasMinInc(const std::string& path) const;

            //******************************************************************
            //    Maximum Inclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMaxInc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "maxInc", value);
            }

            template <class ValueType>
            const ValueType& getMaxInc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "maxInc");
            }

            template <class T>
            T getMaxIncAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "maxInc");
            }

            bool hasMaxInc(const std::string& path) const;

            //******************************************************************
            //    Minimum Exclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMinExc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "minExc", value);
            }

            template <class ValueType>
            const ValueType& getMinExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "minExc");
            }

            template <class T>
            T getMinExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "minExc");
            }

            bool hasMinExc(const std::string& path) const;

            //******************************************************************
            //    Maximum Exclusive value                                      *
            //******************************************************************

            template <class ValueType>
            void setMaxExc(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "maxExc", value);
            }

            template <class ValueType>
            const ValueType& getMaxExc(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "maxExc");
            }

            template <class T>
            T getMaxExcAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "maxExc");
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
               m_hash.setAttribute(path, "warnLow", value);
            }
            
            template <class ValueType>
            const ValueType& getWarnLow(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "warnLow");
            }

            template <class T>
            T getWarnLowAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "warnLow");
            }

            bool hasWarnLow(const std::string& path) const;
            
            
            //******************************************************
            //                   WarnHigh                         *  
            //******************************************************
            template <class ValueType>
            void setWarnHigh(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "warnHigh", value);
            }
            
            template <class ValueType>
            const ValueType& getWarnHigh(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "warnHigh");
            }

            template <class T>
            T getWarnHighAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "warnHigh");
            }

            bool hasWarnHigh(const std::string& path) const;
            
            //******************************************************
            //                   AlarmLow                          *  
            //******************************************************
            
            template <class ValueType>
            void setAlarmLow(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "alarmLow", value);
            }
            
            template <class ValueType>
            const ValueType& getAlarmLow(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "alarmLow");
            }

            template <class T>
            T getAlarmLowAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "alarmLow");
            }

            bool hasAlarmLow(const std::string& path) const;
            
            //******************************************************
            //                   AlarmHigh                          *  
            //******************************************************
            
            template <class ValueType>
            void setAlarmHigh(const std::string& path, const ValueType & value) {
                m_hash.setAttribute(path, "alarmHigh", value);
            }

            template <class ValueType>
            const ValueType& getAlarmHigh(const std::string& path) const {
                return m_hash.getAttribute<ValueType > (path, "alarmHigh");
            }

            template <class T>
            T getAlarmHighAs(const std::string& path) const {
                return m_hash.getAttributeAs<T > (path, "alarmHigh");
            }

            bool hasAlarmHigh(const std::string& path) const;
            
            //******************************************************
            //      min/max for number of nodes in ListElement     *                     *  
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
