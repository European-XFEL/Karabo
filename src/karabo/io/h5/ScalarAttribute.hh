/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALARATTRIBUTE_HH
#define	KARABO_IO_H5_SCALARATTRIBUTE_HH

#include <string>


#include "TypeTraits.hh"
#include "Attribute.hh"
#include <karabo/util/Configurator.hh>
#include <karabo/log/Logger.hh>

namespace karabo {

    namespace io {

        namespace h5 {

            template<class T>
            class ScalarAttribute : public karabo::io::h5::Attribute {

                public:

                KARABO_CLASSINFO(ScalarAttribute, karabo::util::ToType<karabo::util::ToLiteral>::
                                 to(
                                    karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0"
                                 )


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {

                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {
                }

                static hid_t m_dspace;

                static hid_t initDataSpace() {
                    hsize_t ex[] = {1};
                    return H5Screate_simple(1, ex, NULL);
                }

                hid_t createDataspace(const std::vector<hsize_t>& ex, const std::vector<hsize_t>& maxEx) {
                    //                    std::clog << "createDataspace: " << m_dspace << std::endl;
                    return this->m_dspace;
                }

                virtual void closeDataspace(hid_t dataSpace) {
                }

                static hid_t getStandardTypeId() {
                    return ScalarTypes::getHdf5StandardType<T>();
                }

                static hid_t getNativeTypeId() {
                    return ScalarTypes::getHdf5NativeType<T>();
                }

                karabo::util::Element<std::string>& bindAttribute(karabo::util::Hash::Node& node) {
                    if (!node.hasAttribute(m_key)) {
                        node.setAttribute(m_key, T());
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        T & value = attrNode.getValue<T >();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        T & value = attrNode.getValue<T >();
                        m_attributeData = &value;
                        return attrNode;
                    }
                }



            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &(attributeNode.getValue<T>())));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - template method";
                    hid_t tid = getNativeTypeId();
                    KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, tid, m_attributeData));
                    KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                }


            private:
                T* m_attributeData;
            };

            template<>
            class ScalarAttribute<std::string> : public karabo::io::h5::Attribute {

                public:

                KARABO_CLASSINFO(ScalarAttribute, karabo::util::ToType<karabo::util::ToLiteral>::
                                 to(
                                    karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (std::string))), "1.0"
                                 )


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {
                }

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
                        std::string & value = attrNode.getValue<std::string >();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        std::string& value = attrNode.getValue<std::string >();
                        m_attributeData = &value;
                        return attrNode;
                    }
                }



            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    try {
                        hid_t tid = getNativeTypeId();
                        KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, tid, &(attributeNode.getValue<std::string>())));
                        KARABO_CHECK_HDF5_STATUS(H5Tclose(tid));
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset"));
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

            template<>
            class ScalarAttribute<bool> : public karabo::io::h5::Attribute {

                public:

                KARABO_CLASSINFO(ScalarAttribute, karabo::util::ToType<karabo::util::ToLiteral>::
                                 to(
                                    karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (bool))), "1.0"
                                 )


                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input, this) {
                }

                static const karabo::util::Dims getSingleValueDimensions() {
                    return karabo::util::Dims();
                }

                virtual ~ScalarAttribute() {
                }

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
                        bool & value = attrNode.getValue<bool>();
                        m_attributeData = &value;
                        return attrNode;
                    } else {
                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                        bool& value = attrNode.getValue<bool >();
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
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset"));
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




            template<class T>
            hid_t ScalarAttribute<T>::m_dspace = ScalarAttribute<T>::initDataSpace();

            // typedefs
            typedef ScalarAttribute<char> CharAttribute;
            typedef ScalarAttribute<signed char> Int8Attribute;
            typedef ScalarAttribute<short> Int16Attribute;
            typedef ScalarAttribute<int> Int32Attribute;
            typedef ScalarAttribute<long long > Int64Attribute;
            typedef ScalarAttribute<unsigned char> UInt8Attribute;
            typedef ScalarAttribute<unsigned short> UInt16Attribute;
            typedef ScalarAttribute<unsigned int> UInt32Attribute;
            typedef ScalarAttribute<unsigned long long > UInt64Attribute;
            typedef ScalarAttribute<double> DoubleAttribute;
            typedef ScalarAttribute<float> FloatAttribute;
            typedef ScalarAttribute<std::string> StringAttribute;
            typedef ScalarAttribute<bool> BoolAttribute;


        }
    }
}

#endif
