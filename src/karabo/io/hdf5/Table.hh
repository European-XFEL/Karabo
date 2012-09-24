/*
 * $Id: Table.hh 6095 2012-05-08 10:05:56Z boukhele $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_IO_TABLE_HH
#define	EXFEL_IO_TABLE_HH

#include "File.hh"
#include "DataFormat.hh"
#include "RecordFormat.hh"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <hdf5/hdf5.h>
#include <hdf5/H5Cpp.h>
#include <hdf5/hdf5_hl.h>
#include <karabo/util/Time.hh>
#include "../iodll.hh"


/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package io
     */
    namespace io {

        namespace hdf5 {

            /**
             Table is an entity defined to store set of records...
             */
            class Table {
                friend class File;
                template<class T> friend class Column;

            public:

                EXFEL_CLASSINFO(Table, "Hdf5", "1.0")
                EXFEL_FACTORY_BASE_CLASS


                /**
                 * Default, empty constructor. 
                 */
                Table() : m_recordFormatVector(), m_useCache(true), m_cacheStart(0), m_cacheEnd(0), m_cacheSize(0) {
                }

                virtual ~Table();


                /**
                 * Defines expected parameters. This function is part of the factory mechanism implemented in exfel::util package.
                 * @param expected Schema object which is filled definition of expected parameters
                 */
                static void expectedParameters(exfel::util::Schema& expected);

                /**
                 * Configures newly created object
                 * @param input validated configuration data
                 */
                void configure(const exfel::util::Hash& input);



                //void select(exfel::util::Hash& activeElementsHash);

                /*
                 * Write a single record to the table at position recordNumber
                 * @param data Hash object representing data record structure in file
                 * @param recordNumber record numbers in table start from zero
                 */
                void write(const exfel::util::Hash& data, size_t recordNumber);


                /**
                 * Write len number of records starting at record recordNumber.
                 * All existing records are replaced.
                 * Each element of the hash must be a std::vector (?filters) of appropriate 
                 * type with len number of elements
                 */
                void writeBuffer(const exfel::util::Hash& data, size_t recordNumber, size_t len);

                /**
                 * Append data record to the end of the table.
                 * @param data Hash object representing data record structure in file
                 */
                void append(const exfel::util::Hash& data);


                void allocate(exfel::util::Hash& data);

                void allocate(exfel::util::Hash& data, size_t len);

                /**
                 * Read data record from the table.
                 * @param data Hash object filled with values read from the table.
                 * @param recordNumber Number identifying record to be read. Record numbers start from 0.
                 */
                void read(exfel::util::Hash& data, size_t recordNumber);

                /**
                 * Buffered reading
                 * @param data
                 * @param recordNumber
                 * @param 
                 */
                void readBuffer(exfel::util::Hash& data, size_t recordNumber, size_t len);

                void read(size_t recordNumber);
                void readAttributes(exfel::util::Hash & attr);

                size_t getNumberOfRecords();

                void close();

            private:

                void openNew(const exfel::io::hdf5::DataFormat::Pointer dataFormat);
                //void openReadOnly(const exfel::util::Hash& dataFormatConfig = exfel::util::Hash());
                void openReadOnly(const exfel::io::hdf5::DataFormat::Pointer dataFormat);
                void openReadOnly();

                void refreshRecordFormatVector();
                void r_refreshRecordFormatVector(const exfel::util::Hash& recordFormat, std::vector< const boost::any*>& recordVector);

                void r_defineStructure(const exfel::util::Hash& recordFormat, boost::shared_ptr<H5::Group> group);
                void r_openStructure(const exfel::util::Hash& recordFormat, boost::shared_ptr<H5::Group> group);
                void r_write(const exfel::util::Hash& data, size_t recordNumber, const exfel::util::Hash& dataSetsInfo);
                void r_write(const exfel::util::Hash& data, size_t recordNumber, size_t len, const exfel::util::Hash& dataSetsInfo);
                void r_read(exfel::util::Hash& data, size_t recordNumber, const exfel::util::Hash& dataSetsDescription);
                void r_read(exfel::util::Hash& data, size_t recordNumber, size_t len, const exfel::util::Hash& dataSetsDescription);
                void r_readAttributes(exfel::util::Hash & attributes, const exfel::util::Hash& dataSetsDescription);
                void r_extendRecordSpace(size_t len, const exfel::util::Hash& recordDescription);
                void r_filter(const exfel::util::Hash& discovered, const exfel::util::Hash& selection, exfel::util::Hash& output);
                hsize_t r_calculateNumberOfRecords(const exfel::util::Hash& recordFormat, bool firstTime = true);
                hsize_t r_getChunkSize(const exfel::util::Hash& recordFormat, bool firstTime = true);

                void r_allocate(exfel::util::Hash& data, const exfel::util::Hash& dataSetsDescription);
                void r_allocate(exfel::util::Hash& data, size_t len, const exfel::util::Hash& dataSetsDescription);

                void r_copyFromCache(exfel::util::Hash& data, size_t recordNumber, const exfel::util::Hash& recordFormat);
                // calback method for used by discover function
                static herr_t fileInfo(hid_t loc_id, const char *name, const H5O_info_t *info, void *opdata);
                void discover(exfel::io::hdf5::DataFormat::Pointer& discovered, std::string groupName);


                void updateNumberOfRecordsAttribute();
                void retrieveNumberOfRecordsFromFile();
                void calculateNumberOfRecords();
                void retrieveChunkSizeFromFile();
                void createEmptyTable(boost::shared_ptr<H5::H5File> h5file, const boost::filesystem::path& fullPath);
                void createInitialNumberOfRecordsAttribute();
                void createSchemaVersionAttribute();

                void saveTableFormatAsAttribute(const exfel::io::hdf5::DataFormat::Pointer dataFormat);
                void readTableFormatFromAttribute(exfel::util::Hash& dataFormatConfig);
                void defineRecordFormat(const exfel::io::hdf5::DataFormat::Pointer dataFormat);
                void openRecordStructure();


                bool hasAttribute(const boost::shared_ptr<H5::Group> group, const std::string& name) const;

                template <class T>
                std::vector<T>& getCacheVector(const std::string& key) {
                    return m_cache.getFromPath<std::vector<T> >(key, "/");
                }

                template <class T>
                exfel::io::ArrayView<T>& getCache(const std::string& key) {
                    return m_cache.getFromPath<exfel::io::ArrayView<T> >(key, "/");
                }
                
                inline size_t updateCache(size_t recordNumber) {
                    if (recordNumber >= m_cacheEnd || recordNumber < m_cacheStart) {
                        refreshCache(recordNumber);
                    }
                    return (recordNumber - m_cacheStart);
                }

                void refreshCache(size_t recordNumber);

                void initializeCache();


                // pointer to H5::H5File object where this table belongs to.
                boost::shared_ptr<H5::H5File> m_h5file;
                // table name, i.e.: /Data/Bla
                boost::filesystem::path m_name;

                boost::shared_ptr<H5::Group> m_group;




                boost::shared_ptr<DataFormat> m_dataFormat;


                // selected record format in Hash representation
                exfel::util::Hash m_recordFormatHash;

                // we can try to optimize it  using std::vector<boost::any*> m_recordFormatVector; 
                // which would contain pointers to Hash elements 
                // it must be recalculated whenever m_recordFormatHash is updated
                std::vector<const boost::any*> m_recordFormatVector;

                // activated elements in Hash representation
                exfel::util::Hash m_activatedElements; // not used at the moment

                exfel::util::Hash m_readData;

                hsize_t m_chunkSize;
                hsize_t m_numberOfRecords;
                H5::Attribute m_numberOfRecordsAttribute;



                // caching (read ahead)
                bool m_useCache;
                exfel::util::Hash m_cache;
                unsigned long long m_cacheStart;
                unsigned long long m_cacheEnd;
                unsigned long long m_cacheSize;


            };

        }
    }
}

EXFEL_REGISTER_FACTORY_BASE_HH(exfel::io::hdf5::Table, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* EXFEL_IO_TABLE_HH */
