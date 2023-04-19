/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_ATTRIBUTE_HH
#define KARABO_IO_H5_ATTRIBUTE_HH


#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <string>

#include "ErrorHandler.hh"
#include "TypeTraits.hh"

namespace karabo {
    namespace io {
        namespace h5 {

            /**
             * @class Attribute
             * @brief This class maps Karabo attributes to HDF5 attributes
             */
            class Attribute {
               public:
                KARABO_CLASSINFO(Attribute, "Attribute", "1.0");
                KARABO_CONFIGURATION_BASE_CLASS

                /**
                 * Expected parameters used for factorized configuration:
                 *
                 * - h5name: the name of the attribute in the HDF5 file
                 * - key: the name of the attribute in the Karabo Hash
                 * - dims: dimensions of the attribute. Determines if it is a scalar or vector attribute.
                 *
                 * @param expected
                 */
                static void expectedParameters(karabo::util::Schema& expected);

                /**
                 * Constructs and Attribute wrapper. The dimensions of a single value of the attribute type are
                 * inferred from Derived* d.
                 *
                 * @param input: configuration Hash as defined by expected parameters, if key is not defined h5name is
                 *               used instead. If dims is not given the dimensions of a single value of type Derived
                 *               are used. Otherwise dimensions found in dims up to the rank of the single value
                 *               dimensions are extracted from dims.
                 * @param d: type determining the single value dimension, native and standard datatypes
                 */
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

                virtual ~Attribute();

                /**
                 * Write the attributes in data as defined by configuration to HDF5
                 * @param data
                 */
                void write(const karabo::util::Hash::Node& data);

                /**
                 * Save the attributes in data as defined by configuration to an HDF5 element.
                 * @param data
                 */
                void save(const karabo::util::Hash::Node& data, hid_t element);

               protected:
                virtual void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) = 0;

               public:
                /**
                 * Read attribute from HDF5 as defined by configuration to a Hash::Node
                 * @param data
                 */
                void read(karabo::util::Hash::Node& data);

               protected:
                virtual void readNodeAttribute(karabo::util::Element<std::string>& attrNode, hid_t attribute) = 0;

               public:
                /**
                 * Create attribute
                 */
                virtual void create(hid_t element);


                /**
                 * Open HDF5 dataset at element
                 * @param element
                 */
                virtual void open(hid_t element);

                /**
                 * Open HDF5 dataset holding the attribute
                 */
                virtual void close() {
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(m_attribute));
                }

                /**
                 * Bind HDF5 attribute to a Hash node
                 * @param
                 * @return
                 */
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

                void configureDataDimensions(const karabo::util::Hash& input,
                                             const karabo::util::Dims& singleValueDims);
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo


#endif
