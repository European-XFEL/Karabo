/*
 * $Id: DataTypesScalar.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_IO_HDF5_DATATYPESSCALAR_HH
#define	EXFEL_IO_HDF5_DATATYPESSCALAR_HH

#include "DataTypes.hh"
#include "FixedLengthArray.hh"
#include "TypeTraits.hh"
#include <boost/any.hpp>

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package io
     */
    namespace io {

        namespace hdf5 {

            template<typename T>
            class DataTypesScalar : public DataTypes {
            public:

                EXFEL_CLASSINFO(DataTypesScalar, typeid (T).name(), "1.0")

                DataTypesScalar() {
                }

                virtual ~DataTypesScalar() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    return ArrayDimensions(0);
                }

                std::string getElementClassId() {
                    return ScalarTypeTraits::classId<T > ();
                }

            };

            template class DataTypesScalar<signed char>;
            template class DataTypesScalar<short>;
            template class DataTypesScalar<int>;
            template class DataTypesScalar<long long>;
            template class DataTypesScalar<unsigned char>;
            template class DataTypesScalar<unsigned short>;
            template class DataTypesScalar<unsigned int>;
            template class DataTypesScalar<unsigned long long>;
            template class DataTypesScalar<float>;
            template class DataTypesScalar<double>;
            template class DataTypesScalar<std::string>;
            template class DataTypesScalar<bool>;

            typedef DataTypesScalar<signed char> Int8ScalarDataTypes;
            typedef DataTypesScalar<short> Int16ScalarDataTypes;
            typedef DataTypesScalar<int> Int32ScalarDataTypes;
            typedef DataTypesScalar<long long> Int64ScalarDataTypes;
            typedef DataTypesScalar<unsigned char> UInt8ScalarDataTypes;
            typedef DataTypesScalar<unsigned short> UInt16ScalarDataTypes;
            typedef DataTypesScalar<unsigned int> UInt32ScalarDataTypes;
            typedef DataTypesScalar<unsigned long long> UInt64ScalarDataTypes;
            typedef DataTypesScalar<float> FloatScalarDataTypes;
            typedef DataTypesScalar<double> DoubleScalarDataTypes;
            typedef DataTypesScalar<std::string> StringScalarDataTypes;
            typedef DataTypesScalar<bool> BoolScalarDataTypes;

        }
    }
}

#endif	
