/*
 * $Id: Complex.hh 9561 2013-04-29 08:01:43Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_COMPLEX_HH
#define	KARABO_IO_H5_COMPLEX_HH


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
            class Complex : public Dataset {

            public:

                KARABO_CLASSINFO(Complex, "COMPLEX_" +
                                 karabo::util::ToType<karabo::util::ToLiteral>::
                                 to(
                                    karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0"
                                 )


                Complex(const karabo::util::Hash& input) : Dataset(input, this) {

                    //karabo::util::Dims dims = getSingleValueDimensions();
                    karabo::util::Hash config("dims", dims().toVector());
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "classId " << Self::classInfo().getClassId();
                    m_datasetWriter = DatasetWriter<T>::create("DatasetWriter_" + Complex<T>::classInfo().getClassId(), config);
                    m_datasetReader = DatasetReader<T>::create("DatasetReader", config);
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims(2);
                }

                virtual ~Complex() {
                }

                static void expectedParameters(karabo::util::Schema& expected) {


                    karabo::util::STRING_ELEMENT(expected)
                            .key("type")
                            .displayedName("Type")
                            .description("Data Type in Hash")
                            .assignmentOptional().defaultValue(Complex<T>::classInfo().getClassId())
                            .reconfigurable()
                            .commit();

                }

                void close() {
                    Dataset::close();
                }

                hid_t getDatasetTypeId() {
                    return ScalarTypes::getHdf5StandardType<T > ();
                }

                void writeNode(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Complex") << "writing one record of " << m_key;
                    try {
                        m_datasetWriter->write(node, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Complex") << "writing " << len << " records of " << m_key;
                    try {
                        m_datasetWriter->write(node, len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" + m_h5PathName));
                    }
                }

                void bind(karabo::util::Hash & data) {

                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    if (!node) {
                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                        vec.resize(dims().size());
                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        m_datasetReader->bind(vec);

                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& vec = node->getValue< std::vector<T> >();
                            m_datasetReader->bind(vec);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T* >();
                            m_datasetReader->bind(ptr);
                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                        }
                    }

                }

                void bind(karabo::util::Hash & data, hsize_t len) {
                    //
                    //                    boost::optional<karabo::util::Hash::Node&> node = data.find(m_key, '/');
                    //                    if (!node) {
                    //                        std::vector<T>& vec = data.bindReference<std::vector<T> >(m_key, '/');
                    //                        vec.resize(dims().size() * len);
                    //                        data.setAttribute(m_key, "dims", dims().toVector(), '/');
                    //                        m_datasetReader->bind(vec);
                    //
                    //                    } else {
                    //                        if (karabo::util::Types::isVector(node->getType())) {
                    //                            std::vector<T>& vec = node->getValue< std::vector<T> >();
                    //                            m_datasetReader->bind(vec);
                    //                        } else if (karabo::util::Types::isPointer(node->getType())) {
                    //                            T* ptr = node->getValue<T* >();
                    //                            m_datasetReader->bind(ptr);
                    //                            data.setAttribute(m_key, "dims", dims().toVector(), '/');
                    //                        }
                    //                    }


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

                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                typename karabo::io::h5::DatasetReader<T>::Pointer m_datasetReader;

            };

            // typedefs
            typedef Complex<double> DoubleComplexElement;
            typedef Complex<float> FloatComplexElement;



        }
    }
}

#endif	/* KARABO_IO_H5_COMPLEX_HH */
