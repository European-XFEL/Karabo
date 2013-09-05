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
#include <karabo/util/TimeProfiler.hh>



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

//                                Profiler p("hdf5");                
                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open new file: " << m_name;
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
//                                clog << "createEmptyTable: " << HighResolutionTimer::time2double(p.getTime("createEmptyTable")) << endl;
//                                clog << "createSchemaVersionAttribute: " << HighResolutionTimer::time2double(p.getTime("createSchemaVersionAttribute")) << endl;
//                                clog << "createNumberOfRecordsAttribute " << HighResolutionTimer::time2double(p.getTime("createNumberOfRecordsAttribute")) << endl;
//                                clog << "saveTableFormat " << HighResolutionTimer::time2double(p.getTime("saveTableFormat")) << endl;
//                                clog << "create:   " << HighResolutionTimer::time2double(p.getTime("create")) << endl;

            }


            void Table::openReadOnly(const karabo::io::h5::Format::Pointer dataFormat) {

                KARABO_LOG_FRAMEWORK_TRACE_CF << "Open file for reading with specific user defined format: " << m_name;
                m_dataFormat = dataFormat;
                try {

                    m_group = H5Gopen(m_h5file, m_name.c_str(), H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);

                    const vector<Element::Pointer >& elements = m_dataFormat->getElements();
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
                        //clog << m_name << " before read   " << HighResolutionTimer::now().sec << endl;
                        readTableFormatFromAttribute(readDataFormatConfig);
                        //clog << m_name << " before format " << HighResolutionTimer::now().sec << endl;
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "read format: \n" << readDataFormatConfig;
                        m_dataFormat = Format::createFormat(readDataFormatConfig);
                        //clog << m_name << " after format  " << HighResolutionTimer::now().sec << endl;
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("auto discovery not enabled yet");
                        // if format not defined as attribute discover it from data structure
                        //discover(m_dataFormat, m_name.c_str());
                        //KARABO_LOG_FRAMEWORK_TRACE_CF << m_dataFormat->getConfig() << endl;
                    }
                } catch (...) {
                    KARABO_RETHROW
                }

                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                KARABO_LOG_FRAMEWORK_TRACE_CF << "elements.size() : " << elements.size();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->open(m_group);
                }
                //clog << m_name << " after open    " << HighResolutionTimer::now().sec << endl;
                retrieveNumberOfRecordsFromFile();
            }


            void Table::append(const karabo::util::Hash& data) {
                write(data, m_tableSize);
            }
 

            void Table::write(const karabo::util::Hash& data, size_t recordId) {

                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    try {
                        elements[i]->write(data, recordId);
                    } catch (Exception& ex) {
                        //TODO think what to do here
                        ex.clearTrace();
                        //clog << "element " << i << " could not be written" << endl;
                    }
                }
                if (m_tableSize <= recordId) {
                    m_tableSize = recordId + 1;
                    updateTableSizeAttribute();
                }
                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));


            }


            void Table::write(const karabo::util::Hash& data, size_t recordId, size_t len) {

                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->write(data, recordId, len);
                }

                hsize_t possibleNewSize = recordId + len;
                if (m_tableSize < possibleNewSize) {
                    m_tableSize = possibleNewSize;
                    updateTableSizeAttribute();
                }

                KARABO_CHECK_HDF5_STATUS(H5Fflush(m_h5file, H5F_SCOPE_LOCAL));

            }


            void Table::writeAttributes(const karabo::util::Hash& data) {
                const vector<Element::Pointer>& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    //elements[i]->open(m_group);
                    elements[i]->saveAttributes(m_group, data);
                    //elements[i]->close();

                }
            }


            void Table::bind(karabo::util::Hash& data) {
                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data);
                }
            }


            void Table::bind(karabo::util::Hash& data, size_t bufferLen) {
                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->bind(data, bufferLen);
                }
            }


            size_t Table::read(size_t recordNumber) {

                if (recordNumber >= m_tableSize) return 0;
                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read  element " << elements[i]->getFullName();
                    elements[i]->read(recordNumber);
                }
                return 1ul;

            }


            size_t Table::read(size_t recordNumber, size_t len) {

                size_t numberReadRecords = (recordNumber + len) < m_tableSize ? len : (m_tableSize - recordNumber);
                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table::read  element " << i;
                    elements[i]->read(recordNumber, numberReadRecords);
                }
                return numberReadRecords;
            }


            void Table::readAttributes(karabo::util::Hash& data) {
                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
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

                const vector<Element::Pointer >& elements = m_dataFormat->getElements();
                for (size_t i = 0; i < elements.size(); ++i) {
                    elements[i]->close();
                }
                if (m_group >= 0) KARABO_CHECK_HDF5_STATUS(H5Gclose(m_group));
                if (m_numberOfRecordsAttribute >= 0) KARABO_CHECK_HDF5_STATUS(H5Aclose(m_numberOfRecordsAttribute));

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

                    //create table group
                    m_group = H5Gcreate(h5file, path.c_str(), lcpl, gcpl, H5P_DEFAULT);
                    KARABO_CHECK_HDF5_STATUS(m_group);
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Table: " << fullPath.string() << " created. group id = " << m_group;
                    //KARABO_CHECK_HDF5_STATUS(H5Gclose(group));
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

                hid_t schemaVersion = H5Acreate(m_group, "schemaVersion", stringType, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
                KARABO_CHECK_HDF5_STATUS(schemaVersion)
                string version = Format::classInfo().getVersion();
                const char* versionPtr = version.c_str();
                //                clog << "schema version: " << version << endl;
                status = H5Awrite(schemaVersion, stringType, &versionPtr);
                KARABO_CHECK_HDF5_STATUS(status);
                KARABO_CHECK_HDF5_STATUS(H5Aclose(schemaVersion));
                KARABO_CHECK_HDF5_STATUS(H5Tclose(stringType))
                KARABO_CHECK_HDF5_STATUS(H5Sclose(dataSpace))

            }


            void Table::createInitialNumberOfRecordsAttribute() {

                hid_t dataSpace = H5Screate(H5S_SCALAR);
                m_numberOfRecordsAttribute = H5Acreate(m_group, TABLE_SIZE, H5T_STD_U64LE, dataSpace, H5P_DEFAULT, H5P_DEFAULT);
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
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute for " << m_name.c_str() << " is " << m_tableSize;
                } else {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "numberOfRecords attribute not defined for " << m_name;
                    //calculateNumberOfRecords();
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

                    KARABO_LOG_FRAMEWORK_TRACE_CF << "Description of format to be written to hdf5 file as group attribute:\n " << dataFormatConfigXml;
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
//                    hsize_t dims[1] = {1};
//                    int ndims = H5Sget_simple_extent_dims(space, dims, NULL);
//                    KARABO_CHECK_HDF5_STATUS(ndims);

                    //char** rdata = (char **) malloc(dims[0] * sizeof (char *));
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
                    //free(rdata);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(memtype));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(space));
                    
                    //KARABO_CHECK_HDF5_STATUS(H5Tclose(tid))
                    KARABO_CHECK_HDF5_STATUS(H5Aclose(tableAttribute));


                } catch (...) {
                    KARABO_RETHROW
                }
            }


            bool Table::hasAttribute(hid_t group, const string& name) const {
                return ( H5Aexists(group, name.c_str()) > 0 ? true : false);
            }

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
