/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_ATTRIBUTE_HH
#define	KARABO_IO_H5_ATTRIBUTE_HH


#include <string>

#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <karabo/log/Logger.hh>

#include "ErrorHandler.hh"

#include "TypeTraits.hh"
//#include <boost/enable_shared_from_this.hpp>


namespace karabo {
    namespace io {
        namespace h5 {

            class Attribute {

            public:
                KARABO_CLASSINFO(Attribute, "Attribute", "1.0");
                KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema& expected);

                template <class Derived>
                Attribute(const karabo::util::Hash& input, Derived* d) {

                    input.get("h5name", m_h5name);
                    if (input.has("key")) {
                        input.get("key", m_key);
                    } else {
                        m_key = m_h5name;
                    }
                    karabo::util::Dims singleValueDims = Derived::getSingleValueDimensions();
                    configureDataDimensions(input, singleValueDims);
                    configureDataSpace();
                    m_nativeTypeId = Derived::getNativeTypeId() ;
                    m_standardTypeId = Derived::getStandardTypeId();
                }

                virtual ~Attribute() {
                }


                void write(const karabo::util::Hash::Node& data);

            protected:

                
                virtual void writeNodeAttribute(const karabo::util::Element<std::string>& node,
                                        hid_t attribute) = 0;

            public:

                void read(karabo::util::Hash::Node& data);

            protected:

                virtual void readNodeAttribute(karabo::util::Element<std::string>& attrNode,
                                        hid_t attribute) = 0;

            public:

                /**
                 * Create attribute                 
                 */
                virtual void create(hid_t element); // = 0;

                // to be removed from here

                virtual void open(hid_t element);

                virtual void close() {
                }

                virtual karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node&) = 0;





            protected:

                const karabo::util::Dims& dims() const {
                    return m_dims;
                }



                std::string m_h5name;
                std::string m_key;

                hid_t m_attribute;
                hid_t m_element;

            private:
                karabo::util::Dims m_dims; // dimension of written/read objects 
                hid_t m_dataSetProperties;
                hid_t m_dataSpace;

                hid_t m_nativeTypeId;
                hid_t m_standardTypeId;

                void configureDataDimensions(const karabo::util::Hash& input, const karabo::util::Dims& singleValueDims);
                void configureDataSpace();

            };


        }
    }
}


#endif

