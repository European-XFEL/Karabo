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
            public:

                KARABO_CLASSINFO(File, "Hdf5", "1.0")
                KARABO_CONFIGURATION_BASE_CLASS

                File(const karabo::util::Hash& input);
                File(const boost::filesystem::path& filename);
                virtual ~File();


                static void expectedParameters(karabo::util::Schema& expected);
                void configure(const karabo::util::Hash& input);

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
                 * \li \c APPEND Appending records to file is possible, new table cannot be created.
                 * \li \c READONLY Readonly mode.
                 */
                void open(File::AccessMode mode);

                /**
                 * Create new table in the file.
                 * @param name Table name. It can be a path with "/" as separator.
                 * @param dataFormat Object describing data format.
                 * @param chunkSize Chunk size is the same for each dataset in the table.
                 * @return Pointer to Table. 
                 *
                 * @see DataFormat::expectedParameters.
                 */
                boost::shared_ptr<Table> createTable(const std::string& name, const Format::Pointer dataFormat, size_t chunkSize = 1);

                /**
                 * Open existing table in the file.
                 * @param name Table name. It can be a path with "/" as separator.       
                 *
                 * The following rules apply to data format:
                 * 
                 * \li If the description is stored in the file as a group attribute it is taken from it.
                 * \li If not (true for files written by different software) the best attempt to discover the data format is
                 *  made.
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
                boost::shared_ptr<Table> getTable(const std::string& name, karabo::io::h5::Format::Pointer dataFormat);



                /**
                 * Close the file. When the file was opened in any mode which allowed modification all data are flushed.
                 */
                void close();


                
            private:

                boost::filesystem::path m_filename;

                hid_t m_h5file;

                AccessMode m_accMode;

                boost::shared_ptr<Table> createReadOnlyTablePointer(const std::string& name);


            };
        }
    }
}

//KARABO_REGISTER_FACTORY_BASE_HH(karabo::io::hdf5::File, TEMPLATE_IO, DECLSPEC_IO)

#endif	
