/* Copyright (C) European XFEL GmbH Schenefeld. All rights reserved. */
#ifndef KARABO_IO_H5_TABLE_HH
#define KARABO_IO_H5_TABLE_HH

#include <hdf5/hdf5.h>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

#include "File.hh"
#include "Format.hh"
#include "ioProfiler.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class Table
             * @brief Table is an entity defined to store set of records
             */
            class Table {
                friend class File;

               public:
                KARABO_CLASSINFO(Table, "Table", "1.0");

               private:
                Table(hid_t h5file, boost::filesystem::path name)
                    : m_h5file(h5file),
                      m_name(name),
                      m_group(-1),
                      m_tableSize(0),
                      m_numberOfRecordsAttribute(-1),
                      m_bindWasExecuted(false),
                      m_bindLenWasExecuted(false) {}

               public:
                virtual ~Table();


                /**
                 * Write a single record to the table at position recordNumber
                 * @param data Hash object representing data record structure in file
                 * @param recordNumber record numbers in table start from zero
                 */
                void write(const karabo::util::Hash& data, size_t recordNumber);


                /**
                 * Write a multiple records to the table at position recordNumber
                 * @param data Hash object representing data record structure in file
                 * @param recordNumber record numbers in table start from zero
                 * @param len number of records to be written
                 */
                void write(const karabo::util::Hash& data, size_t recordNumber, size_t len);

                /**
                 * Append data record to the end of the table.
                 * @param data Hash object representing data record structure in file
                 */
                void append(const karabo::util::Hash& data);


                /**
                 * Bind hash object to the record in file
                 * @param data Hash object representing data record
                 */
                void bind(karabo::util::Hash& data);

                /**
                 * Bind hash object to the multiple records in file
                 * @param data Hash object representing multiple data records
                 */
                void bind(karabo::util::Hash& data, size_t bufferLen);

                /**
                 * Read data record from the table. Before using read one need to bind the table to the Hash object.
                 * This Hash object will be filled with corresponding data.
                 * @param recordNumber Number identifying record to be read. Record numbers start from 0.
                 */
                size_t read(size_t recordNumber);

                /**
                 * Buffered reading
                 * @param recordNumber Number identifying the first record to be read. Record numbers start from 0.
                 * @param len number of records to be read
                 */
                size_t read(size_t recordNumber, size_t len);

                /**
                 * Get table size.
                 * This function returns the index to the first record greater than the last written to the table.
                 * i.e.
                 *  table->write(data, 0, 10);
                 *  table->write(data, 12, 5);
                 *  table.size() ==> returns 17                 *
                 */
                size_t size();

                /**
                 * Write attributes to file
                 * @param data Hash object representing data record. Only attributes are written.
                 */
                void writeAttributes(const karabo::util::Hash& data);


                /**
                 * Read attributes from file
                 * @param data Hash object representing attributes. Only attributes are filled.
                 */
                void readAttributes(karabo::util::Hash& data);

                /**
                 * Get data format describing this table
                 */
                Format::ConstPointer getFormat() const {
                    return m_dataFormat;
                }

                /**
                 * Get data format describing this table
                 */
                Format::Pointer getFormat() {
                    return m_dataFormat;
                }

                /**
                 * Get table name
                 */
                std::string getName() const {
                    return m_name.string();
                }

               private:
                void close();

                void openNew(const karabo::io::h5::Format::Pointer dataFormat);

                void openReadOnly(const karabo::io::h5::Format::Pointer dataFormat, hsize_t numberOfRecords);

                void openReadOnly();

                void updateTableSizeAttribute();

                void retrieveNumberOfRecordsFromFile();

                void createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath);

                void createInitialNumberOfRecordsAttribute();

                void createSchemaVersionAttribute();

                void saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat);

                void readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig);

                bool hasAttribute(hid_t group, const std::string& name) const;

                void setUniqueId(const Format::Pointer dataFormat, hsize_t numberOfRecords);

                void setUniqueId();

                const std::string& getUniqueId() const;

                static std::string generateUniqueId(const std::string& name);

                static std::string generateUniqueId(const std::string& name, const Format::ConstPointer dataFormat,
                                                    hsize_t numberOfRecords);


               private:         // data members
                hid_t m_h5file; // file where this table belongs to.

                boost::filesystem::path m_name; // table name, i.e.: /Data/Bla

                hid_t m_group; // hdf5 group to this table

                Format::Pointer m_dataFormat;

                hsize_t m_tableSize;

                hid_t m_numberOfRecordsAttribute;

                std::string m_id; // table unique id

                static const char* TABLE_SIZE;

                bool m_bindWasExecuted;
                bool m_bindLenWasExecuted;
            };

        } // namespace h5
    }     // namespace io
} // namespace karabo

// KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::Table, TEMPLATE_IO, DECLSPEC_IO)

#endif /* KARABO_IO_TABLE_HH */
