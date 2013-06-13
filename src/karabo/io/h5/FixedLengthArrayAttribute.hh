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
                    //                    if (!node.hasAttribute(m_key)) {
                    //                        node.setAttribute(m_key, T());
                    //                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                    //                        T & value = attrNode.getValue<T >();
                    //                        m_attributeData = &value;
                    //                        return attrNode;
                    //                    } else {                        
                    //                        karabo::util::Element<std::string>& attrNode = node.getAttributes().getNode(m_key);
                    //                        T & value = attrNode.getValue<T >();
                    //                        m_attributeData = &value;
                    //                        return attrNode;
                    //                    }                    
                }

                static void expectedParameters(karabo::util::Schema& expected) {

                }

            protected:

                void writeNodeAttribute(const karabo::util::Element<std::string>& node, hid_t attribute) {
                    try {
                        write<T>(node, attribute);
                    } catch (...) {
                        KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write attributes for node " + this->m_key + " to dataset /"));
                    }
                }

                void readNodeAttribute(karabo::util::Element<std::string>& attributeNode, hid_t attribute) {
                    KARABO_LOG_FRAMEWORK_TRACE_CF << "entering empty function";
                    //read<T>(attributeNode, attribute);
                }

            private:

                template<typename U>
                inline void write(const karabo::util::Element<std::string>& node, hid_t attribute);

            };

            template<class T>
            template<typename U>
            inline void FixedLengthArrayAttribute<T>::write(const karabo::util::Element<std::string>& node, hid_t attribute) {

                KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, getNativeTypeId(), &(node.getValue<std::vector<U> >())[0]));
            }

            template<> template<>
            inline void FixedLengthArrayAttribute<bool>::write<bool>(const karabo::util::Element<std::string>& node, hid_t attribute) {

                const std::vector<bool>& vec = node.getValue < std::vector<bool> > ();
                hsize_t len = vec.size();
                std::vector<unsigned char> converted(len, 0);
                for (size_t i = 0; i < len; ++i) {
                    converted[i] = boost::numeric_cast<unsigned char>(vec[i]);
                }
                const unsigned char* ptr = &converted[0];
                KARABO_CHECK_HDF5_STATUS(H5Awrite(m_attribute, getNativeTypeId(), ptr));
            }

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
