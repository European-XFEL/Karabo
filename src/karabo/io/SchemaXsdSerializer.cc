/*
 * $Id: SchemaXsdSerializer.cc 6764 2012-07-18 09:29:46Z heisenb $
 *
 * File:   SchemaXsdSerializer.cc
 * Author: <irina.kozlova@xfel.eu>
 *
 * Created on March 12, 2013, 10:31 AM
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <karabo/util/Configurator.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/ToXsd.hh>
#include "SchemaXsdSerializer.hh"

using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace io {


        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer<Schema>, SchemaXsdSerializer)

        void SchemaXsdSerializer::expectedParameters(karabo::util::Schema& expected) {

            INT32_ELEMENT(expected)
                    .key("indentation")
                    .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. "
                                 "Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                    .displayedName("Indentation")
                    .options("-1 0 1 2 3 4")
                    .assignmentOptional().defaultValue(2)
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("xmlns")
                    .description("Sets the default XML namespace")
                    .displayedName("XML Namespace")
                    .assignmentOptional().defaultValue("http://www.w3.org/2001/XMLSchema")
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("xmlnsa")
                    .description("Sets the annotation namespace")
                    .displayedName("Annotation Namespace")
                    .assignmentOptional().defaultValue("http://www.karabo.eu")
                    .expertAccess()
                    .commit();
        }


        SchemaXsdSerializer::SchemaXsdSerializer(const Hash& input) {
            int indentation = input.get<int>("indentation");
            if (indentation == -1) {
                m_indentation = "";
                m_writeCompact = true;
            } else {
                m_indentation = string(indentation, ' ');
                m_writeCompact = false;
            }

            m_defaultNamespace = input.get<string > ("xmlns");
            m_xmlnsa = input.get<string > ("xmlnsa");
        }


        void SchemaXsdSerializer::load(karabo::util::Schema& object, const std::string& archive) {
            throw KARABO_NOT_SUPPORTED_EXCEPTION("Loading (de-serialization) of an XSD file into a Schema object is not supported");
        }


        void SchemaXsdSerializer::save(const Schema& object, std::string& archive) {
            pugi::xml_document doc;

            pugi::xml_node schemaNode = doc.append_child("xs:schema");
            schemaNode.append_attribute("xmlns:xs") = m_defaultNamespace.c_str();
            schemaNode.append_attribute("xmlns:a") = m_xmlnsa.c_str();

            r_createXsd(object, schemaNode, false);

            CustomWriter writer(archive);
            if (m_writeCompact) doc.save(writer, "", pugi::format_raw);
            else doc.save(writer, m_indentation.c_str(), pugi::format_indent);
        }


        void SchemaXsdSerializer::r_createXsd(const Schema& schema, pugi::xml_node& node, const bool isChildNodeOfListElement, const string& key) const {

            vector<string> keys;
            string elementName;

            if (key.empty()) {//top schema element
                elementName = schema.getRootName();
                keys = schema.getKeys();
            } else {//other Node schema element
                if (key.rfind(".") != string::npos) {
                    keys = schema.getKeys(key);
                    elementName = extractKey(key);
                } else {
                    elementName = key;
                    keys = schema.getKeys(key);
                }
            }

            pugi::xml_node rootElementNode = node.append_child("xs:element");
            rootElementNode.append_attribute("name") = elementName.c_str();

            if (isChildNodeOfListElement) {
                rootElementNode.append_attribute("minOccurs") = "0";
                rootElementNode.append_attribute("maxOccurs") = "unbounded";
            }

            if (!key.empty()) {
                if (schema.hasDisplayedName(key)) {//sufficient condition for 'annotation' to be added to element 
                    pugi::xml_node annotationNode = rootElementNode.append_child("xs:annotation");
                    createDocumentationNode(schema, key, annotationNode);
                }
            }

            pugi::xml_node complexTypeNode = rootElementNode.append_child("xs:complexType");

            pugi::xml_node allNode = complexTypeNode.append_child("xs:all");


            BOOST_FOREACH(string name, keys) {

                if (!key.empty()) {
                    name = key + "." + name;
                }

                if (schema.getNodeType(name) == Schema::LEAF && schema.getAssignment(name) != Schema::INTERNAL_PARAM) {

                    leafToXsd(schema, name, allNode);

                } else if (schema.getNodeType(name) == Schema::NODE) {

                    r_createXsd(schema, allNode, false, name);

                } else if (schema.getNodeType(name) == Schema::CHOICE_OF_NODES) {

                    choiceOfNodesToXsd(schema, name, allNode);

                } else if (schema.getNodeType(name) == Schema::LIST_OF_NODES) {

                    listOfNodesToXsd(schema, name, allNode);

                }
            }
        }


        void SchemaXsdSerializer::leafToXsd(const Schema& schema, const string& key, pugi::xml_node& node) const {

            pugi::xml_node simpleElementNode = node.append_child("xs:element");
            appendAttributes(schema, key, simpleElementNode);
            Types::ReferenceType typeOfLeafElem = schema.getValueType(key);

            bool annotation = annotationExists(schema, key);
            if (annotation) {
                pugi::xml_node annotationNode = simpleElementNode.append_child("xs:annotation");
                if (Types::category(typeOfLeafElem) == Types::SEQUENCE) {
                    createDocumentationNode(schema, key, annotationNode, true);
                } else {
                    createDocumentationNode(schema, key, annotationNode);
                }
            }

            if (schema.hasOptions(key)) {
                vector<string> options = schema.getOptions(key);
                pugi::xml_node simpleElem = simpleElementNode.append_child("xs:simpleType");
                pugi::xml_node restrictionElem = simpleElem.append_child("xs:restriction");
                string baseXsdType = Types::to<ToXsd > (schema.getValueType(key));
                restrictionElem.append_attribute("base") = baseXsdType.c_str();


                BOOST_FOREACH(string opt, options) {
                    pugi::xml_node enumerationElem = restrictionElem.append_child("xs:enumeration");
                    enumerationElem.append_attribute("value") = opt.c_str();
                }

            } else if (schema.hasMinInc(key) || schema.hasMinExc(key) || schema.hasMaxInc(key) || schema.hasMaxExc(key)) {
                pugi::xml_node simpleElem = simpleElementNode.append_child("xs:simpleType");
                pugi::xml_node restrictionElem = simpleElem.append_child("xs:restriction");
                string baseXsdType = Types::to<ToXsd > (schema.getValueType(key));
                restrictionElem.append_attribute("base") = baseXsdType.c_str();

                if (schema.hasMinInc(key)) {
                    pugi::xml_node enumerationElem = restrictionElem.append_child("xs:minInclusive");
                    enumerationElem.append_attribute("value") = schema.getMinIncAs<string > (key).c_str();
                } else if (schema.hasMinExc(key)) {
                    pugi::xml_node enumerationElem = restrictionElem.append_child("xs:minExclusive");
                    enumerationElem.append_attribute("value") = schema.getMinExcAs<string > (key).c_str();
                }

                if (schema.hasMaxInc(key)) {
                    pugi::xml_node enumerationElem = restrictionElem.append_child("xs:maxInclusive");
                    enumerationElem.append_attribute("value") = schema.getMaxIncAs<string > (key).c_str();
                } else if (schema.hasMaxExc(key)) {
                    pugi::xml_node enumerationElem = restrictionElem.append_child("xs:maxExclusive");
                    enumerationElem.append_attribute("value") = schema.getMaxExcAs<string > (key).c_str();
                }
            }
        }


        void SchemaXsdSerializer::choiceOfNodesToXsd(const Schema& schema, const string& key, pugi::xml_node& node) const {

            pugi::xml_node choiceElement = node.append_child("xs:element");
            appendAttributes(schema, key, choiceElement);

            bool annotation = annotationExists(schema, key);
            if (annotation) {
                pugi::xml_node annotationNode = choiceElement.append_child("xs:annotation");
                createDocumentationNode(schema, key, annotationNode);
            }

            pugi::xml_node complexTypeElement = choiceElement.append_child("xs:complexType");
            pugi::xml_node choiceTag = complexTypeElement.append_child("xs:choice");

            vector<string> keys = schema.getKeys(key);


            BOOST_FOREACH(string name, keys) {
                string currentKey = key + "." + name;
                if (schema.getNodeType(currentKey) == Schema::NODE) {
                    r_createXsd(schema, choiceTag, false, currentKey);
                } else if (schema.getNodeType(currentKey) == Schema::CHOICE_OF_NODES) {
                    choiceOfNodesToXsd(schema, currentKey, choiceTag);
                } else if (schema.getNodeType(currentKey) == Schema::LIST_OF_NODES) {
                    listOfNodesToXsd(schema, currentKey, choiceTag);
                }
            }
        }


        void SchemaXsdSerializer::listOfNodesToXsd(const Schema& schema, const string& key, pugi::xml_node& node) const {

            pugi::xml_node sequenceElement = node.append_child("xs:element");
            appendAttributes(schema, key, sequenceElement);

            bool annotation = annotationExists(schema, key);
            if (annotation) {
                pugi::xml_node annotationNode = sequenceElement.append_child("xs:annotation");
                createDocumentationNode(schema, key, annotationNode);
            }

            pugi::xml_node complexTypeElement = sequenceElement.append_child("xs:complexType");
            pugi::xml_node sequenceTag = complexTypeElement.append_child("xs:sequence");

            vector<string> keys = schema.getKeys(key);


            BOOST_FOREACH(string name, keys) {
                string currentKey = key + "." + name;
                if (schema.getNodeType(currentKey) == Schema::NODE) {
                    r_createXsd(schema, sequenceTag, true, currentKey);
                } else if (schema.getNodeType(currentKey) == Schema::CHOICE_OF_NODES) {
                    choiceOfNodesToXsd(schema, currentKey, sequenceTag);
                } else if (schema.getNodeType(currentKey) == Schema::LIST_OF_NODES) {
                    listOfNodesToXsd(schema, currentKey, sequenceTag);
                }
            }
        }


        void SchemaXsdSerializer::appendAttributes(const Schema& schema, const string& key, pugi::xml_node& node) const {
            //name
            string lastKey = extractKey(key);
            node.append_attribute("name") = lastKey.c_str();

            //type
            if ((schema.getNodeType(key) == Schema::LEAF) && !schema.hasOptions(key) &&
                !schema.hasMinInc(key) && !schema.hasMinExc(key) && !schema.hasMaxInc(key) && !schema.hasMaxExc(key)) {
                string xsdType = Types::to<ToXsd > (schema.getValueType(key));
                node.append_attribute("type") = xsdType.c_str();
            }

            //default
            if ((schema.getNodeType(key) == Schema::LEAF) && schema.hasDefaultValue(key)) {
                string defaultValue = schema.getDefaultValueAs<string>(key);
                node.append_attribute("default") = defaultValue.c_str();
            }

            //assignment -> minOccurs, maxOccurs
            if (schema.getAssignment(key) == Schema::OPTIONAL_PARAM) {
                node.append_attribute("minOccurs") = "0";
                node.append_attribute("maxOccurs") = "1";
            } else if (schema.getAssignment(key) == Schema::MANDATORY_PARAM) {
                node.append_attribute("minOccurs") = "1";
                node.append_attribute("maxOccurs") = "1";
            }

        }


        void SchemaXsdSerializer::createDocumentationNode(const Schema& schema, const string& key, pugi::xml_node& annotationNode, const bool isVector) const {
            pugi::xml_node documentationNode = annotationNode.append_child("xs:documentation");

            if (schema.hasDescription(key)) {
                string description = schema.getDescription(key);
                pugi::xml_node descriptionElem = documentationNode.append_child("a:description");
                descriptionElem.append_child(pugi::node_pcdata).set_value(description.c_str());
            }

            if (schema.hasDisplayedName(key)) {
                string displayedName = schema.getDisplayedName(key);
                pugi::xml_node displayedNameElem = documentationNode.append_child("a:displayedName");
                displayedNameElem.append_child(pugi::node_pcdata).set_value(displayedName.c_str());
            }

            if (schema.keyHasAlias(key)) {
                string alias = schema.getAliasAsString(key);
                pugi::xml_node aliasElem = documentationNode.append_child("a:alias");
                aliasElem.append_child(pugi::node_pcdata).set_value(alias.c_str());
            }


            int expertLevel = schema.getRequiredAccessLevel(key);
            pugi::xml_node expertLevelElem = documentationNode.append_child("a:requiredAccessLevel");
            expertLevelElem.append_child(pugi::node_pcdata).set_value(toString(expertLevel).c_str());


            if (schema.hasDefaultValue(key)) {
                string defaultValue = schema.getDefaultValueAs<string > (key);
                pugi::xml_node defaultValueElem = documentationNode.append_child("a:default");
                defaultValueElem.append_child(pugi::node_pcdata).set_value(defaultValue.c_str());
            }

            if (schema.hasAccessMode(key)) {
                int accessMode = schema.getAccessMode(key);
                pugi::xml_node accessModeElem = documentationNode.append_child("a:accessType");
                accessModeElem.append_child(pugi::node_pcdata).set_value(toString(accessMode).c_str());
            }

            if (schema.hasDisplayType(key)) {
                string displayType = schema.getDisplayType(key);
                pugi::xml_node displayTypeElem = documentationNode.append_child("a:displayType");
                displayTypeElem.append_child(pugi::node_pcdata).set_value(displayType.c_str());
            }

            if (schema.hasAllowedStates(key)) {
                vector<string> allowedStates = schema.getAllowedStates(key);
                pugi::xml_node allowedStatesElem = documentationNode.append_child("a:allowedStates");
                allowedStatesElem.append_child(pugi::node_pcdata).set_value(toString(allowedStates).c_str());
            }

            if (schema.hasTags(key)) {
                vector<string> tags = schema.getTags(key);
                pugi::xml_node tagsElem = documentationNode.append_child("a:tags");
                tagsElem.append_child(pugi::node_pcdata).set_value(toString(tags).c_str());
            }

            if (schema.hasUnit(key)) {
                string unitName = schema.getUnitName(key);
                string unitSymbol = schema.getUnitSymbol(key);

                pugi::xml_node unitNameElem = documentationNode.append_child("a:unitName");
                unitNameElem.append_child(pugi::node_pcdata).set_value(unitName.c_str());

                pugi::xml_node unitSymbolElem = documentationNode.append_child("a:unitSymbol");
                unitSymbolElem.append_child(pugi::node_pcdata).set_value(unitSymbol.c_str());
            }

            if (schema.hasMetricPrefix(key)) {
                string prefixName = schema.getMetricPrefixName(key);
                string prefixSymbol = schema.getMetricPrefixSymbol(key);

                pugi::xml_node prefixNameElem = documentationNode.append_child("a:metricPrefixName");
                prefixNameElem.append_child(pugi::node_pcdata).set_value(prefixName.c_str());

                pugi::xml_node prefixSymbolElem = documentationNode.append_child("a:metricPrefixSymbol");
                prefixSymbolElem.append_child(pugi::node_pcdata).set_value(prefixSymbol.c_str());
            }

            if (schema.hasMin(key)) { //relevant for LIST element
                int minNumNodes = schema.getMin(key);
                pugi::xml_node minNumNodesElem = documentationNode.append_child("a:min");
                minNumNodesElem.append_child(pugi::node_pcdata).set_value(toString(minNumNodes).c_str());
            }

            if (schema.hasMax(key)) { //relevant for LIST element
                int maxNumNodes = schema.getMax(key);
                pugi::xml_node maxNumNodesElem = documentationNode.append_child("a:max");
                maxNumNodesElem.append_child(pugi::node_pcdata).set_value(toString(maxNumNodes).c_str());
            }

            if (schema.isAccessReadOnly(key)) {//if element 'readOnly', check for Warn and Alarm

                if (schema.hasWarnLow(key)) {
                    string warnLow = schema.getWarnLowAs<string>(key);

                    pugi::xml_node warnLowElem = documentationNode.append_child("a:warnLow");
                    warnLowElem.append_child(pugi::node_pcdata).set_value(warnLow.c_str());
                }

                if (schema.hasWarnHigh(key)) {
                    string warnHigh = schema.getWarnHighAs<string>(key);

                    pugi::xml_node warnHighElem = documentationNode.append_child("a:warnHigh");
                    warnHighElem.append_child(pugi::node_pcdata).set_value(warnHigh.c_str());
                }

                if (schema.hasAlarmLow(key)) {
                    string alarmLow = schema.getAlarmLowAs<string>(key);

                    pugi::xml_node alarmLowElem = documentationNode.append_child("a:alarmLow");
                    alarmLowElem.append_child(pugi::node_pcdata).set_value(alarmLow.c_str());
                }

                if (schema.hasAlarmHigh(key)) {
                    string alarmHigh = schema.getAlarmHighAs<string>(key);

                    pugi::xml_node alarmHighElem = documentationNode.append_child("a:alarmHigh");
                    alarmHighElem.append_child(pugi::node_pcdata).set_value(alarmHigh.c_str());
                }

                if (schema.hasArchivePolicy(key)) {
                    int archivePolicy = schema.getArchivePolicy(key);
                    pugi::xml_node archivePolicyElem = documentationNode.append_child("a:archivePolicy");
                    archivePolicyElem.append_child(pugi::node_pcdata).set_value(toString(archivePolicy).c_str());
                }

            }

            if (isVector) {
                string valueTypeStr = Types::to<ToLiteral>(schema.getValueType(key));
                pugi::xml_node displayTypeVect = documentationNode.append_child("a:displayType");
                displayTypeVect.append_child(pugi::node_pcdata).set_value(valueTypeStr.c_str());

                if (schema.hasMinSize(key)) {
                    unsigned int minSize = schema.getMinSize(key);
                    pugi::xml_node minSizeElem = documentationNode.append_child("a:minSize");
                    minSizeElem.append_child(pugi::node_pcdata).set_value(toString(minSize).c_str());
                }
                if (schema.hasMaxSize(key)) {
                    unsigned int maxSize = schema.getMaxSize(key);
                    pugi::xml_node maxSizeElem = documentationNode.append_child("a:maxSize");
                    maxSizeElem.append_child(pugi::node_pcdata).set_value(toString(maxSize).c_str());
                }
            }

        }


        bool SchemaXsdSerializer::annotationExists(const Schema& schema, const string& key) const {

            if (schema.hasDescription(key) || schema.hasDisplayedName(key) ||
                schema.hasDefaultValue(key) || schema.hasUnit(key) || schema.hasAccessMode(key) ||
                schema.hasDisplayType(key) || schema.hasAllowedStates(key) || schema.hasTags(key) ||
                schema.hasMin(key) || schema.hasMax(key)) {
                return true;
            } else {
                return false;
            }
        }


        string SchemaXsdSerializer::extractKey(const string& key) const {
            string newKey;
            if (key.rfind(".") != string::npos) {
                vector<string> tokens;
                boost::split(tokens, key, boost::is_any_of("."));
                int i = tokens.size() - 1;
                newKey = tokens[i];
            } else {
                newKey = key;
            }
            return newKey;
        }

    } // namespace io
} // namespace karabo
