/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_FORMAT_HH
#define	KARABO_IO_H5_FORMAT_HH

#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/ToLiteral.hh>



#include <string>
#include <vector>

#include "Element.hh"
#include "FormatDiscoveryPolicy.hh"


namespace karabo {
    namespace io {
        namespace h5 {

            class Table;

            class Format {

                friend class Table;
            public:

                KARABO_CLASSINFO(Format, "Format", "2.0")
                KARABO_CONFIGURATION_BASE_CLASS

                Format(const karabo::util::Hash& input);

                virtual ~Format() {
                }

                static void expectedParameters(karabo::util::Schema& expected);

                static Format::Pointer createFormat(const karabo::util::Hash& config, bool validate = true);
                
                static Format::Pointer createEmptyFormat();                              

                static Format::Pointer discover(const karabo::util::Hash& data, FormatDiscoveryPolicy::Pointer);

                static Format::Pointer discover(const karabo::util::Hash& data);

                const karabo::util::Hash& getConfig() const {
                    return m_config;
                }

                void getPersistentConfig(karabo::util::Hash& config) const;

                void getElementsNames(std::vector<std::string>& names) const;

                void addElement(karabo::io::h5::Element::Pointer element);

                void removeElement(const std::string& fullPath);

                void replaceElement(const std::string& fullPath, karabo::io::h5::Element::Pointer element);

                karabo::io::h5::Element::Pointer getElement(const std::string& fullPath);

                karabo::io::h5::Element::ConstPointer getElement(const std::string& fullPath) const;

            private:

                static void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy, karabo::util::Hash& config);

                static void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy,
                                             std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);

                static void discoverFromHashElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
                                                    std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);

                static void discoverFromVectorOfHashesElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
                                                              std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);

                static void discoverFromDataElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
                                                    std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);

                static void discoverAttributes(const karabo::util::Hash::Node& el, karabo::util::Hash& config);

                template< class T>
                static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims;
                    if (el.hasAttribute("dims")) {
                        dims = el.getAttributeAs<unsigned long long, std::vector >("dims");
                    } else {
                        const std::vector<T>& vec = el.getValue< std::vector<T> >();
                        dims.push_back(vec.size());
                    }
                    h.set("dims", dims);
                }

                template< class T>
                static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Attributes::Node el) {
                    const std::vector<T>& vec = el.getValue< std::vector<T> >();
                    unsigned long long size = vec.size();
                    h.set("dims", std::vector<unsigned long long>(1, size));
                }

                template< class T>
                static void discoverPtrSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims = el.getAttributeAs<unsigned long long, std::vector >("dims");
                    h.set("dims", dims);
                }
                
                template< class T>
                static void discoverArraySize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims;
                    if (el.hasAttribute("dims")) {
                        dims = el.getAttributeAs<unsigned long long, std::vector >("dims");
                    } else {
                        unsigned long long size = el.getValue< std::pair<const T*, size_t> >().second;
                        dims.push_back(size);
                    }
                    h.set("dims", dims);
                }

                const std::vector<Element::Pointer>& getElements() const {
                    return m_elements;
                }

            private:

                void mapElementsToKeys();
                
                

                static const char m_h5Sep = '/';
                karabo::util::Hash m_config;
                std::vector<karabo::io::h5::Element::Pointer> m_elements;

                std::map<std::string, size_t> m_mapElements;
                
                static karabo::util::Schema m_schema;
                static bool m_schemaExists;
                
            };


        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::h5::Format, TEMPLATE_IO, DECLSPEC_IO)

#endif

