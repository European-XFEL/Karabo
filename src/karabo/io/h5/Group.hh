/*
 * $Id$
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
 */


#ifndef KARABO_IO_H5_GROUP_HH
#define	KARABO_IO_H5_GROUP_HH

#include <string>

#include "Element.hh"
#include "TypeTraits.hh"
#include <karabo/util/util.hh>
#include "ErrorHandler.hh"

//#include "ScalarFilter.hh"


//#include <karabo/util/Time.hh>
//#include "../ioProfiler.hh"

namespace karabo {

    namespace io {

        namespace h5 {

            class Group : public karabo::io::h5::Element {
            public:

                KARABO_CLASSINFO(Group, "Group", "1.0")

                static void expectedParameters(karabo::util::Schema& expected);

                Group(const karabo::util::Hash& input) : Element(input) {

                }

                virtual ~Group() {
                }

                void create(hsize_t chunkSize);

                void write(const karabo::util::Hash& data, hsize_t recordId) {
                }

                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                }
                void open(hid_t group);
                //
                //                void write(const karabo::util::Hash& data, hsize_t recordId) {
                //
                //                    //here we do not use filters, for performance reason (?)
                //                    karabo::util::Hash::const_iterator it = data.find(m_key);
                //                    if (it == data.end()) { // TODO: do we need here to check if iterator is ok, is this performance issue
                //                        throw KARABO_PARAMETER_EXCEPTION("Invalid key in the Hash");
                //                    }
                //                    try {
                //                        selectFileRecord(recordId);
                //                        ScalarWriter<T>::write(data.get<const T > (it), m_dataSet, m_memoryDataSpace, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                void write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //
                //                    KARABO_PROFILER_SCALAR1
                //
                //                    try {
                //                        KARABO_PROFILER_START_SCALAR1("select")
                //                        selectFileRecord(recordId, len);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_START_SCALAR1("find")
                //                        karabo::util::Hash::const_iterator it = data.find(m_key);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_START_SCALAR1("any")
                //                        const boost::any& any = data.getAny(it);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //
                //                        if (!m_filter) {
                //                            tracer << "creating a filter" << std::endl;
                //                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                //                        }
                //                        KARABO_PROFILER_START_SCALAR1("filter")
                //                        m_filter->write(*this, any, len);
                //                        KARABO_PROFILER_STOP_SCALAR1
                //                        KARABO_PROFILER_REPORT_SCALAR1("select")
                //                        KARABO_PROFILER_REPORT_SCALAR1("find")
                //                        KARABO_PROFILER_REPORT_SCALAR1("any")
                //                        KARABO_PROFILER_REPORT_SCALAR1("filter")
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only
                //                 */
                //                template<class U>
                //                void writeBuffer(const U* ptr, size_t len) const {
                //                    H5::DataSpace mds = this->getBufferDataSpace(len);
                //                    m_dataSet.write(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only
                //                 */
                //
                //                template<class U>
                //                inline void readBuffer(U* ptr, size_t len) const {
                //                    try {
                //                        H5::DataSpace mds = getBufferDataSpace(len);
                //                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<U > (), mds, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //                /*
                //                 * This function is not available via Element interface
                //                 * To be used by filters only.
                //                 * Variant with two parameters. Used for strings (and possibly to be used for converters)
                //                 */
                //                template<class U, class V >
                //                inline void readBuffer(U* ptr, const V& p, size_t len) const {
                //                    try {
                //                        H5::DataSpace mds = getBufferDataSpace(len);
                //                        m_dataSet.read(ptr, ScalarTypes::getHdf5NativeType<V > (), mds, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //

                inline void bind(karabo::util::Hash & data) {
                }

                inline void allocate(karabo::util::Hash& buffer, size_t len) {
                }

                inline void read(karabo::util::Hash& data, hsize_t recordId) {
                }
                
                inline void read( hsize_t recordId) {
                }
                
                //
                //                inline void readValue(T& value, hsize_t recordId) {
                //                    try {
                //                        selectFileRecord(recordId);
                //                        ScalarReader<T>::read(value, m_dataSet, m_memoryDataSpace, m_fileDataSpace);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //                }
                //
                //
                //                // buffered reading
                //
                //                void read(karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                //                    try {
                //                        selectFileRecord(recordId, len);
                //                        karabo::util::Hash::iterator it = data.find(m_key);
                //                        boost::any& any = data.getAny(it);
                //                        if (!m_filter) {
                //                            tracer << "creating read filter" << std::endl;
                //                            m_filter = ScalarFilter<T>::createDefault(any.type().name());
                //                        }
                //                        m_filter->read(*this, any, len);
                //                    } catch (...) {
                //                        KARABO_RETHROW
                //                    }
                //
                //                }
                //
                //                inline void readSpecificAttributes(karabo::util::Hash& attributes) {
                //                    attributes.setFromPath(m_key + ".rank", 0);
                //                    attributes.setFromPath(m_key + ".typeCategory", "Scalar");
                //                }
                //
                //
                //            protected:
                //
                //
                //            private:
                //                boost::shared_ptr<ScalarFilter<T> > m_filter;

                hid_t m_group; // this group


            };



        }
    }
}

#endif	
