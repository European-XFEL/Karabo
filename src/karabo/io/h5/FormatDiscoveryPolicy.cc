/*
 * $Id: FormatDiscoveryPolicy.cc 9598 2013-05-05 10:52:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "FormatDiscoveryPolicy.hh"
#include <karabo/util/SimpleElement.hh>
#include <karabo/log/Logger.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(FormatDiscoveryPolicy)

            void FormatDiscoveryPolicy::expectedParameters(karabo::util::Schema& expected) {

                UINT64_ELEMENT(expected)
                        .key("chunkSize")
                        .displayedName("Default Chunk Size")
                        .description("Default chunk size for discovery")
                        .assignmentOptional().defaultValue(1)
                        .commit();

                UINT32_ELEMENT(expected)
                        .key("compressionLevel")
                        .displayedName("Default compression Level")
                        .description("Default compression Level")
                        .minInc(0)
                        .maxInc(9)
                        .assignmentOptional().defaultValue(0)
                        .commit();
            }


            FormatDiscoveryPolicy::FormatDiscoveryPolicy(const karabo::util::Hash& input) {
                input.get("chunkSize", m_defaultChunkSize);
                m_defaultCompressionLevel = input.getAs<int>("compressionLevel");
            }



            /////////////////////////////////////////////

//
//            void FormatDiscoveryFromHash::discoverFromHash(const Hash& data, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {
//
//                // This hash does not have attributes
//                // It is not rooted. Either top or element of vector<Hash>
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "path: " << path << " keyPath: " << keyPath;
//                for (Hash::const_iterator it = data.begin(); it != data.end(); ++it) {
//                    if (it->is<Hash > ()) {
//                        discoverFromHashElement(*it, policy, config, path, keyPath);
//                    } else if (it->is<vector<Hash> >()) {
//                        discoverFromVectorOfHashesElement(*it, policy, config, path, keyPath);
//                    } else {
//                        discoverFromDataElement(*it, policy, config, path, keyPath);
//                    }
//                }
//            }
//
//
//            void FormatDiscoveryFromHash::discoverFromHashElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {
//
//                const Hash& h = el.getValue<Hash > ();
//                const std::string& key = el.getKey();
//                std::string newPath;
//                if (path != "") {
//                    newPath = path + m_h5Sep + key;
//                } else {
//                    newPath = key;
//                }
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "1 path: " << path << " key: " << key << " newPath: " << newPath;
//
//                std::string newKeyPath;
//                if (keyPath != "") {
//                    newKeyPath = keyPath + m_h5Sep + key;
//                } else {
//                    newKeyPath = key;
//                }
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "2 keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;
//
//
//                if (!el.getAttributes().empty() || h.size() == 0) {
//                    config.push_back(Hash());
//                    Hash& hcGroup = config.back();
//                    Hash& hc = hcGroup.bindReference<Hash > ("Group");
//                    hc.set("h5name", key);
//                    hc.set("h5path", path);
//                    hc.set("key", newKeyPath);
//                    //hc.set("type", "HASH");
//                    discoverAttributes(el, hc);
//
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << "HashElement:\n" << config.back();
//                }
//
//
//
//                for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
//                    if (it->is<Hash > ()) {
//                        discoverFromHashElement(*it, policy, config, newPath, newKeyPath);
//                    } else if (it->is<vector<Hash> >()) {
//                        discoverFromVectorOfHashesElement(*it, policy, config, newPath, newKeyPath);
//                    } else {
//                        discoverFromDataElement(*it, policy, config, newPath, newKeyPath);
//                    }
//                }
//            }
//
//
//            void FormatDiscoveryFromHash::discoverFromVectorOfHashesElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy, vector<Hash>& config, const string& path, const string& keyPath) {
//
//                const vector<Hash>& vec = el.getValue<vector<Hash> >();
//
//                const std::string& key = el.getKey();
//
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "vector of hashes";
//                config.push_back(Hash());
//                Hash& hcGroup = config.back();
//                Hash& hc = hcGroup.bindReference<Hash > ("Group");
//
//                std::string newKeyPath;
//                if (keyPath != "") {
//                    newKeyPath = keyPath + m_h5Sep + key;
//                } else {
//                    newKeyPath = key;
//                }
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "/1/ keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;
//
//                hc.set("h5name", key);
//                hc.set("h5path", path);
//                hc.set("key", newKeyPath);
//                hc.set("type", "VECTOR_HASH");
//                hc.set("size", static_cast<unsigned long long> (vec.size()));
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "/A/ h5name: " << key << " h5path: " << path << " key: " << newKeyPath
//                        << " type: VECTOR_HASH size: " << vec.size();
//                discoverAttributes(el, hc);
//
//                for (size_t i = 0; i < vec.size(); ++i) {
//
//                    config.push_back(Hash());
//                    Hash& h = config.back();
//                    Hash& hc = h.bindReference<Hash > ("Group");
//
//                    ostringstream oss1;
//                    oss1 << "[" << i << "]";
//
//
//                    hc.set("h5name", key + oss1.str());
//
//                    std::string newPath;
//                    if (path != "") {
//                        newPath = path + m_h5Sep + key;
//                    } else {
//                        newPath = key;
//                    }
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << "/2/ path: " << path << " key: " << key << " newPath: " << newPath;
//
//
//
//                    hc.set("h5path", path);
//                    //hc.set("key", newKeyPath);
//                    hc.set("key", newKeyPath + oss1.str());
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << "/B/ h5name: " << oss1.str() << " h5Path: " << newPath
//                            << " key: " << newKeyPath << oss1.str();
//
//                    //hc.set("type", "HASH");
//                    discoverAttributes(el, hc);
//
//                    ostringstream oss2, oss3;
//                    if (path != "") {
//                        oss2 << path << m_h5Sep;
//                    }
//                    oss2 << key << "[" << i << "]";
//                    oss3 << newKeyPath << "[" << i << "]";
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << " before discoverFromHash, path: " << path << " key: " << key;
//                    discoverFromHash(vec[i], policy, config, oss2.str(), oss3.str());
//                }
//
//            }
//
//
//            #define _KARABO_IO_H5_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  FormatDiscoveryFromHash::discoverVectorSize<cppType>(h,el); break;            
//            #define _KARABO_IO_H5_SEQUENCE_PTR_SIZE(T,cppType) case Types::PTR_##T:  FormatDiscoveryFromHash::discoverPtrSize<cppType>(h,el); break;      
//
//
//            void FormatDiscoveryFromHash::discoverFromDataElement(const Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
//                                                 vector<Hash>& config, const string& path, const string& keyPath) {
//
//                Types::ReferenceType t = el.getType();
//                const std::string& key = el.getKey();
//                config.push_back(Hash());
//                Hash& hc = config.back();
//
//
//                std::string newKeyPath;
//                if (keyPath != "") {
//                    newKeyPath = keyPath + m_h5Sep + key;
//                } else {
//                    newKeyPath = key;
//                }
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "keyPath: " << keyPath << " key: " << key << " newKeyPath: " << newKeyPath;
//
//                if (Types::isPointer(t)) {
//                    string ptrType = ToType<ToLiteral>::to(t);
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << ptrType;
//                    string vecType = "VECTOR_" + ptrType.substr(4);
//                    Hash& h = hc.bindReference<Hash > (vecType);
//                    h.set("h5name", key);
//                    h.set("h5path", path);
//                    h.set("key", newKeyPath);
//                    h.set("type", ptrType);
//                    h.set("chunkSize", policy->getDefaultChunkSize());
//                    h.set("compressionLevel", policy->getDefaultCompressionLevel());
//                    if (Types::category(t) == Types::SEQUENCE) {
//                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
//                        switch (t) {
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT32, int)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT32, unsigned int)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(FLOAT, float)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(DOUBLE, double)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT16, short)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT16, unsigned short)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT64, long long)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT64, unsigned long long)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(INT8, signed char)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(UINT8, unsigned char)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(CHAR, char)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(BOOL, bool)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(STRING, std::string)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(COMPLEX_FLOAT, complex<float>)
//                                _KARABO_IO_H5_SEQUENCE_PTR_SIZE(COMPLEX_DOUBLE, complex<double>)
//
//                            default:
//                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
//                        }
//
//                    }
//                    discoverAttributes(el, h);
//
//                } else {
//                    Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(t));
//                    h.set("h5name", key);
//                    h.set("h5path", path);
//                    h.set("key", newKeyPath);
//                    h.set("chunkSize", policy->getDefaultChunkSize());
//                    h.set("compressionLevel", policy->getDefaultCompressionLevel());
//                    if (Types::category(t) == Types::SEQUENCE) {
//                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
//                        switch (t) {
//                                _KARABO_IO_H5_SEQUENCE_SIZE(INT32, int)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT32, unsigned int)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(FLOAT, float)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(DOUBLE, double)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(INT16, short)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT16, unsigned short)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(INT64, long long)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT64, unsigned long long)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(INT8, signed char)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(UINT8, unsigned char)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(CHAR, char)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(BOOL, bool)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(STRING, std::string)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(COMPLEX_FLOAT, complex<float>)
//                                _KARABO_IO_H5_SEQUENCE_SIZE(COMPLEX_DOUBLE, complex<double>)
//
//                            default:
//                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
//                        }
//
//                    }
//                    discoverAttributes(el, h);
//
//                }
//
//                KARABO_LOG_FRAMEWORK_TRACE_CF << "FormatDiscoveryFromHash::discoverFromDataElement type: " << ToType<ToLiteral>::to(t);
//
//
//
//            }
//
//            #undef _KARABO_IO_H5_SEQUENCE_SIZE
//
//
//            #define _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  FormatDiscoveryFromHash::discoverVectorSize<cppType>(h,(*it)); break;                        
//
//
//            void FormatDiscoveryFromHash::discoverAttributes(const Hash::Node& el, Hash& config) {
//                const Hash::Attributes& attr = el.getAttributes();
//                if (attr.empty()) return;
//
//                KARABO_LOG_FRAMEWORK_TRACE_CF << el.getKey() << " has some attributes";
//
//                const std::string& key = el.getKey();
//                vector<Hash>& config_attr = config.bindReference<vector<Hash> > ("attributes");
//
//
//                for (Hash::Attributes::const_iterator it = attr.begin(); it != attr.end(); ++it) {
//                    KARABO_LOG_FRAMEWORK_TRACE_CF << "attr key: " << it->getKey();
//
//                    config_attr.push_back(Hash());
//                    Hash& hc = config_attr.back();
//                    Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(it->getType()));
//
//                    // h.set("type", ToType<ToLiteral>::to(it->getType()));
//                    h.set("h5name", it->getKey());
//                    Types::ReferenceType t = it->getType();
//                    if (Types::category(t) == Types::SEQUENCE) {
//                        KARABO_LOG_FRAMEWORK_TRACE_CF << "SEQUENCE: " << key;
//                        switch (t) {
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT32, int)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT32, unsigned int)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT16, short)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT16, unsigned short)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT64, long long)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT64, unsigned long long)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(INT8, signed char)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(UINT8, unsigned char)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(CHAR, char)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(FLOAT, float)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(DOUBLE, double)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(BOOL, bool)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(STRING, std::string)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(COMPLEX_FLOAT, complex<float>)
//                                _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(COMPLEX_DOUBLE, complex<double>)
//                            default:
//                                throw KARABO_NOT_SUPPORTED_EXCEPTION("Type not supported for key " + key);
//                        }
//
//                    }
//                }
//            }
//
//            #undef _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE
//
//
//
//
//













        }
    }
}
