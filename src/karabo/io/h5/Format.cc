/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Format.hh"
#include <karabo/log/Tracer.hh>
#include <vector>
#include <karabo/util/ListElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/SimpleElement.hh>

#include "Element.hh"
#include "karabo/util/TimeProfiler.hh"
#include <karabo/util/HashFilter.hh>



using namespace std;
using namespace karabo::util;
using namespace karabo::io::h5;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Format)

            karabo::util::Schema Format::m_schema = Schema();
            bool Format::m_schemaExists = false;


            void Format::expectedParameters(Schema& expected) {

                LIST_ELEMENT(expected)
                        .key("elements")
                        .displayedName("Elements")
                        .description("Definition of hdf5 objects.")
                        .appendNodesOfConfigurationBase<Element > ()
                        .assignmentOptional().noDefaultValue()
                        .commit();

            }


            Format::Pointer Format::createFormat(const karabo::util::Hash& config, bool validate) {
                return karabo::util::Configurator<Self>::createNode("Format", "Format", config, false);
            }


            Format::Pointer Format::createEmptyFormat() {
                Hash config("Format.elements", vector<Hash>());
                return Format::createNode("Format", "Format", config);
            }


            Format::Format(const karabo::util::Hash& input) {

                if (!m_schemaExists) {
                    // we do it here as we cannot run it in static initialization function, because not all pluggable 
                    // element classes may be initialized yet.
                    m_schema = Format::getSchema("Format");
                    m_schemaExists = true;
                }
                m_elements = Configurator<Element>::createList("elements", input, false);
                m_config = Hash("Format", input);
                mapElementsToKeys();
            }


            Format::Pointer Format::discover(const karabo::util::Hash& data, FormatDiscoveryPolicy::Pointer policy) {
                Hash config;
                discoverFromHash(data, policy, config);
                return karabo::util::Configurator<Format>::createNode("Format", "Format", config, false);
            }


            Format::Pointer Format::discover(const karabo::util::Hash& data) {
                Hash config;
                FormatDiscoveryPolicy::Pointer policy = FormatDiscoveryPolicy::create("Policy", Hash());
                discoverFromHash(data, policy, config);
                return karabo::util::Configurator<Format>::createNode("Format", "Format", config, false);
            }


            void Format::getPersistentConfig(karabo::util::Hash& config) const {
                Hash& elements = config.bindReference<Hash>("Format");
                HashFilter::byTag(m_schema, m_config.get<Hash>("Format"), elements, "persistent");
            }


            void Format::discoverFromHash(const Hash& data, FormatDiscoveryPolicy::ConstPointer policy, Hash& config) {
                Hash& hh = config.bindReference<Hash > ("Format");
                vector<Hash>& vec = hh.bindReference< vector<Hash> >("elements");
                discoverFromHash(data, policy, vec, "", "");
                KARABO_LOG_FRAMEWORK_TRACE_CF << "after discovery:\n" << config;
            }


            void Format::getElementsNames(std::vector<std::string>& names) const {
                for (std::map<std::string, size_t>::const_iterator it = m_mapElements.begin();
                        it != m_mapElements.end(); ++it) {
                    names.push_back(boost::replace_all_copy(it->first, "/", "."));
                }
            }


            void Format::addElement(karabo::io::h5::Element::Pointer element) {

                m_elements.push_back(element);

                // update config
                vector<Hash>& vec = m_config.get<vector<Hash> >("Format.elements");
                vec.push_back(Hash());
                Hash& last = vec.back();
                Hash& elementConfig = last.bindReference<Hash > (element->getClassInfo().getClassId());
                element->getConfig(elementConfig);
                // update map between h5PathName and index in element;
                m_mapElements[element->getFullName()] = m_elements.size() - 1;
            }


            void Format::mapElementsToKeys() {
                for (size_t i = 0; i < m_elements.size(); ++i) {
                    m_mapElements[m_elements[i]->getFullName()] = i;
                }
            }


            void Format::removeElement(const std::string& fullPath) {
                string fullPathSlash = boost::replace_all_copy(fullPath, ".", "/");
                size_t idx = m_mapElements[fullPathSlash];
                m_elements.erase(m_elements.begin() + idx);

                vector<Hash>& config = m_config.get<vector<Hash> >("Format.elements");
                config.erase(config.begin() + idx);

                m_mapElements.clear();
                mapElementsToKeys();
            }


            void Format::replaceElement(const std::string& fullPath, karabo::io::h5::Element::Pointer element) {
                string fullPathSlash = boost::replace_all_copy(fullPath, ".", "/");
                size_t idx = m_mapElements[fullPathSlash];
                m_elements[idx] = element;

                vector<Hash>& vecConfig = m_config.get<vector<Hash> >("Format.elements");
                Hash newConfig;
                Hash& elementConfig = newConfig.bindReference<Hash > (element->getClassInfo().getClassId());
                element->getConfig(elementConfig);
                vecConfig[idx] = newConfig;

                m_mapElements.clear();
                mapElementsToKeys();
            }


            boost::shared_ptr<karabo::io::h5::Element> Format::getElement(const std::string& fullPath) {
                string fullPathSlash = boost::replace_all_copy(fullPath, ".", "/");
                size_t idx = m_mapElements[fullPathSlash];
                return m_elements[idx];

            }


            boost::shared_ptr<const karabo::io::h5::Element> Format::getElement(const std::string& fullPath) const {
                string fullPathSlash = boost::replace_all_copy(fullPath, ".", "/");
                map<string, size_t>::const_iterator it = m_mapElements.find(fullPathSlash);
                if (it != m_mapElements.end()) {
                    size_t idx = it->second;
                    return m_elements[idx];
                }
                return boost::shared_ptr<const karabo::io::h5::Element>();

            }


            void Format::discoverFromHash(const Hash& data, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {

                // This hash does not have attributes
                // It is not rooted. Either top or element of vector<Hash>
                KARABO_LOG_FRAMEWORK_TRACE_CF << "path: " << path << " keyPath: " << keyPath;
                for (Hash::const_iterator it = data.begin(); it != data.end(); ++it) {
                    if (it->is<Hash > ()) {
                        discoverFromHashElement(*it, policy, config, path, keyPath);
                    } else if (it->is<vector<Hash> >()) {
                        discoverFromVectorOfHashesElement(*it, policy, config, path, keyPath);
                    } else {
                        discoverFromDataElement(*it, policy, config, path, keyPath);
                    }
                }
            }


            void Format::discoverFromHashElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {

                const Hash& h = el.getValue<Hash > ();
                const std::string& key = el.getKey();
                std::string newPath;
                if (path != "") {
                    newPath = path + m_h5Sep + key;
                } else {
                    newPath = key;
                }
                KARABO_LOG_FRAMEWORK_TRACE_CF << "1 path: " << path << " key: " << key << " newPath: " << newPath;

                std::string newKeyPath;
                if (keyPath != "") {
                    newKeyPath = keyPath + m_h5Sep + key;
                } else {
                    newKeyPath = key;
                }
                KARABO_LOG_FRAMEWORK_TRACE_CF << "2 keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;


                if (!el.getAttributes().empty() || h.size() == 0) {
                    config.push_back(Hash());
                    Hash& hcGroup = config.back();
                    Hash& hc = hcGroup.bindReference<Hash > ("Group");
                    hc.set("h5name", key);
                    hc.set("h5path", path);
                    hc.set("key", newKeyPath);
                    //hc.set("type", "HASH");
                    discoverAttributes(el, hc);

                    KARABO_LOG_FRAMEWORK_TRACE_CF << "HashElement:\n" << config.back();
                }



                for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                    if (it->is<Hash > ()) {
                        discoverFromHashElement(*it, policy, config, newPath, newKeyPath);
                    } else if (it->is<vector<Hash> >()) {
                        discoverFromVectorOfHashesElement(*it, policy, config, newPath, newKeyPath);
                    } else {
                        discoverFromDataElement(*it, policy, config, newPath, newKeyPath);
                    }
                }
            }


            void Format::discoverFromVectorOfHashesElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {

                const vector<Hash>& vec = el.getValue<vector<Hash> >();

                const std::string& key = el.getKey();

                KARABO_LOG_FRAMEWORK_TRACE_CF << "vector of hashes";
                config.push_back(Hash());
                Hash& hcGroup = config.back();
                Hash& hc = hcGroup.bindReference<Hash > ("Group");

                std::string newKeyPath;
                if (keyPath != "") {
                    newKeyPath = keyPath + m_h5Sep + key;
                } else {
                    newKeyPath = key;
                }
                KARABO_LOG_FRAMEWORK_TRACE_CF << "/1/ keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;

                hc.set("h5name", key);
                hc.set("h5path", path);
                hc.set("key", newKeyPath);
                hc.set("type", "VECTOR_HASH");
                hc.set("size", static_cast<unsigned long long> (vec.size()));
                KARABO_LOG_FRAMEWORK_TRACE_CF << "/A/ h5name: " << key << " h5path: " << path << " key: " << newKeyPath
                        << " type: VECTOR_HASH size: " << vec.size();
                discoverAttributes(el, hc);

                for (size_t i = 0; i < vec.size(); ++i) {

                    config.push_back(Hash());
                    Hash& h = config.back();
                    Hash& hc = h.bindReference<Hash > ("Group");

                    ostringstream oss1;
                    oss1 << "[" << i << "]";


                    hc.set("h5name", key + oss1.str());

                    std::string newPath;
                    if (path != "") {
                        newPath = path + m_h5Sep + key;
                    } else {
                        newPath = key;
                    }
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "/2/ path: " << path << " key: " << key << " newPath: " << newPath;



                    hc.set("h5path", path);
                    //hc.set("key", newKeyPath);
                    hc.set("key", newKeyPath + oss1.str());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "/B/ h5name: " << oss1.str() << " h5Path: " << newPath
                            << " key: " << newKeyPath << oss1.str();

                    //hc.set("type", "HASH");
                    discoverAttributes(el, hc);

                    ostringstream oss2, oss3;
                    if (path != "") {
                        oss2 << path << m_h5Sep;
                    }
                    oss2 << key << "[" << i << "]";
                    oss3 << newKeyPath << "[" << i << "]";
                    KARABO_LOG_FRAMEWORK_TRACE_CF << " before discoverFromHash, path: " << path << " key: " << key;
                    discoverFromHash(vec[i], policy, config, oss2.str(), oss3.str());
                }

            }


            #define _KARABO_IO_H5_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  Format::discoverVectorSize<cppType>(h,el); break;            
            #define _KARABO_IO_H5_SEQUENCE_PTR_SIZE(T,cppType) case Types::PTR_##T:  Format::discoverPtrSize<cppType>(h,el); break;      
            #define _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(T,cppType) case Types::ARRAY_##T:  Format::discoverArraySize<cppType>(h,el); break;      


            void Format::discoverFromDataElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
                                                 vector<Hash>& config, const string& path, const string& keyPath) {

                Types::ReferenceType t = el.getType();
                const std::string& key = el.getKey();
                config.push_back(Hash());
                Hash& hc = config.back();


                std::string newKeyPath;
                if (keyPath != "") {
                    newKeyPath = keyPath + m_h5Sep + key;
                } else {
                    newKeyPath = key;
                }
                KARABO_LOG_FRAMEWORK_TRACE_CF << "keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;

                if (Types::isPointer(t)) {
                    string ptrType = ToType<ToLiteral>::to(t);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << ptrType;
                    string vecType = "VECTOR_" + ptrType.substr(4);
                    Hash& h = hc.bindReference<Hash > (vecType);
                    h.set("h5name", key);
                    h.set("h5path", path);
                    h.set("key", newKeyPath);
                    h.set("type", ptrType);
                    h.set("chunkSize", policy->getDefaultChunkSize());
                    h.set("compressionLevel", policy->getDefaultCompressionLevel());
                    if (Types::category(t) == Types::SEQUENCE) {
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
                        switch (t) {
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT32, int)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT32, unsigned int)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(FLOAT, float)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(DOUBLE, double)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT16, short)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT16, unsigned short)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT64, long long)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT64, unsigned long long)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT8, signed char)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT8, unsigned char)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(CHAR, char)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(BOOL, bool)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(STRING, std::string)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(COMPLEX_FLOAT, complex<float>)
                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(COMPLEX_DOUBLE, complex<double>)

                            default:
                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
                        }

                    }
                    discoverAttributes(el, h);

                } else if (Types::isRawArray(t)) {
                    string ptrType = ToType<ToLiteral>::to(t);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << ptrType;
                    string vecType = "VECTOR_" + ptrType.substr(6);
                    Hash& h = hc.bindReference<Hash > (vecType);
                    h.set("h5name", key);
                    h.set("h5path", path);
                    h.set("key", newKeyPath);
                    h.set("type", ptrType);
                    h.set("chunkSize", policy->getDefaultChunkSize());
                    h.set("compressionLevel", policy->getDefaultCompressionLevel());
                    switch (t) {
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(INT32, int)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(UINT32, unsigned int)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(FLOAT, float)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(DOUBLE, double)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(INT16, short)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(UINT16, unsigned short)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(INT64, long long)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(UINT64, unsigned long long)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(INT8, signed char)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(UINT8, unsigned char)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(CHAR, char)
                            _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE(BOOL, bool)

                        default:
                            throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
                    }


                    discoverAttributes(el, h);

                } else {
                    Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(t));
                    h.set("h5name", key);
                    h.set("h5path", path);
                    h.set("key", newKeyPath);
                    h.set("chunkSize", policy->getDefaultChunkSize());
                    h.set("compressionLevel", policy->getDefaultCompressionLevel());
                    if (Types::category(t) == Types::SEQUENCE) {
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
                        switch (t) {
                                _KARABO_IO_H5_SEQUENCE_SIZE(INT32, int)
                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT32, unsigned int)
                                _KARABO_IO_H5_SEQUENCE_SIZE(FLOAT, float)
                                _KARABO_IO_H5_SEQUENCE_SIZE(DOUBLE, double)
                                _KARABO_IO_H5_SEQUENCE_SIZE(INT16, short)
                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT16, unsigned short)
                                _KARABO_IO_H5_SEQUENCE_SIZE(INT64, long long)
                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT64, unsigned long long)
                                _KARABO_IO_H5_SEQUENCE_SIZE(INT8, signed char)
                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT8, unsigned char)
                                _KARABO_IO_H5_SEQUENCE_SIZE(CHAR, char)
                                _KARABO_IO_H5_SEQUENCE_SIZE(BOOL, bool)
                                _KARABO_IO_H5_SEQUENCE_SIZE(STRING, std::string)
                                _KARABO_IO_H5_SEQUENCE_SIZE(COMPLEX_FLOAT, complex<float>)
                                _KARABO_IO_H5_SEQUENCE_SIZE(COMPLEX_DOUBLE, complex<double>)

                            default:
                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
                        }

                    }
                    discoverAttributes(el, h);

                }

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Format::discoverFromDataElement type: " << ToType<ToLiteral>::to(t);



            }

            #undef _KARABO_IO_H5_SEQUENCE_SIZE
            #undef _KARABO_IO_H5_SEQUENCE_PTR_SIZE
            #undef _KARABO_IO_H5_SEQUENCE_ARRAY_SIZE


            #define _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  Format::discoverVectorSize<cppType>(h,(*it)); break;                        


            void Format::discoverAttributes(const Hash::Node& el, Hash& config) {
                const Hash::Attributes& attr = el.getAttributes();
                if (attr.empty()) return;

                KARABO_LOG_FRAMEWORK_TRACE_CF << el.getKey() << " has some attributes";

                const std::string& key = el.getKey();
                vector<Hash>& config_attr = config.bindReference<vector<Hash> > ("attributes");


                for (Hash::Attributes::const_iterator it = attr.begin(); it != attr.end(); ++it) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "attr key: " << it->getKey();

                    config_attr.push_back(Hash());
                    Hash& hc = config_attr.back();
                    Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(it->getType()));

                    // h.set("type", ToType<ToLiteral>::to(it->getType()));
                    h.set("h5name", it->getKey());
                    Types::ReferenceType t = it->getType();
                    if (Types::category(t) == Types::SEQUENCE) {
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
                        switch (t) {
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT32, int)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT32, unsigned int)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT16, short)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT16, unsigned short)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT64, long long)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT64, unsigned long long)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT8, signed char)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT8, unsigned char)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(CHAR, char)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(FLOAT, float)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(DOUBLE, double)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(BOOL, bool)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(STRING, std::string)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(COMPLEX_FLOAT, complex<float>)
                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(COMPLEX_DOUBLE, complex<double>)
                            default:
                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
                        }

                    }
                }
            }

            #undef _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE





        }
    }
}

