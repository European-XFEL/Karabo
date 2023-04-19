/*
 * $Id: DatasetAttribute.cc 9852 2013-05-26 21:52:12Z wrona $
 *
 * Author: <krzysztof.wrona@xfel.eu>
 *
 * Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
 */


#include "DatasetAttribute.hh"


using namespace karabo::io::h5;
using namespace karabo::util;

namespace karabo {
    namespace io {
        namespace h5 {


            void DatasetAttribute::write(const karabo::util::Hash& data, hsize_t recordId) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.DatasetAttribute")
                      << "Writing hash data: key=" << m_key << " recordId=" << recordId << " len=1";
                try {
                    if (data.has(m_key, '/')) {
                        extendFileDataspace(recordId, 1);
                        selectFileRecords(recordId, 1);
                        const karabo::util::Element<std::string>& node = data.getAttributes(m_key, '/').getNode("aa");
                        writeNode(node, m_h5obj, m_fileDataSpace);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " key in the hash");
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" +
                                                                  m_h5PathName));
                }
            }


            void DatasetAttribute::write(const karabo::util::Hash& data, hsize_t recordId, hsize_t len) {
                KARABO_LOG_FRAMEWORK_TRACE_C("karabo.io.h5.DatasetAttribute")
                      << "Writing hash data: key=" << m_key << " recordId=" << recordId << " len=" << len;

                try {
                    if (data.has(m_key, '/')) {
                        // OPT1
                        if (m_h5obj < 0) {
                            m_h5obj = H5Dopen2(m_tableGroup, m_h5PathName.c_str(), H5P_DEFAULT);
                        }
                        //
                        extendFileDataspace(recordId, len);
                        selectFileRecords(recordId, len);
                        const karabo::util::Element<std::string>& node = data.getAttributes(m_key, '/').getNode("aa");
                        writeNode(node, len, m_h5obj, m_fileDataSpace);
                    } else {
                        throw KARABO_HDF_IO_EXCEPTION("No " + m_key + " key in the hash");
                    }
                } catch (karabo::util::Exception& e) {
                    KARABO_RETHROW_AS(KARABO_PROPAGATED_EXCEPTION("Cannot write Hash node " + m_key + " to dataset /" +
                                                                  m_h5PathName));
                }
            }


        } // namespace h5
    }     // namespace io
} // namespace karabo
