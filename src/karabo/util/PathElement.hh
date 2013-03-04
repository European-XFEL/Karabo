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
                this->m_node->setAttribute("options", karabo::util::fromString<std::string, std::vector > (opts, sep));
                return *this;
            }

            /**
             * The <b>options</b> method specifies values allowed for this parameter. Each value is an element of the vector.
             * This function can be used when space cannot be used as a separator.
             * @param opts vector of strings. The values are casted to the proper type.
             * @return reference to the PathElement
             */
            PathElement& options(const std::vector<std::string>& opts) {
                this->m_node->setAttribute("options", opts);
                return *this;
            }
            
            PathElement& isInputFile() {
                this->m_node->setAttribute("displayType", "fileIn");
                return *this;
            }
            
             PathElement& isOutputFile() {
                this->m_node->setAttribute("displayType", "fileOut");
                return *this;
            }
             
             PathElement& isDirectory() {
                 this->m_node->setAttribute("displayType", "directory");
                 return *this;
             }
             
        protected:

            void beforeAddition() {
                this->m_node->setAttribute<int>("nodeType", Schema::LEAF);
                this->m_node->setAttribute("valueType", ToLiteral::to<Types::STRING>());
                if (!this->m_node->hasAttribute("accessMode")) this->init(); // This is the default
            }
        };
        typedef PathElement PATH_ELEMENT;
    }
}

#endif

