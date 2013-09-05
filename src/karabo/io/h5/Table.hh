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
#include "Format.hh"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <hdf5/hdf5.h>
#include "ioProfiler.hh"




namespace karabo {

    namespace io {

        namespace h5 {

            /**
             Table is an entity defined to store set of records...
             */
            class Table {

                friend class File;

            public:

                KARABO_CLASSINFO(Table, "Table", "1.0")
               
                Table(hid_t h5file, boost::filesystem::path name)
                : m_h5file(h5file), m_name(name), m_group(-1), m_tableSize(0),  m_numberOfRecordsAttribute(-1)                
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
                 * Read data record from the table. Before using read one need to bind the table to the Hash object.
                 * This Hash will be filled with corresponding data.
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

                /*
                 * Get table size.                 
                 * This function returns the index to the first record greater than the last written to the table.
                 * i.e.                  
                 *  table->write(data, 0, 10);
                 *  table->write(data, 12, 5);
                 *  table.size() ==> returns 17                 * 
                 */
                size_t size();

                
                void close();

                Format::ConstPointer getFormat() const {
                    return m_dataFormat;
                }
                
                Format::Pointer getFormat() {
                    return m_dataFormat;
                }
                
                void writeAttributes(const karabo::util::Hash& data);

                void readAttributes(karabo::util::Hash& data);

            private:

                void openNew(const karabo::io::h5::Format::Pointer dataFormat);
                
                //                //void openReadOnly(const karabo::util::Hash& dataFormatConfig = karabo::util::Hash());
                
                void openReadOnly(const karabo::io::h5::Format::Pointer dataFormat);
                
                void openReadOnly();

                void updateTableSizeAttribute();
                
                void retrieveNumberOfRecordsFromFile();

                void createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath);
                
                void createInitialNumberOfRecordsAttribute();
                
                void createSchemaVersionAttribute();

                void saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat);
                
                void readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig);

                bool hasAttribute(hid_t group, const std::string& name) const;

                
                // file where this table belongs to.
                hid_t m_h5file;

                // table name, i.e.: /Data/Bla
                boost::filesystem::path m_name;

                hid_t m_group; // hdf5 group to this table

                Format::Pointer m_dataFormat;
                
                hsize_t m_tableSize;
                hid_t m_numberOfRecordsAttribute;

            private:
                static const char* TABLE_SIZE;

            };

        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::Table, TEMPLATE_IO, DECLSPEC_IO)

#endif	/* KARABO_IO_TABLE_HH */
