/*
 * $Id: ScalarFilterBuffer.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef EXFEL_IO_HDF5_SCALARFILTERBUFFER_HH
#define	EXFEL_IO_HDF5_SCALARFILTERBUFFER_HH

#include "ScalarFilter.hh"
#include "TypeTraits.hh"
#include "Scalar.hh"
#include "../ioProfiler.hh"

/**
 * The main European XFEL namespace
 */
namespace exfel {

    /**
     * Namespace for package io
     */
    namespace io {
        namespace hdf5 {


            // Here we pre-define filters for writing and reading Array types (InputData)
            // and discovering Array properties (InputDataType)

            template<typename T, template <typename ELEM> class CONT = exfel::io::ArrayView >
            class ScalarFilterBufferArrayView :
            public exfel::io::hdf5::ScalarFilter<T> {
            public:

                // important!
                // registration key is the rtti name

                EXFEL_CLASSINFO(ScalarFilterBufferArrayView, typeid (CONT<T>).name(), "1.0")

                ScalarFilterBufferArrayView() {
                }

                virtual ~ScalarFilterBufferArrayView() {
                }

                void write(const Scalar<T>& element, const boost::any& any, const size_t len) {
                    EXFEL_PROFILER_SCALARFILTERBUFFER1
                    EXFEL_PROFILER_START_SCALARFILTERBUFFER1("getPointer")
                    const CONT<T>& av = *(boost::any_cast<CONT<T> >(&any));
                    const T* rawPtr = &av[0];
                    EXFEL_PROFILER_STOP_SCALARFILTERBUFFER1
                    EXFEL_PROFILER_START_SCALARFILTERBUFFER1("writeBuffer")
                    element.writeBuffer(rawPtr, len);
                    EXFEL_PROFILER_STOP_SCALARFILTERBUFFER1
                    EXFEL_PROFILER_REPORT_SCALARFILTERBUFFER1("getPointer")
                    EXFEL_PROFILER_REPORT_SCALARFILTERBUFFER1("writeBuffer")
                    
                }

                void read(const Scalar<T>& element, boost::any& any, size_t len) {
                    CONT<T>& av = *(boost::any_cast<CONT<T> >(&any));
                    T* rawPtr = &av[0];                    
                    element.readBuffer(rawPtr, len);
                }

            };

            template<>
            class ScalarFilterBufferArrayView<std::string, exfel::io::ArrayView> :
            public exfel::io::hdf5::ScalarFilter<std::string> {
            public:

                EXFEL_CLASSINFO(ScalarFilterBufferArrayView, typeid (exfel::io::ArrayView<std::string>).name(), "1.0")

                ScalarFilterBufferArrayView() {
                }

                virtual ~ScalarFilterBufferArrayView() {
                }

                void write(const Scalar<std::string>& element, const boost::any& any, const size_t len) {
                    const exfel::io::ArrayView<std::string>& av = *(boost::any_cast<exfel::io::ArrayView<std::string> >(&any));
                    const std::string* strPtr = &av[0];
                    element.writeBuffer(strPtr, len);
                }

                void read(const Scalar<std::string>& element, boost::any& any, size_t len) {
                    exfel::io::ArrayView<std::string>& av = *(boost::any_cast<exfel::io::ArrayView<std::string> >(&any));
                    std::string* str = &av[0];
                    // declare temporary array of char pointers
                    boost::shared_array<char*> arrChar = boost::shared_array<char*>(new char*[len]);
                    // here we need to use second variant of read method as reading directly 
                    // to the memory provided by strings cannot be utilized
                    element.readBuffer(arrChar.get(), std::string(), len);

                    // after reading from hdf5 file we need to copy the full array of strings
                    // to the original place
                    for (size_t i = 0; i < len; i++) {
                        str[i] = arrChar[i];
                    }
                }

            };

