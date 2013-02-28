/*
 * $Id: ScalarAttribute.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_SCALARATTRIBUTE_HH
#define	KARABO_IO_H5_SCALARATTRIBUTE_HH

#include <string>

#include "Dataset.hh"
#include "TypeTraits.hh"
#include "Attribute.hh"
#include <karabo/util/util.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>

//#include "ScalarFilter.hh"


//#include <karabo/util/Time.hh>
//#include "../ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {


 
            template<class T>
            class ScalarAttribute : public karabo::io::h5::Attribute {
            public:

                KARABO_CLASSINFO(ScalarAttribute, karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "1.0")



                ScalarAttribute(const karabo::util::Hash& input) : Attribute(input) {

                }

                virtual ~ScalarAttribute() {
                }

                void create(hsize_t chunkSize) {
//                    try {
//                        m_chunkSize = chunkSize;
//                        createDataSetProperties(chunkSize);
//                        m_fileDataSpace = Dataset::dataSpace(0);
//                        m_dataSet = m_group->createDataSet(m_key.c_str(), ScalarTypes::getHdf5StandardType<T > (), m_fileDataSpace, *m_dataSetProperties);
//                    } catch (...) {
//                        KARABO_RETHROW
//                    }

                }


                void write(const karabo::util::Hash& data, hsize_t recordId) {

                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {

                }

      



                




            };

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
