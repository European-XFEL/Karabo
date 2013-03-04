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

#include <karabo/util/util.hh>
//#include "../ioProfiler.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T>
            class FixedLengthArrayAttribute : public Attribute {
            public:

                KARABO_CLASSINFO(FixedLengthArrayAttribute, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "2.0")          

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input) {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));
                }

                virtual ~FixedLengthArrayAttribute() {
                }

                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentOptional().noDefaultValue()
                            .init()
                            .commit();
                }

                void create(hsize_t chunkSize) {
                    
                }

 
                void write(const karabo::util::Hash& data, hsize_t recordId) {
 
                }
                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                 }
 

            protected:

                hsize_t m_size; // size of the array is fixed
                karabo::util::Dims m_dims;
                hid_t m_dataAccessPropListId;


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