            template<>
            class ScalarFilterBufferArrayView<bool, exfel::io::ArrayView> :
            public exfel::io::hdf5::ScalarFilter<bool> {
            public:

                EXFEL_CLASSINFO(ScalarFilterBufferArrayView, typeid (exfel::io::ArrayView<bool>).name(), "1.0")

                ScalarFilterBufferArrayView() {
                }

                virtual ~ScalarFilterBufferArrayView() {
                }

                void write(const Scalar<bool>& element, const boost::any& any, const size_t len) {
                    const exfel::io::ArrayView<bool>& av = *(boost::any_cast < exfel::io::ArrayView<bool> >(&any));
                    const bool* boolPtr = &av[0];

                    boost::shared_array<unsigned char> out(new unsigned char[len]);
                    for (size_t i = 0; i < len; ++i) {
                        out[i] = boost::numeric_cast<unsigned char>(boolPtr[i]);
                    }

                    element.writeBuffer(out.get(), len);
                }

                void read(const Scalar<bool>& element, boost::any& any, size_t len) {
                    exfel::io::ArrayView<bool>& av = *(boost::any_cast < exfel::io::ArrayView<bool> >(&any));
                    bool* str = &av[0];

                    // declare temporary array of char pointers
                    boost::shared_array<char*> arrChar = boost::shared_array<char*>(new char*[len]);
                    // here we need to use second variant of read method as reading directly 
                    // to the memory provided by strings cannot be utilized
                    element.readBuffer(arrChar.get(), bool(), len);

                    // after reading from hdf5 file we need to copy the full array of strings
                    // to the original place
                    for (size_t i = 0; i < len; i++) {
                        str[i] = arrChar[i];
                    }
                }

            };


            template class ScalarFilterBufferArrayView<signed char>;
            template class ScalarFilterBufferArrayView<short>;
            template class ScalarFilterBufferArrayView<int>;
            template class ScalarFilterBufferArrayView<long long>;
            template class ScalarFilterBufferArrayView<unsigned char>;
            template class ScalarFilterBufferArrayView<unsigned short>;
            template class ScalarFilterBufferArrayView<unsigned int>;
            template class ScalarFilterBufferArrayView<unsigned long long>;
            template class ScalarFilterBufferArrayView<float>;
            template class ScalarFilterBufferArrayView<double>;
            template class ScalarFilterBufferArrayView<std::string>;
            template class ScalarFilterBufferArrayView<bool>;

            typedef ScalarFilterBufferArrayView<signed char> Int8ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<short> Int16ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<int> Int32ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<long long> Int64ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<unsigned char> UInt8ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<unsigned short> UInt16ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<unsigned int> UInt32ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<unsigned long long> UInt64ArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<float> FloatArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<double> DoubleArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<std::string> StringArrayViewScalarFilter;
            typedef ScalarFilterBufferArrayView<bool> BoolArrayViewScalarFilter;

            template<typename T >
            class ScalarFilterBufferVector :
            public exfel::io::hdf5::ScalarFilter<T> {
            public:

                // important!
                // registration key is the rtti name

                EXFEL_CLASSINFO(ScalarFilterBufferVector, typeid (std::vector<T>).name(), "1.0")

                ScalarFilterBufferVector() {
                }

                virtual ~ScalarFilterBufferVector() {
                }

                void write(const Scalar<T>& element, const boost::any& any, const size_t len) {
                    const std::vector<T>& av = *(boost::any_cast<std::vector<T> >(&any));
                    const T* rawPtr = &av[0];
                    element.writeBuffer(rawPtr, len);
                }

                void read(const Scalar<T>& element, boost::any& any, size_t len) {
                    std::vector<T>& av = *(boost::any_cast<std::vector<T> >(&any));
                    T* rawPtr = &av[0];
                    //std::cout << "reading buffer data" << std::endl;
                    element.readBuffer(rawPtr, len);
                }

            };

