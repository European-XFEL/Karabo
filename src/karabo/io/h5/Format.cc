/*
 * $Id: Format.cc 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Format.hh"
#include <vector>
#include <karabo/util/ListElement.hh>
#include <karabo/util/ChoiceElement.hh>
#include <karabo/util/SimpleElement.hh>

#include "Element.hh"

#define _TRACER
#undef _TRACER

#ifdef _TRACER
#define dftracer std::clog
#else 
#define dftracer if(1); else std::clog
#endif

// the following macro allows quickly to disable some printouts
#define no_dftracer if(1); else std::clog
#define yes_dftracer std::clog

using namespace std;
using namespace karabo::util;
using namespace karabo::io::h5;

namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::Format)


            void Format::expectedParameters(Schema& expected) {

                LIST_ELEMENT(expected)
                        .key("elements")
                        .displayedName("Elements")
                        .description("Definition of hdf5 objects.")
                        .appendNodesOfConfigurationBase<Element > ()
                        .assignmentOptional().noDefaultValue()
                        .commit();
            }

            Format::Pointer Format::createFormat(const karabo::util::Hash& config) {
                return Format::createNode("Format", "Format", config);
            }

            Format::Pointer Format::createEmptyFormat(){
                Hash config("Format", Hash());
                return Format::createNode("Format", "Format", config );
            }
                        

            Format::Format(const karabo::util::Hash& input) {

                clog << "configure: " << endl << input << endl;
                m_elements = Element::createList("elements", input);
                m_config = Hash("Format", input);
                mapElementsToKeys();
            }

            void Format::discoverFromHash(const Hash& data, Hash& config) {
                Hash& hh = config.bindReference<Hash > ("Format");
                vector<Hash>& vec = hh.bindReference< vector<Hash> >("elements");
                string path = "";
                discoverFromHash(data, vec, path);
                //clog << "after discovery: " << endl << config << endl;


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
                m_mapElements.clear();
                mapElementsToKeys();
            }

            void Format::discoverFromHash(const Hash& data, vector<Hash>& config, const string& path) {

                // This hash does not have attributes
                // It is not rooted. Either top or element of vector<Hash>

                for (Hash::const_iterator it = data.begin(); it != data.end(); ++it) {
                    if (it->is<Hash > ()) {
                        discoverFromHashElement(*it, config, path);
                    } else if (it->is<vector<Hash> >()) {
                        discoverFromVectorOfHashesElement(*it, config, path);
                    } else {
                        discoverFromDataElement(*it, config, path);
                    }
                }
            }

            void Format::discoverFromHashElement(const Hash::Node& el, vector<Hash>& config, const string& path) {

                const Hash& h = el.getValue<Hash > ();
                const std::string& key = el.getKey();
                if (!el.getAttributes().empty()) {
                    config.push_back(Hash());
                    Hash& hcGroup = config.back();
                    Hash& hc = hcGroup.bindReference<Hash > ("Group");
                    hc.set("h5name", key);
                    hc.set("h5path", path);
                    hc.set("type", "HASH");
                    discoverAttributes(el, hc);
                }
                dftracer << "HashElement: " << endl << config.back() << endl;

                std::string newPath;
                if (path != "") {
                    newPath = path + m_h5Sep + key;
                } else {
                    newPath = key;
                }
                for (Hash::const_iterator it = h.begin(); it != h.end(); ++it) {
                    if (it->is<Hash > ()) {
                        discoverFromHashElement(*it, config, newPath);
                    } else if (it->is<vector<Hash> >()) {
                        discoverFromVectorOfHashesElement(*it, config, newPath);
                    } else {
                        discoverFromDataElement(*it, config, newPath);
                    }
                }
            }

            void Format::discoverFromVectorOfHashesElement(const Hash::Node& el, vector<Hash>& config, const string& path) {

                const vector<Hash>& vec = el.getValue<vector<Hash> >();

                const std::string& key = el.getKey();

                no_dftracer << "vector of hashes" << endl;
                config.push_back(Hash());
                Hash& hcGroup = config.back();
                Hash& hc = hcGroup.bindReference<Hash > ("Group");

                hc.set("h5name", key);
                hc.set("h5path", path);
                hc.set("type", "VECTOR_HASH");
                hc.set("size", static_cast<unsigned long long> (vec.size()));
                discoverAttributes(el, hc);

                for (size_t i = 0; i < vec.size(); ++i) {

                    config.push_back(Hash());
                    Hash& h = config.back();
                    Hash& hc = h.bindReference<Hash > ("Group");

                    ostringstream oss1;
                    oss1 << "[" << i << "]";

                    hc.set("h5name", oss1.str());


                    std::string newPath = path + m_h5Sep + key;
                    hc.set("h5path", newPath);
                    hc.set("type", "HASH");
                    discoverAttributes(el, hc);

                    ostringstream oss2;
                    oss2 << path << m_h5Sep << key << m_h5Sep << "[" << i << "]";

                    discoverFromHash(vec[i], config, oss2.str());
                }

            }


            #define _KARABO_IO_H5_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  Format::discoverVectorSize<cppType>(h,el); break;            
            #define _KARABO_IO_H5_SEQUENCE_PTR_SIZE(T,cppType) case Types::PTR_##T:  Format::discoverPtrSize<cppType>(h,el); break;      

            void Format::discoverFromDataElement(const Hash::Node& el, vector<Hash>& config, const string& path) {

                Types::ReferenceType t = el.getType();
                const std::string& key = el.getKey();
                config.push_back(Hash());
                Hash& hc = config.back();
                Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(t));
                h.set("h5name", key);
                h.set("h5path", path);
                //                if (t != Types::UNKNOWN) {
                h.set("type", ToType<ToLiteral>::to(t));
                //                } else {
                //                    
                //                }
                dftracer << "Format::discoverFromDataElement type: " << h.get<string > ("type") << endl;

                if (Types::category(t) == Types::SEQUENCE) {
                    dftracer << "SEQUENCE: " << key << endl;
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

            }

            #undef _KARABO_IO_H5_SEQUENCE_SIZE


            #define _KARABO_IO_H5_ATTRIBUTE_SEQUENCE_SIZE(T,cppType) case Types::VECTOR_##T:  Format::discoverVectorSize<cppType>(h,(*it)); break;                        

            void Format::discoverAttributes(const Hash::Node& el, Hash& config) {
                const Hash::Attributes& attr = el.getAttributes();
                if (attr.empty()) return;

                dftracer << el.getKey() << " has some attributes" << endl;

                const std::string& key = el.getKey();
                vector<Hash>& config_attr = config.bindReference<vector<Hash> > ("attributes");


                for (Hash::Attributes::const_iterator it = attr.begin(); it != attr.end(); ++it) {
                    no_dftracer << "attr key: " << it->getKey() << endl;

                    config_attr.push_back(Hash());
                    Hash& hc = config_attr.back();
                    Hash& h = hc.bindReference<Hash > (ToType<ToLiteral>::to(it->getType()));

                    h.set("type", ToType<ToLiteral>::to(it->getType()));
                    h.set("h5name", it->getKey());
                    Types::ReferenceType t = it->getType();
                    if (Types::category(t) == Types::SEQUENCE) {
                        dftracer << "SEQUENCE: " << key << endl;
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


            #undef _TRACER
            #undef dftracer
            #undef no_dftracer
            #undef yes_dftracer            


        }
    }
}

