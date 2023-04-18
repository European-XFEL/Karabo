/*
 * $Id: FixedLengthArray.cc 9370 2013-04-17 05:54:29Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */

#include "DatasetWriter.hh"

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<char>, karabo::io::h5::DatasetVectorWriter<char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<signed char>,
                                  karabo::io::h5::DatasetVectorWriter<signed char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<short>, karabo::io::h5::DatasetVectorWriter<short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<int>, karabo::io::h5::DatasetVectorWriter<int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<long long>,
                                  karabo::io::h5::DatasetVectorWriter<long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned char>,
                                  karabo::io::h5::DatasetVectorWriter<unsigned char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned short>,
                                  karabo::io::h5::DatasetVectorWriter<unsigned short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned int>,
                                  karabo::io::h5::DatasetVectorWriter<unsigned int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned long long>,
                                  karabo::io::h5::DatasetVectorWriter<unsigned long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<double>, karabo::io::h5::DatasetVectorWriter<double>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<float>, karabo::io::h5::DatasetVectorWriter<float>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::string>,
                                  karabo::io::h5::DatasetVectorWriter<std::string>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<bool>, karabo::io::h5::DatasetVectorWriter<bool>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<float> >,
                                  karabo::io::h5::DatasetVectorWriter<std::complex<float> >)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<double> >,
                                  karabo::io::h5::DatasetVectorWriter<std::complex<double> >)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<char>, karabo::io::h5::DatasetPointerWriter<char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<signed char>,
                                  karabo::io::h5::DatasetPointerWriter<signed char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<short>, karabo::io::h5::DatasetPointerWriter<short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<int>, karabo::io::h5::DatasetPointerWriter<int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<long long>,
                                  karabo::io::h5::DatasetPointerWriter<long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned char>,
                                  karabo::io::h5::DatasetPointerWriter<unsigned char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned short>,
                                  karabo::io::h5::DatasetPointerWriter<unsigned short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned int>,
                                  karabo::io::h5::DatasetPointerWriter<unsigned int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned long long>,
                                  karabo::io::h5::DatasetPointerWriter<unsigned long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<double>, karabo::io::h5::DatasetPointerWriter<double>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<float>, karabo::io::h5::DatasetPointerWriter<float>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::string>,
                                  karabo::io::h5::DatasetPointerWriter<std::string>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<bool>, karabo::io::h5::DatasetPointerWriter<bool>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<float> >,
                                  karabo::io::h5::DatasetPointerWriter<std::complex<float> >)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<double> >,
                                  karabo::io::h5::DatasetPointerWriter<std::complex<double> >)

KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<char>, karabo::io::h5::DatasetNDArrayH5Writer<char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<signed char>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<signed char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<short>, karabo::io::h5::DatasetNDArrayH5Writer<short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<int>, karabo::io::h5::DatasetNDArrayH5Writer<int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<long long>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned char>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<unsigned char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned short>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<unsigned short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned int>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<unsigned int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned long long>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<unsigned long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<double>, karabo::io::h5::DatasetNDArrayH5Writer<double>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<float>, karabo::io::h5::DatasetNDArrayH5Writer<float>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::string>,
                                  karabo::io::h5::DatasetNDArrayH5Writer<std::string>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<bool>, karabo::io::h5::DatasetNDArrayH5Writer<bool>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<float> >,
                                  karabo::io::h5::DatasetNDArrayH5Writer<std::complex<float> >)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<double> >,
                                  karabo::io::h5::DatasetNDArrayH5Writer<std::complex<double> >)


KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<char>, karabo::io::h5::DatasetScalarWriter<char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<signed char>,
                                  karabo::io::h5::DatasetScalarWriter<signed char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<short>, karabo::io::h5::DatasetScalarWriter<short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<int>, karabo::io::h5::DatasetScalarWriter<int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<long long>,
                                  karabo::io::h5::DatasetScalarWriter<long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned char>,
                                  karabo::io::h5::DatasetScalarWriter<unsigned char>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned short>,
                                  karabo::io::h5::DatasetScalarWriter<unsigned short>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned int>,
                                  karabo::io::h5::DatasetScalarWriter<unsigned int>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<unsigned long long>,
                                  karabo::io::h5::DatasetScalarWriter<unsigned long long>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<double>, karabo::io::h5::DatasetScalarWriter<double>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<float>, karabo::io::h5::DatasetScalarWriter<float>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::string>,
                                  karabo::io::h5::DatasetScalarWriter<std::string>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<bool>, karabo::io::h5::DatasetScalarWriter<bool>)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<float> >,
                                  karabo::io::h5::DatasetScalarWriter<std::complex<float> >)
KARABO_REGISTER_FOR_CONFIGURATION(karabo::io::h5::DatasetWriter<std::complex<double> >,
                                  karabo::io::h5::DatasetScalarWriter<std::complex<double> >)
