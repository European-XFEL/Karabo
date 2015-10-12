/* 
 * File:   TableElement.hh
 * Author: haufs
 *
 * Created on August 7, 2015, 9:01 AM
 */

#ifndef KARABO_UTIL_TABLEELEMENT_HH
#define	KARABO_UTIL_TABLEELEMENT_HH


#include "GenericElement.hh"
#include "LeafElement.hh"
#include "Configurator.hh"
#include "Validator.hh"
#include <karabo/log/Logger.hh>
#include <vector>
#include <boost/any.hpp>
#include <boost/cast.hpp>


namespace karabo {
    namespace util {
        
        
        /**
         * The DefaultValue class defines a default value for element.
         */
        
        template<typename Element>
        class TableDefaultValue {

            Element* m_genericElement;
            
            
        public:

            TableDefaultValue() : m_genericElement(0) {
            }

            void setElement(Element* el) {
                m_genericElement = el;
            }

            /**
             * The <b>defaultValue</b> method serves for setting up the default value to be used when User
             * configuration does not specify another value.
             * @param val  Default value
             * @return reference to the Element for proper methods chaining
             */
            Element& defaultValue(const std::vector<Hash>& defaultValue) { 
                if(m_genericElement->m_nodeSchema.empty()){
                    throw KARABO_PARAMETER_EXCEPTION("Need to set a node schema first for defaults to be set");
                }
                
                std::vector<Hash> validated;
                
                Validator validator;
                Validator::ValidationRules rules;
                rules.allowAdditionalKeys = false;
                rules.allowMissingKeys = false;
                rules.allowUnrootedConfiguration = true;
                validator.setValidationRules(rules);
                
                //validated.push_back(m_genericElement->m_nodeSchema.getParameterHash());
                for(std::vector<Hash>::const_iterator it = defaultValue.begin(); it != defaultValue.end(); ++it){
                    Hash validatedHash;
                    std::pair<bool, std::string> validationResult = validator.validate(m_genericElement->m_nodeSchema, *it, validatedHash);
                    if(!validationResult.first){
                        throw KARABO_PARAMETER_EXCEPTION("Node schema didn't validate against preset node schema");
                    }
                    validated.push_back(validatedHash);
                }
                
                
                m_genericElement->getNode().setAttribute(KARABO_SCHEMA_DEFAULT_VALUE, validated);
                //for(std::vector<Hash>::const_iterator it = defaultValue.begin(); it != defaultValue.end(); ++it){
                //    m_genericElement->addRow(*it);
                //}
                return *m_genericElement;
            }

            

            /**
             * The <b>noDefaultValue</b> serves for setting up the element that does not have a default value.
             * @return reference to the Element for proper methods chaining
             */
            Element& noDefaultValue() {
                return *m_genericElement;
            }     
        };
        
        class TableElement : public GenericElement<TableElement> {
            
            friend class TableDefaultValue<TableElement>;
            
            Schema m_nodeSchema;
            TableDefaultValue<TableElement> m_defaultValue;
            
        public:

            TableElement(Schema& expected) : GenericElement<TableElement>(expected) {
                
                m_defaultValue.setElement(this);
                
            }

            TableElement& minSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MIN_SIZE, value);
                return *this;
            }

            TableElement& maxSize(const unsigned int& value) {
                this->m_node->setAttribute(KARABO_SCHEMA_MAX_SIZE, value);
                return *this;
            }

            /*virtual ReadOnlySpecific<TableElement, std::vector<Hash> >& readOnly() {
                ReadOnlySpecific<TableElement,  std::vector<Hash> >& _readOnlySpecific = TableElement::readOnly();
                this->m_node->setAttribute(KARABO_SCHEMA_DEFAULT_VALUE,  std::vector<Hash>());
                return _readOnlySpecific;
            }*/
            
            /**
             * The <b>allowedStates</b> method serves for setting up allowed states for the element
             * @param states A string describing list of possible states.
             * @param sep A separator symbol used for parsing previous argument for list of states
             * @return reference to the Element (to allow method's chaining)
             */
            TableElement& allowedStates(const std::string& states, const std::string& sep = " ,;") {
                this->m_node->setAttribute(KARABO_SCHEMA_ALLOWED_STATES, karabo::util::fromString<std::string, std::vector>(states, sep));
                return *this;
            }
            

