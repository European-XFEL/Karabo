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


                static Format::Pointer createFormat(const karabo::util::Hash& config);
                
                static Format::Pointer createEmptyFormat();
                
                static void discoverFromHash(const karabo::util::Hash& data, karabo::util::Hash& config);

                const karabo::util::Hash& getConfig() const {
                    return m_config;
                }
                
                void getPersistentConfig(karabo::util::Hash& config) const;

                void addElement(karabo::io::h5::Element::Pointer element);
                
                void removeElement(const std::string& fullPath );
                
                void replaceElement(const std::string& fullPath, karabo::io::h5::Element::Pointer element);
                
                
            private:

                static void discoverFromHash(const karabo::util::Hash& data,
                        std::vector<karabo::util::Hash>& config, const std::string& path);

                static void discoverFromHashElement(const karabo::util::Hash::Node& el,
                        std::vector<karabo::util::Hash>& config, const std::string& path);

                static void discoverFromVectorOfHashesElement(const karabo::util::Hash::Node& el,
                        std::vector<karabo::util::Hash>& config, const std::string& path);

                static void discoverFromDataElement(const karabo::util::Hash::Node& el,
                        std::vector<karabo::util::Hash>& config, const std::string& path);

                static void discoverAttributes(const karabo::util::Hash::Node& el, karabo::util::Hash& config);

                template< class T> static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    
                    std::vector<unsigned long long> dims;
                    if (el.hasAttribute("dims")) {
                        dims = el.getAttribute<std::vector<unsigned long long> >("dims");
                    } else {
                        const std::vector<T>& vec = el.getValue< std::vector<T> >();
                        dims.push_back(vec.size());
                    }
                    h.set("dims", dims);
                }

                template< class T> static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Attributes::Node el) {
                    const std::vector<T>& vec = el.getValue< std::vector<T> >();
                    unsigned long long size = vec.size();
                    h.set("dims", std::vector<unsigned long long>(1, size));
                }

                template< class T> static void discoverPtrSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    const std::vector<unsigned long long>& dims = el.getAttribute<std::vector<unsigned long long> >("dims");
                    unsigned long long size = dims[0];
                    for (size_t i = 1; i < dims.size(); ++i) {
                        size *= dims[i];
                    }
                    h.set("dims", size);
                }


                std::vector<Element::Pointer> getElements() {
                    return m_elements;
                }

                void mapElementsToKeys();
                
            private:
                static const char m_h5Sep = '/';
                karabo::util::Hash m_config;
                std::vector<karabo::io::h5::Element::Pointer> m_elements;

                std::map<std::string, size_t> m_mapElements;
            };


        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::h5::Format, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_H5_FORMAT_HH */

