/*
 * $Id: Table.cc 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "Table.hh"
#include "Scalar.hh"


#include "../Writer.hh"
#include "../Reader.hh"

#include "../ioProfiler.hh"

using namespace std;
using namespace exfel::util;
using namespace H5;
using namespace boost;

namespace exfel {
    namespace io {
        namespace hdf5 {

            EXFEL_REGISTER_ONLY_ME_CC(Table)


            Table::~Table() {
            }

            void Table::expectedParameters(Schema& expected) {

                PATH_ELEMENT(expected).key("table")
                        .description("Name of the table with defined record structure")
                        .displayedName("Table")
                        .assignmentMandatory()
                        .commit();

                UINT64_ELEMENT(expected)
                        .key("chunkSize")
                        .displayedName("HDF5 Chunk Size")
                        .description("Chunk size used for HDF5")
                        .assignmentOptional().defaultValue(1)
                        .minInc(1)
                        .advanced()
                        .commit();


                INTERNAL_ANY_ELEMENT(expected)
                        .key("h5file")
                        .description("shared pointer to H5::H5File")
                        .commit();

            }

            void Table::configure(const Hash& input) {
                input.get("table", m_name);
                input.get("chunkSize", m_chunkSize);
                input.get("h5file", m_h5file);
                m_numberOfRecords = 0;

            }

            void Table::openNew(const DataFormat::Pointer dataFormat) {

                createEmptyTable(m_h5file, m_name);
                createSchemaVersionAttribute();
                createInitialNumberOfRecordsAttribute();
                saveTableFormatAsAttribute(dataFormat);
                defineRecordFormat(dataFormat);
            }

            void Table::openReadOnly(const exfel::io::hdf5::DataFormat::Pointer dataFormat) {

                m_dataFormat = dataFormat;
                try {
                    m_group = boost::shared_ptr<H5::Group > (new H5::Group(m_h5file->openGroup(m_name.c_str())));
                    openRecordStructure();
                    retrieveNumberOfRecordsFromFile();
                } catch (...) {
                    RETHROW
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
                // entries (implemented as 1Dim dataset) and there is one-to-one relation. 
                // 3) this should allow essentially any file to be read. Here the table could be even just one dataset
                //


                try {
                    m_group = boost::shared_ptr<H5::Group > (new H5::Group(m_h5file->openGroup(m_name.c_str())));
                    if (hasAttribute(m_group, "table")) {
                        Hash readDataFormatConfig;
                        readTableFormatFromAttribute(readDataFormatConfig);
                        m_dataFormat = DataFormat::create(readDataFormatConfig);
                    } else {
                        // if format not defined as attribute discover it from data structure
                        discover(m_dataFormat, m_name.c_str());
                        tracer << m_dataFormat->getConfig() << endl;
                    }
                } catch (...) {
                    RETHROW
                }
                openRecordStructure();
                retrieveNumberOfRecordsFromFile();
                retrieveChunkSizeFromFile();
                if (m_useCache) initializeCache();
            }

            void Table::append(const exfel::util::Hash& data) {
                size_t recordNumber = m_numberOfRecords;
                write(data, recordNumber);
            }

            void Table::write(const exfel::util::Hash& data, size_t recordNumber) {

                if (recordNumber >= m_numberOfRecords && recordNumber % m_chunkSize == 0) {
                    m_h5file->flush(H5F_SCOPE_GLOBAL);
                    r_extendRecordSpace(m_chunkSize, m_recordFormatHash);
                }
                r_write(data, recordNumber, m_recordFormatHash);
                if (m_numberOfRecords <= recordNumber) {
                    m_numberOfRecords++;
                }
                updateNumberOfRecordsAttribute();
            }

            void Table::writeBuffer(const exfel::util::Hash& data, size_t recordNumber, size_t len) {

                size_t missingRecords = recordNumber + len - m_numberOfRecords;
                tracer << "recordNumber: " << recordNumber << " len: " << len << endl
                        << "m_numberOfRecords: " << m_numberOfRecords << " missing: " << missingRecords << endl;

                if (missingRecords > 0) {
                    m_h5file->flush(H5F_SCOPE_GLOBAL);
                    //r_extendRecordSpace(missingRecords, m_recordFormatHash);
                }

                EXFEL_PROFILER_TABLE1

                if (missingRecords > 0) {

                    EXFEL_PROFILER_START_TABLE1("flush")
                    m_h5file->flush(H5F_SCOPE_GLOBAL);
                    EXFEL_PROFILER_STOP_TABLE1

                    for (size_t j = 0; j < m_recordFormatVector.size(); ++j) {
                        EXFEL_PROFILER_START_TABLE1("getElement")
                                const boost::any& anyElement = *(m_recordFormatVector[j]);
                        boost::shared_ptr<RecordElement> element = boost::any_cast<boost::shared_ptr<RecordElement> >(anyElement);
                        EXFEL_PROFILER_STOP_TABLE1
                        EXFEL_PROFILER_START_TABLE1("extend")
                        element->extend(missingRecords);
                        EXFEL_PROFILER_STOP_TABLE1
                        EXFEL_PROFILER_START_TABLE1("write");
                        element->write(data, recordNumber, len);
                        EXFEL_PROFILER_STOP_TABLE1
                    }

                    EXFEL_PROFILER_REPORT_TABLE1("getElement")
                    EXFEL_PROFILER_REPORT_TABLE1("extend")
                    EXFEL_PROFILER_REPORT_TABLE1("write")



                } else {
                    for (size_t j = 0; j < m_recordFormatVector.size(); ++j) {
                        const boost::any& anyElement = *(m_recordFormatVector[j]);
                        boost::shared_ptr<RecordElement> element = boost::any_cast<boost::shared_ptr<RecordElement> >(anyElement);
                        element->write(data, recordNumber, len);
                    }
                }
                //		p.start("r_write");
                //		r_write(data, recordNumber, len, m_recordFormatHash);
                //		p.stop();
                //		cout << "r_write     : " << fixed << HighResolutionTimer::time2double(p.getTime("r_write")) << endl;
                if (m_numberOfRecords <= recordNumber) {
                    m_numberOfRecords += len;
                }
                updateNumberOfRecordsAttribute();
            }

            void Table::allocate(exfel::util::Hash& data) {
                r_allocate(data, m_recordFormatHash);
            }

            void Table::allocate(exfel::util::Hash& data, size_t len) {
                r_allocate(data, len, m_recordFormatHash);
            }

            void Table::read(Hash & data, size_t recordNumber) {

                r_read(data, recordNumber, m_recordFormatHash);
            }

            void Table::readBuffer(exfel::util::Hash& data, size_t recordNumber, size_t len) {
                r_read(data, recordNumber, len, m_recordFormatHash);
            }

            void Table::read(size_t recordNumber) {
                r_read(m_readData, recordNumber, m_recordFormatHash);
            }

            void Table::readAttributes(Hash & attr) {
                r_readAttributes(attr, m_recordFormatHash);
            }

            size_t Table::getNumberOfRecords() {
                return m_numberOfRecords;
            }

            void Table::close() {
            }


            // end of public functions







            //////////////////////////////////////////////////////////////////////////////

            void Table::createEmptyTable(boost::shared_ptr<H5::H5File> h5file, const boost::filesystem::path& fullPath) {

                try {
                    vector<string> tokens;
                    boost::split(tokens, fullPath.string(), boost::is_any_of("/"));
                    H5::Group group(h5file->openGroup("/"));
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        // skip empty tokens (like in: "/a/b//c" -> "a","b","","c") 
                        if (tokens[i].size() == 0) continue;

                        H5::Group nextGroup;
                        if (H5Lexists(group.getLocId(), tokens[i].c_str(), H5P_DEFAULT) != 0) {
                            if (i == tokens.size() - 1) {
                                ostringstream os;
                                os << "Table " << fullPath.c_str() << " already exists";
                                throw IO_EXCEPTION(os.str());
                            }
                            nextGroup = group.openGroup(tokens[i]);
                        } else {
                            nextGroup = group.createGroup(tokens[i]);
                        }
                        group.close();
                        group = nextGroup;
                    }
                    group.close();
                    m_group = boost::shared_ptr<H5::Group > (new H5::Group(h5file->openGroup(fullPath.c_str())));
                } catch (...) {
                    RETHROW
                }

            }

            void Table::createSchemaVersionAttribute() {
                H5::StrType strType(H5::PredType::C_S1, H5T_VARIABLE);
                H5::Attribute schemaVersion = m_group->createAttribute("schemaVersion", strType, DataSpace(H5S_SCALAR));
                schemaVersion.write(strType, DataFormat::classInfo().getConfigVersion());
            }

            void Table::createInitialNumberOfRecordsAttribute() {
                m_numberOfRecordsAttribute = m_group->createAttribute("numberOfRecords", PredType::STD_U32LE, DataSpace(H5S_SCALAR));
                updateNumberOfRecordsAttribute();
            }

            void Table::updateNumberOfRecordsAttribute() {
                m_numberOfRecordsAttribute.write(PredType::NATIVE_HSIZE, &m_numberOfRecords);
            }

            void Table::retrieveNumberOfRecordsFromFile() {
                if (hasAttribute(m_group, "numberOfRecords")) {
                    m_numberOfRecordsAttribute = m_group->openAttribute("numberOfRecords");
                    m_numberOfRecordsAttribute.read(PredType::NATIVE_UINT, &m_numberOfRecords);
                    tracer << "numberOfRecords attribute for " << m_name.c_str() << " is " << m_numberOfRecords << endl;
                } else {
                    tracer << "numberOfRecords attribute not defined for " << m_name.c_str() << endl;
                    calculateNumberOfRecords();
                    tracer << "Calculated number of records: " << m_numberOfRecords << endl;
                }
            }

            void Table::calculateNumberOfRecords() {
                m_numberOfRecords = r_calculateNumberOfRecords(m_recordFormatHash);
            }

            void Table::retrieveChunkSizeFromFile() {
                m_chunkSize = r_getChunkSize(m_recordFormatHash);
            }

            void Table::saveTableFormatAsAttribute(const exfel::io::hdf5::DataFormat::Pointer dataFormat) {

                try {

                    Hash dataFormatConfig = dataFormat->getConfig();

                    StrType stringType(PredType::C_S1, H5T_VARIABLE);
                    Attribute attributeTable(m_group->createAttribute("table", stringType, DataSpace(H5S_SCALAR)));

                    string dataFormatConfigXml;
                    Hash writerConfig;

                    // write xml format description as group attribute
                    writerConfig.setFromPath("StringStream.format.Xml.indentation", 1);
                    writerConfig.setFromPath("StringStream.stringPointer", &dataFormatConfigXml);
                    Writer<Hash>::Pointer formatWriter = Writer<Hash>::create(writerConfig);
                    formatWriter->write(dataFormatConfig);
                    attributeTable.write(stringType, dataFormatConfigXml);

                } catch (...) {
                    RETHROW
                }
            }

            void Table::readTableFormatFromAttribute(exfel::util::Hash& dataFormatConfig) {

                try {
                    // read the format from group attribute
                    StrType stringType(PredType::C_S1, H5T_VARIABLE);
                    Attribute attribute(m_group->openAttribute("table"));

                    string dataFormatConfigXml("");
                    attribute.read(stringType, dataFormatConfigXml);

                    //tracer << "Xml: " << dataFormatConfigXml << endl;
                    Hash readerConfig;
                    readerConfig.setFromPath("StringStream.format.Xml");
                    readerConfig.setFromPath("StringStream.string", dataFormatConfigXml);
                    Reader<Hash>::Pointer formatReader = Reader<Hash>::create(readerConfig);
                    formatReader->read(dataFormatConfig);


                } catch (...) {
                    RETHROW
                }
            }

            void Table::defineRecordFormat(const DataFormat::Pointer dataFormat) {
                m_dataFormat = dataFormat;
                RecordFormat::Pointer recordFormat = m_dataFormat->getRecordFormat();
                recordFormat->getHash(m_recordFormatHash);
                r_defineStructure(m_recordFormatHash, m_group);
                refreshRecordFormatVector();
            }

            void Table::openRecordStructure() {

                RecordFormat::Pointer recordFormat = m_dataFormat->getRecordFormat();
                Hash discoveredRecordFormatHash;
                recordFormat->getHash(discoveredRecordFormatHash);

                //TODO: here place holder for selection
                //m_recordFormatHash.clear();
                //r_filter(discoveredRecordFormatHash, m_activatedElements, m_recordFormatHash );

                m_recordFormatHash = discoveredRecordFormatHash;
                refreshRecordFormatVector();

                r_openStructure(m_recordFormatHash, m_group);

            }

            void Table::refreshRecordFormatVector() {
                m_recordFormatVector.clear();
                r_refreshRecordFormatVector(m_recordFormatHash, m_recordFormatVector);
            }

            void Table::r_refreshRecordFormatVector(const exfel::util::Hash& recordFormat, std::vector< const boost::any*>& recordFormatVector) {
                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_refreshRecordFormatVector(recordFormat.get<Hash > (it), recordFormatVector);
                        continue;
                    }
                    m_recordFormatVector.push_back(&(it->second));
                }

            }

            void Table::r_defineStructure(const Hash& recordFormat, boost::shared_ptr<H5::Group> group) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        boost::shared_ptr<H5::Group> groupNext = boost::shared_ptr<H5::Group > (new H5::Group(group->createGroup(key)));
                        r_defineStructure(recordFormat.get<Hash > (it), groupNext);
                        continue;
                    }
                    RecordElement::Pointer element = recordFormat.get<RecordElement::Pointer > (it);
                    element->create(group, m_chunkSize);
                }
            }

            void Table::r_openStructure(const Hash& recordFormat, boost::shared_ptr<H5::Group> group) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        boost::shared_ptr<H5::Group> groupNext = boost::shared_ptr<H5::Group > (new H5::Group(group->openGroup(key)));
                        r_openStructure(recordFormat.get<Hash > (it), groupNext);
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->open(group);
                }
            }

            void Table::r_write(const exfel::util::Hash& data, size_t recordNumber, const Hash& recordFormat) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_write(data.get<Hash > (key), recordNumber, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->write(data, recordNumber);
                }
            }

            void Table::r_write(const exfel::util::Hash& data, size_t recordNumber, size_t len, const Hash& recordFormat) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_write(data.get<Hash > (key), recordNumber, len, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->write(data, recordNumber, len);
                }
            }

            void Table::r_extendRecordSpace(size_t len, const Hash & recordFormat) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_extendRecordSpace(len, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->extend(len);
                }
            }

            void Table::r_allocate(exfel::util::Hash& data, const exfel::util::Hash& recordFormat) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        if (!data.has(key)) {
                            data.set(key, Hash());
                        }
                        r_allocate(data.get<Hash > (key), recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->allocate(data);
                }
            }

            void Table::r_allocate(exfel::util::Hash& data, size_t len, const exfel::util::Hash& recordFormat) {

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        if (!data.has(key)) {
                            data.set(key, Hash());
                        }
                        r_allocate(data.get<Hash > (key), len, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->allocate(data, len);
                }
            }

            void Table::r_read(exfel::util::Hash& data, size_t recordNumber, const Hash & recordFormat) {
                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_read(data.get<Hash > (key), recordNumber, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->read(data, recordNumber);
                }
            }

            void Table::r_read(exfel::util::Hash& data, size_t recordNumber, size_t len, const Hash & recordFormat) {
                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_read(data.get<Hash > (key), recordNumber, len, recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->read(data, recordNumber, len);
                }
            }

            void Table::r_readAttributes(exfel::util::Hash& attr, const Hash & recordFormat) {
                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        attr.set(key, Hash());
                        r_readAttributes(attr.get<Hash > (key), recordFormat.get<Hash > (it));
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(it);
                    element->readAttributes(attr);
                }
            }

            hsize_t Table::r_calculateNumberOfRecords(const Hash& recordFormat, bool firstTime) {

                // This function calculates the number of records for each element (dataset)
                // It throws exception when the number of records for all elements is not the same.
                // This is important as a client can define essentially any element to be part of a table (for reading purpose)
                // The last feature exists to allow any hdf5 file to be read by this software.

                static hsize_t lastCalculatedNumberOfRecords = 0;
                if (firstTime == true) {
                    lastCalculatedNumberOfRecords = 0;
                }
                tracer << "r_calculateNumberOfRecords enter function: last: " << lastCalculatedNumberOfRecords << endl;

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_calculateNumberOfRecords(recordFormat.get<Hash > (key), firstTime);
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(key);
                    hsize_t numberOfRecords = element->getNumberOfRecords();
                    //tracer << "r_calculateNumberOfRecords  last: " << lastCalculatedNumberOfRecords << endl;
                    //tracer << "numberOfRecords : " << numberOfRecords << endl;
                    if (firstTime == false && numberOfRecords != lastCalculatedNumberOfRecords) {
                        throw IO_EXCEPTION("Table contains fields with not consistent number of records.");
                    }
                    if (firstTime == true) {
                        lastCalculatedNumberOfRecords = numberOfRecords;
                        firstTime = false;
                    }
                }
                return lastCalculatedNumberOfRecords;
            }

            hsize_t Table::r_getChunkSize(const Hash& recordFormat, bool firstTime) {

                // This function calculates the number of records for each element (dataset)
                // It throws exception when the number of records for all elements is not the same.
                // This is important as a client can define essentially any element to be part of a table (for reading purpose)
                // The last feature exists to allow any hdf5 file to be read by this software.

                static hsize_t lastCalculatedChunkSize = 0;
                if (firstTime == true) {
                    lastCalculatedChunkSize = 0;
                }
                tracer << "r_getChunkSize enter function: last: " << lastCalculatedChunkSize << endl;

                for (Hash::const_iterator it = recordFormat.begin(); it != recordFormat.end(); ++it) {
                    const string& key = it->first;
                    if (recordFormat.getTypeAsId(it) == Types::HASH) {
                        r_getChunkSize(recordFormat.get<Hash > (key), firstTime);
                        continue;
                    }
                    boost::shared_ptr<RecordElement> element = recordFormat.get<boost::shared_ptr<RecordElement> >(key);
                    hsize_t chunkSize = element->getChunkSize();
                    //tracer << "r_calculateNumberOfRecords  last: " << lastCalculatedNumberOfRecords << endl;
                    //tracer << "numberOfRecords : " << numberOfRecords << endl;
                    if (firstTime == false && chunkSize != lastCalculatedChunkSize) {
                        throw IO_EXCEPTION("Table contains fields with not consistent chunk size - this is not supported at the moment.");
                    }
                    if (firstTime == true) {
                        lastCalculatedChunkSize = chunkSize;
                        firstTime = false;
                    }
                }
                return lastCalculatedChunkSize;
            }

            bool Table::hasAttribute(const boost::shared_ptr<H5::Group> group, const string & name) const {
                return ( H5Aexists(group->getLocId(), name.c_str()) > 0 ? true : false);
            }

            void Table::initializeCache() {
                m_cacheStart = 0; //inclusive
                m_cacheEnd = 0; // exclusive
                m_cacheSize = m_chunkSize;
                allocate(m_cache, m_chunkSize);
            }

            void Table::refreshCache(size_t recordNumber) {

                size_t chunkNumber = static_cast<size_t> (recordNumber / m_chunkSize);
                tracer << "chunkNumber: " << chunkNumber << endl;
                size_t firstChunkRecord = chunkNumber * m_chunkSize;
                unsigned long long lastChunkRecord = firstChunkRecord + m_cacheSize;
                long long t = lastChunkRecord - m_numberOfRecords;
                tracer << "t: " << t << endl;
                size_t nRecordsToRead = m_chunkSize;
                tracer << "firstChunkRecord: " << firstChunkRecord << endl;
                tracer << "lastChunkRecord: " << lastChunkRecord << endl;
                tracer << "nRecordsToRead: " << nRecordsToRead << endl;

                if (t > 0) {
                    nRecordsToRead = m_chunkSize - t;
                    tracer << "nRecordsToRead updated: " << nRecordsToRead << endl;
                }
                readBuffer(m_cache, firstChunkRecord, nRecordsToRead);
                m_cacheStart = firstChunkRecord;
                m_cacheEnd = m_cacheStart + m_cacheSize;

            }


            // Code below needs cleaning



            //void Table::select(Hash& activeElements) {
            //  m_activatedElements = activeElements;
            //}

            void Table::r_filter(const Hash& discovered, const Hash& selection, Hash & output) {

                //tracer << "r_filter " << endl;
                for (Hash::const_iterator it = selection.begin(); it != selection.end(); ++it) {
                    const string& key = it->first;
                    if (!discovered.has(key)) {
                        string msg = "Requested data structure \"" + key + "\" does not exist";
                        throw IO_EXCEPTION(msg);
                    }
                    if (discovered.getTypeAsId(key) == Types::HASH) {
                        //tracer << "discovered Hash " << key << endl;
                        if (discovered.has(key)) {
                            //tracer << "discovered exists  " << key << endl;
                            if (selection.getTypeAsId(key) == Types::HASH) {
                                //tracer << "selection is Hash  " << key << endl;
                                output.set(key, Hash());
                                r_filter(discovered.get<Hash > (key), selection.get<Hash > (key), output.get<Hash > (key));
                            } else {
                                // check if true/false
                                //tracer << "discovered is NOT a Hash  " << key << endl;
                                output.set(key, discovered.get<Hash > (key));
                            }
                        }
                        continue;
                    }

                    //tracer << "discovered is not a Hash  " << key << endl;
                    if (selection.has(key)) {
                        //tracer << "selection is element  " << key << endl;
                        boost::shared_ptr<RecordElement> element = discovered.get<boost::shared_ptr<RecordElement> >(key);
                        output.set(key, element);
                        /*
                                  if (selection.is<int>(key)) {
                                    tracer << "Hash element " << key << " is int" << endl;
                                  } else if (selection.is<short>(key)) {
                                    tracer << "Hash element " << key << " is short" << endl;
                                  } else {
                                    tracer << "Hash element " << key << " is not known" << endl;
                                  }
                         */



                    }
                }

            }

            /*
             *  Used by discover function. Callback for H5Ovisit.
             */
            herr_t Table::fileInfo(hid_t loc_id, const char *name, const H5O_info_t *info, void *opdata) {

                Hash* discovered = (Hash*) opdata;

                if (name[0] == '.') {
                    return 0;
                }
                if (info->type == H5O_TYPE_GROUP) {
                    tracer << "Group:   " << name << endl;

                } else if (info->type == H5O_TYPE_DATASET) {
                    tracer << "Dataset: " << name << endl;
                    hid_t ds_id = H5Dopen(loc_id, name, H5P_DEFAULT);
                    DataSet dataset(ds_id);
                    string p(name);
                    boost::trim(p);
                    vector<string> v;
                    boost::split(v, p, is_any_of("/"));
                    tracer << "v.size(): " << v.size() << endl;
                    string dataSetName = v[v.size() - 1];
                    string dataBlockName = "";
                    if (v.size() > 1) {
                        dataBlockName = v[v.size() - 2];
                    }
                    tracer << "After dataBlockName discovered: " << dataBlockName << endl;
                    ostringstream os;

                    if (v.size() > 2) {
                        for (size_t i = 0; i < v.size() - 2; ++i) {
                            os << v[i] << "/";
                        }
                    }

                    tracer << "Before prefix" << endl;
                    string prefix = os.str();
                    tracer << "dsN: " << dataSetName << endl;
                    tracer << "groupN: " << dataBlockName << endl;
                    tracer << "prefix: " << prefix << endl;
                    trim_right_if(prefix, is_any_of("/"));
                    tracer << "prefix: " << prefix << endl;
                    if (!prefix.empty()) prefix = prefix + "/";


                    Hash& dbs = discovered->get<Hash > ("dataBlocks");
                    tracer << "dbs1: " << dbs << endl;
                    if (!dbs.has(dataBlockName)) {
                        dbs.set(dataBlockName, Hash());
                    }
                    tracer << "dbs2: " << dbs << endl;

                    Hash& dataBlock = dbs.get<Hash > (dataBlockName);
                    dataBlock.setFromPath(dataSetName, Hash(), "/");
                    Hash & dataSet = dataBlock.get<Hash > (dataSetName);

                    bool isSupportedType = true;

                    H5T_class_t typeClass = dataset.getTypeClass();
                    if (typeClass == H5T_INTEGER) {

                        IntType intype = dataset.getIntType();
                        if (intype == PredType::NATIVE_INT8) {
                            dataSet.set("type", "Int8");
                        } else if (intype == PredType::NATIVE_INT16) {
                            dataSet.set("type", "Int16");
                        } else if (intype == PredType::NATIVE_INT32) {
                            dataSet.set("type", "Int32");
                        } else if (intype == PredType::NATIVE_INT64) {
                            dataSet.set("type", "Int64");
                        } else if (intype == PredType::NATIVE_UINT8) {
                            dataSet.set("type", "UInt8");
                        } else if (intype == PredType::NATIVE_UINT16) {
                            dataSet.set("type", "UInt16");
                        } else if (intype == PredType::NATIVE_UINT32) {
                            dataSet.set("type", "UInt32");
                        } else if (intype == PredType::NATIVE_UINT64) {
                            dataSet.set("type", "UInt64");
                        } else {
                            isSupportedType = false;
                        }
                    } else if (typeClass == H5T_FLOAT) {

                        FloatType ftype = dataset.getFloatType();
                        if (ftype == PredType::NATIVE_DOUBLE) {
                            dataSet.set("type", "Double");
                        } else if (ftype == PredType::NATIVE_FLOAT) {
                            dataSet.set("type", "Float");
                        }

                    } else if (typeClass == H5T_STRING) {
                        dataSet.set("type", "String");

                    } else if (typeClass == H5T_ARRAY) {
                        ArrayType atype = dataset.getArrayType();
                        int ndims = atype.getArrayNDims();

                        vector<hsize_t> dims(ndims, 0);
                        atype.getArrayDims(&dims[0]);

                        DataType dataType = atype.getSuper();
                        if (dataType == PredType::NATIVE_UINT16) {
                            tracer << "array of uint16" << endl;
                            dataSet.set("type", "UInt16Array");
                        } else if (dataType == PredType::NATIVE_FLOAT) {
                            dataSet.set("type", "FloatArray");
                        } else if (dataType == PredType::NATIVE_DOUBLE) {
                            dataSet.set("type", "DoubleArray");
                        } else if (dataType == PredType::NATIVE_INT32) {
                            dataSet.set("type", "Int32Array");
                        } else if (dataType == PredType::NATIVE_INT16) {
                            dataSet.set("type", "Int16Array");
                        } else if (dataType == PredType::NATIVE_UINT32) {
                            dataSet.set("type", "UInt32Array");
                        } else if (dataType == PredType::NATIVE_INT64) {
                            dataSet.set("type", "Int64Array");
                        } else if (dataType == PredType::NATIVE_UINT64) {
                            dataSet.set("type", "UInt64Array");
                        } else if (dataType == PredType::NATIVE_INT8) {
                            dataSet.set("type", "Int8Array");
                        } else if (dataType == PredType::NATIVE_UINT8) {
                            dataSet.set("type", "UInt8Array");


                        } else {
                            tracer << "unknown array" << endl;
                            dataBlock.erase(dataSetName);
                            isSupportedType = false;
                        }
                        if (isSupportedType) {
                            dataSet.set("dims", dims);
                            tracer << "type: " << dataSet.get<string > ("type") << endl;
                        }

                    } else if (typeClass == H5T_COMPOUND) {
                        //dataSet.set("type", "compound");
                        dataBlock.erase(dataSetName);
                        isSupportedType = false;
                    } else {
                        dataBlock.erase(dataSetName);
                        isSupportedType = false;
                    }


                    if (isSupportedType) {
                        Hash& recordFormat = discovered->get<Hash > ("RecordFormat");
                        Hash& groupHash = recordFormat.bindReference<Hash > (prefix + dataBlockName);
                        groupHash.set("group", dataBlockName);
                        groupHash.set("path", prefix);
                    }


                    // remove everything which is not supported
                    if (dbs.has(dataBlockName)) {
                        const Hash& dblock = dbs.get<Hash > (dataBlockName);
                        if (dblock.size() == 0) {
                            dbs.erase(dataBlockName);
                        }
                    }




                    tracer << "DUPA: \n" << dbs << "\nDUPA-END" << endl;
                    // TODO - check if compression is used
                    // DSetCreatPropList plist = dataset.getCreatePlist();


                    //dataSet.set("useCompression", false);
                    dataset.close();
                }
                return 0;
            }

            void Table::discover(DataFormat::Pointer& dataFormat, std::string groupName) {

                try {
                    Hash discovered;

                    discovered.setFromPath("dataBlocks", Hash());
                    discovered.setFromPath("RecordFormat", Hash());
                    void* d = (void*) &discovered;

                    tracer << "Iterating over all elements starting from group " << groupName << endl;

                    H5::Group dataGroup(m_h5file->openGroup(groupName));
                    herr_t status = H5Ovisit(dataGroup.getId(), H5_INDEX_NAME, H5_ITER_NATIVE, &Table::fileInfo, d);
                    if (status < 0) {
                        throw IO_EXCEPTION("Could not discover hdf5 structure in file: " + m_h5file->getFileName());
                    }


                    tracer << "discovered: \n" << discovered << endl;

                    Hash& discoveredDataBlocks = discovered.get<Hash > ("dataBlocks");
                    Hash& discoveredRecordFormat = discovered.get<Hash > ("RecordFormat");
                    Hash conf;
                    conf.setFromPath("DataFormat.dataBlocks", vector<Hash > ());

                    vector<Hash>& vectorOfDataBlocks = conf.getFromPath<vector<Hash> >("DataFormat.dataBlocks");

                    for (Hash::const_iterator it = discoveredDataBlocks.begin(); it != discoveredDataBlocks.end(); ++it) {
                        const string& key = it->first;
                        tracer << "key: " << key << endl;
                        Hash & dataBlockHash = discoveredDataBlocks.get<Hash > (key);
                        Hash dataBlock;
                        dataBlock.setFromPath("DataBlock.name", key);
                        for (Hash::const_iterator it2 = dataBlockHash.begin(); it2 != dataBlockHash.end(); ++it2) {
                            const string& key2 = it2->first;
                            tracer << "  key2:" << key2 << endl;
                            const string& type = dataBlockHash.getFromPath<string > (key2 + ".type");
                            dataBlock.setFromPath("DataBlock.elements[next]." + type + ".dataset", key2);


                            Hash& blockHash = dataBlockHash.getFromPath<Hash > (key2);
                            tracer << "blockHash: " << blockHash << endl;
                            if (blockHash.has("dims")) {
                                tracer << "blockHash has key 'dims' " << endl;
                                vector<unsigned long long> dims = dataBlockHash.getFromPath<vector<unsigned long long> >(key2 + ".dims");
                                dataBlock.setFromPath("DataBlock.elements[last]." + type + ".dims", dims);
                            }

                        }
                        vectorOfDataBlocks.push_back(dataBlock);
                    }

                    tracer << "before adding Group.name and Group.path" << endl;
                    tracer << "discoveredRecordFormat: \n" << discoveredRecordFormat << endl;
                    for (Hash::const_iterator it = discoveredRecordFormat.begin(); it != discoveredRecordFormat.end(); ++it) {
                        const string& key = it->first;
                        tracer << "key: " << key << endl;
                        const Hash& tmp = discoveredRecordFormat.get<Hash > (key);
                        tracer << "tmp (what is that?) " << tmp << endl;
                        conf.setFromPath("DataFormat.RecordFormat.groups[next].Group.name", tmp.get<string > ("group"));
                        conf.setFromPath("DataFormat.RecordFormat.groups[last].Group.path", tmp.get<string > ("path"));
                    }
                    tracer << "before writing CrossCheckFormat" << endl;
                    conf.setFromPath("DataFormat.RecordFormat.root", "");

                    Hash writerConfig;
                    writerConfig.setFromPath("TextFile.filename", "CrossCheckFormat.xml");
                    Writer<Hash>::Pointer formatWriter = Writer<Hash>::create(writerConfig);
                    formatWriter->write(conf);

                    tracer << "after writing CrossCheckFormat" << endl;
                    tracer << "Full: \n" << conf << endl;
                    for (size_t i = 0; i < vectorOfDataBlocks.size(); ++i) {
                        tracer << "vec:\n" << vectorOfDataBlocks[i] << endl;
                    }


                    dataFormat = DataFormat::create(conf);
                    tracer << "dataFormat " << dataFormat << endl;
                    RecordFormat::Pointer recordFormat = dataFormat->getRecordFormat();
                    tracer << "After getRecordFormat" << endl;
                    recordFormat->getHash(m_recordFormatHash);
                    tracer << "Hash structure: " << m_recordFormatHash << endl;
                } catch (...) {
                    RETHROW;
                }
                return;
            }


        }
    }
}
