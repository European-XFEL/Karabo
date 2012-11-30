/*
 * $Id: FLArrayFilterArrayView.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_HDF5_FLARRAYFILTERARRAYVIEW_HH
#define	KARABO_IO_HDF5_FLARRAYFILTERARRAYVIEW_HH

#include "FLArrayFilter.hh"
#include "DataTypes.hh"
#include "FixedLengthArray.hh"
#include "TypeTraits.hh"
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


            // Here we pre-define filters for writing and reading Array types
            // and discovering Array properties (DataTypes)

            template<typename T, template <typename ELEM> class CONT = karabo::io::ArrayView >
            class FLArrayFilterArrayView :
            public FLArrayFilter<T>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayView, typeid (CONT<T>).name(), "1.0")

                FLArrayFilterArrayView() {
                }

                virtual ~FLArrayFilterArrayView() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const CONT<T>& arr = boost::any_cast<CONT<T> > (any);
                    return arr.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<T > ();
                }

                void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const CONT<T>& av = *(boost::any_cast<CONT<T> >(&any));
                    const T* rawPtr = &av[0];
                    element.write(rawPtr);
                }

                void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims) {
                    CONT<T>& av = *(boost::any_cast<CONT<T> >(&any));
                    T* rawPtr = &av[0];
                    element.read(rawPtr);
                }

            };

            template<>
            class FLArrayFilterArrayView<std::string, karabo::io::ArrayView> :
            public FLArrayFilter<std::string>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayView, typeid (karabo::io::ArrayView<std::string>).name(), "1.0")

                FLArrayFilterArrayView() {
                }

                virtual ~FLArrayFilterArrayView() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const karabo::io::ArrayView<std::string>& arr = boost::any_cast<karabo::io::ArrayView<std::string> > (any);
                    return arr.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<std::string > ();
                }

                void write(const FixedLengthArray<std::string>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const karabo::io::ArrayView<std::string>& av = *(boost::any_cast<karabo::io::ArrayView<std::string> >(&any));
                    const std::string* strPtr = &av[0];
                    element.write(strPtr);
                }

                void read(const FixedLengthArray<std::string>& element, boost::any& any, ArrayDimensions& dims) {
                    karabo::io::ArrayView<std::string>& av = *(boost::any_cast<karabo::io::ArrayView<std::string> >(&any));
                    std::string* str = &av[0];

                    // calculate number of strings to be read.
                    // Note 1: remember that container may be multi-dimensional
                    // Note 2: ArrayView also contains this information but we use function argument to be consistent 
                    //         with other data container as they may not provide this feature

                    unsigned long long totalNumberOfStrings = dims.getNumberOfElements();

                    // declare temporary array of char pointers
                    boost::shared_array<char*> arrChar = boost::shared_array<char*>(new char*[totalNumberOfStrings]);
                    // here we need to use second variant of read method as reading directly 
                    // to the memory provided by strings cannot be utilized
                    element.read(arrChar.get(), std::string());

                    // after reading from hdf5 file we need to copy the full array of strings
                    // to the original place
                    for (size_t i = 0; i < totalNumberOfStrings; i++) {
                        str[i] = arrChar[i];
                    }
                }

            };

            template<>
            class FLArrayFilterArrayView<bool, karabo::io::ArrayView> :
            public FLArrayFilter<bool>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayView, typeid (karabo::io::ArrayView<bool>).name(), "1.0")

                FLArrayFilterArrayView() {
                }

                virtual ~FLArrayFilterArrayView() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const karabo::io::ArrayView<bool>& arr = boost::any_cast < karabo::io::ArrayView<bool> > (any);
                    return arr.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<bool > ();
                }

                void write(const FixedLengthArray<bool>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const karabo::io::ArrayView<bool>& av = *(boost::any_cast < karabo::io::ArrayView<bool> >(&any));
                    const bool* boolPtr = &av[0];
                    unsigned long long totalSize = dims.getNumberOfElements();
                    boost::shared_array<unsigned char> out(new unsigned char[totalSize]);
                    for (size_t i = 0; i < totalSize; ++i) {
                        out[i] = boost::numeric_cast<unsigned char>(boolPtr[i]);
                    }

                    element.write(out.get());
                }

                void read(const FixedLengthArray<bool>& element, boost::any& any, ArrayDimensions& dims) {
                    karabo::io::ArrayView<bool>& av = *(boost::any_cast < karabo::io::ArrayView<bool> >(&any));
                    bool* b = &av[0];

                    // calculate number of booeanl values to be read.
                    // Note 1: remember that container may be multi-dimensional
                    // Note 2: ArrayView also contains this information but we use function argument to be consistent 
                    //         with other data container as they may not provide this feature
                    unsigned long long totalNumberOfBoolValues = dims.getNumberOfElements();

                    // declare temporary array of unsigned chars
                    boost::shared_array<unsigned char> arrUChars = boost::shared_array<unsigned char>(new unsigned char[totalNumberOfBoolValues]);

                    element.read(arrUChars.get());

                 

                    // after reading from hdf5 file we need to copy the full array of strings
                    // to the original place
                    for (size_t i = 0; i < totalNumberOfBoolValues; i++) {
                        b[i] = arrUChars[i];
                    }
                }

            };

            typedef FLArrayFilterArrayView<signed char> Int8ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<short> Int16ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<int> Int32ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<long long> Int64ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<unsigned char> UInt8ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<unsigned short> UInt16ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<unsigned int> UInt32ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<unsigned long long> UInt64ArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<float> FloatArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<double> DoubleArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<std::string> StringArrayViewFLArrayFilter;
            typedef FLArrayFilterArrayView<bool> BoolArrayViewFLArrayFilter;


        }
    }
}

#endif	
