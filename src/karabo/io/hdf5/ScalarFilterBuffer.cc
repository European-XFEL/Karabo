/*
 * $Id: ScalarFilterBuffer.cc 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ScalarFilterBuffer.hh"


using namespace std;
using namespace exfel::util;

namespace exfel {
    namespace io {
	namespace hdf5 {

	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<signed char>, Int8ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<short>, Int16ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<int>, Int32ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<long long>, Int64ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned char>, UInt8ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned short>, UInt16ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned int>, UInt32ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned long long>, UInt64ArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<float>, FloatArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<double>, DoubleArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<std::string>, StringArrayViewScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<bool>, BoolArrayViewScalarFilter)


	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<signed char>, Int8VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<short>, Int16VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<int>, Int32VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<long long>, Int64VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned char>, UInt8VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned short>, UInt16VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned int>, UInt32VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<unsigned long long>, UInt64VectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<float>, FloatVectorScalarFilter)
	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<double>, DoubleVectorScalarFilter)
//	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<std::string>, StringVectorScalarFilter)
//	    EXFEL_REGISTER_FACTORY_CC(ScalarFilter<bool>, BoolVectorScalarFilter)



	   

	    //      EXFEL_REGISTER_FACTORY_CC(InputData<signed char>, Int8VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<short>, Int16VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<int>, Int32VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<long long>, Int64VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned char>, UInt8VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned short>, UInt16VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned int>, UInt32VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned long long>, UInt64VectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<float>, FloatVectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<double>, DoubleVectorInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<std::string>, StringVectorInputData)
	    //      // std::vector<bool> is not supported

	    //      EXFEL_REGISTER_FACTORY_CC(InputData<float>, FloatDoubleVectorConverter)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<float>, FloatInt32VectorConverter)
	    //
	    //
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<signed char>, Int8DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<short>, Int16DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<int>, Int32DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<long long>, Int64DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned char>, UInt8DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned short>, UInt16DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned int>, UInt32DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<unsigned long long>, UInt64DequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<float>, FloatDequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<double>, DoubleDequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<std::string>, StringDequeInputData)
	    //      EXFEL_REGISTER_FACTORY_CC(InputData<bool>, BoolDequeInputData)







	}
    }
}
