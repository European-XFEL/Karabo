/*
 * $Id: Complex.hh 9561 2013-04-29 08:01:43Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_COMPLEX_HH
#define KARABO_IO_H5_COMPLEX_HH


#include <karabo/util/Configurator.hh>
#include <karabo/util/FromLiteral.hh>
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
             * @class Complex
             * @brief Maps complex number Karabo datatypes to corresponding HD5 types
             */
            template <typename T>
            class Complex : public Dataset {
               public:
                KARABO_CLASSINFO(Complex,
                                 "COMPLEX_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0")


                Complex(const karabo::util::Hash& input) : Dataset(input, this) {
                    // karabo::util::Dims dims = getSingleValueDimensions();
                    karabo::util::Hash config("dims", dims().toVector());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "classId " << Self::classInfo().getClassId();
                    m_datasetWriter = karabo::util::Configurator<DatasetWriter<std::complex<T> > >::create(
                          "DatasetWriter_" + Self::classInfo().getClassId(), config, false);
                    m_datasetReader = karabo::util::Configurator<DatasetReader<std::complex<T> > >::create(
                          "DatasetReader", config, false);
                }

                /**
                 * Return the dimensions of the element, overwrite for consistent interface.
                 * Will return vector of length two with entries 1
                 * @return
                 */
                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims(2);
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    return karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(std::complex<T>));
                }

                virtual ~Complex() {}

                static void expectedParameters(karabo::util::Schema& expected) {
                    karabo::util::STRING_ELEMENT(expected)
                          .key("type")
                          .displayedName("Type")
                          .description("Data Type in Hash")
                          .assignmentOptional()
                          .defaultValue(Complex<T>::classInfo().getClassId())
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
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Complex") << "writing one record of " << m_key;
                    try {
                        m_datasetWriter->write(node, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key +
                                                                      " to dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Complex")
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
                        std::complex<T>& cx = data.bindReference<std::complex<T> >(m_key, '/');
                        m_datasetReader->bind(&cx);

                    } else {
                        std::complex<T>& cx = node->getValue<std::complex<T> >();
                        m_datasetReader->bind(&cx);
                    }
                }

                void bind(karabo::util::Hash& data, hsize_t len) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Complex") << "bind " << len << " records of " << m_key;
                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<std::complex<T> >& vec =
                              data.bindReference<std::vector<std::complex<T> > >(m_key, '/');
                        vec.resize(len);

                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<std::complex<T> >& vec = node->getValue<std::vector<std::complex<T> > >();
                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            std::complex<T>* ptr = node->getValue<std::complex<T>*>();
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
                    //                    try {
                    //                        m_datasetReader->read(len, dataSet, fileDataSpace);
                    //                    } catch (...) {
                    //                        KARABO_RETHROW;
                    //                    }
                }


                //                void readSpecificAttributes(karabo::util::Hash& attributes) {
                //                    attributes.setFromPath(m_key + ".rank", static_cast<int> (dims().size()));
                //                    attributes.setFromPath(m_key + ".dims", dims());
                //                    attributes.setFromPath(m_key + ".typeCategory", "Complex");
                //                }
                //


               protected:
                typename karabo::io::h5::DatasetWriter<std::complex<T> >::Pointer m_datasetWriter;
                typename karabo::io::h5::DatasetReader<std::complex<T> >::Pointer m_datasetReader;
            };

            // typedefs
            typedef Complex<double> DoubleComplexElement;
            typedef Complex<float> FloatComplexElement;


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif /* KARABO_IO_H5_COMPLEX_HH */