            template<>
            class ScalarFilterBufferVector<std::string> :
            public exfel::io::hdf5::ScalarFilter<std::string> {
            public:

                // important!
                // registration key is the rtti name

                EXFEL_CLASSINFO(ScalarFilterBufferVector, typeid (std::vector<std::string>).name(), "1.0")

                ScalarFilterBufferVector() {
                }

                virtual ~ScalarFilterBufferVector() {
                }

                void write(const Scalar<std::string>& element, const boost::any& any, const size_t len) {
//                    const std::vector<std::string>& av = *(boost::any_cast<std::vector<std::string> >(&any));
//                    const std::string* rawPtr = &av[0];
//                    element.writeBuffer(rawPtr, len);
                }

                void read(const Scalar<std::string>& element, boost::any& any, size_t len) {
//                    std::vector<std::string>& av = *(boost::any_cast<std::vector<std::string> >(&any));
//                    std::string* rawPtr = &av[0];
//                    //std::cout << "reading buffer data" << std::endl;
//                    element.readBuffer(rawPtr, len);
                }

            };


                        template<>
            class ScalarFilterBufferVector<bool> :
            public exfel::io::hdf5::ScalarFilter<bool> {
            public:

                // important!
                // registration key is the rtti name

                EXFEL_CLASSINFO(ScalarFilterBufferVector, typeid (std::vector<bool>).name(), "1.0")

                ScalarFilterBufferVector() {
                }

                virtual ~ScalarFilterBufferVector() {
                }

                void write(const Scalar<bool>& element, const boost::any& any, const size_t len) {
//                    const std::vector<bool>& av = *(boost::any_cast<std::vector<bool> >(&any));
//                    const bool* rawPtr = &av[0];
//                    element.writeBuffer(rawPtr, len);
                }

                void read(const Scalar<bool>& element, boost::any& any, size_t len) {
//                    std::vector<bool>& av = *(boost::any_cast<std::vector<bool> >(&any));
//                    bool* rawPtr = &av[0];
//                    //std::cout << "reading buffer data" << std::endl;
//                    element.readBuffer(rawPtr, len);
                }

            };


            template class ScalarFilterBufferVector<signed char>;
            template class ScalarFilterBufferVector<short>;
            template class ScalarFilterBufferVector<int>;
            template class ScalarFilterBufferVector<long long>;
            template class ScalarFilterBufferVector<unsigned char>;
            template class ScalarFilterBufferVector<unsigned short>;
            template class ScalarFilterBufferVector<unsigned int>;
            template class ScalarFilterBufferVector<unsigned long long>;
            template class ScalarFilterBufferVector<float>;
            template class ScalarFilterBufferVector<double>;
            //            template class ScalarFilterBufferVector<std::string>;
            //            template class ScalarFilterBufferVector<bool>;

            typedef ScalarFilterBufferVector<signed char> Int8VectorScalarFilter;
            typedef ScalarFilterBufferVector<short> Int16VectorScalarFilter;
            typedef ScalarFilterBufferVector<int> Int32VectorScalarFilter;
            typedef ScalarFilterBufferVector<long long> Int64VectorScalarFilter;
            typedef ScalarFilterBufferVector<unsigned char> UInt8VectorScalarFilter;
            typedef ScalarFilterBufferVector<unsigned short> UInt16VectorScalarFilter;
            typedef ScalarFilterBufferVector<unsigned int> UInt32VectorScalarFilter;
            typedef ScalarFilterBufferVector<unsigned long long> UInt64VectorScalarFilter;
            typedef ScalarFilterBufferVector<float> FloatVectorScalarFilter;
            typedef ScalarFilterBufferVector<double> DoubleVectorScalarFilter;
            //typedef ScalarFilterBufferVector<std::string> StringVectorScalarFilter;
            //typedef ScalarFilterBufferVector<bool> BoolVectorScalarFilter;






