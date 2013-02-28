/*
 * $Id: File.cc 5260 2012-02-26 22:13:16Z wrona $
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
using namespace H5;

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
                        m_h5file = boost::shared_ptr<H5File > (new H5File(m_filename.c_str(), H5F_ACC_TRUNC));
                    } else if (mode == EXCLUSIVE) {
                        m_h5file = boost::shared_ptr<H5File > (new H5File(m_filename.c_str(), H5F_ACC_EXCL));
                    } else if (mode == READONLY) {
                        m_h5file = boost::shared_ptr<H5File > (new H5File(m_filename.c_str(), H5F_ACC_RDONLY));
                    } else if (mode == APPEND) {
                        m_h5file = boost::shared_ptr<H5File > (new H5File(m_filename.c_str(), H5F_ACC_RDWR));
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

                Table::Pointer table = Table::Pointer( new Table(m_h5file, name, chunkSize));
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
                //         table->openReadOnly(dataFormat);
                return table;
            }

            void File::close() {
                if (m_accMode == TRUNCATE || m_accMode == EXCLUSIVE || m_accMode == APPEND) {
                    m_h5file->flush(H5F_SCOPE_GLOBAL);
                }
                m_h5file->close();
            }

            Table::Pointer File::createReadOnlyTablePointer(const std::string& name) {
//                Hash conf;
//                conf.set("t.table", name);
//                conf.set("t.h5file", m_h5file);
                return Table::Pointer( new Table(m_h5file, name));
                //return Configurator<Table>::createNode("t", "Hdf5", conf);
            }
        }
    }
}
