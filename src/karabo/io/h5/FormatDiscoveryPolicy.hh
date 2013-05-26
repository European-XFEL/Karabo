/*
 * $Id: FormatDiscoveryPolicy.hh 9598 2013-05-05 10:52:42Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH
#define	KARABO_IO_H5_FORMATDISCOVERYPOLICY_HH

#include <string>
#include <karabo/util/Configurator.hh>


namespace karabo {

    namespace io {

        namespace h5 {

            class FormatDiscoveryPolicy {

            public:

                KARABO_CLASSINFO(FormatDiscoveryPolicy, "Policy", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS


                static void expectedParameters(karabo::util::Schema& expected);

                FormatDiscoveryPolicy(const karabo::util::Hash& input);

                virtual ~FormatDiscoveryPolicy() {
                }

                virtual void discover() {
                }

                int getDefaultCompressionLevel() const {
                    return m_defaultCompressionLevel;
                }

                unsigned long long getDefaultChunkSize() const {
                    return m_defaultChunkSize;
                }

            private:
                int m_defaultCompressionLevel;
                unsigned long long m_defaultChunkSize;

            };
//
//            template<class Derived>
//            class FormatDiscovery {
//
//                FormatDiscovery *m_policy;
//            public:
//
//                FormatDiscovery() : m_policy(this) {
//                }
//
//                virtual void run() = 0;
//
//
//
//            };
//
//            
//            class DiscoverFromHash : public FormatDiscovery<DiscoverFromHash> {
//
//                const karabo::util::Hash& m_data;
//  
//            public:
//
//                DiscoverFromHash(const karabo::util::Hash& data)
//                : FormatDiscovery<DiscoverFromHash>(), m_data(data) {
//                }
//
//                DiscoverFromHash& useCompressionLevel(int level) {
//                    m_compressionLevel = level;
//                }            
//
//                void run() {
//
//                }
//
//            private:
//                int m_compressionLevel;
//
//                //                void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy, karabo::util::Hash& config);
//                //
//                //                void discoverFromHash(const karabo::util::Hash& data, FormatDiscoveryPolicy::ConstPointer policy,
//                //                                      std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);
//
//            private:
//                //                void discoverFromHashElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
//                //                                             std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);
//                //
//                //                void discoverFromVectorOfHashesElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
//                //                                                       std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);
//                //
//                //                void discoverFromDataElement(const karabo::util::Hash::Node& el, FormatDiscoveryPolicy::ConstPointer policy,
//                //                                             std::vector<karabo::util::Hash>& config, const std::string& path, const std::string& keyPath);
//                //
//                //                void discoverAttributes(const karabo::util::Hash::Node& el, karabo::util::Hash& config);
//                //
//                //                template< class T>
//                //                void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
//                //                    std::vector<unsigned long long> dims;
//                //                    if (el.hasAttribute("dims")) {
//                //                        dims = el.getAttribute<std::vector<unsigned long long> >("dims");
//                //                    } else {
//                //                        const std::vector<T>& vec = el.getValue< std::vector<T> >();
//                //                        dims.push_back(vec.size());
//                //                    }
//                //                    h.set("dims", dims);
//                //                }
//                //
//                //                template< class T>
//                //                void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Attributes::Node el) {
//                //                    const std::vector<T>& vec = el.getValue< std::vector<T> >();
//                //                    unsigned long long size = vec.size();
//                //                    h.set("dims", std::vector<unsigned long long>(1, size));
//                //                }
//                //
//                //                template< class T>
//                //                void discoverPtrSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
//                //                    const std::vector<unsigned long long>& dims = el.getAttribute<std::vector<unsigned long long> >("dims");
//                //                    h.set("dims", dims);
//                //                }
//                //
//                //
//                //                static const char m_h5Sep = '/';
//
//            };
//
//

        }
    }
}

#endif	
