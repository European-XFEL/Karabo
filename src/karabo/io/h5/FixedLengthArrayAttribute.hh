/*
 * $Id: FixedLengthArrayAttribute.hh 5491 2012-03-09 17:27:25Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH
#define	KARABO_IO_FIXEDLENGTHARRAYATTRIBUTE_HH


#include <vector>
#include <string>

#include "Attribute.hh"
#include "TypeTraits.hh"
#include <karabo/util/Hash.hh>
#include <karabo/util/ToLiteral.hh>
#include <karabo/util/FromTypeInfo.hh>
#include <karabo/util/VectorElement.hh>

#include <karabo/util/util.hh>
//#include "../ioProfiler.hh"



namespace karabo {

    namespace io {

        namespace h5 {

            template<typename T>
            class FixedLengthArrayAttribute : public Attribute {
            public:

                KARABO_CLASSINFO(FixedLengthArrayAttribute, "VECTOR_" + karabo::util::ToType<karabo::util::ToLiteral>::to(karabo::util::FromType<karabo::util::FromTypeInfo>::from(typeid (T))), "2.0")          

                FixedLengthArrayAttribute(const karabo::util::Hash& input) : Attribute(input) {
                    m_dims = karabo::util::Dims(input.get<std::vector<unsigned long long> >("dims"));
                }

                virtual ~FixedLengthArrayAttribute() {
                }

                static void expectedParameters(karabo::util::Schema& expected) {

                    karabo::util::VECTOR_UINT64_ELEMENT(expected)
                            .key("dims")
                            .displayedName("Dimensions")
                            .description("Array dimensions.")
                            .assignmentOptional().noDefaultValue()
                            .init()
                            .commit();
                }

                void create(hsize_t chunkSize) {
                    
                }

 
                void write(const karabo::util::Hash& data, hsize_t recordId) {
                    //
                    //                    try {
                    //                        selectFileRecord(recordId);
                    //                        karabo::util::Hash::const_iterator it = data.find(m_key);
                    //                        if (it == data.end()) { // TODO: do we need here to check if iterator is ok, is this performance issue
                    //                            throw KARABO_PARAMETER_EXCEPTION("Invalid key in the Hash");
                    //                        }
                    //                        const boost::any& any = data.getAny(it);
                    //                        if (!m_filter) {
                    //                            tracer << "creating a filter for FixedLengthArray " << any.type().name() << std::endl;
                    //                            // this uses factory mechanism combined with rtti.
                    //                            m_filter = FLArrayFilter<T>::createDefault(any.type().name());
                    //                        }
                    //                        //tracer << "about to write " << any.type().name() << std::endl;                        
                    //                        m_filter->write(*this, any, m_dims);
                    //                        //tracer << m_filter << std::endl;
                    //                        //tracer << "after write " << any.type().name() << std::endl;                        
                    //                    } catch (...) {
                    //                        //tracer << "exception caught" << std::endl;
                    //                        KARABO_RETHROW;
                    //                    }
                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //                template<class U>
                //                inline void write(const U* ptr) const {
                //                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), m_memoryDataSpace, m_fileDataSpace);
                //                }
                //
                //                /*
                //                 * This function is not available via RecordElement interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                void writeBuffer(const U* ptr, size_t len) const {
                //                    H5::DataSpace mds = this->getBufferDataSpace(len);
                //                    m_dataSet.write(ptr, ArrayTypes::getHdf5NativeType<U > (m_dims), mds, m_fileDataSpace);
                //                }
                //

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                    //                    try {
                    //                        selectFileRecord(recordId, len);
                    //                        karabo::util::Hash::const_iterator it = data.find(m_key);
                    //                        const boost::any& any = data.getAny(it);
                    //                        if (!m_bufferFilter) {
                    //                            //std::cout << "creating a filter" << std::endl;
                    //                            m_bufferFilter = FLArrayFilterBuffer<T>::createDefault(any.type().name());
                    //                        }
                    //                        m_bufferFilter->write(*this, any, m_dims, len);
                    //                    } catch (...) {
                    //                        KARABO_RETHROW
                    //                    }
                    //
                }
 

            protected:

//                H5::DataSet m_dataSet;
//                H5::DataSpace m_memoryDataSpace;
//                H5::DataSpace m_fileDataSpace;
//                boost::shared_ptr<H5::DSetCreatPropList> m_dataSetProperties;
//                hsize_t m_chunkSize;




                hsize_t m_size; // size of the array is fixed
                karabo::util::Dims m_dims;

                hid_t m_dataAccessPropListId;

                //                boost::shared_ptr<FLArrayFilter<T> > m_filter;
                //                boost::shared_ptr<FLArrayFilterBuffer<T> > m_bufferFilter;


            };

            // typedefs
            typedef FixedLengthArrayAttribute<signed char> Int8ArrayAttribute;
            typedef FixedLengthArrayAttribute<short> Int16ArrayAttribute;
            typedef FixedLengthArrayAttribute<int> Int32ArrayAttribute;
            typedef FixedLengthArrayAttribute<long long > Int64ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned char> UInt8ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned short> UInt16ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned int> UInt32ArrayAttribute;
            typedef FixedLengthArrayAttribute<unsigned long long > UInt64ArrayAttribute;
            typedef FixedLengthArrayAttribute<double> DoubleArrayAttribute;
            typedef FixedLengthArrayAttribute<float> FloatArrayAttribute;
            typedef FixedLengthArrayAttribute<std::string> StringArrayAttribute;
            typedef FixedLengthArrayAttribute<bool> BoolArrayAttribute;

        }
    }
}

#endif	
