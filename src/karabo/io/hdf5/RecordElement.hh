/*
 * $Id: RecordElement.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */




#ifndef EXFEL_IO_RECORDELEMENT_HH
#define	EXFEL_IO_RECORDELEMENT_HH

#include <karabo/util/Factory.hh>
#include <karabo/util/Hash.hh>
#include <hdf5/hdf5.h>
#include <hdf5/H5Cpp.h>
#include <boost/enable_shared_from_this.hpp>
#include "../iodll.hh"


namespace exfel {
    namespace io {

        /*
         * Class representing hdf5 chunked dataset.
         * In Hdf5 file each dataset contains records with values of appropriate types as implemented by derived classes.
         * These can be integral or floating data types, strings, booleans or corresponding fixed length arrays.
         * Variable length arrays need to be added (should be easy). Compound data types (structres) needs to be supported
         * as well.
         * This class support creation and opening dataset, writing and reading data to and from appropriate record.
         * This class should be used only by H5Table class and must not be used in client code.
         */
        class RecordElement : public boost::enable_shared_from_this<RecordElement> {
        public:
            EXFEL_CLASSINFO(RecordElement, "RecordElement", "1.0")
            EXFEL_FACTORY_BASE_CLASS


            static void expectedParameters(exfel::util::Schema& expected);
            void configure(const exfel::util::Hash& input);

            RecordElement() {
                //      m_writeFilter = boost::shared_ptr<WriteFilter > ();
            }

            virtual ~RecordElement() {
            }

            /**
             * Get dataset name
             * @return name dataset name
             */
            const std::string& getName();

            /** 
             * Get Hash representation of this RecordElement.
             * 
             * @param Hash this object will be filled by the function. 
             * Key is equal to dataset name, value is this object.
             * In case nested groups are defined, for each group an instance of Hash is created.
             * i.e. if dataset is "a.b.c" -> hash looks like:
             * a => Hash
             *   b => Hash
             *     c => RecordElement
             */
            virtual void getElement(exfel::util::Hash& element);

            /**
             * Create UNLIMITED CHUNKED HDF5 dataset.
             * @param group Hdf5 group where the dataset belongs to.
             * @param chunkSize Chunk size as defined by hdf5
             */
            virtual void create(boost::shared_ptr<H5::Group> group, hsize_t chunkSize) = 0;


            /**
             * Extend CHUNKED HDF5 dataset.
             * @param size The size of extended space corresponding to the number of newly added records
             */
            virtual void extend(hsize_t size);


            /**
             * Open existing HDF5 dataset.
             * @param group Hdf5 group where the dataset belongs to.
             */
            virtual void open(boost::shared_ptr<H5::Group> group);


            /**
             * Write data to dataset. Hash structure must contain key and value pair.
             * The key is the name of the dataset, value must correspond to the type as defined at the dataset creation time
             * 
             * @param data Hash with data to be written.
             * @param recordId Record number (numbering starts from 0)
             */
            virtual void write(const exfel::util::Hash& data, hsize_t recordId) = 0;



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
            virtual void write(const exfel::util::Hash& data, hsize_t recordId, hsize_t len) = 0;

            /**
             * Allocate memory for single record
             * If the entry in the Hash does not exist, this function must allocate memory to hold the complete dataset
             * If the entry exist assume the memory is allocated. This can be used when client delivers own buffers.
             * @param data Hash where the data will be stored when using read function
             */
            virtual void allocate(exfel::util::Hash& data) = 0;

            /**
             * allocate memory for len number of records
             * @param data Hash where the data will be stored when using read function
             * @param len number of records to be allocated
             */
            virtual void allocate(exfel::util::Hash& buffer, size_t len) = 0;

            /*
             * Read data from the dataset. Hash structure is filled with the key, value pair.
             * The key is the name of the dataset, value is read from file.
             * The data Hash structure must already contain corresponding key and value.
             * Therefore this function can be used with binding variables (references)
             * from client code.
             *
             * @param data Hash to be filled
             * @param recordId Record number (numbering starts from 0)
             */
            virtual void read(exfel::util::Hash& data, hsize_t recordId) = 0;


            virtual void read(exfel::util::Hash& data, hsize_t recordId, hsize_t len) = 0;



            virtual void readAttributes(exfel::util::Hash& attributes);

            //      void addFilter(exfel::io::WriteFilter::Pointer filter) {
            //        m_writeFilter->add(filter);
            //      }


            /**
             * Get number of records in the hdf5 dataset
             * @return number of records
             */
            hsize_t getNumberOfRecords();

            hsize_t getChunkSize();


        protected:

            std::string m_key; // dataset name
            std::string m_relativeGroup; // nested group if defined 
            int m_compressionLevel;
            bool m_implicitConversion; // should the value be converted if necessary and possible

            H5::DataSet m_dataSet;
            H5::DataSpace m_memoryDataSpace;
            H5::DataSpace m_fileDataSpace;

            boost::shared_ptr<H5::Group> m_group;
            boost::shared_ptr<H5::DSetCreatPropList> m_dataSetProperties;


            // functions      
            static H5::DataSpace scalarFileDataSpace(hsize_t size);

            H5::DataSpace scalarDataSpace();

            virtual void createDataSetProperties(hsize_t chunkSize);

            virtual void selectFileRecord(hsize_t recordId, hsize_t len = 1);

            virtual void readSpecificAttributes(exfel::util::Hash& attributes);

            inline static H5::DataSpace getBufferDataSpace(hsize_t len) {
                hsize_t dims[1];
                dims[0] = len;
                hsize_t maxdims[] = {len};
                return H5::DataSpace(1, dims, maxdims);
            }


        private:

        };
    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::RecordElement, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_RECORDELEMENT_HH */

