/* 
 * File:   HashXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include <karabo/util/Schema.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/log/Logger.hh>
#include "HashXmlSerializer.hh"

#include <boost/algorithm/string/replace.hpp>

using namespace karabo::util;
using namespace std;

KARABO_EXPLICIT_TEMPLATE(karabo::io::TextSerializer<karabo::util::Hash>)

namespace karabo {
    namespace io {


        KARABO_REGISTER_FOR_CONFIGURATION(TextSerializer<Hash>, HashXmlSerializer)


        void HashXmlSerializer::expectedParameters(karabo::util::Schema& expected) {

            INT32_ELEMENT(expected)
                    .key("indentation")
                    .description("Set the indent characters for printing. Value -1: the most dense formatting without linebreaks. "
                                 "Value 0: no indentation, value 1/2/3: one/two/three space indentation. If not set, default is 2 spaces.")
                    .displayedName("Indentation")
                    .options("-1 0 1 2 3 4")
                    .assignmentOptional().defaultValue(2)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("writeDataTypes")
                    .description("This flag controls whether to add data-type information to the generated XML string")
                    .displayedName("Write data types")
                    .assignmentOptional().defaultValue(true)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("readDataTypes")
                    .description("This flag controls whether to use any potentially existing data type information to do automatic casting into the described types")
                    .displayedName("Read data types")
                    .assignmentOptional().defaultValue(true)
                    .expertAccess()
                    .commit();

            BOOL_ELEMENT(expected)
                    .key("insertXmlNamespace")
                    .displayedName("Insert XML Namespace")
                    .description("Flag toggling whether to insert or not an xmlns attribute")
                    .assignmentOptional().defaultValue(false)
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("xmlns")
                    .description("Sets the default XML namespace")
                    .displayedName("XML Namespace")
                    .assignmentOptional().defaultValue("http://xfel.eu/config")
                    .expertAccess()
                    .commit();

            STRING_ELEMENT(expected)
                    .key("prefix")
                    .displayedName("Prefix")
                    .description("Prefix flagging auxiliary constructs needed for serialization")
                    .assignmentOptional().defaultValue("KRB_")
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
                const Hash& value = object.begin()->getValue<Hash > ();
                pugi::xml_node node = doc.append_child(escapeElementName(key).c_str());

                // Set xml namespace
                if (m_insertXmlNamespace) node.append_attribute("xmlns") = m_xmlns.c_str();
                if (m_writeDataTypes) node.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (Types::HASH).c_str();

                // Set root attributes
                writeAttributes(object.begin()->getAttributes(), node);

                createXml(value, node);
            } else { // No root
                pugi::xml_node node = doc.append_child("root"); // Create fake root element
                node.append_attribute(m_artificialRootFlag.c_str()) = ""; // Flag it to be fake
                if (m_writeDataTypes) node.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (Types::HASH).c_str();
                createXml(object, node);
            }
            CustomWriter writer(archive);
            if (m_writeCompact) doc.save(writer, "", pugi::format_raw);
            else doc.save(writer, m_indentation.c_str(), pugi::format_indent);
        }


        void HashXmlSerializer::writeAttributes(const Hash::Attributes& attrs, pugi::xml_node& node) const {
            for (Hash::Attributes::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
                if (m_writeDataTypes) {
                    Types::ReferenceType attrType = it->getType();
                    node.append_attribute(it->getKey().c_str()) = (m_prefix + Types::to<ToLiteral > (attrType) + ":" + it->getValueAs<string > ()).c_str();
                } else {
                    node.append_attribute(it->getKey().c_str()) = it->getValueAs<string > ().c_str();
                }
            }
        }


        void HashXmlSerializer::createXml(const Hash& hash, pugi::xml_node& node) const {
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                Types::ReferenceType type = it->getType();

                pugi::xml_node nextNode = node.append_child(escapeElementName(it->getKey()).c_str());

                writeAttributes(it->getAttributes(), nextNode);

                if (type == Types::HASH) {
                    if (m_writeDataTypes) nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (type).c_str();
                    this->createXml(it->getValue<Hash > (), nextNode);
                } else if (type == Types::VECTOR_HASH) {
                    if (m_writeDataTypes) nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (type).c_str();
                    const vector<Hash>& hashes = it->getValue<vector<Hash> >();
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        pugi::xml_node itemNode = nextNode.append_child(m_itemFlag.c_str());
                        this->createXml(hashes[i], itemNode);
                    }
                } else if (type == Types::SCHEMA) {
                    TextSerializer<Schema>::Pointer p = TextSerializer<Schema>::create("Xml", Hash("indentation", -1));
                    std::string schema;
                    p->save(it->getValue<Schema>(), schema);
                    if (m_writeDataTypes) nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (type).c_str();
                    nextNode.append_child(pugi::node_pcdata).set_value(schema.c_str());
                } else {
                    if (m_writeDataTypes) nextNode.append_attribute(m_typeFlag.c_str()) = Types::to<ToLiteral > (type).c_str();
                    nextNode.append_child(pugi::node_pcdata).set_value(it->getValueAs<string > ().c_str());
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
                KARABO_LOG_FRAMEWORK_ERROR << KARABO_IO_EXCEPTION(std::string("Error parsing XML document: ") + result.description());
                KARABO_LOG_FRAMEWORK_INFO << "Responsible string:\n" <<  std::string (archive);
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


        void HashXmlSerializer::readAttributes(Hash::Attributes& attrs, const pugi::xml_node& node) const {
            for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); ++it) {
                string attributeName(it->name());
                if (attributeName.substr(0, m_prefix.size()) != m_prefix) {
                    std::pair<std::string, Types::ReferenceType> attr = this->readXmlAttribute(std::string(it->value()));
                    Hash::Attributes::Node& attrNode = attrs.set<std::string > (it->name(), attr.first); // Sets as string
                    if (attr.second != Types::UNKNOWN && m_readDataTypes) {
                        attrNode.setType(attr.second); // Shapes it into correct type
                    }
                }
            }
        }


        std::pair<std::string, Types::ReferenceType> HashXmlSerializer::readXmlAttribute(const std::string& attributeValue) const {
            if ((attributeValue.substr(0, m_prefix.size())) == m_prefix) { // Attribute value with type
                size_t pos = attributeValue.find_first_of(":");
                Types::ReferenceType type = Types::UNKNOWN;
                if (pos != attributeValue.npos) {
                    try {
                        type = Types::from<FromLiteral > (attributeValue.substr(m_prefix.size(), pos - m_prefix.size()));
                    } catch (const karabo::util::Exception& e) {
                        KARABO_LOG_FRAMEWORK_WARN << "Could not understand xml attribute type: \"" << attributeValue.substr(m_prefix.size(), pos - m_prefix.size()) << "\". Will interprete type as string.";
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

                Hash::Attributes attrs;
                readAttributes(attrs, node);

                string nodeName(unescapeElementName(node.name()));

                if (node.first_child().type() == pugi::node_element) {
                    if (node.first_child().name() == m_itemFlag) { // This node describes a vector of Hashes
                        vector<Hash>& tmp = hash.bindReference<vector<Hash> >(nodeName);
                        pugi::xml_node itemNode = node.first_child();
                        while (string(itemNode.name()) == m_itemFlag) {
                            Hash h;
                            this->createHash(h, itemNode.first_child());
                            tmp.push_back(h);
                            itemNode = itemNode.next_sibling();
                        }
                        hash.setAttributes(nodeName, attrs);
                    } else { // Regular Hash
                        hash.set(nodeName, Hash());
                        hash.setAttributes(nodeName, attrs);
                        this->createHash(hash.get<Hash > (nodeName), node.first_child());
                    }
                } else if (node.first_child().type() == pugi::node_pcdata) {
                    Hash::Node& hashNode = hash.set<std::string > (nodeName, node.first_child().value());
                    if (m_readDataTypes) {
                        pugi::xml_attribute attr = node.attribute(m_typeFlag.c_str());
                        if (!attr.empty()) {
                            string attributeValue(attr.value());
                            // Special case: Schema
                            if (attributeValue == "SCHEMA") {
                                TextSerializer<Schema>::Pointer p = TextSerializer<Schema>::create("Xml", Hash("indentation", -1));
                                Schema s;
                                p->load(s, hashNode.getValue<string>());
                                hashNode.setValue(s);
                            } else {
                                try {
                                    hashNode.setType(Types::from<FromLiteral > (attributeValue));
                                } catch (const karabo::util::Exception& e) {
                                    cout << "WARN: Could not understand xml attribute type: \"" << attributeValue << "\". Will interprete type as string." << endl;
                                    e.clearTrace();
                                }
                            }
                        }
                    }
                    hashNode.setAttributes(attrs);

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
                                    hashNode.setType(Types::from<FromLiteral > (attributeValue));
                                } catch (const karabo::util::Exception& e) {
                                    cout << "WARN: Could not understand xml attribute type: \"" << attributeValue << "\". Will interprete type as string." << endl;
                                    e.clearTrace();
                                }
                            }
                        } else {
                            hash.set(nodeName, string());
                        }
                    } else {
                        hash.set(nodeName, string());
                    }
                    hash.setAttributes(nodeName, attrs);
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
                    objects.swap(tmp[0].get<vector<Hash> >(m_prefix + "Sequence"));
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

    }
}