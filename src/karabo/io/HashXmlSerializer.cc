/* 
 * File:   HashXmlSerializer.cc
 * Author: <burkhard.heisen@xsmail.com>
 *
 * Created on February 21, 2013, 8:42 AM
 *
 */

#include <karabo/util/ToLiteral.hh>

#include "HashXmlSerializer.hh"

using namespace karabo::util;
using namespace karabo::io;

//KARABO_REGISTER_FOR_CONFIGURATION_2(TextSerializer<Hash>, HashXmlSerializer)

//namespace karabo {
//       
//    namespace io {

        
        
        
//        template<> const karabo::util::FactoryMember0<karabo::util::Configurator< TextSerializer<Hash> >, karabo::util::Configurator1< TextSerializer<Hash> , HashXmlSerializer > > 
//        karabo::util::Register0<karabo::util::Configurator< TextSerializer<Hash> >, 
//                karabo::util::Configurator1< TextSerializer<Hash> , HashXmlSerializer > >::registerAs (HashXmlSerializer::classInfo().getClassId());
        
        HashXmlSerializer::HashXmlSerializer(const Hash& hash) {
            hash.get("writeDataTypes", _writeDataTypes);
            hash.get("indentation", _indentation);
            hash.get("writeCompact", _writeCompact);
        }

        void HashXmlSerializer::save(const Hash& object, std::string& archive) {
            pugi::xml_document doc;
            if (object.size() == 1 && object.begin()->getType() == Types::HASH) { // Is rooted
                std::string key = object.begin()->getKey();
                const Hash& value = object.begin()->getValue<Hash>();
                pugi::xml_node node = doc.append_child(key.c_str());
                if (_writeDataTypes) node.append_attribute("_type") = ToType<ToLiteral>::to(Types::HASH).c_str();
                createXml(value, node);
            } else { // No root
                pugi::xml_node node = doc.append_child("root"); // Create fake root element
                node.append_attribute("__artificial") = ""; // Flag it to be fake
                if (_writeDataTypes) node.append_attribute("__type") = ToType<ToLiteral>::to(Types::HASH).c_str();
                createXml(object, node);
            }
            CustomWriter writer(archive);
            if (_writeCompact) doc.save(writer, "", pugi::format_raw);
            else doc.save(writer, _indentation.c_str(), pugi::format_indent);
        }

        void HashXmlSerializer::createXml(const Hash& hash, pugi::xml_node& node) const {
            for (Hash::const_iterator it = hash.begin(); it != hash.end(); ++it) {
                Types::ReferenceType type = it->getType();
                pugi::xml_node nextNode = node.append_child(it->getKey().c_str());
                const Hash::Attributes& attrs = it->getAttributes();
                for (Hash::Attributes::const_iterator jt = attrs.begin(); jt != attrs.end(); ++jt) {
                    Types::ReferenceType attrType = jt->getType();
                    nextNode.append_attribute(jt->getKey().c_str()) = (ToType<ToLiteral>::to(attrType) + ":" + jt->getValueAs<std::string > ()).c_str();
                }
                if (type == Types::HASH) {
                    if (_writeDataTypes) nextNode.append_attribute("_type") = ToType<ToRefString>::to(type).c_str();
                    this->createXml(hash.get<Hash > (it), nextNode);
                } else if (type == Types::VECTOR_HASH) {
                    nextNode.append_attribute("_type") = ToType<ToRefString>::to(type).c_str();
                    const std::vector<Hash>& hashes = hash.get<std::vector<Hash> >(it);
                    for (size_t i = 0; i < hashes.size(); ++i) {
                        pugi::xml_node itemNode = nextNode.append_child("item");
                        this->createXml(hashes[i], itemNode);
                    }
                } else {
                    if (_writeDataTypes) nextNode.append_attribute("_type") = ToType<ToRefString>::to(type).c_str();
                    nextNode.append_child(pugi::node_pcdata).set_value(hash.getAs<std::string > (it).c_str());
                }
            }
        }

        void HashXmlSerializer::load(Hash& object, const std::string& archive) {
            pugi::xml_document doc;
            doc.load(archive.c_str());
            pugi::xml_node node = doc.first_child();
            if (std::string(node.first_attribute().name()) == "_artificial") { // ignore
                this->createHash(object, node.first_child());
            } else {
                this->createHash(object, node);
            }
        }

        void HashXmlSerializer::createHash(Hash& hash, pugi::xml_node node) const {
            while (node.type() != pugi::node_null) {
                Attributes attrs;
                //Types::ReferenceType valueType = Types::UNKNOWN;
                for (pugi::xml_attribute_iterator it = node.attributes_begin(); it != node.attributes_end(); ++it) {
                    std::pair<std::string, Types::ReferenceType> attr = this->decodeXmlAttribute(std::string(it->value()));
                    attrs.set<std::string > (it->name(), attr.first);
                    if (attr.second != Types::UNKNOWN) {
                        attrs.setType(it->name(), attr.second);
                    }
                }
                std::string nodeName(node.name());
                if (node.first_child().type() == pugi::node_element) {
                    hash.set(nodeName, Hash());
                    hash.setAttributes(nodeName, attrs);
                    this->createHash(hash.get<Hash > (nodeName), node.first_child());
                } else if (node.first_child().type() == pugi::node_pcdata) {
                    hash.set<std::string > (nodeName, node.first_child().value());
                    hash.setAttributes(nodeName, attrs);
                }
                node = node.next_sibling();
            }
        }

        std::pair<std::string, Types::ReferenceType> HashXmlSerializer::decodeXmlAttribute(const std::string& xmlAttribute) const {
            size_t pos = xmlAttribute.find_first_of(":");
            Types::ReferenceType type = Types::UNKNOWN;
            std::string value = xmlAttribute;
            if (pos != xmlAttribute.npos) {
                type = FromType<FromRefString>::from(xmlAttribute.substr(0, pos));
                value = xmlAttribute.substr(pos);
            }
            return std::make_pair(value, type);
        }
//    }
//}
