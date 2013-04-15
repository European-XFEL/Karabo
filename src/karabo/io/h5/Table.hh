/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_TABLE_HH
#define	KARABO_IO_H5_TABLE_HH

#include "File.hh"
//#include "DataFormat.hh"
#include "Format.hh"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <hdf5/hdf5.h>
#include <karabo/util/Time.hh>
#include "ioProfiler.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             Table is an entity defined to store set of records...
             */
            class Table {

                friend class File;
                //                template<class T> friend class Column;


            public:

                KARABO_CLASSINFO(Table, "Table", "1.0")
                //KARABO_CONFIGURATION_BASE_CLASS

                typedef std::map<std::string, hid_t > H5GroupsMap;

                Table(hid_t h5file, boost::filesystem::path name, hsize_t chunkSize = 1)
                : m_h5file(h5file), m_name(name), m_chunkSize(chunkSize), m_numberOfRecords(0)
                #ifdef KARABO_USE_PROFILER_TABLE1
                , table1("table1")
                #endif
                {
                }

                virtual ~Table();



                //void select(karabo::util::Hash& activeElementsHash);

                /*
                 * Write a single record to the table at position recordNumber
                 * @param data Hash object representing data record structure in file
                 * @param recordNumber record numbers in table start from zero
                 */
                void write(const karabo::util::Hash& data, size_t recordNumber);

                void write(const karabo::util::Hash& data, size_t recordNumber, size_t len);


                //                /**
                //                 * Write len number of records starting at record recordNumber.
                //                 * All existing records are replaced.
                //                 * Each element of the hash must be a std::vector (?filters) of appropriate 
                //                 * type with len number of elements
                //                 */
                //                void writeBuffer(const karabo::util::Hash& data, size_t recordNumber, size_t len);
                //

                /**
                 * Append data record to the end of the table.
                 * @param data Hash object representing data record structure in file
                 */
                void append(const karabo::util::Hash& data);


                void bind(karabo::util::Hash& data);

                //                void allocate(karabo::util::Hash& data, size_t len);

                /**
                 * Read data record from the table.
                 * @param data Hash object filled with values read from the table.
                 * @param recordNumber Number identifying record to be read. Record numbers start from 0.
                 */
                void read(size_t recordNumber);

                //                /**
                //                 * Buffered reading
                //                 * @param data
                //                 * @param recordNumber
                //                 * @param 
                //                 */
                //                void readBuffer(karabo::util::Hash& data, size_t recordNumber, size_t len);
                //
                //                void read(size_t recordNumber);
                //                void readAttributes(karabo::util::Hash & attr);

                size_t getNumberOfRecords();

                void close();
                //
                //            private:
                //
                void openNew(const karabo::io::h5::Format::Pointer dataFormat);
                //                //void openReadOnly(const karabo::util::Hash& dataFormatConfig = karabo::util::Hash());
                void openReadOnly(const karabo::io::h5::Format::Pointer dataFormat);
                void openReadOnly();
                //
                //                void refreshRecordFormatVector();
                //                void r_refreshRecordFormatVector(const karabo::util::Hash& recordFormat, std::vector< const boost::any*>& recordVector);
                //
                //void r_defineStructure(const karabo::util::Hash& recordFormat, boost::shared_ptr<H5::Group> group);
                void defineStructure();
                //                void r_openStructure(const karabo::util::Hash& recordFormat, boost::shared_ptr<H5::Group> group);
                //                void r_write(const karabo::util::Hash& data, size_t recordNumber, const karabo::util::Hash& dataSetsInfo);
                //                void r_write(const karabo::util::Hash& data, size_t recordNumber, size_t len, const karabo::util::Hash& dataSetsInfo);
                //                void r_read(karabo::util::Hash& data, size_t recordNumber, const karabo::util::Hash& dataSetsDescription);
                //                void r_read(karabo::util::Hash& data, size_t recordNumber, size_t len, const karabo::util::Hash& dataSetsDescription);
                //                void r_readAttributes(karabo::util::Hash & attributes, const karabo::util::Hash& dataSetsDescription);
                //                void r_extendRecordSpace(size_t len, const karabo::util::Hash& recordDescription);
                //                void r_filter(const karabo::util::Hash& discovered, const karabo::util::Hash& selection, karabo::util::Hash& output);
                //                hsize_t r_calculateNumberOfRecords(const karabo::util::Hash& recordFormat, bool firstTime = true);
                //                hsize_t r_getChunkSize(const karabo::util::Hash& recordFormat, bool firstTime = true);
                //
                //                void r_allocate(karabo::util::Hash& data, const karabo::util::Hash& dataSetsDescription);
                //                void r_allocate(karabo::util::Hash& data, size_t len, const karabo::util::Hash& dataSetsDescription);
                //
                //                void r_copyFromCache(karabo::util::Hash& data, size_t recordNumber, const karabo::util::Hash& recordFormat);
                //                // calback method for used by discover function
                //                static herr_t fileInfo(hid_t loc_id, const char *name, const H5O_info_t *info, void *opdata);
                //                void discover(karabo::io::hdf5::DataFormat::Pointer& discovered, std::string groupName);
                //
                //
                void updateNumberOfRecordsAttribute();
                void retrieveNumberOfRecordsFromFile();
                //                void calculateNumberOfRecords();
                //                void retrieveChunkSizeFromFile();
                void createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath);
                void createInitialNumberOfRecordsAttribute();
                void createSchemaVersionAttribute();

                void saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat);
                void readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig);

                //
                //
                bool hasAttribute(hid_t group, const std::string& name) const;
                //
                //                template <class T>
                //                std::vector<T>& getCacheVector(const std::string& key) {
                //                    return m_cache.getFromPath<std::vector<T> >(key, "/");
                //                }
                //
                //                template <class T>
                //                karabo::io::ArrayView<T>& getCache(const std::string& key) {
                //                    return m_cache.getFromPath<karabo::io::ArrayView<T> >(key, "/");
                //                }
                //                
                //                inline size_t updateCache(size_t recordNumber) {
                //                    if (recordNumber >= m_cacheEnd || recordNumber < m_cacheStart) {
                //                        refreshCache(recordNumber);
                //                    }
                //                    return (recordNumber - m_cacheStart);
                //                }
                //
                //                void refreshCache(size_t recordNumber);
                //
                //                void initializeCache();
                //
                //

                // file where this table belongs to.
                hid_t m_h5file;

                // table name, i.e.: /Data/Bla
                boost::filesystem::path m_name;



                //boost::shared_ptr<H5::Group> 
                hid_t m_group; // hdf5 group to this table

                //std::map<std::string,boost::shared_ptr<H5::Group> > 
                H5GroupsMap m_h5Groups;


                boost::shared_ptr<Format> m_dataFormat;
                //
                //
                //                // selected record format in Hash representation
                //                karabo::util::Hash m_recordFormatHash;
                //
                //                karabo::util::Hash m_readData;
                //

                hsize_t m_chunkSize;
                hsize_t m_numberOfRecords;
                hid_t m_numberOfRecordsAttribute;
                #ifdef KARABO_USE_PROFILER_TABLE1
                karabo::util::Profiler table1;
                #endif

            private:
                static const char* NUMBER_OF_RECORDS;

            };

        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::Table, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_TABLE_HH */
