/*
 * $Id: FLArrayFilterRawPointer.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_HDF5_FLARRAYFILTERRAWPOINTER_HH
#define	KARABO_IO_HDF5_FLARRAYFILTERRAWPOINTER_HH

#include "FLArrayFilter.hh"
#include "TypeTraits.hh"
#include "FixedLengthArray.hh"
#include <boost/any.hpp>

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        namespace hdf5 {

            template<typename T>
            class FLArrayFilterRawPointer :
            public FLArrayFilter<T> {
            public:

                KARABO_CLASSINFO(FLArrayFilterRawPointer, typeid (T*).name(), "1.0")

                FLArrayFilterRawPointer() {
                }

                virtual ~FLArrayFilterRawPointer() {
                }

                void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const T* rawPtr = boost::any_cast<T* >(any);
                    element.write(rawPtr);
                }

                void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims) {
                    T* rawPtr = boost::any_cast<T*>(any);
                    element.read(rawPtr);
                }

            };

            typedef FLArrayFilterRawPointer<signed char> Int8RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<short> Int16RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<int> Int32RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<long long > Int64RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<unsigned char> UInt8RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<unsigned short> UInt16RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<unsigned int> UInt32RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<unsigned long long> UInt64RawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<float> FloatRawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<double> DoubleRawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<std::string> StringRawPointerFLArrayFilter;
            typedef FLArrayFilterRawPointer<bool> BoolRawPointerFLArrayFilter;





        }
    }
}

#endif	/* KARABO_IO_HDF5_FLARRAYFILTERRAWPOINTER_HH */
