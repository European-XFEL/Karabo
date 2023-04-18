/*
 * $Id$
 *
 * Author: <steffen.hauf@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_FIXEDLENGTHARRAY_HH
#define KARABO_IO_H5_FIXEDLENGTHARRAY_HH


#include <karabo/util/Configurator.hh>
#include <karabo/util/FromLiteral.hh>
#include <karabo/util/NDArray.hh>
#include <karabo/util/SimpleElement.hh>
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
             * @class NDArrayH5
             * @brief The NDArrayH5 class is an implementation of Dataset for karabo::util::NDArray type data
             */
            template <typename T>
            class NDArrayH5 : public Dataset {
               public:
                KARABO_CLASSINFO(NDArrayH5,
                                 "NDArrayH5_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "2.0")


                NDArrayH5(const karabo::util::Hash& input) : Dataset(input, this) {
                    const size_t prefixLength = std::string("NDArrayH5_").size();
                    const std::string type = (NDArrayH5<T>::classInfo().getClassId()).substr(prefixLength);

                    m_memoryType = karabo::util::Types::from<karabo::util::FromLiteral>(type);
                    const std::string datasetWriterClassId = "DatasetWriter_NDArrayH5" + type;

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

                virtual ~NDArrayH5() {}

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
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.NDArrayH5") << "writing one record of " << m_key;
                    try {
                        m_datasetWriter->write(node, 1, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.NDArrayH5")
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
                        karabo::util::NDArray arr(dims(),
                                                  karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T)));
                        T* ptr = arr.getData<T>();
                        m_datasetReader->bind(ptr);
                        data.set(m_key, arr, '/');
                    } else {
                        if (node->getType() == karabo::util::Types::HASH) {
                            if (node->hasAttribute(KARABO_HASH_CLASS_ID) &&
                                node->getAttribute<std::string>(KARABO_HASH_CLASS_ID) ==
                                      karabo::util::NDArray::classInfo().getClassId()) {
                                karabo::util::NDArray& arr = node->getValue<karabo::util::NDArray>();
                                T* ptr = arr.getData<T>();
                                arr.setShape(dims()); // technically not needed but assures that NDarray shape and data
                                                      // shape match
                                m_datasetReader->bind(ptr);
                            }
                        }
                    }
                }

                void bind(karabo::util::Hash& data, hsize_t len) {
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        auto dimsV = dims().toVector();
                        dimsV.insert(dimsV.begin(), len);
                        karabo::util::NDArray arr(karabo::util::Dims(dimsV),
                                                  karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T)));
                        T* ptr = arr.getData<T>();
                        m_datasetReader->bind(ptr);
                        data.set(m_key, arr, '/');

                    } else {
                        if (node->getType() == karabo::util::Types::HASH) {
                            if (node->hasAttribute(KARABO_HASH_CLASS_ID) &&
                                node->getAttribute<std::string>(KARABO_HASH_CLASS_ID) ==
                                      karabo::util::NDArray::classInfo().getClassId()) {
                                karabo::util::NDArray& arr = node->getValue<karabo::util::NDArray>();
                                T* ptr = arr.getData<T>();
                                auto dimsV = dims().toVector();
                                dimsV.insert(dimsV.begin(), len);
                                arr.setShape(karabo::util::Dims(dimsV)); // technically not needed but assures that
                                                                         // NDarray shape and data shape match
                                m_datasetReader->bind(ptr);
                            }
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
            typedef NDArrayH5<char> CharNDArrayH5Element;
            typedef NDArrayH5<signed char> Int8NDArrayH5Element;
            typedef NDArrayH5<short> Int16NDArrayH5Element;
            typedef NDArrayH5<int> Int32NDArrayH5Element;
            typedef NDArrayH5<long long> Int64NDArrayH5Element;
            typedef NDArrayH5<unsigned char> UInt8NDArrayH5Element;
            typedef NDArrayH5<unsigned short> UInt16NDArrayH5Element;
            typedef NDArrayH5<unsigned int> UInt32NDArrayH5Element;
            typedef NDArrayH5<unsigned long long> UInt64NDArrayH5Element;
            typedef NDArrayH5<double> DoubleNDArrayH5Element;
            typedef NDArrayH5<float> FloatNDArrayH5Element;
            typedef NDArrayH5<std::string> StringNDArrayH5Element;
            typedef NDArrayH5<bool> BoolNDArrayH5Element;
            typedef NDArrayH5<std::complex<float> > ComplexFloatNDArrayH5Element;
            typedef NDArrayH5<std::complex<double> > ComplexDoubleNDArrayH5Element;


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif /* KARABO_IO_NDARRAYH5_HH */
