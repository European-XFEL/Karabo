/*
 * $Id: SchemaXsdFormat.hh 6608 2012-06-25 19:41:16Z wrona $
 *
 * File:   SchemaXsdFormat.hh
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_XMLFORMAT_HH
#define	EXFEL_IO_XMLFORMAT_HH

#include <karabo/xml/tinyxml.h>
#include <karabo/util/Factory.hh>
#include <boost/foreach.hpp>

#include "Format.hh"

namespace exfel {

    namespace io {

        /**
         * The MasterConfigXsdFormat class.
         */
        class SchemaXsdFormat : public Format<exfel::util::Schema> {
        public:

            EXFEL_CLASSINFO(SchemaXsdFormat, "Xsd", "1.0")

            SchemaXsdFormat() {
            };

            virtual ~SchemaXsdFormat() {
            };

            static void expectedParameters(exfel::util::Schema& expected);

            void configure(const exfel::util::Hash& input);

            /**
             * Reading a configuration file.
             * Function reads configuration file (XML Document) and
             * creates an object of the class exfel::util::Hash
             * @param in Input parameter representing XML document
             * @param out Output parameter representing object of the class exfel::util::Hash
             */
            void convert(std::stringstream& in, exfel::util::Schema& out);


            /**
             * Writing a configuration file.
             * Function gets as input an object of the class exfel::util::Hash
             * and creates a configuration file in XML format.
             * @param in Input parameter representing an object of the class exfel::util::Hash
             * @param out Output parameter representing constructed XML Document
             */
            void convert(const exfel::util::Schema& in, std::stringstream& out);

        private: // members

            int m_indentation;

            std::string m_defaultNamespace;

        private: // functions

            void r_writeXmlExpectedObject(const exfel::util::Schema& expected, exfel::tinyxml::TiXmlElement* pTheElement);

            void setAssignmentTypeInXml(exfel::tinyxml::TiXmlElement* element, exfel::util::Schema::AssignmentType at);

            void setAppearenceSequenceTag(const exfel::util::Schema& complex, exfel::tinyxml::TiXmlElement* complexTypeTag, int minOccurs);

            void setAppearenceAllTag(const exfel::util::Schema& complex, exfel::tinyxml::TiXmlElement* complexTypeTag, int minOccurs);

            std::string rewriteTypeToXsd(const exfel::util::Types::Type typeOfElement);
        };
    } // namespace io
} // namespace exfel

#endif	/* EXFEL_PACKAGENAME_XMLFORMAT_HH */
