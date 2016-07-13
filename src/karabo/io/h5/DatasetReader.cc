/*
 * $Id: FixedLengthArray.cc 9370 2013-04-17 05:54:29Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DatasetReader.hh"
using namespace karabo::io;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<bool>)

            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader< std::complex<double> >)

        }
    }
}
