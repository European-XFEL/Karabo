/*
 * $Id: FixedLengthArray.cc 9370 2013-04-17 05:54:29Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "DatasetWriter.hh"
using namespace karabo::io;

namespace karabo {
    namespace io {
        namespace h5 {


            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetVectorWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetVectorWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetVectorWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetVectorWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetVectorWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetVectorWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetVectorWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetVectorWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetVectorWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetVectorWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetVectorWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetVectorWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetVectorWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<float> >, DatasetVectorWriter< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<double> >, DatasetVectorWriter< std::complex<double> >)



            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetPointerWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetPointerWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetPointerWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetPointerWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetPointerWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetPointerWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetPointerWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetPointerWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetPointerWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetPointerWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetPointerWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetPointerWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetPointerWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<float> >, DatasetPointerWriter< std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter< std::complex<double> >, DatasetPointerWriter< std::complex<double> >)



            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetArrayWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetArrayWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetArrayWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetArrayWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetArrayWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetArrayWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetArrayWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetArrayWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetArrayWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetArrayWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetArrayWriter<float>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetArrayWriter<bool>)

            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<char>, DatasetScalarWriter<char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<signed char>, DatasetScalarWriter<signed char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<short>, DatasetScalarWriter<short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<int>, DatasetScalarWriter<int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<long long>, DatasetScalarWriter<long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned char>, DatasetScalarWriter<unsigned char>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned short>, DatasetScalarWriter<unsigned short>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned int>, DatasetScalarWriter<unsigned int>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<unsigned long long>, DatasetScalarWriter<unsigned long long>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<double>, DatasetScalarWriter<double>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<float>, DatasetScalarWriter<float>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::string>, DatasetScalarWriter<std::string>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<bool>, DatasetScalarWriter<bool>)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::complex<float> >, DatasetScalarWriter<std::complex<float> >)
            KARABO_REGISTER_FOR_CONFIGURATION(DatasetWriter<std::complex<double> >, DatasetScalarWriter<std::complex<double> >)


        }
    }
}
