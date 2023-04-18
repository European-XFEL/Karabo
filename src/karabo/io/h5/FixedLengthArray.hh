/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_FIXEDLENGTHARRAY_HH
#define KARABO_IO_H5_FIXEDLENGTHARRAY_HH


#include <karabo/util/Configurator.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/SimpleElement.hh>
#include <karabo/util/VectorElement.hh>
#include <string>
#include <vector>

#include "Dataset.hh"
#include "DatasetReader.hh"
#include "DatasetWriter.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class FixedLengthArray
             * @brief The FixedLengthArray class is an implementation of Dataset for arrays of fixed length
             */
            template <typename T>
            class FixedLengthArray : public Dataset {
               public:
                KARABO_CLASSINFO(FixedLengthArray,
                                 "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(
                                                   karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "2.0")


                FixedLengthArray(const karabo::util::Hash& input) : Dataset(input, this) {
                    std::string type = FixedLengthArray<T>::classInfo().getClassId();
                    if (input.has("type")) {
                        type = input.get<std::string>("type");
                    }
                    m_memoryType = karabo::util::Types::from<karabo::util::FromLiteral>(type);
                    std::string datasetWriterClassId = "DatasetWriter_" + type;

                    KARABO_LOG_FRAMEWORK_TRACE_CF << "dWClassId " << datasetWriterClassId;
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "classId " << Self::classInfo().getClassId();
                    karabo::util::Hash config("dims", dims().toVector());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "config " << config;
                    m_datasetWriter =
                          karabo::util::Configurator<DatasetWriter<T> >::create(datasetWriterClassId, config, false);
                    m_datasetReader =
                          karabo::util::Configurator<DatasetReader<T> >::create("DatasetReader", config, false);
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    return m_memoryType;
                }

                virtual ~FixedLengthArray() {}

                static void expectedParameters(karabo::util::Schema& expected) {
                    karabo::util::STRING_ELEMENT(expected)
                          .key("type")
                          .displayedName("Type")
                          .description("Data Type in Hash")
                          .assignmentOptional()
                          .noDefaultValue()
                          .reconfigurable()
                          .commit();
                }

                void close() {
                    Dataset::close();
                }

                hid_t getDatasetTypeId() {
                    return ScalarTypes::getHdf5StandardType<T>();
                }

                void writeNode(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.FixedLengthArray") << "writing one record of " << m_key;
                    try {
                        m_datasetWriter->write(node, 1, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.FixedLengthArray")
                          << "writing " << len << " records of " << m_key;
                    try {
                        m_datasetWriter->write(node, len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to dataset /" + m_h5PathName));
                    }
                }

                void bind(karabo::util::Hash& data) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(dims().size());
                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue<std::vector<T> >();
                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T*>();
                            m_datasetReader->bind(ptr);
                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        }
                    }
                }

                void bind(karabo::util::Hash& data, hsize_t len) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(dims().size() * len);
                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue<std::vector<T> >();
                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T*>();
                            m_datasetReader->bind(ptr);
                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        }
                    }
                }

                void readRecord(const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        m_datasetReader->read(dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }

                void readRecords(hsize_t len, const hid_t& dataSet, const hid_t& fileDataSpace) {
                    try {
                        m_datasetReader->read(len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW;
                    }
                }


               protected:
                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                typename karabo::io::h5::DatasetReader<T>::Pointer m_datasetReader;
                karabo::util::Types::ReferenceType m_memoryType;
            };

            // typedefs
            typedef FixedLengthArray<char> CharArrayElement;
            typedef FixedLengthArray<signed char> Int8ArrayElement;
            typedef FixedLengthArray<short> Int16ArrayElement;
            typedef FixedLengthArray<int> Int32ArrayElement;
            typedef FixedLengthArray<long long> Int64ArrayElement;
            typedef FixedLengthArray<unsigned char> UInt8ArrayElement;
            typedef FixedLengthArray<unsigned short> UInt16ArrayElement;
            typedef FixedLengthArray<unsigned int> UInt32ArrayElement;
            typedef FixedLengthArray<unsigned long long> UInt64ArrayElement;
            typedef FixedLengthArray<double> DoubleArrayElement;
            typedef FixedLengthArray<float> FloatArrayElement;
            typedef FixedLengthArray<std::string> StringArrayElement;
            typedef FixedLengthArray<bool> BoolArrayElement;


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif /* KARABO_IO_FIXEDLENGTHARRAY_HH */
