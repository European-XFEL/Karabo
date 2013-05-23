/*
 * $Id: VLArray.hh 9577 2013-04-30 16:06:45Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_VLARRAY_HH
#define	KARABO_IO_H5_VLARRAY_HH


#include <vector>
#include <string>

#include <karabo/util/Configurator.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/FromLiteral.hh>

#include "Dataset.hh"
#include "TypeTraits.hh"
#include "DatasetReader.hh"
#include "DatasetWriter.hh"
#include "ErrorHandler.hh"




namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T>
            class VLArray : public Dataset {

            public:

                KARABO_CLASSINFO(VLArray, "VLARRAY_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")


                VLArray(const karabo::util::Hash& input) : Dataset(input, this) {

                    //                    std::string type = input.get<std::string>("type");
                    //                    m_memoryType = karabo::util::Types::from<karabo::util::FromLiteral>(type);
                    //                    std::string datasetWriterClassId = "DatasetWriter_" + type;

                    //                    KARABO_LOG_FRAMEWORK_TRACE_CF << "dWClassId " << datasetWriterClassId;
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "classId " << Self::classInfo().getClassId();
                    karabo::util::Hash config("dims", dims().toVector());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "config " << config;
                    //                    m_datasetWriter = DatasetWriter<T>::create(datasetWriterClassId, config);
                    //                    m_datasetReader = DatasetReader<T>::create("DatasetReader", config);

                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    return m_memoryType;
                }

                virtual ~VLArray() {
                }

                static void expectedParameters(karabo::util::Schema& expected) {


                    //                    karabo::util::STRING_ELEMENT(expected)
                    //                            .key("type")
                    //                            .displayedName("Type")
                    //                            .description("Data Type in Hash")
                    //                            .assignmentOptional().defaultValue(VLArray<T>::classInfo().getClassId())
                    //                            .reconfigurable()
                    //                            .commit();

                }

                void close() {
                    Dataset::close();
                }

                hid_t getDatasetTypeId() {
                    return H5Tvlen_create(ScalarTypes::getHdf5StandardType<T > ());
                }

                void writeNode(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "writing one record of " << m_key;
                    try {

                        const T* value = node.getValue<T*>();
                        
                        hvl_t v;
                        v.p = (void *)value;
                        v.len = node.getAttribute<int>("size");
                        hid_t tid = H5Tvlen_create( ScalarTypes::getHdf5NativeType<T > ());
                        hid_t ms = Dataset::dataSpace(karabo::util::Dims());
                        herr_t status = H5Dwrite(dataSet, tid, ms, fileDataSpace, H5P_DEFAULT, &v);
                        KARABO_CHECK_HDF5_STATUS(status);
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                        //                        m_datasetWriter->write(node, 1, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.VLArray") << "writing " << len << " records of " << m_key;
                    try {
                        //                        m_datasetWriter->write(node, len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }

                void bind(karabo::util::Hash & data) {

                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(dims().size());
                        //                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        //                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            int Please_Ignore_These_Warnings =0;
                            std::vector<T>& vec = node->getValue< std::vector<T> >();
                            //                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T* >();
                            //                            m_datasetReader->bind(ptr);
                            //                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        }
                    }

                }

                void bind(karabo::util::Hash & data, hsize_t len) {

                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(dims().size() * len);
                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        //                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue< std::vector<T> >();
                            //                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T* >();
                            //                            m_datasetReader->bind(ptr);
                            //                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        }
                    }


                }

                void readRecord(const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        //                        m_datasetReader->read(dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }

                void readRecords(hsize_t len, const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        //                        m_datasetReader->read(len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW;
                    }

                }





            protected:

                //                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                //                typename karabo::io::h5::DatasetReader<T>::Pointer m_datasetReader;
                karabo::util::Types::ReferenceType m_memoryType;

            };

            // typedefs
            typedef VLArray<signed char> Int8VLArrayElement;
            typedef VLArray<short> Int16VLArrayElement;
            typedef VLArray<int> Int32VLArrayElement;
            typedef VLArray<long long > Int64VLArrayElement;
            typedef VLArray<unsigned char> UInt8VLArrayElement;
            typedef VLArray<unsigned short> UInt16VLArrayElement;
            typedef VLArray<unsigned int> UInt32VLArrayElement;
            typedef VLArray<unsigned long long > UInt64VLArrayElement;
            typedef VLArray<double> DoubleVLArrayElement;
            typedef VLArray<float> FloatVLArrayElement;
            typedef VLArray<std::string> StringVLArrayElement;
            typedef VLArray<bool> BoolVLArrayElement;


        }
    }
}

#endif	
