/*
 * $Id: HashXmlFormat.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * Author: <burkhard.heisen@xfel.eu>
 * Adapted from MasterConfigXsdFormat.hh of <irina.kozlova@xfel.eu>
 *
 * Created on September 14, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <stdio.h>
#include <complex>
#include <string>

#include "HashXmlFormat.hh"

using namespace karabo::tinyxml;

KARABO_REGISTER_FACTORY_CC(karabo::io::Format<karabo::util::Hash>, karabo::io::HashXmlFormat)

namespace karabo {
    namespace io {

        using namespace std;
        using namespace karabo::util;

        void HashXmlFormat::expectedParameters(karabo::util::Schema& expected) {

            INT32_ELEMENT(expected)
                    .key("indentation")
                    .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                    .displayedName("Indentation")
                    .options("-1 0 1 2 3 4")
                    .assignmentOptional().defaultValue(2)
                    .advanced()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("printDataType")
                    .description("Default value is false (or 0). In order to print data types information in XML document, set this parameter to true (or 1).")
                    .displayedName("Printing data types")
                    .assignmentOptional().defaultValue(false)
                    .advanced()
                    .commit();
            
            BOOL_ELEMENT(expected)
                    .key("ignoreDataType")
                    .description("If this flag is true, any data type information will be ignored upon reading. Leave elements will always be interpreted as strings.")
                    .displayedName("Ignore data type")
                    .assignmentOptional().defaultValue(false)
                    .advanced()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("xmlns")
                    .description("Sets the default XML namespace")
                    .displayedName("XML Namespace")
                    .assignmentOptional().defaultValue("http://xfel.eu/config")
                    .advanced()
                    .commit();
        }

        void HashXmlFormat::configure(const karabo::util::Hash& input) {
            
            input.get("indentation", m_indentation);
            input.get("printDataType", m_printDataType);
            input.get("ignoreDataType", m_ignoreDataType);
            input.get<string > ("xmlns", m_defaultNamespace);
            
            m_schemaFormat = Format<Schema>::create("Xml");
        }

        /**
         * Reading a configuration file.
         * Function reads configuration file (XML Document) and
         * creates an object of the class karabo::util::Hash
         * @param in Input parameter representing XML document
         * @param out Output parameter representing object of the class karabo::util::Hash
         */
        void HashXmlFormat::convert(stringstream& in, Hash& out) {
            try {
                TiXmlDocument doc;
                doc.Parse(in.str().c_str(), 0, TIXML_DEFAULT_ENCODING);
                TiXmlNode* rootNode = doc.RootElement();
                removeNamespaceAttribute(rootNode);
                r_readXmlDocument(rootNode, out);
            } catch (...) {
                KARABO_RETHROW;
            }
        }

        void HashXmlFormat::removeNamespaceAttribute(TiXmlNode* node) {
            // at the moment only removes the attribute if exists      
            const char* attribute = node->ToElement()->Attribute("xmlns");
            if (attribute) {
                node->ToElement()->RemoveAttribute("xmlns");
            }
        }

        /**
         * Reads an XML document recursively by nodes
         * @param pTheNode Input parameter representing XML node
         * @param data Output parameter
         */
        void HashXmlFormat::r_readXmlDocument(TiXmlNode* node, Hash& data) {

            while (node != 0) {

                int nodeType = node->Type();

                if (nodeType == TiXmlNode::TINYXML_TEXT) { // Simple type
                    string elementName = node->Parent()->ToElement()->Value();
                    TiXmlText* nodeText = node->ToText();
                    // Set as string by default
                    data.set(elementName, nodeText->Value());
                    // Read all attributes
                    TiXmlAttribute* attribute = node->Parent()->ToElement()->FirstAttribute();
                    while (attribute != 0) {
                        string attributeName = attribute->Name();
                        string attributeValue = attribute->Value();
                        boost::to_upper(attributeName);
                        boost::to_upper(attributeValue);
                        if (!m_ignoreDataType && attributeName == "DATATYPE") {
                            if (attributeValue == "SCHEMA") {
                                Schema schema = m_schemaFormat->unserialize(nodeText->Value());
                                data.set(elementName, schema);
                            }
                            Types::ReferenceType dataType = Types::convert(attributeValue);
                            data.convertFromString(elementName, dataType);
                        } else {
                            cout << "Ignoring attribute \"" << attributeName << "\" of XML element \"" << elementName << "\"" << endl;
                        }
                        attribute = attribute->Next();
                    }
                } else if (nodeType == TiXmlNode::TINYXML_ELEMENT) { // Complex type
                    TiXmlElement* nodeElement = node->ToElement();
                    string elementName = nodeElement->Value();
                    bool isArray = false;
                    string arrayType("");
                    bool isList = false;
                    bool isSchema = false;
                    bool isHash = false;
                    TiXmlAttribute* attribute = node->ToElement()->FirstAttribute();
                    while (attribute != 0) {
                        string attributeName = attribute->Name();
                        string attributeValue = attribute->Value();
                        boost::to_upper(attributeName);
                        boost::to_upper(attributeValue);
                        if (attributeName == "DATATYPE") {
                            if (attributeValue == "LIST") isList = true;
                            if (attributeValue == "SCHEMA") isSchema = true;
                            if (attributeValue == "HASH") isHash = true;
                            if (attributeValue.substr(0, 5) == "ARRAY") {
                                isArray = true;
                                if (attributeValue.size() > 6) arrayType = attributeValue.substr(6);
                            }


                        } else if (attributeName == "ARTIFICIAL") { // Remove artificial root
                            data.clear();
                            r_readXmlDocument(node->FirstChild(), data);
                            return;
                        } else {
                            cout << "Ignoring attribute \"" << attributeName << "\" of XML element \"" << elementName << "\"" << endl;
                        }
                        attribute = attribute->Next();
                    }
                    if (isList) {
                        data.set(elementName, vector<Hash > ());
                        vector<Hash>& list = data.get<vector<Hash> >(elementName);
                        TiXmlElement* childElement = nodeElement->FirstChildElement();
                        while (childElement != 0) {
                            string childName = childElement->Value();
                            if (childName != "item") {
                                throw KARABO_PARAMETER_EXCEPTION("HashXmlFormat::r_readXmlDocument -> Unexpected child element: <" + childName + "> in the LIST-element <" + elementName + ">. List entries have to be surrounded by <item>[...]</item> elements");
                            }
                            list.push_back(Hash());
                            r_readXmlDocument(childElement->FirstChild(), *(list.rbegin()));
                            childElement = childElement->NextSiblingElement();
                        }
                    } else if (isArray) {
                        readArrayElement(nodeElement, arrayType, data);
                    } else {
                        if (node->FirstChild() == 0) { // empty element, i.e. <foo></foo>
                            if (isHash) {
                                data.set(elementName, Hash());
                            } else {
                                data.set(elementName, "");
                            }
                        } else {
                            if (node->FirstChild()->Type() == TiXmlNode::TINYXML_ELEMENT) {
                               
                                
                                    data.set(elementName, Hash());
                                    r_readXmlDocument(node->FirstChild(), data.get<Hash > (elementName));
                                
                            } else {
                                r_readXmlDocument(node->FirstChild(), data);
                            }
                        }
                    }
                }
                node = node->NextSibling();
            }
        }

        void HashXmlFormat::readArrayElement(TiXmlElement* nodeElement, const string& arrayType, Hash& data) const {
            vector<string> tmpArray;
            string elementName = nodeElement->Value();
            TiXmlElement* childElement = nodeElement->FirstChildElement();
            while (childElement != 0) {
                string childName = childElement->Value();
                if (childName != "item") {
                    throw KARABO_CAST_EXCEPTION("HashXmlFormat::r_readXmlDocument -> Unexpected child element \"" + childName + "\" in the ARRAY-element \"" + elementName + "\". Expected: \"item\".");
                }
                string valueOfItem = childElement->FirstChild()->ToText()->Value();
                tmpArray.push_back(valueOfItem);
                childElement = childElement->NextSiblingElement();
            }
            if (arrayType != "") {
                Types::ReferenceType type = Types::convert("VECTOR_" + arrayType);
                string stringArray = String::sequenceToString(tmpArray);
                data.set(elementName, stringArray);
                data.convertFromString(elementName, type);
            } else {
                data.set(elementName, tmpArray);
            }
        }

        /**
         * Writing a configuration file.
         * Function gets as input an object of the class karabo::util::Hash
         * and creates a configuration file in XML format.
         * @param in Input parameter representing an object of the class karabo::util::Hash
         * @param out Output parameter representing constructed XML Document
         */
        void HashXmlFormat::convert(const Hash& in, stringstream& out) {

            TiXmlDocument doc;
            TiXmlDeclaration * decl = new TiXmlDeclaration("1.0", "", "");
            doc.LinkEndChild(decl);

            TiXmlPrinter printer;
            if (m_indentation == -1) {
                printer.SetStreamPrinting();
            } else {
                string str(m_indentation, ' ');
                printer.SetIndent(str.c_str());
            }
            if (in.size() == 1 && in.getTypeAsId(in.begin()) == Types::HASH) { //Using Hash root element here
                const string& rootKey = in.begin()->first;
                TiXmlElement* rootElem = new TiXmlElement(rootKey.c_str());
                rootElem->SetAttribute("xmlns", m_defaultNamespace.c_str());
                if (m_printDataType) {
                    rootElem->SetAttribute("datatype", "HASH");
                }
                doc.LinkEndChild(rootElem);
                r_createXmlFile(in.get<Hash > (rootKey), rootElem);
            } else {//If there are no Hash root element here, create artificial root
                // Create artificial root
                TiXmlElement* rootElem = new TiXmlElement("karabo");
                rootElem->SetAttribute("artificial", ""); // Flag this root as being artificial
                doc.LinkEndChild(rootElem);
                r_createXmlFile(in, rootElem);
            }
            doc.Accept(&printer);
            out << printer.CStr();
        }

        /**
         * Recursive function for creating an XML document from an object of the class karabo::util::Hash
         * @param data Input parameter representing the object of the class karabo::util::Hash
         * @param pTheElement Input parameter is an XML element (as defined by TinyXml) that will be recursively constructed
         */
        void HashXmlFormat::r_createXmlFile(const Hash& data, TiXmlElement* pTheElement) {

            try {
                for (Hash::const_iterator it = data.begin(); it != data.end(); it++) {
                    string nameOfElement = it->first;
                    Types::ReferenceType type = data.getTypeAsId(it);
                    switch (type) {
                        case Types::HASH:
                        {
                            TiXmlElement* pChildElement = new TiXmlElement(nameOfElement.c_str());
                            pTheElement->LinkEndChild(pChildElement);
                            if (m_printDataType) {
                                pChildElement->SetAttribute("datatype", "HASH");
                            }
                            r_createXmlFile(data.get<Hash > (it), pChildElement);
                        }
                            break;

                        case Types::SCHEMA:
                        {
                            
                            TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
                            newElement->SetAttribute("dataType", "SCHEMA");
                            
                                                        
                            string schema = m_schemaFormat->serialize(data.get<Schema>(it));
                            
                            //std::cout << schema << std::endl;
                            TiXmlText* text = new TiXmlText(schema.c_str());
                            newElement->LinkEndChild(text);
                            pTheElement->LinkEndChild(newElement);
                        }
                        break;

                        case Types::VECTOR_HASH:
                        {
                            TiXmlElement* pChildElement = new TiXmlElement(nameOfElement.c_str());
                            //pChildElement->SetAttribute("type", "VECTOR_HASH"); //previous notation
                            pChildElement->SetAttribute("dataType", "LIST"); //new notation
                            pTheElement->LinkEndChild(pChildElement);

                            const vector<Hash>& tmp = data.get<vector<Hash> > (it);
                            for (size_t i = 0; i < tmp.size(); i++) {
                                TiXmlElement* itemElement = new TiXmlElement("item");
                                pChildElement->LinkEndChild(itemElement);
                                r_createXmlFile(tmp[i], itemElement);
                            }
                        }
                            break;

                        case Types::VECTOR_STRING:
                            fillXmlElementWithItems<string > ("ARRAY_STRING", nameOfElement, data, pTheElement);
                            break;

                        case Types::INT8:
                            fillXmlElementINT8("INT8", nameOfElement, data, pTheElement);
                            break;

                        case Types::UINT8:
                            fillXmlElementUINT8("UINT8", nameOfElement, data, pTheElement);
                            break;
                            
                       case Types::CHAR:
                            fillXmlElementCHAR("CHAR", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_INT8:
                            fillXmlElementWithItemsINT8("ARRAY_INT8", nameOfElement, data, pTheElement);
                            break;
                        
                        case Types::VECTOR_CHAR:
                            fillXmlElementWithItemsCHAR("ARRAY_CHAR", nameOfElement, data, pTheElement);
                            break;
                            
                        case Types::VECTOR_INT16:
                            fillXmlElementWithItems<signed short > ("ARRAY_INT16", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_INT32:
                            fillXmlElementWithItems<signed int>("ARRAY_INT32", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_INT64:
                            fillXmlElementWithItems<signed long long>("ARRAY_INT64", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_UINT8:
                            fillXmlElementWithItemsUINT8("ARRAY_UINT8", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_UINT16:
                            fillXmlElementWithItems<unsigned short > ("ARRAY_UINT16", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_UINT32:
                            fillXmlElementWithItems<unsigned int > ("ARRAY_UINT32", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_UINT64:
                            fillXmlElementWithItems<unsigned long long > ("ARRAY_UINT64", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_BOOL:
                            fillXmlElementWithItemsBool("ARRAY_BOOL", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_DOUBLE:
                            fillXmlElementWithItems<double>("ARRAY_DOUBLE", nameOfElement, data, pTheElement);
                            break;

                        case Types::VECTOR_FLOAT:
                            fillXmlElementWithItems<float>("ARRAY_FLOAT", nameOfElement, data, pTheElement);
                            break;

                        default:
                        {
                            TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());

                            if (m_printDataType) {
                                newElement->SetAttribute("dataType", Types::convert(type).c_str());
                            }

                            TiXmlText* text = new TiXmlText(data.getAsString(nameOfElement).c_str());
                            newElement->LinkEndChild(text);

                            pTheElement->LinkEndChild(newElement);
                        }
                            break;

                    } // switch
                } // for
            } catch (...) {
                KARABO_RETHROW;
            }

        } //HashXmlFormat::r_createXmlFile

        void HashXmlFormat::fillXmlElementUINT8(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
            if (m_printDataType) {
                newElement->SetAttribute("dataType", typeOfElement);
            }
            int x = data.get<unsigned char>(nameOfElement);
            std::string elemAsString = karabo::util::String::toString(x);
            TiXmlText* text = new TiXmlText(elemAsString.c_str());
            newElement->LinkEndChild(text);

            pTheElement->LinkEndChild(newElement);
        }

        void HashXmlFormat::fillXmlElementINT8(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
            if (m_printDataType) {
                newElement->SetAttribute("dataType", typeOfElement);
            }
            int x = data.get<signed char>(nameOfElement);
            std::string elemAsString = karabo::util::String::toString(x);
            TiXmlText* text = new TiXmlText(elemAsString.c_str());
            newElement->LinkEndChild(text);

            pTheElement->LinkEndChild(newElement);
        }
        
        void HashXmlFormat::fillXmlElementCHAR(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
            if (m_printDataType) {
                newElement->SetAttribute("dataType", typeOfElement);
            }
            int x = data.get<char>(nameOfElement);
            std::string elemAsString = karabo::util::String::toString(x);
            TiXmlText* text = new TiXmlText(elemAsString.c_str());
            newElement->LinkEndChild(text);

            pTheElement->LinkEndChild(newElement);
        }

        void HashXmlFormat::fillXmlElementWithItemsBool(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            try {

                TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());

                newElement->SetAttribute("dataType", typeOfElement);

                const std::deque<bool>& arrayOfElements = data.get < std::deque<bool> >(nameOfElement);

                BOOST_FOREACH(bool elem, arrayOfElements) {
                    TiXmlElement* itemElement = new TiXmlElement("item");
                    newElement->LinkEndChild(itemElement);
                    std::string elemAsString = karabo::util::String::toString(elem);
                    TiXmlText* text = new TiXmlText(elemAsString.c_str());
                    itemElement->LinkEndChild(text);
                }

                pTheElement->LinkEndChild(newElement);
            } catch (...) {
                KARABO_RETHROW;
            }

        }//HashXmlFormat::fillXmlBoolDeque

        void HashXmlFormat::fillXmlElementWithItemsUINT8(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            try {

                TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
                newElement->SetAttribute("dataType", typeOfElement);

                const std::vector<unsigned char>& arrayOfElements = data.get<std::vector<unsigned char> >(nameOfElement);

                BOOST_FOREACH(unsigned char elem, arrayOfElements) {
                    int x = elem;
                    TiXmlElement* itemElement = new TiXmlElement("item");
                    newElement->LinkEndChild(itemElement);
                    std::string elemAsString = karabo::util::String::toString(x);
                    TiXmlText* text = new TiXmlText(elemAsString.c_str());
                    itemElement->LinkEndChild(text);

                }
                pTheElement->LinkEndChild(newElement);
            } catch (...) {
                KARABO_RETHROW;
            }
        }

        void HashXmlFormat::fillXmlElementWithItemsINT8(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            try {

                TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
                newElement->SetAttribute("dataType", typeOfElement);

                const std::vector<signed char>& arrayOfElements = data.get < std::vector<signed char> >(nameOfElement);

                BOOST_FOREACH(char elem, arrayOfElements) {
                    int x = elem;
                    TiXmlElement* itemElement = new TiXmlElement("item");
                    newElement->LinkEndChild(itemElement);
                    std::string elemAsString = karabo::util::String::toString(x);
                    TiXmlText* text = new TiXmlText(elemAsString.c_str());
                    itemElement->LinkEndChild(text);

                }
                pTheElement->LinkEndChild(newElement);
            } catch (...) {
                KARABO_RETHROW;
            }
        }
        
        void HashXmlFormat::fillXmlElementWithItemsCHAR(const char* typeOfElement, const std::string& nameOfElement, const karabo::util::Hash& data, TiXmlElement* pTheElement) const {

            try {

                TiXmlElement* newElement = new TiXmlElement(nameOfElement.c_str());
                newElement->SetAttribute("dataType", typeOfElement);

                const std::vector<char>& arrayOfElements = data.get < std::vector<char> >(nameOfElement);

                BOOST_FOREACH(char elem, arrayOfElements) {
                    int x = elem;
                    TiXmlElement* itemElement = new TiXmlElement("item");
                    newElement->LinkEndChild(itemElement);
                    std::string elemAsString = karabo::util::String::toString(x);
                    TiXmlText* text = new TiXmlText(elemAsString.c_str());
                    itemElement->LinkEndChild(text);

                }
                pTheElement->LinkEndChild(newElement);
            } catch (...) {
                KARABO_RETHROW;
            }
        }

    } // namespace io
} // namespace karabo
