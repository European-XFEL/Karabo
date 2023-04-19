/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALAR_HH
#define KARABO_IO_H5_SCALAR_HH

#include <karabo/util/Configurator.hh>
#include <string>

#include "Dataset.hh"
#include "DatasetAttribute.hh"
#include "DatasetReader.hh"
#include "DatasetWriter.hh"
#include "ErrorHandler.hh"
#include "TypeTraits.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class Scalar
             * @brief The Scalar class is an implementation of Dataset for scalar data types
             */
            template <class T, class U = karabo::io::h5::Dataset>
            class Scalar : public U /*karabo::io::h5::Dataset*/ {
                static const std::string suffix(const DatasetAttribute*) {
                    return karabo::util::ToType<karabo::util::ToLiteral>::to(
                                 karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))) +
                           "ATTR";
                }

                static const std::string suffix(const Dataset*) {
                    return karabo::util::ToType<karabo::util::ToLiteral>::to(
                          karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T)));
                }

               public:
                KARABO_CLASSINFO(Scalar, suffix(static_cast<U*>(0)), "1.0")


                Scalar(const karabo::util::Hash& input) : U(input, this) {
                    karabo::util::Dims dims;
                    karabo::util::Hash config("dims", dims.toVector());
                    m_datasetWriter = karabo::util::Configurator<DatasetWriter<T> >::create(
                          "DatasetWriter_" + Scalar<T>::classInfo().getClassId(), config, false);
                    m_datasetReader =
                          karabo::util::Configurator<DatasetReader<T> >::create("DatasetReader", config, false);
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                karabo::util::Types::ReferenceType getMemoryType() const {
                    return karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T));
                }

                static hid_t m_dspace;

                static hid_t initDataSpace() {
                    hsize_t ex[] = {0};
                    hsize_t maxEx[] = {H5S_UNLIMITED};
                    return H5Screate_simple(1, ex, maxEx);
                }

                hid_t createDataspace(const std::vector<hsize_t>& ex, const std::vector<hsize_t>& maxEx) {
                    return this->m_dspace;
                }

                virtual ~Scalar() {}

                void closeDataspace(hid_t dataSpace) {}

                hid_t getDatasetTypeId() {
                    return ScalarTypes::getHdf5StandardType<T>();
                }

                void writeNode(const karabo::util::Hash::Node& node, hid_t dataSet, hid_t fileDataSpace) {
                    writeNode1(node, dataSet, fileDataSpace);
                }

                template <class HASH_ELEMENT>
                void writeNode1(const HASH_ELEMENT& node, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar") << "writing one record of " << this->m_key;
                    try {
                        m_datasetWriter->write(node, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + this->m_key +
                                                                      " to dataset /" + this->m_h5PathName));
                    }
                }

                void writeNode(const karabo::util::Hash::Node& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    writeNode1(node, len, dataSet, fileDataSpace);
                }

                void writeNode(const karabo::util::Element<std::string>& node, hsize_t len, hid_t dataSet,
                               hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar")
                          << "writing attr: " << len << " records of " << this->m_key;
                    writeNode1(node, len, dataSet, fileDataSpace);
                }

                template <class HASH_ELEMENT>
                void writeNode1(const HASH_ELEMENT& node, hsize_t len, hid_t dataSet, hid_t fileDataSpace) {
                    KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.Scalar")
                          << "writing " << len << " records of " << this->m_key;
                    try {
                        m_datasetWriter->write(node, len, dataSet, fileDataSpace);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + this->m_key +
                                                                      " to dataset /" + this->m_h5PathName));
                    }
                }

                inline void bind(karabo::util::Hash& data) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "bind: " << this->m_key;
                    if (!data.has(this->m_key, '/')) {
                        T& value = data.bindReference<T>(this->m_key, '/');
                        m_datasetReader->bind(&value);
                    } else {
                        T& value = data.get<T>(this->m_key, '/');
                        m_datasetReader->bind(&value);
                    }
                }

                void bind(karabo::util::Hash& data, hsize_t bufferLen) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "bind: " << this->m_key << " bufferLen: " << bufferLen;
                    boost::optional<karabo::util::Hash::Node&> node = data.find(this->m_key, '/');

                    if (!node) {
                        std::vector<T>& buf = data.bindReference<std::vector<T> >(this->m_key, '/');
                        buf.resize(bufferLen);
                        m_datasetReader->bind(buf);
                    } else {
                        if (karabo::util::Types::isVector(node->getType())) {
                            std::vector<T>& buf = node->getValue<std::vector<T> >();
                            m_datasetReader->bind(buf);
                        } else if (karabo::util::Types::isPointer(node->getType())) {
                            T* ptr = node->getValue<T*>();
                            m_datasetReader->bind(ptr);
                            data.setAttribute(this->m_key, "dims", this->dims().toVector(), '/');
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

               private:
                typename karabo::io::h5::DatasetWriter<T>::Pointer m_datasetWriter;
                typename karabo::io::h5::DatasetReader<T>::Pointer m_datasetReader;
            };

            // typedefs
            typedef Scalar<char> CharElement;
            typedef Scalar<signed char> Int8Element;
            typedef Scalar<short> Int16Element;
            typedef Scalar<int> Int32Element;
            typedef Scalar<long long> Int64Element;
            typedef Scalar<unsigned char> UInt8Element;
            typedef Scalar<unsigned short> UInt16Element;
            typedef Scalar<unsigned int> UInt32Element;
            typedef Scalar<unsigned long long> UInt64Element;
            typedef Scalar<double> DoubleElement;
            typedef Scalar<float> FloatElement;
            typedef Scalar<std::string> StringElement;
            typedef Scalar<bool> BoolElement;

            typedef Scalar<char, karabo::io::h5::DatasetAttribute> CharAttributeElement;
            typedef Scalar<signed char, karabo::io::h5::DatasetAttribute> Int8AttributeElement;
            typedef Scalar<short, karabo::io::h5::DatasetAttribute> Int16AttributeElement;
            typedef Scalar<int, karabo::io::h5::DatasetAttribute> Int32AttributeElement;
            typedef Scalar<long long, karabo::io::h5::DatasetAttribute> Int64AttributeElement;
            typedef Scalar<unsigned char, karabo::io::h5::DatasetAttribute> UInt8AttributeElement;
            typedef Scalar<unsigned short, karabo::io::h5::DatasetAttribute> UInt16AttributeElement;
            typedef Scalar<unsigned int, karabo::io::h5::DatasetAttribute> UInt32AttributeElement;
            typedef Scalar<unsigned long long, karabo::io::h5::DatasetAttribute> UInt64AttributeElement;
            typedef Scalar<double, karabo::io::h5::DatasetAttribute> DoubleAttributeElement;
            typedef Scalar<float, karabo::io::h5::DatasetAttribute> FloatAttributeElement;
            typedef Scalar<std::string, karabo::io::h5::DatasetAttribute> StringAttributeElement;
            typedef Scalar<bool, karabo::io::h5::DatasetAttribute> BoolAttributeElement;


            template <class T, class U>
            hid_t Scalar<T, U>::m_dspace = Scalar<T, U>::initDataSpace();
        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif /* KARABO_IO_SCALAR_HH */
