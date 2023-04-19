/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "Table.hh"

#include <boost/smart_ptr/shared_array.hpp>
#include <karabo/io/HashXmlSerializer.hh>
#include <karabo/log/Logger.hh>
#include <karabo/util/HashFilter.hh>
#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/TimeProfiler.hh>

#include "Scalar.hh"
#include "ioProfiler.hh"


using namespace std;
using namespace karabo::util;
using namespace boost;

namespace karabo {
    namespace io {
        namespace h5 {


            const char* Table::TABLE_SIZE = "tableSize";


            Table::~Table() {}


            void Table::openNew(const Format::Pointer dataFormat) {
                //                                TimeProfiler p("hdf5");
                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open new Table: " << m_name;
                //                                p.start("createEmptyTable");
                createEmptyTable(m_h5file, m_name);
                //                                p.stop("createEmptyTable");
                //                                p.start("createSchemaVersionAttribute");
                createSchemaVersionAttribute();
                //                                p.stop("createSchemaVersionAttribute");
                //                                p.start("createNumberOfRecordsAttribute");
                createInitialNumberOfRecordsAttribute();
                //                                p.stop("createNumberOfRecordsAttribute");
                //                                p.start("saveTableFormat");
                saveTableFormatAsAttribute(dataFormat);
                m_dataFormat = dataFormat;
                //                                p.stop("saveTableFormat");
                //                                p.start("create");
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->create(m_group);
                }
                //                                p.stop("create");
                //
                //                                clog << "createEmptyTable: " <<
                //                                HighResolutionTimer::time2double(p.getTime("createEmptyTable")) <<
                //                                endl; clog << "createSchemaVersionAttribute: " <<
                //                                HighResolutionTimer::time2double(p.getTime("createSchemaVersionAttribute"))
                //                                << endl; clog << "createNumberOfRecordsAttribute " <<
                //                                HighResolutionTimer::time2double(p.getTime("createNumberOfRecordsAttribute"))
                //                                << endl; clog << "saveTableFormat " <<
                //                                HighResolutionTimer::time2double(p.getTime("saveTableFormat")) <<
                //                                endl; clog << "create:   " <<
                //                                HighResolutionTimer::time2double(p.getTime("create")) << endl;
            }


            void Table::openReadOnly(const karabo::io::h5::Format::Pointer dataFormat, hsize_t numberOfRecords) {
                setUniqueId(dataFormat, numberOfRecords);

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open file for reading with specific user defined format: " << m_name;
                m_dataFormat = dataFormat;
                try {
                    m_group = H5Gopen(m_h5file, m_name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);

                    const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                    for (size_t i = 0; i < elements.size(); ++i) {
                        elements[i]->open(m_group);
                    }
                    if (numberOfRecords <= 0) {
                        retrieveNumberOfRecordsFromFile();
                    } else {
                        m_tableSize = numberOfRecords;
                    }
                } catch (...) {
                    KARABO_RETHROW
                }
            }


