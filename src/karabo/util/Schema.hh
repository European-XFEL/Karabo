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


#ifndef KARABO_UTIL_MASTERCONFIG_HH
#define	KARABO_UTIL_MASTERCONFIG_HH

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tuple/tuple.hpp>

#include "ConfigConstants.hh"
#include "Hash.hh"
#include "StringTools.hh"
#include "Test.hh"
#include "ToLiteral.hh"

#include "karaboDll.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package util
     */
    namespace util {

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
        class DECLSPEC_UTIL Schema : public Hash {
        public:

            enum OccuranceType {
                EXACTLY_ONCE,
                ONE_OR_MORE,
                ZERO_OR_ONE,
                ZERO_OR_MORE,
                EITHER_OR
            };

            enum ExpertLevelType {
                SIMPLE,
                MEDIUM,
                ADVANCED
            };

            enum AssignmentType {
                OPTIONAL_PARAM,
                MANDATORY_PARAM,
                INTERNAL_PARAM
            };

            /**
             * Schema class.
             */
            Schema() : m_inputOrder(0) {
            }

            Schema(AccessType at, const std::string& currentState) : m_inputOrder(0), m_access(at), m_currentState(currentState) {
            }

            /**
             *  Help function to show all parameters on console
             */
            void help(const std::string& classId = "");

            std::vector<std::string> getAllParameters();

            /**
             * Default output using help function
             * @param key
             * @return 
             */
            DECLSPEC_UTIL friend std::ostream & operator<<(std::ostream& os, const Schema& schema);

            /**
             * This predicate checks that the parameter is a valid key in current <b>Schema</b> container.
             * @param key  A key to be tested
             * @return <b>true</b> or <b>false</b>.
             */
            bool hasKey(const std::string& key) {

                ensureValidCache();

                return m_keys.find(key) != m_keys.end();
            }

            /**
             * This predicate checks that the parameter is an alias of some key in current <b>Schema</b> container
             * @param alias
             * @return <b>true</b> or <b>false</b>.
             */
            template <class T>
            bool hasAlias(const T & alias) {
                ensureValidCache();
                return m_alias2key.has(karabo::util::toString(alias));
            }

            /**
             * This function retrieves a description block of a parameter
             * @param key The parameter's key
             * @return A schema object containing meta-information to the key
             */
            const Schema& getDescriptionByKey(const std::string& key) {

                ensureValidCache();

                std::map<std::string, Schema*>::const_iterator it = m_keys.find(key);
                if (it == m_keys.end()) {
                    throw KARABO_PARAMETER_EXCEPTION("Requested key \"" + key + "\" is not registered within this Schema object");
                }
                return *(it->second);
            }

            /**
             * This function retrieves a description block of a parameter by alias
             * @param alias A valid alias of some key
             * @return A schema object containing meta-information to the key/alias
             */
            template <class T>
            const Schema& getDescriptionByAlias(const T& alias) {

                return getDescriptionByKey(alias2key(alias));

            }

            /**
             * This predicate checks that the parameter <b>key</b> has associated <b>alias</b> defined
             * @param key
             * @return <b>true</b> or <b>false</b>.
             */
            bool keyHasAlias(const std::string& key) {

                ensureValidCache();

                return m_key2alias.has(key);
            }

            /**
             * This predicate checks that the alias associated with key given in parameter is of type of template argument
             * @param key
             * @return <b>true</b> or <b>false</b>.
             */
            template <class T>
            bool aliasIsOfType(const std::string& key) {

                ensureValidCache();

                return m_key2alias.is<T > (key);
            }

            /**
             * This predicate checks that the key given as an argument is of type of template argument
             * @param key
             * @return <b>true</b> or <b>false</b>.
             */
            template <class T>
            bool parameterIsOfType(const std::string& key) {
                return getDescriptionByKey(key).isValueOfType<T > ();
            }

            /**
             * This method returns <b>alias</b> using parameter <b>key</b>
             * @param key
             * @return <i>const</i> reference to the <b>alias</b> of type <i>T</i>
             */
            template <class T>
            const T & key2alias(const std::string & key) {

                ensureValidCache();

                if (!m_key2alias.has(key)) {
                    throw KARABO_PARAMETER_EXCEPTION("No alias is registered for the key \"" + key + "\"");
                }
                return m_key2alias.get<T > (key);
            }

            /**
             * This method returns <b>key</b> as <i>std::string</i> using parameter <b>alias</b>
             * @param alias
             * @return <i>const std::string</i> reference to the <b>key</b>.
             */
            template <class T>
            const std::string & alias2key(const T & alias) {

                ensureValidCache();

                std::string aliasKey = karabo::util::toString(alias);

                if (!m_alias2key.has(aliasKey)) {
                    throw KARABO_PARAMETER_EXCEPTION("No key is registered for this alias");
                }
                return m_alias2key.get<std::string > (aliasKey);
            }

            /**
             * TODO This documentation is deprecated and needs to be updated !!!
             * 
             * This function should be called prior to configure any object using the expectedParameters function.
             * An additional hierarchy is artificially added in order to artificially root the naturally un-rooted
             * object-description and to furthermore keep track about the ordering of parameters.
             * The function will add two keys to the current object: root (STRING) and elements (SCHEMA). 
             * The reference to elements is returned and must be used to fill in the expected parameters of any configurable object.
             *
             * PLEASE NOTE: Typically there is no need to call this function manually, as the factory-framework
             * will do this automatically as needed.
             */
            Schema& initParameterDescription(const std::string& key, AccessType accessMode = INIT | WRITE, const std::string& currentState = "");

            Hash mergeUserInput(const std::vector<Hash>& userConfigurations);

            Hash injectRootedDefaults(const Hash& user = Hash()) const;
            
            Hash injectUnrootedDefaults(const Hash& user = Hash()) const;
            
            /**
             * The validate function validates any Hash against this Schema. Several flags control the detailed behavior of the validation.
             * @param user
             * @param forceRootedConfiguration
             * @param allowAdditionalKeys
             * @param allowMissingKeys
             * @param injectDefaults
             * @return 
             */
            Hash validate(const Hash& user, bool injectDefaults = true, bool allowUnrootedConfiguration = false, bool allowAdditionalKeys = false, bool allowMissingKeys = false);

            AccessType getAccessMode();

            const std::string& getCurrentState();

            /**
             * Add schema descriptions defined externally to current schema definitions
             * @param params user-defined schema container
             * @return current schema 
             */
            Schema& addExternalSchema(const Schema& params);
            

            //**********************************************//
            //            Per Key Functionality             //
            //**********************************************//
            
            std::vector<std::string> getKeys(const std::string& path = "") {
                const Schema* schema;
                if (path.empty()) schema = this;
                else schema = &(getDescriptionByKey(path));
                
                std::vector<std::string> ret;
                if (schema->has("elements")) {
                    const Schema& params = schema->get<Schema > ("elements");
                    for (Schema::const_iterator it = params.begin(); it != params.end(); ++it) {
                        const Schema& param = it->getValue<Schema>();
                        ret.push_back(param.get<std::string > ("key"));
                    }
                } else if (schema->has("complexType")) {
                    const Schema& params = schema->get<Schema > ("complexType");
                    for (Schema::const_iterator it = params.begin(); it != params.end(); ++it) {
                        const Schema& param = it->getValue<Schema>();
                        ret.push_back(param.get<std::string > ("root"));
                    }
                }
                return ret;
            }
            
            bool hasParameters() const {
                return has("elements") || has("complexType");
            }
            
            const Schema& getParameters() const {
                if (has("elements")) return get<Schema > ("elements");
                else if (has("complexType")) return get<Schema > ("complexType");
                else throw KARABO_LOGIC_EXCEPTION("No parameters available");
            }

            bool hasKey() const {
                return has("key");
            }

            const std::string& getKey() const {
                return get<std::string > ("key");
            }
            
            bool hasRoot() const {
                return has("root");
            }
            
            const std::string& getRoot() const {
                return get<std::string>("root");
            }

            bool isCommand() const {
                return (has("elements") && has("displayType") && get<std::string > ("displayType") == "Slot");
            }

            bool isAttribute() const {
                return !isCommand();
            }

            bool isLeaf() const {
                return has("simpleType");
            }

            bool isNode() const {
                return has("elements");
            }

            bool isChoiceOfNodes() const {
                return (has("complexType") && get<OccuranceType > ("occurrence") == EITHER_OR);
            }

            bool isListOfNodes() const {
                return (has("complexType") && get<OccuranceType > ("occurrence") == ZERO_OR_MORE);
            }

            bool isNonEmptyListOfNodes() const {
                return (has("complexType") && get<OccuranceType > ("occurrence") == ONE_OR_MORE);
            }

            bool isAccessInitOnly() const {
                return getAccess() == INIT;
            }

            bool isAccessReadOnly() const {
                return getAccess() == READ;
            }

            bool isAccessReconfigurable() const {
                return getAccess() == WRITE;
            }

            bool isAssignmentMandatory() const {
                return getAssignment() == MANDATORY_PARAM;
            }

            bool isAssignmentOptional() const {
                return getAssignment() == OPTIONAL_PARAM;
            }

            bool isAssignmentInternal() const {
                return getAssignment() == INTERNAL_PARAM;
            }

            template <class T>
            bool isValueOfType() const {
                if (has("simpleType")) {
                    return Types::from<T > () == get<Types::ReferenceType > ("simpleType");
                } else {
                    return Types::from<T > () == Types::HASH;
                }
            }

            Types::ReferenceType getValueType() const {
                if (has("simpleType")) {
                    return get<Types::ReferenceType > ("simpleType");
                } else {
                    return Types::HASH;
                }
            }

            std::string getValueTypeAsString() const {
                if (has("simpleType")) {
                    return Types::to<ToLiteral>(get<Types::ReferenceType > ("simpleType"));
                } else {
                    return Types::to<ToLiteral>(Types::HASH);
                }
            }

            bool hasDisplayedName() const {
                return has("displayedName");
            }

            std::string getDisplayedName() const {
                return get<std::string > ("displayedName");
            }

            bool hasDisplayType() const {
                return has("displayType");
            }

            std::string getDisplayType() const {
                return get<std::string > ("displayType");
            }

            bool hasDescription() const {
                return has("description");
            }

            const std::string& getDescription() const {
                return get<std::string > ("description");
            }

            bool hasAssignment() const {
                return has("assignment");
            }

            const AssignmentType& getAssignment() const {
                return get<AssignmentType > ("assignment");
            }

            bool hasValueOptions() const {
                return has("options");
            }

            const std::vector<std::string>& getValueOptions() const {
                return get<std::vector<std::string> >("options");
            }

            bool hasAllowedStates() const {
                return has("allowedStates");
            }

            const std::vector<std::string>& getAllowedStates() const {
                return get<std::vector<std::string> >("allowedStates");
            }
            
            bool hasAccess() const {
                return has("access");
            }

            const AccessType& getAccess() const {
                return get<AccessType > ("access");
            }

            bool hasDefaultValue() const {
                return has("default");
            }

            template <class T>
            const T& getDefaultValue() const {
                return get<T > ("default");
            }

            bool hasAlias() const {
                return has("alias");
            }

            template <class T>
            const T& getAlias() const {
                return get<T > ("alias");
            }
            
            bool hasUnitName() const {
                return has("unitName");
            }
            
            const std::string& getUnitName() const {
                return get<std::string>("unitName");
            }
            
            bool hasUnitSymbol() const {
                return has("unitSymbol");
            }
            
            const std::string& getUnitSymbol() const {
                return get<std::string>("unitSymbol");
            }
          
          

        protected:

            void r_toStream(std::ostream& os, const Hash& config, int depth) const;


        private: // functions

            static void processingDescription(const Schema& desc, std::ostringstream& stream);

            static void processingExpectParams(const Schema& expected, const std::string& classId, std::ostringstream& stream);

            static void processingComplexType(const Schema& elementsComplexType, std::ostringstream& stream);

            static void r_processingPathToElem(const Schema& expected, std::vector<std::string>& tokens, std::ostringstream& stream);

            static void showSingleElement(const Schema& desc, std::ostringstream& stream);

            void setAccessMode(AccessType at);

            void setCurrentState(const std::string& currentState);

            void key(const std::string & parameterKey);

            void displayedName(const std::string& name);

            void assignment(AssignmentType ass);

            void options(const std::string& options, const std::string& sep = " ,;");

            void options(const std::vector<std::string>& options);

            void allowedStates(const std::string& states, const std::string& sep = " ,;");

            void description(const std::string& description);

            void expertLevel(ExpertLevelType level);

            void access(AccessType at);

            void simpleType(const Types::ReferenceType type);

            void choiceType(const Schema & elements);

            void listType(const Schema & elements);

            void nonEmptyListType(const Schema & elements);

            void singleElementType(const Schema&);

            Schema & addElement(Schema & item);

            template <class T>
            void defaultValue(const T & defaultValue) {
                set("default", defaultValue);
            }

            template <class T>
            void overwriteDefault(const T& defaultValue) {
                set("overwriteDefault", defaultValue);
            }
            
            template <class T>
            void overwriteAlias(const T& defaultAlias) {
                set("overwriteAlias", defaultAlias);
            }

            template <class T>
            void minInc(const T & value) {
                set("minInc", value);
            }

            template <class T>
            void maxInc(const T & value) {
                set("maxInc", value);
            }

            template <class T>
            void minExc(const T & value) {
                set("minExc", value);
            }

            template <class T>
            void maxExc(const T & value) {
                set("maxExc", value);
            }

            //For VECTOR-datatypes: VECTOR_ INT8,16,32,64, UINT8,16,32,64, DOUBLE, FLOAT
            //setting minSize (min number of elements in array)  ...

            void minSize(const int& value) {
                set("minSize", value);
            }
            //... and maxSize (max number of elements in array)

            void maxSize(const int& value) {
                set("maxSize", value);
            }

            void unitName(const std::string & unitName) {
                set("unitName", unitName);
            }

            void unitSymbol(const std::string & unitSymbol) {
                set("unitSymbol", unitSymbol);
            }

            template <class T>
            void alias(const T & alias) {
                set("alias", alias);
            }

            void displayType(const std::string& displayType) {
                set("displayType", displayType);
            }



        private: // members

            
            // Validation behavior
            bool m_allowAdditionalKeys;
            bool m_allowMissingKeys;
            bool m_injectDefaults;
            bool m_allowUnrootedConfiguration;
            
            
            unsigned int m_inputOrder;
            AccessType m_access;
            std::string m_currentState;
            Hash m_key2alias;
            Hash m_alias2key;
            std::map<std::string, Schema*> m_keys;


        private: // functions

            void ensureValidCache();

        

            void r_generateAliasMaps(Schema& config, std::string path);

            void complexType(const Schema & type);

            void occurance(OccuranceType occ);

            void overwriteIfDuplicated(const std::string& key);

            // TODO Implement support for SingleElement type Schemas
            void overwriteNestedDefaults(Schema& item);

            void reportWrongComplexTypeFormat(const std::string & scope) const;

            void applyDefault(const std::string& key, const Schema& desc, Hash& uParam, Hash & wParam) const;

            void reportNotNeededInformation(const std::string& scope, const std::set<std::string>& sufficientParameters) const;

            
            
            void r_injectDefaults(const Schema& schema, Hash& user) const;
            
            void r_validate(const Schema& master, Hash& user, Hash& working, std::ostringstream& report, std::string scope = "") const;
            
            void assertSimpleType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, std::string & scope) const;

            void assertComplexType(const Schema& desc, Hash& uParam, Hash& wParam, std::ostringstream& report, std::string & scope) const;

            template <class T>
            void checkOptions(const std::string& key, const T& value, const Schema& desc, std::ostringstream & report) const {
                const std::vector<std::string>& options = desc.get<std::vector<std::string> >("options");
                for (size_t i = 0; i < options.size(); ++i) {
                    T option = boost::lexical_cast<T > (options[i]);
                    if (option == value) return;
                }
                report << "Value " << value << " for parameter \"" << key << "\" is not one of the valid options: " << karabo::util::toString(options) << std::endl;
            }

            template <class T>
            void checkMinInc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
                if (iValue < eValue) {
                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of lower bound: " << eValue << std::endl;
                }
            }

            template <class T>
            void checkMinExc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
                if (iValue <= eValue) {
                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of lower bound: " << eValue << std::endl;
                }
            }

            template <class T>
            void checkMaxInc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
                if (iValue > eValue) {
                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of upper bound: " << eValue << std::endl;
                }
            }

            template <class T>
            void checkMaxExc(const std::string& key, const T& iValue, const T& eValue, std::ostringstream & report) const {
                if (iValue >= eValue) {
                    report << "Value " << iValue << " for parameter \"" << key << "\" is out of upper bound: " << eValue << std::endl;
                }
            }

            void checkSizeOfVector(const Schema& desc, const std::string& scope, const std::string& key, unsigned int iValue, std::ostringstream & report) const;

            template <class T>
            void checkRangeOfVectorElements(const Schema& desc, const std::string& scope, const std::string& key, unsigned int iValue, const std::vector<T>& iValueVect, std::ostringstream & report) const {
                if (desc.has("minInc")) {
                    double eValue = desc.getAs<double>("minInc");
                    for (size_t i = 0; i < iValue; i++) {
                        if (iValueVect[i] < eValue) {
                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of min Inclusive boundary: " << eValue << std::endl;
                        }
                    }
                } else if (desc.has("minExc")) {
                    double eValue = desc.getAs<double>("minExc");
                    for (size_t i = 0; i < iValue; i++) {
                        if (iValueVect[i] <= eValue) {
                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of min Exclusive boundary: " << eValue << std::endl;
                        }
                    }
                }

                if (desc.has("maxInc")) {
                    double eValue = desc.getAs<double>("maxInc");
                    for (size_t i = 0; i < iValue; i++) {
                        if (iValueVect[i] > eValue) {
                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of max Inclusive boundary: " << eValue << std::endl;
                        }
                    }
                } else if (desc.has("maxExc")) {
                    double eValue = desc.getAs<double>("maxExc");
                    for (size_t i = 0; i < iValue; i++) {
                        if (iValueVect[i] >= eValue) {
                            report << "Parameter \"" << scope << "." << key << "[" << i << "]" << "\" = " << iValueVect[i] << " is out of max Exclusive boundary: " << eValue << std::endl;
                        }
                    }
                }
            }

            void assertOccuranceEitherOr(const Schema& mComplex, Hash& uComplex, Hash& wParam, std::ostringstream& report, const std::string & scope) const;

            void assertOccuranceZeroOrMore(const Schema& mComplex, std::vector<Hash>& uComplex, std::vector<Hash>& wComplex, std::ostringstream& report, const std::string & scope) const;

            void assertOccuranceOneOrMore(const Schema& mComplex, std::vector<Hash>& uComplex, std::vector<Hash>& wComplex, std::ostringstream& report, const std::string & scope) const;

            template< class T> friend class SimpleElement;
            friend class ComplexElement;
            friend class OverwriteElement;
            template< typename T, template <typename, typename> class CONT> friend class VectorElement;
            template< class T> friend class CHOICE_ELEMENT;
            template< class T> friend class LIST_ELEMENT;
            template< class T> friend class NON_EMPTY_LIST_ELEMENT;
            template< class BASE, class DERIVED> friend class SINGLE_ELEMENT;
            template< class Element, class T> friend class GenericElement;
            template< class Element, class T> friend class DefaultValue;
        };

    } // namespace util
} // namespace karabo

#endif	/* KARABO_UTIL_MASTERCONFIG_HH */
