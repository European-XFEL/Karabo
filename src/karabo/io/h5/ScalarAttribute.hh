/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALARATTRIBUTE_HH
#define KARABO_IO_H5_SCALARATTRIBUTE_HH

#include <karabo/log/Logger.hh>
#include <karabo/util/Configurator.hh>
#include <string>

#include "Attribute.hh"
#include "TypeTraits.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            /**
             * @class ScalarAttribute
             * @brief A class to represent scalar attributes in HDF5
             */
            template <class T>
            class ScalarAttribute : public karabo::io::h5::Attribute {
               public:
                KARABO_CLASSINFO(ScalarAttribute,
                                 karabo::util::ToType<karabo::util::ToLiteral>::to(
                                       karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(T))),
                                 "1.0")


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {}

                /**
                 * Return the dimensions of the attribute, overwrite for consistent interface.
                 * Will return vector of length one with entry 1
                 * @return
                 */
                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {}

                /**
                 The HDF5 dataspace the attribute refers to.
                 */
                static hid_t m_dspace;

                /**
                 * Initialize the HDF5 dataspace with a scalar value of 1
                 * @return
                 */
                static hid_t initDataSpace() {
                    hsize_t ex[] = {1};
                    return H5Screate_simple(1, ex, NULL);
                }

                /**
                 * Return the HDF5 dataspace
                 * @param ex not evaluated
                 * @param maxEx not evaluated
                 * @return
                 */
                hid_t createDataspace(const std::vector<hsize_t>& ex, const std::vector<hsize_t>& maxEx) {
                    return this->m_dspace;
                }

                /**
                 * A noop for scalar attributes
                 * @param dataSpace
                 */
                virtual void closeDataspace(hid_t dataSpace) {}

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
                        node.setAttribute(m_key, T());
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        T& value = attrNode.getValue<T>();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        T& value = attrNode.getValue<T>();
                        m_attributeData = &value;
                        return attrNode;
                    }
                }


               protected:
                /**
                 * Write value of an Element into the HDF5 attribute
                 * @param attributeNode Element holding the value
                 * @param attribute note evaluate in this context
                 */
                void writeNodeAttribute(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &(attributeNode.getValue<T>())));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset"));
                    }
                }

                /**
                 * Read value from HDF5 into internal represenation
                 * @param attributeNode not evaluated
                 * @param attribute note evaluated
                 */
                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - template method";
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, m_attributeData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }


               private:
                T* m_attributeData;
            };

            /**
             * @class ScalarAttribute
             * @brief A class to represent scalar attributes in HDF5 -specialization for strings
             */
            template <>
            class ScalarAttribute<std::string> : public karabo::io::h5::Attribute {
               public:
                KARABO_CLASSINFO(ScalarAttribute,
                                 karabo::util::ToType<karabo::util::ToLiteral>::to(
                                       karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(std::string))),
                                 "1.0")


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {}

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {}

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<std::string>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<std::string>();
                }

                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, std::string());
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        std::string& value = attrNode.getValue<std::string>();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        std::string& value = attrNode.getValue<std::string>();
                        m_attributeData = &value;
                        return attrNode;
                    }
                }


               protected:
                void writeNodeAttribute(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        const std::string& value = attributeNode.getValue<std::string>();
                        const char* converted = value.c_str();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &converted));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - string attribute";
                    hid_t space = H5Aget_space(m_attribute);
                    KARABO_CHECK_HDF5_STATUS(space);
                    char* rdata[1];
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, &rdata));
                    *m_attributeData = rdata[0];

                    KARABO_CHECK_HDF5_STATUS(H5Dvlen_reclaim(tid, space, H5P_DEFAULT, &rdata));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(space));
                }

               private:
                std::string* m_attributeData;
            };

            /**
             * @class ScalarAttribute
             * @brief A class to represent scalar attributes in HDF5 -specialization for booleans
             */
            template <>
            class ScalarAttribute<bool> : public karabo::io::h5::Attribute {
               public:
                KARABO_CLASSINFO(ScalarAttribute,
                                 karabo::util::ToType<karabo::util::ToLiteral>::to(
                                       karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid(bool))),
                                 "1.0")


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {}

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {}

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<bool>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<bool>();
                }

                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, bool());
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        bool& value = attrNode.getValue<bool>();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        bool& value = attrNode.getValue<bool>();
                        m_attributeData = &value;
                        return attrNode;
                    }
                }


               protected:
                void writeNodeAttribute(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        bool value = attributeNode.getValue<bool>();
                        unsigned char converted = boost::numeric_cast<unsigned char>(value);
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &converted));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " +
                                                                      this->m_key + " to dataset"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - string attribute";
                    hid_t space = H5Aget_space(m_attribute);
                    KARABO_CHECK_HDF5_STATUS(space);
                    unsigned char rdata;
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, &rdata));
                    *m_attributeData = boost::numeric_cast<bool>(rdata);
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    KARABO_CHECK_HDF5_STATUS(H5Sclose(space));
                }

               private:
                bool* m_attributeData;
            };


            template <class T>
            hid_t ScalarAttribute<T>::m_dspace = ScalarAttribute<T>::initDataSpace();

            // typedefs
            typedef ScalarAttribute<char> CharAttribute;
            typedef ScalarAttribute<signed char> Int8Attribute;
            typedef ScalarAttribute<short> Int16Attribute;
            typedef ScalarAttribute<int> Int32Attribute;
            typedef ScalarAttribute<long long> Int64Attribute;
            typedef ScalarAttribute<unsigned char> UInt8Attribute;
            typedef ScalarAttribute<unsigned short> UInt16Attribute;
            typedef ScalarAttribute<unsigned int> UInt32Attribute;
            typedef ScalarAttribute<unsigned long long> UInt64Attribute;
            typedef ScalarAttribute<double> DoubleAttribute;
            typedef ScalarAttribute<float> FloatAttribute;
            typedef ScalarAttribute<std::string> StringAttribute;
            typedef ScalarAttribute<bool> BoolAttribute;


        } // namespace h5
    }     // namespace io
} // namespace karabo

#endif
