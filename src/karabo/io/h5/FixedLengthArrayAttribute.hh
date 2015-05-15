/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH
#define	KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH


#include <vector>
#include <string>

#include "Attribute.hh"
#include "TypeTraits.hh"
#include "FixedLengthArray.hh"
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/VectorElement.hh>
#include <karabo/util/Dims.hh>
#include <karabo/util/Configurator.hh>
#include <karabo/log/Logger.hh>


namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T>
            class FixedLengthArrayAttribute : public Attribute {

            public:

                KARABO_CLASSINFO(FixedLengthArrayAttribute, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input, this) {

                }

                virtual ~FixedLengthArrayAttribute() {
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<T>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<T>();
                }

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

                static void expectedParameters(karabo::util::Schema& expected) {

                }

            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &(node.getValue<std::vector<T> >())[0]));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset /"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entering readNodeAttribute function";
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, m_attributeData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));

                }

            private:
                T* m_attributeData;
            };

            template<>
            class FixedLengthArrayAttribute<bool> : public Attribute {

            public:

                KARABO_CLASSINFO(FixedLengthArrayAttribute, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool))), "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input, this),
                m_attributeData(0) {
                }

                virtual ~FixedLengthArrayAttribute() {
                }

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
                    m_attributeData = &attrNode.getValue < std::vector<bool> >();
                    m_attributeData->resize(dims().size());
                    return attrNode;
                }

                static void expectedParameters(karabo::util::Schema& expected) {
                }

            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        const std::vector<bool>& vec = node.getValue < std::vector<bool> >();
                        hsize_t len = vec.size();
                        std::vector<unsigned char> converted(len, 0);
                        for (size_t i = 0; i < len; ++i) {
                            converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                        }
                        const unsigned char* ptr = &converted[0];
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, ptr));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset /"));
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

            template<>
            class FixedLengthArrayAttribute<std::string> : public Attribute {

            public:

                KARABO_CLASSINFO(FixedLengthArrayAttribute, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::string))), "2.0")

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input, this),
                m_attributeData(0) {
                }

                virtual ~FixedLengthArrayAttribute() {
                }

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
                    m_attributeData = &attrNode.getValue < std::vector<std::string> >();
                    m_attributeData->resize(dims().size());
                    return attrNode;
                }

                static void expectedParameters(karabo::util::Schema& expected) {
                }

            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &(node.getValue<std::vector<std::string> >())[0]));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset /"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {

                    try {
                        KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - string attribute";
                        hid_t space = H5Aget_space(m_attribute);
                        KARABO_CHECK_HDF5_STATUS(space);
                        hsize_t dims[1];
                        /*int ndims = NOT USED */ H5Sget_simple_extent_dims(space, dims, NULL);                        
                        char** rdata = (char **) malloc(dims[0] * sizeof (char *));

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
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset /"));
                    }

                }

            private:
                std::vector<std::string>* m_attributeData;

            };

            // typedefs
            typedef FixedLengthArrayAttribute<signed char> Int8ArrayAttribute;
            typedef FixedLengthArrayAttribute<short> Int16ArrayAttribute;
            typedef FixedLengthArrayAttribute<int> Int32ArrayAttribute;
            typedef FixedLengthArrayAttribute<long long > Int64ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned char> UInt8ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned short> UInt16ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned int> UInt32ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned long long > UInt64ArrayAttribute;
            typedef FixedLengthArrayAttribute<double> DoubleArrayAttribute;
            typedef FixedLengthArrayAttribute<float> FloatArrayAttribute;
            typedef FixedLengthArrayAttribute<std::string> StringArrayAttribute;
            typedef FixedLengthArrayAttribute<bool> BoolArrayAttribute;

        }
    }
}

#endif	
