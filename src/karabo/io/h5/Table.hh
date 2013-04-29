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

                typedef std::map<std::string, hid_t > H5GroupsMap;

                Table(hid_t h5file, boost::filesystem::path name, hsize_t chunkSize = 1)
                : m_h5file(h5file), m_name(name), m_chunkSize(chunkSize), m_tableSize(0)
                #ifdef KARABO_USE_PROFILER_TABLE1
                , table1("table1")
                #endif
                {
                }

                virtual ~Table();




                /*
                 * Write a single record to the table at position recordNumber
                 * @param data Hash object representing data record structure in file
                 * @param recordNumber record numbers in table start from zero
                 */
                void write(const karabo::util::Hash& data, size_t recordNumber);

                void write(const karabo::util::Hash& data, size_t recordNumber, size_t len);


                /**
                 * Append data record to the end of the table.
                 * @param data Hash object representing data record structure in file
                 */
                void append(const karabo::util::Hash& data);


                void bind(karabo::util::Hash& data);
                
                void bind(karabo::util::Hash& data, size_t bufferLen);


                /**
                 * Read data record from the table.
                 * @param data Hash object filled with values read from the table.
                 * @param recordNumber Number identifying record to be read. Record numbers start from 0.
                 */
                size_t read(size_t recordNumber);

                /**
                 * Buffered reading
                 * @param data
                 * @param recordNumber
                 * @param 
                 */
                size_t read(size_t recordNumber, size_t len);                
                //                void readAttributes(karabo::util::Hash & attr);

                /*
                 * Get table size.                 
                 * This function returns the index to the first record greater than the last written to the table.
                 * i.e.
                 * 
                 *  table->write(data, 0, 10);
                 *  table->write(data, 12, 5);
                 *  table.size() ==> returns 17
                 */
                size_t size();

                void close();
                //
                //            private:
                //
                void openNew(const karabo::io::h5::Format::Pointer dataFormat);
                //                //void openReadOnly(const karabo::util::Hash& dataFormatConfig = karabo::util::Hash());
                void openReadOnly(const karabo::io::h5::Format::Pointer dataFormat);
                void openReadOnly();
                //
                void defineStructure();
                //
                //                void r_copyFromCache(karabo::util::Hash& data, size_t recordNumber, const karabo::util::Hash& recordFormat);
                //                // calback method for used by discover function
                //                static herr_t fileInfo(hid_t loc_id, const char *name, const H5O_info_t *info, void *opdata);
                //                void discover(karabo::io::hdf5::DataFormat::Pointer& discovered, std::string groupName);
                //
                //
                void updateTableSizeAttribute();
                void retrieveNumberOfRecordsFromFile();
                //                void calculateNumberOfRecords();
                //                void retrieveChunkSizeFromFile();
                void createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath);
                void createInitialNumberOfRecordsAttribute();
                void createSchemaVersionAttribute();

                void saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat);
                void readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig);

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



                hid_t m_group; // hdf5 group to this table

                //std::map<std::string,boost::shared_ptr<H5::Group> > 
                H5GroupsMap m_h5Groups;


                boost::shared_ptr<Format> m_dataFormat;

                hsize_t m_chunkSize;
                hsize_t m_tableSize;
                hid_t m_numberOfRecordsAttribute;
                #ifdef KARABO_USE_PROFILER_TABLE1
                karabo::util::Profiler table1;
                #endif

            private:
                static const char* TABLE_SIZE;

            };

        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::Table, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_TABLE_HH */
