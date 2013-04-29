/*
 * $Id$
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
                const std::string& getFullName();

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

                void getConfig(karabo::util::Hash& config) {

                    config = m_config;
                }
                /**
                 * Create UNLIMITED CHUNKED HDF5 dataset.
                 * @param group Hdf5 group where the dataset belongs to.
                 * @param chunkSize Chunk size as defined by hdf5
                 */
                virtual void create(hsize_t chunkSize) = 0;

                void openParentGroup(std::map<std::string, hid_t >& groups);

                /**
                 * Open existing HDF5 dataset.
                 * @param group Hdf5 group where the dataset belongs to.
                 */
                virtual void open(hid_t group) = 0;

                virtual void close() = 0;

                //                /**
                //                 * Write data to dataset. Hash structure must contain key and value pair.
                //                 * The key is the name of the dataset, value must correspond to the type as defined at the dataset creation time
                //                 * 
                //                 * @param data Hash with data to be written.
                //                 * @param recordId Record number (numbering starts from 0)
                //                 */
               virtual void write(const karabo::util::Hash& data, hsize_t recordId) = 0;

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
                /**
                 * Allocate memory for single record
                 * If the entry in the Hash does not exist, this function must allocate memory to hold the complete dataset
                 * If the entry exist assume the memory is allocated. This can be used when client delivers own buffers.
                 * @param data Hash where the data will be stored when using read function
                 */
                virtual void bind(karabo::util::Hash& data) = 0;
                
                /**
                 * allocate memory for len number of records
                 * @param data Hash where the data will be stored when using read function
                 * @param len number of records to be allocated
                 */
                virtual void bind(karabo::util::Hash& buffer, hsize_t len) = 0;
                
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
               
                
                virtual void read( hsize_t recordId) = 0;
                
                
                virtual void read( hsize_t recordId, hsize_t len) = 0;
                //
                //
                //
                //                virtual void readAttributes(karabo::util::Hash& attributes);
                //
                //                //      void addFilter(karabo::io::WriteFilter::Pointer filter) {
                //                //        m_writeFilter->add(filter);
                //                //      }



            protected:

                std::string m_h5name; // name of this element in hdf5 file               
                std::string m_h5path; // path to the parent of this element from the root of the table (/ as separator)
                std::string m_h5PathName;
                std::string m_key; // key  (including path) to the data element in hash                
                hid_t m_parentGroup; // parent group of this element
                karabo::util::Hash m_config;
            };


        }
    }
}


#endif

