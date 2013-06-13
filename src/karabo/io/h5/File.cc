/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include <karabo/util/Factory.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/PathElement.hh>

#include "File.hh"
#include "Scalar.hh"


using namespace std;
using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::File)


            File::File(const Hash& input) {
                m_filename = boost::filesystem::path(input.get<std::string > ("filename"));
            }


            File::File(const boost::filesystem::path& filename)
            : m_filename(filename) {
            }


            File::~File() {
            }


            void File::expectedParameters(Schema& expected) {


                PATH_ELEMENT(expected).key("filename")
                        .description("Name of the file to be written")
                        .displayedName("Filename")
                        .assignmentMandatory()
                        .commit();
            }


            void File::open(File::AccessMode mode) {
                
                hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
                H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);
//                H5Pset_meta_block_size(fapl, 1024*1024);

                try {
                    if (mode == TRUNCATE) {
//                        hid_t fip = H5Pcreate(H5P_FILE_ACCESS );
//                        H5Pset_meta_block_size(fip, 8*8192);
//                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fip);
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, fapl);
                    } else if (mode == EXCLUSIVE) {
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, fapl);
                    } else if (mode == READONLY) {
                        m_h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, fapl);
                    } else if (mode == APPEND) {
                        m_h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDWR, fapl);
                    }
                } catch (...) {
                    ostringstream os;
                    os << "Could not open file " << m_filename;
                    string msg = os.str();
                    throw KARABO_IO_EXCEPTION(msg);
                }
                m_accMode = mode;
            }


            Table::Pointer File::createTable(const string& name, const Format::Pointer dataFormat) {

                if (m_accMode == READONLY || m_accMode == APPEND) {
                    throw KARABO_IO_EXCEPTION("Cannot create table when file is opened in READONLY or APPEND mode");
                }

                Table::Pointer table = Table::Pointer(new Table(m_h5file, name));
                table->openNew(dataFormat);
                updateTableIndex(name);
                return table;

            }


            Table::Pointer File::getTable(const std::string& name) {
                Table::Pointer table = createReadOnlyTablePointer(name);
                table->openReadOnly();
                return table;
            }


            Table::Pointer File::getTable(const std::string& name, const karabo::io::h5::Format::Pointer dataFormat) {
                Table::Pointer table = createReadOnlyTablePointer(name);
                table->openReadOnly(dataFormat);
                return table;
            }


            void File::close() {
                if (m_accMode == TRUNCATE || m_accMode == EXCLUSIVE || m_accMode == APPEND) {
                    KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));
                }
                KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
            }


            Table::Pointer File::createReadOnlyTablePointer(const std::string& name) {
                return Table::Pointer(new Table(m_h5file, name));
            }


            void File::updateTableIndex(const std::string& path) {

                hid_t tables;
                std::string tablePaths;

                if (!H5Aexists(m_h5file, "tables")) {
                    hid_t stringType = H5Tcopy(H5T_C_S1);
                    herr_t status = H5Tset_size(stringType, H5T_VARIABLE);
                    KARABO_CHECK_HDF5_STATUS(status)

                    hsize_t dims[1] = {1};
                    hid_t dataSpace = H5Screate_simple(1, dims, NULL);
                    KARABO_CHECK_HDF5_STATUS(dataSpace);

                    tables = H5Acreate(m_h5file, "tables", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tables);
                } else {
                    tables = H5Aopen(m_h5file, "tables", H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tables);
                    char* ptr[1];

                    KARABO_CHECK_HDF5_STATUS(H5Aread(tables, ScalarTypes::getHdf5NativeType<string>(), &ptr));
                    tablePaths = ptr[0];
                }

                tablePaths += "\n" + path;
                const char* ptr = tablePaths.c_str();
                KARABO_CHECK_HDF5_STATUS(H5Awrite(tables, ScalarTypes::getHdf5NativeType<string>(), &ptr));
            }
        }
    }
}
