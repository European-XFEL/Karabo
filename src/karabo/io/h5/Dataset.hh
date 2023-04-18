/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASET_HH
#define KARABO_IO_H5_DATASET_HH

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <string>

#include "Element.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class Dataset
             * @brief The base class representing Datasets in an HDF5 file
             */
            class Dataset : public karabo::io::h5::Element {
               public:
                KARABO_CLASSINFO(Dataset, "Dataset", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                /**
                 * Construct a dataset base representing an Element of type Derived in the HDF5 file
                 * @param input, input configuration
                 * @param d, pointer to the derived. Needed for type information only
                 */
                template <class Derived>
                Dataset(const karabo::util::Hash& input, Derived* d)
                    : Element(input), m_numberAllocatedRecords(0), m_fileDataSpace(-1) {
                    if (input.has("compressionLevel")) {
                        m_compressionLevel = input.getAs<int>("compressionLevel");
                    } else {
                        m_compressionLevel = 0;
                    }
                    if (input.has("chunkSize")) {
                        m_chunkSize = input.getAs<unsigned long long>("chunkSize");
                    } else {
                        m_chunkSize = 1;
                    }
                    karabo::util::Dims singleValueDims = Derived::getSingleValueDimensions();
                    configureDataDimensions(input, singleValueDims);
                }

                virtual ~Dataset() {}


                virtual hid_t open(hid_t group);

                /**
                 * Write data to the dataset
                 * @param data to write
                 * @param recordId to write to
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId);

                /**
                 * Batch write data to the dataset
                 * @param data to write
                 * @param recordId to start writing to
                 * @param len number of records to write, as contained in data
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len);

               protected:
                /**
                 * Write a Hash::Node to a dataSet in an HDF5 data space
                 * @param data
                 * @param dataSet
                 * @param fileDataSpace
                 */
                virtual void writeNode(const karabo::util::Hash::Node& data, hid_t dataSet, hid_t fileDataSpace) {}

                /**
                 * Batch write len Hash::Nodes to a dataSet in an HDF5 data space
                 * @param data
                 * @param dataSet
                 * @param fileDataSpace
                 */
                virtual void writeNode(const karabo::util::Hash::Node& data, hsize_t len, hid_t dataSet,
                                       hid_t fileDataSpace) {}

               public:
                /**
                 * Read data from record id
                 * @param recordId
                 */
                void read(hsize_t recordId);

                /**
                 * Batch read len records from recordId onwards
                 * @param recordId
                 * @param len
                 */
                void read(hsize_t recordId, hsize_t len);

               protected:
                /**
                 * Read a record identified by dataSet in an HDF5 dataspace
                 * @param dataSet
                 * @param fileDataSpace
                 */
                virtual void readRecord(const hid_t& dataSet, const hid_t& fileDataSpace) = 0;

                /**
                 * Batch read len records identified by dataSet in an HDF5 dataspace
                 * @param len
                 * @param dataSet
                 * @param fileDataSpace
                 */
                virtual void readRecords(hsize_t len, const hid_t& dataSet, const hid_t& fileDataSpace) = 0;

               public:
                /**
                 * Get info on the HDF5 dataspace
                 * @param dataSpace
                 * @param oss
                 */
                static void getDataSpaceInfo(hid_t dataSpace, std::ostringstream& oss);

                /**
                 * Return a dataspace with size of dims
                 * @param dims
                 * @return
                 */
                static hid_t dataSpace(const karabo::util::Dims& dims);

                /**
                 * Return a 1D-dataspace with the only dimension set to len size
                 * @param len
                 * @return
                 */
                static hid_t dataSpaceOneDim(hsize_t len);

                virtual hid_t createDataspace(const std::vector<hsize_t>& ex, const std::vector<hsize_t>& maxEx) {
                    hid_t dataSpace = H5Screate_simple(ex.size(), &ex[0], &maxEx[0]);
                    KARABO_CHECK_HDF5_STATUS(dataSpace);
                    return dataSpace;
                }


                /**
                 * Create a HDF5 Table group
                 * @param tableGroup
                 */
                virtual void create(hid_t tableGroup);

                /**
                 * Check if this Dataset identifies an HDF5 dataset
                 * @return
                 */
                bool isDataset() const {
                    return true;
                }

                /**
                 * Check if this Dataset identifies an HDF5 group
                 * @return
                 */
                bool isGroup() const {
                    return false;
                }

                /**
                 * Get the dimensions of this dataset
                 * @return
                 */
                karabo::util::Dims getDims() const {
                    return m_dims;
                }

                /**
                 * Set the compression level for this dataset
                 * @param level
                 */
                void setCompressionLevel(int level) {
                    m_compressionLevel = level;
                }


               protected:
                const karabo::util::Dims& dims() const {
                    return m_dims;
                }

                virtual hid_t getDatasetTypeId() = 0;

                static hid_t dataSpace() {
                    hsize_t dims[] = {1};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    hid_t ds = H5Screate_simple(1, dims, maxdims);
                    KARABO_CHECK_HDF5_STATUS(ds);
                    return ds;
                }

                void extendFileDataspace(hsize_t recordId, hsize_t len);
                void selectFileRecords(hsize_t recordId, hsize_t len = 1);


                static hid_t extend(hid_t dataSet, hid_t dataSpace, hsize_t len);

                void close();

                virtual void closeDataspace(hid_t dataSpace) {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace));
                }


                // private:
               protected:
                int m_compressionLevel;
                hsize_t m_numberAllocatedRecords;

                karabo::util::Dims m_dims; // dimension of written/read objects (= dataset dims minus one)

                // description of file data space
                std::vector<hsize_t> m_dataSetExtent;
                std::vector<hsize_t> m_dataSetMaxExtent;

                hsize_t m_chunkSize;

               protected:
                hid_t m_fileDataSpace;

                // private:

                void configureDataDimensions(const karabo::util::Hash& input,
                                             const karabo::util::Dims& singleValueDims);
                hid_t configureFileDataSpace();
                hid_t createDataSetProperties();

                void openH5(hid_t group) {
                    if (m_h5obj < 0) {
                        m_h5obj = H5Dopen2(group, m_h5PathName.c_str(), H5P_DEFAULT);
                        KARABO_CHECK_HDF5_STATUS(m_h5obj);
                    }
                }

                void closeH5() {
                    if (m_h5obj > -1) {
                        KARABO_CHECK_HDF5_STATUS(H5Dclose(m_h5obj));
                        m_h5obj = -1;
                    }
                }


                static hid_t m_dataSetProperties;
                static hid_t m_linkCreateProperties;

                static hid_t initDataSetProperties() {
                    return 0; // H5Pcreate(H5P_DATASET_CREATE);
                }

                static hid_t initLinkCreateProperties() {
                    hid_t lid = H5Pcreate(H5P_LINK_CREATE);
                    KARABO_CHECK_HDF5_STATUS(lid);
                    KARABO_CHECK_HDF5_STATUS(H5Pset_create_intermediate_group(lid, 1));
                    return lid;
                }
            };


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
