/*
 * $Id$
 *
 * File:   PathElement.hh
 * Author: <burkhard.heisen@xfel.eu>
 *
 * Created on February 6, 2013, 11:14 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_UTIL_PATH_ELEMENT_HH
#define	KARABO_UTIL_PATH_ELEMENT_HH

#include "LeafElement.hh"

namespace karabo {
    namespace util {

        /**
         * The PathElement represents a leaf and can be of any (supported) type
         */
        class PathElement : public LeafElement<PathElement, std::string > {

        public:

            PathElement(Schema& expected) : LeafElement<PathElement, std::string >(expected) {
            }

            /**
             * The <b>options</b> method specifies values allowed for the parameter.
             * @param opts A string with space separated values. The values are casted to the proper type.
             * @param sep  A separator symbols. Default values are " ,;"
             * @return reference to the PathElement
             */
            PathElement& options(const std::string& opts, const std::string& sep = " ,;") {
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            /**
             * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the vector.
             * This function can be used when space cannot be used as a separator.
             * @param opts vector of strings. The values are casted to the proper type.
             * @return reference to the PathElement
             */
            PathElement& options(const std::vector<std::string>& opts) {
                this->m_node->setAttribute(KARABO_SCHEMA_OPTIONS, opts);
                return *this;
            }

            PathElement& isInputFile() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "fileIn");
                return *this;
            }

            PathElement& isOutputFile() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "fileOut");
                return *this;
            }

            PathElement& isDirectory() {
                this->m_node->setAttribute(KARABO_SCHEMA_DISPLAY_TYPE, "directory");
                return *this;
            }

        protected:

            void beforeAddition() {
                this->m_node->setAttribute<int>(KARABO_SCHEMA_NODE_TYPE, Schema::LEAF);
                this->m_node->setAttribute<int>(KARABO_SCHEMA_LEAF_TYPE, karabo::util::Schema::PROPERTY);
                this->m_node->setAttribute(KARABO_SCHEMA_VALUE_TYPE, ToLiteral::to<Types::STRING>());
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE)) this->init(); // This is the default
                
                if (!this->m_node->hasAttribute(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL)) { 
                    
                    //for init, reconfigurable elements - set default value of requiredAccessLevel to USER
                    if (!this->m_node->hasAttribute(KARABO_SCHEMA_ACCESS_MODE) ||
                         this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == INIT ||
                         this->m_node->getAttribute<int>(KARABO_SCHEMA_ACCESS_MODE) == WRITE ) {         
                        
                        this->userAccess();   
                    
                    } else { //else set default value of requiredAccessLevel to OBSERVER 
                       this->m_node->setAttribute<int>(KARABO_SCHEMA_REQUIRED_ACCESS_LEVEL, Schema::OBSERVER);
                    } 
                }
            }
        };
        typedef PathElement PATH_ELEMENT;
    }
}

#endif

