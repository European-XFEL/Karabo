/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */

#include <boost/smart_ptr/shared_array.hpp>

#include <karabo/log/Logger.hh>

#include "Table.hh"
#include "Scalar.hh"

#include <karabo/util/PathElement.hh>
#include <karabo/util/SimpleElement.hh>

#include <karabo/io/HashXmlSerializer.hh>
#include <karabo/util/HashFilter.hh>


#include "ioProfiler.hh"


using namespace std;
using namespace karabo::util;
using namespace boost;

namespace karabo {
    namespace io {
        namespace h5 {


            const char* Table::TABLE_SIZE = "tableSize";


            Table::~Table() {             
            }


            void Table::openNew(const Format::Pointer dataFormat) {

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open new file: " << m_name;
                createEmptyTable(m_h5file, m_name);
                createSchemaVersionAttribute();
                createInitialNumberOfRecordsAttribute();
                saveTableFormatAsAttribute(dataFormat);
                //////////////////////////////////////
                // to be removed: test only
                //                Hash config;
                //                readTableFormatFromAttribute(config);
                //////////////////////////////////////
                m_dataFormat = dataFormat;
                defineStructure();
                //refreshRecordFormatVector();

            }


            void Table::openReadOnly(const karabo::io::h5::Format::Pointer dataFormat) {

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open file for reading with specific user defined format: " << m_name;
                m_dataFormat = dataFormat;
                try {

                    m_group = H5Gopen(m_h5file, m_name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);

                    vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                    for (size_t i = 0; i < elements.size(); ++i) {
                        elements[i]->open(m_group);
                    }
                    retrieveNumberOfRecordsFromFile();
                } catch (...) {
                    KARABO_RETHROW
                }
            }


            void Table::openReadOnly() {
                // there are 3 ways of opening file for reading
                // 1 - table structure is read from the file. Hdf5 group contains the attribute table
                // 2 - table structure is discovered from the file/group structure
                // 3 - user defines the table structure - this is implemented by openReadOnly(DataFormat::Pointer) function
                //
                // 1) standard way, only files written by this software can use it as it requires the attribute ("table") is set
                // 2) this is more general, but still requires that within the table (h5 group) datasets have the same number of
                // entries (implemented as NDim dataset) and there is one-to-one relation. 
                // 3) this should allow essentially any file to be read. Here the table could be even just one dataset
                //

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open file for reading using stored table definition: " << m_name;

                try {

                    m_group = H5Gopen(m_h5file, m_name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);

                    if (hasAttribute(m_group, "table")) {
                        Hash readDataFormatConfig;
                        readTableFormatFromAttribute(readDataFormatConfig);
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "read format: \n" << readDataFormatConfig;
                        m_dataFormat = Format::createNode("Format", "Format", readDataFormatConfig);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("auto discovery not enabled yet");
                        // if format not defined as attribute discover it from data structure
                        //discover(m_dataFormat, m_name.c_str());
                        //KARABO_LOG_FRAMEWORK_TRACE_CF << m_dataFormat->getConfig() << endl;
                    }
                } catch (...) {
                    KARABO_RETHROW
                }

                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                KARABO_LOG_FRAMEWORK_TRACE_CF << "elements.size() : " << elements.size();
                for (size_t i = 0; i < elements.size(); ++i) {                    
                    elements[i]->open(m_group);
                }
                retrieveNumberOfRecordsFromFile();
            }


            void Table::append(const karabo::util::Hash& data) {
                size_t recordNumber = m_tableSize;
                write(data, recordNumber);
            }


