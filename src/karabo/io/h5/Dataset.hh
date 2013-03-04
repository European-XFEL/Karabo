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

#include "Element.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>

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
                    H5Pset_layout(m_dataSetProperties, H5D_CHUNKED);
                    if (m_compressionLevel > 0) {
                        //         m_dataSetProperties->setShuffle();
                        H5Pset_deflate(m_dataSetProperties, m_compressionLevel);
                    }

                    H5Pset_chunk(m_dataSetProperties, chunkDims.rank(), &(chunkDims.toVector())[0]);
                }

                static hid_t dataSpace(karabo::util::Dims& dims) {
                    std::vector<hsize_t> curdims(dims.rank(), 0);
                    std::vector<hsize_t> maxdims(dims.rank(), 0);
                    for (size_t i = 0; i < dims.rank(); ++i) {
                        curdims[i] = dims.extentIn(i);
                        maxdims[i] = curdims[i];
                    }
                    maxdims[0] = H5S_UNLIMITED;
                    return H5Screate_simple(dims.rank(), &curdims[0], &maxdims[0]);


                }

                static hid_t dataSpace() {
                    hsize_t dims[] = {1};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    return H5Screate_simple(1, dims, maxdims);
                }

                static hid_t dataSpace(hsize_t len) {
                    hsize_t dims[] = {len};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    return H5Screate_simple(1, dims, maxdims);
                }

                static hid_t extend(hid_t dataSet, hid_t dataSpace, hsize_t len) {
                    int ndims = H5Sget_simple_extent_ndims(dataSpace);
                    std::vector<hsize_t> extent(ndims, 0);
                    std::vector<hsize_t> maxExtent(ndims, 0);
                    H5Sget_simple_extent_dims(dataSpace, &extent[0], &maxExtent[0]);
                    //                    for (int i = 0; i < ndims; ++i) {
                    //                        std::clog << "extent[" << i << "] = " << extent[i] << std::endl;
                    //                        std::clog << "maxExtent[" << i << "] = " << maxExtent[i] << std::endl;
                    //                    }
                    extent[0] += len;
                    H5Dset_extent(dataSet, &extent[0]);
                    dataSpace = H5Dget_space(dataSet);

                    //                    H5Sget_simple_extent_dims(dataSpace, &extent[0], &maxExtent[0]);
                    //                    for (int i = 0; i < ndims; ++i) {
                    //                        std::clog << "after extent[" << i << "] = " << extent[i] << std::endl;
                    //                        std::clog << "maxExtent[" << i << "] = " << maxExtent[i] << std::endl;
                    //                    }
                    return dataSpace;
                }

                static hid_t selectScalarRecord(hid_t dataSpace, hsize_t recordId, hsize_t len = 1) {
                    hsize_t start[] = {recordId};
                    hsize_t count[] = {len};
                    H5Sselect_hyperslab(dataSpace, H5S_SELECT_SET, start, NULL, count, NULL);
                    return dataSpace;

                }

                static hid_t selectRecord(hid_t dataSpace, hsize_t recordId, hsize_t len = 1) {
                    int ndims = H5Sget_simple_extent_ndims(dataSpace);
                    if( ndims < 0 ) {
                        throw KARABO_HDF_IO_EXCEPTION("Could not obtain rank of data space");
                    }
                    std::vector<hsize_t> start(ndims, 0);
                    start[0] = recordId;

                    std::vector<hsize_t> count(ndims, 0);
                    std::vector<hsize_t> maxExtent(ndims, 0);
                    int status = H5Sget_simple_extent_dims(dataSpace, &count[0], &maxExtent[0]);
                    if (status < 0 ){
                           throw KARABO_HDF_IO_EXCEPTION("Could not obtain extents of data space");
                    }
                    //                    for (int i = 0; i < ndims; ++i) {
                    //                        std::clog << "selectRecord count[" << i << "] = " << count[i] << std::endl;
                    //                    }
                    count[0] = len;
                    //                    for (int i = 0; i < ndims; ++i) {
                    //                        std::clog << "selectRecord after count[" << i << "] = " << count[i] << std::endl;
                    //                    }
                    herr_t st = H5Sselect_hyperslab(dataSpace, H5S_SELECT_SET, &start[0], NULL, &count[0], NULL);
                    if (st < 0 ){
                           throw KARABO_HDF_IO_EXCEPTION("Could not select hyperslab");
                    }
                    return dataSpace;
                }
                

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
