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
                    m_nativeTypeId = Derived::getNativeTypeId();
                    m_standardTypeId = Derived::getStandardTypeId();
                }

                virtual ~Attribute() {
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(m_nativeTypeId))
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(m_standardTypeId))
                }


                void write(const karabo::util::Hash::Node& data);

                void save(const karabo::util::Hash::Node& data, hid_t element);

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
                virtual void create(hid_t element);

                // to be removed from here

                virtual void open(hid_t element);

                virtual void close() {
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(m_attribute));
                }

                virtual karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node&) = 0;





            protected:

                const karabo::util::Dims& dims() const {
                    return m_dims;
                }



                std::string m_h5name;
                std::string m_key;

                hid_t m_attribute;
                hid_t m_h5ElementObj;

                hid_t configureDataSpace();

                virtual hid_t createDataspace(const std::vector<hsize_t>& ex, const std::vector<hsize_t>& maxEx) {
                    return H5Screate_simple(ex.size(), &ex[0], &maxEx[0]);
                }

                virtual void closeDataspace(hid_t dataSpace) {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace));
                }

            private:
                karabo::util::Dims m_dims; // dimension of written/read objects 
                hid_t m_dataSetProperties;

                hid_t m_nativeTypeId;
                hid_t m_standardTypeId;

                void configureDataDimensions(const karabo::util::Hash& input, const karabo::util::Dims& singleValueDims);


            };


        }
    }
}


#endif

