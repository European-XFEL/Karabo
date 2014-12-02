/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_FILE_HH
#define	KARABO_IO_H5_FILE_HH

#include "Table.hh"
#include "Format.hh"
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <hdf5/hdf5.h>


namespace karabo {

    namespace io {

        namespace h5 {


            class Format;

            /**
             * Class representing physical Hdf5 file.
             * Each File may contain any number of Tables.
             */
            class File {

                static int m_init;
                static int initErrorHandling();

            public:

                KARABO_CLASSINFO(File, "Hdf5", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

            private:
                typedef std::map<std::string, boost::shared_ptr<karabo::io::h5::Table> > TableMap;

            public:

                File(const hid_t & h5file);
                
                File(const karabo::util::Hash& input);

                File(const boost::filesystem::path& filename);

                File(const std::string& filename);

                virtual ~File();


                static void expectedParameters(karabo::util::Schema& expected);

                enum AccessMode {

                    TRUNCATE,
                    EXCLUSIVE,
                    APPEND,
                    READONLY
                };

                /**
                 * Open a file. The following modes are supported.
                 *
                 * \li \c TRUNCATE Truncate file if exists.
                 * \li \c EXCLUSIVE Fails to open a file if already exists.
                 * \li \c APPEND Appending records to existing tables and creating new tables within the file is possible
                 * \li \c READONLY Readonly mode.
                 */
                void open(File::AccessMode mode);
                
                
                

                /**
                 * Check if file is open
                 * @return true if file is open, otherwise false
                 */
                bool isOpen();

                /**
                 * Check if the table exists in the file
                 * @return True if the table exists, otherwise false
                 */
                bool hasTable(const std::string& name) const;
                
                /**
                 * Create new table in the file.
                 * @param name Table name. It can be a path with "/" as separator.
                 * @param dataFormat Object describing data format.
                 * @return Pointer to Table. 
                 *
                 * @see DataFormat::expectedParameters.
                 */
                boost::shared_ptr<Table> createTable(const std::string& name, const Format::Pointer dataFormat);

                /**
                 * Open existing table in the file.
                 * @param name Table name. It can be a path with "/" as separator.       
                 *
                 * The following rules apply to data format:
                 * 
                 * \li If the description is stored in the file as a group attribute it is taken from it.                 
                 */
                boost::shared_ptr<Table> getTable(const std::string& name);

                /**
                 * Open existing table in the file.
                 * @param name Table name. It can be a path with "/" as separator.       
                 * @param dataFormat Object describing data format.
                 * 
                 * 
                 * This function passes the data format and forces to overwrite any existing definition in the file. 
                 * No attempt is made to discover format from the file content.
                 * This can be useful if one wants to read only certain datasets.
                 * Client is fully responsible to make sure that the supplied format is compatible with stored data.
                 */
                boost::shared_ptr<Table> getTable(const std::string& name, karabo::io::h5::Format::Pointer dataFormat, size_t numberOfRecords = 0);

                void closeTable(boost::shared_ptr<Table> table);

                /**
                 * Close the file. When the file was opened in any mode which allowed modification all data are flushed.
                 */
                void close();

                /*
                 * Return file name
                 */
                std::string getName() const {
                    return m_filename.string();
                }

                karabo::util::Hash& reportOpenObjects(karabo::util::Hash& hash);
                
              
            private:

                boost::filesystem::path m_filename;

                hid_t m_h5file;

                AccessMode m_accMode;

                TableMap m_openTables;
                
                std::vector<std::string> m_existingTables;

                bool m_isOpen;

                boost::shared_ptr<Table> createReadOnlyTablePointer(const std::string& name);

                void updateTableIndex(const std::string& tablePath);
                
                bool m_managed;

                void closeTable(const std::string& uniqueId);

            };
        }
    }
}


#endif	
