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


            int File::m_init = File::initErrorHandling();


            int File::initErrorHandling() {
                H5Eset_auto(H5E_DEFAULT, NULL, NULL);
                return 1;
            }

            File::File(const hid_t & h5file) : m_h5file(h5file), m_managed(true){
                size_t nameSize = H5Fget_name(h5file, NULL, 0);
                char filename[nameSize+1];
                H5Fget_name(h5file, filename, nameSize+1);
                m_filename = std::string(filename);
                
                
            }

            File::File(const Hash& input) : m_h5file(-1), m_managed(false) {
                m_filename = boost::filesystem::path(input.get<std::string > ("filename"));
            }


            File::File(const boost::filesystem::path& filename)
            : m_filename(filename), m_h5file(-1), m_managed(false) {
            }


            File::File(const std::string& filename)
            : m_filename(filename), m_h5file(-1), m_managed(false) {
            }


            File::~File() {
                if (m_h5file > -1 ) {
                    close();
                }
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

                if (isOpen()) {
                    ostringstream os;
                    os << "File " << m_filename << " is already open";
                    throw KARABO_IO_EXCEPTION(os.str());
                }

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
                    KARABO_CHECK_HDF5_STATUS(m_h5file);
                } catch (...) {
                    ostringstream os;
                    os << "Could not open file " << m_filename;
                    throw KARABO_IO_EXCEPTION(os.str());
                }
                m_accMode = mode;
                KARABO_CHECK_HDF5_STATUS(H5Pclose(fapl));
            }


            bool File::isOpen() {
                return (m_h5file > 0 ? true : false);
            }


            bool File::hasTable(const std::string& path) const {
                for (size_t i = 0; i < m_existingTables.size(); ++i) {
                    if (m_existingTables[i] == path) return true;
                }
                return false;
            }


            Table::Pointer File::createTable(const string& name, const Format::Pointer dataFormat) {


                if( name[0] != '/'){
                    throw KARABO_IO_EXCEPTION("Table name must start with /");
                }
                
                if (m_accMode == READONLY) {
                    throw KARABO_IO_EXCEPTION("Cannot create table when file is open in READONLY mode");
                }

                if(hasTable(name)){
                    throw KARABO_IO_EXCEPTION("Cannot create table " + name +" - already exists");
                }

                Table::Pointer table = Table::Pointer(new Table(m_h5file, name));
                table->setUniqueId();
                table->openNew(dataFormat);
                updateTableIndex(name);
                KARABO_LOG_FRAMEWORK_TRACE_CF << "register table " << table->getUniqueId();
                m_openTables[table->getUniqueId()] = table;
                return table;

            }


            Table::Pointer File::getTable(const std::string & name) {
                string defaultId = Table::generateUniqueId(name);
                TableMap::iterator it = m_openTables.find(defaultId);
                if (it != m_openTables.end()) {
                    return it->second;
                }
                Table::Pointer table = createReadOnlyTablePointer(name);
                table->openReadOnly();
                KARABO_LOG_FRAMEWORK_TRACE_CF << "register table " << defaultId;
                m_openTables[defaultId] = table;
                return table;
            }


            Table::Pointer File::getTable(const std::string& name, const karabo::io::h5::Format::Pointer dataFormat, size_t numberOfRecords) {
                //table name + format + number of records is unique
                string uniqueId = Table::generateUniqueId(name, dataFormat, numberOfRecords);
                TableMap::iterator it = m_openTables.find(uniqueId);
                if (it != m_openTables.end()) {
                    return it->second;
                }
                Table::Pointer table = createReadOnlyTablePointer(name);
                table->openReadOnly(dataFormat, numberOfRecords);
                KARABO_LOG_FRAMEWORK_TRACE_CF << "register table " << uniqueId;
                m_openTables[uniqueId] = table;
                return table;
            }


            void File::close() {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "start closing file " << m_filename.string() << " Num. open tables: " << m_openTables.size();
                if (!isOpen()) return;
                if (m_accMode == TRUNCATE || m_accMode == EXCLUSIVE || m_accMode == APPEND) {
                    KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));
                }
                TableMap::iterator it = m_openTables.begin();
                while (it != m_openTables.end()) {
                    it->second->close();
                    m_openTables.erase(it++);

                }
                if(!m_managed) KARABO_CHECK_HDF5_STATUS(H5Fclose(m_h5file));
                m_h5file = -1;
                KARABO_LOG_FRAMEWORK_TRACE_CF << "file " << m_filename.string() << " closed";
            }


            void File::closeTable(boost::shared_ptr<Table> table) {
                closeTable(table->getUniqueId());
            }


            void File::closeTable(const std::string & name) {
                TableMap::iterator it = m_openTables.find(name);
                if (it != m_openTables.end()) {
                    it->second->close();
                    m_openTables.erase(it);
                }
            }


            Table::Pointer File::createReadOnlyTablePointer(const std::string & name) {
                return Table::Pointer(new Table(m_h5file, name));
            }
            
            
            void File::updateTableIndex(const std::string & path) {

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
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(stringType));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace));

                } else {
                    tables = H5Aopen(m_h5file, "tables", H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tables);
                    char* ptr[1];

                    hid_t tid = ScalarTypes::getHdf5NativeType<string>();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(tables, tid, &ptr));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    tablePaths = ptr[0];
                }

                tablePaths += "\n" + path;
                const char* ptr = tablePaths.c_str();
                hid_t tid = ScalarTypes::getHdf5NativeType<string>();
                KARABO_CHECK_HDF5_STATUS(H5Awrite(tables, tid, &ptr));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(tables))
                
                boost::split(m_existingTables, tablePaths, boost::is_any_of("\n"));

            }


            Hash & File::reportOpenObjects(karabo::util::Hash & hash) {
                hash.set("Number of open datasets", H5Fget_obj_count(m_h5file, H5F_OBJ_DATASET));
                hash.set("Number of open groups: ", H5Fget_obj_count(m_h5file, H5F_OBJ_GROUP));
                hash.set("Number of open datatypes", H5Fget_obj_count(m_h5file, H5F_OBJ_DATATYPE));
                hash.set("Number of open attributes", H5Fget_obj_count(m_h5file, H5F_OBJ_ATTR));
                return hash;
            }
        }
    }
}
