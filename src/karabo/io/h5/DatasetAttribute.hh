/*
 * $Id: DatasetAttribute.hh 9852 2013-05-26 21:52:12Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETATTRIBUTE_HH
#define KARABO_IO_H5_DATASETATTRIBUTE_HH

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <string>

#include "Dataset.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class DatasetAttribute
             * @brief Represents a Karabo Attribute as a HDF5 dataset (useful for complex attributes)
             */
            class DatasetAttribute : public karabo::io::h5::Dataset {
               public:
                KARABO_CLASSINFO(DatasetAttribute, "DatasetAttribute", "1.0")


                template <class Derived>
                DatasetAttribute(const karabo::util::Hash& input, Derived* d) : karabo::io::h5::Dataset(input, d) {}

                virtual ~DatasetAttribute() {}

                /**
                 * Create a table in the HDF5 file to hold the attributes
                 * @param tableGroup
                 */
                void create(hid_t tableGroup) {
                    // OPT1
                    m_tableGroup = tableGroup;
                    ///
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset")
                          << "Create dataset " << m_h5PathName << " with chunk size = " << m_chunkSize;
                    try {
                        hid_t fileDataSpace = configureFileDataSpace();
                        hid_t dataSetProperties = createDataSetProperties();
                        hid_t tid = this->getDatasetTypeId();
                        m_h5obj = H5Dcreate2(tableGroup, (m_h5PathName + "aa").c_str(), tid, fileDataSpace,
                                             m_linkCreateProperties, dataSetProperties, H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_h5obj);
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                        KARABO_CHECK_HDF5_STATUS(H5Pclose(dataSetProperties));
                        closeDataspace(fileDataSpace);
                        m_fileDataSpace = H5Dget_space(m_h5obj);
                        KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);

                        // createAttributes(m_h5obj);

                        //// OPT1
                        H5Dclose(m_h5obj);
                        // m_h5objOpen = false;
                        m_h5obj = -1;
                        ////

                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                    }
                }

                /**
                 * Write a data hash to as attribute for a given record
                 * @param data
                 * @param recordId
                 */
                void write(const karabo::util::Hash& data, hsize_t recordId);

                /**
                 * Write a data hash to as attribute for a N=len records starting at recordId
                 * @param data
                 * @param recordId
                 */
                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len);

               protected:
                virtual void writeNode(const karabo::util::Element<std::string>& data, hid_t dataSet,
                                       hid_t fileDataSpace) {}

                virtual void writeNode(const karabo::util::Element<std::string>& data, hsize_t len, hid_t dataSet,
                                       hid_t fileDataSpace) {}
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
