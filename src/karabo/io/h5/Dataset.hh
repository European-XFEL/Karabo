/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASET_HH
#define	KARABO_IO_H5_DATASET_HH

#include <string>

#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <karabo/log/Logger.hh>

#include "Element.hh"
#include "ErrorHandler.hh"

#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            class Dataset : public karabo::io::h5::Element {

            public:

                KARABO_CLASSINFO(Dataset, "Dataset", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                Dataset(const karabo::util::Hash& input);

                virtual ~Dataset() {
                }



                void write(const karabo::util::Hash& data, hsize_t recordId);

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len);

            protected:
                virtual void writeNode(const karabo::util::Hash::Node& data,
                                       hid_t dataSet, hid_t fileDataSpace) = 0;

                virtual void writeNode(const karabo::util::Hash::Node& data, hsize_t len,
                                       hid_t dataSet, hid_t fileDataSpace) = 0;

            public:

                void read(hsize_t recordId);
                
                void read(hsize_t recordId, hsize_t len);

            protected:

                virtual void readRecord(const hid_t& dataSet, const hid_t& fileDataSpace) = 0;
                
                virtual void readRecords(hsize_t len, const hid_t& dataSet, const hid_t& fileDataSpace) = 0;

            public:

                static void getDataSpaceInfo(hid_t dataSpace, const std::string& msg) {
                    int ndims = H5Sget_simple_extent_ndims(dataSpace);
                    KARABO_CHECK_HDF5_STATUS(ndims);
                    std::vector<hsize_t> extent(ndims, 0);                   
                    std::vector<hsize_t> maxExtent(ndims, 0);
                    KARABO_CHECK_HDF5_STATUS(H5Sget_simple_extent_dims(dataSpace, &extent[0], &maxExtent[0]));
                }

                static hid_t dataSpace(const karabo::util::Dims& dims);

                static hid_t dataSpace1dim(hsize_t len);

                void create(hsize_t chunkSize);

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


                virtual void open(hid_t group);

                virtual void close();

                hid_t m_dataSet;




            private:
                int m_compressionLevel;
                hsize_t m_numberAllocatedRecords;

                karabo::util::Dims m_dims; // dimension of written/read objects (= dataset dims minus one)

                // description of file data space
                std::vector<hsize_t> m_dataSetExtent;
                std::vector<hsize_t> m_dataSetMaxExtent;

                hsize_t m_chunkSize;
                hid_t m_dataSetProperties;

                hid_t m_fileDataSpace;

            private:

                void configureDataDimensions(const karabo::util::Hash& input);
                void configureFileDataSpace(const karabo::util::Hash& input);
                void createDataSetProperties();

            };



        }
    }
}

#endif	