            //      template<typename T,
            //              template <typename ELEM, typename = std::allocator<ELEM> > class CONT = std::vector >
            //              class InputDataImplVector :
            //              public exfel::io::hdf5::InputData<T>, public exfel::io::hdf5::InputDataType {
            //      public:
            //
            //        EXFEL_CLASSINFO(InputDataImplVector, typeid (CONT<T>).name(), "1.0")
            //
            //        InputDataImplVector() {
            //        }
            //
            //        virtual ~InputDataImplVector() {
            //        }
            //
            //        ArrayDimensions getDims(const boost::any& any) {
            //          const CONT<T>& vec = boost::any_cast<CONT<T> > (any);
            //          return ArrayDimensions(1, vec.size());
            //        }
            //
            //        std::string getElementClassId() {
            //          return ArrayTypeTraits::classId<T > ();
            //        }
            //
            //        void write(const Scalar<T>& element, const boost::any& any, const ArrayDimensions len) {
            //          const CONT<T>& vec = boost::any_cast< CONT<T> > (any);
            //          element.write(&vec[0]);
            //        }
            //
            //        void read(const Scalar<T>& element, boost::any& any, ArrayDimensions len) {
            //          CONT<T>& vec = *(boost::any_cast<CONT<T> >(&any));
            //          element.read(&vec[0]);
            //        }
            //      };
            //
            //      template<>
            //      class InputDataImplVector<std::string, std::vector > :
            //      public exfel::io::hdf5::InputData<std::string>, public exfel::io::hdf5::InputDataType {
            //      public:
            //
            //        EXFEL_CLASSINFO(InputDataImplVector, typeid (std::vector<std::string>).name(), "1.0")
            //
            //        InputDataImplVector() {
            //        }
            //
            //        virtual ~InputDataImplVector() {
            //        }
            //
            //        ArrayDimensions getDims(const boost::any& any) {
            //          const std::vector<std::string>& vec = boost::any_cast<std::vector<std::string> > (any);
            //          return ArrayDimensions(1, vec.size());
            //        }
            //
            //        std::string getElementClassId() {
            //          return ArrayTypeTraits::classId<std::string > ();
            //        }
            //
            //        void write(const Scalar<std::string>& element, const boost::any& any, const ArrayDimensions len) {
            //          const std::vector<std::string>& vec = boost::any_cast< std::vector<std::string> > (any);
            //          element.write(&vec[0]);
            //        }
            //
            //        void read(const Scalar<std::string>& element, boost::any& any, ArrayDimensions len) {
            //          std::vector<std::string>& vec = *(boost::any_cast<std::vector<std::string> >(&any));
            //
            //          std::string* str = &vec[0];
            //
            //          // calculate number of strings to be read.
            //          // Note 1: remember that vector has 1 dim but array in hdf5 may have more
            //          // Note 2: ArrayView also contains this information but we use function argument to be consistent 
            //          //         with other data container as they may not provide this feature
            //
            //          unsigned long long totalNumberOfStrings = dims.getNumberOfElements();
            //
            //          // declare temporary array of char pointers
            //          boost::shared_array<char*> arrChar = boost::shared_array<char*>(new char*[totalNumberOfStrings]);
            //          // here we need to use second variant of read method as reading directly 
            //          // to the memory provided by strings cannot be utilized
            //          element.read(arrChar.get(), std::string());
            //
            //          // after reading from hdf5 file we need to copy the full array of strings
            //          // to the original place
            //          for (size_t i = 0; i < totalNumberOfStrings; i++) {
            //            str[i] = arrChar[i];
            //          }
            //        }
            //      };
            //
            //
            //      template class InputDataImplVector<signed char>;
            //      template class InputDataImplVector<short>;
            //      template class InputDataImplVector<int>;
            //      template class InputDataImplVector<long long>;
            //      template class InputDataImplVector<unsigned char>;
            //      template class InputDataImplVector<unsigned short>;
            //      template class InputDataImplVector<unsigned int>;
            //      template class InputDataImplVector<unsigned long long>;
            //      template class InputDataImplVector<float>;
            //      template class InputDataImplVector<double>;
            //      template class InputDataImplVector<std::string>;
            //      // std::vector<bool> is not supported because it is broken
            //
            //      typedef InputDataImplVector<signed char> Int8VectorInputData;
            //      typedef InputDataImplVector<short> Int16VectorInputData;
            //      typedef InputDataImplVector<int> Int32VectorInputData;
            //      typedef InputDataImplVector<long long > Int64VectorInputData;
            //      typedef InputDataImplVector<unsigned char> UInt8VectorInputData;
            //      typedef InputDataImplVector<unsigned short> UInt16VectorInputData;
            //      typedef InputDataImplVector<unsigned int> UInt32VectorInputData;
            //      typedef InputDataImplVector<unsigned long long> UInt64VectorInputData;
            //      typedef InputDataImplVector<float> FloatVectorInputData;
            //      typedef InputDataImplVector<double> DoubleVectorInputData;
            //      typedef InputDataImplVector<std::string> StringVectorInputData;
            //
            //      template<typename T>
            //      class InputDataImplDeque :
            //      public exfel::io::hdf5::InputData<T>, public exfel::io::hdf5::InputDataType {
            //      public:
            //
            //        EXFEL_CLASSINFO(InputDataImplDeque, typeid (std::deque<T>).name(), "1.0")
            //
            //        InputDataImplDeque() {
            //        }
            //
            //        virtual ~InputDataImplDeque() {
            //        }
            //
            //        ArrayDimensions getDims(const boost::any& any) {
            //          const std::deque<T>& deq = boost::any_cast<std::deque<T> > (any);
            //          return ArrayDimensions(1, deq.size());
            //        }
            //
            //        std::string getElementClassId() {
            //          return ArrayTypeTraits::classId<T > ();
            //        }
            //
            //        void write(const Scalar<T>& element, const boost::any& any, const ArrayDimensions len) {
            //          const std::deque<T>& deq = boost::any_cast< std::deque<T> > (any);
            //          unsigned long long totalNumberOfElements = dims.getNumberOfElements();
            //
            //          boost::shared_array<T> sa(new T[totalNumberOfElements]);
            //          for (size_t i = 0; i < totalNumberOfElements; ++i) {
            //            sa[i] = deq[i];
            //          }
            //          element.write(sa.get());
            //        }
            //
            //        void read(const Scalar<T>& element, boost::any& any, ArrayDimensions len) {
            //          std::deque<T>& deq = *(boost::any_cast<std::deque<T> >(&any));
            //          unsigned long long totalNumberOfElements = dims.getNumberOfElements();
            //          boost::shared_array<T> sa(new T[totalNumberOfElements]);
            //          element.read(sa.get());
            //          for (size_t i = 0; i < totalNumberOfElements; ++i) {
            //            deq[i] = sa[i];
            //          }
            //        }
            //      };
            //
            //
            //
            //      template class InputDataImplDeque<signed char>;
            //      template class InputDataImplDeque<short>;
            //      template class InputDataImplDeque<int>;
            //      template class InputDataImplDeque<long long>;
            //      template class InputDataImplDeque<unsigned char>;
            //      template class InputDataImplDeque<unsigned short>;
            //      template class InputDataImplDeque<unsigned int>;
            //      template class InputDataImplDeque<unsigned long long>;
            //      template class InputDataImplDeque<float>;
            //      template class InputDataImplDeque<double>;
            //      template class InputDataImplDeque<std::string>;
            //      template class InputDataImplDeque<bool>;
            //
            //      typedef InputDataImplDeque<signed char> Int8DequeInputData;
            //      typedef InputDataImplDeque<short> Int16DequeInputData;
            //      typedef InputDataImplDeque<int> Int32DequeInputData;
            //      typedef InputDataImplDeque<long long > Int64DequeInputData;
            //      typedef InputDataImplDeque<unsigned char> UInt8DequeInputData;
            //      typedef InputDataImplDeque<unsigned short> UInt16DequeInputData;
            //      typedef InputDataImplDeque<unsigned int> UInt32DequeInputData;
            //      typedef InputDataImplDeque<unsigned long long> UInt64DequeInputData;
            //      typedef InputDataImplDeque<float> FloatDequeInputData;
            //      typedef InputDataImplDeque<double> DoubleDequeInputData;
            //      typedef InputDataImplDeque<std::string> StringDequeInputData;
            //      typedef InputDataImplDeque<bool> BoolDequeInputData;
            //
            //      template<typename T>
            //      class InputDataImplRawPointer :
            //      public exfel::io::hdf5::InputData<T> {
            //      public:
            //
            //        EXFEL_CLASSINFO(InputDataImplRawPointer, typeid (T*).name(), "1.0")
            //
            //        InputDataImplRawPointer() {
            //        }
            //
            //        virtual ~InputDataImplRawPointer() {
            //        }
            //
            //        void write(const Scalar<T>& element, const boost::any& any, const ArrayDimensions len) {
            //          const T* rawPtr = boost::any_cast<T* >(any);
            //          element.write(rawPtr);
            //        }
            //
            //        void read(const Scalar<T>& element, boost::any& any, ArrayDimensions len) {
            //          T* rawPtr = boost::any_cast<T*>(any);
            //          element.read(rawPtr);
            //        }
            //
            //      };
            //
            //      template class InputDataImplRawPointer<signed char>;
            //      template class InputDataImplRawPointer<short>;
            //      template class InputDataImplRawPointer<int>;
            //      template class InputDataImplRawPointer<long long>;
            //      template class InputDataImplRawPointer<unsigned char>;
            //      template class InputDataImplRawPointer<unsigned short>;
            //      template class InputDataImplRawPointer<unsigned int>;
            //      template class InputDataImplRawPointer<unsigned long long>;
            //      template class InputDataImplRawPointer<float>;
            //      template class InputDataImplRawPointer<double>;
            //      template class InputDataImplRawPointer<std::string>;
            //      template class InputDataImplRawPointer<bool>;
            //
            //      typedef InputDataImplRawPointer<signed char> Int8RawPointerInputData;
            //      typedef InputDataImplRawPointer<short> Int16RawPointerInputData;
            //      typedef InputDataImplRawPointer<int> Int32RawPointerInputData;
            //      typedef InputDataImplRawPointer<long long > Int64RawPointerInputData;
            //      typedef InputDataImplRawPointer<unsigned char> UInt8RawPointerInputData;
            //      typedef InputDataImplRawPointer<unsigned short> UInt16RawPointerInputData;
            //      typedef InputDataImplRawPointer<unsigned int> UInt32RawPointerInputData;
            //      typedef InputDataImplRawPointer<unsigned long long> UInt64RawPointerInputData;
            //      typedef InputDataImplRawPointer<float> FloatRawPointerInputData;
            //      typedef InputDataImplRawPointer<double> DoubleRawPointerInputData;
            //      typedef InputDataImplRawPointer<std::string> StringRawPointerInputData;
            //      typedef InputDataImplRawPointer<bool> BoolRawPointerInputData;
            //
            //
            //      // T - type of RecordElement
            //      // U - type of IN/OUT vector
            //
            //      template<typename T, typename U >
            //      class InputDataImplVectorTypeConverter :
            //      public exfel::io::hdf5::InputData<T> {
            //      public:
            //
            //        EXFEL_CLASSINFO(InputDataImplVectorTypeConverter, typeid (std::vector<U>).name(), "1.0")
            //
            //        InputDataImplVectorTypeConverter() {
            //        }
            //
            //        virtual ~InputDataImplVectorTypeConverter() {
            //        }
            //
            //        void write(const Scalar<T>& element, const boost::any& any, const ArrayDimensions len) {
            //          const std::vector<U>& vec = boost::any_cast< std::vector<U> > (any);
            //          element.write(&vec[0]);
            //        }
            //
            //        void read(const Scalar<T>& element, boost::any& any, ArrayDimensions len) {
            //          std::vector<U>& vec = *(boost::any_cast<std::vector<U> >(&any));
            //          element.read(&vec[0]);
            //        }
            //      };
            //
            //      template class InputDataImplVectorTypeConverter<float, double >;
            //      template class InputDataImplVectorTypeConverter<float, int >;
            //      typedef InputDataImplVectorTypeConverter<float, double > FloatDoubleVectorConverter;
            //      typedef InputDataImplVectorTypeConverter<float, int > FloatInt32VectorConverter;
            //


        }
    }
}

#endif	/* EXFEL_IO_HDF5_INPUTDATAIMPL_HH */
