/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_GROUP_HH
#define	KARABO_IO_H5_GROUP_HH

#include <string>

#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>

#include "Element.hh"
#include "ErrorHandler.hh"

#include "TypeTraits.hh"


//#include "ScalarFilter.hh"


//#include <karabo/util/Time.hh>
//#include "../ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            class Dataset : public karabo::io::h5::Element {
            public:

                KARABO_CLASSINFO(Dataset, "Dataset", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                Dataset(const karabo::util::Hash& input) : Element(input) {
                    m_compressionLevel = input.get<int>("compressionLevel");
                }

                virtual ~Dataset() {
                }


            protected:

                void createDataSetProperties(karabo::util::Dims& chunkDims) {
                    m_dataSetProperties = H5Pcreate(H5P_DATASET_CREATE);
                    herr_t status = H5Pset_layout(m_dataSetProperties, H5D_CHUNKED);
                    KARABO_CHECK_HDF5_STATUS(status);
                    if (m_compressionLevel > 0) {
                        //         m_dataSetProperties->setShuffle();
                        status = H5Pset_deflate(m_dataSetProperties, m_compressionLevel);
                        KARABO_CHECK_HDF5_STATUS(status);
                    }
                    status = H5Pset_chunk(m_dataSetProperties, chunkDims.rank(), &(chunkDims.toVector())[0]);
                    KARABO_CHECK_HDF5_STATUS(status);
                }

                static hid_t dataSpace(karabo::util::Dims& dims) {
                    std::vector<hsize_t> curdims(dims.rank(), 0);
                    std::vector<hsize_t> maxdims(dims.rank(), 0);
                    for (size_t i = 0; i < dims.rank(); ++i) {
                        curdims[i] = dims.extentIn(i);
                        maxdims[i] = curdims[i];
                    }
                    maxdims[0] = H5S_UNLIMITED;
                    hid_t ds = H5Screate_simple(dims.rank(), &curdims[0], &maxdims[0]);
                    //std::clog << "dataSpace " << ds << std::endl;
                    KARABO_CHECK_HDF5_STATUS(ds);
                    return ds;
                }

                static hid_t dataSpace() {
                    hsize_t dims[] = {1};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    hid_t ds =  H5Screate_simple(1, dims, maxdims);
                    KARABO_CHECK_HDF5_STATUS(ds);
                    return ds;
                }

                static hid_t dataSpace(hsize_t len) {
                    hsize_t dims[] = {len};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    hid_t ds = H5Screate_simple(1, dims, maxdims);
                    KARABO_CHECK_HDF5_STATUS(ds);
                    return ds;
                }

                static hid_t extend(hid_t dataSet, hid_t dataSpace, hsize_t len) {
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

                static hid_t selectScalarRecord(hid_t dataSpace, hsize_t recordId, hsize_t len = 1) {
                    hsize_t start[] = {recordId};
                    hsize_t count[] = {len};
                    herr_t status = H5Sselect_hyperslab(dataSpace, H5S_SELECT_SET, start, NULL, count, NULL);
                    KARABO_CHECK_HDF5_STATUS(status);
                    return dataSpace;

                }

                static hid_t selectRecord(hid_t dataSpace, hsize_t recordId, hsize_t len = 1) {
                    int ndims = H5Sget_simple_extent_ndims(dataSpace);
                    KARABO_CHECK_HDF5_STATUS(ndims);

                    std::vector<hsize_t> start(ndims, 0);
                    start[0] = recordId;

                    std::vector<hsize_t> count(ndims, 0);
                    std::vector<hsize_t> maxExtent(ndims, 0);
                    int status = H5Sget_simple_extent_dims(dataSpace, &count[0], &maxExtent[0]);
                    KARABO_CHECK_HDF5_STATUS(status);
                    count[0] = len;
                    herr_t st = H5Sselect_hyperslab(dataSpace, H5S_SELECT_SET, &start[0], NULL, &count[0], NULL);
                    KARABO_CHECK_HDF5_STATUS(st);
                    return dataSpace;
                }


                virtual void open( hid_t group);
                
                virtual void close();
                
                int m_compressionLevel;

                hid_t m_dataSet;


                hid_t m_fileDataSpace;
                hsize_t m_chunkSize;
                hid_t m_dataSetProperties;


            };



        }
    }
}

#endif	
