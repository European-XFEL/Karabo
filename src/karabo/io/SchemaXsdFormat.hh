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

#ifndef KARABO_IO_XMLFORMAT_HH
#define	KARABO_IO_XMLFORMAT_HH

#include <karabo/xml/tinyxml.h>
#include <karabo/util/Factory.hh>
#include <boost/foreach.hpp>

#include "Format.hh"

namespace karabo {

    namespace io {

        class SchemaXsdFormat : public Format<karabo::util::Schema> {
        public:

            KARABO_CLASSINFO(SchemaXsdFormat, "Xsd", "1.0")

            SchemaXsdFormat() {
            };

            virtual ~SchemaXsdFormat() {
            };

            static void expectedParameters(karabo::util::Schema& expected);

            void configure(const karabo::util::Hash& input);

            /**
             * Reading an XSD file (we do not need such functionality, therefore function is not implemented).
             * Function reads XSD Document and
             * creates an object of the class karabo::util::Schema
             * @param in Input parameter representing XSD document
             * @param out Output parameter representing object of the class karabo::util::Schema
             */
             void convert(std::stringstream& in, karabo::util::Schema& out);


            /**
             * Writing an XSD file.
             * Function gets as input an object of the class karabo::util::Schema
             * and creates an XSD file.
             * @param in Input parameter representing an object of the class karabo::util::Schema
             * @param out Output parameter representing constructed XSD Document
             */
             void convert(const karabo::util::Schema& in, std::stringstream& out);

        private: // members

            int m_indentation;

            std::string m_defaultNamespace;

        private: // functions

            void r_writeXmlExpectedObject(const karabo::util::Schema& expected, karabo::tinyxml::TiXmlElement* pTheElement);

            void setAssignmentTypeInXml(karabo::tinyxml::TiXmlElement* element, karabo::util::Schema::AssignmentType at);

            void setAppearenceSequenceTag(const karabo::util::Schema& complex, karabo::tinyxml::TiXmlElement* complexTypeTag, int minOccurs);

            void setAppearenceAllTag(const karabo::util::Schema& complex, karabo::tinyxml::TiXmlElement* complexTypeTag, int minOccurs);

            std::string rewriteTypeToXsd(const karabo::util::Types::Type typeOfElement);
        };
    } // namespace io
} // namespace karabo

#endif	/* KARABO_PACKAGENAME_XMLFORMAT_HH */
