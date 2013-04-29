/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <sstream> 
#include "Dataset.hh"
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/Dims.hh>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            void Dataset::expectedParameters(Schema & expected) {

                karabo::util::VECTOR_UINT64_ELEMENT(expected)
                        .key("dims")
                        .displayedName("Dimensions")
                        .description("Array dimensions.")
                        .tags("persistent")
                        .assignmentOptional().noDefaultValue()
                        .init()
                        .commit();

                INT32_ELEMENT(expected)
                        .key("compressionLevel")
                        .displayedName("Use Compression Level")
                        .description("Defines compression level: [0-9]. 0 - no compression (default), 9 - attempt the best compression.")
                        .tags("persistent")
                        .minInc(0).maxInc(9)
                        .assignmentOptional().defaultValue(0)
                        .reconfigurable()
                        .commit();

                INT32_ELEMENT(expected)
                        .key("chunkSize")
                        .displayedName("Chunk size")
                        .description("Number of records in the chunk")
                        .tags("persistent")
                        .minInc(0).maxInc(9)
                        .assignmentOptional().defaultValue(1)
                        .reconfigurable()
                        .commit();
            }


            Dataset::Dataset(const karabo::util::Hash& input) : Element(input), m_numberAllocatedRecords(0) {
                m_compressionLevel = input.get<int>("compressionLevel");
                configureDataDimensions(input);
                configureFileDataSpace(input);

            }


            void Dataset::configureDataDimensions(const karabo::util::Hash& input) {
                if (input.has("dims")) {
                    m_dims = Dims(input.get< vector<unsigned long long> >("dims"));
                } else {
                    // assume scalar value
                    m_dims = Dims();
                }

                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset.configureDataDimensions") << m_dims.rank();
                #ifdef KARABO_LOG_ENABLE_TRACE
                for (int i = 0; i < m_dims.rank(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset.configureDataDimensions") << "m_dims[" << i << "] = " << m_dims.extentIn(i);
                }
                #endif                

            }


            void Dataset::configureFileDataSpace(const karabo::util::Hash& input) {

                vector<unsigned long long> dimsVector = m_dims.toVector();

                m_dataSetExtent = vector<hsize_t>(dimsVector.size() + 1, 0);
                m_dataSetMaxExtent = vector<hsize_t>(dimsVector.size() + 1, 0);

                // m_dataSetExtent[0] is zero
                m_dataSetMaxExtent[0] = H5S_UNLIMITED;
                for (hsize_t i = 1; i < m_dataSetExtent.size(); ++i) {
                    m_dataSetExtent[i] = dimsVector[i - 1];
                    m_dataSetMaxExtent[i] = dimsVector[i - 1];
                }
                m_fileDataSpace = H5Screate_simple(m_dataSetExtent.size(),
                                                   &m_dataSetExtent[0],
                                                   &m_dataSetMaxExtent[0]);
                KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
            }


            void Dataset::create(hsize_t chunkSize) {

                m_chunkSize = chunkSize;

                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset") << "Create dataset " << m_h5PathName
                        << " with chunk size = " << chunkSize;
                try {

                    createDataSetProperties();
                    hid_t tid = this->getDatasetTypeId();
                    m_dataSet = H5Dcreate(m_parentGroup, m_h5name.c_str(), tid,
                                          m_fileDataSpace, H5P_DEFAULT, m_dataSetProperties, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_dataSet);
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                }

            }


            void Dataset::createDataSetProperties() {
                m_dataSetProperties = H5Pcreate(H5P_DATASET_CREATE);
                KARABO_CHECK_HDF5_STATUS(H5Pset_layout(m_dataSetProperties, H5D_CHUNKED));

                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset") << "Dataset property list created, chunkDims.rank="
                        << m_chunkSize << " comp=" << m_compressionLevel;

                if (m_compressionLevel > 0) {
                    //         m_dataSetProperties->setShuffle();
                    KARABO_CHECK_HDF5_STATUS(H5Pset_deflate(m_dataSetProperties, m_compressionLevel));
                }

                std::vector<hsize_t> chunk = m_dataSetExtent;
                chunk[0] = m_chunkSize;
                KARABO_CHECK_HDF5_STATUS(H5Pset_chunk(m_dataSetProperties, chunk.size(), &chunk[0]));
            }


            void Dataset::open(hid_t group) {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "opening dataset: " << m_h5PathName.c_str();
                m_dataSet = H5Dopen2(group, m_h5PathName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_dataSet);
                m_fileDataSpace = H5Dget_space(m_dataSet);
                KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
                int ndims = H5Sget_simple_extent_ndims(m_fileDataSpace);
                KARABO_CHECK_HDF5_STATUS(ndims);
                m_dataSetExtent.resize(ndims);
                m_dataSetMaxExtent.resize(ndims);
                KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(m_fileDataSpace,
                                                                   &m_dataSetExtent[0],
                                                                   &m_dataSetMaxExtent[0])
                                         );
            }


            void Dataset::write(const karabo::util::Hash& data, hsize_t recordId) {

                try {
                    extendFileDataspace(recordId, 1);
                    selectFileRecords(recordId, 1);
                    const Hash::Node& node = data.getNode(m_key, '/');
                    writeNode(node, m_dataSet, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                }

            }


            void Dataset::write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                try {
                    extendFileDataspace(recordId, len);
                    selectFileRecords(recordId, len);
                    const Hash::Node& node = data.getNode(m_key, '/');
                    writeNode(node, len, m_dataSet, m_fileDataSpace);

                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                }

            }


            void Dataset::read(hsize_t recordId) {
                try {
                    selectFileRecords(recordId, 1);
                    readRecord(m_dataSet, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Could not read " + m_h5PathName + " dataset"));
                }
            }


            void Dataset::read(hsize_t recordId, hsize_t len) {
                try {
                    selectFileRecords(recordId, len);
                    readRecords(len, m_dataSet, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Could not read " + m_h5PathName + " dataset"));
                }
            }


            void Dataset::close() {
                KARABO_CHECK_HDF5_STATUS(H5Pclose(m_dataSetProperties));
                KARABO_CHECK_HDF5_STATUS(H5Dclose(m_dataSet));
                KARABO_CHECK_HDF5_STATUS(H5Sclose(m_fileDataSpace));
            }


            void Dataset::extendFileDataspace(hsize_t recordId, hsize_t len) {

                hsize_t lastRecord = recordId + len;
                if (lastRecord > m_dataSetExtent[0]) {
                    hsize_t numNewChunks = (lastRecord - m_dataSetExtent[0] + m_chunkSize - 1) / m_chunkSize;
                    m_dataSetExtent[0] += numNewChunks * m_chunkSize;
                    KARABO_CHECK_HDF5_STATUS(H5Dset_extent(m_dataSet, &m_dataSetExtent[0]));
                    m_fileDataSpace = H5Dget_space(m_dataSet);
                    KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
                }
            }


            void Dataset::selectFileRecords(hsize_t recordId, hsize_t len) {
                int ndims = m_dataSetExtent.size();
                std::vector<hsize_t> start(ndims, 0);
                start[0] = recordId;

                std::vector<hsize_t> count = m_dataSetExtent;
                count[0] = len;
                KARABO_CHECK_HDF5_STATUS(H5Sselect_hyperslab(m_fileDataSpace,
                                                             H5S_SELECT_SET,
                                                             &start[0],
                                                             NULL,
                                                             &count[0],
                                                             NULL));
                //
                //                int ndims = H5Sget_simple_extent_ndims(m_fileDataSpace);
                //                KARABO_CHECK_HDF5_STATUS(ndims);
                //                std::vector<hsize_t> start(ndims, 0);
                //                start[0] = recordId;
                //
                //                std::vector<hsize_t> count(ndims, 0);
                //                std::vector<hsize_t> maxExtent(ndims, 0);
                //                KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(m_fileDataSpace, &count[0], &maxExtent[0]));
                //                count[0] = len;
                //                KARABO_CHECK_HDF5_STATUS(H5Sselect_hyperslab(m_fileDataSpace,
                //                                                             H5S_SELECT_SET,
                //                                                             &start[0],
                //                                                             NULL,
                //                                                             &count[0],
                //                                                             NULL)
                //                                         );
            }




            ///////////////////////


            hid_t Dataset::extend(hid_t dataSet, hid_t dataSpace, hsize_t len) {

                int ndims = H5Sget_simple_extent_ndims(dataSpace);
                KARABO_CHECK_HDF5_STATUS(ndims);
                std::vector<hsize_t> extent(ndims, 0);
                std::vector<hsize_t> maxExtent(ndims, 0);
                herr_t status = H5Sget_simple_extent_dims(dataSpace, &extent[0], &maxExtent[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                extent[0] += len;
                status = H5Dset_extent(dataSet, &extent[0]);
                KARABO_CHECK_HDF5_STATUS(status);
                dataSpace = H5Dget_space(dataSet);
                return dataSpace;
            }


            hid_t Dataset::dataSpace(const karabo::util::Dims& dims) {
                if (dims.rank() == 0) {
                    return dataSpace();
                }
                std::vector<hsize_t> curdims(dims.rank(), 0);
                std::vector<hsize_t> maxdims(dims.rank(), 0);
                for (size_t i = 0; i < dims.rank(); ++i) {
                    curdims[i] = dims.extentIn(i);
                    maxdims[i] = curdims[i];
                }
                maxdims[0] = H5S_UNLIMITED;
                hid_t ds = H5Screate_simple(dims.rank(), &curdims[0], &maxdims[0]);
                KARABO_CHECK_HDF5_STATUS(ds);
                #ifdef KARABO_LOG_ENABLE_TRACE
                getDataSpaceInfo(ds, "Just after creating dataspace");
                #endif
                return ds;
            }


            hid_t Dataset::dataSpace1dim(hsize_t len) {
                hsize_t dims[] = {len};
                hsize_t maxdims[] = {H5S_UNLIMITED};
                hid_t ds = H5Screate_simple(1, dims, maxdims);
                KARABO_CHECK_HDF5_STATUS(ds);
                return ds;
            }

        }
    }
}