            void Table::openReadOnly() {
                // there are 3 ways of opening file for reading
                // 1 - table structure is read from the file. Hdf5 group contains the attribute table
                // 2 - table structure is discovered from the file/group structure
                // 3 - user defines the table structure - this is implemented by openReadOnly(DataFormat::Pointer)
                // function
                //
                // 1) standard way, only files written by this software can use it as it requires the attribute
                // ("table") is set 2) this is more general, but still requires that within the table (h5 group)
                // datasets have the same number of entries (implemented as NDim dataset) and there is one-to-one
                // relation. 3) this should allow essentially any file to be read. Here the table could be even just one
                // dataset
                //

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open file for reading using stored table definition: " << m_name;

                setUniqueId();

                try {
                    m_group = H5Gopen(m_h5file, m_name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);

                    if (hasAttribute(m_group, "table")) {
                        Hash readDataFormatConfig;
                        // clog << m_name << " before read   " << HighResolutionTimer::now().sec << endl;
                        readTableFormatFromAttribute(readDataFormatConfig);
                        // clog << m_name << " before format " << HighResolutionTimer::now().sec << endl;
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "read format: \n" << readDataFormatConfig;
                        m_dataFormat = Format::createFormat(readDataFormatConfig);
                        // clog << m_name << " after format  " << HighResolutionTimer::now().sec << endl;
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("auto discovery not enabled yet");
                        // if format not defined as attribute discover it from data structure
                        // discover(m_dataFormat, m_name.c_str());
                        // KARABO_LOG_FRAMEWORK_TRACE_CF << m_dataFormat->getConfig() << endl;
                    }
                } catch (...) {
                    KARABO_RETHROW
                }

                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                KARABO_LOG_FRAMEWORK_TRACE_CF << "elements.size() : " << elements.size();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->open(m_group);
                }
                // clog << m_name << " after open    " << HighResolutionTimer::now().sec << endl;
                retrieveNumberOfRecordsFromFile();
            }


            void Table::append(const karabo::util::Hash& data) {
                write(data, m_tableSize);
            }


