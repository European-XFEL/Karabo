/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#include "ScalarFilterBuffer.hh"


using namespace std;
using namespace karabo::util;

namespace karabo {
    namespace io {
	namespace hdf5 {

	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<signed char>, Int8ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<short>, Int16ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<int>, Int32ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<long long>, Int64ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned char>, UInt8ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned short>, UInt16ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned int>, UInt32ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned long long>, UInt64ArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<float>, FloatArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<double>, DoubleArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<std::string>, StringArrayViewScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<bool>, BoolArrayViewScalarFilter)


	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<signed char>, Int8VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<short>, Int16VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<int>, Int32VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<long long>, Int64VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned char>, UInt8VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned short>, UInt16VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned int>, UInt32VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<unsigned long long>, UInt64VectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<float>, FloatVectorScalarFilter)
	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<double>, DoubleVectorScalarFilter)
//	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<std::string>, StringVectorScalarFilter)
//	    KARABO_REGISTER_FACTORY_CC(ScalarFilter<bool>, BoolVectorScalarFilter)



	   

	    //      KARABO_REGISTER_FACTORY_CC(InputData<signed char>, Int8VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<short>, Int16VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<int>, Int32VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<long long>, Int64VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned char>, UInt8VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned short>, UInt16VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned int>, UInt32VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned long long>, UInt64VectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<float>, FloatVectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<double>, DoubleVectorInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<std::string>, StringVectorInputData)
	    //      // std::vector<bool> is not supported

	    //      KARABO_REGISTER_FACTORY_CC(InputData<float>, FloatDoubleVectorConverter)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<float>, FloatInt32VectorConverter)
	    //
	    //
	    //      KARABO_REGISTER_FACTORY_CC(InputData<signed char>, Int8DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<short>, Int16DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<int>, Int32DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<long long>, Int64DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned char>, UInt8DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned short>, UInt16DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned int>, UInt32DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<unsigned long long>, UInt64DequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<float>, FloatDequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<double>, DoubleDequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<std::string>, StringDequeInputData)
	    //      KARABO_REGISTER_FACTORY_CC(InputData<bool>, BoolDequeInputData)







	}
    }
}
