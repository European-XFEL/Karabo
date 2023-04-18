/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Dataset.hh"

#include <karabo/log/Logger.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <sstream>

using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            hid_t Dataset::m_dataSetProperties = Dataset::initDataSetProperties();
            hid_t Dataset::m_linkCreateProperties = Dataset::initLinkCreateProperties();


            void Dataset::expectedParameters(Schema& expected) {
                VECTOR_UINT64_ELEMENT(expected)
                      .key("dims")
                      .displayedName("Dimensions")
                      .description(
                            "Array dimensions (x1,x2,x3,...). For example, for a simple image it is (width, height) ")
                      .tags("persistent")
                      .assignmentOptional()
                      .noDefaultValue()
                      .init()
                      .commit();

                INT32_ELEMENT(expected)
                      .key("compressionLevel")
                      .displayedName("Use Compression Level")
                      .description(
                            "Defines compression level: [0-9]. 0 - no compression (default), 9 - attempt the best "
                            "compression.")
                      .tags("persistent")
                      .minInc(0)
                      .maxInc(9)
                      .assignmentOptional()
                      .noDefaultValue()
                      .reconfigurable()
                      .commit();

                UINT64_ELEMENT(expected)
                      .key("chunkSize")
                      .displayedName("Chunk size")
                      .description("Number of records in the chunk")
                      .tags("persistent")
                      .assignmentOptional()
                      .noDefaultValue()
                      .reconfigurable()
                      .commit();
            }


            void Dataset::configureDataDimensions(const karabo::util::Hash& input, const Dims& singleValueDims) {
                size_t singleValueRank = singleValueDims.rank();

                if (input.has("dims") || input.has("shape")) {
                    const std::string attrName = input.has("dims") ? "dims" : "shape";
                    std::vector<unsigned long long> dimsVec = input.getAs<unsigned long long, std::vector>(attrName);
                    // reverse order as we need to store in hdf5
                    std::reverse(dimsVec.begin(), dimsVec.end());

                    for (size_t i = 0; i < singleValueRank; ++i) {
                        dimsVec.push_back(singleValueDims.extentIn(i));
                    }
                    m_dims = Dims(dimsVec);

                } else {
                    m_dims = singleValueDims;
                }

#ifdef KARABO_ENABLE_TRACE_LOG
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset.configureDataDimensions") << m_dims.rank();
                for (size_t i = 0; i < m_dims.rank(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset.configureDataDimensions")
                          << "m_dims[" << i << "] = " << m_dims.extentIn(i);
                }
#endif
            }


            hid_t Dataset::configureFileDataSpace() {
                std::vector<unsigned long long> dimsVector = m_dims.toVector();

                m_dataSetExtent = std::vector<hsize_t>(dimsVector.size() + 1, 0);
                m_dataSetMaxExtent = std::vector<hsize_t>(dimsVector.size() + 1, 0);

                // m_dataSetExtent[0] is zero
                m_dataSetMaxExtent[0] = H5S_UNLIMITED;
                for (hsize_t i = 1; i < m_dataSetExtent.size(); ++i) {
                    m_dataSetExtent[i] = dimsVector[i - 1];
                    m_dataSetMaxExtent[i] = dimsVector[i - 1];
                }

                hid_t fileDataSpace = this->createDataspace(m_dataSetExtent, m_dataSetMaxExtent);
                return fileDataSpace;
            }


            void Dataset::create(hid_t tableGroup) {
                // OPT1
                m_tableGroup = tableGroup;
                ///
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset")
                      << "Create dataset " << m_h5PathName << " with chunk size = " << m_chunkSize;
                try {
                    hid_t fileDataSpace = configureFileDataSpace();
                    hid_t dataSetProperties = createDataSetProperties();
                    hid_t tid = this->getDatasetTypeId();
                    m_h5obj = H5Dcreate2(tableGroup, m_h5PathName.c_str(), tid, fileDataSpace, m_linkCreateProperties,
                                         dataSetProperties, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_h5obj);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(dataSetProperties));
                    closeDataspace(fileDataSpace);
                    m_fileDataSpace = H5Dget_space(m_h5obj);
                    KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
                    // createAttributes(m_h5obj);
                    //// OPT1
                    KARABO_CHECK_HDF5_STATUS(H5Dclose(m_h5obj));
                    // m_h5objOpen = false;
                    m_h5obj = -1;
                    ////
                } catch (...) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot create dataset /" + m_h5PathName));
                }
            }


            hid_t Dataset::createDataSetProperties() {
                hid_t dataSetProperties = H5Pcreate(H5P_DATASET_CREATE);
                //                KARABO_CHECK_HDF5_STATUS(H5Pset_attr_phase_change(m_dataSetProperties, 0, 0));
                KARABO_CHECK_HDF5_STATUS(H5Pset_layout(dataSetProperties, H5D_CHUNKED));
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset")
                      << "Dataset property list created, chunkDims.rank=" << m_chunkSize
                      << " comp=" << m_compressionLevel;
                if (m_compressionLevel > 0) {
                    //         m_dataSetProperties->setShuffle();
                    KARABO_CHECK_HDF5_STATUS(H5Pset_deflate(dataSetProperties, m_compressionLevel));
                }
                m_dataSetExtent[0] = m_chunkSize;
                KARABO_CHECK_HDF5_STATUS(H5Pset_chunk(dataSetProperties, m_dataSetExtent.size(), &m_dataSetExtent[0]));
                m_dataSetExtent[0] = 0;
                return dataSetProperties;
            }


            hid_t Dataset::open(hid_t group) {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "opening dataset: " << m_h5PathName;
                m_h5obj = H5Dopen2(group, m_h5PathName.c_str(), H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_h5obj);
                m_fileDataSpace = H5Dget_space(m_h5obj);
                KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
                int ndims = H5Sget_simple_extent_ndims(m_fileDataSpace);
                KARABO_CHECK_HDF5_STATUS(ndims);
                m_dataSetExtent.resize(ndims);
                m_dataSetMaxExtent.resize(ndims);
                KARABO_CHECK_HDF5_STATUS(
                      H5Sget_simple_extent_dims(m_fileDataSpace, &m_dataSetExtent[0], &m_dataSetMaxExtent[0]));
                KARABO_LOG_FRAMEWORK_TRACE_CF << "m_h5obj=" << m_h5obj << " m_fileDataSpace=" << m_fileDataSpace;
                return m_h5obj;
            }


            void Dataset::write(const karabo::util::Hash& data, hsize_t recordId) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset")
                      << "Writing hash data: key=" << m_key << " recordId=" << recordId << " len=1";
                try {
                    if (data.has(m_key, '/')) {
                        openH5(m_tableGroup);
                        extendFileDataspace(recordId, 1);
                        selectFileRecords(recordId, 1);
                        const Hash::Node& node = data.getNode(m_key, '/');
                        writeNode(node, m_h5obj, m_fileDataSpace);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " key in the hash");
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" +
                                                                  m_h5PathName));
                }
            }


            void Dataset::write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Dataset")
                      << "Writing hash data: key=" << m_key << " recordId=" << recordId << " len=" << len;

                try {
                    if (data.has(m_key, '/')) {
                        openH5(m_tableGroup);
                        extendFileDataspace(recordId, len);
                        selectFileRecords(recordId, len);
                        const Hash::Node& node = data.getNode(m_key, '/');
                        writeNode(node, len, m_h5obj, m_fileDataSpace);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " key in the hash");
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" +
                                                                  m_h5PathName));
                }
            }


            void Dataset::read(hsize_t recordId) {
                try {
                    // openH5(m_tableGroup);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "m_h5obj=" << m_h5obj << " m_fileDataSpace=" << m_fileDataSpace;
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "select ";
                    selectFileRecords(recordId, 1);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "read record";
                    readRecord(m_h5obj, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Could not read " + m_h5PathName + " dataset"));
                }
            }


            void Dataset::read(hsize_t recordId, hsize_t len) {
                try {
                    //                    openH5(m_tableGroup);
                    selectFileRecords(recordId, len);
                    readRecords(len, m_h5obj, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Could not read " + m_h5PathName + " dataset"));
                }
            }


            void Dataset::close() {
                if (m_h5obj > -1) {
                    KARABO_CHECK_HDF5_STATUS(H5Dclose(m_h5obj));
                    m_h5obj = -1;
                }
                if (m_fileDataSpace > -1) {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_fileDataSpace));
                    m_fileDataSpace = -1;
                }
            }


            void Dataset::extendFileDataspace(hsize_t recordId, hsize_t len) {
                hsize_t lastRecord = recordId + len;
                if (lastRecord > m_dataSetExtent[0]) {
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(m_fileDataSpace));

                    hsize_t numNewChunks = (lastRecord - m_dataSetExtent[0] + m_chunkSize - 1) / m_chunkSize;
                    m_dataSetExtent[0] += numNewChunks * m_chunkSize;
                    KARABO_CHECK_HDF5_STATUS(H5Dset_extent(m_h5obj, &m_dataSetExtent[0]));
                    m_fileDataSpace = H5Dget_space(m_h5obj);
                    KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
                }
            }


            void Dataset::selectFileRecords(hsize_t recordId, hsize_t len) {
                int ndims = m_dataSetExtent.size();
                std::vector<hsize_t> start(ndims, 0);
                start[0] = recordId;

                std::vector<hsize_t> count = m_dataSetExtent;
                count[0] = len;
                KARABO_CHECK_HDF5_STATUS(
                      H5Sselect_hyperslab(m_fileDataSpace, H5S_SELECT_SET, &start[0], NULL, &count[0], NULL));
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
                return ds;
            }


            hid_t Dataset::dataSpaceOneDim(hsize_t len) {
                hsize_t dims[] = {len};
                hsize_t maxdims[] = {H5S_UNLIMITED};
                hid_t ds = H5Screate_simple(1, dims, maxdims);
                KARABO_CHECK_HDF5_STATUS(ds);
                return ds;
            }


            void Dataset::getDataSpaceInfo(hid_t dataSpace, std::ostringstream& oss) {
                int ndims = H5Sget_simple_extent_ndims(dataSpace);
                KARABO_CHECK_HDF5_STATUS(ndims);
                std::vector<hsize_t> extent(ndims, 0);
                std::vector<hsize_t> maxExtent(ndims, 0);
                KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(dataSpace, &extent[0], &maxExtent[0]));
                for (int i = 0; i < ndims; ++i) {
                    oss << "[0]={" << extent[i] << "," << maxExtent[i] << "}; ";
                }
            }
        } // namespace h5
    }     // namespace io
} // namespace karabo
