/*
 * $Id: SchemaXsdFormat.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * File:   SchemaXsdFormat.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on September 10, 2010, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <stdio.h>
#include <complex>
#include <string>
#include <karabo/util/ConfigConstants.hh>

#include "SchemaXsdFormat.hh"

KARABO_REGISTER_FACTORY_CC(karabo::io::Format<karabo::util::Schema>, karabo::io::SchemaXsdFormat)

namespace karabo {
    namespace io {

        using namespace std;
        using namespace karabo::util;
        using namespace karabo::tinyxml;


        void SchemaXsdFormat::expectedParameters(karabo::util::Schema& expected) {

            INT32_ELEMENT(expected)
                    .key("indentation")
                    .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                    .displayedName("Indentation")
                    .options("-1 0 1 2 3 4")
                    .assignmentOptional().defaultValue(2)
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


        void SchemaXsdFormat::configure(const karabo::util::Hash& input) {
            if (input.has("indentation")) {
                input.get("indentation", m_indentation);
            } else {
                m_indentation = 2;
            }
            m_defaultNamespace = input.get<string > ("xmlns");
        }


        /**
         * Reading a configuration file.
         * Function reads configuration file (XML Document) and
         * creates an object of the class karabo::util::Hash
         * @param in Input parameter representing XML document
         * @param out Output parameter representing object of the class karabo::util::Hash
         */
        void SchemaXsdFormat::convert(stringstream& in, Schema& out) {
            throw KARABO_NOT_IMPLEMENTED_EXCEPTION("Reading (de-serialization) of .xsd file into Schema objects is currently not implemented");
        }


        /**
         * Writing a configuration file.
         * Function gets as input an object of the class karabo::util::Hash
         * and creates a configuration file in XML format.
         * @param in Input parameter representing an object of the class karabo::util::Hash
         * @param out Output parameter representing constructed XML Document
         */
        void SchemaXsdFormat::convert(const Schema& in, stringstream& out) {

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

            TiXmlElement* schemaElem = new TiXmlElement("xs:schema");
            schemaElem->SetAttribute("xmlns:xs", "http://www.w3.org/2001/XMLSchema");
            schemaElem->SetAttribute("xmlns:a", "http://www.karabo.eu");
            doc.LinkEndChild(schemaElem);

            //creating xsd-description of root element
            if (!in.has("root")) return;
            string key = in.getAsString("root"); //nameOfRootElement at the beginnig
            TiXmlElement* rootElem = new TiXmlElement("xs:element");
            rootElem->SetAttribute("name", key.c_str());

            //adding root Element as first element to schemaElem
            schemaElem->LinkEndChild(rootElem);

            r_writeXmlExpectedObject(in, rootElem);

            doc.Accept(&printer);
            out << printer.CStr();

        }


        void SchemaXsdFormat::r_writeXmlExpectedObject(const Schema& expected, TiXmlElement* pTheElement) {
            try {
                if (!expected.has("root")) return;

                //string key = expected.getAsString("root");
                //cout<<"Root-key: "<<key<<endl;

                TiXmlElement* complexTypeRoot = new TiXmlElement("xs:complexType");
                TiXmlElement* sequenceRoot = new TiXmlElement("xs:all"); //instead of xs:sequence
                complexTypeRoot->LinkEndChild(sequenceRoot);

                pTheElement->LinkEndChild(complexTypeRoot);

                //Getting elements of Root
                const Schema& elements = expected.get<Schema > ("elements");

                for (Schema::const_iterator it = elements.begin(); it != elements.end(); it++) {
                    const Schema& desc = elements.get<Schema > (it);
                    Schema::AssignmentType at = desc.get<Schema::AssignmentType > ("assignment");
                    string elementName;
                    if (!desc.has("root")) {
                        elementName = desc.getAsString("key");
                    }

                    TiXmlElement* annotationTag;
                    bool annotationExists = false;
                    if (desc.has("description") || desc.has("displayedName") || desc.has("expertLevel") || desc.has("default") || desc.has("unitName") || desc.has("unitSymbol") || desc.has("access") || desc.has("displayType") || desc.has("allowedStates")) {
                        annotationExists = true;
                        annotationTag = new TiXmlElement("xs:annotation");
                        TiXmlElement* documentationTag = new TiXmlElement("xs:documentation");
                        annotationTag->LinkEndChild(documentationTag);

                        if (desc.has("description")) {
                            TiXmlElement* descriptionTag = new TiXmlElement("a:description");
                            TiXmlText * text = new TiXmlText(desc.getAsString("description").c_str());
                            descriptionTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(descriptionTag);
                        }

                        if (desc.has("displayedName")) {
                            TiXmlElement* displayedNameTag = new TiXmlElement("a:displayedName");
                            TiXmlText * text = new TiXmlText(desc.getAsString("displayedName").c_str());
                            displayedNameTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(displayedNameTag);
                        }

                        if (desc.has("expertLevel")) {
                            TiXmlElement* expertLevelTag = new TiXmlElement("a:expertLevel");
                            TiXmlText * text = new TiXmlText(desc.getAsString("expertLevel").c_str());
                            expertLevelTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(expertLevelTag);
                        }

                        if (desc.has("unitName")) {
                            TiXmlElement* unitNameTag = new TiXmlElement("a:unitName");
                            TiXmlText * text = new TiXmlText(desc.getAsString("unitName").c_str());
                            unitNameTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(unitNameTag);
                        }

                        if (desc.has("unitSymbol")) {
                            TiXmlElement* unitSymbolTag = new TiXmlElement("a:unitSymbol");
                            TiXmlText * text = new TiXmlText(desc.getAsString("unitSymbol").c_str());
                            unitSymbolTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(unitSymbolTag);
                        }

                        if (desc.has("default")) {
                            TiXmlElement* defaultValueTag = new TiXmlElement("a:default");
                            TiXmlText * text = new TiXmlText(desc.getAsString("default").c_str());
                            defaultValueTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(defaultValueTag);
                        }

                        if (desc.has("access")) {
                            TiXmlElement* accessValueTag = new TiXmlElement("a:accessType");
                            AccessType aT = desc.get<AccessType >("access");
                            string str = String::toString(aT);
                            TiXmlText * text = new TiXmlText(str.c_str());
                            accessValueTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(accessValueTag);
                        }

                        if (desc.has("displayType")) {
                            TiXmlElement* displayTypeTag = new TiXmlElement("a:displayType");
                            TiXmlText * text = new TiXmlText(desc.getAsString("displayType").c_str());
                            displayTypeTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(displayTypeTag);
                        }

                        if (desc.has("allowedStates")) {
                            TiXmlElement* allowedStatesValueTag = new TiXmlElement("a:allowedStates");
                            TiXmlText * text = new TiXmlText(desc.getAsString("allowedStates").c_str());
                            allowedStatesValueTag->LinkEndChild(text);
                            documentationTag->LinkEndChild(allowedStatesValueTag);
                        }

                    }

                    if (at != Schema::INTERNAL_PARAM) {//consider here cases MANDATORY_PARAM or OPTIONAL_PARAM
                        if (desc.has("root")) {//processing root-type

                            string key = desc.getAsString("root");
                            TiXmlElement* elemRoot = new TiXmlElement("xs:element");
                            elemRoot->SetAttribute("name", key.c_str());
                            sequenceRoot->LinkEndChild(elemRoot);

                            setAssignmentTypeInXml(elemRoot, at);

                            if (annotationExists) {
                                elemRoot->LinkEndChild(annotationTag);
                            }

                            r_writeXmlExpectedObject(desc, elemRoot);

                        } else if (desc.has("simpleType")) {//processing simple type
                            TiXmlElement* simpleElem = new TiXmlElement("xs:element");
                            simpleElem->SetAttribute("name", elementName.c_str());

                            if (annotationExists) {
                                simpleElem->LinkEndChild(annotationTag);
                            }

                            Types::ReferenceType typeOfElem = desc.get<Types::ReferenceType > ("simpleType");

                            //type of element or type of all elements of the vector
                            string typeOfElemXsd = Types::convertToXsd(typeOfElem);

                            //in case there is VECTOR_ simple type, set boolean isVector to true
                            bool isVector = false;
                            if (Types::convert(typeOfElem).substr(0, 6) == "VECTOR") {
                                isVector = true;
                            }

                            if (desc.has("options")) {

                                TiXmlElement* simpleType = new TiXmlElement("xs:simpleType");
                                simpleElem->LinkEndChild(simpleType);

                                TiXmlElement* restriction = new TiXmlElement("xs:restriction");
                                simpleType->LinkEndChild(restriction);
                                restriction->SetAttribute("base", typeOfElemXsd.c_str());

                                vector<string> options = desc.get<vector<string> >("options");
                                size_t length = options.size();
                                for (size_t i = 0; i < length; i++) {
                                    TiXmlElement* enumeration = new TiXmlElement("xs:enumeration");
                                    restriction->LinkEndChild(enumeration);
                                    enumeration->SetAttribute("value", options[i].c_str());
                                }

                            } else if (desc.has("minInc") || desc.has("minExc") || desc.has("maxInc") || desc.has("maxExc") || desc.has("minSize") || desc.has("maxSize") || isVector) {

                                TiXmlElement* restriction = new TiXmlElement("xs:restriction");
                                TiXmlElement* simpleType;

                                if (!isVector) { //processing non-vector simple type
                                    simpleType = new TiXmlElement("xs:simpleType");
                                    simpleElem->LinkEndChild(simpleType);

                                    simpleType->LinkEndChild(restriction);

                                } else { //processing VECTOR_ simple type
                                    simpleType = new TiXmlElement("xs:complexType"); //in VECTOR_ case processing as complexType
                                    simpleElem->LinkEndChild(simpleType);

                                    TiXmlElement* sequen = new TiXmlElement("xs:sequence");
                                    simpleType->LinkEndChild(sequen);

                                    TiXmlElement* attrDataType = new TiXmlElement("xs:attribute");
                                    simpleType->LinkEndChild(attrDataType);
                                    attrDataType->SetAttribute("name", "dataType");
                                    attrDataType->SetAttribute("type", "xs:string");

                                    TiXmlElement* vectorElem = new TiXmlElement("xs:element");
                                    sequen->LinkEndChild(vectorElem);

                                    TiXmlElement* simpleT = new TiXmlElement("xs:simpleType");
                                    vectorElem->LinkEndChild(simpleT);

                                    simpleT->LinkEndChild(restriction);

                                    //for element vectorElem set attributes name="item"
                                    //and (if present) minOccurs and maxOccurs (corresponding to minSize and maxSize in expectedParameters)
                                    vectorElem->SetAttribute("name", "item");
                                    if (desc.has("minSize"))
                                        vectorElem->SetAttribute("minOccurs", desc.getAsString("minSize").c_str());
                                    if (desc.has("maxSize"))
                                        vectorElem->SetAttribute("maxOccurs", desc.getAsString("maxSize").c_str());
                                    if (!desc.has("minSize") && !desc.has("maxSize"))
                                        vectorElem->SetAttribute("maxOccurs", "unbounded");

                                }

                                restriction->SetAttribute("base", typeOfElemXsd.c_str());

                                if (desc.has("minInc")) {
                                    TiXmlElement* minInc = new TiXmlElement("xs:minInclusive");
                                    restriction->LinkEndChild(minInc);
                                    minInc->SetAttribute("value", desc.getAsString("minInc").c_str());
                                } else if (desc.has("minExc")) {
                                    TiXmlElement* minExc = new TiXmlElement("xs:minExclusive");
                                    restriction->LinkEndChild(minExc);
                                    minExc->SetAttribute("value", desc.getAsString("minExc").c_str());
                                }

                                if (desc.has("maxInc")) {
                                    TiXmlElement* maxInc = new TiXmlElement("xs:maxInclusive");
                                    restriction->LinkEndChild(maxInc);
                                    maxInc->SetAttribute("value", desc.getAsString("maxInc").c_str());
                                } else if (desc.has("maxExc")) {
                                    TiXmlElement* maxExc = new TiXmlElement("xs:maxExclusive");
                                    restriction->LinkEndChild(maxExc);
                                    maxExc->SetAttribute("value", desc.getAsString("maxExc").c_str());
                                }

                            } else {
                                if (!isVector) simpleElem->SetAttribute("type", typeOfElemXsd.c_str());
                                //case isVector has been already processed
                            }

                            if (desc.has("default") && !isVector) {
                                simpleElem->SetAttribute("default", desc.getAsString("default").c_str());
                            }

                            setAssignmentTypeInXml(simpleElem, at);

                            sequenceRoot->LinkEndChild(simpleElem);

                        } else if (desc.has("complexType")) {//processsing complex type

                            Schema::OccuranceType occ = desc.get<Schema::OccuranceType > ("occurrence");

                            TiXmlElement* complexElem = new TiXmlElement("xs:element");
                            complexElem->SetAttribute("name", elementName.c_str());

                            if (annotationExists) {
                                complexElem->LinkEndChild(annotationTag);
                            }

                            TiXmlElement* complexTypeTag = new TiXmlElement("xs:complexType");
                            complexElem->LinkEndChild(complexTypeTag);

                            setAssignmentTypeInXml(complexElem, at);

                            //getting elements contained in this complexElement
                            const Schema& complex = desc.get<Schema > ("complexType");

                            //choice or sequence
                            TiXmlElement* appearenceTag;
                            if (occ == Schema::EITHER_OR) {
                                appearenceTag = new TiXmlElement("xs:choice");
                                complexTypeTag->LinkEndChild(appearenceTag);
                                for (Schema::const_iterator it = complex.begin(); it != complex.end(); it++) {
                                    string nameElem = it->first;
                                    TiXmlElement* contentAppearence = new TiXmlElement("xs:element");
                                    contentAppearence->SetAttribute("name", nameElem.c_str());

                                    appearenceTag->LinkEndChild(contentAppearence);

                                    r_writeXmlExpectedObject(complex.get<Schema > (it), contentAppearence);
                                }

                            } else if (occ == Schema::ONE_OR_MORE) {
                                setAppearenceSequenceTag(complex, complexTypeTag, 1);
                            } else if (occ == Schema::ZERO_OR_MORE) {
                                setAppearenceSequenceTag(complex, complexTypeTag, 0);
                            } else if (occ == Schema::ZERO_OR_ONE) {
                                setAppearenceAllTag(complex, complexTypeTag, 0);
                            } else if (occ == Schema::EXACTLY_ONCE) {
                                setAppearenceAllTag(complex, complexTypeTag, 1);
                            }

                            sequenceRoot->LinkEndChild(complexElem);

                        }//processsing complex type

                    }//considering elements with AssignmentType != Schema::INTERNAL_PARAM (i.e., OPTIONAL_PARAM or MANDATORY_PARAM)
                }//for-loop over root-elements

            } catch (...) {
                KARABO_RETHROW;
            }
        }


        void SchemaXsdFormat::setAssignmentTypeInXml(TiXmlElement* element, Schema::AssignmentType at) {
            if (at == Schema::OPTIONAL_PARAM) {
                element->SetAttribute("minOccurs", 0);
                element->SetAttribute("maxOccurs", 1);
            } else if (at == Schema::MANDATORY_PARAM) {
                element->SetAttribute("minOccurs", 1);
                element->SetAttribute("maxOccurs", 1);
            }
        }


        void SchemaXsdFormat::setAppearenceSequenceTag(const Schema& complex, TiXmlElement* complexTypeTag, int minOccurs) {
            TiXmlElement* appearenceTag = new TiXmlElement("xs:sequence");
            complexTypeTag->LinkEndChild(appearenceTag);

            //Complex element can have attribute named 'dataType', representation in XML Schema:
            //<xs:attribute name="dataType" type="xs:string" default="LIST"/>
            TiXmlElement* attributeTag = new TiXmlElement("xs:attribute");
            attributeTag->SetAttribute("name", "dataType");
            attributeTag->SetAttribute("type", "xs:string");
            attributeTag->SetAttribute("default", "LIST");
            complexTypeTag->LinkEndChild(attributeTag);

            TiXmlElement* itemElem = new TiXmlElement("xs:element");
            itemElem->SetAttribute("name", "item");
            itemElem->SetAttribute("minOccurs", minOccurs);
            itemElem->SetAttribute("maxOccurs", "unbounded");
            TiXmlElement* complexTag = new TiXmlElement("xs:complexType");
            TiXmlElement* choiceTag = new TiXmlElement("xs:choice");
            itemElem->LinkEndChild(complexTag);
            complexTag->LinkEndChild(choiceTag);

            appearenceTag->LinkEndChild(itemElem);

            for (Schema::const_iterator it = complex.begin(); it != complex.end(); it++) {
                string nameElem = it->first;
                TiXmlElement* contentAppearence = new TiXmlElement("xs:element");
                contentAppearence->SetAttribute("name", nameElem.c_str());
                choiceTag->LinkEndChild(contentAppearence);

                r_writeXmlExpectedObject(complex.get<Schema > (it), contentAppearence);
            }
        }


        void SchemaXsdFormat::setAppearenceAllTag(const Schema& complex, TiXmlElement* complexTypeTag, int minOccurs) {
            TiXmlElement* appearenceTag = new TiXmlElement("xs:all"); //instead of xs:sequence
            complexTypeTag->LinkEndChild(appearenceTag);

            for (Schema::const_iterator it = complex.begin(); it != complex.end(); it++) {
                string nameElem = it->first;
                TiXmlElement* contentAppearence = new TiXmlElement("xs:element");
                contentAppearence->SetAttribute("name", nameElem.c_str());

                contentAppearence->SetAttribute("minOccurs", minOccurs);
                contentAppearence->SetAttribute("maxOccurs", 1);
                appearenceTag->LinkEndChild(contentAppearence);

                r_writeXmlExpectedObject(complex.get<Schema > (it), contentAppearence);
            }
        }
    } // namespace io
} // namespace karabo
