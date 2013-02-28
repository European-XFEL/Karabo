/*
 * $Id: Scalar.hh 5491 2012-03-09 17:27:25Z wrona $
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

                H5::DataSpace getBufferDataSpace(hsize_t len) {
                    hsize_t dims[] = {len};
                    hsize_t maxdims[] = {len};
                    return H5::DataSpace(1, dims, maxdims);
                }

                void createDataSetProperties(hsize_t chunkSize) {
                    m_dataSetProperties = boost::shared_ptr<H5::DSetCreatPropList > (new H5::DSetCreatPropList());
                    if (m_compressionLevel > 0) {
                        //         m_dataSetProperties->setShuffle();
                        m_dataSetProperties->setDeflate(m_compressionLevel);
                    }
                    hsize_t chunkDims[1] = {chunkSize};
                    m_dataSetProperties->setChunk(1, chunkDims);
                }

                H5::DataSpace scalarFileDataSpace(hsize_t size) {
                    hsize_t dims[] = {size};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    return H5::DataSpace(1, dims, maxdims);
                }

                void selectFileRecord(hsize_t recordId, hsize_t len = 1) {
                    hsize_t start[] = {recordId};
                    hsize_t count[] = {len};
                    m_fileDataSpace.selectHyperslab(H5S_SELECT_SET, count, start, NULL, NULL);
                }

                void extend(hsize_t size) {
                    hsize_t currentSizeDims[1];
                    m_fileDataSpace.getSimpleExtentDims(currentSizeDims);
                    hsize_t newSizeDims[1];
                    newSizeDims[0] = currentSizeDims[0] + size;
                    m_dataSet.extend(newSizeDims);
                    m_fileDataSpace = m_dataSet.getSpace();
                }


                ///////
                
                void createDataSetProperties(karabo::util::Dims& chunkDims) {
                    std::clog << "200" << std::endl;
                    m_dataSetProperties = boost::shared_ptr<H5::DSetCreatPropList > (new H5::DSetCreatPropList());
                    std::clog << "201" << std::endl;
                    if (m_compressionLevel > 0) {
                        //         m_dataSetProperties->setShuffle();
                        m_dataSetProperties->setDeflate(m_compressionLevel);
                    }
                    std::clog << "202" << std::endl;
                   
                   // hsize_t chunkDims[1] = {chunkSize};
                    m_dataSetProperties->setChunk(chunkDims.rank(), &(chunkDims.toVector())[0] );
                    std::clog << "203" << std::endl;
                }
                

                static H5::DataSpace dataSpace(karabo::util::Dims& dims) {
                    std::vector<hsize_t> curdims(dims.rank(),0);
                    std::vector<hsize_t> maxdims(dims.rank(),0);
                    for (size_t i = 0; i < dims.rank(); ++i) {
                        curdims[i] = dims.extentIn(i);
                        maxdims[i] = curdims[i];
                    }
                    maxdims[0] = H5S_UNLIMITED;
                    return H5::DataSpace(dims.rank(), &curdims[0], &maxdims[0]);                    
                }

                static H5::DataSpace dataSpace() {
                    hsize_t dims[] = {1};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    return H5::DataSpace(1, dims, maxdims);
                }

                static H5::DataSpace dataSpace(hsize_t len) {
                    hsize_t dims[] = {len};
                    hsize_t maxdims[] = {H5S_UNLIMITED};
                    return H5::DataSpace(1, dims, maxdims);
                }

                static H5::DataSpace& extend(H5::DataSet& dataSet, H5::DataSpace& dataSpace, hsize_t len) {
                    int ndims = dataSpace.getSimpleExtentNdims();
                    std::vector<hsize_t> extent(ndims, 0);
                    dataSpace.getSimpleExtentDims(&extent[0]);
                    extent[0] += len;
                    dataSet.extend(&extent[0]);
                    dataSpace = dataSet.getSpace();
                    return dataSpace;
                }
                

                static void selectScalarRecord(H5::DataSpace& dataSpace, hsize_t recordId, hsize_t len = 1) {
                    hsize_t start[] = {recordId};
                    hsize_t count[] = {len};
                    dataSpace.selectHyperslab(H5S_SELECT_SET, count, start, NULL, NULL);
                }

                static H5::DataSpace& selectRecord(H5::DataSpace& dataSpace, hsize_t recordId, hsize_t len = 1) {
                    int ndims = dataSpace.getSimpleExtentNdims();
                    std::vector<hsize_t> start(ndims, 0);
                    start[0] = recordId;
                    
                    std::vector<hsize_t> count(ndims, 0);
                    dataSpace.getSimpleExtentDims(&count[0]);
                    count[0] = len;                                        
                    dataSpace.selectHyperslab(H5S_SELECT_SET, &count[0], &start[0], NULL, NULL);
                    return dataSpace;
                }





                //            private:

                int m_compressionLevel;

                H5::DataSet m_dataSet;
                //H5::DataSpace m_memoryDataSpace;
                H5::DataSpace m_fileDataSpace;
                hsize_t m_chunkSize;
                boost::shared_ptr<H5::DSetCreatPropList> m_dataSetProperties;


            };



        }
    }
}

#endif	
