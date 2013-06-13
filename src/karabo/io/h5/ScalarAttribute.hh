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
                    try{
                    write<T>(attributeNode, attribute);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    read<T>(attributeNode, attribute);
                }

            private:

                template<class U>
                void write(const karabo::util::Element<std::string>& node, hid_t attribute);

                template<class U>
                void read(karabo::util::Element<std::string>& node, hid_t attribute);



            private:
                T* m_attributeData;
            };

            template<class T>
            template<class U>
            inline void ScalarAttribute<T>::write(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, getNativeTypeId(), &(attributeNode.getValue<U>())));

            }

            template<>
            template<>
            inline void ScalarAttribute<bool>::write<bool>(const karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                bool value = attributeNode.getValue<bool>();
                unsigned char converted = boost::numeric_cast<unsigned char>(value);
                KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, getNativeTypeId(), &converted));

            }

            template<class T>
            template<class U>
            inline void ScalarAttribute<T>::read(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - template method";
                KARABO_CHECK_HDF5_STATUS(H5Aread(m_attribute, getNativeTypeId(), m_attributeData));

            }

            template<>
            template<>
            inline void ScalarAttribute<bool>::read<bool>(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                KARABO_LOG_FRAMEWORK_TRACE_CF << "reading - template specialization for bool";
                unsigned char valueUCh;
                KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, getNativeTypeId(), &valueUCh));
                m_attributeData[0] = boost::numeric_cast<bool>(valueUCh);


            }

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
