/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH
#define KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH


#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/VectorElement.hh>
#include <string>
#include <vector>

#include "Attribute.hh"
#include "FixedLengthArray.hh"
#include "TypeTraits.hh"


namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class FixedLengthArrayAttribute
             * @brief A class to represent vector attributes of fixed length in HDF5
             */
            template <typename T>
            class FixedLengthArrayAttribute : public Attribute {
               public:
                KARABO_CLASSINFO(FixedLengthArrayAttribute,
                                 "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(
                                                   karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input, this) {}

                virtual ~FixedLengthArrayAttribute() {}

                /**
                 * Return the dimensions of the attribute, overwrite for consistent interface.
                 * Will return vector of length one with entry 1
                 * @return
                 */
                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                /**
                 * Return the HDF5 type-id for the templatized type of ScalarAttribute
                 * @return
                 */
                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<T>();
                }

                /**
                 * Return the system native type-id for the templatized type of ScalarAttribute
                 * @return
                 */
                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<T>();
                }

                /**
                 * Bind the HDF5 data to an attribute in a Hash::Node
                 * @param node to bind to
                 * @return the bound node
                 */
                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, std::vector<T>());
                    }
                    karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                    std::vector<T>& value = attrNode.getValue<std::vector<T> >();
                    value.resize(dims().size());
                    m_attributeData = &value[0];
                    return attrNode;
                }

                /**
                 * Has empty expected parameter section
                 * @param expected
                 */
                static void expectedParameters(karabo::util::Schema& expected) {}

               protected:
                /**
                 * Write value of an Element into the HDF5 attribute
                 * @param attributeNode Element holding the value
                 * @param attribute note evaluate in this context
                 */
                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();

                        auto& content = (node.getValue<std::vector<T> >());

                        if (!content.empty()) {
                            KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, content.data()));
                        }

                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset /"));
                    }
                }

                /**
                 * Read value from HDF5 into internal represenation
                 * @param attributeNode not evaluated
                 * @param attribute note evaluated
                 */
                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entering readNodeAttribute function";
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, m_attributeData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }

               private:
                T* m_attributeData;
            };

            /**
             * @class FixedLengthArrayAttribute
             * @brief A class to represent vector attributes of fixed length in HDF5 -specialization for booleans
             */
            template <>
            class FixedLengthArrayAttribute<bool> : public Attribute {
               public:
                KARABO_CLASSINFO(FixedLengthArrayAttribute,
                                 "VECTOR_" +
                                       karabo::util::ToType<karabo::util::ToLiteral>::to(
                                             karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(bool))),
                                 "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input)
                    : Attribute(input, this), m_attributeData(0) {}

                virtual ~FixedLengthArrayAttribute() {}

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<bool>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<bool>();
                }

                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, std::vector<bool>());
                    }
                    karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                    m_attributeData = &attrNode.getValue<std::vector<bool> >();
                    m_attributeData->resize(dims().size());
                    return attrNode;
                }

                static void expectedParameters(karabo::util::Schema& expected) {}

               protected:
                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        const std::vector<bool>& vec = node.getValue<std::vector<bool> >();
                        hsize_t len = vec.size();
                        if (len > 0) {
                            std::vector<unsigned char> converted(len, 0);
                            for (size_t i = 0; i < len; ++i) {
                                converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                            }
                            const unsigned char* ptr = &converted[0];
                            KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, ptr));
                        }
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset /"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entering readNodeAttribute function";
                    hid_t tid = getNativeTypeId();
                    hsize_t len = m_attributeData->size();
                    std::vector<unsigned char> vec(len, 0);
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, &vec[0]));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    for (size_t i = 0; i < len; ++i) {
                        (*m_attributeData)[i] = boost::numeric_cast<bool>(vec[i]);
                    }
                }

               private:
                std::vector<bool>* m_attributeData;
            };

            /**
             * @class FixedLengthArrayAttribute
             * @brief A class to represent vector attributes of fixed length in HDF5 - specialization for strings
             */
            template <>
            class FixedLengthArrayAttribute<std::string> : public Attribute {
               public:
                KARABO_CLASSINFO(
                      FixedLengthArrayAttribute,
                      "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(
                                        karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(std::string))),
                      "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input)
                    : Attribute(input, this), m_attributeData(0) {}

                virtual ~FixedLengthArrayAttribute() {}

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<std::string>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<std::string>();
                }

                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, std::vector<std::string>());
                    }
                    karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                    m_attributeData = &attrNode.getValue<std::vector<std::string> >();
                    m_attributeData->resize(dims().size());
                    return attrNode;
                }

                static void expectedParameters(karabo::util::Schema& expected) {}

               protected:
                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        const std::vector<std::string>& value = node.getValue<std::vector<std::string> >();
                        if (!value.empty()) {
                            std::vector<const char*> value_copy(value.size(), NULL);
                            for (size_t i = 0; i < value.size(); ++i) {
                                value_copy[i] = value[i].c_str();
                            }
                            KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &value_copy[0]));
                        }

                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset /"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - string attribute";
                        hid_t space = H5Aget_space(m_attribute);
                        KARABO_CHECK_HDF5_STATUS(space);
                        hsize_t dims[1];
                        /*int ndims = NOT USED */ H5Sget_simple_extent_dims(space, dims, NULL);
                        char** rdata = (char**)malloc(dims[0] * sizeof(char*));

                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, rdata));
                        for (size_t i = 0; i < dims[0]; ++i) {
                            (*m_attributeData)[i] = rdata[i];
                        }
                        KARABO_CHECK_HDF5_STATUS(H5Dvlen_reclaim(tid, space, H5P_DEFAULT, rdata));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                        KARABO_CHECK_HDF5_STATUS(H5Sclose(space));
                        free(rdata);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset /"));
                    }
                }

               private:
                std::vector<std::string>* m_attributeData;
            };

            // typedefs
            typedef FixedLengthArrayAttribute<char> CharArrayAttribute;
            typedef FixedLengthArrayAttribute<signed char> Int8ArrayAttribute;
            typedef FixedLengthArrayAttribute<short> Int16ArrayAttribute;
            typedef FixedLengthArrayAttribute<int> Int32ArrayAttribute;
            typedef FixedLengthArrayAttribute<long long> Int64ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned char> UInt8ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned short> UInt16ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned int> UInt32ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned long long> UInt64ArrayAttribute;
            typedef FixedLengthArrayAttribute<double> DoubleArrayAttribute;
            typedef FixedLengthArrayAttribute<float> FloatArrayAttribute;
            typedef FixedLengthArrayAttribute<std::string> StringArrayAttribute;
            typedef FixedLengthArrayAttribute<bool> BoolArrayAttribute;

        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
