/*
 * This file is part of Karabo.
 *
 * http://www.karabo.eu
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 *
 * Karabo is free software: you can redistribute it and/or modify it under
 * the terms of the MPL-2 Mozilla Public License.
 *
 * You should have received a copy of the MPL-2 Public License along with
 * Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
 *
 * Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 */
/*
 * File:   HashXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include "HashXmlSerializer.hh"

#include <boost/algorithm/string.hpp>
#include <karabo/log/Logger.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>

using namespace karabo::util;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::TextSerializer<karabo::util::Hash>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::TextSerializer<karabo::util::Hash>, karabo::io::HashXmlSerializer)

namespace karabo {
    namespace io {

        void HashXmlSerializer::expectedParameters(karabo::util::Schema& expected) {
            INT32_ELEMENT(expected)
                  .key("indentation")
                  .description(
                        "Set the indent characters for printing. Value -1: the most dense formatting without "
                        "linebreaks. "
                        "Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is "
                        "2 spaces.")
                  .displayedName("Indentation")
                  .options("-1 0 1 2 3 4")
                  .assignmentOptional()
                  .defaultValue(2)
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("writeDataTypes")
                  .description("This flag controls whether to add data-type information to the generated XML string")
                  .displayedName("Write data types")
                  .assignmentOptional()
                  .defaultValue(true)
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("readDataTypes")
                  .description(
                        "This flag controls whether to use any potentially existing data type information to do "
                        "automatic casting into the described types")
                  .displayedName("Read data types")
                  .assignmentOptional()
                  .defaultValue(true)
                  .expertAccess()
                  .commit();

            BOOL_ELEMENT(expected)
                  .key("insertXmlNamespace")
                  .displayedName("Insert XML Namespace")
                  .description("Flag toggling whether to insert or not an xmlns attribute")
                  .assignmentOptional()
                  .defaultValue(false)
                  .expertAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("xmlns")
                  .description("Sets the default XML namespace")
                  .displayedName("XML Namespace")
                  .assignmentOptional()
                  .defaultValue("http://xfel.eu/config")
                  .expertAccess()
                  .commit();

            STRING_ELEMENT(expected)
                  .key("prefix")
                  .displayedName("Prefix")
                  .description("Prefix flagging auxiliary constructs needed for serialization")
                  .assignmentOptional()
                  .defaultValue("KRB_")
                  .expertAccess()
                  .commit();
        }


        HashXmlSerializer::HashXmlSerializer(const Hash& input) {
            input.get("writeDataTypes", m_writeDataTypes);
            input.get("readDataTypes", m_readDataTypes);
            input.get("insertXmlNamespace", m_insertXmlNamespace);
            input.get("xmlns", m_xmlns);
            input.get("prefix", m_prefix);

            m_typeFlag = m_prefix + "Type";
            m_artificialRootFlag = m_prefix + "Artificial";
            m_itemFlag = m_prefix + "Item";

            int indentation = input.get<int>("indentation");
            if (indentation == -1) {
                m_indentation = "";
                m_writeCompact = true;
            } else {
                m_indentation = string(indentation, ' ');
                m_writeCompact = false;
            }
        }


        void HashXmlSerializer::save(const Hash& object, std::string& archive) {
            pugi::xml_document doc;
            if (object.size() == 1 && object.begin()->getType() == Types::HASH) { // Is rooted
                std::string key = object.begin()->getKey();
                const Hash& value = object.begin()->getValue<Hash>();
                pugi::xml_node node = doc.append_child(escapeElementName(key).c_str());

                // Set xml namespace
                if (m_insertXmlNamespace) node.append_attribute("xmlns") = m_xmlns.c_str();
                if (m_writeDataTypes)
                    node.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(Types::HASH).c_str();

                // Set root attributes
                writeAttributes(object.begin()->getAttributes(), node);

                createXml(value, node);
            } else {                                                      // No root
                pugi::xml_node node = doc.append_child("root");           // Create fake root element
                node.append_attribute(m_artificialRootFlag.c_str()) = ""; // Flag it to be fake
                if (m_writeDataTypes)
                    node.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(Types::HASH).c_str();
                createXml(object, node);
            }
            CustomWriter writer(archive);
            if (m_writeCompact) doc.save(writer, "", pugi::format_raw);
            else doc.save(writer, m_indentation.c_str(), pugi::format_indent);
        }


        void HashXmlSerializer::writeAttributes(const Hash::Attributes& attrs, pugi::xml_node& node) const {
            for (Hash::Attributes::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
                Types::ReferenceType attrType = it->getType();
                if (attrType == Types::VECTOR_HASH || attrType == Types::SCHEMA) {
                    // Attributes that are vector<Hash> or Schema are serialized as children of the node that holds the
                    // attribute. The name of the serialized attribute node is the path of the node that contains
                    // the attribute plus the attribute name.
                    std::string attrPath = "_attr" + node.path('_') + '_' + it->getKey();
                    if (m_writeDataTypes) {
                        node.append_attribute(it->getKey().c_str()) =
                              (m_prefix + Types::to<ToLiteral>(attrType) + ":" + attrPath).c_str();
                        pugi::xml_node attrSerialNode = node.append_child(attrPath.c_str());
                        if (attrType == Types::VECTOR_HASH) {
                            Hash hashVectHash(attrPath + "_value", it->getValue<vector<Hash>>());
                            createXml(hashVectHash, attrSerialNode);
                        } else if (attrType == Types::SCHEMA) {
                            Hash hashSchema(attrPath + "_value", it->getValue<Schema>());
                            createXml(hashSchema, attrSerialNode);
                        }
                    }
                } else {
                    if (m_writeDataTypes) {
                        node.append_attribute(it->getKey().c_str()) =
                              (m_prefix + Types::to<ToLiteral>(attrType) + ":" + it->getValueAs<string>()).c_str();
                    } else {
                        node.append_attribute(it->getKey().c_str()) = it->getValueAs<string>().c_str();
                    }
                }
            }
        }


        void HashXmlSerializer::createXml(const Hash& hash, pugi::xml_node& node) const {
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                Types::ReferenceType type = it->getType();

                pugi::xml_node nextNode = node.append_child(escapeElementName(it->getKey()).c_str());

                /*
                 * Note:
                 * Writting the attributes before its parent Hash node is what guarantees proper serialization in
                 * the (unlikely) scenarios where a name clash happens between an Xml node created to hold the
                 * serialized form of a Hash attribute of type vector<Hash> or Schema and an Xml node corresponding to
                 * the actual Hash node.
                 * The deserialization code will always pick the Xml node corresponding to the serialized Hash
                 * attribute, process and remove it from the Xml hierarchy before the node corresponding to the Hash
                 * node is processed. A test for that unlikely scenario can be found on HashXmlSerializer_Test.cc.
                 */
                writeAttributes(it->getAttributes(), nextNode);

                if (type == Types::HASH) {
                    if (m_writeDataTypes)
                        nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(type).c_str();
                    this->createXml(it->getValue<Hash>(), nextNode);
                } else if (type == Types::VECTOR_HASH) {
                    if (m_writeDataTypes)
                        nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(type).c_str();
                    const vector<Hash>& hashes = it->getValue<vector<Hash>>();
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        pugi::xml_node itemNode = nextNode.append_child(m_itemFlag.c_str());
                        this->createXml(hashes[i], itemNode);
                    }
                } else if (type == Types::SCHEMA) {
                    TextSerializer<Schema>::Pointer p = TextSerializer<Schema>::create("Xml", Hash("indentation", -1));
                    std::string schema;
                    p->save(it->getValue<Schema>(), schema);
                    if (m_writeDataTypes)
                        nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(type).c_str();
                    nextNode.append_child(pugi::node_pcdata).set_value(schema.c_str());
                } else {
                    if (m_writeDataTypes)
                        nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral>(type).c_str();
                    nextNode.append_child(pugi::node_pcdata).set_value(it->getValueAs<string>().c_str());
                }
            }
        }


        void HashXmlSerializer::load(Hash& object, const std::string& archive) {
            this->load(object, archive.c_str());
        }


        void HashXmlSerializer::load(karabo::util::Hash& object, const char* archive) {
            pugi::xml_document doc;
            pugi::xml_parse_result result = doc.load(archive);
            if (!result) {
                KARABO_LOG_FRAMEWORK_ERROR
                      << KARABO_IO_EXCEPTION(std::string("Error parsing XML document: ") + result.description());
                KARABO_LOG_FRAMEWORK_INFO << "Responsible string:\n" << (archive ? archive : "");
            }
            object.clear();
            if (!doc) return;
            pugi::xml_node node = doc.first_child();
            if (!node) return;
            if (std::string(node.first_attribute().name()) == m_artificialRootFlag) { // ignore
                this->createHash(object, node.first_child());
            } else {
                node.remove_attribute("xmlns");
                this->createHash(object, node);
            }
        }


        bool HashXmlSerializer::readStrConvertibleAttrs(Hash::Attributes& attrs, const pugi::xml_node& node) const {
            bool allAttrsRead = true;
            for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); ++it) {
                string attributeName(it->name());
                if (attributeName.substr(0, m_prefix.size()) != m_prefix) {
                    std::pair<std::string, Types::ReferenceType> attr =
                          this->readXmlAttribute(std::string(it->value()));
                    if (attr.second == Types::VECTOR_HASH || attr.second == Types::SCHEMA) {
                        // Special cases: An attribute of types VECTOR_HASH or SCHEMA is serialized as a child node of
                        // the node containing the attribute. Reason: they cannot be initialized from a string form and
                        // then have their type set to either VECTOR_CHAR or SCHEMA because there's no conversion
                        // from string available.
                        allAttrsRead = false;
                    } else {
                        // Sets as string -- can move since `attr.first` not used anymore before it goes out of scope
                        Hash::Attributes::Node& attrNode = attrs.set(it->name(), std::move(attr.first));
                        if (attr.second != Types::UNKNOWN && m_readDataTypes) {
                            // Shapes it into correct type
                            attrNode.setType(attr.second);
                        }
                    }
                }
            }
            return allAttrsRead;
        }


        void HashXmlSerializer::extractNonStrConvertibleAttrs(vector<Hash>& nonStrAttrs,
                                                              const pugi::xml_node& node) const {
            nonStrAttrs.clear();
            for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); ++it) {
                const string& attributeName = it->name();
                if (attributeName.substr(0, m_prefix.size()) != m_prefix) {
                    std::pair<std::string, Types::ReferenceType> attr =
                          this->readXmlAttribute(std::string(it->value()));

                    if (boost::starts_with(attr.first, "_attr_") && boost::ends_with(attr.first, attributeName) &&
                        (attr.second == Types::VECTOR_HASH || attr.second == Types::SCHEMA)) {
                        // Attributes of types VECTOR_HASH or SCHEMA are serialized as a child node of the node
                        // containing the attribute - they are of the specially handled types and conform to the new
                        // name convention. If they do not conform to the new name convention, the xml content is assu-
                        // med to be in the old format and is processed in the legacy way.
                        const string& attrNodeName = attr.first;
                        pugi::xml_node attrNode = node.child(attrNodeName.c_str());
                        pugi::xml_node attrValueNode = attrNode.child((attrNodeName + "_value").c_str());
                        Hash h;
                        createHash(h, attrValueNode);
                        if (attr.second == Types::VECTOR_HASH) {
                            const vector<Hash>& vh = h.get<vector<Hash>>(attrValueNode.name());
                            // Adds attribute to the output vector of hashes with the non stringfied attributes.
                            nonStrAttrs.push_back(Hash(attributeName, vh));
                        } else if (attr.second == Types::SCHEMA) {
                            const Schema& sch = h.get<Schema>(attrValueNode.name());
                            // Adds attribute to the output vector of hashes with the non stringfied attributes.
                            nonStrAttrs.push_back(Hash(attributeName, sch));
                        }
                        // Clean-up of auxiliary node used to keep the value of the attribute of type vector of hash.
                        attrNode.remove_child(attrValueNode);
                        pugi::xml_node nodeCpy = node;
                        nodeCpy.remove_child(attrNode);
                    }
                }
            }
        }


        void HashXmlSerializer::addNonStrConvertibleAttrs(karabo::util::Hash& hash, const std::string& hashPath,
                                                          std::vector<karabo::util::Hash>& attrs) const {
            if (hash.has(hashPath)) {
                for (auto& attrHash : attrs) {
                    vector<string> attrHashKeys;
                    attrHash.getKeys(attrHashKeys);
                    if (attrHashKeys.size() == 1) {
                        const string& attrName = attrHashKeys[0];
                        Types::ReferenceType attrType = attrHash.getType(attrName);
                        switch (attrType) {
                            case Types::VECTOR_HASH:
                                hash.setAttribute(hashPath, attrName, std::move(attrHash.get<vector<Hash>>(attrName)));
                                break;
                            case Types::SCHEMA:
                                hash.setAttribute(hashPath, attrName, std::move(attrHash.get<Schema>(attrName)));
                                break;
                            default:
                                KARABO_LOG_FRAMEWORK_ERROR << "Unsupported type for attribute '" << attrName << "'.\n"
                                                           << "Supported types are VECTOR_HASH and SCHEMA.";
                        }
                    } else {
                        // This will only be reached if any change in HashXmlSerializer::extractNonStrConvertibleAttrs
                        // has changed the way it outputs the non string convertible attributes it finds.
                        // To avoid silent failures, a message is being logged.
                        KARABO_LOG_FRAMEWORK_ERROR
                              << "Logic error: HashXmlSerializer::extractNonStrConvertibleAttrs produced a hash with "
                              << attrHashKeys.size() << "key(s) for an attribute at path '" << hashPath
                              << "' of the hash "
                                 "being deserialized.";
                    }
                }
            } else {
                KARABO_LOG_FRAMEWORK_ERROR << "No path '" << hashPath
                                           << "' found in the hash. No attribute will be added.";
            }
        }


        std::pair<std::string, Types::ReferenceType> HashXmlSerializer::readXmlAttribute(
              const std::string& attributeValue) const {
            if ((attributeValue.substr(0, m_prefix.size())) == m_prefix) { // Attribute value with type
                size_t pos = attributeValue.find_first_of(":");
                Types::ReferenceType type = Types::UNKNOWN;
                if (pos != attributeValue.npos) {
                    try {
                        type = Types::from<FromLiteral>(attributeValue.substr(m_prefix.size(), pos - m_prefix.size()));
                    } catch (const karabo::util::Exception& e) {
                        KARABO_LOG_FRAMEWORK_WARN << "Could not understand xml attribute type: \""
                                                  << attributeValue.substr(m_prefix.size(), pos - m_prefix.size())
                                                  << "\". Will interprete type as string.";
                        KARABO_LOG_FRAMEWORK_DEBUG << "Failure details: " << e.detailedMsg();
                        type = Types::UNKNOWN;
                        e.clearTrace();
                    }
                    string value = attributeValue.substr(pos + 1);
                    return std::make_pair(value, type);
                } else {
                    throw KARABO_IO_EXCEPTION("Encountered suspicious attribute type assignment");
                }
            } else {
                return std::make_pair(attributeValue, Types::UNKNOWN);
            }
        }


        void HashXmlSerializer::createHash(Hash& hash, pugi::xml_node node) const {
            while (node.type() != pugi::node_null) {
                string nodeName(unescapeElementName(node.name()));

                Hash::Attributes attrs;
                bool allAttrsRead = readStrConvertibleAttrs(attrs, node);
                vector<Hash> nonStrAttrs;
                if (!allAttrsRead) {
                    // There are attributes in the xml node that are not directly convertible from their string
                    // representation - must extract them and later add them directy to the Hash node.
                    extractNonStrConvertibleAttrs(nonStrAttrs, node);
                }

                bool readyForAttrs = true;
                if (node.first_child().type() == pugi::node_element) {
                    if (node.first_child().name() == m_itemFlag) { // This node describes a vector of Hashes
                        vector<Hash>& tmp = hash.bindReference<vector<Hash>>(nodeName);
                        pugi::xml_node itemNode = node.first_child();
                        while (string(itemNode.name()) == m_itemFlag) {
                            Hash h;
                            this->createHash(h, itemNode.first_child());
                            tmp.push_back(std::move(h));
                            itemNode = itemNode.next_sibling();
                        }
                    } else { // Regular Hash
                        hash.set(nodeName, Hash());
                        this->createHash(hash.get<Hash>(nodeName), node.first_child());
                    }
                } else if (node.first_child().type() == pugi::node_pcdata) {
                    Hash::Node& hashNode = hash.set<std::string>(nodeName, node.first_child().value());
                    if (m_readDataTypes) {
                        pugi::xml_attribute attr = node.attribute(m_typeFlag.c_str());
                        if (!attr.empty()) {
                            string attributeValue(attr.value());
                            if (attributeValue == "SCHEMA") {
                                // Special case: Schema
                                TextSerializer<Schema>::Pointer p =
                                      TextSerializer<Schema>::create("Xml", Hash("indentation", -1));
                                Schema s;
                                p->load(s, hashNode.getValue<string>());
                                hashNode.setValue(std::move(s));
                            } else {
                                try {
                                    hashNode.setType(Types::from<FromLiteral>(attributeValue));
                                } catch (const karabo::util::Exception& e) {
                                    cout << "WARN: Could not understand xml attribute type: \"" << attributeValue
                                         << "\". Will interprete type as string." << endl;
                                    e.clearTrace();
                                }
                            }
                        }
                    }
                } else if (node.first_child().type() == pugi::node_null) {
                    if (m_readDataTypes) {
                        pugi::xml_attribute attr = node.attribute(m_typeFlag.c_str());
                        if (!attr.empty()) {
                            string attributeValue(attr.value());
                            // Special case: Schema
                            if (attributeValue == "HASH") hash.set(nodeName, Hash());
                            else if (attributeValue == "SCHEMA") hash.set(nodeName, Schema());
                            else if (attributeValue == "VECTOR_HASH") hash.set(nodeName, vector<Hash>());
                            else {
                                Hash::Node& hashNode = hash.set(nodeName, string());
                                try {
                                    hashNode.setType(Types::from<FromLiteral>(attributeValue));
                                } catch (const karabo::util::Exception& e) {
                                    cout << "WARN: Could not understand xml attribute type: \"" << attributeValue
                                         << "\". Will interprete type as string." << endl;
                                    e.clearTrace();
                                }
                            }
                        } else {
                            hash.set(nodeName, string());
                        }
                    } else {
                        hash.set(nodeName, string());
                    }
                } else {
                    readyForAttrs = false;
                    KARABO_LOG_FRAMEWORK_WARN << "Failed to prepare attributes for '" << nodeName << "'";
                }
                if (readyForAttrs) {
                    hash.setAttributes(nodeName, std::move(attrs));
                    addNonStrConvertibleAttrs(hash, nodeName, nonStrAttrs);
                }

                // Go to next sibling
                node = node.next_sibling();
            }
        }


        void HashXmlSerializer::save(const std::vector<karabo::util::Hash>& objects, std::string& archive) {
            Hash tmp(m_prefix + "Sequence", objects);
            this->save(tmp, archive);
        }


        void HashXmlSerializer::load(std::vector<karabo::util::Hash>& objects, const std::string& archive) {
            vector<Hash> tmp(1);
            this->load(tmp[0], archive);
            if (tmp[0].empty()) {
                objects.swap(tmp);
            } else {
                if (tmp[0].begin()->getKey() == m_prefix + "Sequence") {
                    objects.swap(tmp[0].get<vector<Hash>>(m_prefix + "Sequence"));
                } else {
                    objects.swap(tmp);
                }
            }
        }


        std::string HashXmlSerializer::escapeElementName(const std::string& data) const {
            return boost::algorithm::replace_all_copy(data, "/", ".KRB_SLASH.");
        }


        std::string HashXmlSerializer::unescapeElementName(const std::string& data) const {
            return boost::algorithm::replace_all_copy(data, ".KRB_SLASH.", "/");
        }

    } // namespace io
} // namespace karabo
