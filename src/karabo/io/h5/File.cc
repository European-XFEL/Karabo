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

//#include "../Writer.hh"

using namespace std;
using namespace karabo::util;


namespace karabo {
    namespace io {
        namespace h5 {

            KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::File)


            File::File(const Hash& input) : m_filename("") {
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

            void File::configure(const Hash& input) {
                m_filename = input.get<boost::filesystem::path > ("filename");
            }

            void File::open(File::AccessMode mode) {
                try {
                    if (mode == TRUNCATE) {
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
                    } else if (mode == EXCLUSIVE) {
                        m_h5file = H5Fcreate(m_filename.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
                    } else if (mode == READONLY) {
                        m_h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
                    } else if (mode == APPEND) {
                        m_h5file = H5Fopen(m_filename.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
                    }
                } catch (...) {
                    ostringstream os;
                    os << "Could not open file " << m_filename;
                    string msg = os.str();
                    throw KARABO_IO_EXCEPTION(msg);
                }
                m_accMode = mode;
            }

            Table::Pointer File::createTable(const string& name, const Format::Pointer dataFormat, size_t chunkSize) {

                if (m_accMode == READONLY || m_accMode == APPEND) {
                    throw KARABO_IO_EXCEPTION("Cannot create table when file is opened in READONLY or APPEND mode");
                }

                Table::Pointer table = Table::Pointer(new Table(m_h5file, name, chunkSize));
                table->openNew(dataFormat);
                return table;

            }

            Table::Pointer File::getTable(const std::string& name) {
                Table::Pointer table = createReadOnlyTablePointer(name);
                //          table->openReadOnly();
                return table;
            }

            Table::Pointer File::getTable(const std::string& name, const karabo::io::h5::Format::Pointer dataFormat) {
                Table::Pointer table = createReadOnlyTablePointer(name);
                table->openReadOnly(dataFormat);
                return table;
            }

            void File::close() {
                if (m_accMode == TRUNCATE || m_accMode == EXCLUSIVE || m_accMode == APPEND) {                    
                    H5Fflush(m_h5file,H5F_SCOPE_GLOBAL);
                }
                H5Fclose(m_h5file);
            }

            Table::Pointer File::createReadOnlyTablePointer(const std::string& name) {
                return Table::Pointer(new Table(m_h5file, name));
            }
        }
    }
}
