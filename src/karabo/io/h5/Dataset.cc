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
                        //.defaultValue(vector<unsigned long long>())
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
                configureFileDataSpace(input);
            }


            void Dataset::configureFileDataSpace(const karabo::util::Hash& input) {
                vector<unsigned long long> dims;
                if (input.has("dims")) {
                    dims = input.get< vector<unsigned long long> >("dims");
                }

                m_dataSetExtent = vector<hsize_t>(dims.size() + 1, 0);
                m_dataSetMaxExtent = vector<hsize_t>(dims.size() + 1, 0);

                m_dataSetMaxExtent[0] = H5S_UNLIMITED;
                for (hsize_t i = 1; i < m_dataSetExtent.size(); ++i) {
                    m_dataSetExtent[i] = dims[i - 1];
                    m_dataSetMaxExtent[i] = dims[i - 1];
                }
                m_fileDataSpace = H5Screate_simple(m_dataSetExtent.size(),
                                                   &m_dataSetExtent[0],
                                                   &m_dataSetMaxExtent[0]);
                KARABO_CHECK_HDF5_STATUS(m_fileDataSpace);
            }


            void Dataset::open(hid_t group) {
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
                    extend(recordId, 1);
                    selectFileRecords(recordId, 1);
                    const Hash::Node& node = data.getNode(m_key, '/');
                    writeNode(node, m_dataSet, m_fileDataSpace);
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                }

            }


            void Dataset::write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                try {
                    extend(recordId, len);
                    selectFileRecords(recordId, len);
                    const Hash::Node& node = data.getNode(m_key, '/');
                    writeNode(node, len, m_dataSet, m_fileDataSpace);

                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                }

            }


            void Dataset::close() {
                KARABO_CHECK_HDF5_STATUS(H5Pclose(m_dataSetProperties));
                KARABO_CHECK_HDF5_STATUS(H5Dclose(m_dataSet));
            }


            void Dataset::extend(hsize_t recordId, hsize_t len) {

                hsize_t lastRecord = recordId + len;
                if (lastRecord > m_dataSetExtent[0]) {
                    hsize_t numNewChunks = (lastRecord - m_dataSetExtent[0] + m_chunkSize - 1) / m_chunkSize;
                    m_dataSetExtent[0] += numNewChunks * m_chunkSize;
                    KARABO_CHECK_HDF5_STATUS(H5Dset_extent(m_dataSet, &m_dataSetExtent[0]));
                    m_fileDataSpace = H5Dget_space(m_dataSet);
                }
            }


            void Dataset::selectFileRecords(hsize_t recordId, hsize_t len) {
                int ndims = H5Sget_simple_extent_ndims(m_fileDataSpace);
                KARABO_CHECK_HDF5_STATUS(ndims);
                std::vector<hsize_t> start(ndims, 0);
                start[0] = recordId;

                std::vector<hsize_t> count(ndims, 0);
                std::vector<hsize_t> maxExtent(ndims, 0);
                KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(m_fileDataSpace, &count[0], &maxExtent[0]));
                count[0] = len;
                KARABO_CHECK_HDF5_STATUS(H5Sselect_hyperslab(m_fileDataSpace,
                                                             H5S_SELECT_SET,
                                                             &start[0],
                                                             NULL,
                                                             &count[0],
                                                             NULL)
                                         );
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
        }
    }
}
