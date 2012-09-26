/*
 * $Id: FLArrayFilterArrayViewBuffer.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_HDF5_FLARRAYFILTERARRAYVIEWBUFFER_HH
#define	KARABO_IO_HDF5_FLARRAYFILTERARRAYVIEWBUFFER_HH

#include "FLArrayFilterBuffer.hh"
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
            class FLArrayFilterArrayViewBuffer :
            public FLArrayFilterBuffer<T>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayViewBuffer, typeid (karabo::io::ArrayView< CONT<T> >).name(), "1.0")

                FLArrayFilterArrayViewBuffer() {
                }

                virtual ~FLArrayFilterArrayViewBuffer() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const karabo::io::ArrayView< CONT<T> >& av = boost::any_cast< karabo::io::ArrayView<CONT<T> > > (any);
                    return av.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<T > ();
                }

                void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims, size_t len) {
                    const karabo::io::ArrayView<CONT<T> >& av = *(boost::any_cast<karabo::io::ArrayView< CONT<T> > >(&any));
                    //const CONT<T>& cont = av[0];
                    //const T* rawPtr = &cont[0];
                    const T* rawPtr = &av[0][0];                    
                    element.writeBuffer(rawPtr, len);
                }

                void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims, size_t len) {
                    karabo::io::ArrayView<CONT<T> >& av = *(boost::any_cast<karabo::io::ArrayView<CONT<T> > >(&any));
                    CONT<T>& cont = av[0];
                    T* rawPtr = &cont[0];
                    element.readBuffer(rawPtr, len);
                }

            };

            template<>
            class FLArrayFilterArrayViewBuffer<std::string, karabo::io::ArrayView> :
            public FLArrayFilterBuffer<std::string>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayViewBuffer, typeid (karabo::io::ArrayView<std::string>).name(), "1.0")

                FLArrayFilterArrayViewBuffer() {
                }

                virtual ~FLArrayFilterArrayViewBuffer() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const karabo::io::ArrayView<std::string>& arr = boost::any_cast<karabo::io::ArrayView<std::string> > (any);
                    return arr.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<std::string > ();
                }

                void write(const FixedLengthArray<std::string>& element, const boost::any& any, const ArrayDimensions& dims, size_t len) {
                    const karabo::io::ArrayView<std::string>& av = *(boost::any_cast<karabo::io::ArrayView<std::string> >(&any));
                    const std::string* strPtr = &av[0];
                    element.write(strPtr);
                }

                void read(const FixedLengthArray<std::string>& element, boost::any& any, ArrayDimensions& dims, size_t len) {
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
            class FLArrayFilterArrayViewBuffer<bool, karabo::io::ArrayView> :
            public FLArrayFilterBuffer<bool>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterArrayViewBuffer, typeid (karabo::io::ArrayView<bool>).name(), "1.0")

                FLArrayFilterArrayViewBuffer() {
                }

                virtual ~FLArrayFilterArrayViewBuffer() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const karabo::io::ArrayView<bool>& arr = boost::any_cast < karabo::io::ArrayView<bool> > (any);
                    return arr.getDims();
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<bool > ();
                }

                void write(const FixedLengthArray<bool>& element, const boost::any& any, const ArrayDimensions& dims, size_t len) {
                    const karabo::io::ArrayView<bool>& av = *(boost::any_cast < karabo::io::ArrayView<bool> >(&any));
                    const bool* boolPtr = &av[0];
                    unsigned long long totalSize = dims.getNumberOfElements();
                    boost::shared_array<unsigned char> out(new unsigned char[totalSize]);
                    for (size_t i = 0; i < totalSize; ++i) {
                        out[i] = boost::numeric_cast<unsigned char>(boolPtr[i]);
                    }

                    element.write(out.get());
                }

                void read(const FixedLengthArray<bool>& element, boost::any& any, ArrayDimensions& dims, size_t len) {
                    karabo::io::ArrayView<bool>& av = *(boost::any_cast < karabo::io::ArrayView<bool> >(&any));
                    bool* str = &av[0];

                    // calculate number of strings to be read.
                    // Note 1: remember that container may be multi-dimensional
                    // Note 2: ArrayView also contains this information but we use function argument to be consistent 
                    //         with other data container as they may not provide this feature

                    unsigned long long totalNumberOfElements = dims.getNumberOfElements();

                    // declare temporary array of char pointers
                    boost::shared_array<char*> arrChar = boost::shared_array<char*>(new char*[totalNumberOfElements]);
                    // here we need to use second variant of read method as reading directly 
                    // to the memory provided by strings cannot be utilized
                    element.read(arrChar.get(), bool());

                    // after reading from hdf5 file we need to copy the full array of strings
                    // to the original place
                    for (size_t i = 0; i < totalNumberOfElements; i++) {
                        str[i] = arrChar[i];
                    }
                }

            };

            
            typedef FLArrayFilterArrayViewBuffer<signed char> Int8ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<short> Int16ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<int> Int32ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<long long> Int64ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<unsigned char> UInt8ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<unsigned short> UInt16ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<unsigned int> UInt32ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<unsigned long long> UInt64ArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<float> FloatArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<double> DoubleArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<std::string> StringArrayViewFLArrayBufferFilter;
            typedef FLArrayFilterArrayViewBuffer<bool> BoolArrayViewFLArrayBufferFilter;


        }
    }
}

#endif	
