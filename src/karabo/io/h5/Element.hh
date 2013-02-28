/*
 * $Id: Format.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */





#ifndef KARABO_IO_H5_ELEMENT_HH
#define	KARABO_IO_H5_ELEMENT_HH


#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/ToLiteral.hh>

#include <karabo/util/Types.hh>
#include <karabo/util/FromTypeInfo.hh>

#include <hdf5/hdf5.h>
#include <hdf5/H5Cpp.h>

#include <boost/enable_shared_from_this.hpp>


namespace karabo {
    namespace io {
        namespace h5 {

            class Table;

            class Element : public boost::enable_shared_from_this<Element> {
            public:
                KARABO_CLASSINFO(Element, "Element", "1.0");
                KARABO_CONFIGURATION_BASE_CLASS

                static void expectedParameters(karabo::util::Schema& expected);

                void configure(const karabo::util::Hash& input);

                Element(const karabo::util::Hash& input);

                virtual ~Element() {
                }

                /**
                 * Get element name. Element can represent hdf5 group or dataset
                 * @return name element name
                 */
                const std::string& getName();

                /** 
                 * Get Hash representation of this Element.
                 * 
                 * @param Hash this object will be filled by the function. 
                 * Key is equal to element name, value is this object.
                 * In case nested groups are defined, for each group an instance of Hash is created.
                 * i.e. if dataset is "a.b.c" -> hash looks like:
                 * a => Hash
                 *   b => Hash
                 *     c => RecordElement
                 */
                virtual void getElement(karabo::util::Hash& element);

                /**
                 * Create UNLIMITED CHUNKED HDF5 dataset.
                 * @param group Hdf5 group where the dataset belongs to.
                 * @param chunkSize Chunk size as defined by hdf5
                 */
                virtual void create(hsize_t chunkSize) = 0;

                void openParentGroup(std::map<std::string, boost::shared_ptr<H5::Group> >& groups);



                //
                //
                //                /**
                //                 * Extend CHUNKED HDF5 dataset.
                //                 * @param size The size of extended space corresponding to the number of newly added records
                //                 */
                //                virtual void extend(hsize_t size);
                //
                //
                //                /**
                //                 * Open existing HDF5 dataset.
                //                 * @param group Hdf5 group where the dataset belongs to.
                //                 */
                //                virtual void open(boost::shared_ptr<H5::Group> group);
                //
                //
                //                /**
                //                 * Write data to dataset. Hash structure must contain key and value pair.
                //                 * The key is the name of the dataset, value must correspond to the type as defined at the dataset creation time
                //                 * 
                //                 * @param data Hash with data to be written.
                //                 * @param recordId Record number (numbering starts from 0)
                //                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId) = 0;
                //
                //

                /**
                 * Write many records of data to a dataset (buffered writing).
                 * The value of the Hash must be a vector(?) of values of type as defined at the dataset creation time.
                 * The length of the vector must be at least len.
                 * The key is the name of the dataset, value must correspond to the type as defined at the dataset creation time
                 * 
                 *
                 * @param data Hash with data to be written.
                 * @param recordId Record number (numbering starts from 0)
                 * @param len Number of values to be written
                 */
                virtual void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) = 0;
                //
                //                /**
                //                 * Allocate memory for single record
                //                 * If the entry in the Hash does not exist, this function must allocate memory to hold the complete dataset
                //                 * If the entry exist assume the memory is allocated. This can be used when client delivers own buffers.
                //                 * @param data Hash where the data will be stored when using read function
                //                 */
                //                virtual void allocate(karabo::util::Hash& data) = 0;
                //
                //                /**
                //                 * allocate memory for len number of records
                //                 * @param data Hash where the data will be stored when using read function
                //                 * @param len number of records to be allocated
                //                 */
                //                virtual void allocate(karabo::util::Hash& buffer, size_t len) = 0;
                //
                //                /*
                //                 * Read data from the dataset. Hash structure is filled with the key, value pair.
                //                 * The key is the name of the dataset, value is read from file.
                //                 * The data Hash structure must already contain corresponding key and value.
                //                 * Therefore this function can be used with binding variables (references)
                //                 * from client code.
                //                 *
                //                 * @param data Hash to be filled
                //                 * @param recordId Record number (numbering starts from 0)
                //                 */
                //                virtual void read(karabo::util::Hash& data, hsize_t recordId) = 0;
                //
                //
                //                virtual void read(karabo::util::Hash& data, hsize_t recordId, hsize_t len) = 0;
                //
                //
                //
                //                virtual void readAttributes(karabo::util::Hash& attributes);
                //
                //                //      void addFilter(karabo::io::WriteFilter::Pointer filter) {
                //                //        m_writeFilter->add(filter);
                //                //      }
                //
                //
                //                /**
                //                 * Get number of records in the hdf5 dataset
                //                 * @return number of records
                //                 */
                //                hsize_t getNumberOfRecords();
                //
                //                hsize_t getChunkSize();
                //



            protected:

                std::string m_key; // name of this element
                std::string m_path; // path to the parent of this element from the root of the table (/ as separator)
                std::string m_path_key;               
                boost::shared_ptr<H5::Group> m_group; // parent group of this element

            };


        }
    }
}


#endif