            void Table::write(const karabo::util::Hash& data, size_t recordId) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    try {
                        elements[i]->write(data, recordId);
                    } catch (Exception& ex) {
                        // TODO think what to do here
                        ex.clearTrace();
                        // clog << "element " << i << " could not be written" << endl;
                    }
                }
                if (m_tableSize <= recordId) {
                    m_tableSize = recordId + 1;
                    updateTableSizeAttribute();
                }
                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));
            }


            void Table::write(const karabo::util::Hash& data, size_t recordId, size_t len) {
                // Here we collect all write errors, but only re-throw it once we have written what we could.
                bool h5_err = false;
                std::string err_log("Problems writing the following elements:\n");

                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    try {
                        elements[i]->write(data, recordId, len);
                    } catch (const std::exception& e) {
                        h5_err = true;
                        std::ostringstream oss;
                        oss << "Exception when writing element: " << elements[i]->getKey()
                            << "\n\tFull path: " << elements[i]->getFullName()
                            << "\n\tElement type: " << elements[i]->getElementType()
                            << "\n\tInto H5 path: " << elements[i]->getH5path() << "\n\tDetailed error log:\n";
                        err_log += oss.str();
                        err_log += e.what();
                    }
                }

                hsize_t possibleNewSize = recordId + len;
                if (m_tableSize < possibleNewSize) {
                    m_tableSize = possibleNewSize;
                    updateTableSizeAttribute();
                }

                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));
                if (h5_err) {
                    // We always log, even if it gets caught and discarded later
                    KARABO_LOG_FRAMEWORK_ERROR << err_log;
                    throw KARABO_HDF_IO_EXCEPTION(err_log);
                }
            }


            void Table::writeAttributes(const karabo::util::Hash& data) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->saveAttributes(m_group, data);
                }
            }


            void Table::bind(karabo::util::Hash& data) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data);
                }
                m_bindWasExecuted = true;
            }


            void Table::bind(karabo::util::Hash& data, size_t bufferLen) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data, bufferLen);
                }
                m_bindLenWasExecuted = true;
            }


            size_t Table::read(size_t recordNumber) {
                if (!m_bindWasExecuted) {
                    throw KARABO_LOGIC_EXCEPTION(
                          "You need to bind a data structure to read to with Table::bind(Hash&) before reading");
                }
                if (recordNumber >= m_tableSize) return 0;
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read element " << elements[i]->getFullName();
                    elements[i]->read(recordNumber);
                }
                return 1ul;
            }


            size_t Table::read(size_t recordNumber, size_t len) {
                if (!m_bindLenWasExecuted) {
                    throw KARABO_LOGIC_EXCEPTION(
                          "You need to bind a data structure to read to with Table::bind(Hash&, size_t) before "
                          "reading");
                }
                size_t numberReadRecords = (recordNumber + len) < m_tableSize ? len : (m_tableSize - recordNumber);
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read  element " << i;
                    elements[i]->read(recordNumber, numberReadRecords);
                }
                return numberReadRecords;
            }


            void Table::readAttributes(karabo::util::Hash& data) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->openAttributes();
                    elements[i]->bind(data);
                    elements[i]->readAttributes(data);
                    elements[i]->closeAttributes();
                }
            }


            size_t Table::size() {
                return m_tableSize;
            }


            void Table::close() {
                KARABO_LOG_FRAMEWORK_TRACE_CF << " closing " << m_id;
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->close();
                }
                if (m_group > -1) {
                    KARABO_CHECK_HDF5_STATUS(H5Gclose(m_group));
                    m_group = -1;
                }

                if (m_numberOfRecordsAttribute > -1) {
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(m_numberOfRecordsAttribute));
                    m_numberOfRecordsAttribute = -1;
                }
            }


            // end of public functions


            void Table::createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath) {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "Table fullpath: " << fullPath.string();
                try {
                    string path = boost::replace_all_copy(fullPath.string(), "//", "/");
                    // open  root group

                    //                    hid_t group = H5Gopen(h5file, "/", H5P_DEFAULT);
                    //                    KARABO_CHECK_HDF5_STATUS(group);

                    hid_t lcpl = H5Pcreate(H5P_LINK_CREATE);
                    KARABO_CHECK_HDF5_STATUS(lcpl);
                    KARABO_CHECK_HDF5_STATUS(H5Pset_create_intermediate_group(lcpl, 1));
                    hid_t gcpl = H5Pcreate(H5P_GROUP_CREATE);
                    // The order tracking influences the performance - do not use it unless really needed
                    // KARABO_CHECK_HDF5_STATUS(H5Pset_link_creation_order(gcpl, H5P_CRT_ORDER_TRACKED));

                    // create table group
                    m_group = H5Gcreate(h5file, path.c_str(), lcpl, gcpl, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table: " << fullPath.string()
                                                  << " created. group id = " << m_group;
                    // KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(lcpl))
                    KARABO_CHECK_HDF5_STATUS(H5Pclose(gcpl))
                } catch (...) {
                    KARABO_RETHROW
                }
            }


            void Table::createSchemaVersionAttribute() {
                hid_t stringType = H5Tcopy(H5T_C_S1);
                herr_t status = H5Tset_size(stringType, H5T_VARIABLE);
                KARABO_CHECK_HDF5_STATUS(status)

                hsize_t dims[1] = {1};
                hid_t dataSpace = H5Screate_simple(1, dims, NULL);
                KARABO_CHECK_HDF5_STATUS(dataSpace);

                hid_t schemaVersion =
                      H5Acreate(m_group, "schemaVersion", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(schemaVersion)
                string version = Format::classInfo().getVersion();
                const char* versionPtr = version.c_str();
                status = H5Awrite(schemaVersion, stringType, &versionPtr);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Aclose(schemaVersion));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stringType))
                KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace))
            }


            void Table::createInitialNumberOfRecordsAttribute() {
                hid_t dataSpace = H5Screate(H5S_SCALAR);
                m_numberOfRecordsAttribute =
                      H5Acreate(m_group, TABLE_SIZE, H5T_STD_U64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_numberOfRecordsAttribute);
                KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace))
                updateTableSizeAttribute();
            }


            void Table::updateTableSizeAttribute() {
                hid_t type = H5Tcopy(H5T_NATIVE_HSIZE);
                herr_t status = H5Awrite(m_numberOfRecordsAttribute, type, &m_tableSize);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not write numberOfRecords attribute");
                }
                KARABO_CHECK_HDF5_STATUS(H5Tclose(type))
            }


            void Table::retrieveNumberOfRecordsFromFile() {
                if (hasAttribute(m_group, TABLE_SIZE)) {
                    m_numberOfRecordsAttribute = H5Aopen(m_group, TABLE_SIZE, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_numberOfRecordsAttribute);

                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_numberOfRecordsAttribute, H5T_NATIVE_HSIZE, &m_tableSize));
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute for " << m_name.c_str() << " is "
                                                  << m_tableSize;
                } else {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute not defined for " << m_name;
                    // calculateNumberOfRecords();
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Calculated number of records: " << m_tableSize;
                }
            }


            void Table::saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat) {
                try {
                    Hash persistentDataFormatConfig;
                    dataFormat->getPersistentConfig(persistentDataFormatConfig);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << persistentDataFormatConfig;
                    Hash c("Xml.indentation", 1, "Xml.writeDataTypes", false);
                    TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create(c);

                    string dataFormatConfigXml;

                    serializer->save(persistentDataFormatConfig, dataFormatConfigXml);

                    KARABO_LOG_FRAMEWORK_TRACE_CF
                          << "Description of format to be written to hdf5 file as group attribute:\n "
                          << dataFormatConfigXml;
                    hid_t dataSpace = H5Screate(H5S_SCALAR);
                    hid_t type = ScalarTypes::getHdf5StandardType<string>();
                    hid_t tableAttribute = H5Acreate(m_group, "table", type, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(type))
                    KARABO_CHECK_HDF5_STATUS(tableAttribute);
                    const char* ptr = dataFormatConfigXml.c_str();
                    type = ScalarTypes::getHdf5NativeType<string>();
                    KARABO_CHECK_HDF5_STATUS(H5Awrite(tableAttribute, type, &ptr));
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(tableAttribute));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace))
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(type))
                } catch (...) {
                    KARABO_RETHROW
                }
            }


            void Table::readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig) {
                try {
                    // read the format from group attribute

                    hid_t tableAttribute = H5Aopen(m_group, "table", H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tableAttribute);

                    hid_t space = H5Aget_space(tableAttribute);
                    KARABO_CHECK_HDF5_STATUS(space);
                    char* rdata[1];
                    hid_t memtype = H5Tcopy(H5T_C_S1);
                    KARABO_CHECK_HDF5_STATUS(memtype)
                    KARABO_CHECK_HDF5_STATUS(H5Tset_size(memtype, H5T_VARIABLE));

                    KARABO_CHECK_HDF5_STATUS(H5Aread(tableAttribute, memtype, &rdata));

                    string dataFormatConfigXml = rdata[0];
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Read format:\n " << dataFormatConfigXml;

                    TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
                    serializer->load(dataFormatConfig, dataFormatConfigXml);

                    KARABO_CHECK_HDF5_STATUS(H5Dvlen_reclaim(memtype, space, H5P_DEFAULT, &rdata));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(memtype));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(space));

                    // KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(tableAttribute));


                } catch (...) {
                    KARABO_RETHROW
                }
            }


            bool Table::hasAttribute(hid_t group, const string& name) const {
                return (H5Aexists(group, name.c_str()) > 0 ? true : false);
            }


            void Table::setUniqueId(const Format::Pointer dataFormat, hsize_t numberOfRecords) {
                std::ostringstream oss;
                oss << m_name.string() << "|" << dataFormat.get() << "|" << numberOfRecords;
                m_id = oss.str();
            }


            void Table::setUniqueId() {
                m_id = m_name.string() + "|0";
            }


            const std::string& Table::getUniqueId() const {
                return m_id;
            }


            std::string Table::generateUniqueId(const std::string& name) {
                return name + "|0";
            }


            std::string Table::generateUniqueId(const std::string& name, const Format::ConstPointer dataFormat,
                                                hsize_t numberOfRecords) {
                std::ostringstream oss;
                oss << name << "|" << dataFormat.get() << "|" << numberOfRecords;
                return oss.str();
            }


        } // namespace h5
    }     // namespace io
} // namespace karabo
