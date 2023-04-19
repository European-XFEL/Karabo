/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_FORMAT_HH
#define KARABO_IO_H5_FORMAT_HH

#include <karabo/util/Configurator.hh>
#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <string>
#include <vector>

#include "Element.hh"
#include "FormatDiscoveryPolicy.hh"


namespace karabo {
    namespace io {
        namespace h5 {

            class Table;

            /**
             * @class Format
             * @brief The Format class contains the information necessary to map a Karabo Hash to an HDF5 file
             */
            class Format {
                friend class Table;

               public:
                KARABO_CLASSINFO(Format, "Format", "2.0")
                KARABO_CONFIGURATION_BASE_CLASS

                /**
                 * Create a new Format according to passed input configuration
                 * @param input
                 */
                Format(const karabo::util::Hash& input);

                virtual ~Format() {}

                /**
                 * Expected parameters of the Format class:
                 *
                 * - elements: a LIST_ELEMENT containing the individual elements this format specifies
                 * @param expected
                 */
                static void expectedParameters(karabo::util::Schema& expected);

                /**
                 * Create a format from an input configuration
                 * @param config
                 * @param validate set to true if the configuration should validated against a Schema
                 * @return a pointer to the evaluated format
                 */
                static Format::Pointer createFormat(const karabo::util::Hash& config, bool validate = true);

                /**
                 * Create an empty format
                 * @return a pointer to an empty Format
                 */
                static Format::Pointer createEmptyFormat();

                /**
                 * Discover the format from an input Hash containing data (may be empty) according to discovery policy
                 * @param data
                 * @param discoveryPolicy
                 * @return
                 */
                static Format::Pointer discover(const karabo::util::Hash& data, FormatDiscoveryPolicy::Pointer);

                /**
                 * Discover the format from an input Hash containing data (may be empty)
                 * @param data
                 * @return
                 */
                static Format::Pointer discover(const karabo::util::Hash& data);

                /**
                 * Return the configuration Hash defining this format
                 * @return
                 */
                const karabo::util::Hash& getConfig() const {
                    return m_config;
                }

                /**
                 * Query configuration Hash containing all elements marked as persistent
                 * @param config
                 */
                void getPersistentConfig(karabo::util::Hash& config) const;

                /**
                 * Query a list of element names this format knows about
                 * @param names
                 */
                void getElementsNames(std::vector<std::string>& names) const;

                /**
                 * And an element to this format
                 * @param element
                 */
                void addElement(karabo::io::h5::Element::Pointer element);

                /**
                 * Remove an element from this format
                 * @param fullPath to the element
                 */
                void removeElement(const std::string& fullPath);

                /**
                 * Replace an element in this format
                 * @param fullPath to the element
                 * @param element to replace the existing one with
                 */
                void replaceElement(const std::string& fullPath, karabo::io::h5::Element::Pointer element);

                /**
                 * Return a pointer to the element at a given path in the format
                 * @param fullPath
                 * @return
                 */
                karabo::io::h5::Element::Pointer getElement(const std::string& fullPath);

                /**
                 * Return a const pointer to the element at a given path
                 * @param fullPath
                 * @return
                 */
                karabo::io::h5::Element::ConstPointer getElement(const std::string& fullPath) const;

               private:
                static void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy,
                                             karabo::util::Hash& config);

                static void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy,
                                             std::vector<karabo::util::Hash>& config, const std::string& path,
                                             const std::string& keyPath);

                static void discoverFromHashElement(const karabo::util::Hash::Node& el,
                                                    FormatDiscoveryPolicy::ConstPointer policy,
                                                    std::vector<karabo::util::Hash>& config, const std::string& path,
                                                    const std::string& keyPath);

                static void discoverFromVectorOfHashesElement(const karabo::util::Hash::Node& el,
                                                              FormatDiscoveryPolicy::ConstPointer policy,
                                                              std::vector<karabo::util::Hash>& config,
                                                              const std::string& path, const std::string& keyPath);

                static void discoverFromDataElement(const karabo::util::Hash::Node& el,
                                                    FormatDiscoveryPolicy::ConstPointer policy,
                                                    std::vector<karabo::util::Hash>& config, const std::string& path,
                                                    const std::string& keyPath);

                static void discoverFromNDArray(const karabo::util::Hash::Node& el,
                                                FormatDiscoveryPolicy::ConstPointer policy,
                                                std::vector<karabo::util::Hash>& config, const std::string& path,
                                                const std::string& keyPath);

                static void discoverAttributes(const karabo::util::Hash::Node& el, karabo::util::Hash& config);

                template <class T>
                static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims;
                    if (el.hasAttribute("dims")) {
                        dims = el.getAttributeAs<unsigned long long, std::vector>("dims");
                    } else {
                        const std::vector<T>& vec = el.getValue<std::vector<T> >();
                        dims.push_back(vec.size());
                    }
                    h.set("dims", dims);
                }

                template <class T>
                static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Attributes::Node el) {
                    const std::vector<T>& vec = el.getValue<std::vector<T> >();
                    unsigned long long size = vec.size();
                    h.set("dims", std::vector<unsigned long long>(1, size));
                }

                template <class T>
                static void discoverPtrSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims = el.getAttributeAs<unsigned long long, std::vector>("dims");
                    h.set("dims", dims);
                }

                template <class T>
                static void discoverArraySize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    std::vector<unsigned long long> dims;
                    if (el.hasAttribute("dims")) {
                        dims = el.getAttributeAs<unsigned long long, std::vector>("dims");
                    } else {
                        unsigned long long size = el.getValue<std::pair<const T*, size_t> >().second;
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


        } // namespace h5
    }     // namespace io
} // namespace karabo

// KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::h5::Format, TEMPLATE_IO, DECLSPEC_IO)

#endif