            void Table::write(const karabo::util::Hash& data, size_t recordId) {

                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->write(data, recordId);
                }
                if (m_tableSize <= recordId) {
                    m_tableSize = recordId + 1;
                    updateTableSizeAttribute();
                }
                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));
           

            }


            void Table::write(const karabo::util::Hash& data, size_t recordId, size_t len) {

                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->write(data, recordId, len);
                }

                hsize_t possibleNewSize = recordId + len;
                if (m_tableSize < possibleNewSize ) {
                    m_tableSize = possibleNewSize;
                    updateTableSizeAttribute();
                }
            
                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_GLOBAL));
                updateTableSizeAttribute();
            }
           

            void Table::bind(karabo::util::Hash& data) {
                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data);
                }
            }
            
            
            void Table::bind(karabo::util::Hash& data, size_t bufferLen) {
                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data, bufferLen);
                }
            }
            
            size_t Table::read(size_t recordNumber) {

                if ( recordNumber >= m_tableSize ) return 0;
                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read  element " << i;
                    elements[i]->read(recordNumber);
                }
                return 1ul;

            }
            
            size_t Table::read(size_t recordNumber, size_t len) {

                size_t numberReadRecords = (recordNumber + len ) < m_tableSize ? len : (m_tableSize - recordNumber);
                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read  element " << i;
                    elements[i]->read(recordNumber, numberReadRecords);
                }
                return numberReadRecords;
            }
            

            size_t Table::size() {
                return m_tableSize;
            }


            void Table::close() {

                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->close();
                }
                KARABO_CHECK_HDF5_STATUS(H5Gclose(m_group));
                KARABO_CHECK_HDF5_STATUS(H5Aclose(m_numberOfRecordsAttribute));
            }




            // end of public functions


            void Table::createEmptyTable(hid_t h5file, const boost::filesystem::path& fullPath) {

                //                clog << "Table fullpath: " << fullPath.string() << endl;
                try {
                    vector<string> tokens;
                    boost::split(tokens, fullPath.string(), boost::is_any_of("/"));

                    hid_t group = H5Gopen(h5file, "/", H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(group);
                    //H5::Group group(h5file->openGroup("/"));
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        // skip empty tokens (like in: "/a/b//c" -> "a","b","","c") 
                        if (tokens[i].size() == 0) continue;

                        hid_t nextGroup;
                        if (H5Lexists(group, tokens[i].c_str(), H5P_DEFAULT) != 0) {
                            if (i == tokens.size() - 1) {
                                ostringstream os;
                                os << "Table " << fullPath.c_str() << " already exists";
                                throw KARABO_IO_EXCEPTION(os.str());
                            }
                            nextGroup = H5Gopen(group, tokens[i].c_str(), H5P_DEFAULT);
                            KARABO_CHECK_HDF5_STATUS(nextGroup);
                        } else {
                            nextGroup = H5Gcreate(group, tokens[i].c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
                            KARABO_CHECK_HDF5_STATUS(nextGroup);
                        }
                        KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
                        group = nextGroup;
                    }
                    KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
                    m_group = H5Gopen(h5file, fullPath.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);
                    m_h5Groups[""] = m_group;
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

                hid_t schemaVersion = H5Acreate(m_group, "schemaVersion", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(schemaVersion)
                string version = Format::classInfo().getVersion();
                const char* versionPtr = version.c_str();
                //                clog << "schema version: " << version << endl;
                status = H5Awrite(schemaVersion, stringType, &versionPtr);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Aclose(schemaVersion));

            }


            void Table::createInitialNumberOfRecordsAttribute() {

                hid_t dataSpace = H5Screate(H5S_SCALAR);
                m_numberOfRecordsAttribute = H5Acreate(m_group, TABLE_SIZE, H5T_STD_U64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(m_numberOfRecordsAttribute);
                updateTableSizeAttribute();
            }


            void Table::updateTableSizeAttribute() {
                hid_t type = H5Tcopy(H5T_NATIVE_HSIZE);
                herr_t status = H5Awrite(m_numberOfRecordsAttribute, type, &m_tableSize);
                if (status < 0) {
                    throw KARABO_HDF_IO_EXCEPTION("Could not write numberOfRecords attribute");
                }
            }


            void Table::retrieveNumberOfRecordsFromFile() {

                if (hasAttribute(m_group, TABLE_SIZE)) {
                    m_numberOfRecordsAttribute = H5Aopen(m_group, TABLE_SIZE, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_numberOfRecordsAttribute);

                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_numberOfRecordsAttribute, H5T_NATIVE_HSIZE, &m_tableSize));
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute for " << m_name.c_str() << " is " << m_tableSize;
                } else {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute not defined for " << m_name;
                    //calculateNumberOfRecords();
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Calculated number of records: " << m_tableSize;
                }
            }

            //
            //            void Table::calculateNumberOfRecords() {
            //                m_numberOfRecords = r_calculateNumberOfRecords(m_recordFormatHash);
            //            }
            //
            //            void Table::retrieveChunkSizeFromFile() {
            //                m_chunkSize = r_getChunkSize(m_recordFormatHash);
            //            }
            //


            void Table::saveTableFormatAsAttribute(const karabo::io::h5::Format::Pointer dataFormat) {

                try {

                    //                    const Hash& dataFormatConfig = dataFormat->getConfig();

                    //                    Schema schema = Format::getSchema("Format");
                    Hash persistentDataFormatConfig;
                    //                    Hash& elements = persistentDataFormatConfig.bindReference<Hash>("Format");
                    //                    HashFilter::byTag(schema, dataFormatConfig.get<Hash>("Format"), elements, "persistent");

                    dataFormat->getPersistentConfig(persistentDataFormatConfig);

                    KARABO_LOG_FRAMEWORK_TRACE_CF << persistentDataFormatConfig;

                    Hash c("Xml.indentation", 1);//, "Xml.writeDataTypes", false);
                    TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create(c);

                    string dataFormatConfigXml;

                    serializer->save(persistentDataFormatConfig, dataFormatConfigXml);

                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Description of format to be written to hdf5 file as group attribute:\n " << dataFormatConfigXml;
                    hid_t dataSpace = H5Screate(H5S_SCALAR);
                    hid_t tableAttribute = H5Acreate(m_group, "table", ScalarTypes::getHdf5StandardType<string>(), dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tableAttribute);
                    const char* ptr = dataFormatConfigXml.c_str();
                    KARABO_CHECK_HDF5_STATUS(H5Awrite(tableAttribute, ScalarTypes::getHdf5NativeType<string>(), &ptr));
                } catch (...) {
                    KARABO_RETHROW
                }
            }


            void Table::readTableFormatFromAttribute(karabo::util::Hash& dataFormatConfig) {

                try {
                    // read the format from group attribute

                    hid_t tableAttribute = H5Aopen(m_group, "table", H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(tableAttribute);

                    char* ptr[1];

                    KARABO_CHECK_HDF5_STATUS(H5Aread(tableAttribute, ScalarTypes::getHdf5NativeType<string>(), &ptr));
                    string dataFormatConfigXml = ptr[0];

                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Read format:\n " << dataFormatConfigXml;

                    TextSerializer<Hash>::Pointer serializer = TextSerializer<Hash>::create("Xml");
                    serializer->load(dataFormatConfig, dataFormatConfigXml);




                } catch (...) {
                    KARABO_RETHROW
                }
            }



            //
            //
            //            void Table::refreshRecordFormatVector() {
            //                m_recordFormatVector.clear();
            //                r_refreshRecordFormatVector(m_recordFormatHash, m_recordFormatVector);
            //            }
            //
            //            void Table::r_refreshRecordFormatVector(const karabo::util::Hash& recordFormat, std::vector< const boost::any*>& recordFormatVector) {
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_refreshRecordFormatVector(recordFormat.get<Hash > (it), recordFormatVector);
            //                        continue;
            //                    }
            //                    m_recordFormatVector.push_back(&(it->second));
            //                }
            //
            //            }
            //


            void Table::defineStructure() {

                vector<boost::shared_ptr<Element> > elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->openParentGroup(m_h5Groups);
                    elements[i]->create(m_chunkSize);
                }
            }
            //
            //            void Table::r_openStructure(const Hash& recordFormat, boost::shared_ptr<H5::Group> group) {
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        boost::shared_ptr<H5::Group> groupNext = boost::shared_ptr<H5::Group > (new H5::Group(group->openGroup(key)));
            //                        r_openStructure(recordFormat.get<Hash > (it), groupNext);
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->open(group);
            //                }
            //            }
            //


            //
            //            void Table::r_write(const karabo::util::Hash& data, size_t recordNumber, size_t len, const Hash& recordFormat) {
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_write(data.get<Hash > (key), recordNumber, len, recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->write(data, recordNumber, len);
            //                }
            //            }
            //
            //            void Table::r_extendRecordSpace(size_t len, const Hash & recordFormat) {
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_extendRecordSpace(len, recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->extend(len);
            //                }
            //            }
            //
            //            void Table::r_allocate(karabo::util::Hash& data, const karabo::util::Hash& recordFormat) {
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        if (!data.has(key)) {
            //                            data.set(key, Hash());
            //                        }
            //                        r_allocate(data.get<Hash > (key), recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->allocate(data);
            //                }
            //            }
            //
            //            void Table::r_allocate(karabo::util::Hash& data, size_t len, const karabo::util::Hash& recordFormat) {
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        if (!data.has(key)) {
            //                            data.set(key, Hash());
            //                        }
            //                        r_allocate(data.get<Hash > (key), len, recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->allocate(data, len);
            //                }
            //            }
            //
            //            void Table::r_read(karabo::util::Hash& data, size_t recordNumber, const Hash & recordFormat) {
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_read(data.get<Hash > (key), recordNumber, recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->read(data, recordNumber);
            //                }
            //            }
            //
            //            void Table::r_read(karabo::util::Hash& data, size_t recordNumber, size_t len, const Hash & recordFormat) {
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_read(data.get<Hash > (key), recordNumber, len, recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->read(data, recordNumber, len);
            //                }
            //            }
            //
            //            void Table::r_readAttributes(karabo::util::Hash& attr, const Hash & recordFormat) {
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        attr.set(key, Hash());
            //                        r_readAttributes(attr.get<Hash > (key), recordFormat.get<Hash > (it));
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
            //                    element->readAttributes(attr);
            //                }
            //            }
            //
            //            hsize_t Table::r_calculateNumberOfRecords(const Hash& recordFormat, bool firstTime) {
            //
            //                // This function calculates the number of records for each element (dataset)
            //                // It throws exception when the number of records for all elements is not the same.
            //                // This is important as a client can define essentially any element to be part of a table (for reading purpose)
            //                // The last feature exists to allow any hdf5 file to be read by this software.
            //
            //                static hsize_t lastCalculatedNumberOfRecords = 0;
            //                if (firstTime == true) {
            //                    lastCalculatedNumberOfRecords = 0;
            //                }
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "r_calculateNumberOfRecords enter function: last: " << lastCalculatedNumberOfRecords << endl;
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_calculateNumberOfRecords(recordFormat.get<Hash > (key), firstTime);
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(key);
            //                    hsize_t numberOfRecords = element->getNumberOfRecords();
            //                    //KARABO_LOG_FRAMEWORK_TRACE_CF<< "r_calculateNumberOfRecords  last: " << lastCalculatedNumberOfRecords << endl;
            //                    //KARABO_LOG_FRAMEWORK_TRACE_CF<< "numberOfRecords : " << numberOfRecords << endl;
            //                    if (firstTime == false && numberOfRecords != lastCalculatedNumberOfRecords) {
            //                        throw KARABO_IO_EXCEPTION("Table contains fields with not consistent number of records.");
            //                    }
            //                    if (firstTime == true) {
            //                        lastCalculatedNumberOfRecords = numberOfRecords;
            //                        firstTime = false;
            //                    }
            //                }
            //                return lastCalculatedNumberOfRecords;
            //            }
            //
            //            hsize_t Table::r_getChunkSize(const Hash& recordFormat, bool firstTime) {
            //
            //                // This function calculates the number of records for each element (dataset)
            //                // It throws exception when the number of records for all elements is not the same.
            //                // This is important as a client can define essentially any element to be part of a table (for reading purpose)
            //                // The last feature exists to allow any hdf5 file to be read by this software.
            //
            //                static hsize_t lastCalculatedChunkSize = 0;
            //                if (firstTime == true) {
            //                    lastCalculatedChunkSize = 0;
            //                }
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "r_getChunkSize enter function: last: " << lastCalculatedChunkSize << endl;
            //
            //                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
            //                        r_getChunkSize(recordFormat.get<Hash > (key), firstTime);
            //                        continue;
            //                    }
            //                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(key);
            //                    hsize_t chunkSize = element->getChunkSize();
            //                    //KARABO_LOG_FRAMEWORK_TRACE_CF<< "r_calculateNumberOfRecords  last: " << lastCalculatedNumberOfRecords << endl;
            //                    //KARABO_LOG_FRAMEWORK_TRACE_CF<< "numberOfRecords : " << numberOfRecords << endl;
            //                    if (firstTime == false && chunkSize != lastCalculatedChunkSize) {
            //                        throw KARABO_IO_EXCEPTION("Table contains fields with not consistent chunk size - this is not supported at the moment.");
            //                    }
            //                    if (firstTime == true) {
            //                        lastCalculatedChunkSize = chunkSize;
            //                        firstTime = false;
            //                    }
            //                }
            //                return lastCalculatedChunkSize;
            //            }
            //


            bool Table::hasAttribute(hid_t group, const string& name) const {
                return ( H5Aexists(group, name.c_str()) > 0 ? true : false);
            }
            //
            //            void Table::initializeCache() {
            //                m_cacheStart = 0; //inclusive
            //                m_cacheEnd = 0; // exclusive
            //                m_cacheSize = m_chunkSize;
            //                allocate(m_cache, m_chunkSize);
            //            }
            //
            //            void Table::refreshCache(size_t recordNumber) {
            //
            //                size_t chunkNumber = static_cast<size_t> (recordNumber / m_chunkSize);
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "chunkNumber: " << chunkNumber << endl;
            //                size_t firstChunkRecord = chunkNumber * m_chunkSize;
            //                unsigned long long lastChunkRecord = firstChunkRecord + m_cacheSize;
            //                long long t = lastChunkRecord - m_numberOfRecords;
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "t: " << t << endl;
            //                size_t nRecordsToRead = m_chunkSize;
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "firstChunkRecord: " << firstChunkRecord << endl;
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "lastChunkRecord: " << lastChunkRecord << endl;
            //                KARABO_LOG_FRAMEWORK_TRACE_CF<< "nRecordsToRead: " << nRecordsToRead << endl;
            //
            //                if (t > 0) {
            //                    nRecordsToRead = m_chunkSize - t;
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "nRecordsToRead updated: " << nRecordsToRead << endl;
            //                }
            //                readBuffer(m_cache, firstChunkRecord, nRecordsToRead);
            //                m_cacheStart = firstChunkRecord;
            //                m_cacheEnd = m_cacheStart + m_cacheSize;
            //
            //            }
            //
            //
            //            // Code below needs cleaning
            //
            //
            //
            //            //void Table::select(Hash& activeElements) {
            //            //  m_activatedElements = activeElements;
            //            //}
            //
            //            void Table::r_filter(const Hash& discovered, const Hash& selection, Hash & output) {
            //
            //                //KARABO_LOG_FRAMEWORK_TRACE_CF<< "r_filter " << endl;
            //                for (Hash::const_iterator it = selection.begin(); it != selection.end(); ++it) {
            //                    const string& key = it->first;
            //                    if (!discovered.has(key)) {
            //                        string msg = "Requested data structure \"" + key + "\" does not exist";
            //                        throw KARABO_IO_EXCEPTION(msg);
            //                    }
            //                    if (discovered.getTypeAsId(key) == Types::HASH) {
            //                        //KARABO_LOG_FRAMEWORK_TRACE_CF<< "discovered Hash " << key << endl;
            //                        if (discovered.has(key)) {
            //                            //KARABO_LOG_FRAMEWORK_TRACE_CF<< "discovered exists  " << key << endl;
            //                            if (selection.getTypeAsId(key) == Types::HASH) {
            //                                //KARABO_LOG_FRAMEWORK_TRACE_CF<< "selection is Hash  " << key << endl;
            //                                output.set(key, Hash());
            //                                r_filter(discovered.get<Hash > (key), selection.get<Hash > (key), output.get<Hash > (key));
            //                            } else {
            //                                // check if true/false
            //                                //KARABO_LOG_FRAMEWORK_TRACE_CF<< "discovered is NOT a Hash  " << key << endl;
            //                                output.set(key, discovered.get<Hash > (key));
            //                            }
            //                        }
            //                        continue;
            //                    }
            //
            //                    //KARABO_LOG_FRAMEWORK_TRACE_CF<< "discovered is not a Hash  " << key << endl;
            //                    if (selection.has(key)) {
            //                        //KARABO_LOG_FRAMEWORK_TRACE_CF<< "selection is element  " << key << endl;
            //                        boost::shared_ptr<RecordElement> element = discovered.get<boost::shared_ptr<RecordElement> >(key);
            //                        output.set(key, element);
            //                        /*
            //                                  if (selection.is<int>(key)) {
            //                                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Hash element " << key << " is int" << endl;
            //                                  } else if (selection.is<short>(key)) {
            //                                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Hash element " << key << " is short" << endl;
            //                                  } else {
            //                                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Hash element " << key << " is not known" << endl;
            //                                  }
            //                         */
            //
            //
            //
            //                    }
            //                }
            //
            //            }
            //
            //            /*
            //             *  Used by discover function. Callback for H5Ovisit.
            //             */
            //            herr_t Table::fileInfo(hid_t loc_id, const char *name, const H5O_info_t *info, void *opdata) {
            //
            //                Hash* discovered = (Hash*) opdata;
            //
            //                if (name[0] == '.') {
            //                    return 0;
            //                }
            //                if (info->type == H5O_TYPE_GROUP) {
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Group:   " << name << endl;
            //
            //                } else if (info->type == H5O_TYPE_DATASET) {
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Dataset: " << name << endl;
            //                    hid_t ds_id = H5Dopen(loc_id, name, H5P_DEFAULT);
            //                    DataSet dataset(ds_id);
            //                    string p(name);
            //                    boost::trim(p);
            //                    vector<string> v;
            //                    boost::split(v, p, is_any_of("/"));
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "v.size(): " << v.size() << endl;
            //                    string dataSetName = v[v.size() - 1];
            //                    string dataBlockName = "";
            //                    if (v.size() > 1) {
            //                        dataBlockName = v[v.size() - 2];
            //                    }
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "After dataBlockName discovered: " << dataBlockName << endl;
            //                    ostringstream os;
            //
            //                    if (v.size() > 2) {
            //                        for (size_t i = 0; i < v.size() - 2; ++i) {
            //                            os << v[i] << "/";
            //                        }
            //                    }
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Before prefix" << endl;
            //                    string prefix = os.str();
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "dsN: " << dataSetName << endl;
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "groupN: " << dataBlockName << endl;
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "prefix: " << prefix << endl;
            //                    trim_right_if(prefix, is_any_of("/"));
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "prefix: " << prefix << endl;
            //                    if (!prefix.empty()) prefix = prefix + "/";
            //
            //
            //                    Hash& dbs = discovered->get<Hash > ("dataBlocks");
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "dbs1: " << dbs << endl;
            //                    if (!dbs.has(dataBlockName)) {
            //                        dbs.set(dataBlockName, Hash());
            //                    }
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "dbs2: " << dbs << endl;
            //
            //                    Hash& dataBlock = dbs.get<Hash > (dataBlockName);
            //                    dataBlock.setFromPath(dataSetName, Hash(), "/");
            //                    Hash & dataSet = dataBlock.get<Hash > (dataSetName);
            //
            //                    bool isSupportedType = true;
            //
            //                    H5T_class_t typeClass = dataset.getTypeClass();
            //                    if (typeClass == H5T_INTEGER) {
            //
            //                        IntType intype = dataset.getIntType();
            //                        if (intype == PredType::NATIVE_INT8) {
            //                            dataSet.set("type", "Int8");
            //                        } else if (intype == PredType::NATIVE_INT16) {
            //                            dataSet.set("type", "Int16");
            //                        } else if (intype == PredType::NATIVE_INT32) {
            //                            dataSet.set("type", "Int32");
            //                        } else if (intype == PredType::NATIVE_INT64) {
            //                            dataSet.set("type", "Int64");
            //                        } else if (intype == PredType::NATIVE_UINT8) {
            //                            dataSet.set("type", "UInt8");
            //                        } else if (intype == PredType::NATIVE_UINT16) {
            //                            dataSet.set("type", "UInt16");
            //                        } else if (intype == PredType::NATIVE_UINT32) {
            //                            dataSet.set("type", "UInt32");
            //                        } else if (intype == PredType::NATIVE_UINT64) {
            //                            dataSet.set("type", "UInt64");
            //                        } else {
            //                            isSupportedType = false;
            //                        }
            //                    } else if (typeClass == H5T_FLOAT) {
            //
            //                        FloatType ftype = dataset.getFloatType();
            //                        if (ftype == PredType::NATIVE_DOUBLE) {
            //                            dataSet.set("type", "Double");
            //                        } else if (ftype == PredType::NATIVE_FLOAT) {
            //                            dataSet.set("type", "Float");
            //                        }
            //
            //                    } else if (typeClass == H5T_STRING) {
            //                        dataSet.set("type", "String");
            //
            //                    } else if (typeClass == H5T_ARRAY) {
            //                        ArrayType atype = dataset.getArrayType();
            //                        int ndims = atype.getArrayNDims();
            //
            //                        vector<hsize_t> dims(ndims, 0);
            //                        atype.getArrayDims(&dims[0]);
            //
            //                        DataType dataType = atype.getSuper();
            //                        if (dataType == PredType::NATIVE_UINT16) {
            //                            KARABO_LOG_FRAMEWORK_TRACE_CF<< "array of uint16" << endl;
            //                            dataSet.set("type", "UInt16Array");
            //                        } else if (dataType == PredType::NATIVE_FLOAT) {
            //                            dataSet.set("type", "FloatArray");
            //                        } else if (dataType == PredType::NATIVE_DOUBLE) {
            //                            dataSet.set("type", "DoubleArray");
            //                        } else if (dataType == PredType::NATIVE_INT32) {
            //                            dataSet.set("type", "Int32Array");
            //                        } else if (dataType == PredType::NATIVE_INT16) {
            //                            dataSet.set("type", "Int16Array");
            //                        } else if (dataType == PredType::NATIVE_UINT32) {
            //                            dataSet.set("type", "UInt32Array");
            //                        } else if (dataType == PredType::NATIVE_INT64) {
            //                            dataSet.set("type", "Int64Array");
            //                        } else if (dataType == PredType::NATIVE_UINT64) {
            //                            dataSet.set("type", "UInt64Array");
            //                        } else if (dataType == PredType::NATIVE_INT8) {
            //                            dataSet.set("type", "Int8Array");
            //                        } else if (dataType == PredType::NATIVE_UINT8) {
            //                            dataSet.set("type", "UInt8Array");
            //
            //
            //                        } else {
            //                            KARABO_LOG_FRAMEWORK_TRACE_CF<< "unknown array" << endl;
            //                            dataBlock.erase(dataSetName);
            //                            isSupportedType = false;
            //                        }
            //                        if (isSupportedType) {
            //                            dataSet.set("dims", dims);
            //                            KARABO_LOG_FRAMEWORK_TRACE_CF<< "type: " << dataSet.get<string > ("type") << endl;
            //                        }
            //
            //                    } else if (typeClass == H5T_COMPOUND) {
            //                        //dataSet.set("type", "compound");
            //                        dataBlock.erase(dataSetName);
            //                        isSupportedType = false;
            //                    } else {
            //                        dataBlock.erase(dataSetName);
            //                        isSupportedType = false;
            //                    }
            //
            //
            //                    if (isSupportedType) {
            //                        Hash& recordFormat = discovered->get<Hash > ("RecordFormat");
            //                        Hash& groupHash = recordFormat.bindReference<Hash > (prefix + dataBlockName);
            //                        groupHash.set("group", dataBlockName);
            //                        groupHash.set("path", prefix);
            //                    }
            //
            //
            //                    // remove everything which is not supported
            //                    if (dbs.has(dataBlockName)) {
            //                        const Hash& dblock = dbs.get<Hash > (dataBlockName);
            //                        if (dblock.size() == 0) {
            //                            dbs.erase(dataBlockName);
            //                        }
            //                    }
            //
            //
            //
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "DUPA: \n" << dbs << "\nDUPA-END" << endl;
            //                    // TODO - check if compression is used
            //                    // DSetCreatPropList plist = dataset.getCreatePlist();
            //
            //
            //                    //dataSet.set("useCompression", false);
            //                    dataset.close();
            //                }
            //                return 0;
            //            }
            //
            //            void Table::discover(DataFormat::Pointer& dataFormat, std::string groupName) {
            //
            //                try {
            //                    Hash discovered;
            //
            //                    discovered.setFromPath("dataBlocks", Hash());
            //                    discovered.setFromPath("RecordFormat", Hash());
            //                    void* d = (void*) &discovered;
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Iterating over all elements starting from group " << groupName << endl;
            //
            //                    H5::Group dataGroup(m_h5file->openGroup(groupName));
            //                    herr_t status = H5Ovisit(dataGroup.getId(), H5_INDEX_NAME, H5_ITER_NATIVE, &Table::fileInfo, d);
            //                    if (status < 0) {
            //                        throw KARABO_IO_EXCEPTION("Could not discover hdf5 structure in file: " + m_h5file->getFileName());
            //                    }
            //
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "discovered: \n" << discovered << endl;
            //
            //                    Hash& discoveredDataBlocks = discovered.get<Hash > ("dataBlocks");
            //                    Hash& discoveredRecordFormat = discovered.get<Hash > ("RecordFormat");
            //                    Hash conf;
            //                    conf.setFromPath("DataFormat.dataBlocks", vector<Hash > ());
            //
            //                    vector<Hash>& vectorOfDataBlocks = conf.getFromPath<vector<Hash> >("DataFormat.dataBlocks");
            //
            //                    for (Hash::const_iterator it = discoveredDataBlocks.begin(); it != discoveredDataBlocks.end(); ++it) {
            //                        const string& key = it->first;
            //                        KARABO_LOG_FRAMEWORK_TRACE_CF<< "key: " << key << endl;
            //                        Hash & dataBlockHash = discoveredDataBlocks.get<Hash > (key);
            //                        Hash dataBlock;
            //                        dataBlock.setFromPath("DataBlock.name", key);
            //                        for (Hash::const_iterator it2 = dataBlockHash.begin(); it2 != dataBlockHash.end(); ++it2) {
            //                            const string& key2 = it2->first;
            //                            KARABO_LOG_FRAMEWORK_TRACE_CF<< "  key2:" << key2 << endl;
            //                            const string& type = dataBlockHash.getFromPath<string > (key2 + ".type");
            //                            dataBlock.setFromPath("DataBlock.elements[next]." + type + ".dataset", key2);
            //
            //
            //                            Hash& blockHash = dataBlockHash.getFromPath<Hash > (key2);
            //                            KARABO_LOG_FRAMEWORK_TRACE_CF<< "blockHash: " << blockHash << endl;
            //                            if (blockHash.has("dims")) {
            //                                KARABO_LOG_FRAMEWORK_TRACE_CF<< "blockHash has key 'dims' " << endl;
            //                                vector<unsigned long long> dims = dataBlockHash.getFromPath<vector<unsigned long long> >(key2 + ".dims");
            //                                dataBlock.setFromPath("DataBlock.elements[last]." + type + ".dims", dims);
            //                            }
            //
            //                        }
            //                        vectorOfDataBlocks.push_back(dataBlock);
            //                    }
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "before adding Group.name and Group.path" << endl;
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "discoveredRecordFormat: \n" << discoveredRecordFormat << endl;
            //                    for (Hash::const_iterator it = discoveredRecordFormat.begin(); it != discoveredRecordFormat.end(); ++it) {
            //                        const string& key = it->first;
            //                        KARABO_LOG_FRAMEWORK_TRACE_CF<< "key: " << key << endl;
            //                        const Hash& tmp = discoveredRecordFormat.get<Hash > (key);
            //                        KARABO_LOG_FRAMEWORK_TRACE_CF<< "tmp (what is that?) " << tmp << endl;
            //                        conf.setFromPath("DataFormat.RecordFormat.groups[next].Group.name", tmp.get<string > ("group"));
            //                        conf.setFromPath("DataFormat.RecordFormat.groups[last].Group.path", tmp.get<string > ("path"));
            //                    }
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "before writing CrossCheckFormat" << endl;
            //                    conf.setFromPath("DataFormat.RecordFormat.root", "");
            //
            //                    Hash writerConfig;
            //                    writerConfig.setFromPath("TextFile.filename", "CrossCheckFormat.xml");
            //                    Writer<Hash>::Pointer formatWriter = Writer<Hash>::create(writerConfig);
            //                    formatWriter->write(conf);
            //
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "after writing CrossCheckFormat" << endl;
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Full: \n" << conf << endl;
            //                    for (size_t i = 0; i < vectorOfDataBlocks.size(); ++i) {
            //                        KARABO_LOG_FRAMEWORK_TRACE_CF<< "vec:\n" << vectorOfDataBlocks[i] << endl;
            //                    }
            //
            //
            //                    dataFormat = DataFormat::create(conf);
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "dataFormat " << dataFormat << endl;
            //                    RecordFormat::Pointer recordFormat = dataFormat->getRecordFormat();
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "After getRecordFormat" << endl;
            //                    recordFormat->getHash(m_recordFormatHash);
            //                    KARABO_LOG_FRAMEWORK_TRACE_CF<< "Hash structure: " << m_recordFormatHash << endl;
            //                } catch (...) {
            //                    KARABO_RETHROW;
            //                }
            //                return;
            //            }
            //

        }
    }
}
