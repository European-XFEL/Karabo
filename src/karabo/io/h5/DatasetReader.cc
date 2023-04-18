/*
 * $Id: FixedLengthArray.cc 9370 2013-04-17 05:54:29Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "DatasetReader.hh"
using namespace karabo::io;

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<signed char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<unsigned char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<unsigned short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<unsigned int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<unsigned long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<double>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<float>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<std::string>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<bool>)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<std::complex<float> >)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetReader<std::complex<double> >)
