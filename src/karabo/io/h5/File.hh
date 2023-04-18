/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_FILE_HH
#define KARABO_IO_H5_FILE_HH

#include <hdf5/hdf5.h>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>

#include "Format.hh"
#include "Table.hh"


namespace karabo {

    namespace io {

        namespace h5 {


            class Format;

            /**
             * @class File
             * @brief A class representing a physical HDF5 file, containing any number of tables.
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
                /**
                 * Initialize a File wrapped from a HDF5 file identified by an h5 file handle
                 * @param h5file
                 */
                File(const hid_t& h5file);

                /**
                 * Initialize a File wrapped as identified by an input configuration. Input should contain
                 * a key filename, pointing to the file path and name
                 *
                 * @param input
                 */
                File(const karabo::util::Hash& input);

                /**
                 * Initialize a File wrapped as identified by a path and name
                 * @param filename
                 */
                File(const boost::filesystem::path& filename);

                /**
                 * Initialize a File wrapped as identified by a path and name
                 * @param filename
                 */
                File(const std::string& filename);

                virtual ~File();

                /**
                 * The expected parameters of this factorizable class.
                 *
                 * - filename: refers to path and name of the wrapped file
                 *
                 * @param expected
                 */
                static void expectedParameters(karabo::util::Schema& expected);

                /**
                 * An enum identifying the access mode the file
                 */
                enum AccessMode {

                    /**
                     * Truncate file upon opening
                     */
                    TRUNCATE,
                    /**
                     *  Fails to open a file if already exists.
                     */
                    EXCLUSIVE,
                    /**
                     * Append to an existing file
                     */
                    APPEND,
                    /**
                     * Open read-only
                     */
                    READONLY
                };

                /**
                 * Open a file. The following modes are supported.
                 *
                 * \li \c TRUNCATE Truncate file if exists.
                 * \li \c EXCLUSIVE Fails to open a file if already exists.
                 * \li \c APPEND Appending records to existing tables and creating new tables within the file is
                 * possible \li \c READONLY Readonly mode.
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
                boost::shared_ptr<Table> getTable(const std::string& name, karabo::io::h5::Format::Pointer dataFormat,
                                                  size_t numberOfRecords = 0);

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

                /**
                 * Fills the passed hash with a list of open objects
                 * @param hash
                 * @return
                 */
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
        } // namespace h5
    }     // namespace io
} // namespace karabo


#endif
