/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "Attribute.hh"

#include <karabo/log/Logger.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>

using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {


            void Attribute::expectedParameters(Schema& expected) {
                STRING_ELEMENT(expected)
                      .key("h5name")
                      .tags("persistent")
                      .displayedName("H5 Attribute Name")
                      .description("Attribute name")
                      .assignmentMandatory()
                      .reconfigurable()
                      .commit();


                STRING_ELEMENT(expected)
                      .key("key")
                      .displayedName("Hash key")
                      .description("Name of the attribute in the Hash node")
                      .assignmentOptional()
                      .noDefaultValue()
                      .reconfigurable()
                      .commit();

                VECTOR_UINT64_ELEMENT(expected)
                      .key("dims")
                      .displayedName("Dimensions")
                      .description("Array dimensions.")
                      .tags("persistent")
                      .assignmentOptional()
                      .noDefaultValue()
                      .init()
                      .commit();
            }


            Attribute::~Attribute() {
                KARABO_CHECK_HDF5_STATUS_NO_THROW(H5Tclose(m_nativeTypeId))
                KARABO_CHECK_HDF5_STATUS_NO_THROW(H5Tclose(m_standardTypeId))
            }


            void Attribute::configureDataDimensions(const karabo::util::Hash& input, const Dims& singleValueDims) {
                size_t singleValueRank = singleValueDims.rank();
                if (input.has("dims")) {
                    std::vector<unsigned long long> dimsVec = input.getAs<unsigned long long, std::vector>("dims");
                    for (size_t i = 0; i < singleValueRank; ++i) {
                        dimsVec.push_back(singleValueDims.extentIn(i));
                    }
                    m_dims = Dims(dimsVec);

                } else {
                    m_dims = singleValueDims;
                }

#ifdef KARABO_ENABLE_TRACE_LOG
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute.configureDataDimensions") << m_dims.rank();
                for (size_t i = 0; i < m_dims.rank(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute.configureDataDimensions")
                          << "m_dims[" << i << "] = " << m_dims.extentIn(i);
                }
#endif
            }


            hid_t Attribute::configureDataSpace() {
                std::vector<unsigned long long> dimsVector = m_dims.toVector();
                return this->createDataspace(dimsVector, dimsVector);
            }


            void Attribute::create(hid_t element) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute") << "Create attribute " << m_h5name;
                try {
                    hid_t dataSpace = configureDataSpace();
                    m_attribute =
                          H5Acreate(element, m_h5name.c_str(), m_standardTypeId, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_attribute);
                    closeDataspace(dataSpace);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create attribute /" + m_h5name));
                }
            }


            void Attribute::open(hid_t element) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute") << "open attribute " << m_h5name;
                try {
                    m_attribute = H5Aopen(element, m_h5name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_attribute);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot open attribute /" + m_h5name));
                }
            }


            void Attribute::write(const karabo::util::Hash::Node& node) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute") << "Writing hash attribute: key=" << m_key;
                try {
                    if (node.hasAttribute(m_key)) {
                        const karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        writeNodeAttribute(attrNode, m_attribute);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " attribute");
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node attribute " + m_key +
                                                                  " to H5 attribute" + m_h5name));
                }
            }


            void Attribute::save(const karabo::util::Hash::Node& node, hid_t element) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Attribute") << "Writing hash attribute: key=" << m_key;
                try {
                    if (node.hasAttribute(m_key)) {
                        hid_t dataSpace = configureDataSpace();
                        const karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        m_attribute = H5Acreate(element, m_h5name.c_str(), m_standardTypeId, dataSpace, H5P_DEFAULT,
                                                H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_attribute);
                        writeNodeAttribute(attrNode, m_attribute);
                        KARABO_CHECK_HDF5_STATUS(H5Aclose(m_attribute));
                        closeDataspace(dataSpace);

                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " attribute");
                    }

                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot save Hash node attribute " + m_key +
                                                                  " to H5 attribute" + m_h5name));
                }
            }


            void Attribute::read(karabo::util::Hash::Node& node) {
                try {
                    karabo::util::Element<std::string>& attrNode = bindAttribute(node);
                    readNodeAttribute(attrNode, m_attribute);
                    //                    } else {
                    //                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " attribute");
                    //                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node attribute " + m_key +
                                                                  " to H5 attribute" + m_h5name));
                }
            }


        } // namespace h5
    }     // namespace io
} // namespace karabo
