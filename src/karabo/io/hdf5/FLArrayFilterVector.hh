/*
 * $Id: FLArrayFilterVector.hh 5260 2012-02-26 22:13:16Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_HDF5_FLARRAYFILTERVECTOR_HH
#define	KARABO_IO_HDF5_FLARRAYFILTERVECTOR_HH

#include "FLArrayFilter.hh"
#include "DataTypes.hh"
#include "FixedLengthArray.hh"
#include "TypeTraits.hh"
#include "../ioProfiler.hh"

/**
 * The main European XFEL namespace
 */
namespace karabo {

    /**
     * Namespace for package io
     */
    namespace io {

        namespace hdf5 {

            template<typename T,
                    template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector >
                    class FLArrayFilterVector :
                    public FLArrayFilter<T>, public DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterVector, typeid (CONT<T>).name(), "1.0")

                FLArrayFilterVector() {
                }

                virtual ~FLArrayFilterVector() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const CONT<T>& vec = boost::any_cast<CONT<T> > (any);
                    return ArrayDimensions(vec.size());
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<T > ();
                }

                void write(const FixedLengthArray<T>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const CONT<T>& vec = boost::any_cast< CONT<T> > (any);
                    element.write(&vec[0]);
                }

                void read(const FixedLengthArray<T>& element, boost::any& any, ArrayDimensions& dims) {
                    CONT<T>& vec = *(boost::any_cast<CONT<T> >(&any));
                    element.read(&vec[0]);
                }
            };

            template<>
            class FLArrayFilterVector<std::string, std::vector > :
            public karabo::io::hdf5::FLArrayFilter<std::string>, public karabo::io::hdf5::DataTypes {
            public:

                KARABO_CLASSINFO(FLArrayFilterVector, typeid (std::vector<std::string>).name(), "1.0")

                FLArrayFilterVector() {
                }

                virtual ~FLArrayFilterVector() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const std::vector<std::string>& vec = boost::any_cast<std::vector<std::string> > (any);
                    return ArrayDimensions(vec.size());
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<std::string > ();
                }

                void write(const FixedLengthArray<std::string>& element, const boost::any& any, const ArrayDimensions& dims) {
                    const std::vector<std::string>& vec = boost::any_cast< std::vector<std::string> > (any);
                    element.write(&vec[0]);
                }

                void read(const FixedLengthArray<std::string>& element, boost::any& any, ArrayDimensions& dims) {
                    std::vector<std::string>& vec = *(boost::any_cast<std::vector<std::string> >(&any));

                    std::string* str = &vec[0];

                    // calculate number of strings to be read.
                    // Note 1: remember that vector has 1 dim but array in hdf5 may have more
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
                        tracer << "vector<string> vec[" << i << "]: " << vec[i] << std::endl;
                    }
                }
            };

            class BoolDequeFLArrayFilter :
            public karabo::io::hdf5::FLArrayFilter<bool>, public karabo::io::hdf5::DataTypes {
            public:

                KARABO_CLASSINFO(BoolDequeFLArrayFilter, typeid (std::deque<bool>).name(), "1.0")

                BoolDequeFLArrayFilter() {
                }

                virtual ~BoolDequeFLArrayFilter() {
                }

                ArrayDimensions getDims(const boost::any& any) {
                    const std::deque<bool>& vec = boost::any_cast < std::deque<bool> > (any);
                    return ArrayDimensions(vec.size());
                }

                std::string getElementClassId() {
                    return ArrayTypeTraits::classId<bool > ();
                }

                void write(const FixedLengthArray<bool>& element, const boost::any& any, const ArrayDimensions& dims) {
                    // Bool values are special in two aspects:
                    // 1. since vector<bool> is broken we use deque<bool> which does not guarantee continues memory layout
                    // 2. Hdf5 does not support bool type so we need to use unsigned char in HDF5 file                                    

                    const std::deque<bool>& deq = boost::any_cast < std::deque<bool> > (any);

                    unsigned long long totalNumberOfBoolValues = dims.getNumberOfElements();
                    tracer << "deque[0] " << deq[0] << " [1]: " << deq[1] << std::endl;
                    tracer << "totalNumberOfElements: " << totalNumberOfBoolValues << std::endl;

                    boost::shared_array<unsigned char> convertedToArray(new unsigned char[totalNumberOfBoolValues]);
                    for (size_t i = 0; i < totalNumberOfBoolValues; ++i) {
                        convertedToArray[i] = boost::numeric_cast<unsigned char>(deq[i]);
                    }

                    element.write(convertedToArray.get());
                }

                void read(const FixedLengthArray<bool>& element, boost::any& any, ArrayDimensions& dims) {
                    std::deque<bool>& deq = *(boost::any_cast < std::deque<bool> >(&any));


                    // calculate number of strings to be read.
                    // Note 1: remember that vector has 1 dim but array in hdf5 may have more
                    // Note 2: ArrayView also contains this information but we use function argument to be consistent 
                    //         with other data container as they may not provide this feature

                    unsigned long long totalNumberOfBoolValues = dims.getNumberOfElements();

                    // declare temporary array of unsigned chars
                    boost::shared_array<unsigned char> arrUChars = boost::shared_array<unsigned char>(new unsigned char[totalNumberOfBoolValues]);

                    element.read(arrUChars.get());

                    // after reading from hdf5 file we need to copy the full array of unsigned chars to 
                    // to the original place converting on the fly from uchar to bool
                    deq.resize(totalNumberOfBoolValues);
                    for (size_t i = 0; i < totalNumberOfBoolValues; i++) {
                        deq[i] = arrUChars[i];
                        tracer << "after read [" << i << "] = " << deq[i] << std::endl;
                    }
                }
            };

            typedef FLArrayFilterVector<signed char> Int8VectorFLArrayFilter;
            typedef FLArrayFilterVector<short> Int16VectorFLArrayFilter;
            typedef FLArrayFilterVector<int> Int32VectorFLArrayFilter;
            typedef FLArrayFilterVector<long long > Int64VectorFLArrayFilter;
            typedef FLArrayFilterVector<unsigned char> UInt8VectorFLArrayFilter;
            typedef FLArrayFilterVector<unsigned short> UInt16VectorFLArrayFilter;
            typedef FLArrayFilterVector<unsigned int> UInt32VectorFLArrayFilter;
            typedef FLArrayFilterVector<unsigned long long> UInt64VectorFLArrayFilter;
            typedef FLArrayFilterVector<float> FloatVectorFLArrayFilter;
            typedef FLArrayFilterVector<double> DoubleVectorFLArrayFilter;
            typedef FLArrayFilterVector<std::string> StringVectorFLArrayFilter;
            //typedef FLArrayFilterVector<bool, std::deque> BoolDequeFLArrayFilter;


        }
    }
}

#endif	/* KARABO_IO_HDF5_FLARRAYFILTERVECTOR_HH */