            /**
             * The <b>assignmentMandatory</b> method serves for setting up a mode that requires the value
             * of the element always being specified. No default value is possible.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& assignmentMandatory() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::MANDATORY_PARAM);
                return *this;
            }

            /**
             * The <b>assignmentOptional</b> method serves for setting up a mode that allows the value of
             * element be optional, so it can be omitted in configuration. Default value is injected if defined.
             * If you chain functions for definition of expected parameters the next
             * function may be only defaultValue or noDefaultValue.
             * When the default value is not specified (noDefaultValue) you must always check
             * if the parameter has a value set in delivered User configuration.
             * @return reference to DefaultValue object allowing proper <b>defaultValue</b> method chaining.
             * 
             * <b>Example:</b>
             * @code
             * SOME_ELEMENT(expected)
             *         ...
             *         .assignmentOptional().defaultValue("client")
             *         ...
             *         .commit();
             * @endcode
             */
            virtual TableDefaultValue<TableElement>& assignmentOptional() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::OPTIONAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>assignmentInternal</b> method serves for setting up the element to be internal. In the code
             * it behaves like optional parameter but it is not exposed to the user. It is omitted when the schema
             * is serialized to XSD. The value of this parameter should be defined programmatically. Conceptually,
             * internal parameter with internal flag can be treated as an argument to the constructor.
             * @return reference to DefaultValue (to allow method's chaining)
             */
            virtual TableDefaultValue<TableElement>& assignmentInternal() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ASSIGNMENT, Schema::INTERNAL_PARAM);
                return m_defaultValue;
            }

            /**
             * The <b>init</b> method serves for setting up an access type property that allows the element
             * to be included in initial schema.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& init() {
                this->m_node->setValue(std::vector<Hash>());
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, INIT);
                return *this;
            }

            /**
             * The <b>reconfigurable</b> method serves for setting up an access type property that allows the element
             * to be included in initial, reconfiguration and monitoring schemas.
             * @return reference to the Element (to allow method's chaining)
             */
            virtual TableElement& reconfigurable() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, WRITE);
                return *this;
            }
            
            
            TableElement& setNodeSchema(const Schema& schema) {
                
                m_nodeSchema = schema;
                
                //this->addRow(); //set first element to containt parameter Hash
                
                return *this;
            }
            
            
            
            /*TableElement& addRow(const Hash& nodeHash = Hash()) {
                if(m_nodeSchema.empty()){
                    throw KARABO_PARAMETER_EXCEPTION("No node schema has been set. Please set one before adding schema-less nodes");
                } else {
                    
                    return this->addRow(m_nodeSchema, nodeHash);
                }
                 
            }
            
            
            
            TableElement& addRow(const Schema& schema,  const Hash& nodeHash = Hash()) {
                if(!m_nodeSchema.empty()){
                    Hash validatedHash;
                    Validator validator;
                    Validator::ValidationRules rules;
                    rules.allowAdditionalKeys = true;
                    rules.allowMissingKeys = true;
                    rules.allowUnrootedConfiguration = true;
                    validator.setValidationRules(rules);
                    std::pair<bool, std::string> validationResult = validator.validate(m_nodeSchema, schema.getParameterHash(), validatedHash);
                    if(!validationResult.first){
                        throw KARABO_PARAMETER_EXCEPTION("Node schema didn't validate against preset node schema");
                    }
                }
                // Create an empty Hash as value of this choice node if not there yet
              
                if (this->m_node->getType() != Types::VECTOR_HASH) this->m_node->setValue(std::vector<Hash>());
                // Retrieve reference for filling
               std::vector<Hash>& listOfNodes = this->m_node->template getValue<std::vector<Hash> >();
                
               
                
                
                Hash validatedHash;
                if(nodeHash.empty()) {
                    validatedHash =  schema.getParameterHash();
                } else {
                   
                
                    
                    Validator validator;
                    std::pair<bool, std::string> validationResult = validator.validate(schema, nodeHash, validatedHash);
                    if(!validationResult.first){
                        throw KARABO_PARAMETER_EXCEPTION("Node Hash didn't validate against preset node schema");
                    }
                    
                   
                }
                
              
                
                listOfNodes.push_back(validatedHash);
                
                return *this;
            }
            */
            
        protected:

            void beforeAddition() {

                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "Table");
                this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, "VECTOR_HASH");
                //this->m_node->setAttribute<int>(KARABO_SCHEMA_ACCESS_MODE, 4);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_ARCHIVE_POLICY, Schema::NO_ARCHIVING); //currently doesn't work
                
                //this->m_node->template setAttribute<int>(KARABO_SCHEMA_ROW_SCHEMA, true);
                this->m_node->setAttribute(KARABO_SCHEMA_ROW_SCHEMA, m_nodeSchema);
                //if (!m_nodeSchema.empty()) this->m_node->template setAttribute<int>(KARABO_SCHEMA_ROW_SCHEMA, 1); 

                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default
                
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) { 
                    
                    //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||                         //init element 
                         this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT ||   //init element 
                         this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE ) { //reconfigurable element
                        
                        this->userAccess();
                          
                    } else { //else set default value of requiredAccessLevel to OBSERVER 
                       this->observerAccess();
                    }  
                }
            }
        };

        typedef util::TableElement TABLE_ELEMENT;
    }
}


#endif	/* TABLEELEMENT_HH */

