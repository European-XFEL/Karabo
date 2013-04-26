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
            
            
            

//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<char>, DatasetVectorReader<char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<signed char>, DatasetVectorReader<signed char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<short>, DatasetVectorReader<short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<int>, DatasetVectorReader<int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<long long>, DatasetVectorReader<long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned char>, DatasetVectorReader<unsigned char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned short>, DatasetVectorReader<unsigned short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned int>, DatasetVectorReader<unsigned int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned long long>, DatasetVectorReader<unsigned long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<double>, DatasetVectorReader<double>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<float>, DatasetVectorReader<float>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<std::string>, DatasetVectorReader<std::string>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<bool>, DatasetVectorReader<bool>)

//
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<char>, DatasetPointerReader<char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<signed char>, DatasetPointerReader<signed char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<short>, DatasetPointerReader<short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<int>, DatasetPointerReader<int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<long long>, DatasetPointerReader<long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned char>, DatasetPointerReader<unsigned char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned short>, DatasetPointerReader<unsigned short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned int>, DatasetPointerReader<unsigned int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned long long>, DatasetPointerReader<unsigned long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<double>, DatasetPointerReader<double>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<float>, DatasetPointerReader<float>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<std::string>, DatasetPointerReader<std::string>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<bool>, DatasetPointerReader<bool>)
//
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<char>, DatasetScalarReader<char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<signed char>, DatasetScalarReader<signed char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<short>, DatasetScalarReader<short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<int>, DatasetScalarReader<int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<long long>, DatasetScalarReader<long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned char>, DatasetScalarReader<unsigned char>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned short>, DatasetScalarReader<unsigned short>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned int>, DatasetScalarReader<unsigned int>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<unsigned long long>, DatasetScalarReader<unsigned long long>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<double>, DatasetScalarReader<double>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<float>, DatasetScalarReader<float>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<std::string>, DatasetScalarReader<std::string>)
//            KARABO_REGISTER_FOR_CONFIGURATION(DatasetReader<bool>, DatasetScalarReader<bool>)

        }
    }
}
