/*
 * $Id: HashXmlFormat.hh 6764 2012-07-18 09:29:46Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 * Adapted from MasterConfigXsdFormat.hh of <irina.kozlova@xfel.eu>
 *
 * Created on September 14, 2011, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#ifndef EXFEL_IO_HASHXMLFORMAT_HH
#define	EXFEL_IO_HASHXMLFORMAT_HH

#include <karabo/xml/tinyxml.h>
#include <karabo/util/Factory.hh>
#include <boost/foreach.hpp>

#include "Format.hh"

namespace exfel {

    namespace io {

        /**
         * The HashXmlFormat class.
         */
        class DECLSPEC_IO HashXmlFormat : public Format<exfel::util::Hash> {
            
        public:

            EXFEL_CLASSINFO(HashXmlFormat, "Xml", "1.0")

            HashXmlFormat() {
            };

            virtual ~HashXmlFormat() {
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
            void convert(std::stringstream& in, exfel::util::Hash& out);


            /**
             * Writing a configuration file.
             * Function gets as input an object of the class exfel::util::Hash
             * and creates a configuration file in XML format.
             * @param in Input parameter representing an object of the class exfel::util::Hash
             * @param out Output parameter representing constructed XML Document
             */
            void convert(const exfel::util::Hash& in, std::stringstream& out);


        private: // members

            int m_indentation;

            bool m_printDataType;
            
            bool m_ignoreDataType;

            std::string m_defaultNamespace;
            
            Format<exfel::util::Schema>::Pointer m_schemaFormat;    

            
        private: // functions

            void removeNamespaceAttribute(exfel::tinyxml::TiXmlNode*);

            void r_readXmlDocument(exfel::tinyxml::TiXmlNode*, exfel::util::Hash&);

            void readArrayElement(exfel::tinyxml::TiXmlElement* nodeElement, const std::string& arrayType, exfel::util::Hash& data) const;

            /**
             * Recursive function for creating an XML document from an object of the class exfel::util::Hash
             * @param data Input parameter representing the object of the class exfel::util::Hash
             * @param pTheElement Input parameter is an XML element (as defined by TinyXml) that will be recursively constructed
             */
            void r_createXmlFile(const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement);

            /**
             * Constructing an XML element from the given VECTOR-datastructure.
             * <nameOfElement dataType="typeOfElement"><item>1st value<item>...<item>N value</item></nameOfElement>
             * Example. Representation in LibConfig format:
             * columns = [ "Last Name", "First Name", "MI" ];
             * Representation in XML document:
             * <columns dataType="VECTOR_STRING"><item>Last Name</item><item>First Name</item><item>MI</item></columns>
             * @param typeOfElement Data Type of the Hash object
             * @param nameOfElement Name of the Hash object
             * @param valueOfElement Value of the Hash object
             * @param pTheElement XML element to be constructed
             */

            template<class T>
            void fillXmlElementWithItems(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const {

                try {

                    exfel::tinyxml::TiXmlElement* newElement = new exfel::tinyxml::TiXmlElement(nameOfElement.c_str());

                    newElement->SetAttribute("dataType", typeOfElement);

                    const std::vector<T>& arrayOfElements = data.get<std::vector<T> >(nameOfElement);

                    BOOST_FOREACH(T elem, arrayOfElements) {
                        exfel::tinyxml::TiXmlElement* itemElement = new exfel::tinyxml::TiXmlElement("item");
                        newElement->LinkEndChild(itemElement);
                        std::string elemAsString = exfel::util::String::toString(elem);
                        exfel::tinyxml::TiXmlText* text = new exfel::tinyxml::TiXmlText(elemAsString.c_str());
                        itemElement->LinkEndChild(text);
                    }

                    pTheElement->LinkEndChild(newElement);
                } catch (...) {
                    RETHROW;
                }

            }//HashXmlFormat::fillXmlElementWithItems

            void fillXmlElementINT8(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;

            void fillXmlElementUINT8(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;
            
            void fillXmlElementCHAR(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;

            void fillXmlElementWithItemsBool(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;

            void fillXmlElementWithItemsUINT8(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;

            void fillXmlElementWithItemsINT8(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;
            
            void fillXmlElementWithItemsCHAR(const char* typeOfElement, const std::string& nameOfElement, const exfel::util::Hash& data, exfel::tinyxml::TiXmlElement* pTheElement) const;
        };
    } // namespace io
} // namespace exfel

#endif	
