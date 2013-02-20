/*
 * $Id: Format.hh 5491 2012-03-09 17:27:25Z wrona $
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

namespace karabo {
    namespace io {
        namespace h5 {

            class Format {
            public:

                KARABO_CLASSINFO(Format, "Format", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

                Format() {
                }

                virtual ~Format() {
                }

                static void expectedParameters(karabo::util::Schema& expected);
                void configure(const karabo::util::Hash& input);

                //static Format::Pointer discoverFromData(const karabo::util::Hash& data);

                static void discoverFromHash(const karabo::util::Hash& data, karabo::util::Hash& config);

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
                    const std::vector<T>& vec = el.getValue< std::vector<T> >();
                    unsigned long long size = vec.size();
                    h.set("size", size);
                }

                template< class T> static void discoverVectorSize(karabo::util::Hash& h, const karabo::util::Hash::Attributes::Node el) {
                    const std::vector<T>& vec = el.getValue< std::vector<T> >();
                    unsigned long long size = vec.size();
                    h.set("size", size);
                }

                template< class T> static void discoverPtrSize(karabo::util::Hash& h, const karabo::util::Hash::Node& el) {
                    const std::vector<unsigned long long>& dims = el.getAttribute<std::vector<unsigned long long> >("dims");
                    unsigned long long size = dims[0];
                    for( size_t i=1; i< dims.size(); ++i){
                        size *= dims[i];
                    }
                    h.set("size", size);
                }


                // const RecordFormat::Pointer getRecordFormat();

                //static void r_discoverFromData(const karabo::util::Hash& data);                
                //static void r_discoverFromData(const karabo::util::Hash& data, karabo::util::Hash& config);
                //        const RecordFormat::Pointer getRecordFormat();
                //        const karabo::util::Hash& getConfig() const;
                //        


            private:
                static const char m_h5Sep = '/';
                karabo::util::Hash m_config;
                //
                //
                //        static std::string getClassIdAsString(const boost::any& any);        
                //        static karabo::io::ArrayDimensions arraySize(const boost::any& any);



            };


        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::h5::Format, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_H5_FORMAT_HH */

