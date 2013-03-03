/*
 * $Id: Scalar.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_DATASETREADER_HH
#define	KARABO_IO_H5_DATASETREADER_HH

#include <string>

#include "Dataset.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>

//#include "ScalarFilter.hh"


//#include <karabo/util/Time.hh>
#include "ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {


            template< typename T>
            class ScalarReader {
            public:

                static void read(T& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
                    H5Dread(dataSet, ScalarTypes::getHdf5NativeType<T > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, &value);
                }
            };

            template<>
            class ScalarReader<std::string> {
            public:

                static void read(std::string& value, hid_t dataSet, hid_t memoryDataSpace, hid_t fileDataSpace) {
//                    value.c_str(
//                    H5Dread(dataSet, ScalarTypes::getHdf5NativeType<std::string > (), memoryDataSpace, fileDataSpace, H5P_DEFAULT, ));
                }
            };



        }
    }
}

#endif	
